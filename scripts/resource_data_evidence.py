"""Fresh historical data-source audit; never grants function or layout acceptance."""
import argparse
import json
import struct

from env import ROOT
from coff import COFF
from match import compile_source, DEFAULT_TOOLCHAIN, digest, verify_toolchain
from pe import PE


def audit():
    source = ROOT / 'calibration/resource_data.c'
    evidence = json.loads((ROOT / 'evidence/resource_writer_layout.json').read_text())
    image_path = ROOT / evidence['target']
    if digest(image_path.read_bytes()) != evidence['target_sha256']:
        raise ValueError('Changed resource oracle')
    image = PE(image_path)
    obj, command = compile_source(source, ['/O2'])
    coff = COFF(obj)

    def symbol(name):
        found = [s for s in coff.symbols.values() if s['name'] == name]
        if len(found) != 1:
            raise ValueError('Ambiguous data symbol: ' + name)
        return found[0]

    def virtual_bytes(section, start, count):
        if start < 0 or start + count > section['size']:
            raise ValueError('String outside COFF contribution')
        data = section['data'][start:start+count]
        if len(data) < count:
            if not section['flags'] & 0x80:
                raise ValueError('Missing initialized bytes')
            data += bytes(count-len(data))
        return data

    def original_byte(rva):
        section = next(s for s in image.sections if s['virtual_address'] <= rva < s['virtual_address']+s['virtual_size'])
        if rva-section['virtual_address'] >= section['raw_size']:
            return b'\0'
        return image.read_rva(rva, 1)

    def target_string(rva):
        value = bytearray()
        for offset in range(1024):
            char = original_byte(rva+offset)
            value += char
            if char == b'\0':
                return bytes(value)
        raise ValueError('Unterminated target string')

    records = symbol('_g_actor_resources')
    section = coff.sections[records['section']-1]
    size = 81*20
    data = section['data'][records['value']:records['value']+size]
    if len(data) != size:
        raise ValueError('Missing resource records')
    expected = image.read_rva(0x5cee0, size)
    relocs = [r for r in section['relocations'] if records['value'] <= r['offset'] < records['value']+size]
    if len(relocs) != 81:
        raise ValueError('Expected exactly 81 local path references')
    strings = []
    for index in range(81):
        offset = index*20
        if data[offset+4:offset+20] != expected[offset+4:offset+20]:
            raise ValueError(f'Record fields differ: {index}')
        relocation = next(r for r in relocs if r['offset'] == records['value']+offset)
        if relocation['type'] != 6:
            raise ValueError('Unexpected path relocation')
        referent = symbol(relocation['symbol'])
        if referent['section'] <= 0:
            raise ValueError('Path is not local data')
        literal_section = coff.sections[referent['section']-1]
        addend = struct.unpack_from('<I', data, offset)[0]
        original_rva = struct.unpack_from('<I', expected, offset)[0]-image.image_base
        original = target_string(original_rva)
        literal = virtual_bytes(literal_section, referent['value']+addend, len(original))
        if literal != original:
            raise ValueError(f'Path bytes differ: {index}')
        strings.append(dict(index=index, target_rva=hex(original_rva), bytes=len(original),
                            sha256=digest(literal), literal_symbol=referent['name']))
    mapping = symbol('_g_actor_link_map')
    map_section = coff.sections[mapping['section']-1]
    map_bytes = map_section['data'][mapping['value']:mapping['value']+16]
    if map_bytes != image.read_rva(0x5e128, 16):
        raise ValueError('Link map differs')
    if any(mapping['value'] <= r['offset'] < mapping['value']+16 for r in map_section['relocations']):
        raise ValueError('Unexpected map relocations')
    zero_views = []
    for name, length in (('_g_actor_token_references',128),('_g_actor_link_rows',3072)):
        row = symbol(name)
        if row['section'] != 0 or row['storage'] != 2 or row['value'] != length:
            raise ValueError('Changed zero-storage view: ' + name)
        zero_views.append(dict(symbol=name, bytes=length, storage='COFF common; initial zero'))
    result = dict(scope='Data-only initial-content/reference audit; no FUNCTION_MATCH, original declarations, capacity beyond reviewed leading extents, runtime alias or natural-layout proof.',
                  source='calibration/resource_data.c', source_sha256=digest(source.read_bytes()),
                  oracle_sha256=digest(image.data), toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
                  command=command, object=str(obj.relative_to(ROOT)), object_sha256=digest(obj.read_bytes()),
                  rows=81, row_bytes=20, nonrelocated_fields_exact=True, local_strings_equal=81,
                  nonrelocated_fields_sha256=digest(b''.join(data[i*20+4:i*20+20] for i in range(81))),
                  map_bytes=16, map_exact=True, map_sha256=digest(map_bytes), zero_views=zero_views,
                  row_zero=strings[0], strings=strings)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', default='work/resource_data_integrated.json')
    args = parser.parse_args()
    destination = (ROOT / args.report).resolve()
    if not any(destination.is_relative_to(ROOT / folder) for folder in ('work','build','candidates')):
        raise ValueError('Generated audit belongs in ignored scratch')
    result = audit()
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ('rows','nonrelocated_fields_exact','local_strings_equal','map_exact','zero_views')}))
