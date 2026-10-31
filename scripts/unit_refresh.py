"""Opt-in, reviewed complete COFF inventory deltas for repin.

This is a preservation proof, never acceptance of changed pending functions.
Symbol-table indices may move; section order and all undeclared content may not.

reviewed-unit-delta-v1 (unchanged): exact plain UNDEF data externals removed/added and
enumerated DIR32 rebases in unaccepted contributions.

reviewed-unit-delta-v2 adds identifier renames that change no generated code:
  * symbol_renames: in-place renames of one external symbol record. The record keeps its
    ordered symbol-table position, value, section, type, storage class and auxiliary
    records; only the name differs, and every relocation naming it follows the rename.
  * renames: enumerated DIR32 or REL32 relocation symbol renames in unaccepted
    contributions (for a new external that does not keep the old position); the stored
    addend bytes stay part of the comparison.
  * removed/added may also list plain UNDEF function externals (COFF type 0x20).
An accepted contribution may never change. By default it may not even reference a renamed
symbol (its reviewed binding names the old symbol); allow_protected_renames=True admits that
only for a caller that re-issues those bindings, and the references are reported.
"""
import copy
import json
from pathlib import Path
import struct

from coff import COFF

DELTA_KEYS = {
    'reviewed-unit-delta-v1': {'profile', 'removed', 'added', 'rebases', 'review', 'review_sha256'},
    'reviewed-unit-delta-v2': {'profile', 'removed', 'added', 'rebases', 'renames', 'symbol_renames',
                               'review', 'review_sha256'},
}
PLAIN_UNDEF_TYPES = {'reviewed-unit-delta-v1': {0}, 'reviewed-unit-delta-v2': {0, 0x20}}
REL32, DIR32 = 20, 6


def symbol_records(coff):
    return [dict(s, aux=[a.hex() for a in s['aux']])
            for s in coff.symbols.values()]


def _symbol_renames(declaration, before, after):
    """Validate declared in-place renames; return {before_name: after_name}."""
    names = {}
    a_records, b_records = symbol_records(before), symbol_records(after)
    for change in declaration.get('symbol_renames', []):
        if not isinstance(change, dict) or set(change) != {'before', 'after'}:
            raise ValueError('Invalid declared symbol rename')
        old, new = change['before'], change['after']
        if (not isinstance(old, dict) or not isinstance(new, dict) or set(old) != set(new) or
                {k: v for k, v in old.items() if k != 'name'} != {k: v for k, v in new.items() if k != 'name'} or
                not isinstance(old.get('name'), str) or not isinstance(new.get('name'), str) or
                old['name'] == new['name'] or old.get('storage') != 2):
            raise ValueError('Symbol rename must change only the name of one external symbol record')
        if (a_records.count(old) != 1 or b_records.count(new) != 1 or
                a_records.index(old) != b_records.index(new)):
            raise ValueError('Symbol rename must keep its exact ordered symbol-table position')
        if any(s['name'] == new['name'] for s in a_records) or any(s['name'] == old['name'] for s in b_records):
            raise ValueError('Symbol rename collides with an existing name')
        if old['name'] in names or new['name'] in names.values():
            raise ValueError('Duplicate symbol rename')
        names[old['name']] = new['name']
    return names


def check_delta(before, after, declaration, protected, allow_protected_renames=False):
    """Check the exact declared delta; protected names are all accepted peers.

    The optional rebase extension admits ONLY explicitly enumerated DIR32 fields
    in unaccepted contributions. No instruction, extent, flags or other fixup
    changes are allowed. Its review records the native destination and rationale;
    it does not turn those pending contributions into function matches.
    """
    if (not isinstance(declaration, dict) or declaration.get('profile') not in DELTA_KEYS or
            set(declaration) != DELTA_KEYS[declaration['profile']]):
        raise ValueError('Invalid reviewed unit delta')
    profile = declaration['profile']
    old, new = copy.deepcopy(before.sections), copy.deepcopy(after.sections)
    if len(old) != len(new):
        raise ValueError('Unit section inventory changed')
    in_place = _symbol_renames(declaration, before, after) if profile == 'reviewed-unit-delta-v2' else {}
    for sec in old:
        for rel in sec['relocations']:
            rel['symbol'] = in_place.get(rel['symbol'], rel['symbol'])
    references = []
    for name in protected:
        if name in in_place:
            raise ValueError('Rename touches an accepted contribution: ' + name)
        a, b = before.function(name), after.function(name)
        renamed = dict(a, relocations=[dict(r, symbol=in_place.get(r['symbol'], r['symbol'])) for r in a['relocations']])
        if renamed != b:
            raise ValueError('Accepted contribution changed: ' + name)
        references += [dict(function=name, offset=r['offset'], before_symbol=r['symbol'], after_symbol=in_place[r['symbol']])
                       for r in a['relocations'] if r['symbol'] in in_place]
    if references and not allow_protected_renames:
        raise ValueError('Accepted contribution references a renamed symbol: ' + references[0]['function'])
    seen = set()
    for change in declaration['rebases']:
        if set(change) != {'function', 'offset', 'before_symbol', 'after_symbol', 'before_addend', 'after_addend'}:
            raise ValueError('Invalid declared DIR32 rebase')
        name, off = change['function'], change['offset']
        if name in protected:
            raise ValueError('Rebase touches an accepted contribution')
        a, b = before.function(name), after.function(name)
        ai, bi = before.sections.index(a), after.sections.index(b)
        if ai != bi or type(off) is not int or off < 0 or off + 4 > a['size'] or (ai, off) in seen:
            raise ValueError('Invalid or duplicate rebase location')
        seen.add((ai, off))
        for sections, symbol, addend in ((old, in_place.get(change['before_symbol'], change['before_symbol']),
                                          change['before_addend']),
                                         (new, change['after_symbol'], change['after_addend'])):
            sec = sections[ai]
            wanted = dict(offset=off, symbol=symbol, type=DIR32)
            if sec['relocations'].count(wanted) != 1 or struct.unpack_from('<i', sec['data'], off)[0] != addend:
                raise ValueError('Rebase differs from complete COFF relocation/addend')
            # The comparison below still sees every other byte and ordered fixup.
            index = sec['relocations'].index(wanted)
            sec['relocations'][index] = dict(offset=off, symbol='<declared-rebase>', type=DIR32)
            sec['data'] = sec['data'][:off] + b'\0' * 4 + sec['data'][off + 4:]
    for change in declaration.get('renames', []):
        if set(change) != {'function', 'offset', 'type', 'before_symbol', 'after_symbol'}:
            raise ValueError('Invalid declared relocation rename')
        name, off, typ = change['function'], change['offset'], change['type']
        if name in protected:
            raise ValueError('Rename touches an accepted contribution')
        if (typ not in (REL32, DIR32) or change['before_symbol'] == change['after_symbol'] or
                change['before_symbol'] in in_place):
            raise ValueError('Invalid declared relocation rename')
        a, b = before.function(name), after.function(name)
        ai, bi = before.sections.index(a), after.sections.index(b)
        if ai != bi or type(off) is not int or off < 0 or off + 4 > a['size'] or (ai, off) in seen:
            raise ValueError('Invalid or duplicate rename location')
        seen.add((ai, off))
        for sections, symbol in ((old, change['before_symbol']), (new, change['after_symbol'])):
            sec = sections[ai]
            wanted = dict(offset=off, symbol=symbol, type=typ)
            if sec['relocations'].count(wanted) != 1:
                raise ValueError('Rename differs from complete COFF relocation')
            # Only the name is masked: the stored addend bytes stay in the comparison.
            sec['relocations'][sec['relocations'].index(wanted)] = dict(offset=off, symbol='<declared-rename>', type=typ)
    if old != new:
        raise ValueError('Undeclared section/byte/ordered-relocation delta')
    a, b = symbol_records(before), symbol_records(after)
    a = [dict(s, name=in_place[s['name']]) if s['name'] in in_place and s.get('storage') == 2 else s for s in a]
    for symbols, changes in ((a, declaration['removed']), (b, declaration['added'])):
        for symbol in changes:
            if (symbol.get('section') != 0 or symbol.get('value') != 0 or
                    symbol.get('type') not in PLAIN_UNDEF_TYPES[profile] or
                    symbol.get('storage') != 2 or symbol.get('aux') != [] or symbols.count(symbol) != 1):
                raise ValueError('Only exact plain UNDEF external deltas are supported')
            symbols.remove(symbol)
    if a != b:
        raise ValueError('Undeclared ordered symbol/auxiliary delta')
    result = dict(protected=sorted(protected), rebases=len(declaration['rebases']),
                  removed=declaration['removed'], added=declaration['added'])
    if profile == 'reviewed-unit-delta-v2':
        result.update(renames=len(declaration['renames']), symbol_renames=in_place,
                      protected_renamed_references=references)
    return result


def refresh(rows, old_results, declarations, original_root):
    """Called inside repin's rollback transaction, before its fresh full gate.

    The prior fresh before report is checked by repin.frozen_before. Each new
    unit is built from its canonical path; old objects never grant acceptance.
    """
    import match
    from env import ROOT
    groups = {}
    for row in rows:
        if row['source'] in declarations:
            groups.setdefault((row['source'], tuple(row['flags'])), []).append(row)
    if set(declarations) != {s for s, _ in groups}:
        raise ValueError('Unit refresh must name accepted source units')
    receipts = []
    for (source, flags), peers in groups.items():
        declaration = declarations[source]
        review = (ROOT / declaration['review']).resolve()
        if not review.is_relative_to(ROOT / 'evidence') or match.digest(review.read_bytes()) != declaration['review_sha256']:
            raise ValueError('Unit delta review must be hash-pinned durable evidence')
        review_doc = json.loads(review.read_bytes())
        declared = {k: v for k, v in declaration.items() if k not in ('review', 'review_sha256')}
        if review_doc.get('units', {}).get(source, {}).get('declaration') != declared:
            raise ValueError('Unit delta differs from independently recorded review')
        obj, command = match.compile_source(ROOT / source, list(flags))
        old_objects = {old_results[r['id']]['object'] for r in peers}
        if len(old_objects) != 1:
            raise ValueError('Before peers must share one freshly verified object')
        old_path = next(iter(old_objects))
        if any(match.digest(Path(old_path).read_bytes()) != old_results[r['id']]['object_sha256']
               for r in peers):
            raise ValueError('Before unit object changed after verification')
        before = COFF(old_path)
        after = COFF(obj)
        delta = check_delta(before, after, declaration, {r['symbol'] for r in peers})
        raw, portable = match.unit_inventory(obj, original_root), match.portable_unit_inventory(obj)
        for row in peers:
            proof = row['proof']
            if 'unit_inventory_sha256' not in proof:
                continue
            history = {k: copy.deepcopy(proof[k]) for k in
                       ('unit_inventory_sha256', 'unit_inventory_portable_sha256') if k in proof}
            history.update(review=declaration['review'], review_sha256=declaration['review_sha256'],
                           declaration_sha256=match.reference_digest(declaration))
            proof.setdefault('unit_inventory_history', []).append(history)
            proof.update(unit_inventory_sha256=raw, unit_inventory_portable_sha256=portable)
        receipts.append(dict(source=source, flags=list(flags), object=str(obj), command=command,
                             object_sha256=match.digest(obj.read_bytes()), delta=delta,
                             unit_inventory_sha256=raw, unit_inventory_portable_sha256=portable))
    return receipts
