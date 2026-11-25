"""Whole-image reference consistency of instruction-equal rows (a separate, weaker tier).

A row is CONSISTENT when its complete contribution is instruction-equal and every
reference fits ONE symbol-to-address map shared by the module's instruction-equal rows:
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
import collections
import glob
import hashlib
import json
import re
import struct
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from env import ROOT  # noqa: E402
from coff import COFF  # noqa: E402
from masked import DIR32, REL32, _compare_section  # noqa: E402
from match import compile_source  # noqa: E402
from pe import PE  # noqa: E402

ACCEPTED = {'FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED', 'LINKED_MATCH'}
DEBT = ROOT / 'evidence/consistency_debt.json'
MEM_EXECUTE = 0x20000000
CNT_CODE, CNT_UNINITIALIZED = 0x20, 0x80
EXTERNAL, STATIC = 2, 3
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
        self.fixups = {e.rva for b in getattr(raw, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries if e.type}
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


def accesses(decoder, code, rva, offset):
    """(kind, width, write) of the operand whose 4-byte field starts at ``offset``, or None.

    direct: absolute flat [disp32] access; indexed: disp32 with a register; value: an
    immediate or an address without access; branch: a rel32 field. ``width`` is the number
    of bytes the access proves to belong to the object: a store's full width, one byte for a
    read, because VC5 widens loads of 8- and 16-bit objects. Segment-overridden memory and
    state-save forms are refused (None).
    """
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    for ins in decoder.disasm(code, rva):
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


def table_fields(decoder, code, rva):
    """First offset at which linear decoding stops (inline tables follow code)."""
    end = 0
    for ins in decoder.disasm(code, rva):
        end = ins.address - rva + ins.size
    return end


WIDTHS = {'DB': 1, 'DW': 2, 'DD': 4, 'DQ': 8, 'DT': 10}
DATA_LINE = re.compile(r'^(\S*)\s+(DB|DW|DD|DQ|DT)\s+(.*)$')
NUMBER = re.compile(r'^(?:[0-9][0-9A-Fa-f]*H|[0-9]+)$')
SCALAR = re.compile(r'^-?(?:[0-9][0-9A-Fa-f]*[Hr]?|[0-9]+|FLAT:[\w$?@]+(?:[+-][0-9A-Fa-f]+H?)?|[\w$?@]+'
                    r'|[\w$?@]+[+-][0-9A-Fa-f]+H?|\?)$')
BOUNDARY = re.compile(r'^\s*(?:ALIGN\s+\d+|COMM\s.*|PUBLIC\s.*|EXTRN\s.*|;.*)?\s*$')


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
    if item.startswith("'") and item.endswith("'") and len(item) >= 2:
        return len(item) - 2 - item[1:-1].count("''")
    return width if item and SCALAR.match(item) else None


def listing_sizes(text):
    """name -> exact object size from the data segments of a compiler listing (None: unknown).

    Every line inside a data segment must be a data line, a complete ORG $+N, or a known
    boundary directive; anything else makes the current object unknown.
    """
    sizes, segment, name = {}, None, None
    for line in text.splitlines():
        match = re.match(r'^(\S+)\s+SEGMENT', line)
        if match:
            segment, name = match[1], None
            continue
        if re.match(r'^\S+\s+ENDS', line):
            segment, name = None, None
            continue
        if segment in (None, '_TEXT') or segment.startswith('.'):
            continue
        body = line.split(';', 1)[0].rstrip()
        match = re.match(r'^\s+ORG\s+\$\+(\S+)$', body)
        if match or re.match(r'^\s+ORG\b', body):
            if name is not None and sizes[name] is not None:
                sizes[name] = sizes[name] + number(match[1]) if match and NUMBER.match(match[1]) else None
            continue
        match = DATA_LINE.match(line)
        if not match:
            if not BOUNDARY.match(line) and name is not None:
                sizes[name] = None   # an unrecognized line inside an object
            name = None
            continue
        if match[1]:
            name = match[1]
            if name in sizes:
                sizes[name] = None
                name = None
                continue
            sizes[name] = 0
        if name is None or sizes[name] is None:
            continue
        operands = split_items(match[3])
        counted = [item_size(x, WIDTHS[match[2]]) for x in operands] if operands is not None else [None]
        sizes[name] = None if None in counted else sizes[name] + sum(counted)
    return sizes


class Unit:
    """Definitions of one freshly compiled object."""

    def __init__(self, coff, sizes=None):
        self.coff = coff
        self.sizes = sizes or {}
        self.by_name = collections.defaultdict(list)
        for s in coff.symbols.values():
            self.by_name[s['name']].append(s)

    def unique(self, name):
        found = self.by_name[name]
        return found[0] if len(found) == 1 else None

    def extent(self, symbol):
        """Exact listed size of a defined data symbol, or None.

        The listed size must fit before the next symbol of the section and leave less than
        16 bytes, which can only be alignment.
        """
        section = self.coff.sections[symbol['section'] - 1]
        later = [s['value'] for s in self.coff.symbols.values()
                 if s['section'] == symbol['section'] and s['value'] > symbol['value']]
        room = min(later + [section['size']]) - symbol['value']
        size = self.sizes.get(symbol['name'])
        return size if size and size <= room < size + 16 else None


def key_of(module, source, flags, symbol):
    """Linker-visible identity: externals per module, file-local names per object profile."""
    if symbol['storage'] == EXTERNAL:
        return module, symbol['name']
    return module, source, tuple(flags), symbol['name']


def ref_kind(name, function):
    return 'import' if name.startswith('__imp_') else 'function' if function else 'data'


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
    result = _compare_section(coff, section, number_, image.pe, rva, size)
    if not result['masked_equal']:
        return dict(reason='not instruction-equal')
    native = image.read(rva, size)
    stop = table_fields(decoder, native, rva)
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
        use = accesses(decoder, native, rva, r['offset']) if r['offset'] < stop else None
        if symbol['section'] == number_:
            # Own contribution: masked._compare_section fixed the arithmetic. The field must have the
            # role of its relocation type, and the target must stay inside the contribution.
            if r['type'] == REL32 and (use is None or use[0] != 'branch'):
                return dict(reason='own-section REL32 that is not a branch at %#x' % r['offset'])
            if r['type'] == DIR32 and use is not None and use[0] == 'branch':
                return dict(reason='own-section DIR32 in a branch at %#x' % r['offset'])
            if not rva <= target < rva + size:
                return dict(reason='own-section reference leaves the contribution at %#x' % r['offset'])
            if symbol['type'] & 0x20:
                if target != rva or addend:
                    return dict(reason='own function reference is not the entry at %#x' % r['offset'])
                refs.append(dict(key=key_of(module, source, flags, symbol), name=r['symbol'], base=rva, addend=0,
                                 use='branch' if r['type'] == REL32 else 'value', width=0, write=False,
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
    if symbol['section'] == 0:
        return dict(size=symbol['value'], payload=None, refs=[]) if symbol['value'] else None
    section = unit.coff.sections[symbol['section'] - 1]
    size, start = unit.extent(symbol), symbol['value']
    if size is None:
        return dict(size=0, payload=None, refs=None)
    if not len(section['data']) or section['flags'] & CNT_UNINITIALIZED:
        return dict(size=size, payload=None, refs=[])
    refs = []
    for r in section['relocations']:
        if r['offset'] + 4 > start and r['offset'] < start + size:
            target = unit.unique(r['symbol'])
            if target is None or r['type'] != DIR32 or r['offset'] < start or r['offset'] + 4 > start + size:
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
        self.definitions = collections.defaultdict(list)   # key -> [definition]
        self.functions = collections.defaultdict(list)     # function key -> [(rva, id, equal)]
        self.anchors = {}                    # key -> dict(rva, size, kind, alias_of) or (rva, size[, kind[, alias]])
        self.anchor_conflicts = set()        # keys with contradictory accepted records
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
    pointers = {o for r in definition['refs'] for o in range(r['offset'], r['offset'] + 4)}
    if any(native[i] != own[i] for i in range(size) if i not in pointers):
        return 'payload differs'
    if image.relocated and fields != {base + r['offset'] for r in definition['refs']}:
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


def evaluate(evidence, minimum=2):
    """Apply the rules. Returns grades, conflicts, defects and the reason each pending row fails."""
    rows, images = evidence.rows, evidence.images
    problems = {}                     # key -> key-wide failure

    def problem(key, text):
        problems.setdefault(key, text)

    # Occurrences: every use of a key with the value it implies, from rows and from placed definitions.
    occurrences = collections.defaultdict(list)    # key -> [dict(source, value, kind, addend, ...)]
    names = {}
    for rid, row in rows.items():
        for i, ref in enumerate(row['refs']):
            kind = ref_kind(ref['name'], ref['function'])
            occurrences[ref['key']].append(dict(source=('row', rid, i), value=ref['base'], kind=kind,
                                                addend=ref['addend'], ref=ref))
            names[ref['key']] = ref['name']

    def import_slot(key):
        plain = import_name(names.get(key, ''))
        slots = [s for s, n in images[key[0]].imports.items() if n == plain] if plain else []
        return slots[0] if len(slots) == 1 else None

    def function_entry(key):
        """Canonical entry of a function key: the accepted pin, else its recorded rows, else uses."""
        anchor = anchor_record(evidence, key)
        ledger = {d[0] for d in evidence.functions.get(key, [])}
        if anchor and anchor['kind'] == 'function':
            if ledger and ledger != {anchor['rva']}:
                problem(key, 'several addresses')
            return anchor['rva']
        if len(ledger) > 1:
            problem(key, 'several addresses')
            return None
        if ledger:
            return next(iter(ledger))
        values = {o['value'] for o in occurrences[key]}
        if len(values) > 1:
            problem(key, 'several addresses')
        return next(iter(values)) if len(values) == 1 else None

    def data_base(key):
        anchor = anchor_record(evidence, key)
        if anchor and anchor['kind'] != 'function':
            return anchor['rva']
        values = {o['value'] for o in occurrences[key]}
        return next(iter(values)) if len(values) == 1 else None

    # Placed definitions bind their pointer targets (never as witnesses); iterate to a fixpoint.
    edges = collections.defaultdict(list)          # source key -> [(target key, occurrence)]
    local, processed = {}, set()
    while True:
        pending = [k for k in list(occurrences) if k not in processed and evidence.definitions.get(k)
                   and data_base(k) is not None]
        if not pending:
            break
        for key in pending:
            processed.add(key)
            image, base = images[key[0]], data_base(key)
            for d in evidence.definitions[key]:
                failure = payload_local(image, base, d)
                if failure:
                    local.setdefault(key, failure)
                    continue
                if d['payload'] is None:
                    continue
                native = image.read(base, d['size'])
                for r in d['refs']:
                    addend = struct.unpack_from('<i', d['payload'], r['offset'])[0]
                    value = (struct.unpack_from('<I', native, r['offset'])[0] - image.base - addend) & 0xffffffff
                    kind = ref_kind(r['name'], r['function'])
                    occurrence = dict(source=('edge', key), value=value, kind=kind, addend=addend, ref=None)
                    occurrences[r['key']].append(occurrence)
                    names.setdefault(r['key'], r['name'])
                    edges[key].append((r['key'], occurrence))
    for key, failure in local.items():
        problem(key, failure)

    # Kinds: one key is one kind of object everywhere (uses, definitions, pins and recorded rows).
    kinds = {}
    for key, found in occurrences.items():
        seen = {o['kind'] for o in found}
        anchor = anchor_record(evidence, key)
        if anchor:
            seen.add('function' if anchor['kind'] == 'function' else 'data')
        if evidence.definitions.get(key):
            seen.add('data')
        if evidence.functions.get(key):
            seen.add('function')
        kinds[key] = seen
        if len(seen) > 1:
            problem(key, 'kind conflict')
    for key in evidence.anchor_conflicts:
        problem(key, 'accepted bindings disagree')

    # Canonical value per key, then key-level validity.
    canonical, content, reach = {}, set(), collections.defaultdict(int)
    for key, seen in kinds.items():
        if key in problems:
            continue
        kind = next(iter(seen))
        image = images[key[0]]
        if kind == 'function':
            entry = function_entry(key)
            if entry is None:
                problem(key, problems.get(key, 'callee without a row'))
                continue
            canonical[key] = entry
            defs = evidence.functions.get(key, [])
            anchor = anchor_record(evidence, key)
            if image.zone(entry, 1) != 'code':
                problem(key, 'callee outside code')
            elif defs:
                if not all(d[2] for d in defs):
                    problem(key, 'callee not instruction-equal')
            elif not (anchor and anchor['kind'] == 'function'):
                problem(key, 'callee recorded under another name' if (key[0], entry) in evidence.starts
                        else 'callee without a row')
            if key not in problems:
                content.add(key)
            continue
        if kind == 'import':
            slot = import_slot(key)
            if slot is None:
                problem(key, 'import slot')
            else:
                canonical[key] = slot
                content.add(key)
            continue
        base = data_base(key)
        if base is None:
            problem(key, 'several addresses')
            continue
        canonical[key] = base
        for o in occurrences[key]:
            ref = o['ref']
            if ref and o['value'] == base and ref['use'] == 'direct' and ref['addend'] >= 0:
                reach[key] = max(reach[key], ref['addend'] + ref['width'])

    # Storage items: accepted pins, IAT slots, data keys and any extent a use adds beyond a pin.
    items = []        # (start, end, owner, authority label or None)
    for key in evidence.anchors:
        anchor = anchor_record(evidence, key)
        if anchor and anchor['kind'] != 'function' and anchor['size']:
            items.append((anchor['rva'], anchor['rva'] + anchor['size'], ('key', key),
                          'pin:' + (anchor.get('alias_of') or key[-1])))
    for module, image in images.items():
        for slot in image.imports:
            items.append((slot, slot + 4, ('key', (module, ('iat', slot))), 'iat:%#x' % slot))
    occurrence_items = {}
    for key, seen in kinds.items():
        if key in problems or 'data' not in seen:
            continue
        base, image = canonical[key], images[key[0]]
        definitions = evidence.definitions.get(key, [])
        anchor = anchor_record(evidence, key)
        defined = max([d['size'] for d in definitions] + [1])
        zone = image.zone(base, max(defined, reach[key]))
        if zone in (None, 'code'):
            problem(key, 'not inside one data zone')
            continue
        if not definitions and zone == 'file' and not anchor:
            problem(key, 'initialized data without a definition')
        if any(reach[key] > d['size'] for d in definitions):
            problem(key, 'access beyond the definition')
        if anchor:
            if defined > anchor['size']:
                items.append((base, base + defined, ('key', key), None))
            for o in occurrences[key]:
                ref = o['ref']
                if ref and o['value'] == base and ref['use'] == 'direct' and ref['addend'] >= 0:
                    end = ref['addend'] + ref['width']
                    if end > anchor['size']:
                        owner = ('use',) + o['source']
                        occurrence_items[owner] = key
                        items.append((base, base + end, owner, None))
        else:
            items.append((base, base + max(defined, reach[key]), ('key', key), None))
        if key not in problems and (anchor or any(literal_content(d) for d in definitions)):
            content.add(key)

    collisions, failed_uses = set(), {}
    by_module = collections.defaultdict(list)
    for start, end, owner, authority in items:
        module = owner[1][0] if owner[0] == 'key' else rows[owner[2]]['module'] if owner[1] == 'row' else None
        if module is None:
            module = occurrence_items[owner][0]
        by_module[module].append((start, end, owner, authority))

    def object_of(owner):
        return owner[1] if owner[0] == 'key' else occurrence_items[owner]

    for module, entries in by_module.items():
        entries.sort(key=lambda t: (t[0], t[1], str(t[2])))
        active = []
        for start, end, owner, authority in entries:
            active = [x for x in active if x[1] > start]
            for other in active:
                if object_of(other[2]) == object_of(owner):
                    continue
                if authority and other[3]:
                    if authority != other[3]:
                        collisions.add(tuple(sorted((object_of(other[2]), object_of(owner)), key=str)))
                    continue
                for item_owner, item_authority in ((owner, authority), (other[2], other[3])):
                    if item_authority:
                        continue
                    if item_owner[0] == 'key':
                        problem(item_owner[1], 'shares storage with another name')
                    else:
                        failed_uses[item_owner] = 'access beyond the accepted extent overlaps another object'
            active.append((start, end, owner, authority))
    # A valid import's key is its IAT slot; ordinary keys must not overlap a slot (handled above).

    # Every occurrence must agree with the canonical value; failures flow back through definitions.
    def occurrence_problem(key, o):
        if key in problems:
            return problems[key]
        if o['kind'] != 'data' and o['addend']:
            return 'function or import reference with an addend'
        if canonical.get(key) != o['value']:
            return 'several addresses'
        return None

    changed = True
    while changed:
        changed = False
        for source, targets in edges.items():
            if source in problems:
                continue
            for target, o in targets:
                failure = occurrence_problem(target, o)
                if failure:
                    problems[source] = 'payload pointer: ' + (
                        'disagrees with the map' if failure == 'several addresses' else failure)
                    changed = True
                    break

    def ref_problem(rid, i, ref):
        key = ref['key']
        o = next(x for x in occurrences[key] if x['source'] == ('row', rid, i))
        failure = occurrence_problem(key, o)
        if failure:
            return failure
        if ('use', 'row', rid, i) in failed_uses:
            return failed_uses['use', 'row', rid, i]
        if ref['use'] == 'direct' and ref['addend'] < 0:
            return 'direct access before the object'
        return None

    reasons, failing_accepted, live, disagreeing = {}, [], set(), set()
    hard = ('several addresses', 'accepted bindings disagree', 'import slot', 'kind conflict',
            'payload pointer: disagrees with the map')
    for rid, row in rows.items():
        found = [(ref_problem(rid, i, r), r) for i, r in enumerate(row['refs'])]
        bad = sorted({p + ': ' + r['name'] for p, r in found if p})
        if bad:
            if row['accepted']:
                failing_accepted.append(rid)
                if any(p and p.startswith(hard) for p, _ in found):
                    disagreeing.add(rid)
            else:
                reasons[rid] = bad
        else:
            live.add(rid)

    def closure(row):
        seen, stack = set(), [r['key'] for r in row['refs']]
        while stack:
            key = stack.pop()
            if key in seen:
                continue
            seen.add(key)
            stack.extend(t for t, o in edges.get(key, ()) if o['kind'] == 'data')
        return seen

    closures = {rid: closure(rows[rid]) for rid in live}
    users = collections.defaultdict(set)
    for rid in live:
        for r in rows[rid]['refs']:
            users[r['key']].add(rid)
    # Rows whose extents overlap are one witness.
    group = {}
    for module in {rows[r]['module'] for r in rows}:
        spans = sorted((rows[r]['start'], rows[r]['start'] + rows[r]['size'], r) for r in rows if rows[r]['module'] == module)
        current, end = None, -1
        for start, stop, rid in spans:
            if start >= end:
                current = rid
            end = max(end, stop)
            group[rid] = current

    def witnessed(key, strong):
        found = [rows[i] for i in users[key] if i in strong]
        return min(len({r.get('native', r['body']) for r in found}), len({r['body'] for r in found}),
                   len({group.get(r_id) for r_id in users[key] if r_id in strong}))

    collided = {k for pair in collisions for k in pair}
    strong = {rid for rid in live if not closures[rid] & collided}
    while True:
        weak = {rid for rid in strong if any(
            key not in content and witnessed(key, strong) < minimum for key in closures[rid])}
        if not weak:
            break
        strong -= weak
    grades = {rid: ('A' if rid in strong else 'B') for rid in live if not rows[rid]['accepted']}

    named = ('several addresses', 'shares storage', 'accepted bindings disagree', 'callee recorded under',
             'kind conflict', 'payload pointer: disagrees with the map')
    conflicts = {label(k) + ' | ' + p for k, p in problems.items() if p.startswith(named)}
    for key, found in occurrences.items():
        if key not in problems and any(occurrence_problem(key, o) == 'several addresses' for o in found):
            conflicts.add(label(key) + ' | several addresses')
    defects = sorted({label(k) + ' | ' + p for k, p in problems.items()
                      if p in ('payload differs', 'payload fixups differ', 'access beyond the definition',
                               'zero definition is not zero in the image', 'zero definition carries loader fixups',
                               'unsupported definition')})
    return dict(grades=grades, reasons=reasons, conflicts=sorted(conflicts), defects=defects, problems=problems,
                accepted_failing=sorted(failing_accepted), accepted_disagreement=sorted(disagreeing),
                collisions=sorted(' / '.join(label(k) for k in pair) for pair in collisions),
                symbols=len(kinds), content=len(content))


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
            if record['kind'] not in ('data', 'bss', 'function') or name.startswith(('$', '.')):
                continue
            kind = 'function' if record['kind'] == 'function' else 'data'
            alias = record['owner']['symbol'] if 'source_alias' in record and 'owner' in record else None
            yield row, module, name, dict(rva=record['rva'], size=record.get('size', 0) if kind == 'data' else 0,
                                          kind=kind, alias_of=alias)


def accepted_anchors(ledger, scope):
    """Anchors keyed by each bound name's real linkage, and the keys whose records disagree.

    ``scope(row, name)`` returns the key of ``name`` in the row's own freshly compiled unit
    (file-local names are scoped to their object), or None when the linkage is unknown.
    """
    out, conflicts, unresolved = {}, set(), []
    for row, module, name, entry in binding_records(ledger):
        key = scope(row, name)
        if key is None:
            unresolved.append((row['id'], name))
            continue
        found = out.setdefault(key, dict(entry))
        if (found['rva'], found['kind']) != (entry['rva'], entry['kind']):
            conflicts.add(key)
        elif found['size'] < entry['size']:
            found['size'] = entry['size']
    for key in conflicts:
        out.pop(key, None)
    return out, conflicts, unresolved


def compile_unit(source, flags):
    """One fresh object with its listing; the listing option is the only addition to the profile."""
    obj, _ = compile_source(ROOT / source, [*flags, '/Falisting.asm'])
    return Unit(COFF(obj), listing_sizes((obj.parent / 'listing.asm').read_text(encoding='latin-1')))


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

    def work(item):
        (source, flags), members = item
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        try:
            unit = compile_unit(source, flags)
        except (RuntimeError, ValueError) as error:
            return [(r, dict(reason='compile: ' + str(error)[:120]), None, {}) for r in members], {}, source
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
        try:
            unit = compile_unit(source, flags)
        except (RuntimeError, ValueError):
            return {}, (source, origin, 'compile failed')
        if (source, flags) not in groups and origin != 'owner' and any(
                s['section'] > 0 and s['type'] & 0x20 for s in unit.coff.symbols.values()):
            return {}, (source, origin, 'behavioural provider skipped')
        return unit_definitions(module, source, flags, unit), (source, origin, 'checked')

    with ThreadPoolExecutor(jobs) as pool:
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
                continue
            evidence.rows[r['id']] = dict(found, accepted=r['status'] in ACCEPTED, own=own)
        for key, found in definitions.items():
            for d in found:
                if d not in evidence.definitions[key]:
                    evidence.definitions[key].append(d)
    for definitions, provenance in data:
        for key, found in definitions.items():
            for d in found:
                if d not in evidence.definitions[key]:
                    evidence.definitions[key].append(d)
    evidence.anchors, evidence.anchor_conflicts, unresolved = accepted_anchors(
        ledger, lambda row, name: linkage.get(row['id'], {}).get(name))
    evidence.scope = dict(compile_failures=sorted(failed),
                          data_units=sorted('%s [%s] %s' % p for _, p in data),
                          unresolved_binding_names=sorted('%s %s' % u for u in unresolved))
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
    print('%d instruction-equal rows (%d accepted), %d symbols, %d conflicts, %d defects, %d accepted collisions' % (
        len(evidence.rows), accepted, result['symbols'], len(result['conflicts']), len(result['defects']),
        len(result['collisions'])))
    print('accepted rows: %d fail a tier rule (not witnesses), %d contradict an address' % (
        len(result['accepted_failing']), len(result['accepted_disagreement'])))
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
            reasons=result['reasons'], conflicts=result['conflicts'], defects=result['defects'],
            collisions=result['collisions'], refused=evidence.refused, inventory=evidence.scope,
            accepted_failing=result['accepted_failing'],
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
