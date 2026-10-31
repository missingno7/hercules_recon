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
            if record.get("data_profile") == "file-data-owner-v1":
                reasons.append(f"{name}: file data owner semantics require the full relocated verifier")
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


def preflight_raw(rows, objects_report, preserve_reconstructed=False, toolchain=None, original_root=None):
    """Diagnose RAW mixed-unit readiness from a recorded object; never compiles or accepts.

    ``rows`` is the explicit candidate selection. Its IDs and complete row data
    must still match the current recovery ledger. Unit peers are always loaded
    from that ledger, and the normal fresh promotion gate remains mandatory.
    """
    from promote import existing_unit_metadata

    rows = list(rows)
    toolchain = Path(toolchain or match.DEFAULT_TOOLCHAIN).resolve()
    target_root = Path(original_root or ROOT).resolve()
    functions = [dict(id=row.get('id') if isinstance(row, dict) else None,
                      actual_relocations=[], errors=[], owner_profiles=[])
                 for row in rows]
    units = []
    top_errors = []
    snapshots = {}
    snapshot_users = {}

    def error(item, message):
        if message not in item['errors']:
            item['errors'].append(message)

    def snapshot(path, expected=None, item=None):
        path = Path(path).resolve()
        try:
            observed = _sha(path)
        except (OSError, ValueError) as exc:
            message = f"cannot snapshot {path}: {exc}"
            if item is None:
                top_errors.append(message)
            else:
                error(item, message)
            return
        if expected is not None and observed != expected:
            message = f"recorded identity differs from current input: {path}"
            if item is None:
                top_errors.append(message)
            else:
                error(item, message)
        previous = snapshots.get(path)
        if previous is not None and previous != observed:
            message = f"input identity changed while preflight read {path}"
            if item is None:
                top_errors.append(message)
            else:
                error(item, message)
        snapshots[path] = observed
        if item is not None:
            snapshot_users.setdefault(path, set()).add(item['id'])

    try:
        report, report_snapshots, memory_report_hash = _read_report(objects_report)
        for path, expected in report_snapshots.items():
            snapshot(path, expected)
        results = _report_results(report)
    except Exception as exc:
        results, memory_report_hash = [], None
        top_errors.append(f"invalid object report: {exc}")

    state_path = (ROOT / 'recovery.json').resolve()
    try:
        state_bytes = state_path.read_bytes()
        snapshot(state_path, match.digest(state_bytes))
        state = json.loads(state_bytes)
        ledger_rows = state['functions']
    except Exception as exc:
        ledger_rows = []
        top_errors.append(f"cannot read current recovery ledger: {exc}")

    try:
        toolchain_id = match.verify_toolchain(toolchain)
        snapshot((ROOT / 'toolchains/manifest.json').resolve())
    except Exception as exc:
        toolchain_id = None
        top_errors.append(f"selected toolchain is not independently pinned: {exc}")

    requested_ids = [row.get('id') for row in rows if isinstance(row, dict)]
    if (len(requested_ids) != len(rows) or any(not isinstance(value, str) for value in requested_ids) or
            len(set(requested_ids)) != len(requested_ids) or not requested_ids):
        top_errors.append('RAW preflight requires unique explicit recovery IDs')
    ledger_by_id = {row.get('id'): row for row in ledger_rows if isinstance(row, dict)}
    selected = [ledger_by_id[value] for value in requested_ids if value in ledger_by_id]
    if len(selected) != len(requested_ids):
        top_errors.append('RAW preflight selection contains unknown recovery IDs')
    elif any(_canonical_json(source) != _canonical_json(current)
             for source, current in zip(rows, selected)):
        top_errors.append('RAW preflight selection differs from the current recovery ledger')

    metadata = None
    if not top_errors:
        try:
            metadata = existing_unit_metadata(ledger_rows, requested_ids, preserve_reconstructed,
                                              root=ROOT, target_root=target_root, toolchain=toolchain)
        except Exception as exc:
            top_errors.append(f"mixed-unit metadata is not ready: {exc}")

    outputs = {item['id']: item for item in functions if isinstance(item.get('id'), str)}
    reference_snapshot = match.ReferenceSnapshot()
    if metadata is not None:
        for source, plan in metadata['units'].items():
            members = plan['members']
            candidate_ids = [row['id'] for row in metadata['selected'] if row['source'] == source]
            unit = dict(source=source, flags=list(plan['flags']), peer_ids=[row['id'] for row in members],
                        candidate_ids=candidate_ids, mapped_symbols=sorted(plan['mapped']),
                        pinned_symbols=sorted(plan['pinned_co_defined']), emitted_symbols=[],
                        source_sha256=None, compile_inputs={}, object=None, object_sha256=None,
                        toolchain=toolchain_id,
                        native_targets=[dict(path=path, sha256=sha) for path, sha in sorted(
                            {str((target_root / row['target']).resolve()): row.get('target_sha256')
                             for row in members}.items())],
                        unit_inventory_sha256=None, preserved_reconstructed=[],
                        independent_references=[],
                        reference_scope='binding/evidence hashes and COFF uses only; full semantics require fresh verification',
                        errors=[])
            units.append(unit)

            def unit_error(message, *, row_id=None):
                if message not in unit['errors']:
                    unit['errors'].append(message)
                if row_id in outputs:
                    error(outputs[row_id], message)

            source_path = _repo_path(source)
            try:
                source_sha = _sha(source_path)
                snapshot(source_path)
                if any(member.get('proof', {}).get('source_sha256') != source_sha for member in members):
                    unit_error('canonical source identity differs from current ledger peer proofs')
                unit['source_sha256'] = source_sha
                current_inputs = _compile_inputs(source_path, plan['flags'], toolchain)
                unit['compile_inputs'] = {str(path): sha for path, sha in sorted(
                    current_inputs.items(), key=lambda item: str(item[0]))}
                for path, expected in current_inputs.items():
                    snapshot(path, expected)
                for detail in _shadowed_includes(current_inputs, plan['flags'], toolchain):
                    unit_error(detail)
            except Exception as exc:
                unit_error(f"cannot freeze current source/include context: {exc}")
                continue

            profile_results = _same_profile_results(results, source_path, plan['flags'], toolchain)
            profile_items = [item for item, _path in profile_results]
            by_candidate = {row_id: [item for item in profile_items if item.get('id') == row_id]
                            for row_id in candidate_ids}
            if any(len(found) != 1 for found in by_candidate.values()):
                unit_error('object report has no unique result for every selected RAW candidate')
                continue
            identities = set()
            for item, object_path in profile_results:
                try:
                    reported_inputs = _reported_inputs(item)
                    identities.add((object_path, item.get('object_sha256'), item.get('source_sha256'),
                                    item.get('toolchain'), _canonical_json(
                                        {str(path): sha for path, sha in reported_inputs.items()})))
                except Exception as exc:
                    unit_error(f"invalid recorded unit profile result: {exc}")
            if len(identities) != 1:
                unit_error('ambiguous object report: unit profile names multiple object identities')
                continue
            profile_rows = [found[0] for found in by_candidate.values()]
            object_path = _profile(profile_rows[0], source_path, plan['flags'], toolchain)
            if any(_profile(item, source_path, plan['flags'], toolchain) != object_path
                   for item in profile_rows):
                unit_error('selected caller profile object paths disagree')
                continue
            if not object_path.is_file():
                unit_error(f"recorded caller object is missing: {object_path}")
                continue
            object_sha = _sha(object_path)
            unit['object'] = str(object_path)
            unit['object_sha256'] = object_sha
            expected_object_sha = profile_rows[0].get('object_sha256')
            if not isinstance(expected_object_sha, str) or not re.fullmatch(r'[0-9a-f]{64}', expected_object_sha):
                unit_error('object report has no valid complete object hash')
            snapshot(object_path, expected_object_sha)
            if object_sha != expected_object_sha:
                unit_error('recorded caller object hash is stale')
            if any(item.get('source_sha256') != source_sha for item in profile_rows):
                unit_error('recorded caller source hash is stale')
            if any(item.get('toolchain') != profile_rows[0].get('toolchain') or
                   item.get('toolchain') != toolchain_id or
                   not isinstance(item.get('toolchain'), str) or not item.get('toolchain')
                   for item in profile_rows):
                unit_error('recorded caller toolchain identity is missing or inconsistent')
            try:
                if any(_reported_inputs(item) != current_inputs for item in profile_rows):
                    unit_error('recorded source/include closure differs from current compile inputs')
            except Exception as exc:
                unit_error(f"invalid recorded compile-input closure: {exc}")

            try:
                coff = COFF(object_path)
                emitted = [symbol['name'] for symbol in coff.symbols.values()
                           if symbol['section'] > 0 and symbol['type'] & 0x20]
                unit['emitted_symbols'] = sorted(emitted)
                if len(emitted) != len(set(emitted)) or set(emitted) != plan['mapped'] | plan['co_defined']:
                    unit_error('complete emitted function inventory differs from current ledger/pinned inventory')
                for symbol in sorted(plan['mapped'] | plan['co_defined']):
                    coff.function(symbol)
                unit['unit_inventory_sha256'] = match.unit_inventory(object_path)
                for member in members:
                    try:
                        match.validate_unit_proof(member, current_inputs, object_path)
                    except Exception as exc:
                        unit_error(f"{member['id']}: {exc}")
            except Exception as exc:
                unit_error(f"cannot inventory complete caller unit: {exc}")
                continue

            for row_id in candidate_ids:
                row = outputs[row_id]
                ledger_row = next(peer for peer in members if peer['id'] == row_id)
                result = by_candidate[row_id][0]
                target = (target_root / ledger_row['target']).resolve()
                try:
                    if not target.is_file():
                        raise ValueError(f"native target is missing: {target}")
                    target_sha = ledger_row.get('target_sha256')
                    if not isinstance(target_sha, str) or not re.fullmatch(r'[0-9a-f]{64}', target_sha):
                        raise ValueError('selected row has no valid pinned native module hash')
                    snapshot(target, target_sha, row)
                    if result.get('source_sha256') != source_sha:
                        error(row, 'recorded caller source hash is stale')
                    measured = match.compare(object_path, ledger_row['symbol'], target,
                                             _rva(ledger_row['rva']), ledger_row['size'])
                    if result.get('target_sha256') != measured['target_sha256']:
                        error(row, 'recorded target extent hash differs from current native bytes')
                    if not measured['exact'] or measured['relocations'] or measured['target_relocations']:
                        error(row, 'selected candidate is not an exact zero-relocation RAW function')
                    if (measured['compiled_size'] != ledger_row['size'] or
                            result.get('compiled_sha256') != measured['compiled_sha256']):
                        error(row, 'selected candidate result differs from its complete COFF contribution')
                    if result.get('exact') is not True:
                        error(row, 'recorded result does not claim an exact RAW match')
                except Exception as exc:
                    error(row, f"cannot compare selected RAW candidate: {exc}")

            for member in members:
                target = (target_root / member['target']).resolve()
                target_sha = member.get('target_sha256')
                if not isinstance(target_sha, str) or not re.fullmatch(r'[0-9a-f]{64}', target_sha):
                    unit_error(f"{member['id']}: missing valid pinned native module hash")
                else:
                    snapshot(target, target_sha)

            for member in plan['reconstructed_members']:
                proof = member['proof']
                try:
                    contribution = coff.function(member['symbol'])
                    if (contribution['size'] != member['size'] or
                            contribution['size'] != proof['compiled_size'] or
                            match.digest(contribution['data']) != proof['compiled_sha256']):
                        raise ValueError('reconstructed peer complete contribution differs from its recorded size/hash')
                    result_toolchain = profile_rows[0].get('toolchain')
                    if proof['source_sha256'] != source_sha or proof['toolchain'] != result_toolchain:
                        raise ValueError('reconstructed peer source/toolchain identity differs from recorded object')
                    unit['preserved_reconstructed'].append(dict(
                        id=member['id'], symbol=member['symbol'], compiled_size=contribution['size'],
                        compiled_sha256=match.digest(contribution['data']), source_sha256=source_sha,
                        toolchain=result_toolchain))
                except Exception as exc:
                    unit_error(f"{member['id']}: {exc}")

            for member in members:
                if member['status'] != 'FUNCTION_MATCH_RELOCATED':
                    continue
                proof = member.get('proof', {})
                required = ('bindings_path', 'bindings_sha256', 'reference_evidence_sha256', 'context_sha256')
                if any(not proof.get(field) for field in required):
                    unit['independent_references'].append(dict(id=member['id'], status='missing'))
                    unit_error(f"{member['id']}: missing independent reference metadata")
                    continue
                try:
                    binding, evidence, observed = match.reference_inputs(
                        proof['bindings_path'], member, snapshot=reference_snapshot)
                    for path, expected in observed.items():
                        snapshot(path, expected)
                    if (match.reference_digest(binding) != proof['bindings_sha256'] or
                            match.reference_digest(evidence) != proof['reference_evidence_sha256']):
                        raise ValueError('independent reference binding/evidence identity changed')
                    if proof['context_sha256'] != match.context_proof(current_inputs):
                        raise ValueError('accepted compiler input context changed')
                    actual, relocation_errors = _actual_relocations(coff.function(member['symbol']))
                    if relocation_errors:
                        raise ValueError('; '.join(relocation_errors))
                    binding_errors = _check_binding_inventory(actual, binding.get('symbols'))
                    if binding_errors:
                        raise ValueError('; '.join(binding_errors))
                    unit['independent_references'].append(dict(id=member['id'], status='inputs-pinned',
                                                               bindings_sha256=proof['bindings_sha256'],
                                                               reference_evidence_sha256=proof['reference_evidence_sha256']))
                except Exception as exc:
                    unit['independent_references'].append(dict(id=member['id'], status='blocked'))
                    unit_error(f"{member['id']}: independent reference preflight failed: {exc}")

    try:
        reference_snapshot.assert_unchanged()
    except Exception as exc:
        top_errors.append(str(exc))
    for path, expected in snapshots.items():
        try:
            if _sha(path) != expected:
                message = f"input changed during preflight: {path}"
                affected = [item for item in functions if item['id'] in snapshot_users.get(path, set())]
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
                top_errors.append('in-memory object report changed during preflight')
        except Exception as exc:
            top_errors.append(f"cannot recheck in-memory object report: {exc}")
    for item in functions:
        item['errors'].sort()
    for unit in units:
        unit['errors'].sort()
    top_errors = sorted(set(top_errors))
    return dict(scope=SCOPE, ok=not top_errors and all(not item['errors'] for item in functions) and
                all(not unit['errors'] for unit in units), compiler_calls=0,
                errors=top_errors, functions=functions, units=units)
