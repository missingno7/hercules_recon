"""Normal-path paint/DC lifetime comparison with explicit unmatched outer EH registration."""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/frame_paint_replay')
args=parser.parse_args();OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
SOURCE=ROOT/'src/pc/host_frame_paint.cpp'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={
 'paint':('?OnRva_00009740@CMainFrame@@QAEXXZ',0x9740,96,0),
 'present':('_graphics_native_06620',0x6620,224,0)}
LIBRARY={
 'paint_ctor':('??0CPaintDC@@QAE@PAVCWnd@@@Z',0x273e5,86,112),
 'paint_dtor':('??1CPaintDC@@UAE@XZ',0x27457,71,112),
 'dc_ctor':('??0CDC@@QAE@XZ',0x26eb0,20,112),
 'dc_dtor':('??1CDC@@UAE@XZ',0x26fe2,64,112),
 'attach':('?Attach@CDC@@QAEHPAUHDC__@@@Z',0x26f7a,55,112),
 'detach':('?Detach@CDC@@QAEPAUHDC__@@XZ',0x26fb1,49,112),
 'set_attrib':('?SetAttribDC@CDC@@UAEXPAUHDC__@@@Z',0x27022,10,112),
 'release_attrib':('?ReleaseAttribDC@CDC@@UAEXXZ',0x27036,5,112),
 'map':('?afxMapHDC@@YAPAVCHandleMap@@H@Z',0x26ef4,112,112),
 'map_slot':('??ACMapPtrToPtr@@QAEAAPAXPAX@Z',0x216cf,80,164),
 'map_remove':('?RemoveKey@CMapPtrToPtr@@QAEHPAX@Z',0x2171f,66,164)}
BOUNDARIES={'map':0,'map_slot':4,'map_remove':4}
FIELDS={
 'dirty':('_g_exe_43a590',0x3a590,1),
 'fullscreen':('_g_fatal_suppress_dialog',0x7a020c,1),
 'owner':('_g_fatal_owner_window',0x7a0200,4),
 'dc':('_g_exe_96e400',0x56e400,4),
 'pixels':('_g_exe_96e3d8',0x56e3d8,4),
 'front':('_g_exe_96e3e8',0x56e3e8,4),
 'bmi':('_g_exe_96e3f4',0x56e3f4,4),
 'width':('_g_exe_ba0220',0x7a0220,4),
 'height':('_g_exe_ba0224',0x7a0224,4)}
APIS={'BeginPaint':8,'EndPaint':8,'StretchDIBits':52,'GdiFlush':0,
 'GetDC':4,'GetClientRect':8,'FillRect':12,'ReleaseDC':8,'DeleteDC':4}
DATA,STACK,ESP,STOP,API=0x61000000,0x62000000,0x62008000,0x6200f000,0x65000000
FRAME,PIXELS,BMI,FRONT,VT,MAP,CELL=[DATA+n for n in[0x100,0x1000,0x2000,0x4000,0x5000,0x6000,0x7000]]
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
NV_SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]


def build(source=None,out=None):
 source=source or SOURCE;out=out or OUT;out.mkdir(parents=True,exist_ok=True)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 verify_toolchain(DEFAULT_TOOLCHAIN)
 files=[source,*[ROOT/p for p in ['src/pc/graphics_update.cpp','src/pc/graphics_storage.cpp','src/pc/fatal_storage.c']]]
 # Same pinned historical MFC-first link inputs as the existing host graph.
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
 watched={ORACLE,Path(__file__),ROOT/'recovery.json',ROOT/'toolchains/manifest.json',
  *[Path(x['path'])for x in libraries]}
 for p in files:watched.update(context_files(p))
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 pins={str(p):sha(p)for p in watched}
 anchor=out/'anchor.cpp'
 anchor.write_bytes(('#include "'+(ROOT/'src/pc/host_system_keys.h').as_posix()+'"\n'+
  'extern "C" int main(void) { for (;;) {} }\n'+
  'extern "C" __declspec(dllexport) void anchor(CMainFrame*p) { p->OnRva_00009740(); }\n').encode())
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
 for role,(sym,rva,n,cleanup)in PROFILE.items():
  obj,width=definitions[sym];raw=compare(obj,sym,ORACLE,rva,n);diag=masked_compare(obj,sym,ORACLE,rva,n)
  if role=='present':assert diag['masked_equal'],(role,diag)
  measurements.append(dict(role=role,symbol=sym,source_size=width,raw=raw,diagnostic=diag))
 proofs=[];members={}
 for relative,indices in [('mfc/lib/nafxcw.lib',[112,164]),('lib/libcmt.lib',[67])]:
  archive=(DEFAULT_TOOLCHAIN/relative).read_bytes();position,index=8,0
  while position<len(archive):
   h=archive[position:position+60];assert h[58:]==b'`\n';length=int(h[48:58])
   if index in indices:
    obj=out/(Path(relative).stem+'_member_'+str(index)+'.obj');obj.write_bytes(archive[position+60:position+60+length]);members[(relative,index)]=obj
   position+=60+length+(length&1);index+=1
 linked=pefile.PE(str(dll));linked_base=linked.OPTIONAL_HEADER.ImageBase
 for role,(sym,rva,n,index)in LIBRARY.items():
  obj=members[('mfc/lib/nafxcw.lib',index)];diag=masked_compare(obj,sym,ORACLE,rva,n);assert diag['masked_equal'],(role,diag)
  section=COFF(obj).function(sym);actual=bytearray(linked.get_data(symbols[sym]-linked_base,n));expected=bytearray(section['data'])
  for rel in section['relocations']:actual[rel['offset']:rel['offset']+4]=bytes(4);expected[rel['offset']:rel['offset']+4]=bytes(4)
  assert actual==expected
  proofs.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,member=index,member_sha256=sha(obj),diagnostic=diag,linked_code_diagnostic_equal=True,execution='modeled identified boundary'if role in BOUNDARIES else'actual unmodified code'))
 obj=members[('lib/libcmt.lib',67)];co=COFF(obj)
 sym=next(x for x in co.symbols.values()if x['name']=='__EH_prolog'and x['section']>0);sec=co.sections[sym['section']-1]
 assert sym['value']==0 and sec['size']==31 and len(sec['data'])==31 and not sec['relocations']
 original=pefile.PE(str(ORACLE));assert sec['data']==original.get_data(0x20590,31)==linked.get_data(symbols['__EH_prolog']-linked_base,31)
 proofs.append(dict(role='eh_prolog',symbol='__EH_prolog',rva='0x20590',bytes=31,member=67,member_sha256=sha(obj),whole_archive_section_raw_equal=True,section_sha256=hashlib.sha256(sec['data']).hexdigest(),linked_raw_equal=True,execution='actual byte-identical library code',scope='Unmodified31-byte non-COMDAT assembly library section;not a source FUNCTION_MATCH gate.'))
 for symbol,rva in [('??_7CPaintDC@@6B@',0x3134c),('??_7CDC@@6B@',0x312cc)]:
  co=COFF(members[('mfc/lib/nafxcw.lib',112)]);s=next(s for s in co.symbols.values()if s['name']==symbol and s['section']>0);section=co.sections[s['section']-1]
  for off,role in [(0x14,'set_attrib'),(0x1c,'release_attrib')]:
   rel=next(x for x in section['relocations']if x['offset']==s['value']+off);assert rel['symbol']==LIBRARY[role][0]
   original=pefile.PE(str(ORACLE));assert struct.unpack('<I',original.get_data(rva+off,4))[0]==0x400000+LIBRARY[role][1]
 original=pefile.PE(str(ORACLE));data=[]
 for role,(sym,rva,n)in FIELDS.items():
  assert len(owners[sym])==1;co,s=owners[sym][0]
  if s['section']==0:assert s['value']==n;value=bytes(n)
  else:
   sec=co.sections[s['section']-1];assert not sec['relocations'];value=sec['data'][s['value']:s['value']+n]if sec['data']else bytes(n)
  target=next(t for t in original.sections if t.VirtualAddress<=rva and rva+n<=t.VirtualAddress+t.Misc_VirtualSize)
  zero=rva>=target.VirtualAddress+target.SizeOfRawData
  assert value==(bytes(n)if zero else original.get_data(rva,n)),role
  data.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,backing='virtual_zero'if zero else'initialized'))
 assert pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,mfc_proofs=proofs,data=data,
  libraries=libraries,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols)


def execute(kind,b,s):
 native=kind=='native';image=pefile.PE(str(ORACLE if native else b['dll']))
 base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
 u.mem_map(0,0x1000);u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000);u.mem_map(API,0x1000)
 parent=s.get('parent_seh',0x68000100);tib=bytearray([0x6d]*0x1000);tib[:4]=pack(parent);u.mem_write(0,bytes(tib))
 fields={k:(0x400000+r if native else b['symbols'][sym],n)for k,(sym,r,n)in FIELDS.items()}
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def put(k,v):a,n=fields[k];u.mem_write(a,pack(v)[:n])
 def get(k):a,n=fields[k];return int.from_bytes(u.mem_read(a,n),'little')
 values=dict(dirty=128,fullscreen=0,owner=0x13572468,dc=0x24681357,pixels=PIXELS,
  front=FRONT,bmi=BMI,width=641,height=481)
 values.update(s.get('values',{}))
 for k,v in values.items():put(k,v)
 arena=bytearray((i*13+(i>>8)*17+7)&255 for i in range(0x10000))
 arena[FRAME-DATA+0x1c:FRAME-DATA+0x20]=pack(0x11223344)
 arena[FRONT-DATA:FRONT-DATA+4]=pack(VT);arena[VT-DATA+0x2c:VT-DATA+0x30]=pack(API+0x800)
 arena[CELL-DATA:CELL-DATA+4]=bytes(4);u.mem_write(DATA,bytes(arena))
 u.mem_write(ESP-0x3000,bytes([0xa7])*0x3100);u.mem_write(ESP,pack(STOP))
 u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
 for reg,v in zip(NV,NV_SEEDS):u.reg_write(reg,v)
 addresses={r:0x400000+a if native else b['symbols'][sym]for r,(sym,a,n,cleanup)in PROFILE.items()}
 widths={x['role']:x['source_size']for x in b['measurements']}
 ranges=[(addresses[r],addresses[r]+(n if native else widths[r]))for r,(sym,a,n,cleanup)in PROFILE.items()]
 endpoints={API+0x800:('Flip',12)}
 for role,(sym,rva,n,index)in LIBRARY.items():
  a=0x400000+rva if native else b['symbols'][sym]
  if role in BOUNDARIES:endpoints[a]=(role,BOUNDARIES[role])
  else:addresses[role]=a;ranges.append((a,a+n))
 addresses['eh_prolog']=0x420590 if native else b['symbols']['__EH_prolog'];ranges.append((addresses['eh_prolog'],addresses['eh_prolog']+31))
 vt_values={0x43134c if native else b['symbols']['??_7CPaintDC@@6B@']:'CPaintDC',
  0x4312cc if native else b['symbols']['??_7CDC@@6B@']:'CDC',
  0x4308ec if native else b['symbols']['??_7CObject@@6B@']:'CObject'}
 for i,(name,cleanup)in enumerate(APIS.items()):
  a=API+i*16;endpoints[a]=(name,cleanup)
  for d in image.DIRECTORY_ENTRY_IMPORT:
   for imp in d.imports:
    if imp.name and imp.name.decode()==name:u.mem_write(imp.address,pack(a))
 paint_hdc=s.get('paint_hdc',0x234589ab);assert paint_hdc!=0
 ps=struct.pack('<II4iII32s',paint_hdc,1,3,4,80,60,0,1,bytes((i*7+13)&255 for i in range(32)))
 events,entries,errors,counts,snapshots,seh_trace=[],[],[],{},{},[]
 pdc=None;captured_hwnd=None
 def chain():
  p=read(0);states=[]
  while p!=parent:
   assert ESP-0x3000<=p and p+12<=ESP and len(states)<8,('bad synthetic exception chain',hex(p),states)
   states.append(read(p+8));p=read(p)
  return states
 def snapshot(stage):
  assert pdc is not None;value=bytes(u.mem_read(pdc,84));assert int.from_bytes(value[:4],'little')in vt_values
  snapshots[stage]=dict(vtable=vt_values[int.from_bytes(value[:4],'little')],remaining80=value[4:].hex())
 def ret(sp,cleanup,result):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4)
  u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EIP,target)
 def on_code(machine,a,n,_):
  nonlocal pdc,captured_hwnd
  if a==STOP:machine.emu_stop();return
  sp=machine.reg_read(UC_X86_REG_ESP)
  if a in addresses.values():
   role=next(k for k,v in addresses.items()if v==a);entries.append(role)
   seh_trace.append(dict(at=role,states=chain()))
   if role=='paint_ctor':
    pdc=machine.reg_read(UC_X86_REG_ECX);assert ESP-0x3000<=pdc and pdc+84<=ESP;assert read(sp+4)==FRAME
   elif role in ['dc_ctor','paint_dtor','dc_dtor','attach','detach','set_attrib','release_attrib']:
    assert machine.reg_read(UC_X86_REG_ECX)==pdc
   if role in ['paint_dtor','dc_dtor','present']:snapshot(role)
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];argv=[read(sp+4+i*4)for i in range(cleanup//4)]
  index=counts.get(name,0);counts[name]=index+1;actions=s.get('actions',{}).get(name,[]);action=actions[index]if index<len(actions)else{}
  seh_trace.append(dict(at=name,states=chain()));event=argv[:];result=action.get('result',1)
  if name=='map':
   value=read(sp+4);assert value in [0,1];event=[value];result=MAP
  elif name=='map_slot':
   assert machine.reg_read(UC_X86_REG_ECX)==MAP and argv==[paint_hdc];result=CELL
  elif name=='map_remove':
   assert machine.reg_read(UC_X86_REG_ECX)==MAP and argv==[paint_hdc]and read(CELL)==pdc
   u.mem_write(CELL,bytes(4));arena[CELL-DATA:CELL-DATA+4]=bytes(4)
  elif name=='BeginPaint':
   assert argv==[read(FRAME+0x1c),pdc+20]and read(pdc+16)==argv[0]
   captured_hwnd=argv[0];u.mem_write(pdc+20,ps);event=[captured_hwnd,ps.hex()];result=paint_hdc
  elif name=='EndPaint':
   assert argv==[captured_hwnd,pdc+20]and bytes(u.mem_read(pdc+20,64))==ps
   event=[captured_hwnd,ps.hex()]
  elif name=='StretchDIBits':assert argv==[get('dc'),0,0,get('width'),get('height'),0,0,320,240,get('pixels'),get('bmi'),0,0xcc0020]
  elif name=='Flip':assert argv==[get('front'),0,1]
  elif name in ['GetDC','GetClientRect','ReleaseDC']:assert argv[0]==get('owner')
  elif name=='DeleteDC':raise AssertionError('Detached paint HDC must not be deleted by CDC destruction')
  if name=='GetDC':result=action.get('result',0x3456789a)
  elif name=='GetClientRect':
   target=argv[1];assert ESP-0x3000<=target and target+16<=ESP
   rect=s.get('rect',[11,23,333,265]);u.mem_write(target,b''.join(pack(v)for v in rect));event=[argv[0],rect]
  elif name=='FillRect':assert argv[2]==6;event=[argv[0],[read(argv[1]+i*4)for i in range(4)],argv[2]]
  for k,v in action.get('mutate',{}).items():put(k,v)
  if 'hwnd'in action:
   u.mem_write(FRAME+0x1c,pack(action['hwnd']));arena[FRAME-DATA+0x1c:FRAME-DATA+0x20]=pack(action['hwnd'])
  events.append([name,*event]);ret(sp,cleanup,result)
 def on_write(machine,access,a,n,value,_):
  if a==CELL and n==4:arena[CELL-DATA:CELL-DATA+4]=pack(value)
  allowed=[(0,4),(ESP-0x3000,ESP+4),(CELL,CELL+4),*[(p,p+w)for p,w in fields.values()]]
  if not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(['write',hex(a),n]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(addresses['paint'],STOP,count=100000)
 snapshot('final');assert bytes(u.mem_read(pdc+4,80))==bytes(12)+pack(captured_hwnd)+ps
 checks=dict(returned=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,
  nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(NV,NV_SEEDS)),
  guards=bytes(u.mem_read(ESP-0x3000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4,32))==bytes([0xa7])*32,
  complete_arena=bytes(u.mem_read(DATA,0x10000))==bytes(arena),parent_chain_restored=bytes(u.mem_read(0,0x1000))==bytes(tib),
  paint_handle_removed=read(CELL)==0,whitelist=not errors)
 assert all(checks.values()),(kind,s,checks,errors)
 assert counts.get('BeginPaint')==1 and counts.get('EndPaint')==1 and counts.get('map_remove')==1
 semantic=dict(checks=checks,events=events,entries=entries,state={k:get(k)for k in fields},objects=snapshots,arena_sha256=hashlib.sha256(bytes(arena)).hexdigest())
 return dict(semantic=semantic,registration_trace=seh_trace)


def scenarios():
 result=[dict(name='fullscreen_'+str(v),values=dict(fullscreen=v))for v in [0,1,2,128,255]]
 for v in [0,1,128,255]:result.append(dict(name='dirty_'+str(v),values=dict(dirty=v)))
 for key in ['dc','pixels']:result.append(dict(name='fallback_'+key,values={key:0}))
 result.extend([
  dict(name='fallback_null_owner',values=dict(pixels=0,owner=0)),
  dict(name='begin_paint_enters_fullscreen',actions={'BeginPaint':[dict(mutate=dict(fullscreen=128))]}),
  dict(name='begin_paint_exits_fullscreen',values=dict(fullscreen=128),actions={'BeginPaint':[dict(mutate=dict(fullscreen=0,dirty=0))]}),
  dict(name='begin_paint_changes_receiver',actions={'BeginPaint':[dict(hwnd=0x87654321)]}),
  dict(name='present_changes_receiver',actions={'StretchDIBits':[dict(hwnd=0x98765432)]}),
  dict(name='end_paint_live_state',actions={'EndPaint':[dict(result=0,mutate=dict(fullscreen=128,dirty=255),hwnd=0xabcdef12)]}),
  dict(name='end_paint_false',actions={'EndPaint':[dict(result=0)]}),
  dict(name='fallback_live_owner',values=dict(dc=0),actions={'GetDC':[dict(mutate=dict(owner=0x87654321))],'GetClientRect':[dict(mutate=dict(owner=0xabcdef12))]}),
  dict(name='fallback_failures_valid_rect',values=dict(pixels=0),actions={'GetDC':[dict(result=0)],'GetClientRect':[dict(result=0)]},rect=[30,40,10,20]),
  dict(name='negative_client_dimensions',values=dict(width=0xffffffff,height=0x80000000)),
  dict(name='map_boundary_mutates_before_branch',actions={'map_slot':[dict(mutate=dict(fullscreen=128))]}),
 ])
 for hdc in [1,0x80000000,0xffffffff]:result.append(dict(name='opaque_paint_hdc_'+hex(hdc),paint_hdc=hdc))
 for parent in [0xffffffff,0x11223344]:result.append(dict(name='parent_seh_'+hex(parent),parent_seh=parent))
 return result


def main():
 b=build();cases=[]
 for s in scenarios():
  native=execute('native',b,s);source=execute('source',b,s)
  assert native['semantic']==source['semantic'],(s,native,source)
  assert len(native['registration_trace'])==len(source['registration_trace'])
  for left,right in zip(native['registration_trace'],source['registration_trace']):
   assert left['at']==right['at']
   expected=right['states']+([]if left['at']=='paint'else[0xffffffff])
   assert left['states']==expected,(s,left,right)
  cases.append(dict(scenario=s,native=native,source=source,normal_semantic_projection_equal=True,outer_registration_equal=False,only_inactive_outer_record_differs=True))
 original=SOURCE.read_text();assert original.count('g_fatal_suppress_dialog == 0')==1
 negative=OUT/'negative_fullscreen.cpp';negative.write_bytes(original.replace('../../src/pc/host_system_keys.h',(ROOT/'src/pc/host_system_keys.h').as_posix()).replace('g_fatal_suppress_dialog == 0','g_fatal_suppress_dialog != 1').encode())
 bad=build(negative,OUT/'negative_fullscreen');s=next(x for x in scenarios()if x['name']=='fullscreen_128')
 native=execute('native',b,s);wrong=execute('source',bad,s)
 assert native['semantic']!=wrong['semantic']and 'present'not in native['semantic']['entries']and 'present'in wrong['semantic']['entries']
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope='Normal-path paint/DC lifetime projection only. Original96-byte outer EH registration is not reproduced by the48-byte source. Parent chain restoration is checked; in-call registration traces deliberately remain different. No exception dispatch/unwinding,real window,message loop,original process or rendering-device acceptance.',
  build=b,cases=cases,case_count=len(cases),all_normal_semantic_projections_equal=True,outer_exception_registration_equal=False,
  negative_control=dict(description='Wrong fullscreen !=1 versus original ==0',detected=True,scenario=s,native=native,wrong=wrong,build=bad))
 p=OUT/'frame_paint_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
 print(json.dumps(dict(cases=len(cases),normal_semantic_projection_equal=True,outer_registration_equal=False,negative_detected=True,report=str(p))))


if __name__=='__main__':main()
