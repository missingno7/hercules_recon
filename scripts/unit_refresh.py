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

reviewed-unit-delta-v3 adds reviewed function contributions of unaccepted functions:
  * added_functions: function definitions that exist only after the change. Each owns one
    new /Gy COMDAT code section, plus any COMDAT sections associative to it (.debug$F);
    nothing else may be inserted. The list must equal the repin plan's added_functions.
  * changed_functions: existing unaccepted definitions whose complete contribution group
    (code section plus associative sections, their section-definition auxiliary records
    and the symbols defined inside) changes; section names, flags and positions stay.
  * contributions: {function: {before_sha256, after_sha256}} pins the exact before/after
    group of every declared function (before_sha256 is null for an added one).
Every other section, symbol, auxiliary record and relocation stays identical, with section
numbers compared through the insertion map and v2 renames still applying.
"""
import copy
import hashlib
import json
from pathlib import Path
import re
import struct

from coff import COFF

V2_KEYS = {'profile', 'removed', 'added', 'rebases', 'renames', 'symbol_renames', 'review', 'review_sha256'}
DELTA_KEYS = {
    'reviewed-unit-delta-v1': {'profile', 'removed', 'added', 'rebases', 'review', 'review_sha256'},
    'reviewed-unit-delta-v2': V2_KEYS,
    'reviewed-unit-delta-v3': V2_KEYS | {'added_functions', 'changed_functions', 'contributions'},
}
PLAIN_UNDEF_TYPES = {'reviewed-unit-delta-v1': {0}, 'reviewed-unit-delta-v2': {0, 0x20},
                     'reviewed-unit-delta-v3': {0, 0x20}}
REL32, DIR32 = 20, 6
CODE, COMDAT, ASSOCIATIVE = 0x20, 0x1000, 5
FUNCTION_NAME = re.compile(r'[_?@$A-Za-z][\w@?$]*')


def symbol_records(coff):
    return [dict(s, aux=[a.hex() for a in s['aux']])
            for s in coff.symbols.values()]


def section_definitions(coff):
    """{section number: {'number', 'selection'}} from the section-definition symbols."""
    found = {}
    for s in coff.symbols.values():
        if (s['storage'] == 3 and s['section'] > 0 and s['value'] == 0 and len(s['aux']) == 1 and
                s['name'] == coff.sections[s['section'] - 1]['name']):
            if s['section'] in found:
                raise ValueError('Duplicate section definition symbol')
            _, _, _, _, number, selection = struct.unpack_from('<IHHIHB', bytes(s['aux'][0]))
            found[s['section']] = dict(number=number, selection=selection)
    return found


def _is_definition(coff, symbol):
    return (symbol['storage'] == 3 and symbol['section'] > 0 and symbol['value'] == 0 and len(symbol['aux']) == 1
            and symbol['name'] == coff.sections[symbol['section'] - 1]['name'])


def contribution_group(coff, function):
    """Section numbers of a function's /Gy code COMDAT and the COMDATs associative to it."""
    hits = [s for s in coff.symbols.values() if s['name'] == function and s['section'] > 0 and s['type'] & 0x20]
    if len(hits) != 1:
        raise ValueError('Expected one defined function: ' + function)
    number = hits[0]['section']
    section = coff.sections[number - 1]
    owners = [s for s in coff.symbols.values() if s['section'] == number and s['type'] & 0x20]
    if not section['flags'] & CODE or not section['flags'] & COMDAT or hits[0]['value'] != 0 or len(owners) != 1:
        raise ValueError('A declared function must own one /Gy COMDAT code section: ' + function)
    definitions = section_definitions(coff)
    return [number] + sorted(n for n, d in definitions.items() if d['selection'] == ASSOCIATIVE and d['number'] == number)


def group_digest(coff, numbers):
    """Position-independent digest of one contribution group (sections, definitions, symbols)."""
    sections = [dict(name=s['name'], flags=s['flags'], size=s['size'], data=s['data'].hex(),
                     relocations=[[r['offset'], r['symbol'], r['type']] for r in s['relocations']])
                for s in (coff.sections[n - 1] for n in numbers)]
    symbols = []
    for index in sorted(coff.symbols):
        s = coff.symbols[index]
        if s['section'] not in numbers:
            continue
        item = dict(name=s['name'], value=s['value'], position=numbers.index(s['section']), type=s['type'],
                    storage=s['storage'])
        if _is_definition(coff, s):
            length, relocations, linenumbers, checksum, number, selection = struct.unpack_from('<IHHIHB', bytes(s['aux'][0]))
            item['definition'] = dict(length=length, relocations=relocations, linenumbers=linenumbers,
                                      checksum=checksum, selection=selection,
                                      associated=numbers.index(number) if selection == ASSOCIATIVE else number)
        else:
            item['aux'] = [a.hex() for a in s['aux']]
        symbols.append(item)
    content = json.dumps(dict(sections=sections, symbols=symbols), sort_keys=True, separators=(',', ':'))
    return hashlib.sha256(content.encode()).hexdigest()


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
        if declaration['profile'] == 'reviewed-unit-delta-v3':
            # Positions are compared without the declared added groups and UNDEF deltas.
            dropped = _added_sections(declaration, after)
            a_check = [r for r in a_records if r not in declaration['removed']]
            b_check = [r for r in b_records if r['section'] not in dropped and r not in declaration['added']]
        else:
            a_check, b_check = a_records, b_records
        if (a_check.count(old) != 1 or b_check.count(new) != 1 or a_check.index(old) != b_check.index(new)):
            raise ValueError('Symbol rename must keep its exact ordered symbol-table position')
        if any(s['name'] == new['name'] for s in a_records) or any(s['name'] == old['name'] for s in b_records):
            raise ValueError('Symbol rename collides with an existing name')
        if old['name'] in names or new['name'] in names.values():
            raise ValueError('Duplicate symbol rename')
        names[old['name']] = new['name']
    return names


def _added_sections(declaration, after):
    return {n for name in declaration.get('added_functions', []) for n in contribution_group(after, name)}


def _function_changes(declaration, before, after, protected):
    """Validate v3 function declarations; return (added section numbers, changed old section numbers)."""
    added, changed, contributions = (declaration['added_functions'], declaration['changed_functions'],
                                     declaration['contributions'])
    for names in (added, changed):
        if (not isinstance(names, list) or len(set(names)) != len(names) or
                any(not isinstance(n, str) or not FUNCTION_NAME.fullmatch(n) for n in names)):
            raise ValueError('added_functions/changed_functions must list distinct COFF function symbols')
    if set(added) & set(changed) or not isinstance(contributions, dict) or set(contributions) != set(added) | set(changed):
        raise ValueError('Every declared function needs exactly one pinned contribution')
    defined_before = {s['name'] for s in before.symbols.values() if s['section'] > 0 and s['type'] & 0x20}
    added_sections, changed_sections = set(), set()
    for name in added + changed:
        pin = contributions[name]
        if not isinstance(pin, dict) or set(pin) != {'before_sha256', 'after_sha256'}:
            raise ValueError('Invalid pinned contribution: ' + name)
        if name in protected:
            raise ValueError('A declared function contribution touches an accepted contribution: ' + name)
        group = contribution_group(after, name)
        if group_digest(after, group) != pin['after_sha256']:
            raise ValueError('Declared after contribution differs: ' + name)
        if name in added:
            if name in defined_before or pin['before_sha256'] is not None:
                raise ValueError('An added function must not exist before the change: ' + name)
            added_sections.update(group)
        else:
            old_group = contribution_group(before, name)
            if group_digest(before, old_group) != pin['before_sha256']:
                raise ValueError('Declared before contribution differs: ' + name)
            changed_sections.add((tuple(old_group), tuple(group)))
    return added_sections, changed_sections


def _number(coff, name):
    """1-based section number of a defined function's contribution, by identity (not equality)."""
    section = coff.function(name)
    hits = [i for i, s in enumerate(coff.sections, 1) if s is section]
    if len(hits) != 1:
        raise ValueError('Expected one contribution section for ' + name)
    return hits[0]


def _aligned(after, added_sections):
    """After section numbers in before order (added groups removed): (list, old->new map)."""
    remaining = [n for n in range(1, len(after.sections) + 1) if n not in added_sections]
    return remaining, {old: new for old, new in enumerate(remaining, 1)}


def _placeholder(section):
    return dict(name=section['name'], flags=section['flags'], data='<declared-contribution>')


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
    v3 = profile == 'reviewed-unit-delta-v3'
    added_sections, changed_pairs = _function_changes(declaration, before, after, protected) if v3 else (set(), set())
    remaining, new_of = _aligned(after, added_sections)
    if len(before.sections) != len(remaining):
        raise ValueError('Unit section inventory changed')
    changed_old = {n for pair in changed_pairs for n in pair[0]}
    for old_group, new_group in changed_pairs:
        if [new_of[n] for n in old_group] != list(new_group):
            raise ValueError('A changed contribution must keep its section positions')
    in_place = _symbol_renames(declaration, before, after) if profile != 'reviewed-unit-delta-v1' else {}
    old = copy.deepcopy(before.sections)
    new = [copy.deepcopy(after.sections[n - 1]) for n in remaining]
    for sec in old:
        for rel in sec['relocations']:
            rel['symbol'] = in_place.get(rel['symbol'], rel['symbol'])
    references = []
    for name in protected:
        if name in in_place:
            raise ValueError('Rename touches an accepted contribution: ' + name)
        a, b = before.function(name), after.function(name)
        renamed = dict(a, relocations=[dict(r, symbol=in_place.get(r['symbol'], r['symbol'])) for r in a['relocations']])
        if renamed != b or new_of[_number(before, name)] != _number(after, name):
            raise ValueError('Accepted contribution changed: ' + name)
        references += [dict(function=name, offset=r['offset'], before_symbol=r['symbol'], after_symbol=in_place[r['symbol']])
                       for r in a['relocations'] if r['symbol'] in in_place]
    if references and not allow_protected_renames:
        raise ValueError('Accepted contribution references a renamed symbol: ' + references[0]['function'])

    def located(name):
        number = _number(before, name)
        if new_of.get(number) != _number(after, name) or number in changed_old:
            raise ValueError('Invalid or duplicate rebase/rename location: ' + name)
        return before.sections[number - 1], number - 1

    seen = set()
    for change in declaration['rebases']:
        if set(change) != {'function', 'offset', 'before_symbol', 'after_symbol', 'before_addend', 'after_addend'}:
            raise ValueError('Invalid declared DIR32 rebase')
        name, off = change['function'], change['offset']
        if name in protected:
            raise ValueError('Rebase touches an accepted contribution')
        a, ai = located(name)
        if type(off) is not int or off < 0 or off + 4 > a['size'] or (ai, off) in seen:
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
        a, ai = located(name)
        if type(off) is not int or off < 0 or off + 4 > a['size'] or (ai, off) in seen:
            raise ValueError('Invalid or duplicate rename location')
        seen.add((ai, off))
        for sections, symbol in ((old, change['before_symbol']), (new, change['after_symbol'])):
            sec = sections[ai]
            wanted = dict(offset=off, symbol=symbol, type=typ)
            if sec['relocations'].count(wanted) != 1:
                raise ValueError('Rename differs from complete COFF relocation')
            # Only the name is masked: the stored addend bytes stay in the comparison.
            sec['relocations'][sec['relocations'].index(wanted)] = dict(offset=off, symbol='<declared-rename>', type=typ)
    for number in changed_old:  # pinned by their declared digests
        old[number - 1] = _placeholder(old[number - 1])
        new[number - 1] = _placeholder(new[number - 1])
    if old != new:
        raise ValueError('Undeclared section/byte/ordered-relocation delta')
    old_of = {n: o for o, n in new_of.items()}
    a = [dict(s, name=in_place[s['name']]) if s['name'] in in_place and s.get('storage') == 2 else s
         for s in _comparable_symbols(before, changed_old, {})]
    b = _comparable_symbols(after, {new_of[n] for n in changed_old}, old_of, added_sections)
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
    if profile != 'reviewed-unit-delta-v1':
        result.update(renames=len(declaration['renames']), symbol_renames=in_place,
                      protected_renamed_references=references)
    if v3:
        result.update(added_functions=list(declaration['added_functions']),
                      changed_functions=list(declaration['changed_functions']))
    return result


def _comparable_symbols(coff, masked_sections, old_of, dropped=frozenset()):
    """Ordered symbol records in before numbering; declared groups' variable fields masked.

    Symbols defined in dropped (added) sections are removed. Section numbers, and the
    associated-section number of associative COMDAT definitions, map through old_of.
    Inside a changed group only the section-definition length/relocation/line/checksum
    fields and the symbol values are masked; names, types and order are still compared.
    """
    records = []
    for index in sorted(coff.symbols):
        s = coff.symbols[index]
        if s['section'] in dropped:
            continue
        record = dict(s, aux=[a.hex() for a in s['aux']])
        definition = _is_definition(coff, s)
        if definition:
            aux = bytearray(bytes(s['aux'][0]))
            number, selection = struct.unpack_from('<HB', aux, 12)
            if selection == ASSOCIATIVE and old_of:
                if number not in old_of:
                    raise ValueError('An existing section is associated with an added function')
                struct.pack_into('<H', aux, 12, old_of[number])
            if s['section'] in masked_sections:
                aux[0:12] = b'\0' * 12
            record['aux'] = [bytes(aux).hex()]
        elif s['section'] in masked_sections:
            record['value'] = '<declared-contribution>'
        if s['section'] > 0 and old_of:
            record['section'] = old_of[s['section']]
        records.append(record)
    return records


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


def declared_added_functions(declaration):
    """Added function definitions declared by a reviewed delta (v1/v2 declare none)."""
    if isinstance(declaration, dict) and declaration.get('profile') == 'reviewed-unit-delta-v3':
        return list(declaration.get('added_functions', []))
    return []
