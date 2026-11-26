"""Whole-image reference consistency of instruction-equal rows (a separate, weaker tier).

A row is CONSISTENT when its complete contribution is instruction-equal and every
reference fits one symbol-to-address map shared by the module's surviving rows:
one address per symbol, one symbol per address, no overlapping objects, definitions
whose bytes and fixups equal the image, pointers inside definitions that bind
consistently, and callees that are themselves instruction-equal at their entry.
docs/consistency_tier.md states the claim and its limits.

This is never FUNCTION_MATCH or FUNCTION_MATCH_RELOCATED: no reference here is
independently proven. recovery.json is read only and no row status changes.

    python scripts/consistency.py report --output work/consistency/report.json
    python scripts/consistency.py check       # shrink-only conflict and defect ledger
"""
import argparse
from bisect import bisect_right
import collections
import glob
import hashlib
import json
import re
import struct
import sys
from concurrent.futures import ThreadPoolExecutor
from itertools import combinations
from functools import lru_cache
from typing import NamedTuple
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from env import ROOT  # noqa: E402
from coff import COFF  # noqa: E402
from cv4_sizes import data_sizes  # noqa: E402
from masked import DIR32, REL32, _compare_section  # noqa: E402
from match import compile_source  # noqa: E402
from pe import PE  # noqa: E402

ACCEPTED = {'FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED', 'LINKED_MATCH'}
DEBT = ROOT / 'evidence/consistency_debt.json'
MEM_EXECUTE = 0x20000000
HIGHLOW = 3
CNT_CODE, CNT_UNINITIALIZED = 0x20, 0x80
EXTERNAL, STATIC, LABEL = 2, 3, 6
X87_STORES = {'fst', 'fstp', 'fist', 'fistp', 'fisttp', 'fbstp', 'fnstcw', 'fstcw', 'fnstsw', 'fstsw'}
STATE_SAVES = {'fnstenv', 'fstenv', 'fnsave', 'fsave', 'fxsave', 'fldenv', 'frstor', 'fxrstor'}
READS_FIRST = {'cmp', 'test', 'bt', 'push', 'call', 'jmp'}
CONTENT_BYTES = 8   # compared literal bytes a definition needs to prove its own placement


class Image:
    """Facts about one original module; injected in tests."""

    def __init__(self, path):
        self.pe = PE(path)
        self.base = self.pe.image_base
        self.sections = self.pe.sections
        raw = self.pe.pe
        self.relocated = bool(raw.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size)
        entries = [e for b in getattr(raw, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries if e.type]
        self.fixups = {e.rva for e in entries}
        self.highlow = {e.rva for e in entries if e.type == HIGHLOW}
        self.imports = {}
        for entry in getattr(raw, 'DIRECTORY_ENTRY_IMPORT', []):
            for item in entry.imports:
                slot = item.address - self.base
                self.imports[slot] = item.name.decode('ascii') if item.name else 'ordinal:%d' % item.ordinal

    def read(self, rva, size):
        return self.pe.read_rva(rva, size)

    def zone(self, rva, size):
        """'code', 'file', 'zero' for a range wholly inside one such zone, else None."""
        for s in self.sections:
            start, end = s['virtual_address'], s['virtual_address'] + s['virtual_size']
            if start <= rva and rva + size <= end:
                if s['characteristics'] & MEM_EXECUTE:
                    return 'code'
                filed = start + min(s['raw_size'], s['virtual_size'])
                return 'file' if rva + size <= filed else 'zero' if rva >= filed else None
        return None


def touching(image, base, size):
    """Native fixup fields (4 bytes) that intersect [base, base + size)."""
    return {f for f in image.fixups if f < base + size and f + 4 > base}


def import_name(symbol):
    """Plain C name of an __imp_ symbol; C++ decorated imports stay unsupported (None)."""
    name = symbol[len('__imp_'):]
    if not name.startswith('_'):
        return None
    return name[1:].split('@')[0]


def code_instructions(decoder, code, rva, cells=()):
    """Decode the code ranges independently, with inline table cells removed."""
    start = 0
    for cell in sorted(cells):
        if start < cell:
            yield from decoder.disasm(code[start:cell], rva + start)
        start = cell + 4
    if start < len(code):
        yield from decoder.disasm(code[start:], rva + start)


def accesses(decoder, code, rva, offset, cells=()):
    """(kind, width, write) of the operand whose 4-byte field starts at ``offset``, or None.

    direct: absolute flat [disp32] access; indexed: disp32 with a register; value: an
    immediate or an address without access; branch: a rel32 field. ``width`` is the number
    of bytes the access proves to belong to the object: a store's full width, one byte for a
    read, because VC5 widens loads of 8- and 16-bit objects. Segment-overridden memory and
    state-save forms are refused (None).
    """
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    for ins in code_instructions(decoder, code, rva, cells):
        start = ins.address - rva
        if not start <= offset < start + ins.size:
            continue
        field = offset - start
        if ins.disp_offset == field and ins.disp_size == 4:
            operands = list(ins.operands)
            memory = [o for o in operands if o.type == X86_OP_MEM]
            if len(memory) != 1 or memory[0].mem.segment or ins.mnemonic in STATE_SAVES:
                return None
            operand = memory[0]
            if operand.mem.base or operand.mem.index:
                return 'indexed', 0, False
            if ins.mnemonic == 'lea':
                return 'value', 0, False
            if ins.mnemonic.startswith('f'):
                store = ins.mnemonic in X87_STORES
            else:
                store = operands[0] is operand and ins.mnemonic not in READS_FIRST
            if store and operand.size <= 0:
                return None
            return 'direct', operand.size if store else 1, store
        if ins.imm_offset == field and ins.imm_size == 4:
            if ins.group(1) or ins.mnemonic == 'call':
                return 'branch', 0, False
            if any(o.type == X86_OP_IMM for o in ins.operands):
                return 'value', 0, False
        return None
    return None


def disjoint_fields(offsets, start, end):
    """Each four-byte relocation occupies a separate, in-bounds field."""
    for offset in sorted(offsets):
        if offset < start or offset + 4 > end:
            return False
        start = offset + 4
    return True


WIDTHS = {'DB': 1, 'DW': 2, 'DD': 4, 'DQ': 8, 'DT': 10}
DATA_LINE = re.compile(r'^(\S*)\s+(DB|DW|DD|DQ|DT)\s+(.*)$')
NUMBER = re.compile(r'^(?:[0-9][0-9A-Fa-f]*H|[0-9]+)$')
SCALAR = re.compile(r'^-?(?:[0-9][0-9A-Fa-f]*[Hr]?|[0-9]+|FLAT:[\w$?@]+(?:[+-][0-9A-Fa-f]+H?)?|[\w$?@]+'
                    r'|[\w$?@]+[+-][0-9A-Fa-f]+H?|\?)$')
VISIBILITY = re.compile(r'^\s*(?:COMM|PUBLIC|EXTRN)\s+\S.*$')
ALIGN = re.compile(r'^\s*ALIGN\s+\d+\s*$')


def number(text):
    return int(text[:-1], 16) if text.endswith('H') else int(text)


def split_items(text):
    """Comma-separated items outside quotes and parentheses; None for unbalanced input."""
    out, current, quoted, depth = [], '', False, 0
    for ch in text:
        if ch == "'":
            quoted = not quoted
        elif not quoted and ch == ';':
            break
        elif not quoted and ch == '(':
            depth += 1
        elif not quoted and ch == ')':
            depth -= 1
            if depth < 0:
                return None
        if ch == ',' and not quoted and depth == 0:
            out.append(current.strip())
            current = ''
        else:
            current += ch
    if quoted or depth:
        return None
    return out + [current.strip()]


def item_size(item, width):
    """Bytes one listing operand contributes, or None for an unrecognized form."""
    repeat = re.match(r'^([0-9][0-9A-Fa-f]*H?)\s*DUP\s*\((.*)\)$', item)
    if repeat:
        if not NUMBER.match(repeat[1]):
            return None
        inner = split_items(repeat[2])
        sizes = [item_size(x, width) for x in inner] if inner is not None else [None]
        return None if None in sizes else number(repeat[1]) * sum(sizes)
    if width == 1 and re.fullmatch(r"'(?:[^']|'')*'", item):
        return len(item) - 2 - item[1:-1].count("''")
    return width if item and SCALAR.match(item) else None


def listing_sizes(text, exact=False):
    """Count recognized listed objects. Exact mode requires a closed object without a trailing ORG."""
    sizes, segment, name = {}, None, None
    trailing_org, aligned = set(), False
    for line in text.splitlines():
        body = line.split(';', 1)[0].rstrip()
        if not body or VISIBILITY.fullmatch(body):
            continue
        match = re.match(r'^(\S+)\s+SEGMENT\b', body)
        if match:
            segment, name, aligned = match[1], None, False
            continue
        if re.match(r'^\S+\s+ENDS\b', body):
            segment, name, aligned = None, None, False
            continue
        if segment in (None, '_TEXT') or segment.startswith('.'):
            continue
        if ALIGN.fullmatch(body):
            aligned = True
            continue
        match = re.match(r'^\s+ORG\s+\$\+(\S+)$', body)
        if match or re.match(r'^\s+ORG\b', body):
            if name is not None and sizes[name] is not None:
                sizes[name] = (sizes[name] + number(match[1])
                               if not aligned and match and NUMBER.fullmatch(match[1]) else None)
                trailing_org.add(name)
            continue
        match = DATA_LINE.match(line)
        if not match:
            if name is not None:
                sizes[name] = None
            name = None
            continue
        if match[1]:
            name, aligned = match[1], False
            if name in sizes:
                sizes[name] = None
                name = None
                continue
            sizes[name] = 0
        if name is None or sizes[name] is None:
            continue
        operands = split_items(match[3])
        counted = [item_size(x, WIDTHS[match[2]]) for x in operands] if operands is not None else [None]
        sizes[name] = None if aligned or None in counted else sizes[name] + sum(counted)
        trailing_org.discard(name)
    if exact:
        sizes.update(dict.fromkeys(trailing_org))
        if name is not None:
            sizes[name] = None
    return sizes


class Unit:
    """Definitions of one freshly compiled object."""

    def __init__(self, coff, sizes=None, debug_error=None):
        self.coff = coff
        self.sizes = sizes or {}
        self.debug_error = debug_error
        self.by_name = collections.defaultdict(list)
        starts = collections.defaultdict(set)
        for s in coff.symbols.values():
            self.by_name[s['name']].append(s)
            starts[s['section']].add(s['value'])
        self.starts = {section: sorted(offsets) for section, offsets in starts.items()}

    def unique(self, name):
        found = self.by_name[name]
        return found[0] if len(found) == 1 else None

    def extent(self, symbol):
        """The declared extent must fit inside the ordinary COFF contribution."""
        name, number, start = symbol['name'], symbol['section'], symbol['value']
        size = self.sizes.get(name)
        if not size or size < 0 or self.unique(name) is None:
            return None
        if number == 0:
            return size if size == start else None   # COFF records an exact COMMON extent
        if not 0 < number <= len(self.coff.sections):
            return None
        offsets = self.starts[number]
        i = bisect_right(offsets, start)
        limit = self.coff.sections[number - 1]['size']
        if i < len(offsets):
            limit = min(limit, offsets[i])
        return size if start + size <= limit else None


def key_of(module, source, flags, symbol):
    """Linker-visible identity: externals per module, file-local names per object profile."""
    if symbol['storage'] == EXTERNAL:
        return module, symbol['name']
    return module, source, tuple(flags), symbol['name']


def ref_kind(name, function):
    return 'import' if name.startswith('__imp_') else 'function' if function else 'data'


def switch_cells(decoder, native, rva, pointers):
    """Find fixup-backed tables reached by an indexed near jump in this contribution."""
    from capstone import CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM

    if len(pointers) < 2:
        return set()
    code, cells, cell_offsets, pending = {}, set(), set(), [0]
    while pending:
        offset = pending.pop()
        if not 0 <= offset < len(native) or offset in code:
            continue
        ins = next(decoder.disasm(native[offset:], rva + offset, count=1), None)
        if ins is None:
            return set()
        end = offset + ins.size
        if any(i in cells for i in range(offset, end)):
            return set()
        code[offset] = end
        if ins.group(CS_GRP_RET) or ins.mnemonic in ('int3', 'ud2'):
            continue
        if not ins.operands:
            pending.append(end)
            continue
        operand = ins.operands[0]
        jump = ins.group(CS_GRP_JUMP) or ins.mnemonic in ('loop', 'loope', 'loopne', 'jecxz')
        if (jump or ins.group(CS_GRP_CALL)) and operand.type == X86_OP_IMM:
            pending.append(operand.imm - rva)
        if ins.mnemonic not in ('jmp', 'ljmp'):
            pending.append(end)
            continue
        disp = offset + ins.disp_offset
        if (ins.mnemonic != 'jmp' or len(ins.operands) != 1 or ins.addr_size != 4
                or operand.type != X86_OP_MEM or operand.size != 4 or operand.mem.segment
                or operand.mem.base or not operand.mem.index or operand.mem.scale != 4
                or ins.disp_size != 4 or disp not in pointers):
            continue
        start = pointers[disp]
        if start in cells or start not in pointers:
            continue
        stop = start
        while stop in pointers:
            pending.append(pointers[stop])
            stop += 4
        span = set(range(start, stop))
        if any(a < stop and start < b for a, b in code.items()):
            return set()
        cell_offsets.update(range(start, stop, 4))
        cells.update(span)
    if not cell_offsets:
        return set()
    # Validate every reached boundary against the same split decode used for operand roles.
    boundaries = {ins.address - rva: ins.address - rva + ins.size
                  for ins in code_instructions(decoder, native, rva, cell_offsets)}
    if any(boundaries.get(a) != b for a, b in code.items()):
        return set()
    return cell_offsets


def collect_row(row, unit, image, decoder):
    """References one instruction-equal row makes, or a refusal reason."""
    module = Path(row['target']).name
    rva, size = int(row['rva'], 16), row['size']
    coff, source, flags = unit.coff, row['source'], row['flags']
    try:
        section = coff.function(row['symbol'])
    except ValueError as error:
        return dict(reason='contribution: ' + str(error))
    number_ = coff.sections.index(section) + 1
    fields = [r['offset'] for r in section['relocations']]
    if not disjoint_fields(fields, 0, min(size, len(section['data']))):
        return dict(reason='overlapping or out-of-bounds code relocation fields')
    native_fields = {base - rva for base in touching(image, rva, size)} if image.relocated else set()
    absolute = {r['offset'] for r in section['relocations'] if r['type'] == DIR32}
    shared = {off for off in native_fields & absolute if rva + off in image.highlow}
    fixups_match = not image.relocated or native_fields == shared == absolute
    result = _compare_section(coff, section, number_, image.pe, rva, size)
    refused = not (fixups_match and result['masked_equal'])
    if refused and (result['differing_bytes'] or result['unsupported']
                    or result['compiled_size'] != size or result['target_size'] != size):
        return dict(reason='not instruction-equal')

    native = image.read(rva, size)
    pointers = {}
    for rel in section['relocations']:
        offset = rel['offset']
        if offset not in shared or rel['type'] != DIR32:
            continue
        symbol = unit.unique(rel['symbol'])
        if (symbol and symbol['section'] == number_
                and symbol['storage'] in (STATIC, LABEL) and not symbol['type'] & 0x20):
            addend = struct.unpack_from('<i', section['data'], offset)[0]
            target = (struct.unpack_from('<I', native, offset)[0] - image.base - rva) & 0xffffffff
            if target == symbol['value'] + addend and target < size:
                pointers[offset] = target
    cells = switch_cells(decoder, native, rva, pointers)
    if refused:
        # Form the comparison's address equations again from independently valid fields.
        # In a relocated image a DIR32 needs the corresponding native HIGHLOW.
        verified = []
        for rel in section['relocations']:
            off = rel['offset']
            if image.relocated and rel['type'] == DIR32 and (
                    off not in shared or any(abs(other - off) < 4 for other in native_fields if other != off)):
                continue
            use = accesses(decoder, native, rva, off, cells)
            if unit.unique(rel['symbol']) and use and (use[0] == 'branch') == (rel['type'] == REL32):
                verified.append(rel)
        checked = _compare_section(coff, dict(section, relocations=verified), number_, image.pe, rva, size)
        disagreements = []
        for conflict in checked['conflicts']:
            if 'previous' in conflict or conflict.get('reason') == 'own section':
                symbol = unit.unique(conflict['symbol'])
                disagreements.append(dict(conflict, key=key_of(module, source, flags, symbol)))
        return dict(reason='not instruction-equal', address_conflicts=disagreements)
    masked = bytearray(native)
    refs = []
    for r in section['relocations']:
        masked[r['offset']:r['offset'] + 4] = b'\0' * 4
        symbol = unit.unique(r['symbol'])
        if symbol is None:
            return dict(reason='ambiguous COFF symbol ' + r['symbol'])
        addend = struct.unpack_from('<i', section['data'], r['offset'])[0]
        field = struct.unpack_from('<I', native, r['offset'])[0]
        if r['type'] == REL32:
            target = (rva + r['offset'] + 4 + field) & 0xffffffff
        else:
            target = (field - image.base) & 0xffffffff
        if r['offset'] in cells:
            continue
        use = accesses(decoder, native, rva, r['offset'], cells)
        if symbol['section'] == number_:
            # Own contribution: masked._compare_section fixed the arithmetic. The field must have the
            # role of its relocation type, and the target must stay inside the contribution.
            if r['type'] == REL32 and (use is None or use[0] != 'branch'):
                return dict(reason='own-section REL32 that is not a branch at %#x' % r['offset'])
            if not symbol['type'] & 0x20 and symbol['storage'] not in (STATIC, LABEL):
                return dict(reason='own-section label without local linkage at %#x' % r['offset'])
            if r['type'] == DIR32 and (use is None or use[0] == 'branch'):
                return dict(reason='own-section DIR32 without a supported operand form at %#x' % r['offset'])
            if not rva <= target < rva + size:
                return dict(reason='own-section reference leaves the contribution at %#x' % r['offset'])
            if symbol['type'] & 0x20:
                if target != rva or addend:
                    return dict(reason='own function reference is not the entry at %#x' % r['offset'])
                refs.append(dict(key=key_of(module, source, flags, symbol), name=r['symbol'], base=rva, addend=0,
                                 use=use[0], width=use[1], write=use[2],
                                 function=True, defined=True, common=0))
            continue
        kind = ref_kind(r['symbol'], bool(symbol['type'] & 0x20))
        if r['type'] == REL32 and kind != 'function':
            return dict(reason='branch to a non-function symbol at %#x' % r['offset'])
        if kind != 'data' and addend:
            return dict(reason='function or import reference with an addend at %#x' % r['offset'])
        if use is None:
            return dict(reason='operand form at %#x' % r['offset'])
        if (use[0] == 'branch') != (r['type'] == REL32):
            return dict(reason='relocation type and operand disagree at %#x' % r['offset'])
        if kind == 'import' and (use[0] != 'direct' or use[2]):
            return dict(reason='import slot used other than by a direct read at %#x' % r['offset'])
        refs.append(dict(key=key_of(module, source, flags, symbol), name=r['symbol'],
                         base=(target - addend) & 0xffffffff, addend=addend, use=use[0], width=use[1],
                         write=use[2], function=kind == 'function', defined=symbol['section'] > 0,
                         common=symbol['value'] if symbol['section'] == 0 else 0))
    return dict(refs=refs, body=hashlib.sha256(bytes(masked)).hexdigest(),
                native=hashlib.sha256(native).hexdigest(), start=rva, size=size, module=module)


def define(module, source, flags, unit, symbol):
    """Definition record of a data symbol in this object (payload, relocations, size).

    ``refs`` is None when the definition cannot be checked completely (unknown size or an
    unsupported relocation); such a definition fails its key rather than being skipped.
    """
    number, start = symbol['section'], symbol['value']
    if number == 0 and not start:                # an external declaration, not a COMMON definition
        return None
    size = unit.extent(symbol)
    if size is None:
        return dict(size=0, payload=None, refs=None)
    if number == 0:
        return dict(size=size, payload=None, refs=[])
    section = unit.coff.sections[number - 1]
    fields = [r for r in section['relocations'] if r['offset'] < start + size and r['offset'] + 4 > start]
    if section['flags'] & CNT_UNINITIALIZED:
        return dict(size=size, payload=None, refs=None if fields else [])
    if start + size > len(section['data']) or not disjoint_fields((r['offset'] for r in fields), start, start + size):
        return dict(size=size, payload=b'', refs=None)
    refs = []
    for r in fields:
        target = unit.unique(r['symbol'])
        if target is None or r['type'] != DIR32:
            return dict(size=size, payload=b'', refs=None)
        refs.append(dict(offset=r['offset'] - start, name=r['symbol'],
                         key=key_of(module, source, flags, target), function=bool(target['type'] & 0x20),
                         defined=target['section'] > 0, section=target['section'], value=target['value'],
                         common=target['value'] if target['section'] == 0 else 0))
    return dict(size=size, payload=section['data'][start:start + size], refs=refs)


def unit_definitions(module, source, flags, unit):
    """key -> [definition] for every data definition of a unit; a key defined twice is unsupported."""
    out = collections.defaultdict(list)
    for symbol in unit.coff.symbols.values():
        if symbol['storage'] not in (EXTERNAL, STATIC) or symbol['type'] & 0x20 or symbol['section'] < 0:
            continue
        if symbol['name'].startswith('.') or (symbol['section'] == 0 and not symbol['value']):
            continue
        if symbol['section'] > 0:
            section = unit.coff.sections[symbol['section'] - 1]
            if section['flags'] & (CNT_CODE | MEM_EXECUTE) or section['name'].startswith(('.debug', '.drectve')):
                continue
        found = define(module, source, flags, unit, symbol)
        if found is not None:
            out[key_of(module, source, flags, symbol)].append(found)
    for key, found in out.items():
        if len(found) > 1:
            out[key] = [dict(size=0, payload=None, refs=None)]
    return dict(out)


class Evidence:
    """Everything the rules need, independent of how it was produced."""

    def __init__(self, images):
        self.images = images                 # module -> Image-like
        self.rows = {}                       # id -> dict(refs, body, native, start, size, module, accepted)
        self.refused = {}                    # id -> reason
        self.collection_conflicts = {}       # id -> typed comparator contradictions
        self.accepted_collection_disagreements = set()
        self.definitions = collections.defaultdict(list)   # key -> [definition]
        self.functions = collections.defaultdict(list)     # function key -> [(rva, id, equal)]
        self.anchors = {}                    # key -> dict(rva, size, kind, alias_of) or (rva, size[, kind[, alias]])
        self.anchor_conflicts = {}           # key -> typed conflict, raw bindings and resolved alias owner
        self.opaque_pins = []                # (reservation key, RVA, size) without a trusted symbolic key
        self.starts = {}                     # (module, rva) -> id of a recorded row
        self.scope = {}                      # provenance of the definition inventory


def anchor_record(evidence, key):
    """Normalized accepted anchor: dict(rva, size, kind, alias_of) or None."""
    if key not in evidence.anchors or key in evidence.anchor_conflicts:
        return None
    found = evidence.anchors[key]
    if isinstance(found, dict):
        return found
    padded = tuple(found) + (None,) * 4
    return dict(rva=padded[0], size=padded[1], kind=padded[2] or 'data', alias_of=padded[3])


def payload_local(image, base, definition):
    """None when the definition's literal bytes and fixup fields equal the image at base."""
    size = definition['size']
    if definition['refs'] is None:
        return 'unsupported definition'
    if size <= 0:
        return 'empty definition'
    fields = touching(image, base, size)
    if definition['payload'] is None:
        if fields:
            return 'zero definition carries loader fixups'
        zone = image.zone(base, size)
        if zone == 'zero' or (zone == 'file' and not any(image.read(base, size))):
            return None
        return 'zero definition is not zero in the image'
    if image.zone(base, size) != 'file':
        return 'initialized definition outside file-backed data'
    native, own = image.read(base, size), definition['payload']
    offsets = [r['offset'] for r in definition['refs']]
    if len(own) != size or len(native) != size or not disjoint_fields(offsets, 0, size):
        return 'unsupported definition'
    pointers = {i for offset in offsets for i in range(offset, offset + 4)}
    if any(native[i] != own[i] for i in range(size) if i not in pointers):
        return 'payload differs'
    if image.relocated and fields != {base + offset for offset in offsets}:
        return 'payload fixups differ'
    return None


def literal_content(definition):
    """Whether a definition's compared literal bytes are enough to place it by themselves."""
    if not definition['payload'] or definition['refs'] is None:
        return False
    pointers = {o for r in definition['refs'] for o in range(r['offset'], r['offset'] + 4)}
    literal = [b for i, b in enumerate(definition['payload']) if i not in pointers]
    return len(literal) >= CONTENT_BYTES and len(set(literal)) > 2


def label(key):
    if isinstance(key[1], tuple):
        return '%s | IAT slot %#x' % (key[0], key[1][1])
    return ' | '.join((key[0], ' '.join(' '.join(p) if isinstance(p, tuple) else str(p) for p in key[1:])))


class Issue(NamedTuple):
    message: str
    category: str
    origin: tuple


class Storage(NamedTuple):
    start: int
    end: int
    key: tuple
    authority: str | None = None
    use: tuple | None = None


def definition_graph(evidence, row_uses, pins, blocked, definitions, functions, identity):
    """Publish locally valid equations, preserving uncertain storage when placement fails."""
    def kinds(key, occurrences):
        seen = {o['kind'] for o in occurrences.get(key, ())}
        if key in pins:
            seen.add(pins[key]['kind'])
        if key in definitions:
            seen.add('data')
        if key in functions:
            seen.add('function')
        return seen

    def data_base(key, occurrences):
        if key in pins:
            return pins[key]['rva']
        values = {o['value'] for o in occurrences.get(key, ())}
        return next(iter(values)) if len(values) == 1 else None

    def placement(key, occurrences):
        return data_base(key, occurrences) if kinds(key, occurrences) == {'data'} else None

    def edge_issue(target, o):
        def issue(text, category):
            return Issue(text, category, o['source'])

        kind, base = o['kind'], o['value']
        if kind != 'data' and o['addend']:
            return issue('function or import reference with an addend', 'invalid')
        if target in pins:
            if pins[target]['kind'] != kind:
                return issue('kind conflict', 'address')
            if pins[target]['rva'] != base:
                return issue('several addresses', 'address')
        if kind == 'function' and target in functions:
            if any(entry != base for entry, _, _ in functions[target]):
                return issue('several addresses', 'address')
        if kind == 'import':
            plain = import_name(target[-1])
            slots = [slot for slot, name in evidence.images[target[0]].imports.items() if plain == name]
            if len(slots) != 1:
                return issue('import slot', 'address')
            if slots[0] != base:
                return issue('several addresses', 'address')
        return None

    @lru_cache(maxsize=None)
    def inspect_definition(key, base):
        image, found = evidence.images[key[0]], definitions[key]
        local = {p for d in found if (p := payload_local(image, base, d))}
        if local:
            return local, [], set()
        targets, rejected = [], set()
        for i, d in enumerate(found):
            if not d['refs']:
                continue
            native = image.read(base, d['size'])
            for r in d['refs']:
                offset = r['offset']
                addend = struct.unpack_from('<i', d['payload'], offset)[0]
                value = (struct.unpack_from('<I', native, offset)[0] - image.base - addend) & 0xffffffff
                target = identity(r['key'])
                kind = ref_kind(r['name'], r['function'])
                o = dict(source=('edge', key, i, offset), value=value, kind=kind, addend=addend, ref=None)
                targets.append((target, o))
                if failure := edge_issue(target, o):
                    rejected.add(failure)
        return local, targets, rejected

    quarantined = {}
    while True:
        occurrences = {key: list(uses) for key, uses in row_uses.items()}
        edges, local, rejected, processed = {}, {}, {}, set()
        while True:
            pending = {}
            for key in occurrences.keys() | pins.keys():
                if key in processed or key in blocked or key in quarantined or key not in definitions:
                    continue
                base = placement(key, occurrences)
                if base is not None:
                    pending[key] = base
            if not pending:
                break
            processed.update(pending)
            batch = {}
            for key, base in pending.items():
                local[key], targets, bad = inspect_definition(key, base)
                if bad:
                    rejected[key] = bad
                elif targets:
                    batch[key] = targets
            for key, targets in batch.items():
                edges[key] = targets
                for target, o in targets:
                    occurrences.setdefault(target, []).append(o)
        late = {key for key in edges if placement(key, occurrences) is None}
        if not late:
            break
        for key in late:
            quarantined[key] = 'kind conflict' if kinds(key, occurrences) != {'data'} else 'several addresses'

    keys = occurrences.keys() | pins.keys()
    seen_kinds = {key: kinds(key, occurrences) for key in keys}
    bases = {key: data_base(key, occurrences) for key in keys}

    # An ambiguous placement may have hidden a locally viable outgoing path. Keep
    # possible storage from those paths out of the witness map, but reserve it against
    # weaker claims. This cannot give an ambiguous source or target any proof.
    pending = [(key, o['value']) for key in occurrences if key in definitions and key not in blocked
               and (key in quarantined or placement(key, occurrences) is None)
               for o in occurrences[key] if o['kind'] == 'data' and
               (key not in pins or o['value'] == pins[key]['rva'])]
    uncertain, visited = set(), set()
    while pending:
        key, base = pending.pop()
        if (key, base) in visited or key in blocked:
            continue
        visited.add((key, base))
        _, targets, bad = inspect_definition(key, base)
        if bad:
            continue
        for target, o in targets:
            if o['kind'] != 'data' or target in blocked:
                continue
            value = o['value']
            if value == bases.get(target) and target not in quarantined:
                continue
            size = max(1, *(d['size'] for d in definitions[target])) if target in definitions else 1
            if evidence.images[target[0]].zone(value, size) in ('file', 'zero'):
                uncertain.add(Storage(value, value + size, target, 'uncertain'))
                if target in definitions:
                    pending.append((target, value))

    return occurrences, edges, local, quarantined, seen_kinds, bases, rejected, uncertain


def evaluate(evidence):
    """Apply the rules. Returns grades, conflicts, defects and the reason each pending row fails."""
    rows, images = evidence.rows, evidence.images
    failures = collections.defaultdict(dict)      # key -> {typed issue: pointer distance}

    def problem(key, text, category='invalid'):
        failures[key][Issue(text, category, ('key', key))] = 0

    raw_pins = {key: anchor_record(evidence, key) for key in evidence.anchors
                if key not in evidence.anchor_conflicts}
    owners = {}
    pin_records = raw_pins | evidence.anchor_conflicts
    for key in pin_records:
        target, seen = key, set()
        while owner := pin_records.get(target, {}).get('alias_of'):
            if owner in seen or owner[0] != key[0]:
                problem(key, 'invalid accepted alias ownership', 'address')
                target = key
                break
            seen.add(target)
            target = owner
        owners[key] = target

    def identity(key):
        return owners.get(key, key)

    pins = {}
    for key, pin in sorted(raw_pins.items(), key=lambda pair: str(pair[0])):
        owner = identity(key)
        found = pins.get(owner)
        if found and (found['rva'], found['kind']) != (pin['rva'], pin['kind']):
            problem(owner, 'accepted alias pins disagree', 'address')
        elif found and found['size'] != pin['size']:
            problem(owner, 'accepted alias pin sizes disagree', 'conflict')
        elif found is None:
            pins[owner] = pin
    definitions = collections.defaultdict(list)
    for key, found in evidence.definitions.items():
        owner = identity(key)
        definitions[owner].extend(d for d in found if d not in definitions[owner])
    functions = collections.defaultdict(list)
    for key, found in evidence.functions.items():
        functions[identity(key)].extend(found)

    row_uses = collections.defaultdict(list)
    for rid, row in rows.items():
        for i, ref in enumerate(row['refs']):
            kind = ref_kind(ref['name'], ref['function'])
            row_uses[identity(ref['key'])].append(dict(source=('row', rid, i), value=ref['base'], kind=kind,
                                                      addend=ref['addend'], ref=ref))

    iat = {module: [Storage(slot, slot + 4, (module, ('iat', slot)), 'iat') for slot in image.imports]
           for module, image in images.items()}
    items = [item for slots in iat.values() for item in slots]
    for key, base, size in [
            *((key, pin['rva'], pin['size']) for key, pin in raw_pins.items() if pin['kind'] == 'data' and pin['size']),
            *evidence.opaque_pins]:
        items.append(Storage(base, base + size, key, 'pin'))
        if any(slot.start < base + size and base < slot.end for slot in iat[key[0]]):
            problem(identity(key), 'shares storage with an IAT slot', 'conflict')

    for key, conflict in evidence.anchor_conflicts.items():
        problem(identity(key), conflict['reason'], conflict['category'])
    occurrences, edges, local, quarantined, kinds, bases, rejected, uncertain = definition_graph(
        evidence, row_uses, pins, failures.keys(), definitions, functions, identity)
    items.extend(uncertain)
    for key, issues in local.items():
        for issue in issues:
            problem(key, issue, 'defect')
    for key, text in quarantined.items():
        problem(key, text, 'address')

    # Keep a known canonical address even when another check fails for that key.
    canonical, content = {}, set()
    for key, seen in kinds.items():
        if not seen:
            continue
        image, anchor = images[key[0]], pins.get(key)
        if len(seen) > 1:
            problem(key, 'kind conflict', 'address')
        kind = anchor['kind'] if anchor else next(iter(seen)) if len(seen) == 1 else None
        if kind == 'function':
            defs = functions.get(key, [])
            addresses = {d[0] for d in defs}
            if len(addresses) > 1 or (anchor and addresses - {anchor['rva']}):
                problem(key, 'several addresses', 'address')
            entry = bases[key] if anchor or not addresses else next(iter(addresses)) if len(addresses) == 1 else None
            if entry is None:
                problem(key, 'several addresses', 'address')
                continue
            canonical[key] = entry
            if image.zone(entry, 1) != 'code':
                problem(key, 'callee outside code')
            elif defs:
                if not all(d[2] for d in defs):
                    problem(key, 'callee not instruction-equal')
            elif not anchor:
                if (key[0], entry) in evidence.starts:
                    problem(key, 'callee recorded under another name', 'conflict')
                else:
                    problem(key, 'callee without a row')
            if key not in failures:
                content.add(key)
        elif kind == 'import':
            plain = import_name(key[-1])
            slots = [s for s in iat[key[0]] if plain and image.imports[s.start] == plain]
            if len(slots) == 1:
                canonical[key] = slots[0].start
                content.add(key)
            else:
                problem(key, 'import slot', 'address')
        elif kind == 'data':
            base = bases[key]
            if base is None:
                problem(key, 'several addresses', 'address')
                continue
            canonical[key] = base
            if key in failures:
                continue
            direct = [o for o in occurrences.get(key, ()) if o['ref'] and o['value'] == base
                      and o['ref']['use'] == 'direct' and o['addend'] >= 0]
            extent = max((o['addend'] + o['ref']['width'] for o in direct), default=0)
            found = definitions.get(key, [])
            defined = max([d['size'] for d in found] + [1])
            zone = image.zone(base, max(defined, extent))
            if zone in (None, 'code'):
                problem(key, 'not inside one data zone')
                continue
            if not found and zone == 'file' and not anchor:
                problem(key, 'initialized data without a definition')
            if any(extent > d['size'] for d in found):
                problem(key, 'access beyond the definition', 'defect')
            if not anchor or defined > anchor['size']:
                items.append(Storage(base, base + (defined if anchor else max(defined, extent)), key))
            if anchor:
                for o in direct:
                    end = base + o['addend'] + o['ref']['width']
                    if end > base + anchor['size']:
                        items.append(Storage(base, end, key, use=o['source']))
            if key not in failures and (anchor or any(literal_content(d) for d in found)):
                content.add(key)

    collisions, failed_uses = set(), set()
    by_module = collections.defaultdict(list)
    for item in items:
        by_module[item.key[0]].append(item)

    for entries in by_module.values():
        entries.sort(key=lambda t: (t.start, t.end, str(t.key)))
        active = []
        for item in entries:
            active = [other for other in active if other.end > item.start]
            for other in active:
                if item.authority == other.authority == 'pin':
                    if item.key != other.key and (item[:2], identity(item.key)) != (other[:2], identity(other.key)):
                        collisions.add(tuple(sorted((item.key, other.key), key=str)))
                    continue
                if identity(other.key) == identity(item.key):
                    continue
                for claim in (item, other):
                    if claim.authority:
                        continue
                    if claim.use:
                        failed_uses.add(claim.use)
                    else:
                        problem(claim.key, 'shares storage with another name', 'conflict')
            active.append(item)

    # Check occurrences independently of key-level failures, then propagate typed causes.
    use_issues, conflicts, reverse, queue = {}, set(), collections.defaultdict(set), collections.deque()
    overlap = 'access beyond the accepted extent overlaps another object'
    for key, found in occurrences.items():
        for o in found:
            source, ref = o['source'], o['ref']
            issues = use_issues[source] = set()
            if key in canonical and canonical[key] != o['value']:
                issues.add(Issue('several addresses', 'address', source))
                if source[0] == 'row':
                    conflicts.add(label(key) + ' | several addresses')
            if o['kind'] != 'data' and o['addend']:
                issues.add(Issue('function or import reference with an addend', 'invalid', source))
            if ref and ref['use'] == 'direct' and ref['addend'] < 0:
                issues.add(Issue('direct access before the object', 'invalid', source))
            if source in failed_uses:
                issues.add(Issue(overlap, 'conflict', source))
                conflicts.add('%s | row %s reference %d | %s' % (label(key), source[1], source[2] + 1, overlap))
            if source[0] == 'edge':
                reverse[key].add(source[1])
                queue.extend((source[1], issue, 1) for issue in issues)
    queue.extend((key, issue, 0) for key, issues in failures.items() for issue in issues)
    queue.extend((key, issue, 1) for key, issues in rejected.items() for issue in issues)
    while queue:
        key, issue, depth = queue.popleft()
        if depth:
            distances = failures[key]
            if issue in distances and distances[issue] <= depth:
                continue
            distances[issue] = depth
        queue.extend((parent, issue, depth + 1) for parent in reverse[key])

    def describe(issue, depth):
        text = 'disagrees with the map' if depth and issue.message == 'several addresses' else issue.message
        return 'payload pointer: ' * depth + text

    for rid, found in evidence.collection_conflicts.items():
        for conflict in found:
            conflicts.add(f"{label(conflict['key'])} | row {rid} | incompatible relocation addresses")
    reasons, failing_accepted, live = {}, [], set()
    disagreeing = set(evidence.accepted_collection_disagreements)
    for rid, row in rows.items():
        bad, address_error = set(), False
        for i, ref in enumerate(row['refs']):
            found = dict(failures.get(identity(ref['key']), {}))
            found.update((issue, 0) for issue in use_issues['row', rid, i])
            bad.update(describe(issue, depth) + ': ' + ref['name'] for issue, depth in found.items())
            address_error |= any(issue.category == 'address' for issue in found)
        if bad:
            if row['accepted']:
                failing_accepted.append(rid)
                if address_error:
                    disagreeing.add(rid)
            else:
                reasons[rid] = sorted(bad)
        else:
            live.add(rid)

    def closure(row):
        seen, stack = set(), [identity(r['key']) for r in row['refs']]
        while stack:
            key = stack.pop()
            if key in seen:
                continue
            seen.add(key)
            stack.extend(t for t, _ in edges.get(key, ()))
        return seen

    closures = {rid: closure(rows[rid]) for rid in live}
    users = collections.defaultdict(set)
    for rid in live:
        for r in rows[rid]['refs']:
            users[identity(r['key'])].add(rid)

    def witnessed(key, strong):
        for a, b in combinations((rows[rid] for rid in users[key] & strong), 2):
            if (a.get('native') and b.get('native') and a['native'] != b['native'] and a['body'] != b['body']
                    and (a['start'] + a['size'] <= b['start'] or b['start'] + b['size'] <= a['start'])):
                return True
        return False

    collided = {identity(k) for pair in collisions for k in pair}
    strong = {rid for rid in live if not closures[rid] & collided}
    weak_reasons = {rid: ['accepted pin collision: ' + label(key) for key in closures[rid] & collided]
                    for rid in live - strong}
    while True:
        needed = set().union(*(closures[rid] - content for rid in strong))
        unwitnessed = {key for key in needed if not witnessed(key, strong)}
        weak = {rid for rid in strong if closures[rid] & unwitnessed}
        if not weak:
            break
        for rid in weak:
            weak_reasons[rid] = ['no qualifying witness pair: ' + label(key) for key in closures[rid] & unwitnessed]
        strong -= weak
    grades = {rid: ('A' if rid in strong else 'B') for rid in sorted(live) if not rows[rid]['accepted']}
    grade_b_reasons = {rid: sorted(weak_reasons[rid]) for rid, grade in grades.items() if grade == 'B'}

    defects, problems = set(), {}
    for key, found in failures.items():
        issue, depth = min(found.items(), key=lambda pair: (pair[1], pair[0].message))
        problems[key] = describe(issue, depth)
        for issue, depth in found.items():
            if not depth and issue.category == 'defect':
                defects.add(label(key) + ' | ' + issue.message)
            if not depth and issue.category in ('address', 'conflict'):
                conflicts.add(label(key) + ' | ' + issue.message)
            if depth and issue.category == 'address':
                conflicts.add(label(key) + ' | ' + describe(issue, 1))
    return dict(grades=grades, grade_b_reasons=grade_b_reasons, reasons=reasons,
                conflicts=sorted(conflicts), defects=sorted(defects),
                problems=problems, accepted_failing=sorted(failing_accepted),
                accepted_disagreement=sorted(disagreeing),
                collisions=sorted(' / '.join(label(k) for k in pair) for pair in collisions),
                symbols=len(occurrences), content=len(content))


def binding_records(ledger):
    """(row, module, name, entry) of every accepted RELOCATED binding record, hash-checked."""
    cache = {}
    for row in ledger:
        if row['status'] != 'FUNCTION_MATCH_RELOCATED':
            continue
        path = row['proof']['bindings_path']
        if path not in cache:
            cache[path] = json.loads((ROOT / path).read_bytes())
        binding = cache[path]['functions'][row['id']]
        canonical = json.dumps(binding, sort_keys=True, separators=(',', ':')).encode()
        if hashlib.sha256(canonical).hexdigest() != row['proof']['bindings_sha256']:
            raise ValueError('Accepted reference bindings changed: ' + row['id'])
        module = Path(row['target']).name
        for name, record in binding['symbols'].items():
            if record['kind'] not in ('data', 'bss', 'function'):
                continue
            kind = 'function' if record['kind'] == 'function' else 'data'
            owner = record.get('owner') if 'source_alias' in record else None
            yield row, module, name, dict(rva=record['rva'], size=record.get('size', 0) if kind == 'data' else 0,
                                          kind=kind, alias_owner=owner)


def accepted_anchors(ledger, scope, owner_scope=None):
    """Resolve symbolic pins and keep physical reservations for unresolved or conflicting records."""
    records, aliases = collections.defaultdict(list), collections.defaultdict(set)
    unresolved, reservations = [], set()
    for row, module, name, entry in binding_records(ledger):
        key = scope(row, name)
        base, size, kind = entry['rva'], entry['size'], entry['kind']
        if key is None:
            unresolved.append((row['id'], name))
            if kind == 'data' and size:
                opaque = (module, 'unresolved pin', row['source'], tuple(row['flags']), name)
                reservations.add((opaque, base, size))
            continue
        records[key].append(dict(row=row['id'], name=name, rva=base, size=size, kind=kind))
        owner = entry['alias_owner']
        if owner:
            aliases[key].add(owner_scope(module, owner) if owner_scope else None)

    out, conflicts = {}, {}
    for key, found in records.items():
        interval = {(r['rva'], r['size'], r['kind']) for r in found}
        owners = aliases[key]
        alias = next(iter(owners)) if len(owners) == 1 else None
        if len(interval) > 1:
            address_conflict = len({(base, kind) for base, _, kind in interval}) > 1
            sizes = ', '.join(map(str, sorted({size for _, size, _ in interval})))
            reason = 'accepted bindings disagree' if address_conflict else f'accepted binding sizes disagree ({sizes} bytes)'
            conflicts[key] = dict(reason=reason, category='address' if address_conflict else 'conflict', alias_of=alias,
                                  records=sorted(found, key=lambda r: (r['rva'], r['size'], r['kind'], r['row'], r['name'])))
            opaque = (key[0], 'conflicting pin', *key[1:])
            reservations.update((opaque, base, size) for base, size, kind in interval if size and kind == 'data')
            continue
        base, size, kind = next(iter(interval))
        out[key] = dict(rva=base, size=size, kind=kind, alias_of=alias)
    return out, conflicts, unresolved, sorted(reservations, key=str)


def compile_unit(source, flags):
    """Build the comparison object and a separate CodeView type sidecar."""
    obj, _ = compile_source(ROOT / source, [*flags, '/Falisting.asm'])
    ordinary = COFF(obj)
    try:
        listing = (obj.parent / 'listing.asm').read_text(encoding='latin-1')
    except OSError:
        listing = ''
    listed = listing_sizes(listing, exact=True)
    try:
        debug, _ = compile_source(ROOT / source, [*flags, '/Z7'])
        sizes = data_sizes(COFF(debug), ordinary)
    except (KeyError, OSError, RuntimeError, ValueError, struct.error) as error:
        return Unit(ordinary, debug_error=str(error).splitlines()[0])
    for name in listed.keys() & sizes.keys():
        if listed[name] is not None and listed[name] != sizes[name]:
            sizes[name] = None
    return Unit(ordinary, listed | sizes)


def data_units(ledger):
    """[(module, source, flags, origin)] of data units outside the recorded profiles.

    origin 'owner': named as an owner by an accepted binding. origin 'module-default': an
    unrecorded file of the module's source folder compiled with /O2 (candidate evidence).
    """
    recorded = {r['source'] for r in ledger}
    modules = {Path(r['target']).name for r in ledger}
    out, cache = {}, {}
    for row in ledger:
        if row['status'] != 'FUNCTION_MATCH_RELOCATED':
            continue
        path = row['proof']['bindings_path']
        if path not in cache:
            cache[path] = json.loads((ROOT / path).read_bytes())
        for record in cache[path]['functions'][row['id']]['symbols'].values():
            if 'owner' in record:
                owner = record['owner']
                out[Path(row['target']).name, owner['source'], tuple(owner['flags'])] = 'owner'
    covered = {(m, s) for m, s, _ in out}
    for folder, module in (('eng1', 'ENG1.DLL'), ('eng3', 'ENG3.DLL'), ('title', 'TITLE.DLL'), ('pc', 'HERCULES.EXE')):
        for path in sorted(glob.glob(str(ROOT / 'src' / folder / '*.c')) + glob.glob(str(ROOT / 'src' / folder / '*.cpp'))):
            source = Path(path).relative_to(ROOT).as_posix()
            if module in modules and source not in recorded and (module, source) not in covered:
                out[module, source, ('/O2',)] = 'module-default'
    return sorted((m, s, f, o) for (m, s, f), o in out.items())


def gather(jobs=8, modules=None):
    """Fresh compilation of every recorded source profile and data unit; returns Evidence."""
    import capstone
    ledger = json.loads((ROOT / 'recovery.json').read_bytes())['functions']
    if modules:
        ledger = [r for r in ledger if Path(r['target']).name in modules]
    images = {Path(t).name: Image(ROOT / t) for t in {r['target'] for r in ledger}}
    evidence = Evidence(images)
    groups = collections.defaultdict(list)
    for r in ledger:
        groups[r['source'], tuple(r['flags'])].append(r)
        evidence.starts[Path(r['target']).name, int(r['rva'], 16)] = r['id']
    owners = data_units(ledger)
    linkage = {}      # row id -> {name: key} in that row's own compiled unit
    profiles = dict.fromkeys([*groups, *((source, flags) for _, source, flags, _ in owners)])

    def compile_profile(profile):
        try:
            return compile_unit(*profile)
        except (RuntimeError, ValueError) as error:
            return error

    def work(item):
        (source, flags), members = item
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        unit = compiled[source, flags]
        if isinstance(unit, Exception):
            return [(r, dict(reason='compile: ' + str(unit)[:120]), None, {}) for r in members], {}, source
        out, definitions = [], collections.defaultdict(list)
        for module in {Path(r['target']).name for r in members}:
            for key, found in unit_definitions(module, source, flags, unit).items():
                definitions[key].extend(found)
        for r in members:
            module = Path(r['target']).name
            found = collect_row(r, unit, images[module], decoder)
            symbol = unit.unique(r['symbol'])
            own = key_of(module, source, flags, symbol) if symbol else (module, r['symbol'])
            names = {name: key_of(module, source, flags, found_[0])
                     for name, found_ in unit.by_name.items() if len(found_) == 1}
            out.append((r, found, own, names))
        return out, definitions, None

    def work_data(item):
        module, source, flags, origin = item
        unit = compiled[source, flags]
        if isinstance(unit, Exception):
            return {}, (source, origin, 'compile failed')
        if (source, flags) not in groups and origin != 'owner' and any(
                s['section'] > 0 and s['type'] & 0x20 for s in unit.coff.symbols.values()):
            return {}, (source, origin, 'behavioural provider skipped')
        return unit_definitions(module, source, flags, unit), (source, origin, 'checked')

    with ThreadPoolExecutor(jobs) as pool:
        compiled = dict(zip(profiles, pool.map(compile_profile, profiles)))
        results = list(pool.map(work, groups.items()))
        data = list(pool.map(work_data, owners))
    failed = []
    for out, definitions, failure in results:
        if failure:
            failed.append(failure)
        for r, found, own, names in out:
            linkage[r['id']] = names
            equal = 'refs' in found
            if own is not None:
                evidence.functions[own].append((int(r['rva'], 16), r['id'], equal))
            if not equal:
                evidence.refused[r['id']] = found['reason']
                if found.get('address_conflicts'):
                    evidence.collection_conflicts[r['id']] = found['address_conflicts']
                    if r['status'] in ACCEPTED:
                        evidence.accepted_collection_disagreements.add(r['id'])
                continue
            evidence.rows[r['id']] = dict(found, accepted=r['status'] in ACCEPTED, own=own)
        for key, found in definitions.items():
            for d in found:
                if d not in evidence.definitions[key]:
                    evidence.definitions[key].append(d)
    owner_linkage = {}
    for (module, source, flags, origin), (definitions, _) in zip(owners, data):
        if origin == 'owner':
            keys = collections.defaultdict(list)
            for key in definitions:
                keys[key[-1]].append(key)
            owner_linkage[module, source, flags] = {name: found[0] for name, found in keys.items() if len(found) == 1}
        for key, found in definitions.items():
            for d in found:
                if d not in evidence.definitions[key]:
                    evidence.definitions[key].append(d)
    def owner_scope(module, owner):
        return owner_linkage.get((module, owner['source'], tuple(owner['flags'])), {}).get(owner['symbol'])

    evidence.anchors, evidence.anchor_conflicts, unresolved, evidence.opaque_pins = accepted_anchors(
        ledger, lambda row, name: linkage.get(row['id'], {}).get(name), owner_scope)
    evidence.scope = dict(compile_failures=sorted(failed),
                          debug_failures=[dict(source=source, flags=flags, reason=unit.debug_error)
                                          for (source, flags), unit in sorted(compiled.items())
                                          if isinstance(unit, Unit) and unit.debug_error],
                          data_units=sorted('%s [%s] %s' % p for _, p in data),
                          unresolved_binding_names=sorted('%s %s' % u for u in unresolved),
                          opaque_pin_intervals=[dict(identity=label(k), rva=base, size=size)
                                                for k, base, size in evidence.opaque_pins])
    return evidence


def summarize(evidence, result, ledger):
    status = {r['id']: r for r in ledger}
    out = collections.defaultdict(lambda: [0, 0])
    for rid, grade in result['grades'].items():
        if status[rid]['status'] != 'CODEGEN_SIMILAR':
            continue
        module = Path(status[rid]['target']).name
        form = '' if evidence.images[module].relocated else ' (no fixups)'
        for k in ((module, grade + form), ('all', grade + form)):
            out[k][0] += 1
            out[k][1] += status[rid]['size']
    return {' '.join(k): v for k, v in sorted(out.items())}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('mode', choices=('report', 'check', 'record-debt'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    before = (ROOT / 'recovery.json').read_bytes()
    ledger = json.loads(before)['functions']
    evidence = gather(args.jobs)
    result = evaluate(evidence)
    if (ROOT / 'recovery.json').read_bytes() != before:
        raise ValueError('recovery.json changed during the run')
    summary = summarize(evidence, result, ledger)
    accepted = sum(r['accepted'] for r in evidence.rows.values())
    accepted_refused = sorted(r['id'] for r in ledger if r['status'] in ACCEPTED and r['id'] in evidence.refused)
    print('%d instruction-equal rows (%d accepted), %d symbols, %d conflicts, %d defects, %d accepted collisions' % (
        len(evidence.rows), accepted, result['symbols'], len(result['conflicts']), len(result['defects']),
        len(result['collisions'])))
    print('accepted rows: %d fail a tier rule (not witnesses), %d contradict an address' % (
        len(result['accepted_failing']), len(result['accepted_disagreement'])))
    if accepted_refused:
        print('%d accepted rows refused during collection' % len(accepted_refused))
    for name, (count, size) in summary.items():
        print('  %-28s %4d rows %7d B' % (name, count, size))
    if args.output:
        output = args.output.resolve()
        if not output.is_relative_to((ROOT / 'work').resolve()):
            parser.error('Reports belong under work/')
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(dict(
            scope='Whole-image reference consistency; not FUNCTION_MATCH or FUNCTION_MATCH_RELOCATED.',
            recovery_sha256=hashlib.sha256(before).hexdigest(), summary=summary, grades=result['grades'],
            reasons=result['reasons'], grade_b_reasons=result['grade_b_reasons'],
            conflicts=result['conflicts'], defects=result['defects'],
            collisions=result['collisions'], refused=evidence.refused, inventory=evidence.scope,
            collection_address_conflicts=evidence.collection_conflicts,
            accepted_binding_conflicts={label(key): conflict for key, conflict
                                        in sorted(evidence.anchor_conflicts.items(), key=lambda pair: str(pair[0]))},
            accepted_failing=result['accepted_failing'], accepted_refused=accepted_refused,
            accepted_disagreement=result['accepted_disagreement']), indent=1) + '\n', encoding='utf-8')
    if args.mode == 'record-debt':
        DEBT.write_text(json.dumps(dict(scope='Shrink-only whole-image name conflicts, definition defects and '
                                              'accepted-name collisions; see docs/consistency_tier.md.',
                                        conflicts=result['conflicts'], defects=result['defects'],
                                        collisions=result['collisions']), indent=1) + '\n',
                        encoding='utf-8', newline='\n')
    if args.mode == 'check':
        debt = json.loads(DEBT.read_bytes())
        recorded = set(debt['conflicts']) | set(debt['defects']) | set(debt.get('collisions', []))
        current = set(result['conflicts']) | set(result['defects']) | set(result['collisions'])
        new, gone = current - recorded, recorded - current
        for item in sorted(new):
            print('NEW', item)
        print('%d new, %d repaid' % (len(new), len(gone)))
        return 1 if new or result['accepted_disagreement'] else 0
    return 0


if __name__ == '__main__':
    sys.exit(main())
