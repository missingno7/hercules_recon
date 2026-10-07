"""Serialize source promotion; freshly verify every accepted function before writing."""
import argparse
import base64
from contextlib import contextmanager
import json
import os
from pathlib import Path

from env import ROOT
from match import check, digest
from coff import COFF

STATE = ROOT / "recovery.json"
JOURNAL = ROOT / "work/promotion-journal.json"


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


def promote(ids, candidate, verify_only=False):
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
    content = candidate.read_bytes()
    if destination.suffix.lower() != ".c":
        raise ValueError("Initial promotion gate supports self-contained C translation units only")
    observed = {STATE: state_before}
    for row in state["functions"]:
        if row["status"] == "FUNCTION_MATCH" or row["id"] in wanted:
            path = ROOT / row["source"]
            observed[path] = path.read_bytes() if path.exists() else None
    allowed_symbols = {r["symbol"] for r in state["functions"] if r.get("source") in sources and
                       (r["status"] == "FUNCTION_MATCH" or r["id"] in wanted)}
    results = {}
    # Freeze candidate bytes for the compile and transaction to avoid a concurrent
    # scratch writer changing the candidate after verification.
    frozen = ROOT / "work/promotion-candidate.c"
    frozen.write_bytes(content)
    try:
        for row in state["functions"]:
            if row["status"] != "FUNCTION_MATCH" and row["id"] not in wanted:
                continue
            result = check(row, frozen if row.get("source") in sources else None)
            if not result["exact"]:
                raise ValueError(f"Promotion rejected: {row['id']} is not exact: {result}")
            results[row["id"]] = result
            if row.get("source") in sources:
                validate_candidate_object(result["object"], allowed_symbols)
        for path, before in observed.items():
            if (path.read_bytes() if path.exists() else None) != before:
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
            atomic_bytes(destination, content)
            atomic_bytes(STATE, state_content)
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
    args = parser.parse_args()
    with lock():
        if args.recover:
            recover()
        elif not args.ids or args.source is None:
            parser.error("provide function ids and --source, or --recover")
        else:
            promote(args.ids, args.source, args.verify_only)


if __name__ == "__main__":
    main()
