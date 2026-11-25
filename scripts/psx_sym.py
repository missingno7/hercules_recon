"""Recover PsyQ .SYM records left in unused file space of a PSX disc (secondary evidence).

The USA v1.0 disc's OV/OV10 file carries developer debug records after its overlay
payload. This reader prints their structure, enum and typedef definitions. It copies
no game code, never modifies a disc and grants no matching acceptance.

    python scripts/psx_sym.py DISC.bin --file OV/OV10 --json work/psx_symbols/types.json --header work/psx_symbols/types.h
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

CLASSES = {8: 'member', 10: 'struct', 11: 'member', 12: 'union', 13: 'typedef', 15: 'enum', 16: 'enumerator',
           18: 'bitfield', 102: 'end'}
BASE = ['null', 'void', 'char', 'short', 'int', 'long', 'float', 'double', 'struct', 'union', 'enum', 'moe',
        'unsigned char', 'unsigned short', 'unsigned int', 'unsigned long']


def sector(stream, number):
    """2048 user bytes of one MODE2/2352 sector."""
    stream.seek(number * 2352 + 24)
    data = stream.read(2048)
    if len(data) != 2048:
        raise ValueError('Truncated disc image')
    return data


def files(stream):
    root = sector(stream, 16)[156:190]
    out = {}
    stack = [('', struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0])]
    while stack:
        path, lba, size = stack.pop()
        data = b''.join(sector(stream, lba + i) for i in range((size + 2047) // 2048))
        at = 0
        while at < len(data):
            length = data[at]
            if not length:
                at = (at // 2048 + 1) * 2048
                continue
            start, extent = struct.unpack_from('<I', data, at + 2)[0], struct.unpack_from('<I', data, at + 10)[0]
            name = data[at + 33:at + 33 + data[at + 32]]
            if name not in (b'\0', b'\1'):
                name = path + name.decode('ascii').split(';')[0].rstrip('.')
                if data[at + 25] & 2:
                    stack.append((name + '/', start, extent))
                else:
                    out[name] = (start, extent)
            at += length
    return out


def type_text(code, dims=(), tag=''):
    text, dims = BASE[code & 15] + (' ' + tag if tag else ''), list(dims)
    suffix, code = '', code >> 4
    while code:
        kind = code & 3
        if kind == 1:
            suffix += ' *'
        elif kind == 2:
            suffix += ' ()'
        elif kind == 3:
            suffix += ' [%s]' % (dims.pop(0) if dims else '')
        code >>= 2
    return text + suffix


def record(data, at):
    """(next offset, record) for one 0x94/0x96 definition record, else None."""
    if at + 13 > len(data):
        return None
    value, kind = struct.unpack_from('<IB', data, at)
    if kind not in (0x94, 0x96):
        return None
    storage, code, size = struct.unpack_from('<HHI', data, at + 5)
    if storage not in CLASSES:
        return None

    def text(pos):
        if pos >= len(data):
            return None
        raw = data[pos + 1:pos + 1 + data[pos]]
        if len(raw) != data[pos] or any(c < 0x20 or c > 0x7e for c in raw):
            return None
        return pos + 1 + len(raw), raw.decode('ascii')

    pos, dims, tag = at + 13, (), ''
    if kind == 0x96:
        if pos + 2 > len(data):
            return None
        count = struct.unpack_from('<H', data, pos)[0]
        if count > 8 or pos + 2 + 4 * count > len(data):
            return None
        dims = struct.unpack_from('<%dI' % count, data, pos + 2)
        step = text(pos + 2 + 4 * count)
        if not step:
            return None
        pos, tag = step
    step = text(pos)
    if not step:
        return None
    return step[0], dict(offset=at, value=value, role=CLASSES[storage], type_code=code,
                         type=type_text(code, dims, tag), size=size, tag=tag, name=step[1])


def runs(data, minimum=8):
    """Maximal consecutive record runs with at least ``minimum`` records."""
    out, at = [], 0
    while at < len(data) - 13:
        if data[at + 4] not in (0x94, 0x96):
            at += 1
            continue
        pos, records = at, []
        step = record(data, pos)
        while step:
            pos = step[0]
            records.append(step[1])
            step = record(data, pos)
        if len(records) >= minimum:
            out.append(dict(start=at, end=pos, records=records))
            at = pos
        else:
            at += 1
    return out


def aggregates(records):
    """Group members between a struct/union/enum tag record and its .eos record."""
    out, aliases = [], {}
    current = dict(tag=None, role='struct', size=None, members=[], complete=False)
    for r in records:
        if r['role'] in ('struct', 'union', 'enum'):
            current = dict(tag=r['name'], role=r['role'], size=r['size'], members=[], complete=True)
        elif r['role'] in ('member', 'enumerator', 'bitfield'):
            if current is None:
                raise ValueError('Member outside an aggregate at %#x' % r['offset'])
            current['members'].append({k: r[k] for k in ('value', 'type', 'size', 'name', 'role', 'tag')})
        elif r['role'] == 'end':
            if current is None or (current['tag'] is not None and current['tag'] != r['tag']):
                raise ValueError('Unmatched .eos at %#x' % r['offset'])
            if current['size'] is not None and current['size'] != r['value']:
                raise ValueError('Aggregate size disagrees at %#x' % r['offset'])
            current.update(tag=r['tag'], size=r['value'])
            out.append(current)
            current = None
        elif r['role'] == 'typedef':
            aliases[r['name']] = r['tag'] or r['type']
    if current and current['members']:
        current['complete'] = False
        out.append(current)
    return out, aliases


def header(groups, aliases):
    names = {v: k for k, v in aliases.items()}
    lines = ['/* Derived from PsyQ SYM records; secondary PSX evidence, not a PC layout proof. */']
    for g in groups:
        lines.append('%s %s { /* %s, %s bytes%s */' % (g['role'], g['tag'], names.get(g['tag'], ''), g['size'],
                                                      '' if g['complete'] else ', incomplete'))
        for m in g['members']:
            if g['role'] == 'enum':
                lines.append('    %s = %d,' % (m['name'], m['value']))
            else:
                width = ' : %d' % m['size'] if m['role'] == 'bitfield' else ''
                lines.append('    /* 0x%03x */ %s %s%s;' % (m['value'], m['type'], m['name'], width))
        lines.append('};')
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('disc', type=Path)
    parser.add_argument('--file', required=True)
    parser.add_argument('--json', type=Path)
    parser.add_argument('--header', type=Path)
    args = parser.parse_args()
    with args.disc.open('rb') as stream:
        start, size = files(stream)[args.file]
        data = b''.join(sector(stream, start + i) for i in range((size + 2047) // 2048))[:size]
    report = dict(file=args.file, lba=start, size=size, sha256=hashlib.sha256(data).hexdigest(), runs=[])
    text = ''
    for run in runs(data):
        groups, aliases = aggregates(run['records'])
        report['runs'].append(dict(start=run['start'], end=run['end'], records=len(run['records']),
                                   aggregates=groups, typedefs=aliases))
        text += header(groups, aliases)
        print('run %#x..%#x: %d records, %d aggregates, %d typedefs' % (
            run['start'], run['end'], len(run['records']), len(groups), len(aliases)))
    if args.json:
        args.json.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8', newline='\n')
    if args.header:
        args.header.write_text(text, encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
