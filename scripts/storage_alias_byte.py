"""Opt-in compile-renaming-byte-v1: complete one-byte scalar aliases by fresh compiler renaming.

A loader-zero ``bss`` record opts in with ``alias_profile: "compile-renaming-byte-v1"``.
storage_alias.py, zero_storage.py and masked.py stay byte-identical and keep
rejecting one-byte compile-renaming records. match.py routes only opted-in records
here (owner/alias profiles), and the fixed_disp32.py comparator copy calls
verify_record for them.

What is proved, and nothing more:
- The record is one complete one-byte scalar (size 1) with exactly one
  caller->owner C-name rename, carried by a pinned canonical header that contains
  only that forward macro and one ``extern <char type> owner;`` declaration.
  match.py compiles the adapted caller with the row flags plus /WX: MSVC5 reports a
  signed/unsigned/plain char redeclaration only as warning C4142 (measured), so
  /WX is what makes that declaration a hard caller-type check. /WX changes no code.
- The unchanged storage_alias.verify_alias_object holds: every function, other
  executable section, non-code contribution and the external inventory are equal
  after only that data-name rename (no object bytes are edited).
- Every use of the caller name in the caller object is an executable,
  non-LEA instruction with exactly one direct absolute byte memory operand
  [disp32] (no base, index or segment) at addend 0, relocated by DIR32. Immediate
  address uses, LEA, wider or offset accesses and data/directive relocations
  (address escapes) reject.
The owner remains the unchanged zero_storage.verify_common exact one-byte COMMON
check, and native identity remains the independent loader-zero anchor. No
capacity, neighbour, signedness of the owner definition or source/TU identity
is inferred.
"""
from pathlib import Path
import re
import struct

from coff import COFF

PROFILE = 'compile-renaming-byte-v1'
BYTE_TYPES = ('signed char', 'unsigned char', 'char')
DIR32 = 6
# Canonical source roots allowed for the header and the owner. Diagnostics may
# extend this in-process; canonical verification uses only src/.
SOURCE_ROOTS = ('src',)


def _identifier(value):
    return isinstance(value, str) and re.fullmatch(r'[A-Za-z_]\w*', value, re.ASCII) is not None


def _within_roots(path, root):
    return any(path.is_relative_to((root / base).resolve()) for base in SOURCE_ROOTS)


def alias_names(record, name):
    if not isinstance(record, dict) or record.get('alias_profile') != PROFILE:
        raise ValueError('Unsupported explicit alias profile')
    if record.get('kind') != 'bss' or type(record.get('size')) is not int or record['size'] != 1:
        raise ValueError('Byte compiler-renaming supports complete one-byte bss scalars only')
    if any(key in record for key in ('owner_storage', 'access_profile', 'anchor_profile', 'fixed_disp32')):
        raise ValueError('Byte compiler-renaming requires the unchanged COMMON owner and legacy anchor')
    owner = record.get('owner')
    alias = record.get('source_alias')
    if not isinstance(owner, dict) or not isinstance(alias, dict) or set(alias) != {'kind', 'header', 'renames'}:
        raise ValueError('Byte compiler-renaming requires an explicit owner and source_alias')
    if alias['kind'] != 'compile-renaming' or not isinstance(alias['header'], str):
        raise ValueError('Byte compiler-renaming requires kind compile-renaming and a canonical header')
    renames = alias['renames']
    if (not isinstance(renames, dict) or len(renames) != 1 or
            any(not _identifier(a) or not _identifier(b) or a == b for a, b in renames.items())):
        raise ValueError('Byte compiler-renaming needs exactly one distinct caller/owner C identifier pair')
    (caller, actual), = renames.items()
    if name != '_' + caller or owner.get('symbol') != '_' + actual:
        raise ValueError('Byte compiler-renaming disagrees with the exact caller/owner names')
    return {'_' + caller: '_' + actual}


def check_header(header, names):
    """Only the forward macro and the owner's exact char-type declaration."""
    (caller, actual), = names.items()
    content = re.sub(r'/\*.*?\*/', ' ', header.read_text(encoding='ascii'), flags=re.S)
    if '//' in content or '\\\n' in content:
        raise ValueError('Byte renaming header must use plain block comments and single lines')
    seen = []
    for line in content.splitlines():
        if not line.strip():
            continue
        macro = re.fullmatch(r'\s*#\s*define\s+([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*', line, re.ASCII)
        decl = re.fullmatch(r'\s*extern\s+(signed\s+char|unsigned\s+char|char)\s+([A-Za-z_]\w*)\s*;\s*', line, re.ASCII)
        if macro:
            seen.append(('define',) + macro.groups())
        elif decl:
            seen.append(('extern', ' '.join(decl.group(1).split()), decl.group(2)))
        else:
            raise ValueError('Byte renaming header may contain only its macro and owner declaration')
    defines = [s for s in seen if s[0] == 'define']
    externs = [s for s in seen if s[0] == 'extern']
    if defines != [('define', caller[1:], actual[1:])] or len(externs) != 1 or externs[0][2] != actual[1:] or \
            externs[0][1] not in BYTE_TYPES:
        raise ValueError('Byte renaming header differs from its exact macro and owner declaration')
    return externs[0][1]


def alias_profile(record, name, root):
    names = alias_names(record, name)
    header = (root / record['source_alias']['header']).resolve()
    if not _within_roots(header, root) or header.suffix != '.h' or not header.is_file():
        raise ValueError('Compiler-renaming header must be existing canonical source')
    check_header(header, names)
    return header, names


def owner_profile(record, name, root):
    """zero_storage.owner_profile for opted-in records; same source/flags rules, C owner only."""
    owner = record.get('owner')
    if not isinstance(owner, dict) or set(owner) != {'source', 'flags', 'symbol'}:
        raise ValueError('Loader-zero binding requires an explicit source/flags/symbol owner')
    alias_profile(record, name, root)
    if not isinstance(owner['source'], str) or not isinstance(owner['flags'], list) or any(
            not isinstance(flag, str) for flag in owner['flags']):
        raise ValueError('Invalid loader-zero owner compile profile')
    source = (root / owner['source']).resolve()
    if not _within_roots(source, root):
        raise ValueError('Loader-zero owner must be canonical repository source')
    if source.suffix != '.c' or any(flag.lower().startswith(('/tp', '-tp')) for flag in owner['flags']):
        raise ValueError('Byte compiler-renaming requires a C owner profile')
    return source, tuple(owner['flags'])


def verify_byte_uses(obj, name):
    """Every relocation naming ``name`` is a direct [disp32] byte memory operand at addend 0."""
    import capstone
    coff = COFF(obj)
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    functions = {s['section']: s['name'] for s in coff.symbols.values() if s['section'] > 0 and s['type'] & 0x20}
    receipts = []
    for number, section in enumerate(coff.sections, 1):
        uses = [r for r in section['relocations'] if r['symbol'] == name]
        if not uses:
            continue
        if not section['flags'] & 0x20 or section['flags'] & 0x800 or number not in functions:
            raise ValueError('Byte alias address escapes through non-code or unowned storage')
        data = section['data']
        instructions = list(decoder.disasm(data, 0))
        if sum(i.size for i in instructions) != len(data):
            raise ValueError('Byte alias use section does not decode completely')
        fields = {}
        for ins in instructions:
            if ins.disp_size == 4:
                fields[ins.address + ins.disp_offset] = ('disp', ins)
            if ins.imm_size == 4:
                fields[ins.address + ins.imm_offset] = ('imm', ins)
        for rel in uses:
            off = rel['offset']
            kind, ins = fields.get(off, (None, None))
            if rel['type'] != DIR32 or kind != 'disp':
                raise ValueError('Byte alias use is not a direct absolute memory displacement')
            mem = [o for o in ins.operands if o.type == capstone.x86.X86_OP_MEM]
            if (ins.mnemonic == 'lea' or ins.group(capstone.CS_GRP_CALL) or ins.group(capstone.CS_GRP_JUMP) or
                    len(mem) != 1 or mem[0].mem.base or mem[0].mem.index or mem[0].mem.segment or mem[0].size != 1):
                raise ValueError('Byte alias use must be one direct one-byte memory operand')
            if struct.unpack_from('<i', data, off)[0] != 0:
                raise ValueError('Byte alias use must address the complete scalar at addend 0')
            receipts.append(dict(function=functions[number], offset=off, mnemonic=ins.mnemonic, width=1, addend=0))
    if not receipts:
        raise ValueError('Byte alias caller does not use its renamed scalar')
    return receipts


def verify_record(caller, adapted, symbol, name, record):
    """Comparator hook: fresh renaming equality plus the complete byte-use survey."""
    names = alias_names(record, name)
    if adapted is None:
        raise ValueError('Loader-zero aliases need a freshly compiled rename object')
    from storage_alias import verify_alias_object
    check = verify_alias_object(caller, adapted, symbol, names)
    caller_uses = verify_byte_uses(caller, name)
    adapted_uses = verify_byte_uses(adapted, names[name])
    if [dict(u, function=None) for u in caller_uses] != [dict(u, function=None) for u in adapted_uses]:
        raise ValueError('Byte alias uses differ between the caller and its renamed object')
    return dict(check, profile=PROFILE, size=1, uses=caller_uses)
