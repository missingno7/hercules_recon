"""Offline original PC joystick-page comparison with actual input/config helpers."""
import argparse,hashlib,json,re,struct,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare,_compare_section
from coff import COFF
from pe import PE
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--source',type=Path,default=ROOT/'src/pc/host_options_joystick.cpp')
parser.add_argument('--header',type=Path,default=ROOT/'calibration/host_options_classes.h')
parser.add_argument('--state',type=Path,default=ROOT/'calibration/host_joystick_state.c')
args=parser.parse_args();OUT=args.output.resolve();SOURCE=args.source.resolve();HEADER=args.header.resolve();STATE=args.state.resolve()
assert OUT.is_relative_to(ROOT/'work');OUT.mkdir(parents=True,exist_ok=True)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
PROFILE={
 'bind':('?OnRva_000083F0@CJoystickPage@@QAEXPAVCEdit@@KH@Z',0x83f0,352),
 'timer':('?OnTimer@CJoystickPage@@QAEXI@Z',0x8370,128),
 'init':('?OnInitDialog@CJoystickPage@@UAEHXZ',0x8550,624),
 'ok':('?OnOK@CJoystickPage@@UAEXXZ',0x87c0,112),
 'destroy':('?OnDestroy@CJoystickPage@@QAEXXZ',0x8830,32),
 'selection':('?OnRva_00008850@CJoystickPage@@QAEXXZ',0x8850,416)}
DEPENDENCIES={
 'sample':('_host_input_sample_7c50',0x7c50,576,'calibration/host_input.c'),
 'rebuild':('_cleanup_native_7aa0',0x7aa0,208,'calibration/host_config_helpers.c'),
 'append':('_cleanup_native_7b70',0x7b70,64,'calibration/host_config_helpers.c')}
LIBRARY={
 'BaseInit':('?OnInitDialog@CDialog@@UAEHXZ',0x220bd,101,124,0),
 'GetDlgItem':('?GetDlgItem@CWnd@@QBEPAV1@H@Z',0x24faa,42,104,0),
 'SetText':('?SetWindowTextA@CWnd@@QAEXPBD@Z',0x2509d,39,104,0),
 'FromHandle':('?FromHandle@CWnd@@SGPAV1@PAUHWND__@@@Z',0x22697,40,148,0),
 'Default':('?Default@CWnd@@IAEJXZ',0x225ec,39,148,0),
 'BaseDestroy':('?OnDestroy@CWnd@@IAEXXZ',0x22df4,30,148,0),
 'BaseOK':('?OnOK@CPropertyPage@@UAEXXZ',0x28ac0,1,121,0),
 'sprintf':('_sprintf',0x10f40,112,419,1)}
CONTROLS=[('sword',0x104,4),('jump',0x140,2),('punch',0x17c,5),('power',0x1b8,6),('weapon',0xc8,1),('pause',0x8c,3)]
DATA,FRAME,LOOKUP,UNKNOWN,OTHER=0x61000000,0x61000100,0x61002000,0x61002100,0x61002200
STACK,ESP,STOP,API=0x62000000,0x62008000,0x6200f000,0x65000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
signed=lambda v:v-0x100000000 if v&0x80000000 else v

def build(source,state,out,accept=True):
 out.mkdir(parents=True,exist_ok=True);verify_toolchain(DEFAULT_TOOLCHAIN)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 graph=ROOT/'evidence/host_current_source_graph.json';libs=json.loads(graph.read_text())['libraries']
 assert Path(libs[0]['path']).name=='nafxcw.lib'and libs[0]['sha256']=='5d9c3585552412a9b94a9a0b7c3a4208061a76d48a6c3283518bfe9f674c9e22'
 assert Path(libs[1]['path']).name=='libcmt.lib'and libs[1]['sha256']=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
 assert all(sha(x['path'])==x['sha256']for x in libs)
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll');assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 files=[source,state,ROOT/'calibration/registry_storage.c',ROOT/'calibration/host_startup_storage.c',ROOT/'calibration/host_input.c',ROOT/'calibration/host_config_helpers.c',ROOT/'calibration/host_bootstrap_text.c']
 watched={Path(__file__),graph,HEADER,ORACLE,runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json',*[Path(x['path'])for x in libs]}
 for p in files:watched.update(context_files(p))
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 pins={str(p):sha(p)for p in watched}
 objects=[];compiles=[];object_by_source={}
 for p in files:
  flags=['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]if p==source else['/O2']
  obj,cmd=compile_source(p,flags);objects.append(obj);object_by_source[p]=obj;compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 measurements=[]
 for role,(sym,rva,n)in PROFILE.items():
  raw=compare(objects[0],sym,ORACLE,rva,n);d=masked_compare(objects[0],sym,ORACLE,rva,n)
  if accept:
   assert raw['compiled_size']==n
   if role=='bind':assert d['differing_bytes']==8 and d['mismatch_regions']==[[77,78],[79,80],[199,200],[225,226],[251,252],[277,278],[303,304],[329,330]]and not d['conflicts']
   else:assert d['masked_equal'],(role,d)
  measurements.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,raw=raw,diagnostic=d))
 dependencies=[];recovery=json.loads((ROOT/'recovery.json').read_text())['functions']
 for role,(sym,rva,n,p)in DEPENDENCIES.items():
  obj=object_by_source[ROOT/p];raw=compare(obj,sym,ORACLE,rva,n);d=masked_compare(obj,sym,ORACLE,rva,n)
  previous=next(x for x in recovery if x['source']==p and x['symbol']==sym and int(x['rva'],0)==rva)
  assert previous['flags']==['/O2']and previous['size']==n
  dependencies.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,source=p,existing_status=previous['status'],raw=raw,diagnostic=d))
 oracle=PE(ORACLE)
 bindings={};literal_proofs=[];co=COFF(objects[0])
 if accept:
  for row in measurements:
   for name,rva in row['diagnostic']['symbols'].items():
    assert name not in bindings or bindings[name]==rva,(name,bindings.get(name),rva);bindings[name]=rva
    if name.startswith('??_C@'):
     ss=next(x for x in co.symbols.values()if x['name']==name and x['section']>0);raw=co.sections[ss['section']-1]['data'][ss['value']:];n=raw.index(0)+1
     assert oracle.read_rva(int(rva,0),n)==raw[:n]
     literal_proofs.append(dict(symbol=name,rva=rva,bytes=n,text=raw[:n-1].decode()))
  ss=next(x for x in co.symbols.values()if x['name']==PROFILE['selection'][0]and x['section']>0);sec=co.sections[ss['section']-1];table=sec['data'][372:416];assert len(table)==44
  rels=[x for x in sec['relocations']if 372<=x['offset']<416];assert len(rels)==11 and {x['offset']for x in rels}==set(range(372,416,4))
  for rel in rels:
   target=next(x for x in co.symbols.values()if x['name']==rel['symbol']and x['section']>0)
   assert target['section']==ss['section']and target['value']<369
   addend=struct.unpack_from('<I',sec['data'],rel['offset'])[0]
   assert struct.unpack('<I',oracle.read_rva(0x8850+rel['offset'],4))[0]==0x408850+target['value']+addend
 state_co=COFF(objects[1]);working=next(x for x in state_co.symbols.values()if x['name']=='_g_joystick_working_rva_56e5d8')
 assert working['section']==0 and working['value']==28 or working['section']>0 and state_co.sections[working['section']-1]['size']==28
 section=next(x for x in oracle.pe.sections if x.VirtualAddress<=0x56e5d8<x.VirtualAddress+x.Misc_VirtualSize);assert 0x56e5d8>=section.VirtualAddress+section.SizeOfRawData
 proofs=[]
 for lib_index in [0,1]:
  archive=Path(libs[lib_index]['path']).read_bytes();pos,index=8,0
  while pos<len(archive):
   h=archive[pos:pos+60];assert h[58:]==b'`\n';n=int(h[48:58])
   for role,(sym,rva,width,member,owner)in LIBRARY.items():
    if index!=member or owner!=lib_index:continue
    obj=out/('lib%d_%d.obj'%(owner,index));obj.write_bytes(archive[pos+60:pos+60+n]);co=COFF(obj);ss=next(x for x in co.symbols.values()if x['name']==sym and x['section']>0);sec=co.sections[ss['section']-1];assert ss['value']==0
    assert sec['size']==width and len(sec['data'])==width
    d=_compare_section(co,sec,ss['section'],oracle,rva,width);assert d['masked_equal'],(role,d)
    proofs.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=width,member=member,library=libs[owner]['path'],member_sha256=sha(obj),diagnostic=d,object=str(obj),execution='actual unchanged body'if role in ['SetText','BaseDestroy','BaseOK']else'modeled identified boundary'))
   pos+=60+n+(n&1);index+=1
 assert len(proofs)==8 and pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,dependencies=dependencies,literals=literal_proofs,working=dict(rva='0x56e5d8',bytes=28,backing='virtual_zero'),external_text=dict(symbol='_g_host_empty_text_3f74c',native_rva='0x3f74c',observed_prefix_bytes=1,canonical_provider=True,capacity_accepted=False),library=proofs,libraries=libs,coff_code_objects=[str(object_by_source[p])for p in [source,ROOT/'calibration/host_input.c',ROOT/'calibration/host_config_helpers.c']],coff_data_objects=[str(object_by_source[p])for p in [state,ROOT/'calibration/registry_storage.c',ROOT/'calibration/host_startup_storage.c',ROOT/'calibration/host_bootstrap_text.c']])

DATA_SPECS=[
 ('_g_host_empty_text_3f74c',0x3f74c,1),
 ('_g_joystick_working_rva_56e5d8',0x56e5d8,28),('_g_registry_config',0x79e858,72),
 ('_g_exe_b9e458',0x79e458,1024),('_g_config_default_values_rva_36158',0x36158,36),
 ('_g_config_table_a_rva_56e4f0',0x56e4f0,112),('_g_config_table_b_rva_56e560',0x56e560,112),
 ('_g_startup_apply_guard_rva_51c450',0x51c450,4),('_g_host_input_joy_available_rva_79e8a0',0x79e8a0,16),
 ('_g_host_input_joy_caps_rva_79e8b0',0x79e8b0,6464),('_g_host_input_joy_count_rva_7a0218',0x7a0218,4),
 ('_g_host_input_last_state_first_rva_56e5d0',0x56e5d0,4),('_g_host_input_last_state_second_rva_56e5d4',0x56e5d4,4),
 ('_g_host_input_pending_rva_7a0234',0x7a0234,4),('_g_host_input_gate_rva_03a594',0x3a594,1)]
API_CALLS={'SendMessageA':16,'SetWindowTextA':8,'GetAsyncKeyState':4,'GetFocus':0,'SetTimer':16,'KillTimer':8,'joyGetPosEx':8}
MODELED={'BaseInit':0,'GetDlgItem':4,'FromHandle':4,'Default':0,'sprintf':0}

def prepare(b):
 """Ordinary COFF relocations for an explicit emulator layout,not a link proof."""
 oracle=PE(ORACLE);segments=[];symbols={};section_addresses={};coffs=[];definitions={};cursor=0x20000000
 providers=[COFF(path)for path in b['coff_data_objects']];provider_paths={id(co):path for co,path in zip(providers,b['coff_data_objects'])};data_proofs=[]
 for name,rva,n in DATA_SPECS:
  matches=[(co,s)for co in providers for s in co.symbols.values()if s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0)]
  assert len(matches)==1,(name,matches);co,s=matches[0]
  if s['section']==0:assert s['value']==n;raw=bytes(n)
  else:
   sec=co.sections[s['section']-1];assert not sec['relocations']
   raw=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n);assert len(raw)==n
  section=next(x for x in oracle.pe.sections if x.VirtualAddress<=rva and rva+n<=x.VirtualAddress+x.Misc_VirtualSize)
  virtual_zero=rva>=section.VirtualAddress+section.SizeOfRawData
  expected=bytes(n)if virtual_zero else oracle.read_rva(rva,n);assert raw==expected,name
  symbols[name]=0x400000+rva;segments.append(dict(address=symbols[name],data=raw.hex(),kind='verified source data prefix'))
  data_proofs.append(dict(symbol=name,rva=hex(rva),bytes=n,source_object=provider_paths[id(co)],sha256=hashlib.sha256(raw).hexdigest(),backing='virtual_zero'if virtual_zero else'file_backed',fixture_native_addresses=True))
 endpoints={API+i*0x20:(name,n)for i,(name,n)in enumerate({**API_CALLS,**MODELED}.items())}
 api_addresses={name:addr for addr,(name,n)in endpoints.items()}
 for role,(sym,rva,n,member,owner)in LIBRARY.items():
  if role in MODELED:symbols[sym]=api_addresses[role]
 units=[]
 for p in b['coff_code_objects']:
  co=COFF(p);coffs.append(co);units.append((co,[i for i,s in enumerate(co.sections,1)if not s['name'].startswith('.debug')and s['name']!='.drectve']))
 for proof in b['library']:
  if proof['execution']!='actual unchanged body':continue
  co=COFF(proof['object']);coffs.append(co);s=next(x for x in co.symbols.values()if x['name']==proof['symbol']and x['section']>0);units.append((co,[s['section']]))
 for co,indices in units:
  for index in indices:
   sec=co.sections[index-1]
   if not sec['size']:continue
   section_addresses[(id(co),index)]=cursor;cursor+=(sec['size']+15)&~15
  for s in co.symbols.values():
   if s['storage']==2 and s['section']>0 and(id(co),s['section'])in section_addresses:
    addr=section_addresses[(id(co),s['section'])]+s['value']
    if s['name']in symbols:assert symbols[s['name']]==addr,('duplicate source definition',s['name'])
    symbols[s['name']]=addr;definitions[s['name']]=(co,s)
 assert cursor<0x20100000
 unsupported={};iat={};iat_values={};relocation_count=0
 def trap(name):
  if name not in unsupported:unsupported[name]=API+0x5000+len(unsupported)*16
  return unsupported[name]
 def resolve(co,name):
  if name in symbols:return symbols[name]
  candidates=[x for x in co.symbols.values()if x['name']==name]
  defined=[s for s in candidates if s['section']>0 and(id(co),s['section'])in section_addresses]
  if defined:return section_addresses[(id(co),defined[0]['section'])]+defined[0]['value']
  if name.startswith('__imp__'):
   if name not in iat:
    match=re.fullmatch(r'__imp__(\w+)@\d+',name);api=match[1]if match else name
    cell=API+0x3000+len(iat)*4;iat[name]=cell;iat_values[cell]=api_addresses[api]if api in api_addresses else trap(name)
   return iat[name]
  return trap(name)
 for co,indices in units:
  for index in indices:
   sec=co.sections[index-1]
   if not sec['size']:continue
   addr=section_addresses[(id(co),index)];raw=bytearray(sec['data']or bytes(sec['size']));assert len(raw)==sec['size']
   for rel in sec['relocations']:
    off=rel['offset'];addend=struct.unpack_from('<I',raw,off)[0];target=resolve(co,rel['symbol'])
    assert rel['type']in [6,20],rel
    value=addend+target-(addr+off+4 if rel['type']==20 else 0);struct.pack_into('<I',raw,off,value&0xffffffff);relocation_count+=1
   segments.append(dict(address=addr,data=raw.hex(),kind='unmodified source COFF section with ordinary relocations'))
 for addr,value in iat_values.items():segments.append(dict(address=addr,data=pack(value).hex(),kind='modeled import cell'))
 widths={x['role']:x['raw']['compiled_size']for x in b['measurements']+b['dependencies']}
 addresses={role:symbols[sym]for role,(sym,*_)in {**PROFILE,**DEPENDENCIES}.items()}
 for role,(sym,rva,n,member,owner)in LIBRARY.items():
  if role not in MODELED:addresses[role]=symbols[sym];widths[role]=n
 return dict(scope='Synthetic COFF layout with verified source data prefixes at oracle addresses;not natural link/global layout acceptance',segments=segments,symbols=symbols,addresses=addresses,widths=widths,endpoints=endpoints,unsupported=unsupported,data_proofs=data_proofs,relocations=relocation_count,source_code_base=0x20000000,source_code_bytes=cursor-0x20000000)

def execute(native,b,p,s):
 oracle=pefile.PE(str(ORACLE));size=oracle.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x400000,(size+4095)&~4095)
 if native:u.mem_write(0x400000,oracle.get_memory_mapped_image().ljust(size,b'\0'))
 else:u.mem_map(0x20000000,0x100000)
 u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000);u.mem_map(API,0x10000)
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def p32(a,v):u.mem_write(a,pack(v))
 def text(a):
  result=bytearray()
  for i in range(256):
   v=bytes(u.mem_read(a+i,1))[0]
   if v==0:return result.decode('latin1')
   result.append(v)
  raise AssertionError('unterminated modeled text')
 if not native:
  for segment in p['segments']:u.mem_write(segment['address'],bytes.fromhex(segment['data']))
 endpoints={int(a):tuple(t)for a,t in p['endpoints'].items()if t[0]in API_CALLS}
 api_addresses={name:a for a,(name,n)in endpoints.items()}
 for role,n in MODELED.items():
  sym,rva,*_=LIBRARY[role];addr=0x400000+rva if native else p['symbols'][sym];endpoints[addr]=(role,n)
 if native:
  for dll in oracle.DIRECTORY_ENTRY_IMPORT:
   for item in dll.imports:
    if item.name and item.name.decode()in api_addresses:p32(item.address,api_addresses[item.name.decode()])
 addresses={role:0x400000+rva if native else p['addresses'][role]for role,(sym,rva,n)in PROFILE.items()}
 widths={role:n if native else p['widths'][role]for role,(sym,rva,n)in PROFILE.items()}
 for role,(sym,rva,n,path)in DEPENDENCIES.items():addresses[role]=0x400000+rva if native else p['addresses'][role];widths[role]=n if native else p['widths'][role]
 for role,(sym,rva,n,member,owner)in LIBRARY.items():
  if role in MODELED:continue
  addresses[role]=0x400000+rva if native else p['addresses'][role];widths[role]=n
 ranges=[(addresses[k],addresses[k]+widths[k])for k in addresses]
 cfg,work,available,caps,ta,tb=0xb9e858,0x96e5d8,0xb9e8a0,0xb9e8b0,0x96e4f0,0x96e560
 u.mem_write(DATA,bytes((i*17+(i>>8)*7+31)&255 for i in range(0x10000)))
 receivers={FRAME:'page',LOOKUP:'lookup',UNKNOWN:'unknown',OTHER:'other',FRAME+0x1f4:'list',**{FRAME+offset:name for name,offset,index in CONTROLS}}
 for i,a in enumerate(receivers):p32(a+0x1c,0x12340000+i*0x100);p32(a+0x34,0);p32(a+0x38,0)
 address=lambda name:next(a for a,n in receivers.items()if n==name)
 hwnd=lambda name:read(address(name)+0x1c)
 for name,value in s.get('handles',{}).items():p32(address(name)+0x1c,value)
 config=bytearray((i*19+13)&255 for i in range(72));config[22:24]=pack(s.get('configured_device',0))[:2]
 for i,value in enumerate(s.get('config_masks',[1,2,4,8,16,32])):config[48+i*4:52+i*4]=pack(value)
 u.mem_write(cfg,bytes(config));u.mem_write(work,b''.join(pack(v)for v in s.get('working',[s.get('selected_device',0),0x41,0x42,0x43,0x44,0x45,0x46])))
 u.mem_write(available,bytes(s.get('available',[1]+[0]*15)));u.mem_write(caps,bytes(6464));u.mem_write(0xb9e458,bytes(1024))
 for i,value in enumerate(s.get('button_counts',[6]*16)):p32(caps+i*404+60,value)
 for i in range(16):
  for offset,value in [(36,0),(40,1000),(44,0),(48,1000)]:p32(caps+i*404+offset,value)
 for offset,value in s.get('negative_caps',{}).items():p32(caps-404+int(offset),value)
 for a in [ta,tb]:u.mem_write(a,b''.join(pack(0x8bad0000+i)for i in range(28)))
 u.mem_write(0x43f74c,b'\0')
 focus=s.get('focus','sword');events=[];entries=[];errors=[];counts={};applied=[];widgets={};selected=s.get('list_selection',0)
 list_data={0:0xffffffff,1:0};root_role=None
 if focus=='alias_sword':p32(OTHER+0x1c,hwnd('sword'))
 def projection():return dict(working=[read(work+4*i)for i in range(7)],config=bytes(u.mem_read(cfg,72)).hex())
 def mutate(action):
  nonlocal focus,selected
  if 'focus'in action:focus=action['focus']
  if 'selected_device'in action:p32(work,action['selected_device'])
  if 'list_selection'in action:selected=action['list_selection']
  for k,v in action.get('working',{}).items():p32(work+4*int(k),v)
  for k,v in action.get('config_masks',{}).items():p32(cfg+48+4*int(k),v)
  if 'configured_device'in action:u.mem_write(cfg+22,pack(action['configured_device'])[:2])
  for k,v in action.get('available',{}).items():u.mem_write(available+int(k),bytes([v]))
  for k,v in action.get('button_counts',{}).items():p32(caps+int(k)*404+60,v)
  for k,v in action.get('caps_fields',{}).items():p32(caps+int(k),v)
  for k,v in action.get('handles',{}).items():p32(address(k)+0x1c,v)
 def ret(sp,cleanup,value):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4);u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EIP,target)
 def on_code(machine,a,n,_):
  nonlocal selected
  if a==STOP:machine.emu_stop();return
  if a in addresses.values():
   role=next(k for k,v in addresses.items()if v==a);sp=machine.reg_read(UC_X86_REG_ESP)
   if role=='bind':
    assert machine.reg_read(UC_X86_REG_ECX)==FRAME;receiver=read(sp+4);assert receiver in receivers
    entries.append(dict(call=role,args=[receivers[receiver],read(sp+8),read(sp+12)],before=projection()))
   elif role in DEPENDENCIES:
    argv=[read(sp+4)]if role=='sample'else[read(sp+4+i*4)for i in range(4)]if role=='append'else[]
    entries.append(dict(call=role,args=argv,before=projection()))
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];sp=machine.reg_read(UC_X86_REG_ESP);argv=[read(sp+4+i*4)for i in range(cleanup//4)];event=argv[:];result=1;message=None
  if name in ['BaseInit','GetDlgItem','Default']:assert machine.reg_read(UC_X86_REG_ECX)==FRAME
  if name=='BaseInit':result=s.get('base_result',1)
  elif name=='Default':result=s.get('default_result',0)
  elif name=='GetDlgItem':assert argv[0]in range(0x46b,0x472);result=LOOKUP
  elif name=='GetFocus':result=0 if focus=='none'else hwnd('sword'if focus=='alias_sword'else focus)
  elif name=='FromHandle':
   assert argv==[0 if focus=='none'else hwnd('sword'if focus=='alias_sword'else focus)]
   result=0 if focus=='none'else OTHER if focus=='alias_sword'else address(focus)
  elif name=='SetWindowTextA':
   event=[argv[0],text(argv[1])];widgets.setdefault(str(argv[0]),{})['text']=event[1];result=s.get('text_result',1)
  elif name=='GetAsyncKeyState':assert argv==[0x20];result=s.get('space',0)
  elif name=='SetTimer':assert argv[1:]==[0xfaf87c4e,200,0];result=s.get('timer_result',0xfaf87c4e)
  elif name=='KillTimer':assert argv[1:]==[0xfaf87c4e];result=s.get('kill_result',1)
  elif name=='joyGetPosEx':
   device,out=argv;assert ESP-0x2000<=out and out+52<ESP;assert read(out)==52 and read(out+4)==0x83
   event=[device,dict(bytes=52,flags=0x83)]
   if s.get('joy_write',True):
    p32(out+8,s.get('joy_x',500));p32(out+12,s.get('joy_y',500));p32(out+32,s.get('joy_buttons',1))
   result=s.get('joy_result',0)
  elif name=='sprintf':
   dst,fmt=read(sp+4),text(read(sp+8));argc=1 if fmt==' %d'else 2 if fmt=='Joy %d, %d buttons'else None;assert argc is not None,fmt
   vals=[read(sp+12+i*4)for i in range(argc)];result_text=fmt%tuple(signed(v)for v in vals)
   caller=read(sp);limit=32 if addresses['bind']<=caller<addresses['bind']+widths['bind']else 256
   assert ESP-0x2000<=dst and dst+limit<=ESP and len(result_text)<limit,(hex(dst),limit,hex(ESP))
   u.mem_write(dst,result_text.encode()+b'\0');event=[fmt,vals,result_text];result=len(result_text)
  elif name=='SendMessageA':
   h,message,wp,lp=argv;state=widgets.setdefault(str(h),{});result=0
   if message==0xc2:
    assert wp==0;event=[h,message,wp,text(lp)];state['text']=state.get('text','')+event[3]
   elif message==0x184:state['items']=[];list_data.clear();selected=0xffffffff
   elif message==0x181:
    assert wp==0xffffffff;caption=text(lp);items=state.setdefault('items',[]);result=s.get('insert_result',len(items));items.append(caption);event=[h,message,wp,caption]
   elif message==0x19a:list_data[wp]=lp
   elif message==0x186:assert lp==0;selected=wp;result=wp
   elif message==0x188:assert wp==0 and lp==0;result=s.get('selection_return',selected)
   elif message==0x199:assert lp==0;result=s.get('item_data_return',list_data.get(wp,0xffffffff))
   else:raise AssertionError(('unexpected message',hex(message)))
  key=(name,message,event[1]if name=='SetWindowTextA'else'');index=counts.get(key,0);counts[key]=index+1
  events.append(dict(call=name,args=event,before=projection()))
  for i,rule in enumerate(s.get('actions',[])):
   if i in applied or rule['call']!=name or rule.get('index',0)!=index:continue
   if 'message'in rule and message!=rule['message']:continue
   if 'text'in rule and(name!='SetWindowTextA'or event[1]!=rule['text']):continue
   mutate(rule);applied.append(i)
  ret(sp,cleanup,result)
 def on_write(machine,access,a,n,value,_):
  allowed=[(ESP-0x2000,ESP+16),(work,work+28)]
  if root_role=='ok':allowed.extend([(cfg+22,cfg+24),(cfg+48,cfg+72),(ta,ta+112),(tb,tb+112)])
  if root_role=='destroy':allowed.append((FRAME+0x34,FRAME+0x38))
  if not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(['write',hex(a),n,root_role]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);returns=[]
 for root_role in s.get('steps',[s.get('role','bind')]):
  argv=[address(s.get('target','sword')),s.get('buttons',1),s.get('active',1)]if root_role=='bind'else[s.get('event',0xfaf87c4e)]if root_role=='timer'else[]
  u.mem_write(ESP-0x2000,bytes([0xa7])*0x2100);p32(ESP,STOP)
  for i,v in enumerate(argv):p32(ESP+4+4*i,v)
  u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
  for reg,v in zip(NV,SEEDS):u.reg_write(reg,v)
  u.emu_start(addresses[root_role],STOP,count=100000)
  assert not errors and u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4+4*len(argv),errors
  assert[u.reg_read(reg)for reg in NV]==SEEDS
  assert bytes(u.mem_read(ESP-0x2000-32,32))==bytes(32)and bytes(u.mem_read(ESP+16,64))==bytes([0xa7])*64
  if root_role=='init':returns.append(u.reg_read(UC_X86_REG_EAX));assert returns[-1]==1
 assert len(applied)==len(s.get('actions',[])),('unused actions',s,applied)
 return dict(events=events,entries=entries,returns=returns,final=projection(),table_a=[read(ta+4*i)for i in range(28)],table_b=[read(tb+4*i)for i in range(28)],availability=bytes(u.mem_read(available,16)).hex(),caps_sha256=hashlib.sha256(u.mem_read(caps,6464)).hexdigest(),prefix_sha256=hashlib.sha256(u.mem_read(0xb9e458,1024)).hexdigest(),arena_sha256=hashlib.sha256(u.mem_read(DATA,0x10000)).hexdigest(),widgets=widgets,stack_nonvolatiles_guards=True)

def scenarios():
 cases=[]
 for buttons in [0,1,2,7,0xb,0xf,1<<27,0xf0000000,0x80000007,0x0fffffff,0xffffffff]:
  for active in [0,1,128,0x80000000]:
   for space in [0,1,0x8000]:cases.append(dict(name='bind_%x_%x_%x'%(buttons,active,space),buttons=buttons,active=active,space=space))
 for i in range(28):cases.append(dict(name='bit_%d'%i,buttons=1<<i))
 for i in range(27):cases.append(dict(name='pair_%d'%i,buttons=3<<i))
 for i in range(26):cases.append(dict(name='triple_%d'%i,buttons=7<<i))
 for target in [x[0]for x in CONTROLS]+['unknown','other']:cases.append(dict(name='target_'+target,target=target,buttons=0x81))
 for count in [*range(17),255,0x80000000,0xffffffff]:cases.append(dict(name='select_buttons_%x'%count,role='selection',item_data_return=0,button_counts=[count]*16))
 cases.extend([dict(name='select_device15',role='selection',item_data_return=15,button_counts=list(range(16))),dict(name='layout_none',role='selection',item_data_return=0xffffffff,layout_model=True),dict(name='layout_none_prefix10',role='selection',item_data_return=0xffffffff,negative_caps={'60':10},layout_model=True)])
 for value in [0,1,15,16,0xffff,0x8000]:cases.append(dict(name='init_device_%x'%value,role='init',configured_device=value,available=[1]*16))
 cases.extend([dict(name='init_no_devices',role='init',available=[0]*16,configured_device=0xffff),dict(name='init_sparse_devices',role='init',available=[0,0xff]+[0]*13+[0x80],button_counts=list(range(16)),configured_device=15),dict(name='init_api_failures',role='init',insert_result=0xffffffff,base_result=0,text_result=0,timer_result=0),dict(name='init_signed_button_count',role='init',button_counts=[0xffffffff]*16)])
 for focus in [x[0]for x in CONTROLS]+['none','page','list','unknown','alias_sword']:
  for buttons in [0,7,15]:cases.append(dict(name='timer_%s_%x'%(focus,buttons),role='timer',focus=focus,joy_buttons=buttons))
 for event in [0,1,0xfaf87c4f]:cases.append(dict(name='other_timer_%x'%event,role='timer',event=event))
 cases.extend([
  dict(name='timer_device15',role='timer',selected_device=15,available=[1]*16),
  dict(name='timer_unavailable',role='timer',available=[0]*16),
  dict(name='timer_joy_failure_no_write',role='timer',joy_write=False,joy_result=1),
  dict(name='timer_joy_failure_writes',role='timer',joy_buttons=5,joy_result=1),
  dict(name='timer_axes_only',role='timer',joy_buttons=0,joy_x=0,joy_y=1000),
  dict(name='timer_axes_three_buttons',role='timer',joy_buttons=7,joy_x=1000,joy_y=0),
  dict(name='timer_position_overflow',role='timer',joy_x=0xffffffff,joy_y=0x80000000),
  dict(name='layout_timer_none',role='timer',selected_device=0xffffffff,layout_model=True),
  dict(name='layout_timer_none_pause_alias',role='timer',selected_device=0xffffffff,config_masks=[1,2,4,8,16,1<<27],joy_write=False,joy_result=1,layout_model=True),
  dict(name='save_raw',role='ok',working=[0x1234ffff,0xf0000001,3,0xffffffff,7,0x80000001,0]),
  dict(name='save_duplicate_priorities',role='ok',working=[0,1,3,1,7,3,1]),
  dict(name='destroy',role='destroy'),
  dict(name='whole_flow',steps=['init','selection','timer','ok','destroy'],item_data_return=1,available=[1]*16,joy_buttons=7)])
 cases.extend([
  dict(name='live_helper_handles',buttons=7,actions=[dict(call='GetAsyncKeyState',handles={'sword':0xdead0001}),dict(call='SendMessageA',message=0xc2,handles={'sword':0xdead0002},working={'0':15})]),
  dict(name='live_enumeration',role='init',actions=[dict(call='SendMessageA',message=0x19a,available={'0':0,'1':255,'15':128},button_counts={'1':10},selected_device=15)]),
  dict(name='live_selected_after_itemdata',role='init',available=[1,1]+[0]*14,actions=[dict(call='SendMessageA',message=0x19a,index=1,selected_device=1)]),
  dict(name='live_init_config',role='init',actions=[dict(call='SetWindowTextA',text='GamePad',configured_device=0xffff,config_masks={'0':7,'1':0x81})]),
  dict(name='live_init_timer_handle',role='init',config_masks=[0]*6,actions=[dict(call='SetWindowTextA',text='None',handles={'page':0xdead1000})]),
  dict(name='live_timer_selection',role='timer',available=[1]*16,actions=[dict(call='Default',selected_device=1),dict(call='joyGetPosEx',selected_device=15,caps_fields={'444':2000})]),
  dict(name='live_timer_fromhandle',role='timer',available=[1]*16,actions=[dict(call='FromHandle',selected_device=15,handles={'sword':0xdead2000})]),
  dict(name='live_destroy_handle',role='destroy',actions=[dict(call='Default',handles={'page':0xdead3000})]),
  dict(name='live_selection_buttons',role='selection',item_data_return=0,actions=[dict(call='SendMessageA',message=0x199,button_counts={'0':10})]),
  dict(name='live_selection_masks',role='selection',item_data_return=0,actions=[dict(call='GetAsyncKeyState',working={'2':0x81},handles={'jump':0xdead4000})])])
 return cases

def main():
 b=build(SOURCE,STATE,OUT);p=prepare(b);cases=[];ss=scenarios()
 for i,s in enumerate(ss):
  native=execute(True,b,p,s);candidate=execute(False,b,p,s)
  if native!=candidate:
   (OUT/'difference.json').write_bytes((json.dumps(dict(scenario=s,native=native,candidate=candidate),indent=2)+'\n').encode());raise AssertionError(('native/source mismatch',s['name'],[k for k in native if native[k]!=candidate[k]]))
  cases.append(dict(scenario=s,equal=True,projection=native))
  if i and i%64==0:print('Verified',i,'joystick scenarios',flush=True)
 text=SOURCE.read_text();wrong={
  'axis_bits_retained':text.replace('buttons &= 0x0fffffff;','buttons &= 0xffffffff;'),
  'three_buttons_rejected':text.replace('if (count > 3)','if (count >= 3)'),
  'cached_selected_device':text.replace('    m_control_0431.ResetContent();','    int configured = SELECTED;\n    m_control_0431.ResetContent();').replace('if (SELECTED == device)','if (configured == device)'),
  'focus_hwnd_equality':text}
 for member in ['03ea','03eb','03ec','03ed','03ef','03f0']:wrong['focus_hwnd_equality']=wrong['focus_hwnd_equality'].replace('focus == &m_control_'+member,'(focus && focus->m_hWnd == m_control_'+member+'.m_hWnd)')
 names={'axis_bits_retained':'bind_f0000000_0_0','three_buttons_rejected':'triple_0','cached_selected_device':'live_selected_after_itemdata','focus_hwnd_equality':'timer_alias_sword_7'}
 negatives=[]
 for name,code in wrong.items():
  assert code!=text,name
  lane=OUT/name;lane.mkdir(exist_ok=True);source=lane/'wrong.cpp';code=re.sub(r'#include "[^"]*host_options_classes.h"','#include "'+HEADER.as_posix()+'"',code);code=re.sub(r'#include "[^"]*host_bootstrap_text.h"','#include "'+(ROOT/'calibration/host_bootstrap_text.h').as_posix()+'"',code);source.write_bytes(code.encode())
  bad=build(source,STATE,lane,False);plan=prepare(bad);s=next(x for x in ss if x['name']==names[name]);native=execute(True,b,p,s);candidate=execute(False,bad,plan,s);assert native!=candidate,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=native,wrong=candidate,build=bad))
 assert b['inputs']=={path:sha(path)for path in b['inputs']}
 report=dict(scope='Finite PC joystick callback/helper comparisons. Source is freshly compiled COFF relocated conventionally into an explicit oracle-data-address fixture model. Actual original/source input sampler and config-table builder execute;their existing RECONSTRUCTED status is unchanged. Actual unmodified MFC SetWindowText/OnDestroy/no-op OnOK ordinary branches execute. Other library/Windows boundaries are modeled. No linked/global-layout,real UI/device/registry/process/game execution acceptance.',build=b,layout_model=dict(scope=p['scope'],data_proofs=p['data_proofs'],relocations=p['relocations'],unsupported_symbols=list(p['unsupported']),unsupported_execution_accepted=False,negative_device_cases=sum(bool(x.get('layout_model'))for x in ss)),case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives)
 path=OUT/'options_joystick_differential.json';path.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negatives=len(negatives),report=str(path))))

if __name__=='__main__':main()
