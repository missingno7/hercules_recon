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
        row['proof']['reference_snapshots'] = dict(baseline)
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
        if path != ROOT / 'recovery.json' and not path.is_relative_to(ROOT / 'src'):
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
        require_equal(sha(state_bytes), plan['state_sha256'], 'plan state')
        before_bytes = before_path.read_bytes()
        before = json.loads(before_bytes)
        require_equal(before['state_sha256'], sha(state_bytes), 'before state')
        baseline = before.get('historical_evidence', {})
        old_results = {r['id']: r for r in before['results']}
        frozen_before(state, before['results'], original_root, baseline)
        changed = set(plan['sources'])
        source_before, candidates = {}, {}
        for name, item in plan['sources'].items():
            path = (ROOT / name).resolve()
            if not path.is_relative_to(ROOT / 'src') or path.suffix not in ('.c', '.cpp'):
                raise ValueError('Only canonical C/C++ units may change')
            source_before[name] = path.read_bytes()
            require_equal(sha(source_before[name]), item['before_sha256'], 'source base')
            candidate = (ROOT / item['candidate']).resolve()
            if not candidate.is_relative_to(ROOT / 'work'):
                raise ValueError('Candidates must be local work files')
            data = candidate.read_bytes()
            require_equal(sha(data), item['candidate_sha256'], 'candidate identity')
            candidates[path] = data
        docs = documents()
        audit = impact(state, docs, changed)
        snapshots = dict(baseline)
        for name, data in source_before.items():
            snapshots[name] = snapshot(name, data)
        # An explicit review binds EVERY direct pin, including unused documents.
        for pin in audit['direct_pins']:
            require_equal(pin['sha256'], sha(source_before[pin['path']]), 'historical source pin')
            pin.update(document_sha256=sha((ROOT/pin['document']).read_bytes()),
                       snapshot=snapshots[pin['path']],
                       usage='historical source provenance; live compiler inputs remain canonical')
        old_rows = accepted_rows(state['functions'])
        affected = []
        for row in old_rows:
            proof = row['proof']
            path = proof.get('bindings_path')
            pins = docs.get(path, {}).get('evidence_files', {})
            mapping = {n: snapshots[n] for n in snapshots if n in pins}
            if mapping:
                proof['reference_snapshots'] = mapping
            if row['source'] in changed:
                proof['source_sha256'] = sha(candidates[ROOT/row['source']])
            if isinstance(proof.get('context_sha256'), dict):
                for name in changed & set(proof['context_sha256']):
                    proof['context_sha256'][name] = sha(candidates[ROOT/name])
            if row['source'] in changed or changed & set(pins) or mapping:
                affected.append(row['id'])
            # Whole-object accepted inventories cannot be refreshed by this gate.
            if proof.get('unit_inventory_sha256') and changed & set(proof.get('context_sha256', {})):
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
        for result in before['results']:
            for p in result['compile_inputs']:
                path = Path(p)
                if path not in candidates:
                    guard[path] = path.read_bytes()
        def check_guard():
            for path, data in guard.items():
                require_equal(path.read_bytes(), data, 'frozen input '+str(path))
        def finish():
            rows = accepted_rows(state['functions'])
            # RELOCATED requests retain their pending status until the gate passes.
            rows += [r for r in selected if r['id'] in pending]
            results, statistics = match.verify_rows(rows, original_root=original_root, relocation_candidates=pending)
            if not all(r['exact'] for r in results):
                raise ValueError('Fresh after acceptance failed')
            by_id = {r['id']: r for r in results}
            require_equal(set(old_results), {r['id'] for r in old_rows}, 'accepted set')
            preserved = []
            for old in accepted_rows(old_state['functions']):
                a, b = old_results[old['id']], by_id[old['id']]
                require_equal(invariant(a), invariant(b), 'all verified findings '+old['id'])
                old_part = contribution(a['object'], old['symbol'])
                new_part = contribution(b['object'], old['symbol'])
                require_equal(old_part, new_part, 'complete contribution/ordered relocations '+old['id'])
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
                           snapshots=snapshots, preserved=preserved, results=results)
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
