"""Complete initialized data payloads with independently bound DIR32 fields.

Nested records carry their own identity evidence and complete use inventory.
No destination is inferred from an unverified native pointer. Cyclic graphs,
section symbols, weak aliases, BSS and non-DIR32 payload fields remain closed.
"""
import hashlib
import struct


def complete_allocation(coff, sym, extent):
    """Compare through the next object/section boundary, including any padding.

    This bounds initialized bytes, not the C type's size. Native object size is
    independently reviewed. Unlike BSS we also compare every allocated byte.
    """
    sec = coff.sections[sym['section'] - 1]
    objects = [s for s in coff.symbols.values() if s['section'] == sym['section'] and
               s['storage'] in (2, 3) and not s['aux']]
    if sum(s['value'] == sym['value'] for s in objects) != 1:
        raise ValueError('Ambiguous initialized object allocation')
    end = min([s['value'] for s in objects if s['value'] > sym['value']] + [sec['size']])
    if end - sym['value'] != extent:
        raise ValueError('Complete initialized allocation differs from reviewed extent')


def verify_payload(coff, pe, name, record, interval, bases, errors,
                   identities=None, active=None):
    identities = {} if identities is None else identities
    active = set() if active is None else active
    if name in active:
        raise ValueError('Cyclic relocated data needs separate verification')
    if record.get('data_profile') != 'relocated-data-v1':
        raise ValueError('Relocated data payload requires separate verification')
    graph = record.get('payload_bindings')
    if not isinstance(graph, dict) or set(graph) != {'symbols'} or not isinstance(graph['symbols'], dict):
        raise ValueError('Relocated data requires an explicit payload binding inventory')
    dest, extent = record['rva'], record['size']
    if type(dest) is not int or type(extent) is not int or dest < 0 or extent <= 0:
        raise ValueError('Invalid relocated data extent')
    payload = interval(dest, extent, False)
    if hashlib.sha256(payload).hexdigest() != record.get('sha256'):
        errors.append('Data payload identity differs: ' + name)
    choices = [s for s in coff.symbols.values() if s['name'] == name]
    if len(choices) != 1:
        raise ValueError('Missing or ambiguous relocated data symbol')
    sym = choices[0]
    if (not 1 <= sym['section'] <= len(coff.sections) or sym['type'] or
            sym['storage'] not in (2, 3) or sym['aux']):
        raise ValueError('Relocated data requires a plain defined object symbol')
    sec, pos = coff.sections[sym['section'] - 1], sym['value']
    complete_allocation(coff, sym, extent)
    if (sec['flags'] & (0x20 | 0x80 | 0x02000000) or
            not sec['flags'] & 0x40 or len(sec['data']) != sec['size'] or
            pos + extent > len(sec['data'])):
        raise ValueError('Relocated data storage is incomplete or executable')
    base = dest - pos
    if sym['section'] in bases and bases[sym['section']] != base:
        raise ValueError('Inconsistent independently bound COFF section addresses')
    bases[sym['section']] = base
    rels = [r for r in sec['relocations'] if r['offset'] < pos + extent and pos < r['offset'] + 4]
    records = graph['symbols']
    if not rels or set(records) != {r['symbol'] for r in rels}:
        raise ValueError('Payload binding inventory differs from COFF relocations')
    reviewed, fields = {}, set()
    active.add(name)
    for child, binding in records.items():
        if not isinstance(binding, dict):
            raise ValueError('Invalid payload binding record')
        evidence = binding.get('evidence')
        if not isinstance(evidence, list) or not evidence or any(not isinstance(e, str) or not e.strip() for e in evidence):
            raise ValueError('Missing independent payload evidence')
        address, length, kind = binding.get('rva'), binding.get('size'), binding.get('kind')
        if 'data_profile' in binding and (kind != 'data' or binding['data_profile'] != 'relocated-data-v1'):
            raise ValueError('Unsupported explicit payload data profile')
        if type(address) is not int or address < 0 or type(length) is not int or length <= 0:
            raise ValueError('Invalid payload destination extent')
        identity = (kind, address, length, binding.get('sha256'), binding.get('dll'), binding.get('name'), binding.get('ordinal'))
        if child in identities and identities[child] != identity:
            raise ValueError('Conflicting independent payload identities')
        identities[child] = identity
        candidates = [s for s in coff.symbols.values() if s['name'] == child]
        if len(candidates) != 1:
            raise ValueError('Missing or ambiguous payload symbol')
        cs = candidates[0]
        if cs['section'] < 0 or cs['storage'] not in (2, 3) or cs['aux']:
            raise ValueError('Unsupported payload section/alias symbol')
        if kind == 'function':
            if not cs['type'] & 0x20:
                raise ValueError('Payload function contradicts COFF type')
            interval(address, length, True)
            if cs['section'] and coff.function(child)['size'] != length:
                raise ValueError('Defined payload function extent differs from complete COMDAT')
        elif kind == 'data':
            if cs['type']:
                raise ValueError('Payload data contradicts COFF type')
            native = interval(address, length, False)
            if hashlib.sha256(native).hexdigest() != binding.get('sha256'):
                errors.append('Data payload identity differs: ' + child)
            if cs['section']:
                ds = coff.sections[cs['section'] - 1]
                cp = cs['value']
                complete_allocation(coff, cs, length)
                if ds['flags'] & (0x20 | 0x80) or cp + length > len(ds['data']):
                    raise ValueError('Payload data storage is incomplete or executable')
                internal = [r for r in ds['relocations'] if r['offset'] < cp + length and cp < r['offset'] + 4]
                if internal:
                    verify_payload(coff, pe, child, binding, interval, bases, errors, identities, active)
                elif ds['data'][cp:cp + length] != native:
                    errors.append('Compiled data payload differs: ' + child)
        elif kind == 'import':
            if (cs['section'] or length != 4 or bool(binding.get('name')) == ('ordinal' in binding)):
                raise ValueError('Payload import requires an exact undefined IAT cell')
            interval(address, 4, False)
            matches = [i for m in getattr(pe.pe, 'DIRECTORY_ENTRY_IMPORT', [])
                       if m.dll.decode('ascii').lower() == binding['dll'].lower() for i in m.imports
                       if i.address - pe.image_base == address and
                       (i.name and i.name.decode('ascii') == binding['name'] if 'name' in binding
                        else i.name is None and i.ordinal == binding['ordinal'])]
            if len(matches) != 1:
                raise ValueError('Payload import differs from original IAT')
        else:
            raise ValueError('Unsupported independently bound payload kind')
        if cs['section']:
            base = address - cs['value']
            if cs['section'] in bases and bases[cs['section']] != base:
                raise ValueError('Inconsistent independently bound COFF section addresses')
            bases[cs['section']] = base
        uses = binding.get('references')
        if not isinstance(uses, list) or not uses:
            raise ValueError('Missing reviewed payload relocation uses')
        for use in uses:
            if not isinstance(use, dict) or set(use) != {'offset', 'type', 'addend'} or any(type(use[k]) is not int for k in use):
                raise ValueError('Invalid reviewed payload relocation use')
            off, typ, addend = (use[k] for k in ('offset', 'type', 'addend'))
            if (off in reviewed or typ != 6 or not 0 <= addend < length or
                    kind in ('function', 'import') and addend != 0):
                raise ValueError('Invalid or duplicate payload relocation/addend')
            reviewed[off] = (child, typ, addend)
    for rel in rels:
        off, typ, child = rel['offset'] - pos, rel['type'], rel['symbol']
        cells = set(range(off, off + 4))
        if typ != 6 or off < 0 or off + 4 > extent or fields & cells:
            raise ValueError('Unsupported, overlapping or partial payload relocation')
        addend = struct.unpack_from('<i', sec['data'], pos + off)[0]
        if reviewed.get(off) != (child, typ, addend):
            raise ValueError('Payload relocation differs from reviewed use')
        if struct.unpack_from('<I', payload, off)[0] != pe.image_base + records[child]['rva'] + addend:
            errors.append('Payload relocation destination differs: ' + child)
        fields |= cells
    if set(reviewed) != {r['offset'] - pos for r in rels}:
        raise ValueError('Reviewed payload inventory differs from COFF')
    if pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size:
        native = [(e.rva - dest, e.type) for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                  for e in b.entries if e.type and dest - 3 <= e.rva < dest + extent]
        if sorted(native) != sorted((r['offset'] - pos, 3) for r in rels):
            raise ValueError('Payload HIGHLOW inventory differs from compiler DIR32 fields')
    if any(sec['data'][pos + i] != payload[i] for i in range(extent) if i not in fields):
        errors.append('Compiled data payload differs: ' + name)
    active.remove(name)
    return dict(profile='relocated-data-v1', symbol=name, bytes=extent, relocations=len(rels))
