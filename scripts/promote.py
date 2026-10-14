"""Serialize source promotion; freshly verify every accepted function before writing."""
import argparse
import base64
from contextlib import contextmanager
import json
import os
from pathlib import Path
import re

from env import ROOT
from match import check, digest, DEFAULT_TOOLCHAIN
from coff import COFF

STATE = ROOT / "recovery.json"
JOURNAL = ROOT / "work/promotion-journal.json"
QUOTED_INCLUDE = re.compile(rb'^\s*#\s*include\s*"([^"\r\n]+)"')


def atomic_bytes(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".promotion-tmp")
    temporary.write_bytes(content)
    os.replace(temporary, path)


@contextmanager
def lock():
    path = ROOT / "work/promotion.lock"
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as stream:
        stream.seek(0, 2)
        if stream.tell() == 0:
            stream.write(b"0")
            stream.flush()
        stream.seek(0)
        if os.name == "nt":
            import msvcrt
            msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield
        finally:
            stream.seek(0)
            if os.name == "nt":
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def recover():
    if not JOURNAL.exists():
        print("No interrupted promotion.")
        return
    journal = json.loads(JOURNAL.read_text())
    for row in journal:
        path = ROOT / row["path"]
        if path.resolve() != STATE.resolve() and not path.resolve().is_relative_to((ROOT / "src").resolve()):
            raise ValueError("Journal destination outside canonical source/state")
        current = digest(path.read_bytes()) if path.exists() else None
        if current not in {row["before_sha256"], row["after_sha256"]}:
            raise ValueError(f"Concurrent edit after interrupted promotion: {path}")
    for row in journal:
        path = ROOT / row["path"]
        if row["before"] is None:
            path.unlink(missing_ok=True)
        else:
            atomic_bytes(path, base64.b64decode(row["before"]))
    JOURNAL.unlink()
    print("Restored pre-promotion source and state.")


def validate_candidate_object(obj, allowed_symbols):
    coff = COFF(obj)
    functions = {s["name"] for s in coff.symbols.values() if s["section"] > 0 and s["type"] & 0x20}
    if functions != set(allowed_symbols):
        raise ValueError(f"Candidate functions must exactly cover accepted/requested rows: {sorted(functions)}")
    for name in functions:
        coff.function(name)  # require independently bounded /Gy code
    for section in coff.sections:
        if section["flags"] & (0x40 | 0x80) and section["size"]:
            if section["name"] not in {".drectve", ".debug$S", ".debug$T", ".debug$F"}:
                raise ValueError(f"Pilot gate does not accept unreviewed data contributions: {section['name']}")


def _include_directories(flags):
    result = []
    index = 0
    while index < len(flags):
        flag = str(flags[index])
        if flag.lower() == "/i" and index + 1 < len(flags):
            index += 1
            value = str(flags[index]).strip('"')
        elif flag[:2].lower() == "/i" and len(flag) > 2:
            value = flag[2:].strip('"')
        else:
            index += 1
            continue
        path = Path(value)
        if not path.is_absolute():
            path = ROOT / path
        result.append(path.resolve())
        index += 1
    result.append((DEFAULT_TOOLCHAIN / "include").resolve())
    return result


def quoted_repository_headers(source, flags):
    """Snapshot transitive literal-quoted headers resolved in ordinary order.

    Angle-bracket SDK/MFC headers are pinned by the existing toolchain manifest.
    This conservative scanner reads quoted directives regardless of conditional
    compilation and records only resolved files inside this repository.
    """
    repository = ROOT.resolve()
    include_dirs = _include_directories(flags)
    pending = [Path(source).resolve()]
    visited = set()
    headers = {}
    while pending:
        current = pending.pop()
        if current in visited or not current.is_file():
            continue
        visited.add(current)
        for line in current.read_bytes().splitlines():
            match = QUOTED_INCLUDE.match(line)
            if not match:
                continue
            included = os.fsdecode(match.group(1))
            candidates = [current.parent / included, *(directory / included for directory in include_dirs)]
            resolved = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
            if resolved is None or not resolved.is_relative_to(repository):
                continue
            if resolved not in headers:
                data = resolved.read_bytes()
                headers[resolved] = data
                pending.append(resolved)
    return headers


def _assert_headers_unchanged(headers, phase):
    for path, before in headers.items():
        if not path.is_file() or path.read_bytes() != before:
            raise ValueError(f"Concurrent quoted-header edit during {phase}: {path}")


def promote(ids, candidate, verify_only=False, original_root=None):
    if JOURNAL.exists():
        raise ValueError("Interrupted promotion: run scripts/promote.py --recover first")
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    wanted = set(ids)
    rows = [r for r in state["functions"] if r["id"] in wanted]
    if not rows or {r["id"] for r in rows} != wanted:
        raise ValueError("Every requested function must already have reviewed target context in recovery.json")
    sources = {r["source"] for r in rows}
    if len(sources) != 1:
        raise ValueError("One source file per promotion")
    destination = ROOT / next(iter(sources))
    if not destination.resolve().is_relative_to((ROOT / "src").resolve()):
        raise ValueError("Promotion destination must be under src/")
    candidate = Path(candidate).resolve()
    content = candidate.read_bytes()
    suffix = destination.suffix.lower()
    if suffix not in {".c", ".cpp"}:
        raise ValueError("Promotion destination must be a .c or .cpp source under src/")
    if candidate.suffix.lower() != suffix:
        raise ValueError("Candidate and promotion destination must use the same source language suffix")
    if suffix == ".cpp" and candidate != destination.resolve():
        raise ValueError("C++ candidates must be staged at their canonical src/*.cpp path so quoted-header lookup is checked in place")
    observed = {STATE: state_before}
    observed[candidate] = content
    header_observed = {}
    for row in state["functions"]:
        if row["status"] == "FUNCTION_MATCH" or row["id"] in wanted:
            path = ROOT / row["source"]
            observed[path] = path.read_bytes() if path.exists() else None
            header_observed.update(quoted_repository_headers(path, row["flags"]))
    observed.update(header_observed)
    allowed_symbols = {r["symbol"] for r in state["functions"] if r.get("source") in sources and
                       (r["status"] == "FUNCTION_MATCH" or r["id"] in wanted)}
    results = {}
    # Freeze candidate bytes for the compile and transaction to avoid a concurrent
    # scratch writer changing the candidate after verification.
    frozen = ROOT / "work/promotion-candidate-stage" / f"promotion-candidate{suffix}"
    frozen.parent.mkdir(parents=True, exist_ok=True)
    frozen.write_bytes(content)
    try:
        for row in state["functions"]:
            if row["status"] != "FUNCTION_MATCH" and row["id"] not in wanted:
                continue
            if row.get("source") in sources and suffix == ".cpp":
                # The frozen source remains C++; prepend the installed source
                # parent to ordinary /I lookup so quoted repository headers
                # resolve exactly where they will for the canonical path.
                frozen_row = dict(row)
                frozen_row["flags"] = ["/I" + str(destination.parent.resolve()), *row["flags"]]
                staged_result = check(frozen_row, frozen, original_root=original_root)
                if not staged_result["exact"]:
                    raise ValueError(f"Promotion rejected: frozen C++ candidate {row['id']} is not exact: {staged_result}")
                validate_candidate_object(staged_result["object"], allowed_symbols)

                # Independently compile the real canonical source-parent path
                # with only the flags recorded in recovery.json. The result
                # below, not the frozen compile, supplies the stored proof.
                result = check(row, destination, original_root=original_root)
            else:
                result = check(row, frozen if row.get("source") in sources else None,
                               original_root=original_root)
            if not result["exact"]:
                raise ValueError(f"Promotion rejected: {row['id']} is not exact: {result}")
            if row.get("source") in sources and result["source_sha256"] != digest(content):
                raise ValueError(f"Candidate changed before the exact source bytes were compiled: {row['id']}")
            results[row["id"]] = result
            if row.get("source") in sources:
                validate_candidate_object(result["object"], allowed_symbols)
        for path, before in observed.items():
            if (path.read_bytes() if path.exists() else None) != before:
                if path in header_observed:
                    raise ValueError(f"Concurrent quoted-header edit during verification: {path}")
                raise ValueError(f"Concurrent canonical edit during verification: {path}")
        print(f"Fresh byte verification passed for {len(results)} functions.")
        if verify_only:
            return
        for row in state["functions"]:
            if row["id"] in results:
                result = results[row["id"]]
                row["status"] = "FUNCTION_MATCH"
                row["proof"] = {k: result[k] for k in ("scope", "source_sha256", "compiled_sha256", "target_sha256", "toolchain")}
        state_content = (json.dumps(state, indent=2) + "\n").encode("utf-8")
        journal = []
        for path, after in ((destination, content), (STATE, state_content)):
            before = observed[path]
            journal.append({"path": path.relative_to(ROOT).as_posix(),
                            "before": base64.b64encode(before).decode("ascii") if before is not None else None,
                            "before_sha256": digest(before) if before is not None else None,
                            "after_sha256": digest(after)})
        atomic_bytes(JOURNAL, json.dumps(journal).encode("utf-8"))
        try:
            _assert_headers_unchanged(header_observed, "promotion commit")
            atomic_bytes(destination, content)
            _assert_headers_unchanged(header_observed, "source replacement")
            atomic_bytes(STATE, state_content)
            _assert_headers_unchanged(header_observed, "state replacement")
            _assert_headers_unchanged(header_observed, "journal removal")
        except BaseException:
            recover()
            raise
        JOURNAL.unlink()
        print("Promoted: " + ", ".join(ids))
    finally:
        frozen.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ids", nargs="*")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--recover", action="store_true")
    parser.add_argument("--original-root", type=Path,
                        help="Read hash-pinned originals from this checkout while keeping local source and recovery state")
    args = parser.parse_args()
    with lock():
        if args.recover:
            recover()
        elif not args.ids or args.source is None:
            parser.error("provide function ids and --source, or --recover")
        else:
            promote(args.ids, args.source, args.verify_only, args.original_root)


if __name__ == "__main__":
    main()
