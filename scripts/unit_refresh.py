"""Opt-in, reviewed complete COFF inventory deltas for repin.

This is a preservation proof, never acceptance of changed pending functions.
Symbol-table indices may move; section order and all undeclared content may not.
"""
import copy
import json
from pathlib import Path
import struct

from coff import COFF


def symbol_records(coff):
    return [dict(s, aux=[a.hex() for a in s['aux']])
            for s in coff.symbols.values()]


def check_delta(before, after, declaration, protected):
    """Check the exact declared delta; protected names are all accepted peers.

    The optional rebase extension admits ONLY explicitly enumerated DIR32 fields
    in unaccepted contributions. No instruction, extent, flags or other fixup
    changes are allowed. Its review records the native destination and rationale;
    it does not turn those pending contributions into function matches.
    """
    if (not isinstance(declaration, dict) or
            set(declaration) != {'profile', 'removed', 'added', 'rebases', 'review', 'review_sha256'} or
            declaration['profile'] != 'reviewed-unit-delta-v1'):
        raise ValueError('Invalid reviewed unit delta')
    old, new = copy.deepcopy(before.sections), copy.deepcopy(after.sections)
    if len(old) != len(new):
        raise ValueError('Unit section inventory changed')
    for name in protected:
        if before.function(name) != after.function(name):
            raise ValueError('Accepted contribution changed: ' + name)
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
        for sections, symbol, addend in ((old, change['before_symbol'], change['before_addend']),
                                          (new, change['after_symbol'], change['after_addend'])):
            sec = sections[ai]
            wanted = dict(offset=off, symbol=symbol, type=6)
            if sec['relocations'].count(wanted) != 1 or struct.unpack_from('<i', sec['data'], off)[0] != addend:
                raise ValueError('Rebase differs from complete COFF relocation/addend')
            # The comparison below still sees every other byte and ordered fixup.
            index = sec['relocations'].index(wanted)
            sec['relocations'][index] = dict(offset=off, symbol='<declared-rebase>', type=6)
            sec['data'] = sec['data'][:off] + b'\0' * 4 + sec['data'][off + 4:]
    if old != new:
        raise ValueError('Undeclared section/byte/ordered-relocation delta')
    a, b = symbol_records(before), symbol_records(after)
    for symbols, changes in ((a, declaration['removed']), (b, declaration['added'])):
        for symbol in changes:
            if (symbol.get('section') != 0 or symbol.get('value') != 0 or symbol.get('type') != 0 or
                    symbol.get('storage') != 2 or symbol.get('aux') != [] or symbols.count(symbol) != 1):
                raise ValueError('Only exact plain UNDEF external deltas are supported')
            symbols.remove(symbol)
    if a != b:
        raise ValueError('Undeclared ordered symbol/auxiliary delta')
    return dict(protected=sorted(protected), rebases=len(seen),
                removed=declaration['removed'], added=declaration['added'])


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
