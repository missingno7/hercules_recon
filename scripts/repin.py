"""Reviewed source re-pin: preserve old evidence and freshly protect acceptance."""
import argparse
import base64
import copy
import hashlib
import json
from pathlib import Path

from env import ROOT
from match import accepted_rows


def sha(data):
    return hashlib.sha256(data).hexdigest()


def impact(state, documents, changed):
    rows = accepted_rows(state['functions'])
    by_binding = {}
    for row in rows:
        path = row['proof'].get('bindings_path')
        if path:
            by_binding.setdefault(path, []).append(row['id'])
    pins = []
    for path, document in sorted(documents.items()):
        for name, expected in document.get('evidence_files', {}).items():
            if name in changed:
                pins.append(dict(document=path, path=name, sha256=expected,
                                 accepted_consumers=sorted(by_binding.get(path, []))))
    peers = sorted(row['id'] for row in rows if row.get('source') in changed)
    return dict(changed=sorted(changed), accepted_source_peers=peers, direct_pins=pins,
                publication_allowed=False, reason='Impact alone never authorizes publication')


def documents():
    result = {}
    for path in (ROOT / 'evidence').rglob('*.json'):
        doc = json.loads(path.read_bytes())
        if isinstance(doc, dict) and isinstance(doc.get('evidence_files'), dict):
            result[path.relative_to(ROOT).as_posix()] = doc
    return result


def contribution(path, symbol):
    from coff import COFF
    section = COFF(path).function(symbol)
    return dict(size=section['size'], flags=section['flags'],
                sha256=sha(section['data']), relocations=section['relocations'])


def invariant(result):
    """All comparator findings; only build location and provenance may differ."""
    excluded = {'object', 'object_sha256', 'command', 'source_sha256', 'compile_inputs',
                'context_sha256', 'storage_objects', 'storage_alias_objects',
                'read_only_original_path'}
    return {k: v for k, v in result.items() if k not in excluded}


def require_equal(before, after, label):
    if before != after:
        raise ValueError('Preservation failed: ' + label)


def snapshot(name, data):
    from reviewed_evidence import snapshot_path
    relative = 'evidence/repin_snapshots/' + sha(data) + '.txt'
    path = snapshot_path(ROOT, name, sha(data), {name: relative})
    if path.exists() and path.read_bytes() != data:
        raise ValueError('Snapshot content address collision')
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        path.write_bytes(data)
    return relative


def context_additions(item):
    """Reviewed, explicitly declared include-context additions of one changed source.

    Only regular files inside the manifest-pinned toolchain tree (a documented
    TU-context header such as <windows.h>) or existing canonical headers under src/
    may be added; each must match its declared hash now. Nothing is removed or
    replaced here: the full after verification still recomputes the closure and every
    accepted contribution, relocation and reference.
    """
    from match import DEFAULT_TOOLCHAIN
    declared = item.get('context_additions', {})
    if not isinstance(declared, dict):
        raise ValueError('context_additions must map paths to sha256')
    toolchain = DEFAULT_TOOLCHAIN.resolve()
    canonical = (ROOT / 'src').resolve()
    for name, expected in declared.items():
        path = Path(name)
        inside = path.is_relative_to(toolchain) or (path.is_relative_to(canonical) and path.suffix == '.h')
        if not path.is_absolute() or path.resolve() != path or not inside or not path.is_file():
            raise ValueError('Context additions must be existing toolchain files or canonical src/ headers: ' + name)
        if sha(path.read_bytes()) != expected:
            raise ValueError('Context addition hash mismatch: ' + name)
    return dict(declared)


def declared_additions(item):
    """Reviewed new function definitions of one changed source.

    Whole-unit alias receipts count the caller object's functions; a reviewed
    addition may raise that count by exactly the declared symbols, which must be
    defined functions of the fresh after object. Everything else stays identical.
    """
    added = item.get('added_functions', [])
    if (not isinstance(added, list) or len(set(added)) != len(added) or
            any(not isinstance(s, str) or not s.startswith('_') for s in added)):
        raise ValueError('added_functions must list distinct COFF function symbols')
    return added


def check_refresh_additions(plan, inventory_refresh):
    """A reviewed unit delta and the plan must declare the same added function definitions.

    added_functions in plan.sources drives the alias function-count receipts; the
    reviewed-unit-delta-v3 added_functions admits the new COMDAT groups in the unit
    inventory. Both describe one source change, so they must agree exactly.
    """
    from unit_refresh import declared_added_functions
    for name, declaration in inventory_refresh.items():
        if sorted(declared_added_functions(declaration)) != sorted(declared_additions(plan['sources'][name])):
            raise ValueError('Inventory refresh and plan disagree about added functions: ' + name)


def expected_after(result, added, after_object):
    """The before findings as they must read after the declared additions."""
    if not added or not result.get('source_alias_checks'):
        return invariant(result)
    from coff import COFF
    defined = {s['name'] for s in COFF(after_object).symbols.values() if s['type'] & 0x20 and s['section'] > 0}
    if not set(added) <= defined:
        raise ValueError('Declared added functions are not defined after the change')
    expected = copy.deepcopy(invariant(result))
    for check in expected['source_alias_checks'].values():
        if check.get('functions'):
            check['functions'] += len(added)
    return expected


def binding_renames(state, plan, changed):
    """Optional reviewed binding renames (binding_rename.py); an absent review changes nothing."""
    import binding_rename
    if 'binding_review' not in plan:
        return binding_rename.Plan()
    review, raw = binding_rename.load_review(plan['binding_review'])
    return binding_rename.derive(state, review, binding_rename.parent_documents(review),
                                 plan['binding_review']['path'], sha(raw), changed)


CHANGEABLE_SUFFIXES = ('.c', '.cpp', '.h')


def canonical_change(name):
    """A reviewed re-pin may replace canonical C/C++ units and the canonical headers they include.

    A header has no ledger rows of its own: every accepted row whose context pins it gets the new
    hash, every evidence pin of it is snapshotted and redirected, and the complete after gate
    recompiles and protects every accepted row. Masked rows of its includers are restaged.
    """
    path = (ROOT / name).resolve()
    if (not path.is_relative_to((ROOT / 'src').resolve()) or path.suffix not in CHANGEABLE_SUFFIXES or
            not path.is_file()):
        raise ValueError('Only canonical C/C++ units and headers may change')
    return path


def pin_snapshot(name, expected, current):
    """Content address satisfying one direct pin of a changed source, or refuse."""
    if expected == sha(current):
        return snapshot(name, current)
    from reviewed_evidence import snapshot_path
    relative = 'evidence/repin_snapshots/' + expected + '.txt'
    path = snapshot_path(ROOT, name, expected, {name: relative})
    if not path.exists() or sha(path.read_bytes()) != expected:
        raise ValueError('Preservation failed: historical source pin has no preserved snapshot: ' + name)
    return relative


def frozen_before(state, results, original_root, baseline):
    """Validate continuation of the completed before phase, never reuse its builds
    for after acceptance. Exact old objects remain preservation witnesses only.
    """
    import match
    from promote import assert_verified_inputs
    require_equal({r['id'] for r in accepted_rows(state['functions'])},
                  {r['id'] for r in results}, 'before accepted ID inventory')
    if not all(r['exact'] for r in results):
        raise ValueError('Before phase was not exact')
    rows = copy.deepcopy(accepted_rows(state['functions']))
    for row in rows:
        # Keep redirects recorded by earlier re-pins; the baseline only adds to them.
        row['proof']['reference_snapshots'] = {**row['proof'].get('reference_snapshots', {}), **baseline}
    assert_verified_inputs(rows, results, original_root)
    match.verify_toolchain.cache_clear()
    match.verify_toolchain(match.DEFAULT_TOOLCHAIN)
    for row in rows:
        result = next(r for r in results if r['id'] == row['id'])
        actual = contribution(result['object'], row['symbol'])
        require_equal(actual['sha256'], result['compiled_sha256'], 'before object bytes')
        if row['status'] == match.RELOCATED:
            require_equal(actual['sha256'], row['proof']['compiled_sha256'], 'before accepted bytes')
        # Complete function checks are repeated below against fresh after objects.


def transaction(entries, journal, action):
    """Recoverable publication using the existing promotion lock and atomic writer."""
    from promote import atomic_bytes
    if journal.exists():
        raise ValueError('Interrupted reviewed re-pin: recover it before retrying')
    records = []
    for path, data in entries.items():
        before = path.read_bytes() if path.exists() else None
        records.append(dict(path=path.relative_to(ROOT).as_posix(),
                            before=None if before is None else base64.b64encode(before).decode(),
                            before_sha256=None if before is None else sha(before),
                            after_sha256=sha(data)))
    atomic_bytes(journal, (json.dumps(records, indent=2)+'\n').encode())
    try:
        for path, data in entries.items():
            atomic_bytes(path, data)
        result = action()
    except BaseException:
        recover_transaction(journal)
        raise
    atomic_bytes(journal.with_name('journal_completed.json'), journal.read_bytes())
    journal.unlink()
    return result


def recover_transaction(journal):
    from promote import atomic_bytes
    records = json.loads(journal.read_bytes())
    for record in records:
        path = (ROOT / record['path']).resolve()
        created_manifest = path.is_relative_to(ROOT / 'evidence') and record['before'] is None
        if path != ROOT / 'recovery.json' and not path.is_relative_to(ROOT / 'src') and not created_manifest:
            raise ValueError('Unsafe reviewed re-pin journal destination')
        current = sha(path.read_bytes()) if path.exists() else None
        if current not in (record['before_sha256'], record['after_sha256'], record.get('intermediate_sha256')):
            raise ValueError('Concurrent edit after interrupted reviewed re-pin')
    for record in records:
        path = ROOT / record['path']
        if record['before'] is None:
            path.unlink(missing_ok=True)
        else:
            atomic_bytes(path, base64.b64decode(record['before']))
    journal.unlink()


def apply(plan_path, before_path, original_root, directory):
    import match
    from promote import lock, atomic_bytes, assert_verified_inputs, JOURNAL
    directory = directory.resolve()
    if not directory.is_relative_to(ROOT / 'work'):
        raise ValueError('Receipts belong under work/')
    directory.mkdir(parents=True, exist_ok=True)
    journal = directory / 'journal.json'
    with lock():
        if JOURNAL.exists() or journal.exists():
            raise ValueError('Recover interrupted transaction first')
        state_bytes = (ROOT / 'recovery.json').read_bytes()
        state = json.loads(state_bytes)
        old_state = copy.deepcopy(state)
        plan_bytes = plan_path.read_bytes()
        plan = json.loads(plan_bytes)
        inventory_refresh = plan.get('inventory_refresh', {})
        if not isinstance(inventory_refresh, dict) or set(inventory_refresh) - set(plan['sources']):
            raise ValueError('Inventory refresh must name changed sources explicitly')
        require_equal(sha(state_bytes), plan['state_sha256'], 'plan state')
        before_bytes = before_path.read_bytes()
        before = json.loads(before_bytes)
        require_equal(before['state_sha256'], sha(state_bytes), 'before state')
        baseline = before.get('historical_evidence', {})
        old_results = {r['id']: r for r in before['results']}
        frozen_before(state, before['results'], original_root, baseline)
        changed = set(plan['sources'])
        added_functions = {name: declared_additions(item) for name, item in plan['sources'].items()}
        check_refresh_additions(plan, inventory_refresh)
        source_before, candidates, additions = {}, {}, {}
        for name, item in plan['sources'].items():
            path = canonical_change(name)
            source_before[name] = path.read_bytes()
            require_equal(sha(source_before[name]), item['before_sha256'], 'source base')
            candidate = (ROOT / item['candidate']).resolve()
            if not candidate.is_relative_to(ROOT / 'work'):
                raise ValueError('Candidates must be local work files')
            data = candidate.read_bytes()
            require_equal(sha(data), item['candidate_sha256'], 'candidate identity')
            candidates[path] = data
            additions[name] = context_additions(item)
        docs = documents()
        audit = impact(state, docs, changed)
        import binding_rename
        renames = binding_renames(state, plan, changed)
        snapshots = dict(baseline)
        for name, data in source_before.items():
            snapshots[name] = snapshot(name, data)
        # An explicit review binds EVERY direct pin, including unused documents.
        for pin in audit['direct_pins']:
            # A pin of the current bytes is snapshotted now; a pin of an older version must
            # already be preserved, by an earlier re-pin, at its own content address.
            address = pin_snapshot(pin['path'], pin['sha256'], source_before[pin['path']])
            pin.update(document_sha256=sha((ROOT/pin['document']).read_bytes()),
                       snapshot=address,
                       usage='historical source provenance; live compiler inputs remain canonical')
        old_rows = accepted_rows(state['functions'])
        before_symbols = {row['id']: row['symbol'] for row in old_rows}
        rebound = {}
        affected = []
        for row in old_rows:
            proof = row['proof']
            if row['id'] in renames.definitions:
                row.setdefault('symbol_history', []).append(dict(review=renames.review[0], before=row['symbol']))
                row['symbol'] = renames.definitions[row['id']]
            if row['id'] in renames.rows:
                # The parent manifest stays provenance; the descendant carries the reviewed names.
                parent, parent_sha = renames.parents[row['id']]
                new_path, document = renames.manifests[parent]
                row.setdefault('reference_rename_history', []).append(dict(
                    review=renames.review[0], review_sha256=renames.review[1],
                    parent_bindings_sha256=parent_sha, proof=copy.deepcopy(proof)))
                proof['bindings_path'] = new_path
                proof.update(binding_rename.proof_fields(document, row['id']))
                if any(delta for _, delta in renames.rows[row['id']].values()):
                    # A rebind re-expresses addend fields; the fresh gate must reproduce exactly these bytes.
                    from coff import COFF
                    section = COFF(old_results[row['id']]['object']).function(row['symbol'])
                    rebound[row['id']] = binding_rename.expected_contribution(
                        section, renames.rows[row['id']], {})['sha256']
                    proof['compiled_sha256'] = rebound[row['id']]
            path = proof.get('bindings_path')
            pins = (renames.binding_document(path) or docs.get(path, {})).get('evidence_files', {})
            # Each pin of a changed source reads the snapshot of exactly the hash it pins.
            mapping = {n: ('evidence/repin_snapshots/' + pins[n] + '.txt' if n in source_before else snapshots[n])
                       for n in snapshots if n in pins}
            if mapping:
                # Merge: a row may already read older pins through earlier snapshots.
                proof['reference_snapshots'] = {**proof.get('reference_snapshots', {}), **mapping}
            if row['source'] in changed:
                proof['source_sha256'] = sha(candidates[ROOT/row['source']])
            if isinstance(proof.get('context_sha256'), dict):
                for name in changed & set(proof['context_sha256']):
                    proof['context_sha256'][name] = sha(candidates[ROOT/name])
                # A reviewed context change only ADDS declared toolchain headers; the after
                # gate recomputes the real closure, so an undeclared or wrong header fails.
                for name in changed & set(proof['context_sha256']):
                    if additions.get(name):
                        # Same keys as match.context_proof: repository files relative, toolchain absolute.
                        keyed = {(Path(k).relative_to(ROOT).as_posix() if Path(k).is_relative_to(ROOT) else k): v
                                 for k, v in additions[name].items()}
                        if set(keyed) & set(proof['context_sha256']):
                            raise ValueError('Declared context addition is already pinned: ' + row['id'])
                        proof['context_sha256'].update(keyed)
            if row['source'] in changed or changed & set(pins) or mapping or row['id'] in renames.rows:
                affected.append(row['id'])
            # Legacy default still refuses whole-unit changes. The opt-in gate
            # checks complete old/new inventories and all accepted contributions.
            if proof.get('unit_inventory_sha256') and changed & set(proof.get('context_sha256', {})):
                if row['source'] not in inventory_refresh or changed & set(proof['context_sha256']) != {row['source']}:
                    raise ValueError('Whole-unit change needs a separately reviewed inventory proof')
        # Retained raw proof dictionaries are references to their canonical fields.
        for row in state['functions']:
            if row['status'] == 'LINKED_MATCH' and row['source'] in changed:
                raise ValueError('Linked source change needs linked re-proof')
        pending = {}
        selected = []
        for request in plan['promotions']:
            if 'row' in request:
                row = copy.deepcopy(request['row'])
                if any(r['id'] == row['id'] for r in state['functions']):
                    raise ValueError('New registration ID already exists')
                state['functions'].append(row)
            else:
                row = next(r for r in state['functions'] if r['id'] == request['id'])
            if row['status'] in match.ACCEPTED or row.get('prior_raw_proof'):
                raise ValueError('Cannot replace existing accepted status')
            selected.append(row)
            row['proof'] = dict(source_sha256=sha(candidates.get(ROOT/row['source'], (ROOT/row['source']).read_bytes())))
            if request['mode'] == 'relocated':
                binding_path = request['bindings']
                if not (ROOT/binding_path).resolve().is_relative_to(ROOT/'evidence'):
                    raise ValueError('New bindings must be durable repository evidence')
                pending[row['id']] = binding_path
            elif request['mode'] == 'raw':
                row['status'] = 'FUNCTION_MATCH'
            else:
                raise ValueError('Unknown acceptance mode')
        # Protect unchanged evidence, all source/include closures and old witnesses
        # across the write/verify/publication interval.
        guard = {ROOT/'toolchains/manifest.json': (ROOT/'toolchains/manifest.json').read_bytes(),
                 plan_path: plan_bytes, before_path: before_bytes}
        for p in docs:
            guard[ROOT/p] = (ROOT/p).read_bytes()
        if 'binding_review' in plan:
            review_file = (ROOT / plan['binding_review']['path']).resolve()
            guard[review_file] = review_file.read_bytes()
        manifests = {}
        for new_path, document in renames.manifests.values():
            payload = binding_rename.manifest_payload(document)
            if (ROOT / new_path).exists():
                if (ROOT / new_path).read_bytes() != payload:
                    raise ValueError('Conflicting immutable descendant manifest: ' + new_path)
                continue
            manifests[ROOT / new_path] = payload
        for declaration in inventory_refresh.values():
            review = (ROOT / declaration['review']).resolve()
            if not review.is_relative_to(ROOT / 'evidence'):
                raise ValueError('Inventory reviews belong under evidence/')
            guard[review] = review.read_bytes()
        for result in before['results']:
            for p in result['compile_inputs']:
                path = Path(p)
                if path not in candidates:
                    guard[path] = path.read_bytes()
        def check_guard():
            for path, data in guard.items():
                require_equal(path.read_bytes(), data, 'frozen input '+str(path))
        def finish():
            inventory_receipts = []
            if inventory_refresh:
                from unit_refresh import refresh
                inventory_receipts = refresh(accepted_rows(state['functions']), old_results,
                                            inventory_refresh, original_root,
                                            renamed={i: before_symbols[i] for i in renames.definitions},
                                            changes=renames.rows)
            rows = accepted_rows(state['functions'])
            # RELOCATED requests retain their pending status until the gate passes.
            rows += [r for r in selected if r['id'] in pending]
            results, statistics = match.verify_rows(rows, original_root=original_root, relocation_candidates=pending)
            if not all(r['exact'] for r in results):
                raise ValueError('Fresh after acceptance failed')
            by_id = {r['id']: r for r in results}
            require_equal(set(old_results), {r['id'] for r in old_rows}, 'accepted set')
            preserved = []
            from coff import COFF
            for old in accepted_rows(old_state['functions']):
                a, b = old_results[old['id']], by_id[old['id']]
                changes = renames.rows.get(old['id'], {})
                require_equal(binding_rename.expected_findings(
                                  expected_after(a, added_functions.get(old.get('source'), []), b['object']), changes,
                                  rebound.get(old['id'])),
                              binding_rename.observed_findings(invariant(b), changes),
                              'all verified findings '+old['id'])
                callee = renames.callee_names.get(old['target'], {})
                old_part = binding_rename.expected_contribution(
                    COFF(a['object']).function(old['symbol']), changes,
                    callee if old['source'] in changed or old['id'] in renames.rows else {})
                new_part = contribution(b['object'], renames.definitions.get(old['id'], old['symbol']))
                require_equal(old_part, new_part, 'complete contribution/ordered relocations '+old['id'])
                if {r['symbol'] for r in new_part['relocations']} & set(callee):
                    raise ValueError('Renamed definition still referenced by accepted ' + old['id'])
                preserved.append(dict(id=old['id'], **new_part))
            assert_verified_inputs(rows, results, original_root)
            check_guard()
            require_equal((ROOT/'recovery.json').read_bytes(), staged_state, 'staged state')
            for row in selected:
                result = by_id[row['id']]
                relocated = row['id'] in pending
                row['status'] = match.RELOCATED if relocated else 'FUNCTION_MATCH'
                fields = ['scope','source_sha256','compiled_sha256','target_sha256','toolchain']
                if relocated:
                    fields += ['bindings_path','bindings_sha256','reference_evidence_sha256','context_sha256']
                row['proof'] = {k: result[k] for k in fields}
            after = (json.dumps(state, indent=2)+'\n').encode()
            receipt = dict(schema=1, plan_sha256=sha(plan_bytes), before_report_sha256=sha(before_bytes),
                           state_before_sha256=sha(state_bytes), state_after_sha256=sha(after),
                           accepted_before=sorted(old_results), promoted=sorted(r['id'] for r in selected),
                           affected=sorted(affected), statistics=statistics, pins=audit['direct_pins'],
                           snapshots=snapshots, preserved=preserved, results=results,
                           inventory_refresh=inventory_receipts,
                           binding_renames=dict(
                               review=list(renames.review), definitions=renames.definitions,
                               rows={k: {o: list(v) for o, v in c.items()} for k, c in renames.rows.items()},
                               manifests={p: m[0] for p, m in renames.manifests.items()}) if renames else None)
            atomic_bytes(directory/'after.json', (json.dumps(receipt,indent=2)+'\n').encode())
            # Keep journal recovery valid across the final ledger replacement.
            records=json.loads(journal.read_bytes())
            record=next(r for r in records if r['path']=='recovery.json')
            record['intermediate_sha256']=record['after_sha256']
            record['after_sha256']=sha(after)
            atomic_bytes(journal,(json.dumps(records,indent=2)+'\n').encode())
            atomic_bytes(ROOT/'recovery.json',after)
            return receipt
        staged_state = (json.dumps(state,indent=2)+'\n').encode()
        entries = dict(candidates)
        entries.update(manifests)
        entries[ROOT/'recovery.json'] = staged_state
        check_guard()
        receipt = transaction(entries,journal,finish)
        print(json.dumps(dict(promoted=receipt['promoted'],preserved=len(old_results),statistics=receipt['statistics'])))
        return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--plan', type=Path)
    parser.add_argument('--before', type=Path)
    parser.add_argument('--original-root', type=Path)
    parser.add_argument('--directory',type=Path, default=ROOT/'work/repin')
    parser.add_argument('--recover', action='store_true')
    args = parser.parse_args()
    if args.recover:
        from promote import lock
        with lock():
            recover_transaction(args.directory/'journal.json')
        return
    if args.plan:
        if not args.before or not args.original_root:
            parser.error('--plan requires --before and --original-root')
        apply(args.plan.resolve(),args.before.resolve(),args.original_root.resolve(),args.directory)
        return
    if not args.paths:
        parser.error('Supply paths for audit or --plan for reviewed publication')
    state_bytes=(ROOT/'recovery.json').read_bytes()
    result=impact(json.loads(state_bytes),documents(),set(args.paths))
    result['state_sha256']=sha(state_bytes)
    result['input_sha256']={p:sha((ROOT/p).read_bytes()) for p in args.paths}
    text=json.dumps(result,indent=2)+'\n'
    if args.report:
        if not args.report.resolve().is_relative_to(ROOT/'work'):
            parser.error('Reports belong under work/')
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(text,encoding='utf-8')
    print(text,end='')


if __name__ == '__main__':
    main()
