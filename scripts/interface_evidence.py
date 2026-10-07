"""Recheck static engine-interface facts from immutable PC files; no image loading."""
import json
import struct
from env import ROOT
from pe import PE
from match import digest


def main():
    evidence=json.loads((ROOT/'evidence/engine_interface_binding.json').read_text())
    images={name:PE(ROOT/row['path']) for name,row in evidence['images'].items()}
    for name,pe in images.items():
        if digest(pe.data)!=evidence['images'][name]['sha256']:raise ValueError('Changed image: '+name)
    host=images['HERCULES.EXE'];table=evidence['source_table'];rva=int(table['rva'],0)
    raw=host.read_rva(rva,table['dword_count']*4)
    if digest(raw)!=table['raw_sha256']:raise ValueError('Changed source table')
    if struct.unpack_from('<I',raw)[0]!=int(evidence['export_copy']['signature'],0):raise ValueError('Wrong signature')
    pointer=struct.unpack('<I',host.read_rva(int(table['pointer_slot_rva'],0),4))[0]
    if pointer!=host.image_base+rva:raise ValueError('Wrong source-table pointer')
    rows=[]
    for slot,entry in table['entries'].items():
        value=struct.unpack_from('<I',raw,int(slot,0)*4)[0]
        if value!=int(entry['target_va'],0):raise ValueError('Changed callback slot')
        rows.append(dict(slot=slot,eng1_destination_rva=entry['eng1_destination_rva'],target_va=hex(value)))
    for name,rva_text in evidence['export_copy']['export_rvas'].items():
        exports={e.name.decode():e.address for e in images[name].pe.DIRECTORY_ENTRY_EXPORT.symbols}
        if exports[evidence['export_copy']['name']]!=int(rva_text,0):raise ValueError('Changed export')
    for span in evidence['code_spans']:
        raw=images[span['module']].read_rva(int(span['rva'],0),span['size'])
        if digest(raw)!=span['sha256']:raise ValueError('Changed reviewed code span')
    print(json.dumps(dict(scope='Static evidence recheck; no runtime mutation/ABI proof.',images=len(images),
                         copied_dwords=table['dword_count'],callback_slots=rows,
                         reviewed_code_spans=len(evidence['code_spans'])),indent=2))


if __name__=='__main__':main()
