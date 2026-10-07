"""Reproduce the actor continuation experiment; strict proof remains match/promote.

Full contributions include inline switch tables. Resolved relocation operands
exist only in a diagnostic buffer, never in the object, executable or source.
"""
import argparse
import json
from pathlib import Path
import struct
import time

from env import ROOT
from coff import COFF
from pe import PE
from match import compile_source,compare,digest
from macro_compare import tokens_and_calls,structure
from difflib import SequenceMatcher
from refinery import semantics


def evaluate(source,output):
    spec=json.loads((ROOT/'evidence/actor_continuation_region.json').read_text())
    output.mkdir(parents=True,exist_ok=True)
    started=time.perf_counter();before=digest(source.read_bytes())
    obj,command=compile_source(source,spec['flags'])
    coff=COFF(obj);pe=PE(ROOT/spec['target']);rows=[]
    if digest(pe.data)!=spec['target_sha256']:raise ValueError('Changed oracle')
    for f in spec['functions']:
        rva=int(f['rva'],0);c=coff.function(f['symbol'])
        raw=compare(obj,f['symbol'],ROOT/spec['target'],rva,f['size'])
        if raw['target_sha256']!=f['target_sha256']:raise ValueError('Changed target extent')
        symbol=next(s for s in coff.symbols.values() if s['name']==f['symbol'] and s['section']>0)
        buf=bytearray(c['data']);original=pe.read_rva(rva,f['size'])
        for relocation in c['relocations']:
            name=relocation['symbol'];offset=relocation['offset']
            addend=struct.unpack_from('<I',buf,offset)[0]
            if name in spec['symbol_rvas']:dest=int(spec['symbol_rvas'][name],0)
            else:
                local=[s for s in coff.symbols.values() if s['name']==name and s['section']==symbol['section']]
                if len(local)!=1:raise ValueError('Unreviewed relocation: '+name)
                dest=rva+local[0]['value']
            if relocation['type']==20:value=dest-rva-offset-4+addend
            elif relocation['type']==6:value=pe.image_base+dest+addend
            else:raise ValueError('Unsupported diagnostic relocation')
            struct.pack_into('<I',buf,offset,value&0xffffffff)
        differences=sum(buf[i:i+1]!=original[i:i+1] for i in range(max(len(buf),len(original))))
        row=dict(symbol=f['symbol'],strict_equal=raw['exact'],target_size=f['size'],candidate_size=len(buf),
            raw_different_bytes=raw['different_bytes'],normalized_different_bytes=differences,
            relocation_count=len(c['relocations']),target_relocation_count=len(raw['target_relocations']),
            instruction_percent=None,ordered_cfg_equal=None,
            category='raw exact' if raw['exact'] else 'relocation-only diagnostic equality' if differences==0 else 'source/codegen residual')
        if not f.get('data_spans'):
            old,_,_=tokens_and_calls(original,pe.image_base+rva)
            new,_,_=tokens_and_calls(bytes(buf),pe.image_base+rva)
            lcs=sum(b.size for b in SequenceMatcher(None,old,new,autojunk=False).get_matching_blocks())
            row['instruction_percent']=round(100*lcs/max(len(old),len(new)),3)
            row['ordered_cfg_equal']=structure(original,pe.image_base+rva)==structure(bytes(buf),pe.image_base+rva)
        # No linear-disassembly/CFG claim over embedded data. Raw and diagnostic
        # byte comparisons above always retain every table and alignment byte.
        rows.append(row)
    if before!=digest(source.read_bytes()):raise ValueError('Source changed during compile')
    report=dict(source_sha256=before,object=str(obj),object_sha256=digest(obj.read_bytes()),command=command,
        target_sha256=spec['target_sha256'],flags=spec['flags'],functions=rows,
        elapsed_seconds=round(time.perf_counter()-started,3),
        summary=dict(functions=len(rows),exact=sum(r['strict_equal'] for r in rows),
            exact_bytes=sum(r['target_size'] for r in rows if r['strict_equal']),
            diagnostic_equal=sum(r['normalized_different_bytes']==0 for r in rows),full_bytes=sum(r['target_size'] for r in rows)))
    (output/'metrics.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(report['summary']))
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=ROOT/'calibration/actor_continuation.c')
    p.add_argument('--output',type=Path,default=ROOT/'work/actor_region_replay')
    p.add_argument('--semantics',action='store_true');a=p.parse_args()
    a.output=a.output.resolve()
    evaluate(a.source.resolve(),a.output)
    if a.semantics:
        result=semantics(a.source.resolve(),ROOT/'tests/actor_continuation_semantics.c',
            {'../calibration/actor_continuation.c':a.source.resolve()},a.output)
        (a.output/'semantics.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
        print(result['stdout'])
        if result['exit_code']:raise SystemExit(result['exit_code'])


if __name__=='__main__':main()
