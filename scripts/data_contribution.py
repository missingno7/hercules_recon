"""Opt-in placement of one complete initialized COFF data contribution (data-contribution-v1).

Question answered: where does a freshly compiled unit's initialized ``.data`` or
``.rdata`` contribution (or one initialized data COMDAT, such as a string
literal) lie in the original image? The whole contribution carries the
identity, so no per-object size record is needed:

* the contribution must be complete initialized, non-executable raw data;
  ``.bss``/uninitialized storage has no content and is always rejected;
* every relocation must be one complete, disjoint DIR32 field;
* every byte outside those fields is compared, including compiler padding,
  and at least ``MIN_NONZERO`` of them must be nonzero;
* the masked pattern must occur exactly once at any byte position (overlapping
  matches included) across the PE headers and every section's virtual image
  (file bytes plus loader-zero fill). Zero hits or two hits reject;
* the one hit must be wholly file-backed, aligned to the COFF section
  alignment, in a non-executable initialized-data PE section with the same
  writability;
* when the image has a base-relocation directory, its HIGHLOW entries inside
  the hit must be exactly the compiler DIR32 fields;
* every DIR32 field must contain ``image_base + target + addend``, where the
  target is this same contribution (resolved against the hit), another
  initialized section of the same object placed by this module, or an
  explicitly supplied independently bound RVA. An unresolved target rejects
  the whole contribution.

Each plain defined object symbol is then bound at ``base + symbol offset``.
Its extent is the allocation interval up to the next symbol or the end of the
contribution, including any compiler padding (``extent_kind``). It bounds the
bytes this symbol's allocation occupies, never the C type's size. Symbols
that share an offset are aliases and are not bound. A symbol whose interval
contains DIR32 fields still needs relocated-data-v1 with independent
payload identities when it is used in a binding document.

This module is a diagnostic evidence producer. It neither edits recovery
state nor changes the default verifier; its placement is evidence for an
ordinary ``kind: data`` record (rva, allocation size, payload SHA-256), which
match.py and masked.py then compare as usual.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

PROFILE = 'data-contribution-v1'
MIN_NONZERO = 4
DIR32 = 0x06
UNINITIALIZED, CODE, INITIALIZED, DISCARDABLE = 0x80, 0x20, 0x40, 0x02000000
EXECUTE, WRITE, COMDAT = 0x20000000, 0x80000000, 0x1000
MAX_HITS = 2


class PlacementError(ValueError):
    pass


def alignment(flags):
    code = (flags >> 20) & 0xf
    return 1 << (code - 1) if code else 16


def virtual_image(pe):
    """Header bytes plus each section's file-backed bytes and its loader-zero fill."""
    headers = pe.data[:pe.pe.OPTIONAL_HEADER.SizeOfHeaders]
    regions = [(0, headers, 0, None)]
    for s in pe.sections:
        backed = min(s['virtual_size'], s['raw_size'])
        raw = pe.read_rva(s['virtual_address'], backed) if backed else b''
        regions.append((s['virtual_address'], raw + bytes(max(s['virtual_size'], backed) - backed), backed, s))
    return regions


def _pattern(data, masked):
    parts = []
    for i, byte in enumerate(data):
        parts.append(b'(?s:.)' if i in masked else re.escape(bytes([byte])))
    return re.compile(b'(?=' + b''.join(parts) + b')', re.S)


def place(coff, number, pe, bound=None, placed=None):
    """Place COFF section ``number`` (1-based) uniquely in ``pe`` or raise PlacementError."""
    bound, placed = dict(bound or {}), dict(placed or {})
    if not 1 <= number <= len(coff.sections):
        raise PlacementError('Unknown COFF section')
    sec = coff.sections[number - 1]
    flags, size, data = sec['flags'], sec['size'], sec['data']
    if flags & UNINITIALIZED or not data:
        raise PlacementError('Uninitialized (.bss) storage has no content identity')
    if flags & (CODE | EXECUTE) or not flags & INITIALIZED or flags & DISCARDABLE:
        raise PlacementError('Contribution is not initialized non-executable data')
    if sec['name'] not in ('.data', '.rdata') or len(data) != size or size <= 0:
        raise PlacementError('Unsupported or incomplete data section')
    masked, fields = set(), []
    for rel in sorted(sec['relocations'], key=lambda r: r['offset']):
        cells = set(range(rel['offset'], rel['offset'] + 4))
        if rel['type'] != DIR32 or rel['offset'] < 0 or rel['offset'] + 4 > size or masked & cells:
            raise PlacementError('Unsupported, partial or overlapping data relocation')
        masked |= cells
        fields.append(rel)
    compared = size - len(masked)
    nonzero = sum(1 for i in range(size) if i not in masked and data[i])
    if nonzero < MIN_NONZERO:
        raise PlacementError('Too little compared content to identify a placement')

    regex, hits = _pattern(data, masked), []
    for start, content, backed, native in virtual_image(pe):
        for match in regex.finditer(content):
            hits.append((start + match.start(), match.start(), backed, native))
            if len(hits) >= MAX_HITS:
                break
        if len(hits) >= MAX_HITS:
            break
    if not hits:
        raise PlacementError('Contribution is absent from the native image')
    if len(hits) > 1:
        raise PlacementError('Ambiguous contribution: %d or more native occurrences (first %s)'
                             % (len(hits), ', '.join(hex(h[0]) for h in hits)))
    base, local, backed, native = hits[0]
    if native is None or local + size > backed:
        raise PlacementError('Unique occurrence is not wholly file-backed section data')
    characteristics = native['characteristics']
    if (characteristics & (CODE | EXECUTE) or not characteristics & INITIALIZED or
            bool(characteristics & WRITE) != bool(flags & WRITE)):
        raise PlacementError('Unique occurrence lies in an incompatible native section')
    if base % alignment(flags):
        raise PlacementError('Unique occurrence violates the contribution alignment')

    offsets = {rel['offset'] for rel in fields}
    if pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size:
        native_fixups = sorted((e.rva - base, e.type) for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                               for e in b.entries if e.type and base - 3 <= e.rva < base + size)
        if native_fixups != sorted((off, 3) for off in offsets):
            raise PlacementError('Native HIGHLOW inventory differs from compiler DIR32 fields')

    by_name = {}
    for s in coff.symbols.values():
        by_name.setdefault(s['name'], []).append(s)
    resolved = []
    for rel in fields:
        choices = by_name.get(rel['symbol'], [])
        if len(choices) != 1:
            raise PlacementError('Missing or ambiguous relocation target ' + rel['symbol'])
        target = choices[0]
        addend = struct.unpack_from('<i', data, rel['offset'])[0]
        if target['section'] == number:
            destination, basis = base + target['value'], 'same-contribution'
        elif target['section'] > 0 and target['section'] in placed:
            destination, basis = placed[target['section']] + target['value'], 'placed-contribution'
        elif rel['symbol'] in bound:
            destination, basis = bound[rel['symbol']], 'independent-binding'
        else:
            raise PlacementError('Unresolved relocation target ' + rel['symbol'])
        value = struct.unpack_from('<I', pe.read_rva(base + rel['offset'], 4))[0]
        if value != (pe.image_base + destination + addend) & 0xffffffff:
            raise PlacementError('Native relocation field differs from bound target ' + rel['symbol'])
        resolved.append(dict(offset=rel['offset'], symbol=rel['symbol'], addend=addend,
                             destination_rva=destination, basis=basis))

    native_bytes = pe.read_rva(base, size)
    if any(native_bytes[i] != data[i] for i in range(size) if i not in masked):
        raise PlacementError('Native bytes differ outside relocation fields')
    objects = [s for s in coff.symbols.values() if s['section'] == number and not s['aux']]
    if any(s['type'] & 0x20 or s['storage'] not in (2, 3) for s in objects):
        raise PlacementError('Data contribution carries unsupported symbol records')
    values = sorted({s['value'] for s in objects} | {size})
    symbols, aliases = {}, []
    for s in sorted(objects, key=lambda x: (x['value'], x['name'])):
        if sum(o['value'] == s['value'] for o in objects) != 1:
            aliases.append(s['name'])
            continue
        if not 0 <= s['value'] < size:
            raise PlacementError('Symbol lies outside its contribution')
        end = min(v for v in values if v > s['value'])
        inside = [r['offset'] - s['value'] for r in fields if s['value'] <= r['offset'] < end]
        symbols[s['name']] = dict(offset=s['value'], rva=base + s['value'], size=end - s['value'],
                                  extent_kind='allocation-interval',
                                  sha256=hashlib.sha256(native_bytes[s['value']:end]).hexdigest(),
                                  relocated_fields=inside)
    return dict(profile=PROFILE, section=number, name=sec['name'], comdat=bool(flags & COMDAT),
                size=size, alignment=alignment(flags), base_rva=base, native_section=native['name'],
                compared_bytes=compared, nonzero_bytes=nonzero, relocations=resolved,
                contribution_sha256=hashlib.sha256(native_bytes).hexdigest(),
                symbols=symbols, aliases=sorted(aliases))


def place_object(coff, pe, bound=None):
    """Place every initialized data section; relocation-free sections first."""
    candidates = [n for n, s in enumerate(coff.sections, 1)
                  if s['name'] in ('.data', '.rdata') and not s['flags'] & (CODE | EXECUTE)]
    order = sorted(candidates, key=lambda n: (bool(coff.sections[n - 1]['relocations']), n))
    placed, results = {}, {}
    for number in order:
        try:
            result = place(coff, number, pe, bound, placed)
        except PlacementError as exc:
            results[number] = dict(profile=PROFILE, section=number, name=coff.sections[number - 1]['name'],
                                   size=coff.sections[number - 1]['size'], rejected=str(exc),
                                   symbols=sorted(s['name'] for s in coff.symbols.values()
                                                  if s['section'] == number and not s['aux']))
            continue
        placed[number] = result['base_rva']
        results[number] = result
    return [results[n] for n in sorted(results)]


def main(argv=None):
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'scripts'))
    from coff import COFF
    from pe import PE
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--flags', nargs='*', default=['/O2'])
    parser.add_argument('--target', required=True, help='repository-relative original image path')
    parser.add_argument('--original-root', type=Path)
    parser.add_argument('--bound', type=Path, help='JSON {symbol: {rva, evidence}} of independent targets')
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args(argv)
    from match import compile_source, digest, verify_toolchain, compile_inputs, DEFAULT_TOOLCHAIN
    source = (root / args.source).resolve()
    target = ((args.original_root or root) / args.target).resolve()
    bound = {}
    if args.bound:
        for name, record in json.loads(args.bound.read_bytes()).items():
            if type(record.get('rva')) is not int or not record.get('evidence'):
                raise ValueError('Independent target needs rva and evidence: ' + name)
            bound[name] = record['rva']
    context = compile_inputs(source, tuple(args.flags), DEFAULT_TOOLCHAIN)
    obj, command = compile_source(source, list(args.flags))
    if compile_inputs(source, tuple(args.flags), DEFAULT_TOOLCHAIN) != context:
        raise ValueError('Compile inputs changed during placement')
    results = place_object(COFF(obj), PE(target), bound)
    receipt = dict(schema=1, profile=PROFILE, source=args.source.as_posix(), flags=args.flags,
                   source_sha256=digest(source.read_bytes()), command=command[1:-2],
                   toolchain=verify_toolchain(DEFAULT_TOOLCHAIN), object_sha256=digest(obj.read_bytes()),
                   compile_inputs={str(p): h for p, h in context.items()},
                   target=args.target, target_sha256=digest(target.read_bytes()),
                   independent_targets={k: json.loads(args.bound.read_bytes())[k] for k in bound} if bound else {},
                   min_nonzero=MIN_NONZERO, contributions=results)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=1) + '\n', encoding='utf-8')
    for item in results:
        print(item['name'], item['section'], item['size'],
              item.get('rejected') or 'placed at %#x, %d symbols' % (item['base_rva'], len(item['symbols'])))


if __name__ == '__main__':
    main()
