"""Fresh full shared-table initializer comparison, with two modeled leaf boundaries."""
import sys,json,hashlib,re,os,struct,subprocess,argparse
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/host_graphics_tables_public');args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,verify_toolchain,DEFAULT_TOOLCHAIN
from coff import COFF
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE');assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d';verify_toolchain(DEFAULT_TOOLCHAIN)
files=[ROOT/'src/pc/host_graphics_tables.cpp',ROOT/'src/pc/host_graphics_table_storage.c'];watched=[*files,ROOT/'src/pc/host_graphics_table_storage.h',ORACLE,Path(__file__),*[ROOT/p for p in['scripts/coff.py','scripts/match.py','scripts/pe.py','scripts/env.py','toolchains/manifest.json']]];pins={str(p):sha(p)for p in watched}
fixture=OUT/'fixture.c';fixture.write_bytes(b'''extern void __cdecl graphics_native_071a0(void);
void __cdecl graphics_native_06780(int a,int b,int c,int d) { for (;;) {} }
void __cdecl graphics_native_069a0(unsigned char wait) { for (;;) {} }
__declspec(dllexport) void __cdecl table_fixture_anchor(void) { graphics_native_071a0(); }
''');files.append(fixture);objects=[];compiles=[]
for source in files:
 obj,cmd=compile_source(source,['/O2']);objects.append(obj);compiles.append(dict(source=str(source),source_sha256=sha(source),object=str(obj),object_sha256=sha(obj),command=cmd))
dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
for key in['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects)]
r=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((r.stdout+r.stderr).encode());assert r.returncode==0,(r.stdout,r.stderr)
pattern=re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})');symbols={m.group(1):int(m.group(2),16)for line in mp.read_text(errors='replace').splitlines()if(m:=pattern.match(line))}
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677];STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
def expected():
 shade=bytes(min((row*column)>>7,255)for row in range(256)for column in range(256));block=bytearray(0x41f40)
 for row in range(256):
  for column in range(256):
   off=row*256+column
   for i,value in enumerate([(row+column)>>1,min(row+column,255),max(column-row,0),min((row>>1)+column,255)]):block[0x1f40+i*65536+off]=value
 block[:8192]=bytes(range(256))*32;return shade,bytes(block)
SHADE,BLOCK=expected()
def execute(kind,seed):
 p=pefile.PE(str(ORACLE if kind=='native'else dll));base=p.OPTIONAL_HEADER.ImageBase;size=p.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(STACK,0x10000)
 entry,shade,block,writer,refresh=(0x4071a0,0x91c488,0x92c490,0x406780,0x4069a0)if kind=='native'else tuple(symbols[s]for s in['_graphics_native_071a0','_g_host_graphics_shade_51c488','_g_host_graphics_tables_52c490','_graphics_native_06780','_graphics_native_069a0'])
 width=256 if kind=='native'else COFF(objects[0]).function('_graphics_native_071a0')['size'];u.mem_write(shade,bytes([seed])*65536);u.mem_write(block,bytes([seed^255])*0x41f40)
 if kind=='native':u.mem_write(0x92c488,bytes([0xd7])*8)
 u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP));u.reg_write(UC_X86_REG_ESP,ESP)
 for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
 events=[];illegal=[];writes=[];count=0
 def read32(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def ret(sp):u.reg_write(UC_X86_REG_ESP,sp+4);u.reg_write(UC_X86_REG_EIP,read32(sp));u.reg_write(UC_X86_REG_EAX,0x12345678)
 def on_code(machine,a,w,user):
  nonlocal count
  count+=1
  if a==STOP:machine.emu_stop();return
  sp=machine.reg_read(UC_X86_REG_ESP)
  if a==writer:events.append(dict(call='palette',args=[read32(sp+i)for i in[4,8,12,16]]));ret(sp);return
  if a==refresh:events.append(dict(call='refresh',arg=read32(sp+4)));ret(sp);return
  if not entry<=a<entry+width:illegal.append(hex(a));machine.emu_stop()
 def on_write(machine,access,a,w,v,user):
  if not any(lo<=a and a+w<=hi for lo,hi in[(shade,shade+65536),(block,block+0x41f40),(ESP-0x1000,ESP)]):writes.append((hex(a),w));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(entry,STOP,count=20000000)
 wanted=[dict(call='palette',args=[i]*4)for i in range(256)]+[dict(call='refresh',arg=1)]
 checks=dict(shade_complete=bytes(u.mem_read(shade,65536))==SHADE,shared_block_complete=bytes(u.mem_read(block,0x41f40))==BLOCK,window_views_ramp=bytes(u.mem_read(block+0x1f00,64))==bytes(range(64)),average_prefix_ramp=bytes(u.mem_read(block+0x1f40,192))==bytes(range(64,256)),average_tail_retained=bytes(u.mem_read(block+0x1f40+192,64))==bytes(column>>1 for column in range(192,256)),events=events==wanted,sentinel=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(reg)==value for reg,value in zip(NV,SEEDS)),stack_guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4,32))==bytes([0xa7])*32,execution_whitelist=not illegal,write_whitelist=not writes,native_gap_untouched=kind!='native'or bytes(u.mem_read(0x92c488,8))==bytes([0xd7])*8)
 assert all(checks.values()),(kind,seed,checks,illegal,writes)
 return dict(checks=checks,shade_sha256=hashlib.sha256(bytes(u.mem_read(shade,65536))).hexdigest(),block_sha256=hashlib.sha256(bytes(u.mem_read(block,0x41f40))).hexdigest(),events=events,instructions=count)
cases=[]
for seed in[0x5c,0xa3]:
 n=execute('native',seed);s=execute('source',seed);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in s.items()if k!='instructions'};cases.append(dict(seed=seed,native=n,source=s,equal=True))
assert pins=={p:sha(Path(p))for p in pins}
report=dict(scope='Complete335680-byte generated tables/window backing comparison. Palette writer/refresh are modeled leaf calls;no device/real process/source identity acceptance.',inputs=pins,compiles=compiles,link_command=cmd,dll_sha256=sha(dll),map_sha256=sha(mp),cases=cases,all_equal=True)
path=OUT/'table_differential.json';path.write_bytes((json.dumps(report,indent=2)+'\n').encode());print('Two full table/overlap comparisons passed:',path)
