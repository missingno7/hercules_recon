"""Offline resize callbacks through genuine unmodified static MFC window methods."""
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
from masked import masked_compare
from coff import COFF
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/frame_resize_replay')
args=parser.parse_args();OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_frame_resize.cpp'
HEADER=ROOT/'calibration/host_system_keys.h'
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
signed=lambda v:v-0x100000000 if v&0x80000000 else v
PROFILE={
 'resize':('?OnRva_000099A0@CMainFrame@@QAEXHH@Z',0x99a0,192,8),
 'small':('?OnRva_00009A60@CMainFrame@@QAEXXZ',0x9a60,16,0),
 'medium':('?OnRva_00009A70@CMainFrame@@QAEXXZ',0x9a70,16,0),
 'large':('?OnRva_00009A80@CMainFrame@@QAEXXZ',0x9a80,16,0)}
MFC={
 'MFCShowWindow':('?ShowWindow@CWnd@@QAEHH@Z',0x25113,39,4),
 'MFCSetWindowPos':('?SetWindowPos@CWnd@@QAEHPBV1@HHHHI@Z',0x250c4,79,24)}
APIS={'ShowWindow':8,'GetWindowRect':8,'SystemParametersInfoA':16,'GetSystemMetrics':4,'SetWindowPos':28}
DATA,FRAME,SITE,VT=0x61000000,0x61000100,0x61000400,0x61001000
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
 files=[source,ROOT/'calibration/host_frame_state_storage.c']
 watched={ORACLE,HEADER,Path(__file__),ROOT/'recovery.json',ROOT/'toolchains/manifest.json',
  *[Path(x['path'])for x in libraries]}
 for p in files:watched.update(context_files(p))
 watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
 pins={str(p):sha(p)for p in watched}
 anchor=out/'anchor.cpp'
 anchor.write_bytes(('#include "'+HEADER.as_posix()+'"\n'+
  'extern "C" int main(void) { for (;;) {} }\n'+
  'extern "C" __declspec(dllexport) void anchor(CMainFrame*p) { p->OnRva_000099A0(123,456); p->OnRva_00009A60(); p->OnRva_00009A70(); p->OnRva_00009A80(); }\n').encode())
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
  if source==SOURCE:assert diag['masked_equal'],(role,diag)
  measurements.append(dict(role=role,symbol=sym,source_size=width,raw=raw,diagnostic=diag))
 archive=Path(libraries[0]['path']).read_bytes();assert Path(libraries[0]['path']).name=='nafxcw.lib'
 position,index,payload=8,0,None
 while position<len(archive):
  h=archive[position:position+60];assert h[58:]==b'`\n';length=int(h[48:58])
  if index==104:payload=archive[position+60:position+60+length];break
  position+=60+length+(length&1);index+=1
 assert payload is not None;member=out/'mfc_member_104.obj';member.write_bytes(payload)
 co=COFF(member);proofs=[]
 for role,(sym,rva,n,cleanup)in MFC.items():
  diag=masked_compare(member,sym,ORACLE,rva,n);assert diag['masked_equal']
  # Check the linked library contribution as well as its original archive body.
  sec=co.function(sym);linked=pefile.PE(str(dll));raw=bytearray(linked.get_data(symbols[sym]-linked.OPTIONAL_HEADER.ImageBase,n));expected=bytearray(sec['data'])
  for rel in sec['relocations']:raw[rel['offset']:rel['offset']+4]=bytes(4);expected[rel['offset']:rel['offset']+4]=bytes(4)
  assert raw==expected
  proofs.append(dict(role=role,symbol=sym,rva=hex(rva),bytes=n,member=104,member_sha256=sha(member),diagnostic=diag,linked_code_diagnostic_equal=True))
 owners=owners['_g_host_frame_maximized_7a022c'];assert len(owners)==1;co,s=owners[0]
 sec=co.sections[s['section']-1];assert sec['size']==1 and not sec['data']and not sec['relocations']
 original=pefile.PE(str(ORACLE));target=next(t for t in original.sections if t.VirtualAddress<=0x7a022c<t.VirtualAddress+t.Misc_VirtualSize)
 assert 0x7a022c>=target.VirtualAddress+target.SizeOfRawData
 assert pins=={p:sha(p)for p in pins}
 return dict(inputs=pins,compiles=compiles,measurements=measurements,mfc_proofs=proofs,
  libraries=libraries,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols)


def execute(kind,b,s):
 native=kind=='native';image=pefile.PE(str(ORACLE if native else b['dll']))
 base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
 u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000);u.mem_map(API,0x1000)
 maximized=0xba022c if native else b['symbols']['_g_host_frame_maximized_7a022c']
 u.mem_write(maximized,bytes([s.get('maximized',0)]))
 read=lambda a:int.from_bytes(u.mem_read(a,4),'little')
 arena=bytearray((i*13+(i>>8)*17+7)&255 for i in range(0x10000))
 def arena_put(a,v):u.mem_write(a,pack(v));arena[a-DATA:a-DATA+4]=pack(v)
 u.mem_write(DATA,bytes(arena));arena_put(FRAME+0x1c,0x11223344);arena_put(FRAME+0x38,SITE if s.get('site')else 0)
 arena_put(SITE,VT);arena_put(VT+0xa4,API+0x800);arena_put(VT+0xa8,API+0x810)
 role=s.get('role','resize');arguments=s.get('dimensions',[357,263])if role=='resize'else[]
 dimensions=arguments if arguments else dict(small=[320,240],medium=[480,360],large=[640,480])[role]
 u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP)+b''.join(pack(v)for v in arguments))
 u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_ECX,FRAME)
 for reg,v in zip(NV,NV_SEEDS):u.reg_write(reg,v)
 addresses={r:0x400000+a if native else b['symbols'][sym]for r,(sym,a,n,cleanup)in PROFILE.items()}
 widths={x['role']:x['source_size']for x in b['measurements']}
 ranges=[(addresses[r],addresses[r]+(n if native else widths[r]))for r,(sym,a,n,cleanup)in PROFILE.items()]
 for r,(sym,a,n,cleanup)in MFC.items():addresses[r]=0x400000+a if native else b['symbols'][sym];ranges.append((addresses[r],addresses[r]+n))
 endpoints={API+0x800:('SiteSetWindowPos',24),API+0x810:('SiteShowWindow',4)}
 for i,(name,cleanup)in enumerate(APIS.items()):
  a=API+i*16;endpoints[a]=(name,cleanup)
  for d in image.DIRECTORY_ENTRY_IMPORT:
   for imp in d.imports:
    if imp.name and imp.name.decode()==name:u.mem_write(imp.address,pack(a))
 metrics={46:3,8:5,15:17,4:23,45:7,7:11,32:31,33:37};metrics.update(s.get('metrics',{}))
 window=s.get('window',[-80,40,700,800]);work=s.get('work',[10,20,1280,720]);metric_reads={}
 events,entries,errors,counts=[],[],[],{}
 def ret(sp,cleanup,result):
  target=read(sp);u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xc1c2c3c4);u.reg_write(UC_X86_REG_EDX,0xd1d2d3d4)
  u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EIP,target)
 def on_code(machine,a,n,_):
  if a==STOP:machine.emu_stop();return
  sp=machine.reg_read(UC_X86_REG_ESP)
  if a in addresses.values():
   r=next(k for k,v in addresses.items()if v==a);entries.append(r)
   if r in MFC:assert machine.reg_read(UC_X86_REG_ECX)==FRAME
  if a not in endpoints:
   if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execute',hex(a)]);machine.emu_stop()
   return
  name,cleanup=endpoints[a];argv=[read(sp+4+i*4)for i in range(cleanup//4)]
  index=counts.get(name,0);counts[name]=index+1;actions=s.get('actions',{}).get(name,[]);action=actions[index]if index<len(actions)else{}
  result=action.get('result',1);event=argv[:]
  if name in ['SiteShowWindow','SiteSetWindowPos']:assert machine.reg_read(UC_X86_REG_ECX)==SITE
  if name=='ShowWindow':assert argv==[read(FRAME+0x1c),9]
  elif name=='SiteShowWindow':assert argv==[9]
  elif name in ['GetWindowRect','SystemParametersInfoA']:
   if name=='GetWindowRect':assert argv[0]==read(FRAME+0x1c);target=argv[1];rect=window;event=[argv[0],rect]
   else:assert argv[:2]==[0x30,0]and argv[3]==0;target=argv[2];rect=work;event=[0x30,0,rect,0]
   assert ESP-0x1000<=target and target+16<=ESP;u.mem_write(target,b''.join(pack(v)for v in rect))
  elif name=='GetSystemMetrics':result=action.get('result',metrics[argv[0]]);metric_reads[argv[0]]=result;event=[argv[0],result]
  elif name in ['SetWindowPos','SiteSetWindowPos']:
   values=[0,max(signed(window[0]&0xffffffff),signed(work[0]&0xffffffff))&0xffffffff,
    max(signed(window[1]&0xffffffff),signed(work[1]&0xffffffff))&0xffffffff,
    (dimensions[0]+2*(metric_reads.get(45,metric_reads.get(32,0))+metric_reads[7]))&0xffffffff,
    (dimensions[1]+2*(metric_reads.get(46,metric_reads.get(33,0))+metric_reads[8])+metric_reads[15]+metric_reads[4])&0xffffffff,4]
   assert argv==([read(FRAME+0x1c),*values]if name=='SetWindowPos'else values),(s,name,argv,values)
  if 'maximized'in action:u.mem_write(maximized,bytes([action['maximized']]))
  if 'hwnd'in action:arena_put(FRAME+0x1c,action['hwnd'])
  if 'site'in action:arena_put(FRAME+0x38,SITE if action['site']else 0)
  events.append(dict(call=name,args=event,maximized=int.from_bytes(u.mem_read(maximized,1),'little')));ret(sp,cleanup,result)
 def on_write(machine,access,a,n,value,_):
  if not(ESP-0x1000<=a and a+n<=ESP+4+len(arguments)*4 or a==maximized and n==1):errors.append(['write',hex(a),n]);machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(addresses[role],STOP,count=100000)
 checks=dict(returned=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4+PROFILE[role][3],
  nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(NV,NV_SEEDS)),
  guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4+len(arguments)*4,32))==bytes([0xa7])*32,
  complete_arena=bytes(u.mem_read(DATA,0x10000))==bytes(arena),whitelist=not errors)
 assert all(checks.values()),(kind,s,checks,errors)
 return dict(checks=checks,events=events,entries=entries,maximized=int.from_bytes(u.mem_read(maximized,1),'little'),arena_sha256=hashlib.sha256(bytes(arena)).hexdigest())


def scenarios():
 result=[dict(name='wrapper_'+r,role=r)for r in ['small','medium','large']]
 for v in [0,1,2,128,255]:result.append(dict(name='restore_'+str(v),maximized=v))
 for window,work in [([100,200,300,400],[10,20,30,40]),([-100,-200,0,0],[-10,-20,1,1]),([0,0,1,1],[0,0,1,1]),([2000,3000,4000,5000],[0,0,640,480]),([0x80000000,0x7fffffff,0,0],[0x7fffffff,0x80000000,1,1])]:
  result.append(dict(name='coordinates_'+str(window),window=window,work=work))
 for dims in [[0,0],[-1,-10],[0x7fffffff,0x7fffffff],[0x80000000,0x80000000],[320,240]]:result.append(dict(name='dimensions_'+str(dims),dimensions=dims))
 result.extend([
  dict(name='metric_zero',metrics={46:0,8:0,15:0,4:0,45:0,7:0}),
  dict(name='metric_highbits',metrics={46:0x7fffffff,8:3,15:0xffffffff,4:0x80000000,45:0x7fffffff,7:1}),
  dict(name='show_live_hwnd_and_flag',maximized=128,actions={'ShowWindow':[dict(result=0,hwnd=0x87654321,maximized=255)]}),
  dict(name='rect_live_hwnd',actions={'GetWindowRect':[dict(result=0,hwnd=0xabcdef12)]}),
  dict(name='work_failure_valid_rect',actions={'SystemParametersInfoA':[dict(result=0,hwnd=0x98765432)]}),
  dict(name='metric_callback_retargets',actions={'GetSystemMetrics':[dict(hwnd=0x11117777),{},{},{},{},dict(hwnd=0x22228888)]}),
  dict(name='set_failure_ignored',actions={'SetWindowPos':[dict(result=0,maximized=128)]}),
  dict(name='control_site',site=True,maximized=1),
  dict(name='site_show_switches_to_win32',site=True,maximized=255,actions={'SiteShowWindow':[dict(site=False,hwnd=0x87654321,maximized=255)]}),
  dict(name='show_switches_to_site',maximized=1,actions={'ShowWindow':[dict(site=True,hwnd=0x98765432)]}),
  dict(name='metric_switches_to_site',actions={'GetSystemMetrics':[dict(site=True)]}),
  dict(name='site_set_failure_ignored',site=True,actions={'SiteSetWindowPos':[dict(result=0,maximized=255)]}),
 ])
 return result


def main():
 b=build();cases=[]
 for s in scenarios():
  native=execute('native',b,s);source=execute('source',b,s);assert native==source,(s,native,source)
  assert [e['args'][0]for e in native['events']if e['call']=='GetSystemMetrics']==[46,8,15,4,45,7]
  assert 'MFCSetWindowPos'in native['entries']
  cases.append(dict(scenario=s,native=native,source=source,equal=True))
 source=SOURCE.read_text();assert source.count('SM_CXEDGE')==1 and source.count('SM_CYEDGE')==1
 negative=OUT/'negative_metrics.cpp';negative.write_bytes(source.replace('"../../calibration/host_system_keys.h"','"'+HEADER.as_posix()+'"').replace('SM_CXEDGE','SM_CXSIZEFRAME').replace('SM_CYEDGE','SM_CYSIZEFRAME').encode())
 bad=build(negative,OUT/'negative_metrics');s=dict(name='wrong_metric_discriminator');native=execute('native',b,s);wrong=execute('source',bad,s)
 assert native!=wrong
 report=dict(scope='Actual resize callbacks and two unmodified MFC method bodies in offline emulation. Windows and control-site virtual endpoints modeled. No original process,window,message loop or device.',build=b,cases=cases,case_count=len(cases),all_equal=True,
  negative_control=dict(description='Wrong size-frame32/33 instead of native edge45/46 metrics',detected=True,scenario=s,native=native,wrong=wrong,build=bad))
 assert b['inputs']=={p:sha(p)for p in b['inputs']}
 p=OUT/'frame_resize_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,negative_detected=True,report=str(p))))


if __name__=='__main__':main()
