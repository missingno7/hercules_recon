"""Fresh cached-color fill/search original/source differential; only COM Blt modeled."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file());sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/host_color_fill_public');args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_color_fill.cpp';CACHE_SOURCE=ROOT/'src/pc/host_color_cache.c';HEADER=ROOT/'src/pc/host_color_cache.h';ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={'fill':('_graphics_native_06ee0',0x6ee0,272),'search':('_graphics_native_06850',0x6850,112)}
PIXELS=0x61000020;PAL=0x61014020;SURFACE=0x64000100;VTABLE=0x64000200;BLT=0x64000300;STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
FIELDS={'locked':('_g_exe_ba01f0',0xba01f0,1),'fullscreen':('_g_fatal_suppress_dialog',0xba020c,1),'pixels':('_g_exe_96e3d8',0x96e3d8,4),'render':('_g_exe_ba0210',0xba0210,4),'pitch':('_g_exe_ba0214',0xba0214,4),'surface':('_g_exe_96e3ec',0x96e3ec,4),'dirty':('_g_exe_43a590',0x43a590,1),'palette':('_g_exe_96e3f8',0x96e3f8,4)}
def read32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def build():
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d';verify_toolchain(DEFAULT_TOOLCHAIN)
 files=[SOURCE,CACHE_SOURCE,*[ROOT/p for p in['src/pc/graphics_storage.cpp','src/pc/fatal_storage.c','src/pc/graphics_lock_storage.c']]];watched=[*files,HEADER,ORACLE,Path(__file__),ROOT/'src/pc/host_graphics_table_storage.h',*[ROOT/p for p in['scripts/coff.py','scripts/match.py','scripts/masked.py','scripts/pe.py','scripts/env.py','toolchains/manifest.json']]];pins={str(p):sha(p)for p in watched}
 fixture=OUT/'fixture.c';fixture.write_bytes(b'''extern void __cdecl graphics_native_06ee0(int,int,int);
extern unsigned char __cdecl graphics_native_06850(int,int,int);
__declspec(dllexport) void __cdecl fill_fixture_anchor(void) { graphics_native_06ee0(0,0,0); graphics_native_06850(0,0,0); }
''');files.append(fixture);objects=[];compiles=[];defs={}
 for source in files:
  obj,cmd=compile_source(source,['/O2']);objects.append(obj);co=COFF(obj);compiles.append(dict(source=str(source),source_sha256=sha(source),object=str(obj),object_sha256=sha(obj),command=cmd))
  for s in co.symbols.values():
   if s['storage']==2 and s['section']>0 and s['type']&0x20:defs[s['name']]=(obj,co.function(s['name'])['size'])
 dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
 for key in['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
 env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
 cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects)]
 p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((p.stdout+p.stderr).encode());assert p.returncode==0,(p.stdout,p.stderr)
 pattern=re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})');symbols={m.group(1):int(m.group(2),16)for line in mp.read_text(errors='replace').splitlines()if(m:=pattern.match(line))};comparisons=[]
 for role,(sym,rva,size)in PROFILE.items():
  obj,width=defs[sym];raw=compare(obj,sym,ORACLE,rva,size);diag=masked_compare(obj,sym,ORACLE,rva,size)
  if role=='fill':assert diag['masked_equal']
  comparisons.append(dict(role=role,symbol=sym,source_size=width,native_size=size,raw=raw,diagnostic=diag))
 co=COFF(objects[1]);s=next(x for x in co.symbols.values()if x['name']=='_g_host_color_cache_56e418');sec=co.sections[s['section']-1];assert sec['size']==16 and sec['flags']&0x80 and not sec['data']and not sec['relocations']
 pe=pefile.PE(str(ORACLE));t=next(t for t in pe.sections if t.VirtualAddress<=0x56e418 and 0x56e428<=t.VirtualAddress+t.Misc_VirtualSize);assert 0x56e418>=t.VirtualAddress+t.SizeOfRawData
 assert pins=={p:sha(Path(p))for p in pins};return dict(inputs=pins,compiles=compiles,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols,comparisons=comparisons,cache_bytes=16,native_cache_rva='0x56e418')
def palette(kind):
 if kind=='gray':return [(i,i,i)for i in range(256)]
 if kind=='ties':return [(80,90,100)]*256
 return [((i*37+11)&255,(i*53+23)&255,(i*71+31)&255)for i in range(256)]
def search(rgb,entries,recurrence=False):
 red,green,blue=rgb;best=None;result=0
 for i,(r,g,b)in enumerate(entries):
  working=red-r;distance=working*working+(green-g)*(green-g)+(blue-b)*(blue-b)
  if best is None or distance<best:
   best=distance;result=i
   if not distance:break
  if recurrence:red=blue-b
 return result
def execute(kind,b,s):
 p=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=p.OPTIONAL_HEADER.ImageBase;size=p.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(0x61000000,0x20000);u.mem_map(STACK,0x10000);u.mem_map(0x64000000,0x1000)
 addresses={r:0x400000+rv if kind=='native'else b['symbols'][sym]for r,(sym,rv,n)in PROFILE.items()};widths={c['role']:c['source_size']for c in b['comparisons']};code=[(addresses[r],addresses[r]+(n if kind=='native'else widths[r]))for r,(sym,rv,n)in PROFILE.items()]
 cells={r:va if kind=='native'else b['symbols'][sym]for r,(sym,va,n)in FIELDS.items()};cache=0x96e418 if kind=='native'else b['symbols']['_g_host_color_cache_56e418'];rgb=s.get('rgb',[19,83,147]);entries=palette(s.get('palette','varied'))
 pb=bytearray(bytes([0x87])*1032)
 for i,(r,g,blue)in enumerate(entries):pb[4+i*4:8+i*4]=bytes([r,g,blue,4])
 u.mem_write(PAL-32,bytes([0xa1])*32+pb+bytes([0xa2])*32);u.mem_write(PIXELS-32,bytes([0xb1])*32+bytes([0x6d])*76800+bytes([0xb2])*32)
 state=dict(locked=s.get('locked',0),fullscreen=s.get('fullscreen',0),pixels=0 if s.get('null_pixels')else PIXELS,render=0x12345678,pitch=0x23456789,surface=0 if s.get('null_surface')else SURFACE,dirty=0x31,palette=PAL);expected_state=state.copy()
 def put(r,v):u.mem_write(cells[r],bytes([v&255])if FIELDS[r][2]==1 else pack(v))
 for r,v in state.items():put(r,v)
 initial_cache=bytearray(bytes([0xa7])*16)
 for i,v in enumerate(s.get('cached',[1,2,3,7])):initial_cache[i*4]=v&255
 u.mem_write(cache,bytes(initial_cache));u.mem_write(SURFACE,pack(VTABLE));u.mem_write(VTABLE+0x14,pack(BLT));u.mem_write(ESP-0x1000,bytes([0xc7])*0x1100);u.mem_write(ESP,pack(STOP)+b''.join(pack(v)for v in rgb));u.reg_write(UC_X86_REG_ESP,ESP)
 for r,v in zip(NV,SEEDS):u.reg_write(r,v)
 expected_cache=initial_cache[:];expected_pixels=bytes([0x6d])*76800;wanted=[];result=search(rgb,entries)
 role=s.get('role','fill')
 if role=='fill'and not state['locked']:
  truncated=[v&255 for v in rgb]
  if truncated!=[initial_cache[i]for i in[0,4,8]]:
   for off,v in zip([0,4,8],truncated):expected_cache[off]=v
   expected_cache[12]=search(truncated,entries);wanted.append(dict(call='search',rgb=truncated,cache_at_entry=[expected_cache[i]for i in[0,4,8]]))
  if state['fullscreen']:
   if state['surface']:
    fx=bytearray(100);fx[:4]=pack(100);fx[80:84]=pack(expected_cache[12]);wanted.append(dict(call='Blt',arguments=[SURFACE,0,0,0,0x1000400],fx_hex=fx.hex(),result=s.get('blt_result',0)))
  else:
   expected_state['render']=state['pixels'];expected_state['pitch']=320
   if state['pixels']:expected_pixels=bytes([expected_cache[12]])*76800
  expected_state['dirty']=1
 events=[];illegal=[];writes=[];count=0
 def on_code(machine,a,w,user):
  nonlocal count
  count+=1
  if a==STOP:machine.emu_stop();return
  sp=machine.reg_read(UC_X86_REG_ESP)
  if a==addresses['search']and role=='fill':events.append(dict(call='search',rgb=[read32(machine,sp+i)for i in[4,8,12]],cache_at_entry=[machine.mem_read(cache+i,1)[0]for i in[0,4,8]]))
  if a==BLT:
   vals=[read32(machine,sp+i)for i in[4,8,12,16,20,24]];fx=vals[-1];assert ESP-0x1000<=fx and fx+100<=ESP;events.append(dict(call='Blt',arguments=vals[:-1],fx_hex=bytes(machine.mem_read(fx,100)).hex(),result=s.get('blt_result',0)));machine.reg_write(UC_X86_REG_EAX,s.get('blt_result',0));machine.reg_write(UC_X86_REG_ESP,sp+28);machine.reg_write(UC_X86_REG_EIP,read32(machine,sp));return
  if not any(lo<=a<hi for lo,hi in code):illegal.append(hex(a));machine.emu_stop()
 def on_write(machine,access,a,w,v,user):
  allowed=[(ESP-0x1000,ESP+16),(cache,cache+16),(PIXELS,PIXELS+76800),*[(cells[r],cells[r]+n)for r,(sym,va,n)in FIELDS.items()]]
  if not any(lo<=a and a+w<=hi for lo,hi in allowed):writes.append((hex(a),w));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(addresses[role],STOP,count=200000)
 actual_state={r:u.mem_read(cells[r],1)[0]if n==1 else read32(u,cells[r])for r,(sym,va,n)in FIELDS.items()}
 checks=dict(full_cache=bytes(u.mem_read(cache,16))==bytes(expected_cache),full_pixels=bytes(u.mem_read(PIXELS,76800))==expected_pixels,selected_state=actual_state==expected_state,palette_untouched=bytes(u.mem_read(PAL,1032))==bytes(pb),events=events==wanted,sentinel_return=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(r)==v for r,v in zip(NV,SEEDS)),guards=bytes(u.mem_read(PAL-32,32))==bytes([0xa1])*32 and bytes(u.mem_read(PAL+1032,32))==bytes([0xa2])*32 and bytes(u.mem_read(PIXELS-32,32))==bytes([0xb1])*32 and bytes(u.mem_read(PIXELS+76800,32))==bytes([0xb2])*32,execution=not illegal,writes=not writes)
 if role=='search':checks['byte_return']=u.reg_read(UC_X86_REG_EAX)&255==result
 assert all(checks.values()),(kind,s,checks,illegal,writes,result,u.reg_read(UC_X86_REG_EAX)&255)
 return dict(checks=checks,cache_hex=bytes(u.mem_read(cache,16)).hex(),pixel_sha256=hashlib.sha256(bytes(u.mem_read(PIXELS,76800))).hexdigest(),state=actual_state,events=events,search_byte=(u.reg_read(UC_X86_REG_EAX)&255)if role=='search'else None,falsified_recurrence_result=search(rgb,entries,True)if role=='search'else None,instructions=count)
def main():
 b=build();scenarios=[]
 for pal in['gray','varied','ties']:
  for rgb in[[0,0,0],[255,255,255],[19,83,147],[80,90,100],[200,20,75]]:scenarios.append(dict(name='search_'+pal+'_'+str(rgb),role='search',palette=pal,rgb=rgb))
 scenarios.extend([dict(name='normal'),dict(name='locked',locked=1),dict(name='locked_highbit',locked=0x80),dict(name='cache_hit',rgb=[1,2,3]),dict(name='upper_bits_ignored',rgb=[0x10001,0xffff0002,0x12340003]),dict(name='normal_null_pixels',null_pixels=True),dict(name='fullscreen',fullscreen=1),dict(name='fullscreen_null_surface',fullscreen=1,null_surface=True),dict(name='fullscreen_blt_failure_ignored',fullscreen=1,blt_result=0x80004005),dict(name='fullscreen_cache_hit',fullscreen=1,rgb=[1,2,3])])
 cases=[]
 for s in scenarios:
  n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},s;cases.append(dict(scenario=s,native=n,source=c,equal=True))
 assert b['inputs']=={p:sha(Path(p))for p in b['inputs']};report=dict(scope='Actual original/fresh fill/search code; COMBltalone modeled; bounded RGB0..255 search andlowBYTE fill inputs. No real device/process or whole graphics/game acceptance.',build=b,cases=cases,case_count=len(cases),all_equal=True);p=OUT/'color_fill_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,report=str(p)),indent=2))
if __name__=='__main__':main()
