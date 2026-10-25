"""Compiler-free diagnostic checks for reviewed relocated-reference inputs.

This module checks a previously recorded caller object and reviewed binding
inventory. It is not an acceptance verifier and never invokes the compiler.
"""
from __future__ import annotations

import json
import re
import struct
from pathlib import Path

import match
from coff import COFF
from env import ROOT
from pe import PE
from zero_storage import owner_profile, verify_native


SCOPE = "diagnostic only: recorded object/reference preflight; no matching or acceptance"
_RELOCATION_WIDTH = 4
_SUPPORTED_RELOCATIONS = {6, 20}  # IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32


def _sha(path: Path) -> str:
    return match.digest(path.read_bytes())


def _repo_path(path) -> Path:
    path = Path(path)
    return (ROOT / path).resolve() if not path.is_absolute() else path.resolve()


def _rva(value) -> int:
    if type(value) is int:
        return value
    if isinstance(value, str):
        return int(value, 0)
    raise ValueError("RVA must be an integer or an integer string")


def _canonical_json(value) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def _read_report(value):
    if isinstance(value, (str, Path)):
        path = _repo_path(value)
        raw = path.read_bytes()
        return json.loads(raw), {path: match.digest(raw)}, None
    # In-memory reports are useful to callers that already decoded JSON. Keep a
    # canonical snapshot so mutation during this call is still detected.
    frozen = json.loads(_canonical_json(value))
    return frozen, {}, match.digest(_canonical_json(value))


def _report_results(document):
    if isinstance(document, list):
        results = document
    elif isinstance(document, dict) and isinstance(document.get("results"), list):
        results = document["results"]
    else:
        raise ValueError("Object report must be a result list or an object containing results")
    if any(not isinstance(item, dict) for item in results):
        raise ValueError("Object report contains a non-object result")
    return results


def _profile(result, source, flags, toolchain):
    """Validate the exact recorded compile command and return its profile paths."""
    command = result.get("command")
    if not isinstance(command, list) or any(not isinstance(arg, str) for arg in command):
        raise ValueError("result has no recorded standard CL command")
    expected_cl = (Path(toolchain).resolve() / "bin" / "cl.exe").resolve()
    if len(command) < 5 or Path(command[0]).resolve() != expected_cl:
        raise ValueError("recorded compiler path is not the selected pinned CL profile")
    if command[1:4] != ["/nologo", "/c", "/Gy"]:
        raise ValueError("recorded command is not the standard /nologo /c /Gy profile")
    if len(command) != 6 + len(flags) or command[4:4 + len(flags)] != list(flags):
        raise ValueError("recorded ordered flags differ from the caller profile")
    output_arg, source_arg = command[-2:]
    if output_arg[:3].lower() != "/fo" or len(output_arg) <= 3:
        raise ValueError("recorded command has no standard /Fo object path")
    command_source = _repo_path(source_arg)
    if command_source != source:
        raise ValueError("recorded command source differs from the caller source")
    object_path = _repo_path(output_arg[3:])
    if _repo_path(result.get("object", "")) != object_path:
        raise ValueError("recorded object path differs from the command /Fo path")
    return object_path


def _same_profile_results(results, source, flags, toolchain):
    found = []
    for result in results:
        try:
            object_path = _profile(result, source, flags, toolchain)
        except (TypeError, ValueError, OSError):
            # A result from another source/profile is irrelevant. A result
            # claiming the requested id is reported as malformed below.
            continue
        found.append((result, object_path))
    return found


def _compile_inputs(source, flags, toolchain):
    context = match.compile_inputs(source, flags, toolchain)
    normalized = {Path(path).resolve(): sha for path, sha in context.items()}
    if len(normalized) != len(context):
        raise ValueError("compile-input paths collapse to duplicate resolved paths")
    return normalized


def _reported_inputs(result):
    raw = result.get("compile_inputs")
    if not isinstance(raw, dict) or not raw:
        raise ValueError("object report has no complete compile-input closure")
    normalized = {}
    for path, sha in raw.items():
        resolved = _repo_path(path)
        if not isinstance(sha, str) or not re.fullmatch(r"[0-9a-f]{64}", sha):
            raise ValueError("object report contains an invalid compile-input hash")
        if resolved in normalized:
            raise ValueError("object report has duplicate resolved compile-input paths")
        normalized[resolved] = sha
    return normalized


def _shadowed_includes(context, flags, toolchain):
    """Reject duplicate literal include candidates in the recorded search order."""
    include_dirs = []
    index = 0
    flags = list(flags)
    while index < len(flags):
        flag = flags[index]
        if flag.startswith(("/I", "-I")):
            value = flag[2:]
            if not value:
                index += 1
                if index == len(flags):
                    raise ValueError("Missing include path")
                value = flags[index]
            path = Path(value.strip('"'))
            if not path.is_absolute():
                raise ValueError("Include search path is not absolute")
            include_dirs.append(path.resolve())
        index += 1
    include_dirs.append((Path(toolchain) / "include").resolve())
    pattern = re.compile(r'^\s*#\s*(include\w*|import)\b\s*([^\n]*)', re.M)
    literals = re.compile(r'"([^"\n]+)"|<([^>\n]+)>')
    found = []
    for path in sorted(context, key=str):
        text = path.read_bytes().decode("latin1").replace("\\\r\n", "").replace("\\\n", "")
        text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                      lambda m: m[0] if m[0].startswith(('"', "'")) else " " + "\n" * m[0].count("\n"),
                      text, flags=re.S)
        for directive in pattern.finditer(text):
            match_name = literals.fullmatch(directive[2].strip())
            if directive[1] != "include" or match_name is None:
                continue  # compile_inputs already reports unsupported forms.
            quoted, angled = match_name.groups()
            name = quoted or angled
            roots = ([path.parent] if quoted else []) + include_dirs
            candidates = list(dict.fromkeys((root / name).resolve() for root in roots
                                            if (root / name).is_file()))
            if len(candidates) > 1:
                found.append(f"shadowed include {name}: " + ", ".join(map(str, candidates)))
    return found


def _actual_relocations(section):
    data = section["data"]
    actual, errors = [], []
    occupied = []
    for relocation in section["relocations"]:
        offset, typ, symbol = relocation["offset"], relocation["type"], relocation["symbol"]
        if type(offset) is not int or type(typ) is not int or not isinstance(symbol, str):
            errors.append("COFF relocation has invalid fields")
            continue
        if typ not in _SUPPORTED_RELOCATIONS:
            errors.append(f"unsupported COFF relocation type {typ} at {offset}")
        end = offset + _RELOCATION_WIDTH
        if offset < 0 or end > len(data):
            errors.append(f"COFF relocation at {offset} is outside the complete contribution")
            actual.append(dict(symbol=symbol, offset=offset, type=typ, addend=None))
            continue
        if any(offset < other_end and other_start < end for other_start, other_end in occupied):
            errors.append(f"overlapping COFF relocation field at {offset}")
        occupied.append((offset, end))
        addend = struct.unpack_from("<i", data, offset)[0]
        actual.append(dict(symbol=symbol, offset=offset, type=typ, addend=addend))
    return sorted(actual, key=lambda item: (item["offset"], item["symbol"], item["type"])), errors


def _check_binding_inventory(actual, symbols):
    errors = []
    if not isinstance(symbols, dict):
        return ["binding has no symbol inventory"]
    actual_names = {item["symbol"] for item in actual}
    if actual_names != set(symbols):
        errors.append("binding symbol names differ from actual COFF relocation symbols")
    for name, record in symbols.items():
        if not isinstance(name, str) or not isinstance(record, dict):
            errors.append("binding contains an invalid symbol record")
            continue
        references = record.get("references")
        if not isinstance(references, list):
            errors.append(f"{name}: binding references must be a list")
            continue
        expected = []
        for use in references:
            if not isinstance(use, dict):
                errors.append(f"{name}: malformed reference use")
                continue
            offset, typ, addend = (use.get(key) for key in ("offset", "type", "addend"))
            if any(type(value) is not int for value in (offset, typ, addend)):
                errors.append(f"{name}: reference offset/type/addend must be integers")
                continue
            expected.append(dict(symbol=name, offset=offset, type=typ, addend=addend))
        actual_for_name = [item for item in actual if item["symbol"] == name]
        if sorted(expected, key=lambda item: (item["offset"], item["type"], item["addend"])) != sorted(
                actual_for_name, key=lambda item: (item["offset"], item["type"], item["addend"])):
            errors.append(f"{name}: binding references differ from actual relocation uses")
    return errors


def _complex_binding_reasons(binding):
    reasons = []
    if binding.get("indirect_calls"):
        reasons.append("explicit indirect-call identities require the full relocated verifier")
    if binding.get("tables") or binding.get("sections"):
        reasons.append("table/section identities require the full relocated verifier")
    if isinstance(binding.get("symbols"), dict):
        for name, record in binding["symbols"].items():
            if not isinstance(record, dict):
                continue
            if "one_past" in record:
                reasons.append(f"{name}: specialized one-past semantics require the full verifier")
            if "source_alias" in record:
                reasons.append(f"{name}: storage alias semantics require the full verifier")
            uses = record.get("references")
            if isinstance(uses, list) and any(isinstance(use, dict) and "one_past" in use for use in uses):
                reasons.append(f"{name}: one-past relocation semantics require the full verifier")
    return sorted(set(reasons))


def preflight(rows, bindings_path, objects_report, toolchain=None, original_root=None):
    """Check recorded objects against bindings without compiling or accepting.

    ``objects_report`` may be a match.py result list, a promotion receipt, or a
    path to either JSON form. Every selected row needs a unique recorded caller
    profile and an independently pinned native target.
    """
    rows = list(rows)
    toolchain = Path(toolchain or match.DEFAULT_TOOLCHAIN).resolve()
    target_root = Path(original_root or ROOT).resolve()
    functions = [dict(id=row.get("id") if isinstance(row, dict) else None,
                      actual_relocations=[], errors=[], owner_profiles=[])
                 for row in rows]
    top_errors = []
    snapshots = {}
    snapshot_users = {}

    def error(result, message):
        if message not in result["errors"]:
            result["errors"].append(message)

    def snapshot(path, expected=None, result=None):
        path = Path(path).resolve()
        try:
            observed = expected if expected is not None else _sha(path)
        except (OSError, ValueError) as exc:
            if result is not None:
                error(result, f"cannot snapshot {path}: {exc}")
            else:
                top_errors.append(f"cannot snapshot {path}: {exc}")
            return
        previous = snapshots.get(path)
        if previous is not None and previous != observed:
            message = f"input identity changed while preflight read {path}"
            if result is not None:
                error(result, message)
            else:
                top_errors.append(message)
        snapshots[path] = observed
        if result is not None:
            snapshot_users.setdefault(path, set()).add(result["id"])

    try:
        report, report_snapshots, memory_report_hash = _read_report(objects_report)
        for path, sha in report_snapshots.items():
            snapshot(path, sha)
        results = _report_results(report)
    except Exception as exc:
        results = []
        top_errors.append(f"invalid object report: {exc}")
        memory_report_hash = None

    bindings_file = _repo_path(bindings_path)
    if bindings_file.exists():
        snapshot(bindings_file)
    else:
        top_errors.append(f"bindings file does not exist: {bindings_file}")

    for row, output in zip(rows, functions):
        if not isinstance(row, dict):
            error(output, "row must be an object")
            continue
        row_id = row.get("id")
        if not isinstance(row_id, str):
            error(output, "row id must be a string")
            continue
        source = _repo_path(row.get("source", ""))
        target = (target_root / row.get("target", "")).resolve()
        if not isinstance(row.get("flags"), list) or any(not isinstance(v, str) for v in row.get("flags", [])):
            error(output, "caller flags must be an ordered string list")
            continue
        try:
            snapshot(source, result=output)
            source_sha = _sha(source)
            binding, _evidence, observed = match.reference_inputs(bindings_path, row)
            for path, sha in observed.items():
                snapshot(path, sha, output)
            profile_results = _same_profile_results(results, source, row["flags"], toolchain)
            same_profile = [item for item, _ in profile_results]
            matching_ids = [item for item in same_profile if item.get("id") == row_id]
            if not profile_results or not matching_ids:
                error(output, "object report has no result for this caller source/ordered-flags profile")
                continue
            identities = set()
            for item, object_path in profile_results:
                identities.add((object_path, item.get("object_sha256"), item.get("source_sha256"),
                                _canonical_json(item.get("compile_inputs"))))
            if len(identities) != 1:
                error(output, "ambiguous object report: caller profile names multiple object identities")
                continue
            result = matching_ids[0]
            object_path = _profile(result, source, row["flags"], toolchain)
            if any(_profile(item, source, row["flags"], toolchain) != object_path for item in same_profile):
                error(output, "caller profile object paths disagree")
                continue
            if not object_path.is_file():
                error(output, f"recorded caller object is missing: {object_path}")
                continue
            object_sha = _sha(object_path)
            snapshot(object_path, result.get("object_sha256"), output)
            if object_sha != result.get("object_sha256"):
                error(output, "recorded caller object hash is stale")
            if result.get("source_sha256") != source_sha:
                error(output, "recorded caller source hash is stale")
            current_inputs = _compile_inputs(source, row["flags"], toolchain)
            reported_inputs = _reported_inputs(result)
            for path, sha in current_inputs.items():
                snapshot(path, sha, output)
            if reported_inputs != current_inputs:
                error(output, "recorded source/include closure differs from current compile inputs (stale or shadow header)")
            for detail in _shadowed_includes(current_inputs, row["flags"], toolchain):
                error(output, detail)
            if digest := result.get("target_sha256"):
                if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
                    error(output, "recorded target extent hash is invalid")
            else:
                error(output, "object report has no target extent hash")

            if not target.is_file():
                error(output, f"native target is missing: {target}")
                continue
            target_sha = _sha(target)
            snapshot(target, row.get("target_sha256"), output)
            if target_sha != row.get("target_sha256"):
                error(output, "native target file hash differs from the selected row")
            image = PE(target)
            rva, extent = _rva(row.get("rva")), row.get("size")
            if type(extent) is not int or extent <= 0:
                raise ValueError("caller extent must be a positive integer")
            executable = any(section["characteristics"] & 0x20000000 and
                             section["virtual_address"] <= rva and
                             rva + extent <= section["virtual_address"] + min(section["virtual_size"], section["raw_size"])
                             for section in image.sections)
            if not executable:
                error(output, "native caller extent is not wholly file-backed executable code")
            native_extent_sha = match.digest(image.read_rva(rva, extent))
            if result.get("target_sha256") != native_extent_sha:
                error(output, "recorded target extent hash differs from the current native bytes")

            try:
                contribution = COFF(object_path).function(row["symbol"])
                if len(contribution["data"]) != extent:
                    error(output, f"complete COFF contribution size {len(contribution['data'])} differs from native extent {extent}")
                output["actual_relocations"], relocation_errors = _actual_relocations(contribution)
                for message in relocation_errors:
                    error(output, message)
                for message in _check_binding_inventory(output["actual_relocations"], binding.get("symbols")):
                    error(output, message)
            except Exception as exc:
                error(output, f"cannot inventory complete caller contribution: {exc}")

            for message in _complex_binding_reasons(binding):
                error(output, message)

            symbols = binding.get("symbols", {})
            if isinstance(symbols, dict):
                for name, record in symbols.items():
                    if not isinstance(record, dict) or record.get("kind") != "bss":
                        continue
                    try:
                        bss_rva, bss_size = record.get("rva"), record.get("size")
                        if type(bss_rva) is not int or type(bss_size) is not int:
                            raise ValueError("BSS RVA and extent must be integers")
                        identity = record.get("identity")
                        anchor_at, anchor_size = identity.get("rva"), identity.get("size")
                        if any(type(v) is not int for v in (anchor_at, anchor_size)):
                            raise ValueError("BSS native identity anchor has invalid extent")
                        selected_ranges = []
                        for other in rows:
                            if not isinstance(other, dict):
                                continue
                            other_target = (target_root / other.get("target", "")).resolve()
                            if other_target == target:
                                selected_ranges.append((_rva(other.get("rva")), other.get("size"), other.get("id")))
                        if any(type(size) is int and anchor_at < at + size and at < anchor_at + anchor_size
                               for at, size, _ in selected_ranges):
                            raise ValueError("BSS native identity anchor overlaps a selected same-module caller")
                        # This native check deliberately precedes owner compile-input discovery.
                        verified = verify_native(image, bss_rva, bss_size, identity, rva, extent)
                        owner_source, owner_flags = owner_profile(record, name, ROOT)
                        owner_inputs = _compile_inputs(owner_source, owner_flags, toolchain)
                        for path, sha in owner_inputs.items():
                            snapshot(path, sha, output)
                        if any(observed.get(path) != sha for path, sha in owner_inputs.items()):
                            raise ValueError("BSS owner source/include closure is not independently pinned")
                        output["owner_profiles"].append(dict(
                            symbol=name, source=str(owner_source), flags=list(owner_flags),
                            compile_inputs={str(path): sha for path, sha in sorted(owner_inputs.items(), key=lambda item: str(item[0]))},
                            native_anchor=verified, requires_fresh_full_gate=True))
                    except Exception as exc:
                        error(output, f"{name}: native BSS/owner preflight failed: {exc}")
        except Exception as exc:
            error(output, str(exc))

    # Re-read every file identity, including report, references, target, object,
    # current headers and owner sources. This catches mutation during diagnosis.
    for path, expected in snapshots.items():
        try:
            if _sha(path) != expected:
                message = f"input changed during preflight: {path}"
                affected = [item for item in functions if item["id"] in snapshot_users.get(path, set())]
                if affected:
                    for item in affected:
                        error(item, message)
                else:
                    top_errors.append(message)
        except OSError as exc:
            top_errors.append(f"input disappeared during preflight: {path}: {exc}")
    if memory_report_hash is not None:
        try:
            if match.digest(_canonical_json(objects_report)) != memory_report_hash:
                top_errors.append("in-memory object report changed during preflight")
        except Exception as exc:
            top_errors.append(f"cannot recheck in-memory object report: {exc}")

    for item in functions:
        item["errors"].sort()
    top_errors = sorted(set(top_errors))
    return dict(scope=SCOPE, ok=not top_errors and all(not item["errors"] for item in functions),
                compiler_calls=0, errors=top_errors, functions=functions)
