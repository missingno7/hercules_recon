"""Fresh full-span resource-destroy measurement; independent table-base evidence.

The usual local map audit is deliberately not relaxed. This routine computes
the table base, while the target embeds only member addresses. Data-row evidence
anchors that one additional address. No diagnostic bytes are saved as code.
"""
import json
import struct
from pathlib import Path
from coff import COFF
from env import ROOT
from match import compile_source, compare, digest
from pe import PE
from macro_compare import structure, tokens_and_calls
from difflib import SequenceMatcher


def evaluate(output):
    spec=json.loads((ROOT/'evidence/resource_destroy_region.json').read_text())
    evidence=json.loads((ROOT/'evidence/resource_data_source.json').read_text())
    source=ROOT/spec['source'];header=ROOT/'calibration/resource_record.h'
    before=digest(source.read_bytes());header_before=digest(header.read_bytes())
    pe=PE(ROOT/spec['target'])
    if digest(pe.data)!=spec['target_sha256']:raise ValueError('Changed oracle')
    # The physical record base is supported by the separate 81-row data audit.
    if spec['symbol_rvas']['_g_actor_resources']!='0x5cee0':raise ValueError('Unreviewed table base')
    if digest((ROOT/'calibration/resource_data.c').read_bytes())!=evidence['source_sha256']:raise ValueError('Changed independent data source')
    obj,command=compile_source(source,spec['flags'])
    f=spec['functions'][0];rva=int(f['rva'],0);coff=COFF(obj);c=coff.function(f['symbol'])
    original=pe.read_rva(rva,f['size']);strict=compare(obj,f['symbol'],ROOT/spec['target'],rva,f['size'])
    if digest(original)!=f['target_sha256']:raise ValueError('Changed target extent')
    diagnostic=bytearray(c['data'])
    for reloc in c['relocations']:
        offset=reloc['offset'];addend=struct.unpack_from('<I',diagnostic,offset)[0]
        dest=int(spec['symbol_rvas'][reloc['symbol']],0)
        if reloc['type']==6:
            if reloc['symbol']!='_g_actor_resources' or addend not in (0,12):raise ValueError('Unreviewed data reference')
            value=pe.image_base+dest+addend
        elif reloc['type']==20:
            if reloc['symbol']!='_release_resource_buffer' or diagnostic[offset-1]!=0xe8:raise ValueError('Unreviewed call')
            value=dest-rva-offset-4+addend
        else:raise ValueError('Unreviewed relocation')
        struct.pack_into('<I',diagnostic,offset,value&0xffffffff)
    old,_,_=tokens_and_calls(original,pe.image_base+rva);new,_,_=tokens_and_calls(bytes(diagnostic),pe.image_base+rva)
    lcs=sum(b.size for b in SequenceMatcher(None,old,new,autojunk=False).get_matching_blocks())
    diff=sum(diagnostic[i:i+1]!=original[i:i+1] for i in range(max(len(diagnostic),len(original))))
    row=dict(symbol=f['symbol'],target_size=f['size'],candidate_size=len(diagnostic),strict_equal=strict['exact'],raw_different_bytes=strict['different_bytes'],normalized_different_bytes=diff,instruction_percent=round(100*lcs/max(len(old),len(new)),3),ordered_cfg_equal=structure(original,pe.image_base+rva)==structure(bytes(diagnostic),pe.image_base+rva),category='source/codegen residual')
    if before!=digest(source.read_bytes()) or header_before!=digest(header.read_bytes()):raise ValueError('Source changed during compile')
    report=dict(scope='Full raw comparison plus diagnostic using independently evidenced table base; no acceptance.',source_sha256=before,header_sha256=header_before,target_sha256=spec['target_sha256'],flags=spec['flags'],object=str(obj),object_sha256=digest(obj.read_bytes()),command=command,functions=[row],summary=dict(functions=1,exact=int(strict['exact']),diagnostic_equal=int(diff==0)),map_validator_limit='Standard local symbol map requires the base itself as a decoded operand; this target contains only member addresses. Separate81-row data evidence anchors the base.',independent_data_evidence_sha256=digest((ROOT/'evidence/resource_data_source.json').read_bytes()))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(row));return report


if __name__=='__main__':evaluate(ROOT/'work/resource_destroy_integrated/metrics.json')
