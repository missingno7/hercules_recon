"""Offline sound/gameplay Options callback comparison with real MFC window helpers."""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare,_compare_section
from pe import PE
from coff import COFF
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/options_sound_gameplay_replay')
args=parser.parse_args();OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_options_sound_gameplay.cpp'
HEADER=ROOT/'calibration/host_options_classes.h'
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
signed=lambda v:v-0x100000000 if v&0x80000000 else v
PROFILE={
 'sound_init':('?OnInitDialog@CSoundPage@@UAEHXZ',0x102d0,192),
 'sound_toggle':('?OnRva_00010390@CSoundPage@@QAEXXZ',0x10390,48),
 'music_toggle':('?OnRva_000103C0@CSoundPage@@QAEXXZ',0x103c0,48),
 'sound_ok':('?OnOK@CSoundPage@@UAEXXZ',0x103f0,80),
 'game_init':('?OnInitDialog@CGamePlayPage@@UAEHXZ',0x6210,192),
 'difficulty':('?OnRva_000062D0@CGamePlayPage@@QAEXXZ',0x62d0,96),
 'lives':('?OnRva_00006330@CGamePlayPage@@QAEXXZ',0x6330,96),
 'game_ok':('?OnOK@CGamePlayPage@@UAEXXZ',0x6390,32)}
MFC={
 'BaseInit':('?OnInitDialog@CDialog@@UAEHXZ',0x220bd,101,124),
 'GetDlgItem':('?GetDlgItem@CWnd@@QBEPAV1@H@Z',0x24faa,42,104),
 'SetText':('?SetWindowTextA@CWnd@@QAEXPBD@Z',0x2509d,39,104),
 'Enable':('?EnableWindow@CWnd@@QAEHH@Z',0x25155,39,104),
 'Show':('?ShowWindow@CWnd@@QAEHH@Z',0x25113,39,104),
 'Range':('?SetRange@CSliderCtrl@@QAEXHHH@Z',0x1c9cc,51,128),
 'BaseOK':('?OnOK@CPropertyPage@@UAEXXZ',0x28ac0,1,121)}
APIS={'SetWindowTextA':8,'EnableWindow':8,'ShowWindow':8,'SendMessageA':16}
DATA,FRAME,LOOKUP,LOOKUP2=0x61000000,0x61000100,0x61002000,0x61002100
STACK,ESP,STOP,API=0x62000000,0x62008000,0x6200f000,0x65000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
NV_SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]


def build(source=None,out=None):
 source=source or SOURCE;out=out or OUT;out.mkdir(parents=True,exist_ok=True)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 verify_toolchain(DEFAULT_TOOLCHAIN)
 inputs=[
  ('mfc/lib/nafxcw.lib', '5d9c3585552412a9b94a9a0b7c3a4208061a76d48a6c3283518bfe9f674c9e22'),
  ('lib/libcmt.lib', '7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'),
  ('lib/kernel32.lib', '095f231e13cb3e6a6c4913d33ca8048506011034c89774b6ba0857466abba3cf'),
  ('lib/user32.lib', '5badb752dbcd6355a666d5b88aa9fd5e27291c16edc76f1b5258468d994f1809'),
  ('lib/gdi32.lib', '3c202569b76ec990be85393d484261fdb9b67614b35d7abd169eecf845d070f8'),
  ('lib/comdlg32.lib', 'cf4320d34c6fa1cfa0738973bdf3e984dddaeadf5050ddf6a5054969e7a4d827'),
  ('lib/winspool.lib', 'a27d801745ad55b08c4bc47fb21d2a9f374f38cb0821501858c1f41623641477'),
  ('lib/advapi32.lib', '2d7da3304b3f0b21fd5a3ddcec1aa91c7d64b270087793eb8e1dfd5be21e6ae5'),
  ('lib/shell32.lib', '2c717c106e8831b7b8dbc37ef93774f1978535b1122dbbcea2e29bd2bbe969c4'),
  ('lib/comctl32.lib', 'c6ee0e9711255dad6cbf371221dfe7b47d38bea641cb31d462906f624e4a2a41'),
  ('lib/winmm.lib', 'a996cf8e569b6f2dd0a478119a57c325733af545ffee2e6c6fa7dba40bfba260'),
  ('lib/ddraw.lib', 'd2150aeb8c09eed04a566751c98ccd1738d186d304f0268f22d5f6f655e80a97'),
  ('lib/dsound.lib', '4d6e8e1e0af841da16eb621f52ef40cab455b9bd31d4b9407a549e827d257773'),
 ]
 libraries=[dict(path=str(DEFAULT_TOOLCHAIN/path),sha256=expected)for path,expected in inputs]
 assert all(sha(x['path'])==x['sha256']for x in libraries)
 files=[source,ROOT/'calibration/registry_storage.c',ROOT/'calibration/host_startup_storage.c']
 watched={ORACLE,HEADER,Path(__file__),ROOT/'recovery.json',ROOT/'toolchains/manifest.json',
  *[Path(x['path'])for x in libraries]}
 for p in files:watched.update(context_files(p))
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll')
 assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 watched.add(runtime)
 pins={str(p):sha(p)for p in watched}
 anchor=out/'anchor.cpp'
 anchor.write_bytes(('#include "'+HEADER.as_posix()+'"\n'+
  'extern "C" int main(void) { for (;;) {} }\n'+
  'extern "C" __declspec(dllexport) void anchor(CSoundPage*s,CGamePlayPage*g) { s->CSoundPage::OnInitDialog(); s->CSoundPage::OnOK(); s->OnRva_00010390(); s->OnRva_000103C0(); g->CGamePlayPage::OnInitDialog(); g->CGamePlayPage::OnOK(); g->OnRva_000062D0(); g->OnRva_00006330(); }\n').encode())
 objects,compiles,definitions,owners=[],[],{},{}
 for p in [*files,anchor]:
  flags=['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]if p in [source,anchor]else['/O2']
  obj,cmd=compile_source(p,flags);objects.append(obj);co=COFF(obj)
  compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
  for s in co.symbols.values():
   if s['storage']==2 and(s['section']>0 or s['section']==0 and s['value']>0):
    owners.setdefault(s['name'],[]).append((co,s))
    if s['type']&0x20:
     assert s['name']not in definitions;definitions[s['name']]=(obj,co.function(s['name'])['size'])
 dll,mp=out/'fixture.dll',out/'fixture.map'
 cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB',
  '/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),
  *map(str,objects),*[x['path']for x in libraries]]
 env=os.environ.copy()
 for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
 env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
 p=subprocess.run(cmd,env=env,capture_output=True,text=True)
 (out/'link.log').write_bytes((p.stdout+p.stderr).encode());assert p.returncode==0,p.stdout+p.stderr
 regex=re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
 symbols={m[1]:int(m[2],16)for line in mp.read_text().splitlines()if(m:=regex.match(line))}
 measurements=[]
 for role,(sym,rva,n)in PROFILE.items():
  obj,width=definitions[sym];raw=compare(obj,sym,ORACLE,rva,n);diag=masked_compare(obj,sym,ORACLE,rva,n)
  if source==SOURCE:assert diag['masked_equal'],(role,diag)
  measurements.append(dict(role=role,symbol=sym,source_size=width,raw=raw,diagnostic=diag))
 archive=Path(libraries[0]['path']).read_bytes();assert Path(libraries[0]['path']).name=='nafxcw.lib'
 position,index,members=8,0,{}
 while position<len(archive):
  h=archive[position:position+60];assert h[58:]==b'`\n';length=int(h[48:58])
  if index in [104,121,124,128]:
   p=out/('mfc_member_'+str(index)+'.obj');p.write_bytes(archive[position+60:position+60+length]);members[index]=p
  position+=60+length+(length&1);index+=1
 linked=pefile.PE(str(dll));original=PE(ORACLE);proofs=[]
 for role,(sym,rva,n,index)in MFC.items():
  obj=members[index];co=COFF(obj);symbol=next(x for x in co.symbols.values()if x['name']==sym and x['section']>0);sec=co.sections[symbol['section']-1]
  assert symbol['value']==0 and sec['size']==n and len(sec['data'])==n
  diag=_compare_section(co,sec,symbol['section'],original,rva,n);assert diag['masked_equal'],(role,diag)
  raw=bytearray(linked.get_data(symbols[sym]-linked.OPTIONAL_HEADER.ImageBase,n));expected=bytearray(sec['data'])
  for rel in sec['relocations']:raw[rel['offset']:rel['offset']+4]=bytes(4);expected[rel['offset']:rel['offset']+4]=bytes(4)
  assert raw==expected
  aliases=[x['name']for x in co.symbols.values()if x['section']==symbol['section']and x['type']&0x20]
  proofs.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,member=index,member_sha256=sha(obj),diagnostic=diag,linked_code_diagnostic_equal=True,archive_function_aliases=aliases,mode='modeled identified boundary'if role in ['BaseInit','GetDlgItem']else'actual unchanged code'))
 data=[]
 for sym,rva,n in [('_g_registry_config',0x79e858,72),('_g_startup_apply_guard_rva_51c450',0x51c450,4)]:
  assert len(owners[sym])==1;co,s=owners[sym][0]
  if s['section']==0:assert s['value']==n;value=bytes(n)
  else:
   sec=co.sections[s['section']-1];assert not sec['relocations'];value=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n)
  target=next(x for x in original.pe.sections if x.VirtualAddress<=rva and rva+n<=x.VirtualAddress+x.Misc_VirtualSize)
  assert value==bytes(n)and rva>=target.VirtualAddress+target.SizeOfRawData
  data.append(dict(symbol=sym,rva=hex(rva),bytes=n,backing='virtual_zero',new_provider=False))
 assert pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,mfc_proofs=proofs,data=data,
  libraries=libraries,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols)


def execute(kind,b,s):
 native=kind=='native';image=pefile.PE(str(ORACLE if native else b['dll']))
 base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
 u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000);u.mem_map(API,0x1000)
 roles=s.get('steps',[s.get('role','sound_init')]);sound=roles[0].startswith('sound')or roles[0]=='music_toggle';object_size=328 if sound else 388
 offsets=dict(sound=0x140,music=0x144)if sound else dict(difficulty=0x17c,lives=0x180)
 controls=dict(slider=0x8c,sound_button=0xc8,music_button=0x104)if sound else dict(difficulty_label=0x8c,lives_label=0xc8,lives_button=0x104,difficulty_button=0x140)
 registry=0xb9e858 if native else b['symbols']['_g_registry_config'];guard=0x91c450 if native else b['symbols']['_g_startup_apply_guard_rva_51c450']
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def p32(a,v):u.mem_write(a,pack(v))
 def string(a):return bytes(u.mem_read(a,160)).split(b'\0')[0].decode('latin1')
 arena=bytearray((i*17+(i>>8)*7+31)&255 for i in range(0x10000));u.mem_write(DATA,bytes(arena))
 receivers={FRAME:'page',LOOKUP:'lookup',LOOKUP2:'lookup2',**{FRAME+o:k for k,o in controls.items()}}
 for i,a in enumerate(receivers):p32(a+0x1c,0x12340000+i*0x100);p32(a+0x38,0)
 p32(LOOKUP+0x1c,0x22334455);p32(LOOKUP2+0x1c,0x33445566)
 for k,o in offsets.items():p32(FRAME+o,s.get('fields',{}).get(k,1 if k in ['sound','music','difficulty']else 3))
 cfg=bytearray((i*19+13)&255 for i in range(72));cfg[12]=s.get('cfg_difficulty',1);cfg[13]=s.get('cfg_music',1);cfg[14]=s.get('cfg_sound',1)
 cfg[16:20]=pack(s.get('cfg_volume',0x2345));cfg[24:28]=pack(s.get('cfg_lives',3));u.mem_write(registry,bytes(cfg));p32(guard,s.get('guard',1))
 views=dict(cfg_difficulty=(registry+12,1),cfg_music=(registry+13,1),cfg_sound=(registry+14,1),cfg_volume=(registry+16,4),cfg_lives=(registry+24,4),guard=(guard,4))
 views.update({k:(FRAME+o,4)for k,o in offsets.items()})
 def mutate(action):
  for k,v in action.get('mutate',{}).items():a,n=views[k];u.mem_write(a,pack(v)[:n])
  for k,v in action.get('handles',{}).items():a=next(a for a,name in receivers.items()if name==k);p32(a+0x1c,v)
 addresses={r:0x400000+a if native else b['symbols'][sym]for r,(sym,a,n)in PROFILE.items()}
 widths={x['role']:x['source_size']for x in b['measurements']};ranges=[(addresses[r],addresses[r]+(n if native else widths[r]))for r,(sym,a,n)in PROFILE.items()]
 endpoints={}
 for r,(sym,a,n,index)in MFC.items():
  addresses[r]=0x400000+a if native else b['symbols'][sym]
  if r in ['BaseInit','GetDlgItem']:endpoints[addresses[r]]=(r,0 if r=='BaseInit'else 4)
  else:ranges.append((addresses[r],addresses[r]+n))
 for i,(name,cleanup)in enumerate(APIS.items()):
  a=API+i*16;endpoints[a]=(name,cleanup)
  for d in image.DIRECTORY_ENTRY_IMPORT:
   for imp in d.imports:
    if imp.name and imp.name.decode()==name:p32(imp.address,a)
 events,entries,errors,counts,returns=[],[],[],{},[];widgets={};slider_position=s.get('slider_position',0x3456)
 def ret(sp,cleanup,value):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4)
  u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EIP,target)
 def on_code(machine,a,n,_):
  nonlocal slider_position
  if a==STOP:machine.emu_stop();return
  if a in addresses.values():
   role=next(k for k,v in addresses.items()if v==a);entries.append(role)
   if role in MFC:assert machine.reg_read(UC_X86_REG_ECX)in receivers
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];sp=machine.reg_read(UC_X86_REG_ESP);argv=[read(sp+4+i*4)for i in range(cleanup//4)]
  index=counts.get(name,0);counts[name]=index+1;actions=s.get('actions',{}).get(name,[]);action=actions[index]if index<len(actions)else{}
  result=action.get('result',1);event=argv[:]
  before={k:int.from_bytes(u.mem_read(p,n),'little')for k,(p,n)in views.items()}
  if name=='BaseInit':assert machine.reg_read(UC_X86_REG_ECX)==FRAME
  elif name=='GetDlgItem':
   assert machine.reg_read(UC_X86_REG_ECX)==FRAME and argv==[0x460 if sound else 0x467]
   result=LOOKUP2 if action.get('second_lookup')else LOOKUP
  else:
   hwnd=argv[0];assert any(read(p+0x1c)==hwnd for p in receivers),('unknown current HWND',hex(hwnd))
   state=widgets.setdefault(str(hwnd),{})
   if name=='SetWindowTextA':event=[hwnd,string(argv[1])];state['text']=event[1]
   elif name=='EnableWindow':state['enabled_bits']=argv[1]
   elif name=='ShowWindow':state['show_command']=argv[1]
   elif name=='SendMessageA':
    message,wparam,lparam=argv[1:];assert message in [0x400,0x405,0x407,0x408,0x414]
    if message==0x400:result=action.get('result',s.get('slider_return',slider_position))
    elif message==0x405:assert wparam==1;slider_position=lparam;state['raw_position']=lparam
    elif message==0x407:assert lparam==0;state['minimum']=lparam;state['min_redraw']=wparam
    elif message==0x408:assert lparam==0x3fff;state['maximum']=lparam;state['max_redraw']=wparam
    elif message==0x414:assert wparam==0x800 and lparam==0;state['tick_frequency']=wparam
  mutate(action);events.append(dict(call=name,args=event,before=before));ret(sp,cleanup,result)
 current=None
 def on_write(machine,access,a,n,value,_):
  allowed=[(ESP-0x2000,ESP+4),*[(FRAME+o,FRAME+o+4)for o in offsets.values()]]
  if current=='sound_ok':allowed.extend([(registry+13,registry+15),(registry+16,registry+20)])
  if current=='game_ok':allowed.extend([(registry+12,registry+13),(registry+24,registry+28)])
  if not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(['write',hex(a),n,current]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
 checks=[]
 for current in roles:
  assert(current.startswith('sound')or current=='music_toggle')==sound
  u.mem_write(ESP-0x2000,bytes([0xa7])*0x2100);p32(ESP,STOP);u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
  for reg,v in zip(NV,NV_SEEDS):u.reg_write(reg,v)
  u.emu_start(addresses[current],STOP,count=100000)
  c=dict(returned=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,
   nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(NV,NV_SEEDS)),
   guards=bytes(u.mem_read(ESP-0x2000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4,32))==bytes([0xa7])*32,whitelist=not errors)
  if current.endswith('_init'):c['return_true']=u.reg_read(UC_X86_REG_EAX)==1;returns.append(1)
  assert all(c.values()),(kind,s,c,errors);checks.append(c)
 return dict(checks=checks,events=events,entries=entries,returns=returns,receiver=bytes(u.mem_read(FRAME,object_size)).hex(),
  complete_arena_sha256=hashlib.sha256(bytes(u.mem_read(DATA,0x10000))).hexdigest(),registry=bytes(u.mem_read(registry,72)).hex(),guard=read(guard),widgets=widgets)


def scenarios():
 result=[dict(name='sound_default'),dict(name='game_default',role='game_init')]
 for v in [0,1,2,128,255]:result.append(dict(name='sound_flag_'+str(v),cfg_sound=v,cfg_music=v))
 for v in [0,1,16383,0xffffffff]:result.append(dict(name='volume_'+hex(v),cfg_volume=v))
 for role,field in [('sound_toggle','sound'),('music_toggle','music')]:
  for v in [0,1,2,128,0x80000000,0xffffffff]:result.append(dict(name=role+'_'+hex(v),role=role,fields={field:v}))
 for difficulty in [0,1,2,255]:
  for lives in [2,3,4,5]:result.append(dict(name='game_values_'+str((difficulty,lives)),role='game_init',cfg_difficulty=difficulty,cfg_lives=lives))
 for v in [0,1,128,0x80000000,0xffffffff]:result.append(dict(name='guard_'+hex(v),role='game_init',guard=v))
 for v in [-1,0,1,2]:result.append(dict(name='difficulty_cycle_'+str(v),role='difficulty',fields=dict(difficulty=v)))
 for v in [1,2,3,4]:result.append(dict(name='lives_cycle_'+str(v),role='lives',fields=dict(lives=v)))
 for v in [0,1,16383,0x80000000,0xffffffff]:result.append(dict(name='sound_commit_'+hex(v),role='sound_ok',slider_return=v,fields=dict(sound=0x12345680,music=0x876543ff)))
 result.extend([
  dict(name='sound_roundtrip',steps=['sound_init','sound_toggle','music_toggle','sound_ok']),
  dict(name='game_roundtrip',steps=['game_init','difficulty','lives','game_ok']),
  dict(name='game_commit_raw_bits',role='game_ok',fields=dict(difficulty=0x12345680,lives=0xffffffff)),
  dict(name='base_init_false_ignored',actions={'BaseInit':[dict(result=0,mutate=dict(cfg_sound=0,cfg_music=255,cfg_volume=0))]}),
  dict(name='second_lookup_wrapper',actions={'GetDlgItem':[dict(second_lookup=True)]}),
  dict(name='sound_callbacks_change_later_config',actions={'SetWindowTextA':[{}, {},dict(mutate=dict(cfg_music=0,cfg_volume=0x1111))]}),
  dict(name='range_reloads_handle_and_volume',actions={'SendMessageA':[dict(handles=dict(slider=0x70000001),mutate=dict(cfg_volume=0x2222)),dict(handles=dict(slider=0x70000002)),dict(handles=dict(slider=0x70000003))]}),
  dict(name='save_order_before_getpos',role='sound_ok',actions={'SendMessageA':[dict(result=0x12345,mutate=dict(sound=0,music=127,cfg_sound=85,cfg_music=102))]}),
  dict(name='guard_live_each_call',role='game_init',guard=1,actions={'EnableWindow':[dict(mutate=dict(guard=0),handles=dict(lives_button=0x70000100)),dict(mutate=dict(guard=0x80000000))],'ShowWindow':[dict(mutate=dict(guard=0)),{}]}),
  dict(name='game_labels_change_later_registry',role='game_init',actions={'SetWindowTextA':[{}, {},dict(mutate=dict(cfg_lives=4)),dict(mutate=dict(guard=0))]}),
  dict(name='windows_api_failures_ignored',actions={'SetWindowTextA':[dict(result=0),dict(result=0)],'SendMessageA':[dict(result=0),dict(result=0),dict(result=0),dict(result=0)]}),
 ])
 return result


def main():
 b=build();cases=[]
 for s in scenarios():
  native=execute('native',b,s);source=execute('source',b,s);assert native==source,(s,native,source)
  cases.append(dict(scenario=s,native=native,source=source,equal=True))
 text=SOURCE.read_text();negative_sources={
  'xor_toggle':text.replace('m_sound_enabled = !m_sound_enabled;','m_sound_enabled ^= 1;')}
 start=text.index('BOOL CGamePlayPage::OnInitDialog()');end=text.index('\nvoid CGamePlayPage::OnRva_',start);part=text[start:end]
 part=part.replace('g_startup_apply_guard_rva_51c450','cached_menu_guard').replace('    m_control_0472.EnableWindow','    DWORD cached_menu_guard = g_startup_apply_guard_rva_51c450;\n    m_control_0472.EnableWindow')
 negative_sources['cached_guard']=text[:start]+part+text[end:];negative=[]
 for name,code in negative_sources.items():
  p=OUT/(name+'.cpp');code,count=re.subn(r'#include "[^"]*host_options_classes.h"','#include "'+HEADER.as_posix()+'"',code);assert count==1;p.write_bytes(code.encode());bad=build(p,OUT/name)
  s=next(x for x in scenarios()if x['name']==('sound_toggle_0x80'if name=='xor_toggle'else'guard_live_each_call'))
  native=execute('native',b,s);wrong=execute('source',bad,s);assert native!=wrong
  negative.append(dict(name=name,scenario=s,detected=True,native=native,wrong=wrong,build=bad))
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope='Finite synthetic sound/gameplay page callbacks. Actual MFC SetWindowText/EnableWindow/ShowWindow/slider SetRange/no-op OnOK bodies execute on ordinary HWND branches. Base CDialog initialization and GetDlgItem plus Windows endpoints are models. No real dialog,DDX,trackbar,device,process or gameplay execution accepted.',build=b,cases=cases,case_count=len(cases),all_equal=True,negative_controls=negative)
 p=OUT/'options_sound_gameplay_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negatives_detected=len(negative),report=str(p))))


if __name__=='__main__':main()
