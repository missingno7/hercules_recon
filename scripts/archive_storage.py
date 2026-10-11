"""Fresh archive/fatal/registry field diagnostic; never function or image acceptance."""
import argparse
import json
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'scripts'))
from env import ROOT
from coff import COFF
from match import compile_source,digest,DEFAULT_TOOLCHAIN,verify_toolchain
from pe import PE
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=root/'work/archive_storage/report.json')
args=parser.parse_args()
output=args.output.resolve()
if not output.is_relative_to(root/'work'):
 parser.error('Diagnostic reports belong in ignored work/')
evidence=root/'evidence/archive_storage_closure_2026_10_11.json'
evidence_before=evidence.read_bytes()
proof=json.loads(evidence_before)
target=root/'work/discs/pc_install/HERCULES.EXE'
pe=PE(target)
assert digest(pe.data)==proof['fresh_canonical']['target_sha256']
sources=['calibration/archive_storage.c','calibration/public_file_handles.c']
fatal_evidence=root/'evidence/fatal_storage_2026_10_11.json'
fatal_source=root/'calibration/fatal_storage.c'
fatal_before=None;fatal_proof=None
if fatal_evidence.exists() or fatal_source.exists():
 assert fatal_evidence.is_file() and fatal_source.is_file()
 fatal_before=fatal_evidence.read_bytes();fatal_proof=json.loads(fatal_before)
 assert fatal_proof['oracle']['sha256']==digest(pe.data)
 assert digest(fatal_source.read_bytes())==fatal_proof['canonical_provider']['sha256']
 sources.append(fatal_proof['canonical_provider']['source'])
registry_evidence=root/'evidence/registry_storage_adoption_2026_10_11.json'
registry_paths=['calibration/registry_storage.c','calibration/registry_literals.c']
registry_before=None;registry_proof=None
if registry_evidence.exists() or any((root/p).exists() for p in registry_paths):
 assert registry_evidence.is_file() and all((root/p).is_file() for p in registry_paths)
 registry_before=registry_evidence.read_bytes();registry_proof=json.loads(registry_before)
 assert registry_proof['oracle']['sha256']==digest(pe.data)
 assert {p['source'] for p in registry_proof['canonical_providers']}==set(registry_paths)
 for provider in registry_proof['canonical_providers']:
  assert digest((root/provider['source']).read_bytes())==provider['source_sha256']
 sources.extend(registry_paths)
before={p:digest((root/p).read_bytes()) for p in sources}
pin=verify_toolchain(DEFAULT_TOOLCHAIN)
reports={};commons={};bss_storage={}
for path in sources:
 obj,command=compile_source(root/path,['/O2'])
 c=COFF(obj)
 assert not any(s['type']&0x20 and s['section']>0 for s in c.symbols.values())
 assert not any(s['relocations'] for s in c.sections)
 reports[path]={'object':str(obj),'object_sha256':digest(Path(obj).read_bytes()),'source_sha256':before[path],'command':command,'sections':[{'name':s['name'],'size':s['size'],'flags':s['flags']} for s in c.sections]}
 symbols={s['name']:s for s in c.symbols.values() if s['storage']==2}
 definitions={name for name,s in symbols.items() if s['section']>0 or (s['section']==0 and s['value']>0)}
 if path.endswith('archive_storage.c'):
  assert definitions=={'_'+name for name in proof['string_evidence']}|{'_g_archive_last_stream_handle','_g_archive_stream','_g_archive_directory_count','_g_public_file_count'}
  measured=[]
  for name,span in proof['string_evidence'].items():
   s=symbols['_'+name];sec=c.sections[s['section']-1]
   data=sec['data'][s['value']:s['value']+span['bytes']]
   native=pe.read_rva(int(span['rva'],0),span['bytes'])
   assert data==native and digest(data)==span['sha256']
   assert sec['name']=='.data' and sec['flags']&0x80000000
   measured.append({'name':name,'rva':span['rva'],'bytes':len(data),'sha256':digest(data),'section':sec['name']})
  last=symbols['_g_archive_last_stream_handle'];sec=c.sections[last['section']-1]
  data=sec['data'][last['value']:last['value']+4]
  assert data==pe.read_rva(0x365a4,4)==bytes.fromhex('ffffffff')
  for name in ('_g_archive_stream','_g_archive_directory_count','_g_public_file_count'):
   s=symbols[name];assert s['section']==0 and s['value']==4
   commons[name]=4
  reports[path]['strings']=measured
  reports[path]['last_handle_bytes_equal']=True
 elif path.endswith('public_file_handles.c'):
  assert definitions=={'_g_public_file_handles'}
  s=symbols['_g_public_file_handles'];assert s['section']==0 and s['value']==64
  commons['_g_public_file_handles']=64
 elif path.endswith('registry_storage.c'):
  assert definitions=={'_g_registry_config'}
  s=symbols['_g_registry_config'];sec=c.sections[s['section']-1]
  assert s['value']==0 and sec['name']=='.bss' and sec['size']==72 and not sec['data'] and sec['flags']==0xc0400080
  assert not any(section['size'] for section in c.sections if section['name'] not in ('.drectve','.bss'))
  rva=int(registry_proof['config']['rva'],0);width=registry_proof['config']['bytes'];assert width==72
  native=next(section for section in pe.sections if section['virtual_address']<=rva<section['virtual_address']+section['virtual_size'])
  assert native['name']=='.data' and native['characteristics']&0x80000000
  assert rva>=native['virtual_address']+native['raw_size'] and rva+width<=native['virtual_address']+native['virtual_size']
  bss_storage['_g_registry_config']=width
  reports[path]['protocol_prefix_bytes']=width
 elif path.endswith('registry_literals.c'):
  assert definitions=={'_'+name for name in registry_proof['strings']}
  measured=[]
  for name,span in registry_proof['strings'].items():
   s=symbols['_'+name];sec=c.sections[s['section']-1]
   data=sec['data'][s['value']:s['value']+span['bytes']]
   assert data==pe.read_rva(int(span['rva'],0),span['bytes']) and digest(data)==span['sha256']
   assert sec['name']=='.data' and sec['flags']&0x80000000
   measured.append({'name':name,'rva':span['rva'],'bytes':len(data),'sha256':digest(data),'section':sec['name']})
  reports[path]['strings']=measured
 else:
  cells=fatal_proof['canonical_provider']['cells']
  assert definitions=={'_'+cell['name'] for cell in cells}
  assert not any(s['size'] for s in c.sections if s['name']!='.drectve')
  for cell in cells:
   name='_'+cell['name'];s=symbols[name]
   assert s['section']==0 and s['value']==cell['bytes']
   commons[name]=cell['bytes']
   rva=int(cell['rva'],0)
   section=next(sec for sec in pe.sections if sec['virtual_address']<=rva<sec['virtual_address']+sec['virtual_size'])
   assert section['name']=='.data' and section['characteristics']&0x80000000
   assert rva>=section['virtual_address']+section['raw_size'] and rva+cell['bytes']<=section['virtual_address']+section['virtual_size']
  reports[path]['access_width_cells']=cells
for scalar in proof['scalar_evidence']:
 if scalar['name']=='g_archive_last_stream_handle':continue
 rva=int(scalar['rva'],0)
 section=next(s for s in pe.sections if s['virtual_address']<=rva< s['virtual_address']+s['virtual_size'])
 assert rva>=section['virtual_address']+section['raw_size'] and rva+4<=section['virtual_address']+section['virtual_size']
rva=0x56e640
section=next(s for s in pe.sections if s['virtual_address']<=rva<s['virtual_address']+s['virtual_size'])
assert rva>=section['virtual_address']+section['raw_size'] and rva+64<=section['virtual_address']+section['virtual_size']
assert before=={p:digest((root/p).read_bytes()) for p in sources}
assert digest(target.read_bytes())==proof['fresh_canonical']['target_sha256']
assert evidence.read_bytes()==evidence_before
if fatal_before is not None:
 assert fatal_evidence.read_bytes()==fatal_before
if registry_before is not None:
 assert registry_evidence.read_bytes()==registry_before
result={'scope':'Fresh canonical data/object equality and source/loader-zero proofs; not native placement or whole-module acceptance.','evidence_sha256':digest(evidence_before),'toolchain':pin,'target_sha256':digest(pe.data),'providers':reports,'common_storage_bytes':commons,'bss_storage_bytes':bss_storage,'initialized_bytes':sum(s['bytes'] for provider in reports.values() for s in provider.get('strings',[]))+4,'zero_storage_bytes':sum(commons.values())+sum(bss_storage.values()),'functions':0,'relocations':0}
if fatal_before is not None:
 result['fatal_evidence_sha256']=digest(fatal_before)
 result['fatal_storage_limit']='Only observed4+1+1byte zero cells; original declaration/aggregate extent, TU and native placement remain unproved.'
if registry_before is not None:
 result['registry_evidence_sha256']=digest(registry_before)
 result['registry_storage_limit']='Only the72-byte protocol prefix and49 observed initialized string bytes; full record extent, original type/TU and literal spacing/native placement remain unproved.'
output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ('initialized_bytes','zero_storage_bytes','functions','relocations')}))
