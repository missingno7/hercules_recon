"""Fresh palette entry and table-chain comparison; refresh69A0 alone modeled."""
import argparse,json,hashlib,sys,os,struct,re,subprocess
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if(p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/host_palette_entries_public');args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_palette_entries.cpp';ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={'set':('_graphics_native_06780',0x6780,144),'get':('_host_palette_get_rgb_6810',0x6810,64),'tables':('_graphics_native_071a0',0x71a0,256)}
PAL=0x61000120;INFO=0x61004020;OUTPUT=0x61007020;STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
def build():
 assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d';verify_toolchain(DEFAULT_TOOLCHAIN)
 files=[SOURCE,*[ROOT/p for p in['src/pc/host_graphics_tables.cpp','calibration/host_graphics_table_storage.c','calibration/graphics_storage.cpp']]]
 watched=[*files,ORACLE,Path(__file__),ROOT/'calibration/host_graphics_table_storage.h',*[ROOT/p for p in['scripts/coff.py','scripts/match.py','scripts/masked.py','scripts/pe.py','scripts/env.py','toolchains/manifest.json']]];pins={str(p):sha(p)for p in watched}
 fixture=OUT/'fixture.c';fixture.write_bytes(b'''extern void __cdecl graphics_native_06780(short,unsigned char,unsigned char,unsigned char);
extern void __cdecl host_palette_get_rgb_6810(short,unsigned char*,unsigned char*,unsigned char*);
extern void __cdecl graphics_native_071a0(void);
void __cdecl graphics_native_069a0(unsigned char value) { for (;;) {} }
__declspec(dllexport) void __cdecl palette_fixture_anchor(void) { graphics_native_06780(0,0,0,0); host_palette_get_rgb_6810(0,0,0,0); graphics_native_071a0(); }
''');files.append(fixture);objects=[];compiles=[];defs={}
 for source in files:
  obj,cmd=compile_source(source,['/O2']);objects.append(obj);co=COFF(obj);compiles.append(dict(source=str(source),source_sha256=sha(source),object=str(obj),object_sha256=sha(obj),command=cmd))
  for s in co.symbols.values():
   if s['storage']==2 and s['section']>0 and s['type']&0x20:assert s['name']not in defs;defs[s['name']]=(obj,co.function(s['name'])['size'])
 dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
 for key in['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
 env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
 cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects)]
 p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((p.stdout+p.stderr).encode());assert p.returncode==0,(p.stdout,p.stderr)
 pattern=re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})');symbols={m.group(1):int(m.group(2),16)for line in mp.read_text(errors='replace').splitlines()if(m:=pattern.match(line))};comparisons=[]
 for role,(sym,rva,size)in PROFILE.items():
  obj,width=defs[sym];raw=compare(obj,sym,ORACLE,rva,size);diag=masked_compare(obj,sym,ORACLE,rva,size)
  if role!='tables':assert diag['masked_equal']
  comparisons.append(dict(role=role,symbol=sym,source_size=width,native_size=size,raw=raw,diagnostic=diag))
 assert pins=={p:sha(Path(p))for p in pins}
 return dict(inputs=pins,compiles=compiles,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols,comparisons=comparisons)
def read32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def execute(kind,b,s):
 p=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=p.OPTIONAL_HEADER.ImageBase;size=p.OPTIONAL_HEADER.SizeOfImage
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(0x61000000,0x10000);u.mem_map(STACK,0x10000)
 entry={r:0x400000+rv if kind=='native'else b['symbols'][sym]for r,(sym,rv,n)in PROFILE.items()};width={c['role']:c['source_size']for c in b['comparisons']};code=[(entry[r],entry[r]+(n if kind=='native'else width[r]))for r,(sym,rv,n)in PROFILE.items()]
 pc,bc,block,shade,refresh=(0x96e3f8,0x96e3f4,0x92c490,0x91c488,0x4069a0)if kind=='native'else tuple(b['symbols'][sym]for sym in['_g_exe_96e3f8','_g_exe_96e3f4','_g_host_graphics_tables_52c490','_g_host_graphics_shade_51c488','_graphics_native_069a0'])
 u.mem_write(pc,pack(PAL));u.mem_write(bc,pack(INFO));palette=bytearray((i*7+19)&255 for i in range(4096));bitmap=bytearray((i*11+23)&255 for i in range(1068));out=bytearray(bytes([0x9c])*16)
 index=s.get('index',7);i=index&65535;assert i<256
 palette[4+i*4:8+i*4]=bytes([0x60,0x71,0x83,0x95])
 palette[0x40+4+i*4:0x40+8+i*4]=bytes([0x82,0x90,0xa3,0xb5])
 palette[0x70+4+i*4:0x70+8+i*4]=bytes([0xc2,0xd1,0xe2,0xf5])
 u.mem_write(PAL-32,bytes([0xa1])*32+palette+bytes([0xa2])*32);u.mem_write(INFO-32,bytes([0xb1])*32+bitmap+bytes([0xb2])*32);u.mem_write(OUTPUT-32,bytes([0xc1])*32+out+bytes([0xc2])*32)
 u.mem_write(block,bytes([0x5c])*0x41f40);u.mem_write(shade,bytes([0x6d])*65536);u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP));u.reg_write(UC_X86_REG_ESP,ESP)
 for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
 role=s.get('role','set');args=[index,s.get('red',0x89abcdef),s.get('green',0x76543210),s.get('blue',0xffffffff)]
 if role=='get':
  args=[index,OUTPUT,OUTPUT+1,OUTPUT+2]
  if s.get('alias')=='outputs':args=[index,OUTPUT,OUTPUT,OUTPUT]
  if s.get('alias')=='palette':args=[index,PAL+5+i*4,PAL+6+i*4,OUTPUT]
  if s.get('alias')=='global':args=[index,pc,pc,OUTPUT]
 if role!='tables':u.mem_write(ESP+4,b''.join(pack(x)for x in args))
 expected_palette=palette[:];expected_bitmap=bitmap[:];expected_out=out[:];expected_pc=PAL;expected_args=args[:]
 if role=='set':
  rgb=[args[j]&255 for j in[1,2,3]]
  if i in[0,255]:rgb=[i]*3;expected_args[2]=(args[2]&0xffffff00)|i
  expected_palette[4+i*4:8+i*4]=bytes(rgb+[4]);expected_bitmap[40+i*4:44+i*4]=bytes([rgb[2],rgb[1],rgb[0],0])
 elif role=='get':
  for channel,target in enumerate(args[1:]):
   value=expected_palette[expected_pc-PAL+4+i*4+channel]
   if target==pc:expected_pc=(expected_pc&0xffffff00)|value
   elif PAL<=target<PAL+4096:expected_palette[target-PAL]=value
   else:expected_out[target-OUTPUT]=value
 else:
  for j in range(256):expected_palette[4+j*4:8+j*4]=bytes([j,j,j,4]);expected_bitmap[40+j*4:44+j*4]=bytes([j,j,j,0])
 events=[];illegal=[];writes=[];count=0
 def on_code(machine,a,w,user):
  nonlocal count
  count+=1
  if a==STOP:machine.emu_stop();return
  if a==entry['set']and role=='tables':events.append(dict(call='set',args=[read32(machine,machine.reg_read(UC_X86_REG_ESP)+j)for j in[4,8,12,16]]))
  if a==refresh:
   sp=machine.reg_read(UC_X86_REG_ESP);events.append(dict(call='refresh',arg=read32(machine,sp+4)));machine.reg_write(UC_X86_REG_EAX,0x12345678);machine.reg_write(UC_X86_REG_ESP,sp+4);machine.reg_write(UC_X86_REG_EIP,read32(machine,sp));return
  if not any(lo<=a<hi for lo,hi in code):illegal.append(hex(a));machine.emu_stop()
 def on_write(machine,access,a,w,v,user):
  allowed=[(ESP-0x1000,ESP),(ESP+4,ESP+20),(PAL,PAL+4096),(INFO,INFO+1068),(OUTPUT,OUTPUT+16),(pc,pc+4),(block,block+0x41f40),(shade,shade+65536)]
  if not any(lo<=a and a+w<=hi for lo,hi in allowed):writes.append((hex(a),w));machine.emu_stop()
 u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(entry[role],STOP,count=20000000)
 checks=dict(palette_complete=bytes(u.mem_read(PAL,4096))==bytes(expected_palette),bitmap_complete=bytes(u.mem_read(INFO,1068))==bytes(expected_bitmap),outputs_complete=bytes(u.mem_read(OUTPUT,16))==bytes(expected_out),live_global=read32(u,pc)==expected_pc,bitmap_pointer_unchanged=read32(u,bc)==INFO,caller_slot_effects=role=='tables'or bytes(u.mem_read(ESP+4,16))==b''.join(pack(x)for x in expected_args),sentinel=u.reg_read(UC_X86_REG_EIP)==STOP,stack_balance=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(reg)==value for reg,value in zip(NV,SEEDS)),guards=all(bytes(u.mem_read(a-32,32))==bytes([before])*32 and bytes(u.mem_read(a+n,32))==bytes([after])*32 for a,n,before,after in[(PAL,4096,0xa1,0xa2),(INFO,1068,0xb1,0xb2),(OUTPUT,16,0xc1,0xc2)]),execution_whitelist=not illegal,write_whitelist=not writes)
 if role=='tables':
  want=[dict(call='set',args=[j]*4)for j in range(256)]+[dict(call='refresh',arg=1)];checks['actual_writer_chain']=events==want
 else:checks['tables_untouched']=bytes(u.mem_read(block,0x41f40))==bytes([0x5c])*0x41f40 and bytes(u.mem_read(shade,65536))==bytes([0x6d])*65536
 assert all(checks.values()),(kind,s,checks,illegal,writes)
 return dict(checks=checks,palette_sha256=hashlib.sha256(bytes(u.mem_read(PAL,4096))).hexdigest(),bitmap_sha256=hashlib.sha256(bytes(u.mem_read(INFO,1068))).hexdigest(),outputs=bytes(u.mem_read(OUTPUT,16)).hex(),live_palette=hex(read32(u,pc)),table_sha256=hashlib.sha256(bytes(u.mem_read(block,0x41f40))).hexdigest(),shade_sha256=hashlib.sha256(bytes(u.mem_read(shade,65536))).hexdigest(),events=events,instructions=count)
def main():
 b=build();scenarios=[dict(name='set_'+str(i),index=i)for i in[0,1,7,127,254,255,0x10000,0x10001,0x100ff,0xffff0001]]
 scenarios.extend(dict(name='get_'+str(i),role='get',index=i)for i in[0,1,127,254,255,0x100ff]);scenarios.extend(dict(name='get_alias_'+alias,role='get',index=7,alias=alias)for alias in['outputs','palette','global']);scenarios.append(dict(name='actual_grayscale_table_chain',role='tables'))
 cases=[]
 for s in scenarios:
  n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},s;cases.append(dict(scenario=s,native=n,source=c,equal=True))
 assert b['inputs']=={p:sha(Path(p))for p in b['inputs']};report=dict(scope='Actual original/fresh SDK palette writer/getter and whole table->256writer chain. Refresh69A0 alone modeled; no real device,platform/process or whole game proof.',build=b,cases=cases,case_count=len(cases),all_equal=True);p=OUT/'palette_entries_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,report=str(p)),indent=2))
if __name__=='__main__':main()
