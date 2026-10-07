"""Recheck reviewed static registry population spans; never load a game image."""
import json
import struct

from env import ROOT
from match import digest
from pe import PE


def verify():
    evidence = json.loads((ROOT / 'evidence/registry_population.json').read_text())
    images = {}
    for key, module in (('eng1', 'ENG1.DLL'), ('host', 'HERCULES.EXE')):
        oracle = evidence['oracle'][key]
        path = ROOT / oracle['path']
        if digest(path.read_bytes()) != oracle['sha256']:
            raise ValueError('Changed registry population oracle: ' + module)
        images[module] = PE(path)
    for span in evidence['code_spans']:
        start, end = int(span['start_rva'], 0), int(span['end_rva_exclusive'], 0)
        if digest(images[span['module']].read_rva(start, end - start)) != span['sha256']:
            raise ValueError('Changed reviewed code span: ' + span['start_rva'])
    binding = evidence['host_binding']
    host = images['HERCULES.EXE']
    table = int(binding['host_static_source_table_rva'], 0)
    slot = int(binding['slot_index'], 0)
    cell = int(binding['source_callback_cell_rva'], 0)
    if cell != table + 4 * slot:
        raise ValueError('Host callback cell disagrees with table slot')
    if digest(host.read_rva(table, 250 * 4)) != binding['source_table_sha256']:
        raise ValueError('Changed host default interface table')
    if struct.unpack('<I', host.read_rva(cell, 4))[0] != int(binding['source_callback_va'], 0):
        raise ValueError('Changed default host allocation callback')
    if int(binding['callback_cell_rva'], 0) != int(binding['eng1_interface_base_rva'], 0) + 4 * slot:
        raise ValueError('ENG1 callback cell disagrees with table slot')
    result = dict(scope='Static reviewed bytes and default binding only; no runtime or payload-capacity proof.',
                  images=len(images), code_spans=len(evidence['code_spans']), callback_slot=hex(slot),
                  population='Output-slot address flows through wrapper and host allocation callback.')
    print(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    verify()
