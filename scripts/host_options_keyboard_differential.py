"""Original PC keyboard page comparison on synthetic receivers, without Windows UI."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
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
parser.add_argument('--source',type=Path,default=ROOT/'src/pc/host_options_keyboard.cpp')
parser.add_argument('--header',type=Path,default=ROOT/'src/pc/host_options_classes.h')
parser.add_argument('--state',type=Path,default=ROOT/'src/pc/host_keyboard_state.c')
args=parser.parse_args();OUT=args.output.resolve();SOURCE=args.source.resolve();HEADER=args.header.resolve();STATE=args.state.resolve()
assert OUT.is_relative_to(ROOT/'work');OUT.mkdir(parents=True,exist_ok=True)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
PROFILE={
 'bind':('?OnRva_00008DD0@CKeyboardPage@@QAEHPAUHWND__@@FH@Z',0x8dd0,608),
 'init':('?OnInitDialog@CKeyboardPage@@UAEHXZ',0x9030,512),
 'ok':('?OnOK@CKeyboardPage@@UAEXXZ',0x9230,144),
 'toggle':('?OnRva_000092C0@CKeyboardPage@@QAEXXZ',0x92c0,48),
 'pretranslate':('?PreTranslateMessage@CKeyboardPage@@UAEHPAUtagMSG@@@Z',0x92f0,64),
 'reset':('?OnReset@CKeyboardPage@@UAEXXZ',0x9330,224),
 'reset_button':('??_9@$BNM@AE',0x9410,8)}
MFC={
 'BaseInit':('?OnInitDialog@CDialog@@UAEHXZ',0x220bd,101,124),
 'GetDlgItem':('?GetDlgItem@CWnd@@QBEPAV1@H@Z',0x24faa,42,104),
 'SetText':('?SetWindowTextA@CWnd@@QAEXPBD@Z',0x2509d,39,104),
 'BaseOK':('?OnOK@CPropertyPage@@UAEXXZ',0x28ac0,1,121),
 'BasePre':('?PreTranslateMessage@CPropertyPage@@MAEHPAUtagMSG@@@Z',0x28c70,14,121),
 'WndPre':('?PreTranslateMessage@CWnd@@UAEHPAUtagMSG@@@Z',0x22fd3,31,148)}
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
DATA,FRAME,LOOKUP,MSG,VT=0x61000000,0x61000100,0x61002000,0x61004000,0x61003000
STACK,ESP,STOP,API=0x62000000,0x62008000,0x6200f000,0x65000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
KEYS=[('up',0x104,2),('down',0x140,8),('left',0x17c,0),('right',0x1b8,3),('sword',0x1f4,6),('jump',0x230,1),('punch',0x26c,7),('power',0x2a8,5),('weapon',0x8c,4)]

def build(source,state,out,accept=True):
 out.mkdir(parents=True,exist_ok=True);verify_toolchain(DEFAULT_TOOLCHAIN)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 graph=ROOT/'evidence/host_current_source_graph.json';libs=json.loads(graph.read_text())['libraries']
 assert Path(libs[0]['path']).name=='nafxcw.lib'and libs[0]['sha256']=='5d9c3585552412a9b94a9a0b7c3a4208061a76d48a6c3283518bfe9f674c9e22'
 assert all(sha(x['path'])==x['sha256']for x in libs)
 runtime=Path('C:/tools/hercules/renderer-oracle-python/unicorn/lib/unicorn.dll')
 assert sha(runtime)=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
 files=[source,state,ROOT/'src/shared/registry_storage.c',ROOT/'src/pc/host_startup_storage.c']
 watched={Path(__file__),graph,HEADER,ORACLE,runtime,ROOT/'toolchains/manifest.json',*[Path(x['path'])for x in libs]}
 for p in files:watched.update(context_files(p))
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 pins={str(p):sha(p)for p in watched}
 anchor=out/'anchor.cpp'
 anchor.write_bytes(('#include "'+HEADER.as_posix()+'"\nextern "C" int main(){for(;;){}}\nextern "C" __declspec(dllexport) AFX_PMSG reset_handler=(AFX_PMSG)&CKeyboardPage::OnReset;\nextern "C" __declspec(dllexport) void anchor(CKeyboardPage*p,MSG*m,HWND w){p->CKeyboardPage::OnInitDialog();p->CKeyboardPage::OnOK();p->CKeyboardPage::OnReset();p->CKeyboardPage::PreTranslateMessage(m);p->OnRva_000092C0();p->OnRva_00008DD0(w,65,1);}\n').encode())
 objects=[];compiles=[]
 for p in [*files,anchor]:
  flags=['/O2','/MT','/GX','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]if p in [source,anchor]else['/O2']
  obj,cmd=compile_source(p,flags);objects.append(obj);compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
 measurements=[]
 for role,(sym,rva,n)in PROFILE.items():
  obj=objects[-1]if role=='reset_button'else objects[0];raw=compare(obj,sym,ORACLE,rva,n);d=masked_compare(obj,sym,ORACLE,rva,n)
  if accept:
   assert raw['compiled_size']==n
   if role=='bind':assert d['differing_bytes']==3 and d['mismatch_regions']==[[529,530],[533,534],[539,540]]and not d['conflicts']
   else:assert d['masked_equal'],(role,d)
  measurements.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,raw=raw,diagnostic=d))
 dll,mp=out/'fixture.dll',out/'fixture.map'
 cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),*[x['path']for x in libs]]
 env=os.environ.copy()
 for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
 env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
 proc=subprocess.run(cmd,env=env,capture_output=True,text=True);(out/'link.log').write_bytes((proc.stdout+proc.stderr).encode());assert proc.returncode==0,proc.stdout+proc.stderr
 regex=re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
 symbols={m[1]:int(m[2],16)for line in mp.read_text().splitlines()if(m:=regex.match(line))}
 linked=pefile.PE(str(dll));oracle=PE(ORACLE)
 def linked_body(co,name):
  s=next(x for x in co.symbols.values()if x['name']==name and x['section']>0);sec=co.sections[s['section']-1];assert s['value']==0
  data=bytearray(linked.get_data(symbols[name]-linked.OPTIONAL_HEADER.ImageBase,sec['size']));expected=bytearray(sec['data'])
  for rel in sec['relocations']:data[rel['offset']:rel['offset']+4]=bytes(4);expected[rel['offset']:rel['offset']+4]=bytes(4)
  assert data==expected
  return s,sec
 for role,(sym,rva,n)in PROFILE.items():linked_body(COFF(objects[-1]if role=='reset_button'else objects[0]),sym)
 bindings={};literals=[]
 if accept:
  callback=COFF(objects[0])
  for row in measurements[:-1]:
   for name,rva in row['diagnostic']['symbols'].items():
    assert name not in bindings or bindings[name]==rva,(name,bindings.get(name),rva);bindings[name]=rva
    if name.startswith('??_C@'):
     s=next(x for x in callback.symbols.values()if x['name']==name and x['section']>0);sec=callback.sections[s['section']-1];raw=sec['data'][s['value']:];n=raw.index(0)+1
     assert oracle.read_rva(int(rva,0),n)==raw[:n]
     assert linked.get_data(symbols[name]-linked.OPTIONAL_HEADER.ImageBase,n)==raw[:n]
     literals.append(dict(symbol=name,rva=rva,bytes=n,text=raw[:n-1].decode()))
 state_co=COFF(objects[1]);s=next(x for x in state_co.symbols.values()if x['name']=='_g_keyboard_labels_rva_36cf0'and x['section']>0);sec=state_co.sections[s['section']-1]
 assert s['value']==0 and sec['size']==1024 and len(sec['relocations'])==89
 rels={x['offset']:x for x in sec['relocations']};rows=[];counts={'zero':0,'invalid':0,'caption':0}
 for i in range(256):
  original=struct.unpack('<I',oracle.read_rva(0x36cf0+i*4,4))[0];value=struct.unpack_from('<I',sec['data'],i*4)[0]
  linked_pointer=struct.unpack('<I',linked.get_data(symbols['_g_keyboard_labels_rva_36cf0']-linked.OPTIONAL_HEADER.ImageBase+i*4,4))[0]
  if i*4 not in rels:
   assert original in [0,0xffffffff]and value==original and linked_pointer==original;kind='zero'if original==0 else'invalid';rows.append(dict(index=i,kind=kind));counts[kind]+=1
  else:
   rel=rels[i*4];assert rel['type']==6 and value==0 and original not in [0,0xffffffff]
   ss=next(x for x in state_co.symbols.values()if x['name']==rel['symbol']and x['section']>0);raw=state_co.sections[ss['section']-1]['data'][ss['value']:];n=raw.index(0)+1
   expected=oracle.read_rva(original-0x400000,n)
   if accept:assert raw[:n]==expected and linked.get_data(linked_pointer-linked.OPTIONAL_HEADER.ImageBase,n)==expected
   rows.append(dict(index=i,kind='caption',symbol=rel['symbol'],native_rva=hex(original-0x400000),bytes=n,text=expected[:-1].decode()));counts['caption']+=1
 assert counts==dict(zero=148,invalid=19,caption=89)
 working=next(x for x in state_co.symbols.values()if x['name']=='_g_keyboard_working_rva_56e5f4')
 assert working['section']==0 and working['value']==36 or working['section']>0 and state_co.sections[working['section']-1]['size']==36
 native_section=next(x for x in oracle.pe.sections if x.VirtualAddress<=0x56e5f4<x.VirtualAddress+x.Misc_VirtualSize)
 assert 0x56e5f4>=native_section.VirtualAddress+native_section.SizeOfRawData
 working_rva=symbols['_g_keyboard_working_rva_56e5f4']-linked.OPTIONAL_HEADER.ImageBase
 assert linked.get_memory_mapped_image().ljust(linked.OPTIONAL_HEADER.SizeOfImage,b'\0')[working_rva:working_rva+36]==bytes(36)
 archive=Path(libs[0]['path']).read_bytes();pos,index=8,0;proofs=[]
 while pos<len(archive):
  h=archive[pos:pos+60];assert h[58:]==b'`\n';n=int(h[48:58])
  for role,(sym,rva,width,member)in MFC.items():
   if index!=member:continue
   obj=out/('mfc_'+str(index)+'.obj');obj.write_bytes(archive[pos+60:pos+60+n]);co=COFF(obj);ss,sec=linked_body(co,sym)
   assert sec['size']==width and len(sec['data'])==width
   d=_compare_section(co,sec,ss['section'],oracle,rva,width);assert d['masked_equal'],(role,d)
   proofs.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=width,member=member,member_sha256=sha(obj),diagnostic=d,linked_code_diagnostic_equal=True,execution='actual unchanged body'if role in ['SetText','BaseOK','BasePre']else'modeled identified boundary'))
  pos+=60+n+(n&1);index+=1
 assert len(proofs)==6 and pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,key_table=dict(rva='0x36cf0',bytes=1024,source_pointer_fields=89,counts=counts,entries=rows),working=dict(rva='0x56e5f4',bytes=36,backing='virtual_zero',measured_slots=9),literals=literals,mfc=proofs,libraries=libs,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols)

def execute(native,b,s):
 image=pefile.PE(str(ORACLE if native else b['dll']));base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
 for addr in [DATA,STACK]:u.mem_map(addr,0x10000)
 u.mem_map(API,0x1000)
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def p32(a,v):u.mem_write(a,pack(v))
 def string(a):return bytes(u.mem_read(a,160)).split(b'\0')[0].decode('latin1')
 config=0xb9e858 if native else b['symbols']['_g_registry_config']
 working=0x96e5f4 if native else b['symbols']['_g_keyboard_working_rva_56e5f4']
 labels=0x436cf0 if native else b['symbols']['_g_keyboard_labels_rva_36cf0']
 defaults=0x436158 if native else b['symbols']['_g_config_default_values_rva_36158']
 u.mem_write(DATA,bytes((i*17+(i>>8)*7+31)&255 for i in range(0x10000)))
 receivers={FRAME:'page',LOOKUP:'lookup',FRAME+0xc8:'enabled',**{FRAME+o:name for name,o,index in KEYS}}
 for i,a in enumerate(receivers):p32(a+0x1c,0x12340000+i*0x100);p32(a+0x38,0)
 handle=lambda name:read(next(a for a,n in receivers.items()if n==name)+0x1c)
 cfg=bytearray((i*19+13)&255 for i in range(72));cfg[20]=s.get('enabled_byte',1)
 default_values=[0x26,0x28,0x25,0x27,0x58,0x5a,0x43,0x44,0x53]
 for i,v in enumerate(s.get('config_keys',default_values)):cfg[28+2*i:30+2*i]=pack(v)[:2]
 u.mem_write(config,bytes(cfg));p32(FRAME+0x2e4,s.get('enabled',1))
 for i,v in enumerate(s.get('working',[0xa5000040+i*0x10001 for i in range(9)])):p32(working+4*i,v)
 for i,v in enumerate(s.get('defaults',default_values)):p32(defaults+4*i,v)
 for name,v in s.get('handles',{}).items():p32(next(a for a,n in receivers.items()if n==name)+0x1c,v)
 addresses={role:0x400000+rva if native else b['symbols'][sym]for role,(sym,rva,n)in PROFILE.items()}
 p32(FRAME,VT);p32(VT+0xdc,addresses['reset'])
 p32(MSG,handle(s.get('target','up'))if s.get('target','up')!='unknown'else 0x76543210)
 p32(MSG+4,s.get('message',0x100));p32(MSG+8,s.get('key',65));p32(MSG+12,0x87654321)
 ranges=[(addresses[role],addresses[role]+next(x['raw']['compiled_size']for x in b['measurements']if x['role']==role))for role in PROFILE]
 endpoints={}
 for role,(sym,rva,n,index)in MFC.items():
  addr=0x400000+rva if native else b['symbols'][sym]
  if role in ['BaseInit','GetDlgItem','WndPre']:endpoints[addr]=(role,0 if role=='BaseInit'else 4)
  else:ranges.append((addr,addr+n))
 for i,(name,cleanup)in enumerate([('SetWindowTextA',8),('MapVirtualKeyA',8)]):
  addr=API+i*16;endpoints[addr]=(name,cleanup)
  for d in image.DIRECTORY_ENTRY_IMPORT:
   for imp in d.imports:
    if imp.name and imp.name.decode()==name:p32(imp.address,addr)
 events=[];entries=[];errors=[];counts={};applied=[]
 def mutate(action):
  for k,v in action.get('working',{}).items():p32(working+4*int(k),v)
  for k,v in action.get('cfg_keys',{}).items():u.mem_write(config+28+2*int(k),pack(v)[:2])
  if 'enabled_byte'in action:u.mem_write(config+20,bytes([action['enabled_byte']]))
  if 'enabled'in action:p32(FRAME+0x2e4,action['enabled'])
  for k,v in action.get('defaults',{}).items():p32(defaults+4*int(k),v)
  for k,v in action.get('handles',{}).items():p32(next(a for a,n in receivers.items()if n==k)+0x1c,v)
  for k,v in action.get('labels',{}).items():
   addr=DATA+0x8000+int(k)*32;u.mem_write(addr,v.encode()+b'\0');p32(labels+4*int(k),addr)
 def ret(sp,n,value):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4)
  u.reg_write(UC_X86_REG_ESP,sp+4+n);u.reg_write(UC_X86_REG_EIP,target)
 def projection():return dict(working=[read(working+4*i)for i in range(9)],config=bytes(u.mem_read(config,72)).hex(),enabled=read(FRAME+0x2e4))
 def code(machine,a,n,_):
  if a==STOP:machine.emu_stop();return
  if a==addresses['bind']:
   sp=machine.reg_read(UC_X86_REG_ESP);assert machine.reg_read(UC_X86_REG_ECX)==FRAME
   entries.append(['bind',read(sp+4),read(sp+8)&0xffff,read(sp+12)])
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];sp=machine.reg_read(UC_X86_REG_ESP);argv=[read(sp+4+i*4)for i in range(cleanup//4)];value=1;event=argv[:]
  if name=='BaseInit':assert machine.reg_read(UC_X86_REG_ECX)==FRAME;value=s.get('base_result',1)
  elif name=='GetDlgItem':
   assert machine.reg_read(UC_X86_REG_ECX)==FRAME and argv[0]in [0x44b,*range(0x472,0x47b)];value=LOOKUP
  elif name=='WndPre':assert machine.reg_read(UC_X86_REG_ECX)==FRAME and argv==[MSG];value=s.get('wnd_pre_result',1);event=[dict(message=read(MSG+4),wparam=read(MSG+8),hwnd=read(MSG))]
  elif name=='SetWindowTextA':event=[argv[0],string(argv[1])];value=s.get('api_result',1)
  elif name=='MapVirtualKeyA':assert argv[1]==2 and argv[0]<=255;value=s.get('mapped',0x41)
  key=(name,event[1]if name=='SetWindowTextA'else'');index=counts.get(key,0);counts[key]=index+1
  events.append(dict(call=name,args=event,before=projection()))
  for i,rule in enumerate(s.get('actions',[])):
   if i not in applied and rule['call']==name and rule.get('index',0)==index and('text'not in rule or name=='SetWindowTextA'and event[1]==rule['text']):mutate(rule);applied.append(i)
  ret(sp,cleanup,value)
 current=None
 def write(machine,access,a,n,value,_):
  allowed=[(ESP-0x2000,ESP+16),(FRAME+0x2e4,FRAME+0x2e8)]
  if current=='ok':allowed.extend([(config+20,config+21),(config+28,config+46)])
  else:allowed.extend((working+4*i,working+4*i+2)for i in range(9))
  if not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(['write',hex(a),n,current]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
 returns=[]
 for current in s.get('steps',[s.get('role','bind')]):
  argv=[MSG]if current=='pretranslate'else[read(MSG),s.get('key',65)&0xffffffff,s.get('clear',1)]if current=='bind'else[]
  u.mem_write(ESP-0x2000,bytes([0xa7])*0x2100);p32(ESP,STOP)
  for i,v in enumerate(argv):p32(ESP+4+4*i,v)
  u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
  for reg,v in zip(NV,SEEDS):u.reg_write(reg,v)
  u.emu_start(addresses[current],STOP,count=100000)
  assert not errors and u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4+4*len(argv),errors
  assert[u.reg_read(x)for x in NV]==SEEDS
  assert bytes(u.mem_read(ESP-0x2000-32,32))==bytes(32)and bytes(u.mem_read(ESP+16,64))==bytes([0xa7])*64
  if current in ['bind','pretranslate','init']:returns.append([current,u.reg_read(UC_X86_REG_EAX)])
 assert len(applied)==len(s.get('actions',[])),('unused actions',s,applied)
 arena=bytearray(u.mem_read(DATA,0x10000));arena[VT-DATA+0xdc:VT-DATA+0xe0]=bytes(4)
 table=[]
 for i in range(256):
  value=read(labels+4*i);table.append(value if value in [0,0xffffffff]else string(value))
 return dict(events=events,entries=entries,returns=returns,final=projection(),table=table,defaults=[read(defaults+4*i)for i in range(9)],arena_sha256=hashlib.sha256(arena).hexdigest(),full_receiver_bytes=744,stack_nonvolatiles_guards=True)

def scenarios():
 out=[dict(name='vk_%02x'%i,key=i)for i in range(256)]
 for name,offset,index in KEYS:out.append(dict(name='target_'+name,target=name,key=65))
 out.extend([dict(name='unknown_window',target='unknown'),dict(name='no_conflict_clear',key=65,clear=0,working=[0xa5000041+i*0x10000 for i in range(9)]),dict(name='last_conflict',key=65,working=[0xa5000041+i*0x10000 for i in range(9)])])
 for key in [0x12340041,0xffff0041,0xffffffff,0xffffff80]:out.append(dict(name='truncate_%08x'%key,key=key))
 for value in [0,1,0x7f,0x80,0xff,0x100,0x180,0xdead0041,0x80000000]:out.append(dict(name='map_%08x'%value,key=0x07,mapped=value))
 for value in [0,1,2,128,0x80000000,0xffffffff]:out.append(dict(name='toggle_%08x'%value,role='toggle',enabled=value))
 for msg in [0x100,0x101,0x104,0x105,0x200]:out.append(dict(name='pre_%03x'%msg,role='pretranslate',message=msg,key=0x41))
 out.extend([dict(name='pre_invalid',role='pretranslate',key=0x1b),dict(name='pre_unknown',role='pretranslate',target='unknown'),dict(name='pre_high_wparam',role='pretranslate',key=0xdead0041),dict(name='pre_base_ignored',role='pretranslate',message=0x101,wnd_pre_result=0xffffffff)])
 for value in [0,1,128,255]:out.append(dict(name='init_%02x'%value,role='init',enabled_byte=value,base_result=0,api_result=0))
 out.extend([dict(name='save_raw',role='ok',enabled=0x12345680),dict(name='reset',role='reset'),dict(name='reset_virtual_button',role='reset_button'),dict(name='whole_flow',steps=['init','toggle','reset_button','ok'])])
 out.extend([
  dict(name='live_clear_state',key=65,working=[0xa5000041+i*0x10000 for i in range(9)],actions=[dict(call='SetWindowTextA',text='None',working={'2':0xbeefdead,'4':0xfeedffff},labels={'65':'Changed after clear'})]),
  dict(name='live_later_init',role='init',actions=[dict(call='SetWindowTextA',text='Reset Keys',cfg_keys={'1':0x46},enabled_byte=0x80,handles={'down':0xdead0001})]),
  dict(name='live_later_default',role='reset',actions=[dict(call='SetWindowTextA',text='Up',defaults={'1':0x46},handles={'down':0xdead0002})]),
  dict(name='map_callback_state',key=7,mapped=0x41,actions=[dict(call='MapVirtualKeyA',working={'2':0xcafe1234})]),
  dict(name='zero_conflict_hwnd',working=[0xa5000041+i*0x10000 for i in range(9)],handles={'weapon':0}),
  dict(name='aliased_target_handles',handles={'up':0xabcdef12,'down':0xabcdef12},target='down')])
 return out

def main():
 b=build(SOURCE,STATE,OUT);cases=[];all_cases=scenarios()
 for i,s in enumerate(all_cases):
  n,c=execute(True,b,s),execute(False,b,s);assert n==c,(s,n,c)
  cases.append(dict(scenario=s,equal=True,projection=n))
  if i and i%64==0:print('Verified',i,'keyboard scenarios',flush=True)
 text=SOURCE.read_text();wrong={
  'first_conflict':text.replace('if (slot != (WORD*)&g_keyboard_working_rva_56e5f4[index] &&','if (previous_slot == 0 && slot != (WORD*)&g_keyboard_working_rva_56e5f4[index] &&'),
  'wide_slot_store':text.replace('*slot = key;','*(DWORD*)slot = key;'),
  'cached_caption':text.replace('    if (clear_conflict) {','    LPCSTR cached_label = g_keyboard_labels_rva_36cf0[key];\n    if (clear_conflict) {').replace('LPCSTR label = g_keyboard_labels_rva_36cf0[key];','LPCSTR label = cached_label;')}
 negatives=[]
 for name,txt in wrong.items():
  assert txt!=text,name
  lane=OUT/name;lane.mkdir(exist_ok=True);p=lane/'wrong.cpp';txt=re.sub(r'#include "[^"]*host_options_classes.h"','#include "'+HEADER.as_posix()+'"',txt);p.write_bytes(txt.encode())
  bad=build(p,STATE,lane,False);s=next(x for x in all_cases if x['name']==('live_clear_state'if name=='cached_caption'else'last_conflict'))
  native=execute(True,b,s)
  try:result=execute(False,bad,s);assert result!=native,name;rejection=None
  except AssertionError as error:
   assert name=='wide_slot_store'and str(error).startswith("[['write',"),error
   result=None;rejection=str(error)
  negatives.append(dict(name=name,detected=True,scenario=s,native=native,wrong=result,guard_rejection=rejection,build=bad))
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope='Finite synthetic PC keyboard callbacks and actual key-binding helper. Actual unchanged MFC SetWindowText,PropertyPage PreTranslateMessage and no-op OnOK execute. Base dialog/GetDlgItem/CWnd PreTranslateMessage and Windows endpoints modeled. Compiler virtual reset thunk identified separately. No dialog,keyboard device,registry or game process execution.',build=b,case_count=len(cases),all_equal=True,cases=cases,negative_controls=negatives)
 path=OUT/'options_keyboard_differential.json';path.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negatives=len(negatives),report=str(path))))

if __name__=='__main__':main()
