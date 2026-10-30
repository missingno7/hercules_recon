"""Opt-in fresh reference checks using the local-byte-table profile.

No ledger writes or acceptance; match.py/promote.py defaults are unchanged.
The frozen-input build driver is copied verbatim from match.verify_rows except
for comparator dispatch, checked by a parity test.
"""
from match import *


def verify_rows(rows, toolchain=DEFAULT_TOOLCHAIN, original_root=None, relocation_candidates=None,
                historical_evidence=None):
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
    reference_snapshot = ReferenceSnapshot(original_root, historical_evidence)
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
                    if record.get('anchor_profile') == 'fixed-i386-disp32-v1':
                        from fixed_disp32 import review_inputs
                        ledger = json.loads((ROOT / 'recovery.json').read_bytes())['functions']
                        review_inputs(record, ledger, row, observed, ROOT)
            callback_calls = binding.get('indirect_calls', [])
            if not isinstance(callback_calls, list):
                raise ValueError('Invalid explicit callback inventory')
            if callback_calls:
                from callback_transfer import identity_span
                for callback in callback_calls:
                    if isinstance(callback, dict) and callback.get('profile') == 'fixed-slot-v1':
                        from fixed_slot import review_inputs
                        _, _, fixed_inputs = review_inputs(callback, target)
                        for item, sha in fixed_inputs.items():
                            if item in observed and observed[item] != sha:
                                raise ValueError('Conflicting fixed-slot input identity')
                            observed[item] = sha
                        continue
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
            validate_unit_proof(row, contexts[(source, flags)], obj, original_root)
            if row['id'] in references:
                from local_byte_tables import relocated_compare
                binding, evidence, path = references[row['id']]
                if any(record.get('anchor_profile') == 'fixed-i386-disp32-v1'
                       for record in binding.get('symbols', {}).values()):
                    from fixed_disp32 import relocated_compare
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
    parser.add_argument('--bindings', required=True, type=Path)
    parser.add_argument('--original-root', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    state_bytes = (ROOT / 'recovery.json').read_bytes()
    document = json.loads(args.bindings.read_bytes())
    path = args.bindings.resolve().relative_to(ROOT).as_posix()
    ids = set(document['functions'])
    rows = [r for r in json.loads(state_bytes)['functions'] if r['id'] in ids]
    if {r['id'] for r in rows} != ids:
        raise ValueError('Unknown binding IDs')
    results, statistics = verify_rows(rows, original_root=args.original_root,
                                     relocation_candidates={r['id']: path for r in rows})
    if (ROOT / 'recovery.json').read_bytes() != state_bytes:
        raise ValueError('Recovery changed during opt-in verification')
    report = args.report.resolve()
    if not report.is_relative_to(ROOT / 'work'):
        raise ValueError('Reports belong under work/')
    report.write_text(json.dumps(dict(results=results, statistics=statistics), indent=2) + '\n')
    print(json.dumps(dict(exact=sum(r['exact'] for r in results), **statistics)))
    if not all(r['exact'] for r in results):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
