"""One static library-identity family; never compile, patch or run originals."""
from pathlib import Path
import sys,json,struct
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/'scripts'))
from env import ROOT
from match import DEFAULT_TOOLCHAIN,digest,verify_toolchain
from coff import COFF
from pe import PE
from macro_compare import decode
out=r/'work/archive_crt_dependencies';out.mkdir(exist_ok=True)
prediction=dict(prediction='The thirteen direct unresolved function dependencies of the already identified archive stdio members are real pinned LIBCMT contributions. At original call destinations, complete independently delimited sections should equal all nonrelocation bytes, with actual encoded symbol destinations consistent across occurrences and known parents.',falsifier='Any missing/ambiguous COMDAT, nonzero symbol offset, nonrelocation byte difference, invalid original instruction operand, inconsistent external symbol destination or out-of-image reference refutes identity. Do not try alternate libraries/flags or rewrite CRT code.',scope='Library identification only; no function acceptance, source identity, complete CRT selection or final link layout proof.')
(out/'prediction.json').write_bytes((json.dumps(prediction,indent=2)+'\n').encode())
verify_toolchain(DEFAULT_TOOLCHAIN)
oracle=r/'work/discs/pc_install/HERCULES.EXE';pe=PE(oracle)
assert digest(pe.data)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
imports={item.address:item.name.decode('ascii') for descriptor in getattr(pe.pe,'DIRECTORY_ENTRY_IMPORT',[]) for item in descriptor.imports if item.name}
lib=DEFAULT_TOOLCHAIN/'lib/libcmt.lib';data=lib.read_bytes();assert digest(data)=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
contract=json.loads((r/'evidence/resource_host_bindings.json').read_text())['library']
wanted=set(contract['remaining_library_dependencies']);mapping={};origins={}
parents=contract['sections']
def bind(name,va,origin):
 if name in mapping and mapping[name]!=va:raise ValueError(('Inconsistent symbol target',name,hex(mapping[name]),hex(va),origin))
 mapping[name]=va;origins.setdefault(name,[]).append(origin)
for p in parents:
 bind(p['name'],pe.image_base+int(p['entry_rva'],0),p['name'])
 for rel in p['relocations']:bind(rel['symbol'],pe.image_base+int(rel['actual_target_rva'],0),p['name']+':'+rel['offset'])
cursor=8;longnames=b'';found={};scratch=out/'member_probe.obj'
while cursor<len(data):
 h=data[cursor:cursor+60];assert h[58:60]==b'`\n';size=int(h[48:58]);body=data[cursor+60:cursor+60+size];name=h[:16].decode().strip()
 if name=='//':longnames=body
 elif name.startswith('/') and name[1:].isdigit():
  off=int(name[1:]);ends=[x for x in (longnames.find(b'\n',off),longnames.find(b'\0',off)) if x>=0];name=longnames[off:min(ends)].decode().rstrip('/\0')
 if len(body)>=2 and body[:2]==b'L\x01':
  scratch.write_bytes(body)
  try:co=COFF(scratch)
  except ValueError:co=None
  if co:
   for sym in co.symbols.values():
    if sym['name'] in wanted and sym['section']>0 and sym['type']&0x20:
     assert sym['name'] not in found,sym['name']
     path=out/(Path(name.replace('\\','/')).name);path.write_bytes(body)
     found[sym['name']]=(path,name,digest(body))
 cursor+=60+size+(size&1)
assert set(found)==wanted, wanted-set(found)
rows=[]
for symbol in sorted(wanted):
 path,name,memberhash=found[symbol];co=COFF(path);sec=co.function(symbol);base=mapping[symbol];target=pe.read_rva(base-pe.image_base,sec['size'])
 code_end=348 if symbol=='__openfile' else sec['size']
 instructions=decode(target[:code_end],base)
 if symbol=='__openfile':
  # Actual dispatch at+0x6a loads the 74-byte index map, then+0x70
  # jumps through ten section-local DWORD destinations. Ret/NOP ends at+348.
  assert target[0x6a:0x70]==b'\x8a\x99'+struct.pack('<I',base+388)
  assert target[0x70:0x77]==b'\xff\x24\x9d'+struct.pack('<I',base+348)
  assert max(target[388:462])==9 and len(target[388:462])==74
  assert target[462:]==b'\x90\x90'
 rels=[];operands=set()
 for rel in sec['relocations']:
  off=rel['offset'];typ=rel['type'];addend=struct.unpack_from('<I',sec['data'],off)[0]
  instruction=next((i for i in instructions if i.address<=base+off and base+off+4<=i.address+i.size),None)
  table_entry=symbol=='__openfile' and 348<=off<388 and (off-348)%4==0
  assert instruction is not None or table_entry,(symbol,rel)
  if typ==20:
   assert instruction.mnemonic in ('call','jmp') and instruction.imm_offset==off-(instruction.address-base) and instruction.imm_size==4,(symbol,rel,instruction.mnemonic)
   actual=(base+off+4+struct.unpack_from('<i',target,off)[0])&0xffffffff
  elif typ==6:
   assert table_entry or (instruction.disp_size==4 and instruction.disp_offset==off-(instruction.address-base)) or (instruction.imm_size==4 and instruction.imm_offset==off-(instruction.address-base)),(symbol,rel)
   actual=struct.unpack_from('<I',target,off)[0]
  else:raise ValueError(('Unknown relocation',rel))
  symmatches=[s for s in co.symbols.values() if s['name']==rel['symbol']]
  external=next((s for s in symmatches if s['storage']==2 and s['section']==0),None)
  inferred=(actual-addend)&0xffffffff
  if external:
   assert any(pe.image_base+s['virtual_address']<=inferred<pe.image_base+s['virtual_address']+max(s['virtual_size'],s['raw_size']) for s in pe.sections),(symbol,rel,hex(inferred))
   bind(rel['symbol'],inferred,symbol+':'+hex(off))
   if rel['symbol'].startswith('__imp__'):
    expected_import=rel['symbol'][7:].split('@')[0]
    assert imports.get(inferred)==expected_import,(symbol,rel,imports.get(inferred),expected_import)
  else:
   local=next((s for s in symmatches if s['section']>0 and co.sections[s['section']-1] is sec),None)
   if local:
    assert actual==(base+local['value']+addend)&0xffffffff,(symbol,rel)
    if table_entry:assert actual in {i.address for i in instructions},(symbol,rel)
  operands.update(range(off,off+4));rels.append(dict(offset=hex(off),symbol=rel['symbol'],type=typ,addend=hex(addend),actual_destination_rva=hex(actual-pe.image_base),inferred_external_base_rva=hex(inferred-pe.image_base) if external else None))
 diffs=[i for i,(a,b) in enumerate(zip(sec['data'],target)) if i not in operands and a!=b]
 rows.append(dict(symbol=symbol,member=name,member_sha256=memberhash,rva=hex(base-pe.image_base),size=sec['size'],target_sha256=digest(target),contribution_sha256=digest(sec['data']),nonrelocation_differences=len(diffs),first_differences=diffs[:12],relocations=rels))
 print(symbol,hex(base-pe.image_base),sec['size'],'diff',len(diffs),'relocs',len(rels))
report=dict(**prediction,oracle=str(oracle.relative_to(r)),oracle_sha256=digest(pe.data),toolchain='msvc5_rtm',toolchain_manifest_sha256=digest((r/'toolchains/manifest.json').read_bytes()),library=str(lib),library_sha256=digest(data),parents='evidence/resource_host_bindings.json',rows=rows,functions=len(rows),bytes=sum(x['size'] for x in rows),nonrelocation_differences=sum(x['nonrelocation_differences'] for x in rows),next_external_functions=sorted({x['symbol'] for row in rows for x in row['relocations'] if x['type']==20 and x['inferred_external_base_rva'] and x['symbol'] not in wanted and x['symbol'] not in {p['name'] for p in parents}}),symbol_bases={k:hex(v-pe.image_base) for k,v in sorted(mapping.items())})
(out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
assert report['nonrelocation_differences']==0, 'Historical CRT identity did not reproduce'
print('Total',report['functions'],report['bytes'],'differences',report['nonrelocation_differences'])
