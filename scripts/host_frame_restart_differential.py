"""Finite Restart Level/frame receiver tests; no real modal window or device."""
import argparse,hashlib,json,re,struct,subprocess,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from coff import COFF
from pe import PE
from match import compile_source,compare,DEFAULT_TOOLCHAIN
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to(ROOT/'work');OUT.mkdir(parents=True,exist_ok=True)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
DATA,FRAME,DLG,LOOKUP,FSROOT=0x61000000,0x61000100,0x61001000,0x61002000,0x61007000
STACK,ESP,STOP,MODEL=0x62000000,0x62008000,0x6200f000,0x65000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
SPECS=[('_g_exe_91c44c',0x51c44c,4),('_g_startup_apply_guard_rva_51c450',0x51c450,4),('_g_exe_96e3d0',0x56e3d0,1),('_g_fatal_suppress_dialog',0x7a020c,1),('_g_exe_ba0230',0x7a0230,4),('_g_registry_config',0x79e858,72),('_g_restart_level_rva_215c90',0x215c90,4)]
GAME_MODELS={'Sync':('_graphics_native_05a50',0x5a50,0),'Transition':('_cleanup_native_6bb0',0x6bb0,0),'Shutdown':('_fatal_shutdown_chain',0x5e90,0),'Options':('?OnRva_00009620@CMainFrame@@QAEXXZ',0x9620,0)}
LIB_CLEANUP={'BaseCtor':8,'BaseDtor':0,'Modal':0,'EndDialog':4,'BaseInit':0,'GetDlgItem':4,'SetText':4,'Enable':4,'Default':0,'FrameClose':0}

def build():
 command=[sys.executable,str(ROOT/'scripts/host_restart_dialog_audit.py'),'--output',str(OUT/'static')]
 result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stdout+result.stderr
 a=json.loads((OUT/'static/class_audit.json').read_text());graph=ROOT/'evidence/host_current_source_graph.json';g=json.loads(graph.read_text())
 paths=['calibration/display_mode_setter.c','calibration/host_restart_state.c','calibration/graphics_storage.cpp','calibration/fatal_storage.c','calibration/host_startup_storage.c','calibration/registry_storage.c','calibration/host_system_key_storage.c']
 watched={Path(__file__),graph,ORACLE,ROOT/'scripts/host_restart_dialog_audit.py',ROOT/'toolchains/manifest.json',Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll')}
 assert sha(next(p for p in watched if p.name=='unicorn.dll'))=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 compiles=[];game_boundaries=[]
 for path in paths:
  p=ROOT/path;watched.update(context_files(p));flags=next((x['flags']for x in g['sources']if x['source']==path),['/O2'])
  obj,cmd=compile_source(p,flags);compiles.append(dict(source=path,source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 for p,h in a['inputs'].items():assert sha(p)==h;watched.add(Path(p))
 recovery=json.loads((ROOT/'recovery.json').read_text())['functions'];watched.add(ROOT/'recovery.json')
 for role,(sym,rva,cleanup)in GAME_MODELS.items():
  row=next(x for x in recovery if x['symbol']==sym and x['target'].endswith('HERCULES.EXE')and int(x['rva'],0)==rva);p=ROOT/row['source'];watched.update(context_files(p));obj,cmd=compile_source(p,row['flags'])
  proof=compare(obj,sym,ORACLE,rva,row['size']);game_boundaries.append(dict(role=role,symbol=sym,source=row['source'],source_sha256=sha(p),rva=row['rva'],bytes=row['size'],existing_status=row['status'],raw=proof,command=cmd,execution='Modeled dependency boundary only;body not executed in this fixture'))
 for name in ['coff.py','pe.py','match.py','masked.py','env.py','title_behavioral.py']:watched.add(ROOT/'scripts'/name)
 pins={str(p):sha(p)for p in watched};mode=compiles[0];raw=compare(Path(mode['object']),'_cleanup_native_6ec0',ORACLE,0x6ec0,32)
 return dict(inputs=pins,static=a,static_command=command,compiles=compiles,mode_measurement=raw,game_boundaries=game_boundaries)

def prepare(b,replacement=None):
 a=b['static'];objects={x['short']:x['object']for x in a['compiles']};objects['mode.c']=b['compiles'][0]['object'];objects.update(replacement or{})
 code={name:COFF(path)for name,path in objects.items()};segments=[];globals={};section_addr={};cursor=0x20000000;definitions={};duplicate_proofs=[]
 for name,rva,n in SPECS:globals[name]=0x400000+rva
 data=[];providers=[COFF(x['object'])for x in b['compiles'][1:]];oracle=PE(ORACLE)
 for name,rva,n in SPECS:
  matches=[(co,s)for co in providers for s in co.symbols.values()if s['name']==name and(s['section']>0 or s['section']==0 and s['value']>0)];assert len(matches)==1,name
  co,s=matches[0]
  if s['section']==0:assert s['value']==n;raw=bytes(n)
  else:
   sec=co.sections[s['section']-1];assert not sec['relocations'];raw=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n);assert len(raw)==n
  native_section=next(x for x in oracle.pe.sections if x.VirtualAddress<=rva and rva+n<=x.VirtualAddress+x.Misc_VirtualSize);assert rva>=native_section.VirtualAddress+native_section.SizeOfRawData and raw==bytes(n)
  segments.append(dict(address=0x400000+rva,data=raw.hex()));data.append(dict(symbol=name,rva=hex(rva),bytes=n,source_initial_equals_native_virtual_zero=True))
 models={};model_symbols={}
 for role,spec in GAME_MODELS.items():
  address=MODEL+len(models)*32;models[address]=(role,spec[2]);model_symbols[spec[0]]=address
 for lib in a['library']:
  address=MODEL+len(models)*32;models[address]=(lib['role'],LIB_CLEANUP[lib['role']]);model_symbols[lib['symbol']]=address
 globals.update(model_symbols)
 for name,co in code.items():
  for i,sec in enumerate(co.sections,1):
   if not sec['size']or sec['name'].startswith('.debug')or sec['name']=='.drectve':continue
   section_addr[(name,i)]=cursor;cursor+=(sec['size']+15)&~15
  for s in co.symbols.values():
   if s['storage']!=2 or s['section']<=0 or(name,s['section'])not in section_addr:continue
   if s['name']in definitions:
    other,ss=definitions[s['name']];old=code[other].sections[ss['section']-1];new=co.sections[s['section']-1]
    assert ss['value']==s['value']and all(old[k]==new[k]for k in ['name','flags','size','data','relocations']),s['name']
    duplicate_proofs.append(s['name']);continue
   assert s['name']not in globals,s['name'];globals[s['name']]=section_addr[(name,s['section'])]+s['value'];definitions[s['name']]=(name,s)
 assert cursor<0x20100000;unsupported={};globals['__except_list']=0
 def resolve(name,symbol):
  if symbol in globals:return globals[symbol]
  co=code[name];candidates=[s for s in co.symbols.values()if s['name']==symbol]
  defined=[s for s in candidates if s['section']>0 and(name,s['section'])in section_addr]
  if defined:return section_addr[(name,defined[0]['section'])]+defined[0]['value']
  weak=[s for s in candidates if s['storage']==105]
  if weak:
   index,search=struct.unpack_from('<II',weak[0]['aux'][0]);assert search==2;return resolve(name,co.symbols[index]['name'])
  if symbol not in unsupported:unsupported[symbol]=MODEL+0x4000+len(unsupported)*16
  return unsupported[symbol]
 for name,co in code.items():
  for i,sec in enumerate(co.sections,1):
   if(name,i)not in section_addr:continue
   addr=section_addr[(name,i)];raw=bytearray(sec['data']or bytes(sec['size']));assert len(raw)==sec['size']
   for rel in sec['relocations']:
    off=rel['offset'];target=resolve(name,rel['symbol']);value=struct.unpack_from('<I',raw,off)[0];assert rel['type']in [6,20]
    value+=target-(addr+off+4 if rel['type']==20 else 0);struct.pack_into('<I',raw,off,value&0xffffffff)
   segments.append(dict(address=addr,data=raw.hex()))
 addresses={};widths={};native={};sym_by_role={}
 for row in a['measurements']:
  if row['role']=='compiler':continue
  role=row['role'];addresses[role]=globals[row['symbol']];widths[role]=code[row['source']].function(row['symbol'])['size'];native[role]=(int(row['rva'],0)+0x400000,row['size']);sym_by_role[role]=row['symbol']
 addresses['mode']=globals['_cleanup_native_6ec0'];widths['mode']=code['mode.c'].function('_cleanup_native_6ec0')['size'];native['mode']=(0x406ec0,32)
 handler=next(k for k,v in next(x for x in a['measurements']if x['role']=='open')['diagnostic']['symbols'].items()if v=='0x2d648')
 return dict(segments=segments,globals=globals,addresses=addresses,widths=widths,native=native,models=models,data=data,unsupported=unsupported,duplicates=duplicate_proofs,handler=resolve('frame.cpp',handler),vtable=globals['??_7CHostRestartDialog@@6B@'],message_map=globals['?messageMap@CHostRestartDialog@@1UAFX_MSGMAP@@B'])

def execute(native,b,p,s):
 image=pefile.PE(str(ORACLE));size=image.OPTIONAL_HEADER.SizeOfImage;u=Uc(UC_ARCH_X86,UC_MODE_32)
 u.mem_map(0,0x1000);u.mem_map(0x400000,(size+4095)&~4095)
 if native:u.mem_write(0x400000,image.get_memory_mapped_image().ljust(size,b'\0'))
 else:u.mem_map(0x20000000,0x100000)
 for addr in [DATA,STACK,MODEL]:u.mem_map(addr,0x10000)
 if not native:
  for part in p['segments']:u.mem_write(part['address'],bytes.fromhex(part['data']))
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def p32(a,v):u.mem_write(a,pack(v))
 def cstring(a):
  result=bytearray()
  for i in range(128):
   v=bytes(u.mem_read(a+i,1))[0]
   if not v:return result.decode('latin1')
   result.append(v)
  raise AssertionError('unbounded string')
 fields={'hot':(0x91c44c,4),'guard':(0x91c450,4),'requested':(0x96e3d0,1),'applied':(0xba020c,1),'depth':(0xba0230,4),'level':(0x615c90,4)};cfg=0xb9e858
 u.mem_write(DATA,bytes((i*17+(i>>8)*7+31)&255 for i in range(0x10000)));u.mem_write(FRAME,bytes(188));u.mem_write(DLG,bytes(92))
 p32(FRAME+0x1c,0x12345678);p32(DLG+0x1c,0x22334455);p32(DLG,0x42ea80 if native else p['vtable']);p32(0,FSROOT)
 for name,(addr,n)in fields.items():u.mem_write(addr,pack(s.get(name,1 if name in ['guard','applied']else 0))[:n])
 u.mem_write(cfg,bytes(72));p32(cfg+12,s.get('difficulty',0));p32(cfg+24,s.get('lives',3))
 for k,v in s.get('progress',{0:s.get('unlocked',11),1:s.get('unlocked',11),2:s.get('unlocked',11)}).items():assert 0<=int(k)<=67;u.mem_write(cfg+4+int(k),bytes([v]))
 addresses={k:int(v[0])if native else p['addresses'][k]for k,v in p['native'].items()};widths={k:int(v[1])if native else p['widths'][k]for k,v in p['native'].items()}
 endpoints={}
 if native:
  for role,(sym,rva,n)in GAME_MODELS.items():endpoints[0x400000+rva]=(role,n)
  for lib in b['static']['library']:endpoints[int(lib['rva'],0)+0x400000]=(lib['role'],LIB_CLEANUP[lib['role']])
 else:endpoints={int(k):tuple(v)for k,v in p['models'].items()}
 ranges=[(addresses[k],addresses[k]+widths[k])for k in addresses]
 events=[];entries=[];errors=[];counts={};applied=[];controls={};lookups={};root_role=None
 def state():return {name:int.from_bytes(u.mem_read(addr,n),'little')for name,(addr,n)in fields.items()}
 def eh():
  top=read(0)
  if top==FSROOT:return None
  assert ESP-0x3000<=top and top+12<=ESP
  assert read(top)==FSROOT and read(top+4)==(0x42d648 if native else p['handler'])
  return dict(previous='fixture_root',handler='frame_restart_handler',state=read(top+8))
 def obj(addr):
  data=bytearray(u.mem_read(addr,92));vt=read(addr)
  if vt==(0x42ea80 if native else p['vtable']):data[:4]=bytes(4);identity='restart_vtable'
  else:identity=hex(vt)
  return dict(vtable=identity,remaining_bytes=data.hex())
 def mutate(action):
  for name,(addr,n)in fields.items():
   if name in action:u.mem_write(addr,pack(action[name])[:n])
  if 'difficulty'in action:p32(cfg+12,action['difficulty'])
  if 'lives'in action:p32(cfg+24,action['lives'])
  for k,v in action.get('progress',{}).items():assert 0<=int(k)<=67;u.mem_write(cfg+4+int(k),bytes([v]))
 def ret(sp,n,result):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4);u.reg_write(UC_X86_REG_ESP,sp+4+n);u.reg_write(UC_X86_REG_EIP,target)
 def on_code(machine,a,n,_):
  if a==STOP:machine.emu_stop();return
  sp=machine.reg_read(UC_X86_REG_ESP)
  if a in addresses.values():
   role=next(k for k,v in addresses.items()if v==a)
   if role in ['mode','selector']:entries.append(dict(call=role,args=[read(sp+4)&0xffff if role=='mode'else read(sp+4)],before=state()))
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];argv=[read(sp+4+i*4)for i in range(cleanup//4)];this=machine.reg_read(UC_X86_REG_ECX);receiver=None;id=None;result=0x12345678;detail=None
  if name in ['Options','Default','FrameClose']:assert this==FRAME;receiver='frame'
  elif name in ['BaseCtor','BaseDtor','Modal','EndDialog','BaseInit']:receiver='dialog';assert this==DLG or ESP-0x3000<=this and this+92<=ESP
  elif name=='GetDlgItem':assert this==DLG;receiver='dialog';id=argv[0]
  elif name in ['SetText','Enable']:
   if this==DLG:receiver='dialog'
   else:assert this in lookups;id=lookups[this];receiver='control:'+str(id)
  if name=='BaseCtor':
   assert argv[0]==0xb5;u.mem_write(this,bytes(92));result=this;detail=dict(eh=eh())
  elif name in ['BaseDtor','Modal']:detail=dict(object=obj(this),eh=eh())
  if name=='Modal':result=s.get('modal_result',0)
  elif name=='BaseInit':result=s.get('base_result',1)
  elif name=='EndDialog':assert argv==[0]
  elif name=='GetDlgItem':
   assert id in [2,*range(0x46a,0x474)];slot=[2,*range(0x46a,0x474)].index(id);result=LOOKUP+slot*0x100;lookups[result]=id
  elif name=='SetText':
   argv=[cstring(argv[0])];controls.setdefault(receiver,{})['text']=argv[0];result=s.get('text_result',1)
  elif name=='Enable':controls.setdefault(receiver,{})['enabled_bits']=argv[0];result=s.get('enable_result',1)
  if name=='Transition'and s.get('transition_applies',True):u.mem_write(fields['applied'][0],bytes(u.mem_read(fields['requested'][0],1)))
  event=dict(call=name,receiver=receiver,id=id,args=argv,before=state(),detail=detail);events.append(event)
  key=(name,id);index=counts.get(key,0);counts[key]=index+1
  for i,action in enumerate(s.get('actions',[])):
   if i in applied or action['call']!=name or action.get('index',0)!=index or'id'in action and action['id']!=id:continue
   mutate(action);applied.append(i)
  ret(sp,cleanup,result)
 def on_write(machine,access,a,n,value,_):
  allowed=[(ESP-0x3000,ESP+16),(0,4),(fields['requested'][0],fields['requested'][0]+1),(fields['hot'][0],fields['hot'][0]+4),(fields['depth'][0],fields['depth'][0]+4),(fields['level'][0],fields['level'][0]+4)]
  if root_role=='constructor':allowed.append((DLG,DLG+4))
  if not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(['write',hex(a),n,root_role]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);returns=[]
 for root_role in s.get('steps',[s.get('role','selector')]):
  argv=[s.get('request',-1)]if root_role=='selector'else[s.get('value',0)]if root_role=='mode'else[s.get('key',0x71),2,0x12345678]if root_role=='keys'else[s.get('parent',0)]if root_role=='constructor'else[0xdeadcafe]if root_role=='ddx'else[]
  u.mem_write(ESP-0x3000,bytes([0xa7])*0x3100);p32(ESP,STOP)
  for i,v in enumerate(argv):p32(ESP+4+i*4,v)
  u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME if root_role in ['open','close','toggle','keys']else DLG)
  for reg,v in zip(NV,SEEDS):u.reg_write(reg,v)
  u.emu_start(addresses[root_role],STOP,count=100000)
  cleanup=0 if root_role in ['selector','mode']else 4*len(argv)
  assert not errors and u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4+cleanup,errors
  assert[u.reg_read(reg)for reg in NV]==SEEDS and read(0)==FSROOT
  assert bytes(u.mem_read(ESP-0x3000-32,32))==bytes(32)and bytes(u.mem_read(ESP+16,64))==bytes([0xa7])*64
  if root_role in ['selector','mode','constructor','init','getmap']:
   result=u.reg_read(UC_X86_REG_EAX)
   if root_role=='mode':result&=0xffff
   if root_role=='getmap':assert result==(0x42e970 if native else p['message_map']);result='restart_message_map'
   returns.append([root_role,result])
 assert len(applied)==len(s.get('actions',[])),('unused actions',s,applied)
 arena=bytearray(u.mem_read(DATA,0x10000));arena[DLG-DATA:DLG-DATA+4]=bytes(4)
 return dict(events=events,entries=entries,returns=returns,final=state(),config=bytes(u.mem_read(cfg,72)).hex(),dialog=obj(DLG),arena_sha256=hashlib.sha256(arena).hexdigest(),controls=controls,fs_stack_nonvolatiles_guards=True)

def scenarios():
 ss=[]
 for request in [-3,-2,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,255,0x7fffffff,0x80000000]:
  for unlocked in [0,1,11,255]:ss.append(dict(name='select_%x_%x'%(request&0xffffffff,unlocked),request=request,unlocked=unlocked,level=0x1234))
 for level in [0,1,0x7fffffff,0x80000000,0xffffffff]:
  for lives in [0,3,0xffffffff]:ss.append(dict(name='packed_%x_%x'%(level,lives),request=-2,level=level,lives=lives,difficulty=0xdead0001))
 for diff in [0,1,2,0x80000001,63]:
  for progress in [0,1,3,4,5,10,11,255]:ss.append(dict(name='init_%x_%x'%(diff,progress),role='init',difficulty=diff,progress={diff&255:progress},base_result=0,text_result=0,enable_result=0))
 for i in range(10):
  for unlocked in [0,11]:ss.append(dict(name='handler_%d_%d'%(i,unlocked),role='level_'+str(i),unlocked=unlocked))
 for hot in [0,1,2,0xffffffff,0x80000000]:
  for key in [0x71,0x72,0x73,0x10000071]:
   for guard in [0,1,0x80000000]:
    for mode in [0,1,255]:ss.append(dict(name='key_%x_%x_%x_%x'%(hot,key,guard,mode),role='keys',hot=hot,key=key,guard=guard,applied=mode))
 for hot in [0,1,2,0xffffffff,0x80000000]:ss.append(dict(name='close_%x'%hot,role='close',hot=hot))
 for mode in [0,1,128,255]:ss.append(dict(name='toggle_%x'%mode,role='toggle',applied=mode))
 ss.extend([dict(name='constructor',role='constructor'),dict(name='constructor_parent',role='constructor',parent=FRAME),dict(name='getmap',role='getmap'),dict(name='ddx',role='ddx'),dict(name='open',role='open'),dict(name='open_wrap_depth',role='open',depth=0xffffffff),dict(name='dialog_flow',steps=['constructor','init','level_9']),dict(name='modal_return_cancel',role='open',modal_result=2)])
 ss.extend([
  dict(name='live_progress_enable',role='init',unlocked=11,actions=[dict(call='Enable',id=0x46a,progress={'0':0})]),
  dict(name='live_progress_lookup',role='init',unlocked=11,actions=[dict(call='GetDlgItem',id=0x46a,index=1,progress={'0':0})]),
  dict(name='live_difficulty',role='init',progress={'0':11,'1':0},actions=[dict(call='Enable',id=0x46a,difficulty=0xdead0001)]),
  dict(name='live_modal_depth',role='open',depth=3,actions=[dict(call='Sync',depth=7),dict(call='Modal',depth=0x12345678)]),
  dict(name='key_modal_state',role='keys',key=0x72,actions=[dict(call='Transition',hot=1),dict(call='Modal',hot=0xffffffff,applied=255)]),
  dict(name='key_options_restore',role='keys',applied=1,actions=[dict(call='Options',applied=255,hot=0xffffffff)]),
  dict(name='close_order',role='close',actions=[dict(call='Shutdown',hot=7)]),
  dict(name='no_transition_apply',role='keys',key=0x72,applied=128,transition_applies=False)])
 return ss

def main():
 b=build();p=prepare(b);cases=[];ss=scenarios()
 for i,s in enumerate(ss):
  native=execute(True,b,p,s);source=execute(False,b,p,s)
  if native!=source:
   (OUT/'difference.json').write_bytes((json.dumps(dict(scenario=s,native=native,source=source),indent=2)+'\n').encode());raise AssertionError(('scenario',s['name'],[k for k in native if native[k]!=source[k]]))
  cases.append(dict(scenario=s,equal=True,projection=native))
  if i and i%100==0:print('Verified',i,'frame/restart scenarios',flush=True)
 sources={'frame.cpp':ROOT/'src/pc/host_frame_remaining.cpp','restart.cpp':ROOT/'src/pc/host_restart_dialog.cpp','selector.c':ROOT/'src/pc/host_restart_selector.c'}
 specs=[('unguarded_F3','frame.cpp','case VK_F3: if (g_startup_apply_guard_rva_51c450 != 0)','case VK_F3: if (TRUE)','key_0_72_0_1'),('lost_mode_restore','frame.cpp','cleanup_native_6ec0(previous);','cleanup_native_6ec0(0);','key_options_restore'),('unsigned_level_bound','selector.c','level > g_registry_config','(DWORD)level > g_registry_config','select_fffffffd_b'),('cached_progress','restart.cpp','#define ENABLE_LEVEL(id,level) GetDlgItem(id)->EnableWindow(g_registry_config[4+(*(DWORD*)(g_registry_config+12)&0xff)] >= level);','BYTE unlocked=g_registry_config[4+(*(DWORD*)(g_registry_config+12)&0xff)];\n#define ENABLE_LEVEL(id,level) GetDlgItem(id)->EnableWindow(unlocked >= level);','live_progress_enable')]
 negatives=[]
 for name,short,old,new,case in specs:
  original=sources[short];text=original.read_text();assert old in text;text=text.replace(old,new)
  text=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(original.parent/m[1]).resolve().as_posix()+'"',text)
  lane=OUT/name;lane.mkdir(exist_ok=True);path=lane/short;path.write_bytes(text.encode());flags=['/O2']if short.endswith('.c')else['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]
  obj,cmd=compile_source(path,flags);plan=prepare(b,{short:str(obj)});s=next(x for x in ss if x['name']==case);native=execute(True,b,p,s);wrong=execute(False,b,plan,s);assert native!=wrong,name
  negatives.append(dict(name=name,detected=True,scenario=s,native=native,wrong=wrong,source_sha256=sha(path),object_sha256=sha(obj),command=cmd))
 assert b['inputs']=={path:sha(path)for path in b['inputs']}
 report=dict(scope='Finite synthetic normal-flow Restart Level/frame/selector comparison. Actual new source/native functions and actual mode setter execute. Ten identified MFC bodies and four existing game dependencies are modeled at entry;no real modal/control/window/display/process behavior or exception dispatch/unwind is accepted. Mode arguments/returns compare defined WORD bits only. Conventional COFF fixture placement is not natural layout/link proof.',build=b,data_prefixes=p['data'],normal_comdat_duplicates_verified=p['duplicates'],unsupported_execution_accepted=False,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives)
 path=OUT/'frame_restart_differential.json';path.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negatives=len(negatives),report=str(path))))

if __name__=='__main__':main()
