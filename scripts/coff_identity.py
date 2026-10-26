"""Opt-in VC5 scalar-deleting destructor UNDEF/COMDAT identity pair.

This resolves symbol identity only. It does not prove the function's body or
native destination; those remain the ordinary complete-contribution checks.
"""
import struct


def verify_pair(coff, name, proof):
    if (not isinstance(proof, dict) or set(proof) != {'profile', 'symbol'} or
            proof != dict(profile='msvc-undef-comdat-v1', symbol=name) or
            not name.startswith('??_G')):
        raise ValueError('Invalid explicit VC5 duplicate function identity')
    choices = [s for s in coff.symbols.values() if s['name'] == name]
    if (len(choices) != 2 or any(s['storage'] != 2 or s['type'] != 0x20 or
                               s['value'] or s['aux'] for s in choices)):
        raise ValueError('VC5 pair requires two plain external function records')
    defined = [s for s in choices if s['section'] > 0]
    if len(defined) != 1 or sum(s['section'] == 0 for s in choices) != 1:
        raise ValueError('VC5 pair requires exactly one definition and one UNDEF')
    sym = defined[0]
    section = coff.function(name)
    owners = [s for s in coff.symbols.values() if s['section'] == sym['section'] and
              s['storage'] == 3 and s['name'] == section['name'] and
              not s['value'] and not s['type'] and len(s['aux']) == 1]
    if (section['name'] != '.text' or len(section['data']) != section['size'] or
            not section['size'] or len(owners) != 1):
        raise ValueError('VC5 pair needs a complete ordinary text COMDAT')
    aux = owners[0]['aux'][0]
    if (struct.unpack_from('<I', aux)[0] != section['size'] or
            struct.unpack_from('<H', aux, 4)[0] != len(section['relocations']) or
            any(aux[6:8]) or any(aux[12:14]) or aux[14] != 2 or any(aux[15:])):
        raise ValueError('VC5 pair requires ANY non-associative COMDAT')
    # Both records are strong externals with the same linker name. COFF
    # relocations can name either index; no weak fallback or second definition
    # exists. The comparator binds this one name to exactly one destination.
    return sym
