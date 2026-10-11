"""Fresh observed data-field diagnostic; never function or image acceptance."""
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
graphics_evidence=root/'evidence/graphics_storage_adoption_2026_10_11.json'
graphics_source=root/'calibration/graphics_storage.cpp'
graphics_before=None;graphics_proof=None
if graphics_evidence.exists() or graphics_source.exists():
 assert graphics_evidence.is_file() and graphics_source.is_file()
 graphics_before=graphics_evidence.read_bytes();graphics_proof=json.loads(graphics_before)
 assert graphics_proof['oracle']['sha256']==digest(pe.data)
 assert graphics_proof['canonical_provider']['source']=='calibration/graphics_storage.cpp'
 assert digest(graphics_source.read_bytes())==graphics_proof['canonical_provider']['source_sha256']
 for header in graphics_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 sources.append('calibration/graphics_storage.cpp')
audio_evidence=root/'evidence/audio_storage_adoption_2026_10_11.json'
audio_source=root/'calibration/audio_storage.cpp'
audio_before=None;audio_proof=None
if audio_evidence.exists() or audio_source.exists():
 assert audio_evidence.is_file() and audio_source.is_file()
 audio_before=audio_evidence.read_bytes();audio_proof=json.loads(audio_before)
 assert audio_proof['oracle']['sha256']==digest(pe.data)
 assert audio_proof['canonical_provider']['source']=='calibration/audio_storage.cpp'
 assert digest(audio_source.read_bytes())==audio_proof['canonical_provider']['source_sha256']
 for header in audio_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 sources.append('calibration/audio_storage.cpp')
extra_fields={}
for source_name,evidence_name,count,initialized,zero,bss_flags in (
 ('calibration/sound_bank_storage.cpp','evidence/sound_bank_storage_adoption_2026_10_11.json',4,0,19209,0xc0400080),
 ('calibration/mci_storage.c','evidence/mci_storage_adoption_2026_10_11.json',7,18,24,0xc0300080),
 ('calibration/archive_array_prefixes.c','evidence/archive_array_prefixes_adoption_2026_10_11.json',2,0,16384,0xc0400080),
 ('calibration/movie_scratch_storage.c','evidence/movie_storage_adoption_2026_10_11.json',1,0,81920,0xc0400080),
 ('calibration/host_resource_storage.c','evidence/host_resource_storage_adoption_2026_10_11.json',2,4,6964,0xc0400080),
 ('calibration/movie_header_storage.c','evidence/movie_header_storage_adoption_2026_10_11.json',1,0,840,0xc0400080),
 ('calibration/movie_initializer_storage.c','evidence/movie_initializer_storage_adoption_2026_10_11.json',4,0,82116,0xc0400080),
):
 source_path=root/source_name;proof_path=root/evidence_name
 if proof_path.exists() or source_path.exists():
  assert proof_path.is_file() and source_path.is_file()
  content=proof_path.read_bytes();field_proof=json.loads(content)
  assert field_proof['oracle']['sha256']==digest(pe.data)
  assert field_proof['canonical_provider']['source']==source_name
  assert digest(source_path.read_bytes())==field_proof['canonical_provider']['source_sha256']
  for header in field_proof['actual_headers']:
   assert digest(Path(header['path']).read_bytes())==header['sha256']
  for span in field_proof.get('native_write_proof',[]):
   assert digest(pe.read_rva(int(span['rva'],0),span['bytes']))==span['sha256']
  extra_fields[source_name]=(field_proof,count,initialized,zero,bss_flags,proof_path,content)
  sources.append(source_name)
field_specs={
 'calibration/graphics_storage.cpp':(graphics_proof,27,1,150,0xc0400080),
 'calibration/audio_storage.cpp':(audio_proof,6,0,24,0xc0300080),
 **{name:entry[:5] for name,entry in extra_fields.items()},
}
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
 elif path in field_specs:
  field_proof,count,expected_initialized,expected_zero,bss_flags=field_specs[path]
  cells=field_proof['canonical_provider']['cells']
  assert len(cells)==count and definitions=={'_'+cell['name'] for cell in cells}
  assert not any(s['section']==0 for s in symbols.values())
  assert not any(s['size'] for s in c.sections if s['name'] not in ('.drectve','.data','.bss'))
  spans={};initialized=0;zero=0
  for cell in cells:
   name='_'+cell['name'];s=symbols[name];sec=c.sections[s['section']-1]
   start=s['value'];width=cell['bytes'];end=start+width;rva=int(cell['rva'],0)
   assert end<=sec['size'] and sec['flags']&0x80000000
   spans.setdefault(s['section'],[]).append((start,end,name))
   native=next(n for n in pe.sections if n['virtual_address']<=rva<n['virtual_address']+n['virtual_size'])
   assert native['name']=='.data' and native['characteristics']&0x80000000
   if cell['native_initial_origin']=='file-backed initialized bytes':
    assert sec['name']=='.data' and sec['data'][start:end]==pe.read_rva(rva,width)==bytes.fromhex(cell['initializer'])
    initialized+=width
   else:
    assert cell['native_initial_origin']=='loader-zero virtual tail'
    assert sec['name']=='.bss' and sec['flags']==bss_flags and not sec['data']
    assert rva>=native['virtual_address']+native['raw_size'] and rva+width<=native['virtual_address']+native['virtual_size']
    if path in ('calibration/sound_bank_storage.cpp','calibration/archive_array_prefixes.c','calibration/movie_scratch_storage.c','calibration/host_resource_storage.c','calibration/movie_header_storage.c','calibration/movie_initializer_storage.c'):
     assert cell['zero_initializer'] is True
    else:
     assert bytes.fromhex(cell['initializer'])==bytes(width)
    bss_storage[name]=width;zero+=width
  for intervals in spans.values():
   intervals.sort();assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:]))
  assert initialized==expected_initialized and zero==expected_zero
  reports[path]['access_width_cells']=cells
  reports[path]['initialized_field_bytes']=initialized
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
if graphics_before is not None:
 assert graphics_evidence.read_bytes()==graphics_before
 for header in graphics_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
if audio_before is not None:
 assert audio_evidence.read_bytes()==audio_before
 for header in audio_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
for field_proof,_,_,_,_,proof_path,content in extra_fields.values():
 assert proof_path.read_bytes()==content
 for header in field_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
result={'scope':'Fresh canonical data/object equality and source/loader-zero proofs; not native placement or whole-module acceptance.','evidence_sha256':digest(evidence_before),'toolchain':pin,'target_sha256':digest(pe.data),'providers':reports,'common_storage_bytes':commons,'bss_storage_bytes':bss_storage,'initialized_bytes':sum(s['bytes'] for provider in reports.values() for s in provider.get('strings',[]))+4+sum(p.get('initialized_field_bytes',0) for p in reports.values()),'zero_storage_bytes':sum(commons.values())+sum(bss_storage.values()),'functions':0,'relocations':0}
if fatal_before is not None:
 result['fatal_evidence_sha256']=digest(fatal_before)
 result['fatal_storage_limit']='Only observed4+1+1byte zero cells; original declaration/aggregate extent, TU and native placement remain unproved.'
if registry_before is not None:
 result['registry_evidence_sha256']=digest(registry_before)
 result['registry_storage_limit']='Only the72-byte protocol prefix and49 observed initialized string bytes; full record extent, original type/TU and literal spacing/native placement remain unproved.'
if graphics_before is not None:
 result['graphics_evidence_sha256']=digest(graphics_before)
 result['graphics_storage_limit']='27observed cells/151bytes; ordinary alignment gaps, heap targets/capacities, original aggregate/TU/order/native placement remain unproved.'
if audio_before is not None:
 result['audio_evidence_sha256']=digest(audio_before)
 result['audio_storage_limit']='Six observed4byte cells/24loader-zero bytes; pointed-to objects, original aggregate/TU/order/native placement remain unproved.'
if extra_fields:
 result['additional_field_evidence']={name:digest(entry[6]) for name,entry in extra_fields.items()}
 result['additional_field_limit']='Observed sound-bank/archive prefixes and MCI cells/literal only; neighbor cache/gaps, scratch capacity, terminal source representation, pointed-to objects, original full extent/aggregate/TU/native placement remain unproved.'
engine_source=root/'calibration/engine_interface_storage.c'
engine_evidence=root/'evidence/engine_interface_storage_adoption_2026_10_11.json'
if engine_source.exists() or engine_evidence.exists():
 assert engine_source.is_file() and engine_evidence.is_file()
 engine_before=engine_evidence.read_bytes();engine_proof=json.loads(engine_before)
 provider=engine_proof['canonical_provider'];engine_sha=digest(engine_source.read_bytes())
 assert provider['source']=='calibration/engine_interface_storage.c' and engine_sha==provider['source_sha256']
 assert provider['symbol']=='_g_engine_interface' and provider['bytes']==1000 and provider['bss_flags']=='0xc0400080'
 for header in engine_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 assert {o['module'] for o in engine_proof['oracles']}=={'ENG1.DLL','ENG3.DLL'} and len(engine_proof['oracles'])==2
 for oracle in engine_proof['oracles']:
  native=PE(root/oracle['path']);assert digest(native.data)==oracle['sha256']
  rva=int(oracle['rva'],0);assert oracle['observed_zero_bytes']==1000 and int(oracle['end_rva'],0)==rva+1000
  sec=next(s for s in native.sections if s['virtual_address']<=rva<s['virtual_address']+s['virtual_size'])
  assert sec['name']=='.data' and sec['characteristics']&0x80000000
  assert rva>=sec['virtual_address']+sec['raw_size'] and rva+1000<=sec['virtual_address']+sec['virtual_size']
  for span in oracle['copy_evidence']:
   assert digest(native.read_rva(int(span['rva'],0),span['bytes']))==span['sha256']
 obj,command=compile_source(engine_source,['/O2']);c=COFF(obj)
 public={s['name']:s for s in c.symbols.values() if s['storage']==2}
 assert set(public)=={'_g_engine_interface'} and public['_g_engine_interface']['section']>0
 assert not any(s['type']&0x20 and s['section']>0 for s in c.symbols.values())
 assert not any(s['relocations'] for s in c.sections)
 symbol=public['_g_engine_interface'];sec=c.sections[symbol['section']-1]
 assert symbol['value']==0 and sec['name']=='.bss' and sec['size']==1000 and sec['flags']==0xc0400080 and not sec['data']
 assert not any(s['size'] for s in c.sections if s['name'] not in ('.drectve','.bss'))
 assert digest(engine_source.read_bytes())==engine_sha and engine_evidence.read_bytes()==engine_before
 for header in engine_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 for oracle in engine_proof['oracles']:
  assert digest((root/oracle['path']).read_bytes())==oracle['sha256']
 result['engine_interface_provider']={'source':provider['source'],'source_sha256':engine_sha,'evidence_sha256':digest(engine_before),'object':str(obj),'object_sha256':digest(Path(obj).read_bytes()),'command':command,'object_zero_bytes':1000,'native_zero_bytes_by_module':{o['module']:1000 for o in engine_proof['oracles']},'functions':0,'relocations':0,'uncredited_padding_bytes':0,'limit':'Separate image-local copies; EXE data totals above exclude these engine prefixes. No body/whole-image/TU/layout acceptance.'}
fx_source=root/'calibration/engine_fx_storage.c'
fx_evidence=root/'evidence/engine_fx_storage_adoption_2026_10_11.json'
if fx_source.exists() or fx_evidence.exists():
 assert fx_source.is_file() and fx_evidence.is_file()
 fx_before=fx_evidence.read_bytes();fx_proof=json.loads(fx_before)
 provider=fx_proof['canonical_provider'];fx_sha=digest(fx_source.read_bytes())
 assert provider['source']=='calibration/engine_fx_storage.c' and fx_sha==provider['source_sha256']
 assert provider['symbol']=='_g_engine_fx_records' and provider['bytes']==1536
 assert provider['rows']==24 and provider['stride']==64 and provider['bss_flags']=='0xc0400080'
 for header in fx_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 assert {o['module'] for o in fx_proof['oracles']}=={'ENG1.DLL','ENG3.DLL'} and len(fx_proof['oracles'])==2
 for oracle in fx_proof['oracles']:
  native=PE(root/oracle['path']);assert digest(native.data)==oracle['sha256']
  rva=int(oracle['rva'],0);assert oracle['observed_zero_bytes']==1536 and int(oracle['end_rva'],0)==rva+1536
  sec=next(s for s in native.sections if s['virtual_address']<=rva<s['virtual_address']+s['virtual_size'])
  assert sec['name']=='.data' and sec['characteristics']&0x80000000
  assert rva>=sec['virtual_address']+sec['raw_size'] and rva+1536<=sec['virtual_address']+sec['virtual_size']
  for span in oracle['access_evidence']:
   assert digest(native.read_rva(int(span['rva'],0),span['bytes']))==span['sha256']
 obj,command=compile_source(fx_source,['/O2']);c=COFF(obj)
 public={s['name']:s for s in c.symbols.values() if s['storage']==2}
 assert set(public)=={'_g_engine_fx_records'} and public['_g_engine_fx_records']['section']>0
 assert not any(s['type']&0x20 and s['section']>0 for s in c.symbols.values())
 assert not any(s['relocations'] for s in c.sections)
 symbol=public['_g_engine_fx_records'];sec=c.sections[symbol['section']-1]
 assert symbol['value']==0 and sec['name']=='.bss' and sec['size']==1536 and sec['flags']==0xc0400080 and not sec['data']
 assert not any(s['size'] for s in c.sections if s['name'] not in ('.drectve','.bss'))
 assert digest(fx_source.read_bytes())==fx_sha and fx_evidence.read_bytes()==fx_before
 for header in fx_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 for oracle in fx_proof['oracles']:
  assert digest((root/oracle['path']).read_bytes())==oracle['sha256']
 result['engine_fx_provider']={'source':provider['source'],'source_sha256':fx_sha,'evidence_sha256':digest(fx_before),'object':str(obj),'object_sha256':digest(Path(obj).read_bytes()),'command':command,'object_zero_bytes':1536,'native_zero_bytes_by_module':{o['module']:1536 for o in fx_proof['oracles']},'functions':0,'relocations':0,'uncredited_padding_bytes':0,'limit':'Minimum observed24-record storage in each engine; separate from EXE totals. Unknown members remain opaque. No original fullcapacity/TU/placement or function acceptance.'}
title_source=root/'calibration/title/title_buffer_cells.c'
title_evidence=root/'evidence/title_buffer_cells_adoption_2026_10_11.json'
if title_source.exists() or title_evidence.exists():
 assert title_source.is_file() and title_evidence.is_file()
 title_before=title_evidence.read_bytes();title_proof=json.loads(title_before)
 provider=title_proof['canonical_provider'];title_sha=digest(title_source.read_bytes())
 assert provider['source']=='calibration/title/title_buffer_cells.c' and title_sha==provider['source_sha256']
 assert provider['bytes']==8 and provider['bss_flags']=='0xc0300080'
 native=PE(root/title_proof['oracle']['path']);assert digest(native.data)==title_proof['oracle']['sha256']
 for header in title_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 obj,command=compile_source(title_source,['/O2']);c=COFF(obj)
 public={s['name']:s for s in c.symbols.values() if s['storage']==2}
 assert set(public)=={'_g_2cc04','_g_2cc08'} and len(provider['cells'])==2
 assert not any(s['type']&0x20 and s['section']>0 for s in c.symbols.values())
 assert not any(s['relocations'] for s in c.sections)
 assert not any(s['size'] for s in c.sections if s['name'] not in ('.drectve','.bss'))
 for cell in provider['cells']:
  symbol=public['_'+cell['name']];sec=c.sections[symbol['section']-1]
  assert symbol['section']>0 and symbol['value']==cell['object_offset'] and cell['bytes']==4
  assert sec['name']=='.bss' and sec['size']==8 and sec['flags']==0xc0300080 and not sec['data']
  assert cell['native_initial_origin']=='loader-zero virtual tail' and cell['zero_initializer'] is True
  rva=int(cell['rva'],0);n=next(s for s in native.sections if s['virtual_address']<=rva<s['virtual_address']+s['virtual_size'])
  assert n['name']=='.data' and n['characteristics']&0x80000000
  assert rva>=n['virtual_address']+n['raw_size'] and rva+4<=n['virtual_address']+n['virtual_size']
 assert sorted(cell['object_offset'] for cell in provider['cells'])==[0,4]
 assert digest(title_source.read_bytes())==title_sha and title_evidence.read_bytes()==title_before
 assert digest((root/title_proof['oracle']['path']).read_bytes())==title_proof['oracle']['sha256']
 for header in title_proof['actual_headers']:
  assert digest(Path(header['path']).read_bytes())==header['sha256']
 result['title_buffer_provider']={'source':provider['source'],'source_sha256':title_sha,'evidence_sha256':digest(title_before),'object':str(obj),'object_sha256':digest(Path(obj).read_bytes()),'command':command,'object_zero_bytes':8,'functions':0,'relocations':0,'uncredited_padding_bytes':0,'limit':'Two observed pointer fields only; separate from EXE totals. No original aggregate/owner/TU/native placement or function acceptance.'}
output.parent.mkdir(parents=True,exist_ok=True)
output.write_bytes((json.dumps(result,indent=2)+'\n').encode())
print(json.dumps({k:result[k] for k in ('initialized_bytes','zero_storage_bytes','functions','relocations')}))
