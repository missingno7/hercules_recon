"""Fresh historical compile, full COFF contribution comparison, strict regression."""
import argparse
import copy
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile
import time
from functools import lru_cache

from env import ROOT
from pe import PE
from coff import COFF

DEFAULT_TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
RAW_STATUSES = {'FUNCTION_MATCH', 'FUNCTION_MATCH_RAW'}
RELOCATED = 'FUNCTION_MATCH_RELOCATED'
ACCEPTED = RAW_STATUSES | {RELOCATED}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def reference_digest(evidence):
    return digest(json.dumps(evidence, sort_keys=True, separators=(',', ':')).encode())


def accepted_rows(rows, raw_only=False):
    """Include retained RAW proofs even when the row also has legacy linked proof."""
    result = []
    for row in rows:
        if row['status'] in (RAW_STATUSES if raw_only else ACCEPTED):
            result.append(row)
        elif row.get('prior_raw_proof'):
            result.append(dict(row, status='FUNCTION_MATCH', proof=row['prior_raw_proof']))
    return result


def unit_inventory(obj):
    """Complete timestamp-independent object identity, not native data equality."""
    coff = COFF(obj)
    content = dict(sections=coff.sections, symbols=coff.symbols)
    return digest(json.dumps(content, sort_keys=True, default=lambda value: value.hex()).encode())


def context_proof(context):
    return {(p.relative_to(ROOT).as_posix() if p.is_relative_to(ROOT) else str(p)): h
            for p, h in context.items()}


def validate_unit_proof(row, context, obj=None):
    proof = row.get('proof', {})
    if 'unit_inventory_sha256' in proof:
        if not proof.get('context_sha256') or context_proof(context) != proof['context_sha256']:
            raise ValueError('Accepted unit source/header context changed: ' + row['id'])
        if obj is not None and unit_inventory(obj) != proof['unit_inventory_sha256']:
            raise ValueError('Accepted complete unit inventory changed: ' + row['id'])


@lru_cache(maxsize=None)
def verify_toolchain(toolchain):
    manifest = json.loads((ROOT / "toolchains/manifest.json").read_text())
    entries = [t for t in manifest["toolchains"] if t["directory"] == toolchain.name]
    if len(entries) != 1:
        raise ValueError("Unpinned toolchain; record and review its provenance first")
    for row in entries[0]["files"]:
        path = toolchain / row["path"]
        if not path.is_file() or path.stat().st_size != row["size"] or digest(path.read_bytes()) != row["sha256"]:
            raise ValueError(f"Toolchain identity changed: {path}")
    return entries[0]["id"]


def compile_source(source, flags, toolchain=DEFAULT_TOOLCHAIN):
    toolchain_id = verify_toolchain(toolchain)
    scratch = ROOT / "build/match"
    scratch.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="compile-", dir=scratch))
    obj = output / "candidate.obj"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    command = [str(toolchain / "bin/cl.exe"), "/nologo", "/c", "/Gy", *flags,
               "/Fo" + str(obj), str(Path(source).resolve())]
    metrics_dir = None
    metrics = os.environ.get("HERCULES_COMPILE_METRICS_DIR")
    if metrics:
        repo_root = Path(__file__).resolve().parents[1]
        requested = Path(metrics)
        metrics_dir = (requested if requested.is_absolute() else repo_root / requested).resolve()
        roots = [(repo_root / name).resolve() for name in ("work", "build")]
        if not any(root.is_relative_to(repo_root) and metrics_dir.is_relative_to(root) for root in roots):
            raise ValueError("Compile metrics directory must resolve under repository work/ or build/")
        metrics_dir.mkdir(parents=True, exist_ok=True)
        metrics_dir = metrics_dir.resolve()
        if not any(root.is_relative_to(repo_root) and metrics_dir.is_relative_to(root) for root in roots):
            raise ValueError("Compile metrics directory must resolve under repository work/ or build/")
        from datetime import datetime, timezone
        import uuid
        source_path = Path(source).resolve()
        try:
            source_hash = digest(source_path.read_bytes())
        except OSError:
            source_hash = None
        started_utc = datetime.now(timezone.utc)
        started = time.perf_counter()

        def record(returncode, error):
            event = {"command": command, "source_path": str(source_path), "source_sha256": source_hash,
                     "flags": command[4:-2], "toolchain_id": toolchain_id,
                     "started_utc": started_utc.isoformat(), "ended_utc": datetime.now(timezone.utc).isoformat(),
                     "elapsed_seconds": time.perf_counter() - started,
                     "returncode": returncode, "error": error}
            with (metrics_dir / (uuid.uuid4().hex + ".json")).open("x", encoding="utf-8") as stream:
                json.dump(event, stream, sort_keys=True)
                stream.write("\n")

        try:
            result = subprocess.run(command, env=env, cwd=output, capture_output=True, text=True)
        except BaseException as exc:
            record(None, f"{type(exc).__name__}: {exc}")
            raise
        record(result.returncode, f"compiler exited with status {result.returncode}" if result.returncode else None)
    else:
        result = subprocess.run(command, env=env, cwd=output, capture_output=True, text=True)
    (output / "compile.log").write_text(result.stdout + result.stderr)
    if result.returncode != 0 or not obj.is_file():
        raise RuntimeError(f"Historical compiler failed ({result.returncode}); see {output / 'compile.log'}")
    return obj, command


def compare(obj, symbol, target, rva, size):
    contribution = COFF(obj).function(symbol)
    actual = contribution["data"]
    pe = PE(target)
    if size <= 0 or not any(s["characteristics"] & 0x20000000 and
                           s["virtual_address"] <= rva and
                           rva + size <= s["virtual_address"] + min(s["virtual_size"], s["raw_size"])
                           for s in pe.sections):
        raise ValueError("Target interval must be wholly inside file-backed executable code")
    target_relocations = [entry.rva - rva for block in getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", [])
                          for entry in block.entries if entry.type and
                          entry.rva < rva + size and entry.rva + 4 > rva]
    expected = pe.read_rva(rva, size)
    differences = [i for i in range(max(len(expected), len(actual)))
                   if expected[i:i+1] != actual[i:i+1]]
    regions = []
    for offset in differences:
        if regions and regions[-1][1] == offset:
            regions[-1][1] = offset + 1
        else:
            regions.append([offset, offset + 1])
    relocations = contribution["relocations"]
    exact = expected == actual and not relocations and not target_relocations
    return {"exact": exact, "scope": "complete relocation-free function COFF contribution",
            "target_size": len(expected), "compiled_size": len(actual),
            "target_sha256": digest(expected), "compiled_sha256": digest(actual),
            "different_bytes": len(differences), "mismatch_regions": regions,
            "relocations": relocations, "target_relocations": target_relocations,
            "object": str(obj.resolve())}


def check(row, source=None, toolchain=DEFAULT_TOOLCHAIN, original_root=None):
    # Source and acceptance state remain local to this worktree. Only the
    # read location of the hash-pinned original may come from another checkout.
    target = (Path(original_root).resolve() if original_root is not None else ROOT) / row["target"]
    if digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError(f"Target hash mismatch: {target}")
    canonical = source is None
    source = Path(source) if source else ROOT / row["source"]
    source_before = source.read_bytes()
    if canonical and row["status"] == "FUNCTION_MATCH" and digest(source_before) != row["proof"]["source_sha256"]:
        raise ValueError("Canonical source differs from accepted proof; review and promote it before verification")
    context = compile_inputs(source, row['flags'], toolchain) if row.get('proof', {}).get('unit_inventory_sha256') else {}
    if context:
        validate_unit_proof(row, context)
    obj, command = compile_source(source, row["flags"], toolchain)
    if source.read_bytes() != source_before:
        raise ValueError("Source changed during compilation")
    result = compare(obj, row["symbol"], target, int(row["rva"], 0), row["size"])
    if digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError("Target changed during verification")
    if context:
        if compile_inputs(source, row['flags'], toolchain) != context:
            raise ValueError('Compile context changed during verification')
        validate_unit_proof(row, context, obj)
    result.update(id=row["id"], source_sha256=digest(source_before), command=command,
                  toolchain=verify_toolchain(toolchain))
    if original_root is not None:
        result["read_only_original_path"] = str(target.resolve())
    return result


def compile_inputs(source, flags, toolchain):
    """Conservative literal-include closure; unsupported input modes fail closed.

    Visit even inactive literal includes. This is not a preprocessor and must not
    silently certify macro includes, forced includes, PCHs or response files.
    """
    include_dirs = []
    index = 0
    while index < len(flags):
        flag = flags[index]
        if flag.startswith(('/I', '-I')):
            value = flag[2:]
            if not value:
                index += 1
                if index == len(flags):
                    raise ValueError('Missing include path')
                value = flags[index]
            path = Path(value.strip('"'))
            if not path.is_absolute():
                raise ValueError('Grouped verification requires absolute include paths')
            include_dirs.append(path)
        elif not re.fullmatch(r'[/-](?:O[12dx]|O[bgistwy][12+-]?|G[3456dryzX]|G[fy][+-]?|W[0-4]|WX|w|Z[ade]|MTd?|MLd?|MDd?|TC|TP|D[^\s]+|U\w+)', flag):
            raise ValueError('Unsupported grouped compile input flag: ' + flag)
        index += 1
    include_dirs.append(toolchain / 'include')
    result = {}

    def visit(path):
        path = path.resolve()
        if path in result:
            return
        data = path.read_bytes()
        result[path] = digest(data)
        text = data.decode('latin1').replace('\\\r\n', '').replace('\\\n', '')
        # Preserve strings while removing comments, including multiline comments.
        text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                      lambda m: ' ' + '\n' * m[0].count('\n') if m[0].startswith('/') else m[0], text, flags=re.S)
        for directive in re.finditer(r'^\s*#\s*(include\w*|import)\b\s*([^\n]*)', text, re.M):
            name = re.fullmatch(r'"([^"\n]+)"|<([^>\n]+)>', directive[2].strip())
            if directive[1] != 'include' or name is None:
                raise ValueError('Unsupported include directive in ' + str(path))
            roots = ([path.parent] if name[1] else []) + include_dirs
            choices = [root / (name[1] or name[2]) for root in roots]
            found = next((p for p in choices if p.is_file()), None)
            if found is None:
                raise ValueError('Unresolved include in ' + str(path) + ': ' + directive[2])
            visit(found)
    visit(source)
    return result


class ReferenceSnapshot:
    """One phase's immutable evidence reads, never a persisted acceptance cache.

    Verification still hashes all frozen inputs at its original guard points.
    Publication starts another snapshot and checks it again before writing.
    """
    def __init__(self):
        self.files = {}
        self.documents = {}

    def read(self, path, row):
        path = (ROOT / path).resolve()
        if not path.is_relative_to(ROOT.resolve()):
            raise ValueError('Reference bindings must be repository evidence')
        if path not in self.documents:
            content = path.read_bytes()
            actual = digest(content)
            if path in self.files and self.files[path] != actual:
                raise ValueError('Independent reference input changed during verification: ' + str(path))
            self.files[path] = actual
            document = json.loads(content)
            if document.get('schema') != 1 or not document.get('evidence_files'):
                raise ValueError('Reference bindings need versioned, pinned independent evidence')
            observed = {path: actual}
            for name, expected in document['evidence_files'].items():
                item = (ROOT / name).resolve()
                if (not any(item.is_relative_to(root.resolve()) for root in (ROOT, DEFAULT_TOOLCHAIN)) or
                        item == ROOT / 'recovery.json'):
                    raise ValueError('Reference evidence must be repository evidence or pinned SDK inputs, independent of recovery state')
                if item not in self.files:
                    self.files[item] = digest(item.read_bytes())
                if self.files[item] != expected:
                    raise ValueError('Independent reference evidence changed: ' + name)
                observed[item] = expected
            identity = {k: v for k, v in document.items() if k not in ('functions', 'evidence_files')}
            context = dict(files=document['evidence_files'], identity_sha256=reference_digest(identity))
            self.documents[path] = document, context, observed
        document, context, observed = self.documents[path]
        # Callers cannot alter a later row's frozen metadata by mutating a result.
        return copy.deepcopy(document['functions'][row['id']]), copy.deepcopy(context), dict(observed)

    def assert_unchanged(self):
        for path, expected in self.files.items():
            if digest(path.read_bytes()) != expected:
                raise ValueError('Independent reference input changed during verification: ' + str(path))


def reference_inputs(path, row, *, snapshot=None):
    """Read reviewed evidence, never infer bindings from the caller's operands."""
    return (snapshot if snapshot is not None else ReferenceSnapshot()).read(path, row)


def verify_rows(rows, toolchain=DEFAULT_TOOLCHAIN, original_root=None, relocation_candidates=None):
    """One fresh compile per immutable source/ordered-profile group, per run only."""
    started = time.perf_counter()
    toolchain = toolchain.resolve()
    manifest = ROOT / 'toolchains/manifest.json'
    manifest_before = manifest.read_bytes()
    verify_toolchain.cache_clear()
    toolchain_id = verify_toolchain(toolchain)
    pending = relocation_candidates or {}
    if not set(pending) <= {r['id'] for r in rows}:
        raise ValueError('Relocation candidates must be included in verification')
    groups, targets, references, evidence_inputs = {}, {}, {}, {}
    storage_profiles, storage_evidence, alias_records = {}, {}, {}
    reference_snapshot = ReferenceSnapshot()
    for row in rows:
        source, target = (ROOT / row['source']).resolve(), ((original_root or ROOT) / row['target']).resolve()
        if row['status'] not in ACCEPTED and row['id'] not in pending:
            raise ValueError('Strict verification requires accepted rows')
        if digest(source.read_bytes()) != row['proof']['source_sha256']:
            raise ValueError('Canonical source differs from accepted proof: ' + row['id'])
        if target in targets and targets[target] != row['target_sha256']:
            raise ValueError('Conflicting target identities')
        targets[target] = row['target_sha256']
        if row['status'] == RELOCATED or row['id'] in pending:
            path = pending.get(row['id']) or row['proof']['bindings_path']
            binding, evidence, observed = reference_inputs(path, row, snapshot=reference_snapshot)
            references[row['id']] = (binding, evidence, path)
            records = binding.get('symbols', {})
            if not isinstance(records, dict) or any(not isinstance(record, dict) for record in records.values()):
                raise ValueError('Invalid reference symbol records')
            for name, record in records.items():
                if record.get('kind') == 'bss':
                    from zero_storage import owner_profile
                    storage_profiles[(row['id'], name)] = owner_profile(record, name, ROOT)
                    storage_evidence[row['id']] = observed
                    if record['owner']['symbol'] != name:
                        from storage_alias import alias_profile
                        alias_records[(row['id'], name)] = alias_profile(record, name, ROOT)
                    identity = record.get('identity', {})
                    if not isinstance(identity, dict):
                        raise ValueError('Invalid loader-zero identity anchor')
                    at, length = identity.get('rva'), identity.get('size')
                    if type(at) is int and type(length) is int and any(
                            other['id'] in pending and ((original_root or ROOT) / other['target']).resolve() == target and
                            at < int(other['rva'], 0) + other['size'] and int(other['rva'], 0) < at + length
                            for other in rows):
                        raise ValueError('Loader-zero identity anchor overlaps a selected caller')
            callback_calls = binding.get('indirect_calls', [])
            if not isinstance(callback_calls, list):
                raise ValueError('Invalid explicit callback inventory')
            if callback_calls:
                from callback_transfer import identity_span
                for callback in callback_calls:
                    at, length = identity_span(callback)
                    if any(other['id'] in pending and
                           ((original_root or ROOT) / other['target']).resolve() == target and
                           at < int(other['rva'], 0) + other['size'] and
                           int(other['rva'], 0) < at + length for other in rows):
                        raise ValueError('Callback identity witness overlaps a selected caller')
            for item, sha in observed.items():
                if item in evidence_inputs and evidence_inputs[item] != sha:
                    raise ValueError('Conflicting independent evidence identities')
                evidence_inputs[item] = sha
            if row['id'] not in pending:
                if reference_digest(evidence) != row['proof']['reference_evidence_sha256']:
                    raise ValueError('Accepted independent evidence inventory changed: ' + row['id'])
                sha = digest(json.dumps(binding, sort_keys=True, separators=(',', ':')).encode())
                if sha != row['proof']['bindings_sha256']:
                    raise ValueError('Accepted reference bindings changed: ' + row['id'])
        groups.setdefault((source, tuple(row['flags'])), []).append(row)
    contexts = {key: compile_inputs(*key, toolchain) for key in groups}
    owner_contexts = {key: compile_inputs(*key, toolchain) for key in dict.fromkeys(storage_profiles.values())}
    alias_profiles, alias_wrappers, alias_origins, caller_alias_profiles = {}, {}, {}, {}
    row_by_id = {row['id']: row for row in rows}
    for (row_id, name), (header, names) in alias_records.items():
        row = row_by_id[row_id]
        record = references[row_id][0]['symbols'][name]
        alias = record['source_alias']
        owner_alias = alias['kind'] == 'owner-compile-renaming'
        source, flags = storage_profiles[(row_id, name)] if owner_alias else ((ROOT / row['source']).resolve(), tuple(row['flags']))
        alias_origins[(row_id, name)] = (source, flags)
        # /WX makes incompatible pointer declarations/qualifiers hard failures.
        compile_flags = (*flags, '/WX') if owner_alias else flags
        types = alias['pointer_types'] if owner_alias else None
        identity = (source, compile_flags, header, tuple(sorted(names.items())), tuple(sorted((types or {}).items())))
        if identity not in alias_wrappers:
            from storage_alias import wrapper_source
            alias_wrappers[identity] = (wrapper_source(source, header, ROOT, pointer_types=types), compile_flags)
        profile = alias_wrappers[identity]
        if not owner_alias and row_id in caller_alias_profiles and caller_alias_profiles[row_id] != profile:
            raise ValueError('One compiler-renaming header/map per source proof is required')
        if not owner_alias:
            caller_alias_profiles[row_id] = profile
        alias_profiles[(row_id, name)] = profile
    alias_contexts = {key: compile_inputs(*key, toolchain) for key in dict.fromkeys(alias_profiles.values())}
    for (row_id, name), key in alias_profiles.items():
        row = row_by_id[row_id]
        origin = alias_origins[(row_id, name)]
        original_context = owner_contexts[origin] if origin in owner_contexts else contexts[origin]
        for path, expected in alias_contexts[key].items():
            if path == key[0]:
                continue  # Generated wrapper is frozen for this run and rechecked before publication.
            if original_context.get(path) != expected and storage_evidence[row_id].get(path) != expected:
                raise ValueError('Compiler-renaming inputs must be independently pinned: ' + str(path))
    for (row_id, name), key in storage_profiles.items():
        if any(storage_evidence[row_id].get(path) != expected for path, expected in owner_contexts[key].items()):
            raise ValueError('Loader-zero owner source/include context must be independently pinned: ' + name)
    all_contexts = {**contexts, **owner_contexts, **alias_contexts}
    for key, members in groups.items():
        if any(contexts[key][key[0]] != row['proof']['source_sha256'] for row in members):
            raise ValueError('Source changed while freezing compile inputs')
        for row in members:
            validate_unit_proof(row, contexts[key])
            if row['status'] == RELOCATED and row['id'] not in pending:
                if context_proof(contexts[key]) != row['proof']['context_sha256']:
                    raise ValueError('Accepted compiler input changed: ' + row['id'])

    frozen_inputs = {}
    for context in all_contexts.values():
        for path, expected in context.items():
            if path in frozen_inputs and frozen_inputs[path] != expected:
                raise ValueError('Conflicting frozen compiler input')
            frozen_inputs[path] = expected

    def stable(resolve_includes=False):
        if manifest.read_bytes() != manifest_before:
            raise ValueError('Toolchain manifest changed during verification')
        for target, expected in targets.items():
            if digest(target.read_bytes()) != expected:
                raise ValueError('Target hash mismatch or mutation: ' + str(target))
        for path, expected in evidence_inputs.items():
            if digest(path.read_bytes()) != expected:
                raise ValueError('Independent reference input changed during verification: ' + str(path))
        for path, expected in frozen_inputs.items():
            if digest(path.read_bytes()) != expected:
                raise ValueError('Compile inputs changed during verification: ' + str(path))
        if resolve_includes:
            for key, expected in all_contexts.items():
                if compile_inputs(*key, toolchain) != expected:
                    raise ValueError('Include resolution changed during verification: ' + str(key[0]))

    stable()
    results, objects, compiled = [], {}, {}

    def build_profile(key):
        source, flags = key
        if key in compiled:
            obj, command = compiled[key]
            if digest(obj.read_bytes()) != objects[obj]:
                raise ValueError('Object changed during verification')
            return obj, command
        # Re-resolve the active group around its compile. Hash every unique
        # frozen input between groups; do not reparse every other TU N times.
        if compile_inputs(source, flags, toolchain) != all_contexts[key]:
            raise ValueError('Include resolution changed before compilation')
        obj, command = compile_source(source, list(flags), toolchain)
        objects[obj] = digest(obj.read_bytes())
        stable()
        if compile_inputs(source, flags, toolchain) != all_contexts[key]:
            raise ValueError('Include resolution changed during compilation')
        compiled[key] = (obj, command)
        return obj, command

    for key in owner_contexts:
        build_profile(key)
    for key in alias_contexts:
        build_profile(key)
    for (source, flags), members in groups.items():
        obj, command = build_profile((source, flags))
        # Full object identity guards all sections, symbols, auxiliaries and
        # relocations against mutation; it is not a cross-build identity/cache.
        for row in members:
            validate_unit_proof(row, contexts[(source, flags)], obj)
            if row['id'] in references:
                from masked import relocated_compare
                binding, evidence, path = references[row['id']]
                owners = {name: compiled[key][0] for (row_id, name), key in storage_profiles.items() if row_id == row['id']}
                aliases = {name: compiled[alias_profiles[(row['id'], name)]][0]
                           for row_id, name in alias_records if row_id == row['id']}
                result = relocated_compare(obj, row['symbol'], (original_root or ROOT) / row['target'],
                                           int(row['rva'], 0), row['size'], binding, storage_owners=owners,
                                           storage_aliases=aliases)
                if owners:
                    result['storage_objects'] = {name: dict(object=str(owner.resolve()), object_sha256=objects[owner],
                        command=compiled[storage_profiles[(row['id'], name)]][1],
                        compile_inputs={str(p): h for p, h in owner_contexts[storage_profiles[(row['id'], name)]].items()})
                        for name, owner in owners.items()}
                if aliases:
                    result['storage_alias_objects'] = {name: dict(object=str(adapted.resolve()), object_sha256=objects[adapted],
                        command=compiled[alias_profiles[(row['id'], name)]][1],
                        compile_inputs={str(p): h for p, h in alias_contexts[alias_profiles[(row['id'], name)]].items()})
                        for name, adapted in aliases.items()}
                result.update(exact=result['relocated_equal'], proof_category=RELOCATED,
                              bindings_path=path, reference_evidence_sha256=reference_digest(evidence),
                              context_sha256=context_proof(contexts[(source, flags)]), object=str(obj.resolve()))
                if row['status'] == RELOCATED and row['id'] not in pending:
                    for field in ('compiled_sha256', 'target_sha256'):
                        if result[field] != row['proof'][field]:
                            raise ValueError('Accepted relocated bytes changed: ' + row['id'])
            else:
                result = compare(obj, row['symbol'], (original_root or ROOT) / row['target'], int(row['rva'], 0), row['size'])
                result['proof_category'] = 'FUNCTION_MATCH_RAW'
            result.update(id=row['id'], source_sha256=contexts[(source, flags)][source],
                          command=command, toolchain=toolchain_id,
                          compile_inputs={str(p): h for p, h in contexts[(source, flags)].items()},
                          object_sha256=objects[obj])
            results.append(result)
        if digest(obj.read_bytes()) != objects[obj]:
            raise ValueError('Object changed during verification')
    stable(resolve_includes=True)
    if any(digest(p.read_bytes()) != h for p, h in objects.items()):
        raise ValueError('Object changed during verification')
    verify_toolchain.cache_clear()
    verify_toolchain(toolchain)
    stable()
    by_id = {r['id']: r for r in results}
    if len(by_id) != len(rows):
        raise ValueError('Duplicate recovery IDs')
    return [by_id[r['id']] for r in rows], dict(compiler_calls=len(compiled), rows=len(rows),
                                               elapsed_seconds=time.perf_counter() - started)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["verify", "candidate", "object", "references", "preflight"])
    parser.add_argument("id", nargs="?")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--obj", type=Path)
    parser.add_argument("--toolchain", type=Path, default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--original-root", type=Path,
                        help="Read hash-pinned originals from this checkout; retain local canonical source, recovery state and strict gate")
    parser.add_argument("--report", type=Path)
    parser.add_argument('--raw-only', action='store_true', help='independent legacy raw regression only')
    parser.add_argument('--bindings', type=Path, help='reviewed evidence for references or preflight')
    parser.add_argument('--objects', type=Path, help='recorded object report for compiler-free preflight')
    parser.add_argument('--select', nargs='+', help='explicit recovery IDs for compiler-free preflight')
    parser.add_argument('--raw', action='store_true', help='preflight unchanged-unit RAW eligibility')
    parser.add_argument('--preserve-reconstructed', action='store_true',
                        help='preflight recorded source-only reconstructed peers with --raw')
    args = parser.parse_args()
    state_before = (ROOT / "recovery.json").read_bytes()
    state = json.loads(state_before)
    rows = [r for r in state["functions"] if r["id"] == args.id] if args.id else state["functions"]
    if args.id and not rows:
        raise SystemExit(f"Unknown function {args.id}")
    if args.select:
        if args.command != 'preflight' or args.id or len(args.select) != len(set(args.select)):
            parser.error('--select requires preflight, unique IDs and no positional ID')
        selected = set(args.select)
        rows = [row for row in rows if row['id'] in selected]
        if {row['id'] for row in rows} != selected:
            parser.error('--select contains unknown recovery IDs')
    if args.raw and args.command != 'preflight':
        parser.error('--raw is only used by preflight')
    if args.preserve_reconstructed and not (args.command == 'preflight' and args.raw):
        parser.error('--preserve-reconstructed requires preflight --raw')
    if args.objects and args.command != 'preflight':
        parser.error('--objects is only used by preflight')
    if args.command == 'preflight':
        if args.objects is None or args.source or args.obj or args.raw_only:
            parser.error('preflight requires --objects, forbids --source/--obj/--raw-only')
        if args.raw:
            if args.bindings or not (args.id or args.select):
                parser.error('preflight --raw requires explicit IDs and no --bindings')
            from reference_preflight import preflight_raw
            result = preflight_raw(rows, args.objects, args.preserve_reconstructed,
                                   args.toolchain, args.original_root)
        else:
            if args.bindings is None:
                parser.error('reference preflight requires --bindings')
            from reference_preflight import preflight
            ids = set(json.loads(args.bindings.read_bytes())['functions'])
            known = {r['id'] for r in state['functions']}
            if ids - known:
                parser.error('Reference bindings contain unknown recovery IDs')
            if (args.id or args.select) and {r['id'] for r in rows} - ids:
                parser.error('Selected functions are missing reviewed bindings')
            rows = [r for r in rows if r['id'] in ids]
            if not rows:
                parser.error('No selected functions have reviewed bindings')
            result = preflight(rows, args.bindings, args.objects, args.toolchain, args.original_root)
        if (ROOT / 'recovery.json').read_bytes() != state_before:
            raise ValueError('Recovery state changed during preflight')
        if args.report:
            if not any(args.report.resolve().is_relative_to((ROOT / directory).resolve())
                       for directory in ('work', 'build')):
                parser.error('Generated reports must go under work/ or build/')
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(f"Diagnostic preflight: {'PASS' if result['ok'] else 'BLOCKED'}; "
              f"{len(result['functions'])} functions; 0 compiler calls; no acceptance proof")
        for message in result['errors']:
            print('  ' + message)
        for item in result['functions']:
            for message in item['errors']:
                print(f"  {item['id']}: {message}")
        for unit in result.get('units', []):
            for message in unit['errors']:
                print(f"  {unit['source']}: {message}")
        if not result['ok']:
            raise SystemExit(1)
        return
    if args.command == 'references':
        if args.bindings is None or args.source or args.obj:
            parser.error('references requires --bindings and canonical source')
        path = args.bindings.resolve().relative_to(ROOT.resolve()).as_posix()
        ids = set(json.loads(args.bindings.read_bytes())['functions'])
        rows = [r for r in rows if r['id'] in ids]
        if not rows:
            parser.error('No selected functions have reviewed bindings')
        results, statistics = verify_rows(rows, args.toolchain, args.original_root,
                                          relocation_candidates={r['id']: path for r in rows})
        print('Reference comparison only; recovery state is unchanged.')
    elif args.command == "object":
        if len(rows) != 1 or args.obj is None:
            parser.error("object requires one function id and --obj")
        row = rows[0]
        target = (args.original_root.resolve() if args.original_root else ROOT) / row["target"]
        if digest(target.read_bytes()) != row["target_sha256"]:
            raise ValueError(f"Target hash mismatch: {target}")
        results = [compare(args.obj, row["symbol"], target, int(row["rva"], 0), row["size"])]
    elif args.command == "candidate":
        if len(rows) != 1 or args.source is None:
            parser.error("candidate requires one function id and --source")
        results = [check(rows[0], args.source, args.toolchain, args.original_root)]
    else:
        rows = accepted_rows(rows, args.raw_only)
        if not rows:
            raise SystemExit("No accepted functions to verify")
        results, statistics = verify_rows(rows, args.toolchain, args.original_root)
        print(f"Fresh verification: {statistics['compiler_calls']} compiler calls for {statistics['rows']} rows; "
              f"{statistics['elapsed_seconds']:.3f} seconds")
    if (ROOT / "recovery.json").read_bytes() != state_before:
        raise ValueError('Recovery state changed during verification')
    if args.report:
        if not any(args.report.resolve().is_relative_to((ROOT / directory).resolve()) for directory in ("work", "build")):
            parser.error("Generated reports must go under work/ or build/")
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(results, indent=2) + "\n")
    for result in results:
        if result.get('proof_category') == RELOCATED:
            print(f"{result['id']}: {'RELOCATED EXACT' if result['exact'] else 'MISMATCH'} "
                  f"{result['target_size']} bytes; {len(result['references'])} verified references; "
                  f"{result['differing_nonreference_bytes']} non-reference differences; {result['errors']}")
            continue
        print(f"{result.get('id', args.id)}: {'EXACT' if result['exact'] else 'MISMATCH'} "
              f"{result['compiled_size']}/{result['target_size']} bytes; "
              f"{result['different_bytes']} differing bytes; "
              f"{len(result['relocations'])} object relocations, {len(result['target_relocations'])} target relocations")
        if result["mismatch_regions"]:
            print("  First mismatch intervals: " + str(result["mismatch_regions"][:12]))
    if not all(r["exact"] for r in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
