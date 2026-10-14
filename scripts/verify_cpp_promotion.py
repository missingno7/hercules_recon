"""Isolated historical tests of strict C/C++ promotion and rollback."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
import argparse
_parser = argparse.ArgumentParser(description=__doc__)
_parser.add_argument("--original-root", type=Path, default=Path(r"D:\Prog\hercules_recon"))
_parser.add_argument("--output", type=Path, default=ROOT / "work/cpp_promotion_verify")
_args = _parser.parse_args()
LANE = _args.output.resolve()
if not LANE.is_relative_to((ROOT / "work").resolve()):
    raise SystemExit("--output must be inside ignored work/")
LANE.mkdir(parents=True, exist_ok=True)
ORACLE = _args.original_root.resolve() / "work/discs/pc_install/HERCULES.EXE"
ORIGINAL_ROOT = ORACLE.parent
CANONICAL_HEADER = ROOT / "calibration/host_system_keys.h"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
sys.path[:0] = [str(ROOT / "scripts"), r"C:\tools\hercules\python"]

import match  # noqa: E402

SPEC = importlib.util.spec_from_file_location("private_promote_cpp", ROOT / "scripts/promote.py")
promote = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(promote)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def hash_file(path: Path) -> str:
    return digest(path.read_bytes())


def private_temp(prefix: str):
    parent = LANE.resolve()
    temp = tempfile.TemporaryDirectory(prefix=prefix, dir=parent)
    if not Path(temp.name).resolve().is_relative_to(parent):
        temp.cleanup()
        raise ValueError("Fixture escaped work/host_cpp_promotion")
    return temp


def create_fixture(source_text: str, *, destination: str = "src/pc/host_frame_erase.cpp",
                   accepted_peer: bool = False, bad_target_hash: bool = False):
    temp = private_temp("fixture-")
    root = Path(temp.name)
    source = root / "src/pc/host_frame_erase.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(source_text, encoding="ascii", newline="\n")

    header = root / "calibration/host_system_keys.h"
    header.parent.mkdir(parents=True)
    header_text = CANONICAL_HEADER.read_text(encoding="ascii")
    marker = "protected:\n    virtual BOOL PreCreateWindow"
    declaration = "public:\n    afx_msg BOOL OnRva_00009900(CDC*);\n\nprotected:\n    virtual BOOL PreCreateWindow"
    if marker not in header_text:
        raise ValueError("Canonical class header shape changed; refresh the private fixture review")
    if "afx_msg BOOL OnRva_00009900(CDC*);" not in header_text:
        header_text = header_text.replace(marker, declaration, 1)
    header.write_text(header_text, encoding="ascii", newline="\n")

    (root / "calibration/nested").mkdir(parents=True)
    (root / "calibration/promotion_context.h").write_text(
        '#ifndef PRIVATE_PROMOTION_CONTEXT_H\n#define PRIVATE_PROMOTION_CONTEXT_H\n'
        '#include "nested/promotion_nested.h"\n#define PRIVATE_UNUSED_CONTEXT_VALUE 19\n#endif\n',
        encoding="ascii", newline="\n")
    (root / "calibration/nested/promotion_nested.h").write_text(
        '#ifndef PRIVATE_PROMOTION_NESTED_H\n#define PRIVATE_PROMOTION_NESTED_H\n'
        '#define PRIVATE_UNUSED_NESTED_VALUE 23\n#endif\n',
        encoding="ascii", newline="\n")

    (root / "toolchains").mkdir()
    shutil.copyfile(ROOT / "toolchains/manifest.json", root / "toolchains/manifest.json")
    flags = ["/O2", "/MT", "/GX", "/I" + str(TOOLCHAIN / "mfc/include")]
    row = {
        "id": "private.host_frame_erase",
        "status": "RECONSTRUCTED",
        "source": destination,
        "target": "HERCULES.EXE",
        "target_sha256": hash_file(ORACLE),
        "symbol": "?OnRva_00009900@CMainFrame@@QAEHPAVCDC@@@Z",
        "flags": flags,
        "rva": "0x9900",
        "size": 16,
    }
    rows = [row]
    if accepted_peer:
        rows.append({
            "id": "private.accepted-peer",
            "status": "FUNCTION_MATCH",
            "source": destination,
            "target": "HERCULES.EXE",
            "target_sha256": hash_file(ORACLE),
            "symbol": "?OnSysKeyUp@CMainFrame@@QAEXIII@Z",
            "flags": flags,
            "rva": "0x9b20",
            "size": 32,
            "proof": {"source_sha256": "0" * 64},
        })
    if bad_target_hash:
        row["target_sha256"] = "0" * 64
    state = {"schema_version": 1, "functions": rows}
    state_path = root / "recovery.json"
    state_before = (json.dumps(state, indent=2) + "\n").encode("utf-8")
    state_path.write_bytes(state_before)
    (root / "work").mkdir()

    return temp, root, source, header, state_path, row, state_before


def invoke(root: Path, row: dict, candidate: Path, *, verify_only: bool = True,
           original_root: Path = ORIGINAL_ROOT, calls_out: list | None = None,
           after_check=None):
    promote.ROOT = root
    promote.STATE = root / "recovery.json"
    promote.JOURNAL = root / "work/promotion-journal.json"
    match.ROOT = root
    match.verify_toolchain.cache_clear()
    calls = calls_out if calls_out is not None else []
    real_check = match.check

    def pinned_check(check_row, source=None, original_root=None):
        result = real_check(check_row, source, original_root=original_root)
        calls.append({
            "source": str(source) if source else None,
            "suffix": Path(source).suffix.lower() if source else None,
            "flags": list(check_row["flags"]),
            "exact": result["exact"],
            "compiled_sha256": result["compiled_sha256"],
            "target_sha256": result["target_sha256"],
            "source_sha256": result["source_sha256"],
            "object_sha256": hash_file(Path(result["object"])),
            "command": result["command"],
        })
        if after_check is not None:
            after_check(len(calls), check_row, source, result)
        return result

    promote.check = pinned_check
    try:
        promote.promote([row["id"]], candidate, verify_only=verify_only,
                        original_root=original_root)
    finally:
        promote.check = real_check
    return calls


def test_case(name: str, source_text: str, *, expected_error: str | None = None,
              candidate_suffix: str | None = None, destination: str | None = None,
              candidate_elsewhere: bool = False, accepted_peer: bool = False,
              bad_target_hash: bool = False, original_root: Path = ORIGINAL_ROOT):
    dest = destination or "src/pc/host_frame_erase.cpp"
    temp, root, candidate, header, state_path, row, state_before = create_fixture(
        source_text, destination=dest, accepted_peer=accepted_peer,
        bad_target_hash=bad_target_hash)
    try:
        if candidate_suffix:
            other = root / f"src/pc/host_frame_erase{candidate_suffix}"
            other.write_bytes(candidate.read_bytes())
            candidate_arg = other
        elif candidate_elsewhere:
            other = root / "work/outside/host_frame_erase.cpp"
            other.parent.mkdir(parents=True)
            other.write_bytes(candidate.read_bytes())
            candidate_arg = other
        else:
            candidate_arg = candidate

        if dest.endswith("protected.h"):
            (root / dest).parent.mkdir(parents=True, exist_ok=True)
            (root / dest).write_bytes(b"protected header\n")
        candidate_before = candidate_arg.read_bytes()
        header_before = header.read_bytes()
        state_now_before = state_path.read_bytes()
        oracle_before = hash_file(ORACLE)
        calls = []
        error = None
        try:
            calls = invoke(root, row, candidate_arg, original_root=original_root)
        except (ValueError, RuntimeError) as exc:
            error = str(exc)

        if expected_error:
            if error is None or expected_error not in error:
                raise AssertionError(f"{name}: expected refusal containing {expected_error!r}; got {error!r}")
        elif error is not None:
            raise AssertionError(f"{name}: unexpected refusal: {error}")

        if state_path.read_bytes() != state_now_before or state_before != state_path.read_bytes():
            raise AssertionError(f"{name}: --verify-only changed isolated recovery status")
        if header.read_bytes() != header_before:
            raise AssertionError(f"{name}: protected header changed")
        if candidate_arg.read_bytes() != candidate_before:
            raise AssertionError(f"{name}: candidate source changed")
        if (root / "work/promotion-journal.json").exists():
            raise AssertionError(f"{name}: verify-only wrote a promotion journal")
        if hash_file(ORACLE) != oracle_before or oracle_before != "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d":
            raise AssertionError(f"{name}: pinned original changed")

        result = {"name": name, "accepted": error is None, "error": error, "calls": calls,
                  "source_sha256": hash_file(candidate),
                  "header_sha256": hash_file(header),
                  "header_context_only": True,
                  "repository_quoted_headers": [
                      {"path": path.relative_to(root).as_posix(), "sha256": digest(data)}
                      for path, data in promote.quoted_repository_headers(root / row["source"], row["flags"]).items()
                  ],
                  "recovery_status_unchanged": state_path.read_bytes() == state_before,
                  "oracle_sha256": oracle_before}
        return result
    finally:
        temp.cleanup()


def test_cpp_transaction(*, fail_state_write: bool):
    label = "cpp-transaction-rollback" if fail_state_write else "cpp-transaction-commit"
    source_text = '#include "../../calibration/host_system_keys.h"\n\nBOOL CMainFrame::OnRva_00009900(CDC*)\n{\n    return TRUE;\n}\n'
    temp, root, candidate, header, state_path, row, state_before = create_fixture(source_text)
    try:
        source_before = candidate.read_bytes()
        header_before = header.read_bytes()
        calls = []
        failure = None
        if fail_state_write:
            real_atomic_bytes = promote.atomic_bytes
            failed = False

            def fail_once(path, content):
                nonlocal failed
                if Path(path).resolve() == state_path.resolve() and not failed:
                    failed = True
                    raise OSError("private one-shot state replacement failure")
                return real_atomic_bytes(path, content)

            promote.atomic_bytes = fail_once
        try:
            calls = invoke(root, row, candidate, verify_only=False)
        except OSError as exc:
            failure = str(exc)
        finally:
            if fail_state_write:
                promote.atomic_bytes = real_atomic_bytes

        if fail_state_write:
            if failure != "private one-shot state replacement failure":
                raise AssertionError(f"Rollback case did not reach the injected transaction failure: {failure}")
            if state_path.read_bytes() != state_before or candidate.read_bytes() != source_before:
                raise AssertionError("Interrupted C++ transaction did not restore source and state")
        else:
            state = json.loads(state_path.read_text(encoding="utf-8"))
            promoted = state["functions"][0]
            if promoted["status"] != "FUNCTION_MATCH" or promoted["proof"]["source_sha256"] != digest(source_before):
                raise AssertionError("Successful isolated C++ transaction did not write the strict proof")
        if header.read_bytes() != header_before:
            raise AssertionError("C++ transaction changed the protected header")
        if promote.JOURNAL.exists():
            raise AssertionError("C++ transaction left a journal after commit/rollback")
        return {"name": label, "accepted": not fail_state_write, "injected_failure": failure,
                "compile_calls": calls, "source_sha256": hash_file(candidate),
                "header_sha256": hash_file(header), "journal_removed": True,
                "repository_quoted_headers": [
                    {"path": path.relative_to(root).as_posix(), "sha256": digest(data)}
                    for path, data in promote.quoted_repository_headers(candidate, row["flags"]).items()
                ],
                "header_context_only": True, "recovery_fixture_only": True,
                "recovery_status_unchanged": fail_state_write}
    finally:
        temp.cleanup()


def test_header_mutation(*, nested: bool, after_journal: bool = False):
    mode = "nested" if nested else "direct"
    timing = "after-journal" if after_journal else "after-first-compile"
    name = f"{mode}-quoted-header-mutation-{timing}-refused"
    base = '#include "../../calibration/host_system_keys.h"\n'
    base += '#include "../../calibration/promotion_context.h"\n\n'
    base += "BOOL CMainFrame::OnRva_00009900(CDC*)\n{\n    return TRUE;\n}\n"
    temp, root, candidate, class_header, state_path, row, state_before = create_fixture(base)
    direct = root / "calibration/promotion_context.h"
    nested_file = root / "calibration/nested/promotion_nested.h"
    changed = nested_file if nested else direct
    changed_before = changed.read_bytes()
    source_before = candidate.read_bytes()
    class_header_before = class_header.read_bytes()
    calls = []
    did_mutate = False
    real_atomic_bytes = promote.atomic_bytes

    def mutate_file():
        nonlocal did_mutate
        if did_mutate:
            return
        changed.write_bytes(changed_before + b"\n// injected concurrent header edit\n")
        did_mutate = True

    def after_check(index, check_row, source, result):
        if not after_journal and index == 1:
            if not result["exact"]:
                raise AssertionError("Header mutation case must begin with an exact leaf comparison")
            mutate_file()

    if after_journal:
        def mutate_after_journal(path, content):
            result = real_atomic_bytes(path, content)
            if Path(path).resolve() == promote.JOURNAL.resolve():
                mutate_file()
            return result

        promote.atomic_bytes = mutate_after_journal
    error = None
    try:
        try:
            invoke(root, row, candidate, verify_only=not after_journal,
                   calls_out=calls, after_check=after_check)
        except ValueError as exc:
            error = str(exc)
    finally:
        if after_journal:
            promote.atomic_bytes = real_atomic_bytes

    try:
        expected_phase = "promotion commit" if after_journal else "verification"
        if not did_mutate or error is None or f"during {expected_phase}" not in error:
            raise AssertionError(f"{name}: header mutation was not rejected at the expected phase: {error}")
        if len(calls) != 2 or not all(call["exact"] for call in calls):
            raise AssertionError(f"{name}: leaf bytes must remain exact before the header edit is detected")
        if state_path.read_bytes() != state_before or candidate.read_bytes() != source_before:
            raise AssertionError(f"{name}: rejection changed source or recovery state")
        if class_header.read_bytes() != class_header_before:
            raise AssertionError(f"{name}: tool changed the class header")
        if changed.read_bytes() == changed_before:
            raise AssertionError(f"{name}: injected repository header edit did not occur")
        if (root / "work/promotion-journal.json").exists():
            raise AssertionError(f"{name}: journal remains after refusal/rollback")
        snapshots = promote.quoted_repository_headers(candidate, row["flags"])
        expected_paths = {direct.resolve(), nested_file.resolve()}
        if not expected_paths.issubset(snapshots):
            raise AssertionError(f"{name}: transitive literal include scan missed a repository header")
        return {"name": name, "accepted": False, "error": error,
                "leaf_checks_exact": [call["compiled_sha256"] for call in calls],
                "leaf_sha256_matches_oracle": all(call["compiled_sha256"] == call["target_sha256"] for call in calls),
                "changed_header": changed.relative_to(root).as_posix(),
                "changed_header_sha256_before": digest(changed_before),
                "changed_header_sha256_after": hash_file(changed),
                "repository_quoted_headers": [
                    {"path": path.relative_to(root).as_posix(), "sha256": digest(data)}
                    for path, data in snapshots.items()
                ],
                "header_context_only": True, "recovery_status_unchanged": True,
                "journal_removed": True, "recovery_fixture_only": True}
    finally:
        temp.cleanup()


def test_self_contained_c_compatibility():
    temp = private_temp("c-compatible-")
    root = Path(temp.name)
    source = root / "src/title/pilot.c"
    source.parent.mkdir(parents=True)
    canonical_source = ROOT / "src/title/pilot.c"
    source_bytes = canonical_source.read_bytes()
    source.write_bytes(source_bytes)
    original_state = json.loads((ROOT / "recovery.json").read_text(encoding="utf-8"))
    rows = [row for row in original_state["functions"]
            if row["source"] == "src/title/pilot.c" and row["status"] == "FUNCTION_MATCH"]
    ids = [row["id"] for row in rows]
    state = {"schema_version": 1, "functions": rows}
    state_path = root / "recovery.json"
    state_before = (json.dumps(state, indent=2) + "\n").encode("utf-8")
    state_path.write_bytes(state_before)
    (root / "toolchains").mkdir()
    shutil.copyfile(ROOT / "toolchains/manifest.json", root / "toolchains/manifest.json")
    (root / "work").mkdir()
    header_context = promote.quoted_repository_headers(source, rows[0]["flags"])
    if header_context:
        raise AssertionError("The C compatibility fixture is no longer self-contained")
    calls = []
    try:
        promote.ROOT = root
        promote.STATE = state_path
        promote.JOURNAL = root / "work/promotion-journal.json"
        match.ROOT = root
        match.verify_toolchain.cache_clear()
        real_check = match.check

        def pinned_check(check_row, candidate=None, original_root=None):
            result = real_check(check_row, candidate, original_root=original_root)
            calls.append({"id": result["id"], "exact": result["exact"],
                          "compiled_sha256": result["compiled_sha256"],
                          "target_sha256": result["target_sha256"],
                          "source_sha256": result["source_sha256"],
                          "command": result["command"]})
            return result

        promote.check = pinned_check
        try:
            promote.promote(ids, source, verify_only=True,
                            original_root=Path(r"D:\Prog\hercules_recon"))
        finally:
            promote.check = real_check
        if len(calls) != len(rows) or not all(call["exact"] for call in calls):
            raise AssertionError("Existing self-contained C rows did not all remain exact")
        if state_path.read_bytes() != state_before or source.read_bytes() != source_bytes:
            raise AssertionError("C --verify-only changed its private source or state")
        return {"name": "self-contained-c-path-compatibility", "accepted": True,
                "verified_rows": len(calls), "ids": ids, "calls": calls,
                "source_sha256": digest(source_bytes), "repository_quoted_headers": [],
                "header_context_only": True, "recovery_status_unchanged": True,
                "oracle_root": r"D:\Prog\hercules_recon", "fixture_only": True}
    finally:
        temp.cleanup()


def test_original_root_cli_override():
    source_text = '#include "../../calibration/host_system_keys.h"\n\nBOOL CMainFrame::OnRva_00009900(CDC*)\n{\n    return TRUE;\n}\n'
    temp, root, candidate, header, state_path, row, state_before = create_fixture(source_text)
    try:
        source_before = candidate.read_bytes()
        override = root / "primary-originals"
        override.mkdir()
        alternate_oracle = override / row["target"]
        alternate_oracle.write_bytes(b"fixture oracle with a different hash")
        promote.ROOT = root
        promote.STATE = state_path
        promote.JOURNAL = root / "work/promotion-journal.json"
        match.ROOT = root
        match.verify_toolchain.cache_clear()
        real_check = match.check
        forwarded = []

        def observe_override(check_row, source=None, original_root=None):
            forwarded.append((check_row["target"], Path(original_root).resolve()))
            return real_check(check_row, source, original_root=original_root)

        promote.check = observe_override
        old_argv = sys.argv
        sys.argv = ["promote_cpp.py", row["id"], "--source", str(candidate),
                    "--verify-only", "--original-root", str(override)]
        error = None
        try:
            promote.main()
        except ValueError as exc:
            error = str(exc)
        finally:
            sys.argv = old_argv
            promote.check = real_check
        expected_target = override.resolve() / row["target"]
        if error is None or f"Target hash mismatch: {expected_target}" not in error:
            raise AssertionError(f"--original-root did not select the fixture oracle: {error}")
        if forwarded != [("HERCULES.EXE", override.resolve())]:
            raise AssertionError(f"CLI did not forward a path override with the relative target intact: {forwarded}")
        if row["target"] != "HERCULES.EXE":
            raise AssertionError("The original target identity was rewritten")
        if state_path.read_bytes() != state_before or candidate.read_bytes() != source_before:
            raise AssertionError("Oracle-hash rejection changed private source or state")
        if (root / "work/promotion-journal.json").exists():
            raise AssertionError("Oracle-hash rejection wrote a transaction journal")
        if (root / "build/match").exists():
            raise AssertionError("Oracle-hash override rejection proceeded to compilation")
        return {"name": "cli-original-root-override-hash-rejection", "accepted": False,
                "error": error, "relative_target_preserved": row["target"],
                "original_root": str(override.resolve()), "override_target_sha256": hash_file(alternate_oracle),
                "expected_target_sha256": row["target_sha256"], "compile_not_reached": True,
                "header_context_only": True, "recovery_status_unchanged": True, "fixture_only": True}
    finally:
        temp.cleanup()


def main():
    if hash_file(ORACLE) != "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d":
        raise ValueError("Pinned HERCULES.EXE hash changed")
    base = '#include "../../calibration/host_system_keys.h"\n\nBOOL CMainFrame::OnRva_00009900(CDC*)\n{\n    return TRUE;\n}\n'
    report = {
        "scope": "Private strict tool experiment only; temporary recovery row is a harness fixture, not repository recovery status.",
        "fixture_row_present_in_real_recovery_json": any(row["id"] == "private.host_frame_erase" for row in json.loads((ROOT / "recovery.json").read_bytes())["functions"]),
        "status_authority": "The repository recovery.json was read-only; temporary source/header hashes are context only and grant no acceptance.",
        "oracle": str(ORACLE),
        "oracle_sha256": hash_file(ORACLE),
        "real_recovery_before_sha256": hash_file(ROOT / "recovery.json"),
        "canonical_header_before_sha256": hash_file(CANONICAL_HEADER),
        "toolchain": str(TOOLCHAIN),
        "toolchain_id": match.verify_toolchain(TOOLCHAIN),
        "toolchain_manifest_sha256": hash_file(ROOT / "toolchains/manifest.json"),
        "compiler_cl_sha256": hash_file(TOOLCHAIN / "bin/cl.exe"),
        "historical_class_source_sha256": hash_file(ROOT / "calibration/host_window_class.cpp"),
        "canonical_header_context_sha256": hash_file(CANONICAL_HEADER),
        "candidate_tool_sha256": hash_file(ROOT / "scripts/promote.py"),
        "runner_sha256": hash_file(Path(__file__).resolve()),
        "cases": [],
    }

    positive = test_case("raw-exact-cpp-verify-only", base)
    if not positive["accepted"] or len(positive["calls"]) != 2:
        raise AssertionError("Positive .cpp case did not perform staged and canonical compiles")
    staged, canonical = positive["calls"]
    if staged["suffix"] != ".cpp" or not staged["exact"] or not canonical["exact"]:
        raise AssertionError("Frozen C++ compile or canonical compile was not exact")
    if staged["compiled_sha256"] != canonical["compiled_sha256"]:
        raise AssertionError("Frozen and canonical contribution bytes differ")
    if staged["source_sha256"] != canonical["source_sha256"]:
        raise AssertionError("Frozen and canonical source bytes differ")
    if staged["flags"][0] != "/I" + str(Path(staged["flags"][0][2:]).resolve()):
        raise AssertionError("Frozen C++ compile did not prepend the canonical source parent to normal /I lookup")
    report["cases"].append(positive)

    report["cases"].append(test_case("raw-byte-mismatch-refused", base.replace("return TRUE;", "return FALSE;"),
                                     expected_error="is not exact"))
    report["cases"].append(test_case("extra-function-refused", base + "\nint unreviewed_helper(void) { return 7; }\n",
                                     expected_error="Candidate functions must exactly cover"))
    report["cases"].append(test_case("writable-data-refused", base + "\nint unreviewed_data = 7;\n",
                                     expected_error="unreviewed data contributions"))
    peer_source = base + "\nvoid CMainFrame::OnSysKeyUp(UINT, UINT, UINT) {}\n"
    report["cases"].append(test_case("accepted-peer-regression-refused", peer_source,
                                     expected_error="frozen C++ candidate private.accepted-peer is not exact",
                                     accepted_peer=True))
    report["cases"].append(test_case("suffix-language-mismatch-refused", base,
                                     expected_error="same source language suffix", candidate_suffix=".c"))
    report["cases"].append(test_case("header-destination-refused", base,
                                     expected_error=".c or .cpp source", destination="src/pc/protected.h"))
    report["cases"].append(test_case("off-canonical-cpp-path-refused", base,
                                     expected_error="staged at their canonical", candidate_elsewhere=True))
    report["cases"].append(test_case("target-hash-mismatch-refused", base,
                                     expected_error="Target hash mismatch", bad_target_hash=True))
    report["cases"].append(test_cpp_transaction(fail_state_write=False))
    report["cases"].append(test_cpp_transaction(fail_state_write=True))
    report["cases"].append(test_header_mutation(nested=False, after_journal=False))
    report["cases"].append(test_header_mutation(nested=True, after_journal=True))
    report["cases"].append(test_self_contained_c_compatibility())
    report["cases"].append(test_original_root_cli_override())

    nontransaction_cases = [case for case in report["cases"]
                            if not case["name"].startswith("cpp-transaction-")]
    c_case = next(case for case in report["cases"]
                  if case["name"] == "self-contained-c-path-compatibility")
    if not c_case["recovery_status_unchanged"]:
        raise AssertionError("The C compatibility fixture changed its private recovery state")
    if not all(case["recovery_status_unchanged"] for case in nontransaction_cases):
        raise AssertionError("A private negative or positive check changed source/header/status inputs")
    if not all(case.get("header_context_only", False) for case in report["cases"]):
        raise AssertionError("A private transaction did not record its header as context-only")
    report["canonical_header_after_sha256"] = hash_file(CANONICAL_HEADER)
    if report["canonical_header_after_sha256"] != report["canonical_header_before_sha256"]:
        raise AssertionError("Canonical header changed during private checks")
    report["inputs_after"] = {
        "oracle_sha256": hash_file(ORACLE),
        "canonical_header_sha256": hash_file(CANONICAL_HEADER),
        "real_recovery_sha256": hash_file(ROOT / "recovery.json"),
    }
    if report["inputs_after"]["real_recovery_sha256"] != report["real_recovery_before_sha256"]:
        raise AssertionError("Repository recovery.json changed during private tests")
    output = LANE / "report.json"
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"cases": [(c["name"], c["accepted"], c.get("error", c.get("injected_failure")))
                               for c in report["cases"]],
                      "report": str(output)}, indent=2))


if __name__ == "__main__":
    main()
