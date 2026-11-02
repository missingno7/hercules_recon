"""Reviewed source re-pin: preserve old evidence and freshly protect acceptance."""
import argparse
import base64
import copy
import hashlib
import json
from pathlib import Path

from env import ROOT
from evidence_keys import canonical_keys, confined_path, key_identity, load_json, repository_key
from match import accepted_rows


def sha(data):
    return hashlib.sha256(data).hexdigest()


def changed_source(changed, relative):
    """The changed source a canonical evidence key names (Windows file-name identity), else None."""
    if relative is None:
        return None
    found = [name for name in changed if key_identity(name) == key_identity(relative)]
    if len(found) > 1:
        raise ValueError('Ambiguous changed sources: ' + ', '.join(sorted(found)))
    return found[0] if found else None


def impact(state, documents, changed):
    rows = accepted_rows(state['functions'])
    by_binding = {}
    for row in rows:
        path = row['proof'].get('bindings_path')
        if path:
            by_binding.setdefault(path, []).append(row['id'])
    pins = []
    for path, document in sorted(documents.items()):
        # Every spelling of a changed source is its pin: the relative key and both recorded absolute aliases.
        keys = canonical_keys(document.get('evidence_files', {}))
        for name, expected in document.get('evidence_files', {}).items():
            if changed_source(changed, keys[name]) is not None:
                pins.append(dict(document=path, path=name, sha256=expected,
                                 accepted_consumers=sorted(by_binding.get(path, []))))
    peers = sorted(row['id'] for row in rows if row.get('source') in changed)
    return dict(changed=sorted(changed), accepted_source_peers=peers, direct_pins=pins,
                publication_allowed=False, reason='Impact alone never authorizes publication')


def documents():
    result = {}
    for path in (ROOT / 'evidence').rglob('*.json'):
        doc = load_json(path.read_bytes())
        if isinstance(doc, dict) and isinstance(doc.get('evidence_files'), dict):
            result[path.relative_to(ROOT).as_posix()] = doc
    return result


def merge_frozen_inputs(frozen, observed):
    """Retain the FIRST digest of every compiler/discovery input."""
    from os.path import abspath
    for name, expected in observed.items():
        path = Path(abspath(name))
        if path in frozen:
            require_equal(frozen[path], expected, 'frozen input read again ' + str(path))
        else:
            frozen[path] = expected


def assert_frozen_inputs(frozen, replacements=None):
    """The re-pin publication guard, also used by linked dependency discovery."""
    replacements = replacements or {}
    for path, expected in frozen.items():
        expected = sha(replacements[path]) if path in replacements else expected
        require_equal(sha(path.read_bytes()), expected, 'frozen input ' + str(path))


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
    hashes = {name: (item['before_sha256'], item['candidate_sha256']) for name, item in plan['sources'].items()}
    return binding_rename.derive(state, review, binding_rename.parent_documents(review),
                                 plan['binding_review']['path'], sha(raw), changed, hashes)


CHANGEABLE_SUFFIXES = ('.c', '.cpp', '.h')


def canonical_change(name):
    """A reviewed re-pin may replace canonical C/C++ units and the canonical headers they include.

    A header has no ledger rows of its own: every accepted row whose context pins it gets the new
    hash, every evidence pin of it is snapshotted and redirected, and the complete after gate
    recompiles and protects every accepted row. Masked rows of its includers are restaged.
    """
    try:
        relative = repository_key(name)
        path = confined_path(ROOT, relative.split('/')) if relative == name else None
    except (AttributeError, ValueError):
        path = None
    if (path is None or not path.is_relative_to((ROOT / 'src').resolve()) or path.suffix not in CHANGEABLE_SUFFIXES or
            not path.is_file() or path.relative_to(ROOT.resolve()).as_posix() != name):
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


def reference_redirects(pins, source_before, candidate_hashes, snapshots):
    """(current, mapping) for one binding document's pins; exact manifest keys are preserved.

    A pin of a changed source, in any of its spellings, reads the snapshot of exactly the hash it
    pins, or the canonical file when it already pins the candidate bytes (a reviewed owner revision).
    Other pins keep an earlier re-pin's snapshot of their exact key.
    """
    keys = canonical_keys(pins)
    current, mapping = set(), {}
    for name, expected in pins.items():
        source = changed_source(source_before, keys[name])
        if source is not None:
            if expected == candidate_hashes[source]:
                current.add(name)
            else:
                mapping[name] = 'evidence/repin_snapshots/' + expected + '.txt'
        elif name in snapshots:
            mapping[name] = snapshots[name]
    return current, mapping


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


def check_owner_pins(rows, renames, document_of):
    """Exactly-once accounting of reviewed owner revisions after the per-row proof update.

    For every accepted row and every revised owner source: a manifest pin of the new bytes is never redirected
    (the live file satisfies it), a pin of the old bytes reads its preserved snapshot, a listed owner user pins
    only the new bytes, and a reviewed caller row of the unit carries the new source hash. Pins are matched by
    their pinned hash, never by key spelling. The after gate still verifies every pin against its file.
    """
    revisions = renames.owner_revisions()
    for row in rows:
        proof = row['proof']
        pins = document_of(proof.get('bindings_path')).get('evidence_files', {})
        redirects = proof.get('reference_snapshots', {})
        for source, item in revisions.items():
            moved = {k for k, v in pins.items() if v == item['after']}
            kept = {k for k, v in pins.items() if v == item['before']}
            snapshot_of_before = 'evidence/repin_snapshots/' + item['before'] + '.txt'
            if (moved & set(redirects) or any(redirects.get(k) != snapshot_of_before for k in kept) or
                    (row['id'] in item['rows'] and (kept or not moved)) or
                    (row['id'] in item.get('caller_rows', ()) and proof['source_sha256'] != item['after'])):
                raise ValueError('Preservation failed: reviewed owner pin of %s not updated exactly once: %s' %
                                 (source, row['id']))


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
    records = load_json(journal.read_bytes())
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


def compiler_profile_revisions(state, declaration, changed_sources):
    """Proposed opt-in: append one macro to accepted RAW/RELOCATED caller profiles.

    Source, owner bindings, context pins and proof categories stay unchanged.
    The existing fresh before/after gates and complete contribution/ordered
    relocation comparison remain mandatory inside the same transaction.
    Masked peers must subsequently be restaged by similar.py. Owner records
    retain their historical independent proof profiles; this is caller-only.
    """
    import re
    if declaration is None:
        return {}, None
    from profile_history import read_review
    path, expected, review = read_review(ROOT, declaration)
    name = declaration['path']
    if (set(review) != {'schema', 'profile', 'reason', 'revisions'} or review['schema'] != 1 or
            review['profile'] != 'reviewed-append-define-v1' or not review['reason'] or
            not isinstance(review['revisions'], list) or not review['revisions']):
        raise ValueError('Invalid compiler profile review')
    revisions = {}
    for item in review['revisions']:
        if set(item) != {'source', 'source_sha256', 'target', 'target_sha256', 'before', 'after', 'rows'}:
            raise ValueError('Invalid compiler profile revision')
        source = canonical_change(item['source'])
        if item['source'] in changed_sources:
            raise ValueError('Profile-only revision must not also replace its source')
        require_equal(sha(source.read_bytes()), item['source_sha256'], 'profile source hash')
        before, after = item['before'], item['after']
        if (not isinstance(before, list) or not before or
                any(not isinstance(flag, str) for flag in before) or
                not isinstance(after, list) or len(after) != len(before) + 1 or
                after[:-1] != before or not isinstance(after[-1], str) or
                not re.fullmatch(r'/D[A-Za-z_][A-Za-z0-9_]*', after[-1]) or after[-1] in before):
            raise ValueError('Only one appended plain macro define is supported')
        peers = [r for r in state['functions'] if r['source'] == item['source'] and r['target'] == item['target']]
        if any(r['status'] == 'LINKED_MATCH' or r.get('prior_raw_proof') for r in peers):
            raise ValueError('Linked or retained RAW peers require a separate reviewed profile mechanism')
        accepted = accepted_rows(peers)
        ids = item['rows']
        if (not isinstance(ids, list) or not ids or len(ids) != len(set(ids)) or
                set(ids) != {r['id'] for r in accepted} or set(ids) & set(revisions)):
            raise ValueError('Compiler profile review must name exactly all accepted source/target peers once')
        for row in accepted:
            if row['status'] not in {'FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED'} or row['proof'].get('unit_inventory_sha256'):
                raise ValueError('Unsupported accepted profile proof: ' + row['id'])
            require_equal(row['flags'], before, 'before flags ' + row['id'])
            require_equal(row['target_sha256'], item['target_sha256'], 'profile target ' + row['id'])
            require_equal(row['proof']['source_sha256'], item['source_sha256'], 'profile source proof ' + row['id'])
            revisions[row['id']] = dict(before=list(before), after=list(after))
    return revisions, dict(path=name, sha256=expected)


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
        state = load_json(state_bytes)
        old_state = copy.deepcopy(state)
        plan_bytes = plan_path.read_bytes()
        plan = load_json(plan_bytes)
        inventory_refresh = plan.get('inventory_refresh', {})
        if not isinstance(inventory_refresh, dict) or set(inventory_refresh) - set(plan['sources']):
            raise ValueError('Inventory refresh must name changed sources explicitly')
        require_equal(sha(state_bytes), plan['state_sha256'], 'plan state')
        before_bytes = before_path.read_bytes()
        before = load_json(before_bytes)
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
        from linked_repin import prepare as prepare_linked_repin
        linked_gate = prepare_linked_repin(state, plan, directory, original_root)
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
            address = pin_snapshot(pin['path'], pin['sha256'],
                                   source_before[changed_source(source_before, repository_key(pin['path']))])
            pin.update(document_sha256=sha((ROOT/pin['document']).read_bytes()),
                       snapshot=address,
                       usage='historical source provenance; live compiler inputs remain canonical')
        profile_changes, profile_review = compiler_profile_revisions(
            state, plan.get('compiler_profile_review'), changed)
        if profile_review:
            review_path = ROOT / profile_review['path']
            require_equal(sha(review_path.read_bytes()), profile_review['sha256'], 'profile review after discovery')
        old_rows = accepted_rows(state['functions'])
        before_symbols = {row['id']: row['symbol'] for row in old_rows}
        rebound = {}
        affected = []
        for row in old_rows:
            if row['id'] in profile_changes:
                revision = profile_changes[row['id']]
                row.setdefault('compiler_profile_history', []).append(dict(
                    review=profile_review, before=revision['before'], after=revision['after']))
                row['flags'] = list(revision['after'])
                affected.append(row['id'])
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
            # A reviewed owner revision pins the new bytes, which the live file already satisfies: no redirect,
            # and any older redirect of that source is dropped so the pin reads the canonical file.
            candidate_hashes = {n: sha(candidates[ROOT/n]) for n in source_before}
            current, mapping = reference_redirects(pins, source_before, candidate_hashes, snapshots)
            pinned_changes = {changed_source(changed, key) for key in canonical_keys(pins).values()} - {None}
            for n in current & set(proof.get('reference_snapshots', {})):
                del proof['reference_snapshots'][n]
            if 'reference_snapshots' in proof and not proof['reference_snapshots'] and current:
                del proof['reference_snapshots']
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
            if row['source'] in changed or pinned_changes or mapping or row['id'] in renames.rows:
                affected.append(row['id'])
            # Legacy default still refuses whole-unit changes. The opt-in gate
            # checks complete old/new inventories and all accepted contributions.
            if proof.get('unit_inventory_sha256') and changed & set(proof.get('context_sha256', {})):
                if row['source'] not in inventory_refresh or changed & set(proof['context_sha256']) != {row['source']}:
                    raise ValueError('Whole-unit change needs a separately reviewed inventory proof')
        # A unit may be both a revised owner (manifest pins, binding_rename) and an accepted caller unit (source
        # and context pins, above): check that every pin of it was written once, by its one writer.
        check_owner_pins(old_rows, renames, lambda path: renames.binding_document(path) or docs.get(path, {}))
        # LINKED_MATCH inputs are protected by the explicit complete-object gate.
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
        if profile_review:
            profile_path = ROOT / profile_review['path']
            guard[profile_path] = profile_path.read_bytes()
            require_equal(sha(guard[profile_path]), profile_review['sha256'], 'frozen profile review')
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
        frozen_guard = {}
        merge_frozen_inputs(frozen_guard, {path: sha(data) for path, data in guard.items()})
        merge_frozen_inputs(frozen_guard, linked_gate.guard)
        def check_guard():
            assert_frozen_inputs(frozen_guard)
        def finish():
            linked_receipts = linked_gate.finish(state, candidates, original_root)
            affected.extend(rid for receipt in linked_receipts for rid in receipt['proofs_before'])
            inventory_receipts = []
            if inventory_refresh:
                from unit_refresh import refresh
                inventory_receipts = refresh(accepted_rows(state['functions']), old_results,
                                            inventory_refresh, original_root,
                                            renamed={i: before_symbols[i] for i in renames.definitions},
                                            changes={k: v for k, v in renames.rows.items() if v})
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
            new_proofs = {row['id']: row['proof'] for row in accepted_rows(state['functions'])}
            for old in accepted_rows(old_state['functions']):
                a, b = old_results[old['id']], by_id[old['id']]
                changes = renames.rows.get(old['id'], {})
                # A re-issued row verifies with its descendant manifest: the fresh result must name exactly
                # the path, binding digest and evidence inventory its updated proof records. An owner-only
                # re-issue has no symbol changes but still moves to its descendant manifest.
                provenance = (binding_rename.reissued_provenance(new_proofs[old['id']])
                              if old['id'] in renames.rows else None)
                require_equal(binding_rename.expected_findings(
                                  expected_after(a, added_functions.get(old.get('source'), []), b['object']), changes,
                                  rebound.get(old['id']), provenance),
                              binding_rename.observed_findings(invariant(b), changes, provenance),
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
            linked_gate.assert_after(candidates)
            for path, data in candidates.items():
                require_equal(path.read_bytes(), data, 'candidate source after verification')
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
                           compiler_profile_review=profile_review, compiler_profile_changes=profile_changes,
                           inventory_refresh=inventory_receipts,
                           linked_object_unchanged=linked_receipts,
                           linked_discovery_inputs={str(p): h for p, h in linked_gate.inputs.items()},
                           binding_renames=dict(
                               review=list(renames.review), definitions=renames.definitions,
                               owner_revisions=renames.owner_revisions(),
                               rows={k: {o: list(v) for o, v in c.items()} for k, c in renames.rows.items()},
                               manifests={p: m[0] for p, m in renames.manifests.items()}) if renames else None)
            atomic_bytes(directory/'after.json', (json.dumps(receipt,indent=2)+'\n').encode())
            # Keep journal recovery valid across the final ledger replacement.
            records=load_json(journal.read_bytes())
            record=next(r for r in records if r['path']=='recovery.json')
            record['intermediate_sha256']=record['after_sha256']
            record['after_sha256']=sha(after)
            atomic_bytes(journal,(json.dumps(records,indent=2)+'\n').encode())
            linked_gate.assert_after(candidates)
            check_guard()
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
    result=impact(load_json(state_bytes),documents(),set(args.paths))
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
