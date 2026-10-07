"""Natural VC5 link of reconstructed caller/callee, diagnostic only.

No /BASE, /ORDER, /ALIGN, /MERGE, /SECTION, padding objects, stubs, or patched
relocations. Inspect the linked image on disk; never load it or change recovery.
"""
import json
import os
import subprocess
from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,compare,digest,verify_toolchain
from coff import COFF
from pe import PE
from macro_compare import decode
from refinery_forms import function_text


def main():
    tc=DEFAULT_TOOLCHAIN;identity=verify_toolchain(tc)
    root=ROOT/'work/linkage_probe';root.mkdir(parents=True,exist_ok=True)
    baseline=ROOT/'calibration/macro_actor.c'
    original=baseline.read_text()
    names=['unlink_12c','hide_and_unlink']
    text='/* Minimal natural link probe, reconstructed C only. */\n'
    text+=original[original.index('typedef struct Actor'):original.index('typedef struct Asset8')]
    text+='\n'.join(function_text(original,name) for name in names)+'\n'
    source=root/'caller_callee.c';source.write_bytes(text.encode())
    obj,compile_command=compile_source(source,['/O2'])
    binary=root/'caller_callee.dll';mapfile=root/'caller_callee.map'
    env=os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
    env.update(PATH=str(tc/'bin')+os.pathsep+env.get('PATH',''),LIB=str(tc/'lib'))
    command=[str(tc/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF',
             '/OUT:'+str(binary),'/MAP:'+str(mapfile),*(('/EXPORT:'+n) for n in names),str(obj)]
    result=subprocess.run(command,cwd=root,env=env,capture_output=True,text=True,timeout=60)
    (root/'link.log').write_text(result.stdout+result.stderr)
    if result.returncode:raise ValueError(result.stdout+result.stderr)
    pe=PE(binary);coff=COFF(obj)
    exports={e.name.decode():e.address for e in pe.pe.DIRECTORY_ENTRY_EXPORT.symbols}
    spec=json.loads((ROOT/'evidence/macro_actor.json').read_text());target=PE(ROOT/spec['target'])
    if digest(target.data)!=spec['target_sha256']:raise ValueError('Original changed')
    rows=[]
    for name in names:
        f=next(f for f in spec['functions'] if f['symbol']=='_'+name)
        contribution=coff.function('_'+name);rva=exports[name];size=contribution['size']
        data=pe.read_rva(rva,size);expected=target.read_rva(int(f['rva'],0),f['size'])
        if digest(expected)!=f['target_sha256']:raise ValueError('Target span changed')
        differences=[n for n in range(max(len(data),len(expected))) if data[n:n+1]!=expected[n:n+1]]
        calls=[i.operands[0].imm-pe.image_base for i in decode(data,pe.image_base+rva) if i.mnemonic=='call']
        if name=='hide_and_unlink' and calls!=[exports['unlink_12c']]:raise ValueError('Natural call did not resolve to reconstructed callee')
        rows.append(dict(symbol='_'+name,linked_rva=hex(rva),original_rva=f['rva'],
            linked_size=size,target_size=f['size'],linked_sha256=digest(data),target_sha256=digest(expected),
            raw_linked_equal=data==expected,raw_differing_offsets=differences,
            call_destinations_rva=[hex(c) for c in calls],object_relocations=contribution['relocations'],
            strict_object_result=compare(obj,'_'+name,ROOT/spec['target'],int(f['rva'],0),f['size'])))
    report=dict(scope='Exploratory natural linked-image byte comparison only; does not grant FUNCTION_MATCH.',
        toolchain=identity,source_sha256=digest(source.read_bytes()),baseline_source_sha256=digest(baseline.read_bytes()),
        target_sha256=spec['target_sha256'],compile_command=compile_command,link_command=command,
        linked_sha256=digest(binary.read_bytes()),map_sha256=digest(mapfile.read_bytes()),
        linked_base_relocations=[e.rva for block in getattr(pe.pe,'DIRECTORY_ENTRY_BASERELOC',[]) for e in block.entries if e.type],
        natural_caller_minus_callee=exports['hide_and_unlink']-exports['unlink_12c'],original_caller_minus_callee=0x200,
        functions=rows)
    (root/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for r in rows:print(r['symbol'],r['raw_linked_equal'],r['raw_differing_offsets'])
    print('Natural/original caller-callee distance:',report['natural_caller_minus_callee'],report['original_caller_minus_callee'])
    return report


if __name__=='__main__':main()
