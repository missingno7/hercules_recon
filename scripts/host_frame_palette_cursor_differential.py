"""Offline frame palette/cursor calls through actual recovered palette/presentation code."""
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
parser.add_argument('--output',type=Path,default=ROOT/'work/frame_palette_cursor_replay')
args=parser.parse_args();OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
SOURCE=ROOT/'src/pc/host_frame_palette_cursor.cpp'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={
 'query':('?OnRva_000097A0@CMainFrame@@QAEHXZ',0x97a0,32,0),
 'changed':('?OnRva_000097C0@CMainFrame@@QAEXPAVCWnd@@@Z',0x97c0,48,4),
 'cursor':('?OnRva_00009BC0@CMainFrame@@QAEHPAVCWnd@@II@Z',0x9bc0,48,12),
 'palette':('_graphics_native_06700',0x6700,128,0),
 'present':('_graphics_native_06620',0x6620,224,0)}
MFC={
 'GetCurrentMessage':('?GetCurrentMessage@CWnd@@KGPBUtagMSG@@XZ',0x225b7,53,148,0),
 'BaseCursor':('?OnSetCursor@CFrameWnd@@IAEHPAVCWnd@@II@Z',0x29b4e,42,147,12)}
FIELDS={
 'dirty':('_g_exe_43a590',0x3a590,1),
 'fullscreen':('_g_fatal_suppress_dialog',0x7a020c,1),
 'owner':('_g_fatal_owner_window',0x7a0200,4),
 'dc':('_g_exe_96e400',0x56e400,4),
 'palette':('_g_exe_96e3fc',0x56e3fc,4),
 'pixels':('_g_exe_96e3d8',0x56e3d8,4),
 'front':('_g_exe_96e3e8',0x56e3e8,4),
 'bmi':('_g_exe_96e3f4',0x56e3f4,4),
 'width':('_g_exe_ba0220',0x7a0220,4),
 'height':('_g_exe_ba0224',0x7a0224,4)}
APIS={'IsChild':8,'SetCursor':4,'UnrealizeObject':4,'SelectPalette':12,
 'RealizePalette':4,'StretchDIBits':52,'GdiFlush':0,'GetDC':4,
 'GetClientRect':8,'FillRect':12,'ReleaseDC':8}
DATA,STACK,ESP,STOP,API=0x61000000,0x62000000,0x62008000,0x6200f000,0x65000000
FRAME,OTHER,PIXELS,BMI,FRONT,VT,MSG=[DATA+n for n in [0x100,0x300,0x1000,0x2000,0x4000,0x5000,0x6000]]
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
NV_SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]


def build(source=None,out=None):
 source=source or SOURCE;out=out or OUT;out.mkdir(parents=True,exist_ok=True)
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
 verify_toolchain(DEFAULT_TOOLCHAIN)
 files=[source,*[ROOT/p for p in ['src/pc/graphics_palette_helpers.cpp',
  'src/pc/graphics_update.cpp','src/pc/graphics_storage.cpp','src/pc/fatal_storage.c']]]
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
  'extern "C" __declspec(dllexport) void anchor(CMainFrame*p) { p->OnRva_000097A0(); p->OnRva_000097C0(0); p->OnRva_00009BC0(0,0,0); }\n').encode())
 objects,compiles,definitions,owners=[],[],{},{}
 for p in [*files,anchor]:
  flags=['/O2','/MT','/I'+str(DEFAULT_TOOLCHAIN/'mfc/include')]if p in [source,anchor]else['/O2']
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
  if source==SOURCE or role in ['palette','present']:assert diag['masked_equal'],(role,diag)
  measurements.append(dict(role=role,symbol=sym,source_size=width,raw=raw,diagnostic=diag))
 archive=Path(libraries[0]['path']).read_bytes();assert Path(libraries[0]['path']).name=='nafxcw.lib'
 position,index,members=8,0,{}
 while position<len(archive):
  h=archive[position:position+60];assert h[58:]==b'`\n';length=int(h[48:58])
  if index in [147,148]:members[index]=archive[position+60:position+60+length]
  position+=60+length+(length&1);index+=1
 proofs=[]
 for role,(sym,rva,n,member,cleanup)in MFC.items():
  obj=out/('mfc_member_'+str(member)+'.obj');obj.write_bytes(members[member]);diag=masked_compare(obj,sym,ORACLE,rva,n)
  assert diag['masked_equal'];proofs.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,member=member,member_sha256=sha(obj),diagnostic=diag))
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
 u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000);u.mem_map(API,0x1000)
 fields={k:(0x400000+r if native else b['symbols'][sym],n)for k,(sym,r,n)in FIELDS.items()}
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 def put(k,v):a,n=fields[k];u.mem_write(a,pack(v)[:n])
 def get(k):a,n=fields[k];return int.from_bytes(u.mem_read(a,n),'little')
 values=dict(dirty=128,fullscreen=0,owner=0x13572468,dc=0x24681357,palette=0x56781234,
  pixels=PIXELS,front=FRONT,bmi=BMI,width=641,height=481)
 values.update(s.get('values',{}))
 for k,v in values.items():put(k,v)
 arena=bytearray((i*13+(i>>8)*17+7)&255 for i in range(0x10000))
 arena[FRAME-DATA+0x1c:FRAME-DATA+0x20]=pack(0x11223344)
 arena[OTHER-DATA+0x1c:OTHER-DATA+0x20]=pack(0x55667788)
 arena[FRONT-DATA:FRONT-DATA+4]=pack(VT)
 arena[VT-DATA+0x2c:VT-DATA+0x30]=pack(API+0x800)
 arena[MSG-DATA:MSG-DATA+28]=struct.pack('<7I',0x77773333,s.get('message',0x30f),0xaabbccdd,0x12345678,99,10,20)
 u.mem_write(DATA,bytes(arena));u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100)
 role=s.get('role','query');arguments=[]
 if role=='changed':arguments=[dict(null=0,self=FRAME,other=OTHER,same_hwnd=OTHER)[s.get('focus','other')]]
 if s.get('focus')=='same_hwnd':u.mem_write(OTHER+0x1c,pack(read(FRAME+0x1c)));arena[OTHER-DATA+0x1c:OTHER-DATA+0x20]=pack(read(FRAME+0x1c))
 if role=='cursor':arguments=[OTHER,s.get('hit',0xffffffff),s.get('cursor_message',0x89abcdef)]
 if role=='palette':arguments=[s.get('message',0x311)]
 u.mem_write(ESP,pack(STOP)+b''.join(pack(v)for v in arguments));u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
 for reg,v in zip(NV,NV_SEEDS):u.reg_write(reg,v)
 addresses={r:0x400000+a if native else b['symbols'][sym]for r,(sym,a,n,cleanup)in PROFILE.items()}
 widths={x['role']:x['source_size']for x in b['measurements']}
 ranges=[(addresses[r],addresses[r]+(n if native else widths[r]))for r,(sym,a,n,cleanup)in PROFILE.items()]
 endpoints={API+0x800:('Flip',12)}
 for i,(name,cleanup)in enumerate(APIS.items()):
  address=API+i*16;endpoints[address]=(name,cleanup)
  for d in image.DIRECTORY_ENTRY_IMPORT:
   for imp in d.imports:
    if imp.name and imp.name.decode()==name:u.mem_write(imp.address,pack(address))
 for name,(sym,rva,n,member,cleanup)in MFC.items():endpoints[0x400000+rva if native else b['symbols'][sym]]=(name,cleanup)
 events,entries,errors,counts=[],[],[],{}
 def ret(sp,cleanup,result):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4)
  u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EIP,target)
 def on_code(machine,a,n,_):
  if a==STOP:machine.emu_stop();return
  if a in addresses.values():entries.append(next(k for k,v in addresses.items()if v==a))
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];sp=machine.reg_read(UC_X86_REG_ESP);argv=[read(sp+4+i*4)for i in range(cleanup//4)]
  index=counts.get(name,0);counts[name]=index+1;actions=s.get('actions',{}).get(name,[]);action=actions[index]if index<len(actions)else{}
  result=action.get('result',dict(GetCurrentMessage=MSG,BaseCursor=0x80000002,IsChild=s.get('child',0),RealizePalette=s.get('realized',0x80000003),GetDC=0x23456789).get(name,1))
  event=argv[:]
  if name=='GetCurrentMessage':assert not argv;result=MSG
  elif name=='BaseCursor':assert machine.reg_read(UC_X86_REG_ECX)==FRAME and argv==arguments
  elif name=='IsChild':assert argv==[read(FRAME+0x1c),0 if not arguments[0]else read(arguments[0]+0x1c)]
  elif name=='SetCursor':assert argv==[0]
  elif name=='UnrealizeObject':assert argv==[get('palette')]
  elif name=='SelectPalette':assert argv==[get('dc'),get('palette'),int(read(MSG+4)==0x311)if role!='palette'else int(s.get('message',0x311)==0x311)]
  elif name=='RealizePalette':assert argv==[get('dc')]
  elif name=='StretchDIBits':assert argv==[get('dc'),0,0,get('width'),get('height'),0,0,320,240,get('pixels'),get('bmi'),0,0xcc0020]
  elif name=='Flip':assert argv==[get('front'),0,1]
  elif name in ['GetDC','GetClientRect','ReleaseDC']:assert argv[0]==get('owner')
  if name=='GetClientRect':
   target=argv[1];assert ESP-0x1000<=target and target+16<=ESP
   rect=s.get('rect',[11,23,333,265]);u.mem_write(target,b''.join(pack(v)for v in rect));event=[argv[0],rect]
  elif name=='FillRect':assert argv[2]==6;event=[argv[0],[read(argv[1]+i*4)for i in range(4)],argv[2]]
  for k,v in action.get('mutate',{}).items():put(k,v)
  if 'retarget_frame'in action:
   u.mem_write(FRAME+0x1c,pack(action['retarget_frame']));arena[FRAME-DATA+0x1c:FRAME-DATA+0x20]=pack(action['retarget_frame'])
  events.append([name,*event]);ret(sp,cleanup,result)
 def on_write(machine,access,a,n,value,_):
  allowed=[(ESP-0x1000,ESP+4+len(arguments)*4),*[(p,p+w)for p,w in fields.values()]]
  if not any(lo<=a and a+n<=hi for lo,hi in allowed):errors.append(['write',hex(a),n]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(addresses[role],STOP,count=100000)
 checks=dict(returned=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4+PROFILE[role][3],
  nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(NV,NV_SEEDS)),
  guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4+len(arguments)*4,32))==bytes([0xa7])*32,
  complete_arena=bytes(u.mem_read(DATA,0x10000))==bytes(arena),whitelist=not errors)
 assert all(checks.values()),(kind,s,checks,errors)
 for name in s.get('requires',[]):assert name in entries,(s,entries)
 result=dict(checks=checks,events=events,entries=entries,state={k:get(k)for k in fields},arena_sha256=hashlib.sha256(bytes(arena)).hexdigest())
 if role in ['query','cursor','palette']:result['return_bits']=u.reg_read(UC_X86_REG_EAX)
 return result


def scenarios():
 result=[dict(name='query_'+hex(m),message=m,requires=['palette','present'])for m in [0,0x30f,0x311,0xffff0311,0xffffffff]]
 result.extend(dict(name='realized_'+hex(v),realized=v)for v in [0,1,255,0x7fffffff,0xffffffff])
 for key in ['dc','palette']:result.append(dict(name='query_null_'+key,values={key:0}))
 for fullscreen in [1,128,255]:result.append(dict(name='query_fullscreen_'+str(fullscreen),values=dict(fullscreen=fullscreen)))
 for focus,child in [('self',0),('null',0),('null',1),('other',0),('other',1),('other',0x80000000),('same_hwnd',0)]:
  result.append(dict(name='change_'+focus+'_'+str(child),role='changed',focus=focus,child=child))
 result.extend([
  dict(name='child_mutates_before_query',role='changed',focus='other',actions={'IsChild':[dict(mutate=dict(fullscreen=128,dc=0),retarget_frame=0xabcdef12)]}),
  dict(name='unrealize_reloads_palette_dc',message=0x311,actions={'UnrealizeObject':[dict(result=0,mutate=dict(palette=0x87654321,dc=0x34567890))]}),
  dict(name='select_reloads_dc',actions={'SelectPalette':[dict(result=0,mutate=dict(dc=0x98765432))]}),
  dict(name='realize_changes_present_to_flip',actions={'RealizePalette':[dict(mutate=dict(fullscreen=128,dirty=0))]}),
  dict(name='realize_changes_present_to_fallback',actions={'RealizePalette':[dict(mutate=dict(pixels=0))]}),
  dict(name='present_fallback_owner_live',role='present',values=dict(dc=0),actions={'GetDC':[dict(mutate=dict(owner=0x87654321))],'GetClientRect':[dict(mutate=dict(owner=0xabcdef12))]}),
  dict(name='fallback_failures_ignored',role='present',values=dict(pixels=0),actions={'GetDC':[dict(result=0)],'GetClientRect':[dict(result=0)]},rect=[30,40,10,20]),
 ])
 for fullscreen in [0,1,2,128,255]:result.append(dict(name='cursor_fullscreen_'+str(fullscreen),role='cursor',values=dict(fullscreen=fullscreen)))
 for v in [0,1,0xffffffff]:result.append(dict(name='cursor_base_return_'+hex(v),role='cursor',actions={'BaseCursor':[dict(result=v)]}))
 result.append(dict(name='cursor_set_failure_still_true',role='cursor',values=dict(fullscreen=128),actions={'SetCursor':[dict(result=0,mutate=dict(fullscreen=0))]}))
 for values in [dict(dirty=0),dict(fullscreen=128),dict(fullscreen=128,front=0),dict(dc=0),dict(pixels=0),dict(dc=0,owner=0),dict(width=0xffffffff,height=0x80000000)]:
  result.append(dict(name='present_'+str(values),role='present',values=values))
 result.append(dict(name='direct_palette_changed',role='palette',message=0x311,requires=['present']))
 return result


def main():
 b=build();cases=[]
 for s in scenarios():
  native=execute('native',b,s);source=execute('source',b,s);assert native==source,(s,native,source)
  cases.append(dict(scenario=s,native=native,source=source,equal=True))
 original=SOURCE.read_text();assert original.count('focus != this && !IsChild(focus)')==1
 negative=OUT/'negative_null.cpp';negative.write_bytes(original.replace('../../src/pc/host_system_keys.h',(ROOT/'src/pc/host_system_keys.h').as_posix()).replace('focus != this && !IsChild(focus)','focus != this && focus && !IsChild(focus)').encode())
 bad=build(negative,OUT/'negative_null');s=next(x for x in scenarios()if x['name']=='change_null_0')
 native=execute('native',b,s);wrong=execute('source',bad,s);assert native!=wrong and 'query'in native['entries']and 'query'not in wrong['entries']
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 report=dict(scope='Actual frame/palette/presentation bodies in offline emulation; identified MFC methods and OS/COM endpoints modeled. No original process,window,message loop or device.',build=b,cases=cases,case_count=len(cases),all_equal=True,
  negative_control=dict(description='Wrong null-focus early suppression',detected=True,scenario=s,native=native,wrong=wrong,build=bad))
 p=OUT/'frame_palette_cursor_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
 print(json.dumps(dict(cases=len(cases),all_equal=True,negative_detected=True,report=str(p))))


if __name__=='__main__':main()
