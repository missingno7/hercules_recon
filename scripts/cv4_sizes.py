"""Declared data extents from the C11 debug records emitted by VC5 /Z7."""

from functools import lru_cache
from struct import calcsize, unpack_from

C11 = b'\x02\x00\x00\x00'
S_LDATA32, S_GDATA32 = 0x1007, 0x1008  # C11 counted names
LF_MODIFIER, LF_POINTER, LF_ARRAY = 0x1001, 0x1002, 0x1003
LF_CLASS, LF_STRUCTURE, LF_UNION, LF_ENUM = 0x1004, 0x1005, 0x1006, 0x1007
SECREL, SECTION = 0x000B, 0x000A

PRIMITIVE = {
    0x10: 1, 0x20: 1, 0x30: 1, 0x68: 1, 0x69: 1, 0x70: 1,
    0x11: 2, 0x21: 2, 0x31: 2, 0x71: 2, 0x72: 2, 0x73: 2,
    0x12: 4, 0x22: 4, 0x32: 4, 0x40: 4, 0x45: 4, 0x74: 4, 0x75: 4,
    0x13: 8, 0x23: 8, 0x33: 8, 0x41: 8, 0x76: 8, 0x77: 8,
    0x42: 10, 0x43: 16, 0x44: 6, 0x46: 2,
}
SIMPLE_POINTER = {1: 2, 2: 4, 3: 4, 4: 4, 5: 6}
POINTER = {0: 2, 1: 4, 2: 4, 10: 4, 11: 6}
NUMERIC = {0x8000: '<b', 0x8001: '<h', 0x8002: '<H', 0x8003: '<i', 0x8004: '<I',
           0x8009: '<q', 0x800a: '<Q'}


def records(data, offset):
    """Yield (record kind, payload offset, payload), with offsets relative to the section."""
    while offset < len(data):
        if offset + 4 > len(data):
            raise ValueError('truncated CodeView record header')
        length, kind = unpack_from('<HH', data, offset)
        end = offset + 2 + length
        if length < 2 or end > len(data):
            raise ValueError('malformed CodeView record length')
        yield kind, offset + 4, data[offset + 4:end]
        offset = end


def numeric(data, offset):
    if offset + 2 > len(data):
        return None, offset
    leaf = unpack_from('<H', data, offset)[0]
    offset += 2
    if leaf < 0x8000:
        return leaf, offset
    form = NUMERIC.get(leaf)
    if not form or offset + calcsize(form) > len(data):
        return None, offset
    return unpack_from(form, data, offset)[0], offset + calcsize(form)


def counted_name(data, offset):
    """Check the counted name and any descending LF_PAD bytes after it."""
    if offset >= len(data):
        return False
    end = offset + 1 + data[offset]
    return end <= len(data) and len(data) - end < 16 and all(
        byte == 0xf0 + len(data) - i for i, byte in enumerate(data[end:], end))


def type_sizes(data):
    """Return a memoized resolver for the type indexes in one C11 stream."""
    if data and not data.startswith(C11):
        raise ValueError('expected C11 type signature')
    types = {}
    for index, (kind, _, payload) in enumerate(records(data, len(C11)), 0x1000):
        # Type streams that import or skip indexes need their own index mapping.
        if kind in (0x0013, 0x0014, 0x0016, 0x0200, 0x100e, 0x1200,
                    0x1501, 0x1509, 0x1515):
            raise ValueError('unsupported CodeView type index mapping')
        types[index] = (kind, payload)

    active = set()

    @lru_cache(maxsize=None)
    def size(index):
        if index < 0x1000:
            if index & 0x800:
                return None
            base, mode = index & 0xff, index >> 8
            if mode:
                return SIMPLE_POINTER.get(mode) if base in PRIMITIVE or base == 3 else None
            return PRIMITIVE.get(base)
        if index in active or index not in types or len(active) >= 64:
            return None
        active.add(index)
        try:
            return complex_size(*types[index])
        finally:
            active.remove(index)

    def complex_size(kind, payload):
        if kind == LF_MODIFIER and len(payload) >= 6:
            return size(unpack_from('<I', payload)[0])
        if kind == LF_POINTER and len(payload) >= 8:
            _, attr = unpack_from('<II', payload)
            pointer, mode, declared = attr & 31, (attr >> 5) & 7, (attr >> 13) & 63
            width = POINTER.get(pointer) if mode in (0, 1) else None
            return width if not declared or width == declared else None
        if kind == LF_ARRAY and len(payload) >= 8:
            count, end = numeric(payload, 8)
            element = size(unpack_from('<I', payload)[0])
            # VC5 can reference a forward UDT here. The array's own byte size is
            # explicit even when the element's separate size is unavailable.
            if (count and count > 0 and element is not None and (element == 0 or count % element == 0)
                    and counted_name(payload, end)):
                return count
        if kind in (LF_CLASS, LF_STRUCTURE, LF_UNION, LF_ENUM) and len(payload) >= 4:
            forward = unpack_from('<H', payload, 2)[0] & 0x80
            if kind == LF_ENUM and counted_name(payload, 12):
                return None if forward else size(unpack_from('<I', payload, 4)[0])
            if kind != LF_ENUM:
                width, end = numeric(payload, 8 if kind == LF_UNION else 16)
                if width is not None and counted_name(payload, end):
                    if forward:
                        return 0 if width == 0 else None
                    return width if width > 0 else None
        return None

    # A well-formed forward declaration (internal zero) does not size a data symbol.
    return lambda index: size(index) or None


def data_sizes(coff, ordinary):
    """Return sizes by linkage name, with None for invalid or unresolved records.

    An unassignable record prevents the caller from proving which symbols have no
    record, so a malformed binding of that kind invalidates the sidecar.
    """
    sections = [s for s in coff.sections if s['name'] == '.debug$S']
    primary = [s for s in sections if s['data'].startswith(C11)]
    types = [s['data'] for s in coff.sections if s['name'] == '.debug$T']
    if len(primary) != 1 or len(types) > 1:
        raise ValueError('expected one primary C11 symbol stream and at most one type stream')
    size = type_sizes(types[0] if types else b'')
    def unique_symbols(obj):
        symbols = {}
        for symbol in obj.symbols.values():
            name = symbol['name']
            symbols[name] = symbol if name not in symbols else None
        return symbols

    debug_symbols, ordinary_symbols = unique_symbols(coff), unique_symbols(ordinary)
    extents = {}
    for section in sections:
        # VC5 gives the primary stream a signature; associative per-function streams are bare.
        offset = len(C11) if section is primary[0] else 0
        fields = {r['offset']: (r['type'], r['symbol']) for r in section['relocations']}
        if len(fields) != len(section['relocations']):
            raise ValueError('ambiguous CodeView relocation offsets')
        for kind, start, payload in records(section['data'], offset):
            if kind not in (S_LDATA32, S_GDATA32):
                continue
            location, segment = fields.get(start + 4), fields.get(start + 8)
            if len(payload) < 10 or not any(
                    field and field[0] == kind_ and field[1] in debug_symbols
                    for field, kind_ in ((location, SECREL), (segment, SECTION))):
                raise ValueError(f'unassignable CodeView data record relocations at {start - 4:#x}')
            names = {field[1] for field in (location, segment) if field}
            for name in names:
                symbol, original = debug_symbols.get(name), ordinary_symbols.get(name)
                expected = 2 if kind == S_GDATA32 else 3
                valid = (location == (SECREL, name) and segment == (SECTION, name)
                         and symbol and original and symbol['storage'] == original['storage'] == expected
                         and not (symbol['type'] | original['type']) & 0x20
                         and (symbol['section'] > 0 or symbol['section'] == 0 and symbol['value'] > 0)
                         and (symbol['section'] == 0) == (original['section'] == 0)
                         and counted_name(payload, 10) and payload[4:10] == bytes(6))
                extents[name] = size(unpack_from('<I', payload)[0]) if valid and name not in extents else None
    return extents
