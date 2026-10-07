"""Experimental bounded source search; strict acceptance remains in match/promote.

Every variant receives a fresh object, a separately compiled semantic executable,
and a frozen-contribution check. Diagnostic normalization never modifies an object.
"""
import argparse
from contextlib import redirect_stdout
import io
import json
import os
from pathlib import Path
import re
import subprocess
import struct
import time

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from coff import COFF
from pe import PE
from macro_compare import evaluate
from refinery_forms import variants


def semantics(candidate, harness, replacements, output):
    source = harness.read_text()
    for old,new in replacements.items():
        source = source.replace(old,str(new).replace('\\','/'))
    # Preserve other harness includes when the harness moves to ignored scratch.
    source = re.sub(r'#include "([^"\n]+)"', lambda m:'#include "'+str((harness.parent/m[1]).resolve()).replace('\\','/')+'"',source)
    path = output/'semantics.c'
    path.write_text(source)
    tc = DEFAULT_TOOLCHAIN
    verify_toolchain(tc)
    env = os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'): env.pop(key,None)
    env.update(PATH=str(tc/'bin')+os.pathsep+env.get('PATH',''), INCLUDE=str(tc/'include'), LIB=str(tc/'lib'))
    exe = output/'semantics.exe'
    command = [str(tc/'bin/cl.exe'),'/nologo','/O2','/Gy','/ML','/Fo'+str(output/'semantics.obj'),'/Fe'+str(exe),str(path),'/link','/INCREMENTAL:NO']
    compiled = subprocess.run(command,cwd=output,env=env,capture_output=True,text=True,timeout=60)
    (output/'semantic_compile.log').write_text(compiled.stdout+compiled.stderr)
    if compiled.returncode: raise ValueError('Semantic compilation failed: '+str(output))
    run = subprocess.run([str(exe)],cwd=output,env=env,capture_output=True,text=True,timeout=30)
    result = dict(exit_code=run.returncode,stdout=run.stdout.strip(),command=command,
                  harness_sha256=digest(path.read_bytes()),executable_sha256=digest(exe.read_bytes()))
    if run.returncode: raise ValueError('Semantic test failed: '+str(result))
    return result


def contributions(obj):
    coff = COFF(Path(obj))
    return {s['name']:coff.function(s['name']) for s in coff.symbols.values()
            if s['section']>0 and s['type']&0x20}


def mismatch_details(contribution,spec,symbol):
    """Localization for diagnostics only; never produce a linkable artifact."""
    f=next(f for f in spec['functions'] if f['symbol']==symbol)
    rva=int(f['rva'],0);pe=PE(ROOT/spec['target'])
    expected=pe.read_rva(rva,f['size']);data=bytearray(contribution['data'])
    for relocation in contribution['relocations']:
        offset=relocation['offset'];destination=int(spec['symbol_rvas'][relocation['symbol']],0)
        addend=struct.unpack_from('<I',data,offset)[0]
        if relocation['type']==20:value=destination-rva-offset-4+addend
        elif relocation['type']==6:value=pe.image_base+destination+addend
        else:raise ValueError('Unsupported relocation')
        struct.pack_into('<I',data,offset,value&0xffffffff)
    positions=[i for i in range(max(len(data),len(expected)))
               if i>=len(data) or i>=len(expected) or data[i]!=expected[i]]
    spans=[]
    for i in positions:
        if spans and spans[-1][1]==i:spans[-1][1]=i+1
        else:spans.append([i,i+1])
    return dict(normalized_mismatch_spans=spans,normalized_mismatch_span_count=len(spans),
                relocations=contribution['relocations'])


def dominates(row, best, relocations_equal):
    if not relocations_equal: return False
    if row['strict_equal']: return True
    # No scalar score can hide a regression in these dimensions.
    dims = lambda r:(-r['normalized_different_bytes'],r['instruction_percent'],
        -abs(r['candidate_instruction_count']-r['original_instruction_count']),
        int(r['ordered_cfg_equal']),int(r['direct_call_targets_equal']))
    a,b = dims(row),dims(best)
    return all(x>=y for x,y in zip(a,b)) and any(x>y for x,y in zip(a,b))


def run(source, spec, symbol, harness, include, output, family=None):
    started = time.perf_counter()
    output.mkdir(parents=True,exist_ok=True)
    source_text = source.read_text()
    rows=[]; frozen=None; best=None; best_source=None
    generated = [('baseline',source_text),*(family if family is not None else variants(source_text,symbol.lstrip('_')))]
    for n,(label,text) in enumerate(generated):
        directory=output/f'{n:03d}-{label.replace(" ","_")}'
        directory.mkdir(exist_ok=True)
        candidate=directory/'candidate.c'; candidate.write_bytes(text.encode())
        with redirect_stdout(io.StringIO()): report=evaluate(candidate,spec,directory/'metrics.json')
        current=contributions(report['object'])
        row=next(r for r in report['functions'] if r['symbol']==symbol)
        details=mismatch_details(current[symbol],json.loads(spec.read_text()),symbol)
        if frozen is None:
            frozen=current
        for name,value in frozen.items():
            if name!=symbol and current[name]!=value:
                raise ValueError('Frozen contribution changed: '+name)
        semantic=semantics(candidate,harness,{include:candidate},directory)
        # Symbol/type multiplicity is stable; offsets may move with instructions.
        identities=lambda r:sorted((x['symbol'],x['type']) for x in r['relocations'])
        same_reloc=identities(current[symbol])==identities(frozen[symbol])
        improved=best is None or dominates(row,best,same_reloc)
        if improved: best=row;best_source=candidate
        entry=dict(index=n,label=label,source_sha256=report['source_sha256'],metrics=row,
                   relocation_identity_equal=same_reloc,retained=improved,semantics=semantic,
                   object_sha256=report['object_sha256'],localization=details)
        rows.append(entry)
        print(f'{symbol} {n:02} {label}: diff={row["normalized_different_bytes"]} ins={row["instruction_percent"]} CFG={row["ordered_cfg_equal"]} retained={improved}',flush=True)
        if row['strict_equal'] or row['normalized_different_bytes']==0: break
    result=dict(schema_version=1,source=str(source.relative_to(ROOT)),source_sha256=digest(source.read_bytes()),
                spec=str(spec.relative_to(ROOT)),symbol=symbol,toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
                generated=len(generated)-1,compiled_variants=len(rows)-1,semantic_builds=len(rows),
                elapsed_seconds=round(time.perf_counter()-started,3),best=best,
                best_source=str(best_source.relative_to(ROOT)),attempts=rows)
    (output/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('source','spec','harness','output'): p.add_argument('--'+arg,type=Path,required=True)
    p.add_argument('--symbol',required=True);p.add_argument('--include',required=True)
    a=p.parse_args()
    if not a.output.resolve().is_relative_to(ROOT/'work'):
        p.error('Refinery outputs must be under work/')
    run(a.source.resolve(),a.spec.resolve(),a.symbol,a.harness.resolve(),a.include,a.output.resolve())
