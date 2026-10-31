"""Reviewed binding renames for accepted rows (reviewed-binding-rename-v1; used by repin.py).

A source re-pin may rename a symbol that accepted rows use. This module derives, from one
hash-pinned review in evidence/, everything that may differ, and nothing else:

  definitions  {row_id: {"before", "after"}}: an accepted RAW/RELOCATED row's own symbol is
               renamed in place in its changed unit (same contribution, new name).
  bindings     {row_id: {"bindings_path", "bindings_sha256", "renames", "rebinds"}}: an accepted
               RELOCATED caller in a changed unit names a symbol differently.
      renames  {old: new}: the record is identical; only its key changes (function, import or
               file-backed data records, and local code/table records between compiler labels
               $L<digits>; storage owners keep their own names). A label is scoped to its row, a
               permutation within one row is allowed. The table declaration that names the label
               (tables[].symbol) follows the rename, and so do byte-table receipts in the gate's
               findings; a label named anywhere else in its binding is refused.
      rebinds  {old: {"symbol": new, "from": {"row", "bindings_path", "bindings_sha256"}}}: the
               new record is the accepted owner record of `from.row` verbatim (without its
               references); the caller's uses keep offset and type, and each addend is re-expressed
               so that rva + addend, the native destination, is unchanged and inside the owner.

  owner_revisions
               {source: {"before", "after", "rows": {row_id: {"bindings_path", "bindings_sha256"}}}}: a
               canonical src/ C/C++ loader-zero (bss) or file-data owner source is revised. Its pin in the
               rows' binding documents moves from `before` to `after`, in the derived descendant manifests
               only; the rows' own binding records stay verbatim. Every accepted RELOCATED row whose binding
               has an owner record in that source must be listed. The owner records are not rewritten: the
               after gate recompiles the new owner source and re-verifies names, sizes and capacities.

Each reviewed parent manifest gets one immutable descendant evidence/rename_<parent>_<review>.json
holding the derived bindings; the parent and the review are pinned in its evidence_files and the
parent stays as provenance. Every derived binding is checked against its parent by undoing exactly
the reviewed changes. The re-pin after gate then re-verifies every accepted row strictly.
"""
import copy
import hashlib
import json
import re

from env import ROOT

PROFILE = 'reviewed-binding-rename-v1'
IDENTIFIER = re.compile(r'[_?@$A-Za-z][\w@?$]*')
ACCEPTED = ('FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED')
RELOCATED = 'FUNCTION_MATCH_RELOCATED'
RENAMEABLE_KINDS = ('function', 'import', 'data')
LABEL = re.compile(r'\$L\d+')  # VC5 compiler-local label; 'local' records are renamed between these only
STORAGE_KINDS = ('bss', 'data')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def reference_digest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(',', ':')).encode())


class Plan:
    """Validated outcome: per-row record maps, row symbol renames and descendant manifests."""

    def __init__(self):
        self.definitions = {}  # row id -> new own symbol
        self.callee_names = {}  # target -> {old definition name: new}
        self.rows = {}  # row id -> {old name: (new name, addend delta)}
        self.manifests = {}  # parent path -> (new path, document)
        self.parents = {}  # row id -> (parent path, parent sha256)
        self.owner_rows = {}  # row id -> revised owner sources its binding pins
        self.owner_pins = {}  # parent path -> {source: (before, after)}
        self.review = None

    def __bool__(self):
        return bool(self.definitions or self.rows)

    def owner_revisions(self):
        """Receipt form: {source: {before, after, rows}} of the revised owner pins."""
        result = {}
        for row_id, sources in sorted(self.owner_rows.items()):
            for source in sources:
                pin = next(p[source] for p in self.owner_pins.values() if source in p)
                entry = result.setdefault(source, dict(before=pin[0], after=pin[1], rows=[]))
                entry['rows'].append(row_id)
        return result

    def binding_path(self, row_id, current):
        parent = self.parents.get(row_id)
        return self.manifests[parent[0]][0] if parent else current

    def binding_document(self, path):
        for new_path, document in self.manifests.values():
            if new_path == path:
                return document
        return None


def _identifier(value):
    return isinstance(value, str) and IDENTIFIER.fullmatch(value) is not None


def _is_label(value):
    return isinstance(value, str) and LABEL.fullmatch(value) is not None


def _mentions(value, name):
    if isinstance(value, str):
        return value == name
    if isinstance(value, dict):
        return any(key == name or _mentions(item, name) for key, item in value.items())
    if isinstance(value, list):
        return any(_mentions(item, name) for item in value)
    return False


def _check_label_rename(binding, old, new, record):
    """A local record may change its key between compiler labels and nothing else.

    VC5 numbers a unit's labels after the whole unit, so adding functions renumbers the jump-table
    and case labels of unrelated accepted rows. The record (kind, role, rva, size, evidence, uses)
    is copied verbatim by the caller; here the names and every other mention are checked.
    """
    if not (_is_label(old) and _is_label(new)):
        raise ValueError('Local records are renamed between compiler labels ($L<digits>) only: %s -> %s' % (old, new))
    if record.get('role') not in ('code', 'table'):
        raise ValueError('Only code and table label records are renamed: ' + old)
    # A table declaration may name its label in exactly one place, tables[].symbol; that field follows the
    # reviewed rename (see _rename_table_labels). Any other mention (another record, an evidence string,
    # coff_identity, another field of a table) is still refused.
    rest = {key: value for key, value in binding.items() if key not in ('symbols', 'tables')}
    tables = [{k: v for k, v in table.items() if not (k == 'symbol' and v == old)} if isinstance(table, dict) else table
              for table in binding.get('tables', [])]
    others = [value for name, value in binding.get('symbols', {}).items() if name != old]
    if _mentions(rest, old) or _mentions(tables, old) or _mentions(others, old) or _mentions(record, old):
        raise ValueError('A renamed label is also referenced by name elsewhere in its binding: ' + old)


def _rename_table_labels(tables, labels):
    """tables[].symbol follows the reviewed label renames simultaneously; the result must stay unique
    and may not take a name that an unrenamed declaration still holds."""
    if not labels:
        return tables
    renamed = copy.deepcopy(tables)
    kept = {t.get('symbol') for t in tables if isinstance(t, dict) and t.get('symbol') not in labels}
    for table in renamed:
        if isinstance(table, dict) and table.get('symbol') in labels:
            new = labels[table['symbol']]
            if new in kept:
                raise ValueError('A renamed label collides with another table declaration: ' + new)
            table['symbol'] = new
    names = [t.get('symbol') for t in renamed if isinstance(t, dict) and 'symbol' in t]
    if len(names) != len(set(names)):
        raise ValueError('Table declarations would name one label twice')
    return renamed


def _accepted_binding(row, documents, path, sha):
    if row is None or row['status'] != RELOCATED or row.get('prior_raw_proof'):
        raise ValueError('Binding renames apply to accepted RELOCATED rows only')
    if row['proof'].get('bindings_path') != path or path not in documents:
        raise ValueError('Review names a different current binding manifest for ' + row['id'])
    raw, document = documents[path]
    if digest(raw) != sha:
        raise ValueError('Reviewed parent manifest changed: ' + path)
    binding = document['functions'][row['id']]
    if reference_digest(binding) != row['proof'].get('bindings_sha256'):
        raise ValueError('Accepted binding differs from its proof: ' + row['id'])
    return binding


def _rebound_record(old, template, row, name):
    """The owner template with the caller's uses re-expressed at the same destinations."""
    if old.get('kind') not in STORAGE_KINDS or template.get('kind') != old.get('kind'):
        raise ValueError('Rebinds re-express storage references of one kind only: ' + name)
    for key in ('rva', 'size'):
        if type(old.get(key)) is not int or type(template.get(key)) is not int:
            raise ValueError('Rebound records need integer rva and size: ' + name)
    delta = old['rva'] - template['rva']
    uses = []
    for use in old['references']:
        new = dict(use, addend=use['addend'] + delta)
        end = new['addend'] == template['size'] and use.get('one_past') is True
        if not (0 <= new['addend'] < template['size'] or end) or new['addend'] > 0x7fffffff:
            raise ValueError('A rebound use must stay inside its owner: ' + name)
        if use.get('one_past') is True and not end:
            raise ValueError('A one-past use must stay the owner end: ' + name)
        uses.append(new)
    record = copy.deepcopy({k: v for k, v in template.items() if k != 'references'})
    record['references'] = uses
    anchor = template.get('identity')
    if isinstance(anchor, dict) and type(anchor.get('rva')) is int and type(anchor.get('size')) is int:
        start, size = int(row['rva'], 16), row['size']
        if anchor['rva'] < start + size and start < anchor['rva'] + anchor['size']:
            raise ValueError('A rebound owner anchor lies inside the caller: ' + name)
    return record, delta


def derive(state, review, documents, review_path, review_sha, changed_sources, source_hashes=None):
    """Validate one review against the ledger and parent manifests; return a Plan.

    documents maps every parent manifest path the review names to (raw bytes, parsed).
    source_hashes maps each changed source to its (before, after) sha256 as the re-pin plan declares
    them; owner revisions must agree with it exactly.
    """
    plan = Plan()
    plan.review = (review_path, review_sha)
    if (not isinstance(review, dict) or review.get('schema') != 1 or review.get('profile') != PROFILE or
            not isinstance(review.get('reason'), str) or not review['reason'].strip() or
            set(review) - {'schema', 'profile', 'reason', 'definitions', 'bindings', 'owner_revisions',
                           'evidence_files'}):
        raise ValueError('Invalid binding rename review')
    definitions, bindings = review.get('definitions', {}), review.get('bindings', {})
    revisions = review.get('owner_revisions', {})
    if (not isinstance(definitions, dict) or not isinstance(bindings, dict) or not isinstance(revisions, dict) or
            not (definitions or bindings or revisions)):
        raise ValueError('A binding rename review must declare definitions, bindings or owner revisions')
    rows = {r['id']: r for r in state['functions']}
    for row_id, item in sorted(definitions.items()):
        row = rows.get(row_id)
        if row is None or row['status'] not in ACCEPTED or row.get('prior_raw_proof'):
            raise ValueError('Definition renames apply to accepted RAW/RELOCATED rows only: ' + row_id)
        if (not isinstance(item, dict) or set(item) != {'before', 'after'} or item['before'] != row['symbol'] or
                not _identifier(item['after']) or item['after'] == item['before']):
            raise ValueError('Invalid definition rename: ' + row_id)
        if row['source'] not in changed_sources:
            raise ValueError('A renamed definition must be in a changed source: ' + row_id)
        names = plan.callee_names.setdefault(row['target'], {})
        if item['before'] in names or item['after'] in names.values():
            raise ValueError('Duplicate definition rename: ' + item['after'])
        names[item['before']] = item['after']
        plan.definitions[row_id] = item['after']
    for row in state['functions']:
        new = plan.definitions.get(row['id'], row.get('symbol'))
        if (row['id'] not in plan.definitions and row.get('symbol') in
                plan.callee_names.get(row['target'], {}).values()):
            raise ValueError('Definition rename collides with an existing symbol: ' + new)
    targets = {}
    for row_id, item in sorted(bindings.items()):
        row = rows.get(row_id)
        if (not isinstance(item, dict) or set(item) - {'bindings_path', 'bindings_sha256', 'renames', 'rebinds'} or
                not isinstance(item.get('renames', {}), dict) or not isinstance(item.get('rebinds', {}), dict) or
                not (item.get('renames') or item.get('rebinds'))):
            raise ValueError('Invalid binding rename: ' + row_id)
        binding = _accepted_binding(row, documents, item.get('bindings_path'), item.get('bindings_sha256'))
        if row['source'] not in changed_sources:
            raise ValueError('A re-issued caller must be in a changed source: ' + row_id)
        symbols = binding.get('symbols', {})
        if set(item.get('renames', {})) & set(item.get('rebinds', {})):
            raise ValueError('A name is either renamed or rebound: ' + row_id)
        derived = copy.deepcopy(binding)
        changes, renamed = {}, {}
        names = targets.setdefault(row['target'], dict(plan.callee_names.get(row['target'], {})))
        labels = set()  # compiler-local labels: scoped to this row, never part of the module-wide name map
        for old, new in item.get('renames', {}).items():
            record = symbols.get(old)
            if record is None or not _identifier(new) or new == old:
                raise ValueError('Invalid reviewed rename %s -> %s in %s' % (old, new, row_id))
            if record.get('kind') == 'local':
                _check_label_rename(binding, old, new, record)
                labels.add(old)
            elif record.get('kind') not in RENAMEABLE_KINDS:
                raise ValueError('Storage owner records are rebound, never renamed: ' + old)
            elif _is_label(old) or _is_label(new):
                raise ValueError('Compiler label names belong to local records only: %s -> %s' % (old, new))
            renamed[new] = copy.deepcopy(record)
            changes[old] = (new, 0)
        for old, spec in item.get('rebinds', {}).items():
            record = symbols.get(old)
            if (record is None or not isinstance(spec, dict) or set(spec) != {'symbol', 'from'} or
                    not _identifier(spec['symbol']) or spec['symbol'] == old or not isinstance(spec['from'], dict) or
                    set(spec['from']) != {'row', 'bindings_path', 'bindings_sha256'}):
                raise ValueError('Invalid reviewed rebind of %s in %s' % (old, row_id))
            source_row = rows.get(spec['from']['row'])
            source = _accepted_binding(source_row, documents, spec['from']['bindings_path'],
                                       spec['from']['bindings_sha256'])
            template = source.get('symbols', {}).get(spec['symbol'])
            if template is None or source_row['target'] != row['target']:
                raise ValueError('The rebind owner record is not an accepted record of the same module: ' + old)
            renamed[spec['symbol']], delta = _rebound_record(record, template, row, old)
            changes[old] = (spec['symbol'], delta)
        for old, (new, _) in changes.items():
            if old not in labels:
                if names.get(old, new) != new:
                    raise ValueError('One old name maps to two new names in one module: ' + old)
                names[old] = new
            del derived['symbols'][old]
        if set(renamed) & set(derived['symbols']) or len(renamed) != len(changes):
            raise ValueError('A reviewed name collides with another bound symbol in ' + row_id)
        derived['symbols'].update(renamed)
        label_map = {old: changes[old][0] for old in labels}
        if 'tables' in derived:
            derived['tables'] = _rename_table_labels(derived['tables'], label_map)
        # Independently undo exactly the reviewed changes: the parent binding must reappear.
        restored = copy.deepcopy(derived)
        if 'tables' in restored:
            restored['tables'] = _rename_table_labels(restored['tables'], {n: o for o, n in label_map.items()})
        taken = {old: restored['symbols'].pop(new) for old, (new, _) in changes.items()}  # simultaneous: labels may permute
        for old, (new, delta) in changes.items():
            record = taken[old]
            if delta or old in item.get('rebinds', {}):
                record = copy.deepcopy(symbols[old]) if _undo_rebind(record, symbols[old], delta) else None
            restored['symbols'][old] = record
        if restored != binding:
            raise ValueError('Derived binding differs from its parent beyond the reviewed names: ' + row_id)
        plan.rows[row_id] = changes
        plan.parents[row_id] = (item['bindings_path'], item['bindings_sha256'])
        _manifest(plan, documents, item['bindings_path'], row_id, derived, review_path, review_sha,
                  [documents[spec['from']['bindings_path']][1] for spec in item.get('rebinds', {}).values()])
    if revisions:
        _revise_owners(plan, state, rows, revisions, documents, bindings, changed_sources, source_hashes,
                       review_path, review_sha)
    return plan


HEX = re.compile(r'[0-9a-f]{64}')


def _canonical_owner_source(name):
    """A revised owner is an existing canonical C/C++ unit under src/, spelled exactly as it is pinned."""
    if not isinstance(name, str) or not name:
        raise ValueError('Owner revisions name repository sources')
    path = (ROOT / name).resolve()
    if (not path.is_relative_to((ROOT / 'src').resolve()) or path.suffix not in ('.c', '.cpp') or
            not path.is_file() or path.relative_to(ROOT.resolve()).as_posix() != name):
        raise ValueError('Only canonical src/ C/C++ owner sources may be revised: ' + name)
    return path


def owner_records(binding, path):
    """{name: record} of the records of one binding whose compile owner is the source at `path`."""
    found = {}
    for name, record in binding.get('symbols', {}).items():
        owner = record.get('owner') if isinstance(record, dict) else None
        if isinstance(owner, dict) and isinstance(owner.get('source'), str) and (ROOT / owner['source']).resolve() == path:
            found[name] = record
    return found


def _is_owner_record(record):
    return record.get('kind') == 'bss' or record.get('data_profile') == 'file-data-owner-v1'


def _revise_owners(plan, state, rows, revisions, documents, bindings, changed_sources, source_hashes,
                   review_path, review_sha):
    """Move the owner-source pin of every listed row's descendant manifest from `before` to `after`.

    Only that pin changes; each binding record is carried over verbatim (or with its reviewed renames,
    already derived above). The re-pin after gate recompiles the owner from the new source and verifies
    every owner record strictly, so nothing about acceptance is skipped here.
    """
    members = {}
    for source, item in sorted(revisions.items()):
        path = _canonical_owner_source(source)
        if (not isinstance(item, dict) or set(item) != {'before', 'after', 'rows'} or
                not isinstance(item['before'], str) or not HEX.fullmatch(item['before']) or
                not isinstance(item['after'], str) or not HEX.fullmatch(item['after']) or
                item['before'] == item['after'] or not isinstance(item['rows'], dict) or not item['rows']):
            raise ValueError('Invalid owner revision: ' + source)
        if source not in changed_sources:
            raise ValueError('A revised owner must be a changed source: ' + source)
        if source_hashes is None or tuple(source_hashes.get(source, ())) != (item['before'], item['after']):
            raise ValueError('Owner revision disagrees with the re-pin plan: ' + source)
        if any(row['source'] == source for row in state['functions'] if row['status'] in ACCEPTED):
            raise ValueError('A revised owner source may not also be an accepted caller unit: ' + source)
        for row_id, spec in sorted(item['rows'].items()):
            row = rows.get(row_id)
            if not isinstance(spec, dict) or set(spec) != {'bindings_path', 'bindings_sha256'}:
                raise ValueError('Invalid owner revision row: ' + row_id)
            binding = _accepted_binding(row, documents, spec['bindings_path'], spec['bindings_sha256'])
            if documents[spec['bindings_path']][1]['evidence_files'].get(source) != item['before']:
                raise ValueError('Owner revision before-hash is not the pinned hash: %s in %s' % (source, row_id))
            users = owner_records(binding, path)
            if not users or not all(_is_owner_record(record) for record in users.values()):
                raise ValueError('The row does not use %s only as a loader-zero or file-data owner: %s' % (source, row_id))
            members.setdefault(source, {})[row_id] = spec['bindings_path']
        # Completeness: a row left on the old pin would read a source it no longer matches.
        cache = {}
        for row in state['functions']:
            if row['status'] != RELOCATED or row.get('prior_raw_proof') or row['id'] in item['rows']:
                continue
            location = row['proof'].get('bindings_path')
            if location not in cache:
                cache[location] = json.loads((ROOT / location).read_bytes())
            if owner_records(cache[location]['functions'].get(row['id'], {}), path):
                raise ValueError('An accepted row also uses the revised owner and is not reviewed: ' + row['id'])
    for source, rows_of in sorted(members.items()):
        for row_id, location in rows_of.items():
            if row_id not in plan.parents:  # no rename of its own: the binding is carried over verbatim
                raw, parent = documents[location]
                _manifest(plan, documents, location, row_id, copy.deepcopy(parent['functions'][row_id]),
                          review_path, review_sha, [])
                plan.rows[row_id] = {}
                plan.parents[row_id] = (location, digest(raw))
            plan.owner_rows.setdefault(row_id, []).append(source)
            plan.owner_pins.setdefault(location, {})[source] = (revisions[source]['before'], revisions[source]['after'])
    for location, pins in plan.owner_pins.items():
        document = plan.manifests[location][1]
        for source, (before, after) in pins.items():
            if document['evidence_files'].get(source) != before:
                raise ValueError('Owner pin conflict in %s: %s' % (location, source))
            document['evidence_files'][source] = after
        _check_owner_undo(plan, documents, location, pins, review_path)


def _check_owner_undo(plan, documents, location, pins, review_path):
    """Undo exactly the reviewed owner pins: the descendant must reduce to its parent.

    Besides the pins, the descendant may differ only by its provenance (parent and review pins, the
    review name) and by the reviewed renames of its re-issued rows (each already undone and checked
    against its parent per row); an owner-only row must equal its parent binding verbatim.
    """
    _, parent = documents[location]
    descendant = copy.deepcopy(plan.manifests[location][1])
    for source, (before, after) in pins.items():
        if descendant['evidence_files'].get(source) != after:
            raise ValueError('Owner pin was not revised: ' + source)
        descendant['evidence_files'][source] = before
    for name in (location, review_path):
        descendant['evidence_files'].pop(name, None)
    descendant.pop('binding_rename_review', None)
    identity = lambda document: {k: v for k, v in document.items() if k not in ('functions', 'evidence_files')}
    if identity(descendant) != identity(parent):
        raise ValueError('Derived manifest differs from its parent beyond the reviewed change: ' + location)
    for name, sha in parent['evidence_files'].items():
        if descendant['evidence_files'].get(name) != sha:
            raise ValueError('Derived manifest changed a pinned evidence hash: %s in %s' % (name, location))
    if set(descendant['functions']) != set(parent['functions']):
        raise ValueError('Derived manifest changed its function set: ' + location)
    for row_id, binding in descendant['functions'].items():
        if binding != parent['functions'][row_id] and not plan.rows.get(row_id):
            raise ValueError('Owner revision changed a binding record: ' + row_id)


def _undo_rebind(record, original, delta):
    """True when a rebound record's uses re-expressed back equal the original uses."""
    uses = [dict(u, addend=u['addend'] - delta) for u in record['references']]
    return uses == original['references']


def _manifest(plan, documents, parent_path, row_id, binding, review_path, review_sha, sources):
    parent_raw, parent = documents[parent_path]
    parent_sha = digest(parent_raw)
    if parent_path not in plan.manifests:
        document = copy.deepcopy(parent)
        for name, sha in ((parent_path, parent_sha), (review_path, review_sha)):
            if document['evidence_files'].get(name, sha) != sha:
                raise ValueError('Conflicting immutable provenance in ' + parent_path)
            document['evidence_files'][name] = sha
        document['binding_rename_review'] = review_path
        plan.manifests[parent_path] = ('evidence/rename_%s_%s.json' % (parent_sha[:12], review_sha[:12]), document)
    document = plan.manifests[parent_path][1]
    for source in sources:  # the owner records cite evidence their own manifests pin
        for name, sha in source.get('evidence_files', {}).items():
            if document['evidence_files'].get(name, sha) != sha:
                raise ValueError('Conflicting pinned evidence for a rebound owner: ' + name)
            document['evidence_files'][name] = sha
    document['functions'][row_id] = binding


def manifest_payload(document):
    return (json.dumps(document, indent=2) + '\n').encode()


def proof_fields(document, row_id):
    """bindings_sha256 / reference_evidence_sha256 exactly as match.ReferenceSnapshot reads them."""
    identity = {k: v for k, v in document.items() if k not in ('functions', 'evidence_files')}
    return dict(bindings_sha256=reference_digest(document['functions'][row_id]),
                reference_evidence_sha256=reference_digest(dict(files=document['evidence_files'],
                                                                identity_sha256=reference_digest(identity))))


PROVENANCE = ('bindings_path', 'bindings_sha256', 'reference_evidence_sha256')


def reissued_provenance(proof):
    """The binding provenance a re-issued row must verify with: its descendant manifest's fields."""
    return {key: proof[key] for key in PROVENANCE}


def expected_findings(result, changes, compiled_sha256=None, provenance=None):
    """Old comparator findings as they must read after the reviewed renames (evidence aside).

    compiled_sha256 is the expected contribution digest when a rebind re-expresses an addend
    (the only bytes a reviewed rename may change are those relocation fields).
    provenance ({bindings_path, bindings_sha256, reference_evidence_sha256} of the descendant
    manifest, as the re-issued proof records them) replaces the parent's binding provenance, so
    the fresh result must name exactly the re-issued manifest. Without it the binding digest is
    dropped from the comparison (legacy callers).
    """
    result = copy.deepcopy(result)
    if not changes and provenance is None:
        return result
    if provenance is None:
        result.pop('bindings_sha256', None)
    else:
        if set(provenance) != set(PROVENANCE):
            raise ValueError('Re-issued provenance names exactly ' + ', '.join(PROVENANCE))
        result.update(provenance)
    if not changes:  # owner-only re-issue: just the manifest provenance moves; every receipt is compared as is
        return result
    if compiled_sha256 is not None:
        result['compiled_sha256'] = compiled_sha256
    for receipt in result.get('references', []):
        if receipt['symbol'] in changes:
            new, delta = changes[receipt['symbol']]
            receipt.update(symbol=new, addend=receipt['addend'] + delta)
        receipt.pop('evidence', None)
    # Byte-table receipts name their table label; a reviewed label rename is the only change they may show.
    for receipt in result.get('byte_table_checks', []):
        if isinstance(receipt, dict) and _is_label(receipt.get('symbol')) and receipt['symbol'] in changes:
            receipt['symbol'] = changes[receipt['symbol']][0]
    checks = result.get('source_alias_checks')
    if isinstance(checks, dict):
        for old in changes:
            checks.pop(old, None)
        if not checks:
            result.pop('source_alias_checks')
    return result


def observed_findings(result, changes, provenance=None):
    """New comparator findings in the same normalized form (provenance as in expected_findings)."""
    result = copy.deepcopy(result)
    if not changes:
        return result
    if provenance is None:
        result.pop('bindings_sha256', None)
    for receipt in result.get('references', []):
        receipt.pop('evidence', None)
    checks = result.get('source_alias_checks')
    if isinstance(checks, dict):
        for new, _ in changes.values():
            checks.pop(new, None)
        if not checks:
            result.pop('source_alias_checks')
    return result


def expected_contribution(section, changes, callee_names):
    """The before contribution with reviewed names and re-expressed addend fields applied."""
    import struct
    data = bytearray(section['data'])
    relocations = []
    for relocation in section['relocations']:
        name = relocation['symbol']
        if name in changes:
            new, delta = changes[name]
            if delta:
                struct.pack_into('<i', data, relocation['offset'],
                                 struct.unpack_from('<i', data, relocation['offset'])[0] + delta)
            name = new
        elif name in callee_names:
            name = callee_names[name]
        relocations.append(dict(relocation, symbol=name))
    return dict(size=section['size'], flags=section['flags'], sha256=digest(bytes(data)), relocations=relocations)


def load_review(plan_item):
    """The plan names the review by path and sha256; it must be durable evidence."""
    if not isinstance(plan_item, dict) or set(plan_item) != {'path', 'sha256'}:
        raise ValueError('binding_review must name {path, sha256}')
    path = (ROOT / plan_item['path']).resolve()
    if not path.is_relative_to((ROOT / 'evidence').resolve()):
        raise ValueError('Binding rename reviews belong under evidence/')
    raw = path.read_bytes()
    if digest(raw) != plan_item['sha256']:
        raise ValueError('Binding rename review hash mismatch')
    return json.loads(raw), raw


def parent_documents(review):
    """Raw bytes and parsed content of every manifest the review names."""
    paths = set()
    for item in review.get('bindings', {}).values():
        paths.add(item.get('bindings_path'))
        for spec in item.get('rebinds', {}).values():
            paths.add(spec.get('from', {}).get('bindings_path'))
    for item in review.get('owner_revisions', {}).values():
        for spec in (item.get('rows', {}) if isinstance(item, dict) else {}).values():
            paths.add(spec.get('bindings_path') if isinstance(spec, dict) else None)
    documents = {}
    for name in sorted(p for p in paths if isinstance(p, str)):
        path = (ROOT / name).resolve()
        if not path.is_relative_to((ROOT / 'evidence').resolve()):
            raise ValueError('Binding manifests must be repository evidence: ' + name)
        raw = path.read_bytes()
        documents[name] = (raw, json.loads(raw))
    return documents
