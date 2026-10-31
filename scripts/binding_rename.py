"""Reviewed binding renames for accepted rows (reviewed-binding-rename-v1; used by repin.py).

A source re-pin may rename a symbol that accepted rows use. This module derives, from one
hash-pinned review in evidence/, everything that may differ, and nothing else:

  definitions  {row_id: {"before", "after"}}: an accepted RAW/RELOCATED row's own symbol is
               renamed in place in its changed unit (same contribution, new name).
  bindings     {row_id: {"bindings_path", "bindings_sha256", "renames", "rebinds"}}: an accepted
               RELOCATED caller in a changed unit names a symbol differently.
      renames  {old: new}: the record is identical; only its key changes (function, import or
               file-backed data records only; storage owners keep their own names).
      rebinds  {old: {"symbol": new, "from": {"row", "bindings_path", "bindings_sha256"}}}: the
               new record is the accepted owner record of `from.row` verbatim (without its
               references); the caller's uses keep offset and type, and each addend is re-expressed
               so that rva + addend, the native destination, is unchanged and inside the owner.

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
        self.review = None

    def __bool__(self):
        return bool(self.definitions or self.rows)

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


def derive(state, review, documents, review_path, review_sha, changed_sources):
    """Validate one review against the ledger and parent manifests; return a Plan.

    documents maps every parent manifest path the review names to (raw bytes, parsed).
    """
    plan = Plan()
    plan.review = (review_path, review_sha)
    if (not isinstance(review, dict) or review.get('schema') != 1 or review.get('profile') != PROFILE or
            not isinstance(review.get('reason'), str) or not review['reason'].strip() or
            set(review) - {'schema', 'profile', 'reason', 'definitions', 'bindings', 'evidence_files'}):
        raise ValueError('Invalid binding rename review')
    definitions, bindings = review.get('definitions', {}), review.get('bindings', {})
    if not isinstance(definitions, dict) or not isinstance(bindings, dict) or not (definitions or bindings):
        raise ValueError('A binding rename review must declare definitions or bindings')
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
        for old, new in item.get('renames', {}).items():
            record = symbols.get(old)
            if record is None or not _identifier(new) or new == old:
                raise ValueError('Invalid reviewed rename %s -> %s in %s' % (old, new, row_id))
            if record.get('kind') not in RENAMEABLE_KINDS:
                raise ValueError('Storage owner records are rebound, never renamed: ' + old)
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
            if names.get(old, new) != new:
                raise ValueError('One old name maps to two new names in one module: ' + old)
            names[old] = new
            del derived['symbols'][old]
        if set(renamed) & set(derived['symbols']) or len(renamed) != len(changes):
            raise ValueError('A reviewed name collides with another bound symbol in ' + row_id)
        derived['symbols'].update(renamed)
        # Independently undo exactly the reviewed changes: the parent binding must reappear.
        restored = copy.deepcopy(derived)
        for old, (new, delta) in changes.items():
            record = restored['symbols'].pop(new)
            if delta or old in item.get('rebinds', {}):
                record = copy.deepcopy(symbols[old]) if _undo_rebind(record, symbols[old], delta) else None
            restored['symbols'][old] = record
        if restored != binding:
            raise ValueError('Derived binding differs from its parent beyond the reviewed names: ' + row_id)
        plan.rows[row_id] = changes
        plan.parents[row_id] = (item['bindings_path'], item['bindings_sha256'])
        _manifest(plan, documents, item['bindings_path'], row_id, derived, review_path, review_sha,
                  [documents[spec['from']['bindings_path']][1] for spec in item.get('rebinds', {}).values()])
    return plan


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
    if not changes:
        return result
    if provenance is None:
        result.pop('bindings_sha256', None)
    else:
        if set(provenance) != set(PROVENANCE):
            raise ValueError('Re-issued provenance names exactly ' + ', '.join(PROVENANCE))
        result.update(provenance)
    if compiled_sha256 is not None:
        result['compiled_sha256'] = compiled_sha256
    for receipt in result.get('references', []):
        if receipt['symbol'] in changes:
            new, delta = changes[receipt['symbol']]
            receipt.update(symbol=new, addend=receipt['addend'] + delta)
        receipt.pop('evidence', None)
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
    documents = {}
    for name in sorted(p for p in paths if isinstance(p, str)):
        path = (ROOT / name).resolve()
        if not path.is_relative_to((ROOT / 'evidence').resolve()):
            raise ValueError('Binding manifests must be repository evidence: ' + name)
        raw = path.read_bytes()
        documents[name] = (raw, json.loads(raw))
    return documents
