"""Natural host pool link with real observed data; diagnostic only.

No placement flags, stubs, patched objects, original execution or recovery writes.
All derived source, objects and images remain in ignored work/build directories.
"""
import json, os, re, subprocess, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'scripts'))
from match import compile_source,DEFAULT_TOOLCHAIN,digest,verify_toolchain
from coff import COFF
from pe import PE
from refinery_forms import function_text
from macro_compare import evaluate
from region import contribution_identity

lane=root/'work/host_pool_natural'
lane.mkdir(exist_ok=True)
spec=json.loads((root/'evidence/host_pool_region.json').read_text())
counts=json.loads((root/'evidence/host_pool_counts_region.json').read_text())
target=PE(root/spec['target'])
assert digest(target.data)==spec['target_sha256']
baseline=(root/'calibration/host_pool_repaired.c').read_text()
prefix=baseline[:baseline.index('void __cdecl save_arena')]
source=lane/'pool_region.c'
source.write_text(baseline+'\n'+function_text((root/'calibration/host_pool_counts.c').read_text(),'count_used')+'\n',encoding='utf-8',newline='\n')
strings=[]
data=prefix+'\nHostPoolState g_host_pool;\n'
for name,rva in spec['symbol_rvas'].items():
    if not name.startswith('_g_fmt_'): continue
    start=int(rva,0)
    value=bytearray()
    for offset in range(256):
        char=target.read_rva(start+offset,1)
        if char==b'\0': break
        value.extend(char)
    else: raise ValueError('Unterminated format string')
    literal=value.decode('ascii')
    data+=f'const char {name[1:]}[] = {json.dumps(literal)};\n'
    strings.append(dict(symbol=name,target_rva=rva,bytes=len(value)+1,sha256=digest(bytes(value)+b'\0')))
(lane/'pool_data.c').write_text(data,encoding='utf-8',newline='\n')
prediction=dict(prediction='The recovered six pool functions, real empty diagnostic, inferred 48-byte zero state and four observed format strings should link with no unresolved symbols. Full contribution bytes and ordered relocations should preserve separate-source peers before linking. Link addresses are expected to differ from the original incomplete module; no acceptance is predicted.',falsifier='Unresolved external, altered prelink contribution/relocations, omitted or changed string bytes, nonzero initial state, any stub/placement flag or modified object.',scope='Natural diagnostic only; no recovery state or original execution.')
(lane/'prediction.json').write_text(json.dumps(prediction,indent=2)+'\n')
spec['functions']+=counts['functions']
spec['source']=str(source.relative_to(root))
(lane/'spec.json').write_text(json.dumps(spec,indent=2)+'\n')
report=evaluate(source,lane/'spec.json',lane/'metrics.json')
objects=[Path(report['object'])]
commands=[report['command']]
for path in (lane/'pool_data.c',root/'src/pc/host_diagnostic.c'):
    obj,command=compile_source(path,['/O2'])
    objects.append(obj);commands.append(command)
names=[f['symbol'][1:] for f in spec['functions']]+['host_diagnostic']
command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF','/OUT:'+str(lane/'pool.dll'),'/MAP:'+str(lane/'pool.map'),*('/EXPORT:'+name for name in names),*(str(p) for p in objects)]
environment=os.environ.copy()
for key in ('CL','_CL_','LINK','_LINK_'): environment.pop(key,None)
environment.update(PATH=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+environment.get('PATH',''),LIB=str(DEFAULT_TOOLCHAIN/'lib'))
linked=subprocess.run(command,cwd=lane,env=environment,capture_output=True,text=True,timeout=60)
(lane/'link.log').write_text(linked.stdout+linked.stderr)
if linked.returncode: raise ValueError(linked.stdout+linked.stderr)
image=PE(lane/'pool.dll')
exports={e.name.decode():e.address for e in image.pe.DIRECTORY_ENTRY_EXPORT.symbols}
maptext=(lane/'pool.map').read_text()
symbols={match[0]:int(match[1],16)-image.image_base for match in re.findall(r'\s+[0-9a-fA-F]+:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8})',maptext)}
data_rows=[]
for row in strings:
    rva=symbols[row['symbol']]
    assert digest(image.read_rva(rva,row['bytes']))==row['sha256']
    data_rows.append(dict(row,linked_rva=hex(rva),linked_bytes_equal=True))
state_rva=symbols['_g_host_pool']
state_section=next(s for s in image.sections if s['virtual_address']<=state_rva and state_rva+48<=s['virtual_address']+s['virtual_size'])
assert state_rva-state_section['virtual_address']>=state_section['raw_size']
original_state_section=next(s for s in target.sections if s['virtual_address']<=0x7a29c0 and 0x7a29c0+48<=s['virtual_address']+s['virtual_size'])
assert 0x7a29c0-original_state_section['virtual_address']>=original_state_section['raw_size']
peers=[]
for baseline_receipt in ('evidence/regions/pool_predicates.json','evidence/regions/host_pool_counts_first.json'):
    old=json.loads((root/baseline_receipt).read_text())
    for row in old['functions']:
        new_function=COFF(objects[0]).function(row['symbol'])
        assert row['output_id']==contribution_identity(new_function)
        peers.append(row['symbol'])
functions=[]
for row in spec['functions']:
    contribution=COFF(objects[0]).function(row['symbol'])
    actual=image.read_rva(exports[row['symbol'][1:]],contribution['size'])
    expected=target.read_rva(int(row['rva'],0),row['size'])
    functions.append(dict(symbol=row['symbol'],linked_rva=hex(exports[row['symbol'][1:]]),original_rva=row['rva'],linked_size=len(actual),target_size=len(expected),raw_linked_equal=actual==expected,raw_differences=sum(actual[n:n+1]!=expected[n:n+1] for n in range(max(len(actual),len(expected))))))
diagnostic=COFF(objects[-1]).function('_host_diagnostic')
assert diagnostic['data']==target.read_rva(0x63c0,16) and not diagnostic['relocations']
result=dict(scope='Natural link of reconstructed sources with real observed data; no stubs, original dispatch, placement flags or FUNCTION_MATCH. Inferred state grouping is not an original declaration proof.',toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),sources={str(p.relative_to(root)):digest(p.read_bytes()) for p in (source,lane/'pool_data.c',root/'src/pc/host_diagnostic.c')},compile_commands=commands,link_command=command,linked_sha256=digest(image.data),state=dict(linked_rva=hex(state_rva),bytes=48,initial_zero_fill=True,target_rva='0x7a29c0',target_storage='virtual zero-filled .data'),strings=data_rows,functions=functions,unchanged_prelink_peers=peers,diagnostic_raw_object_equal=True,scope_limit='Fresh compilation is compared with frozen diagnostic output identities only for peer preservation. These identities and this natural link grant no acceptance or compile-proof reuse.',builds=dict(source_compiles=3,natural_links=1))
(lane/'report.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(functions=len(names),strings=len(strings),initial_state_zero=True,unresolved_externals=0)))
