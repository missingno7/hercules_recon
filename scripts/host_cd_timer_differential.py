"""Bounded fresh original/source CD interval callback; APIs and restart modeled."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from coff import COFF
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_cd_timer_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_cd_timer.c';STORAGE=ROOT/'calibration/mci_storage.c';ORACLE=args.native_exe.resolve()
ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
symbol='_host_timing_service_409cd0';CELLS=['_g_exe_96e61c','_g_exe_96e620','_g_exe_96e624','_g_exe_96e628','_g_exe_96e62c','_g_exe_96e630']
STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000;TIME=0x64000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
def read32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def maps(p):
    pattern=re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})')
    return {m.group(1):int(m.group(2),16)for line in p.read_text(errors='replace').splitlines()if (m:=pattern.match(line))}
def build():
    assert sha(ORACLE)==ORACLE_SHA;verify_toolchain(DEFAULT_TOOLCHAIN)
    lib=DEFAULT_TOOLCHAIN/'lib/winmm.lib';files=[SOURCE,STORAGE]
    watched=[*files,ORACLE,lib,Path(__file__),ROOT/'toolchains/manifest.json',ROOT/'scripts/coff.py',ROOT/'scripts/match.py',ROOT/'scripts/masked.py',ROOT/'scripts/pe.py',ROOT/'scripts/env.py']
    pins={str(p):sha(p)for p in watched}
    fixture=OUT/'fixture.c';fixture.write_bytes(b'''#include <windows.h>
extern void __cdecl host_timing_service_409cd0(void);
void __cdecl host_cd_play_9e60(DWORD start,DWORD end) { for (;;) {} }
__declspec(dllexport) void __cdecl timer_fixture_anchor(void) { host_timing_service_409cd0(); }
''');files.append(fixture);compiles=[];objects=[]
    for p in files:
        obj,cmd=compile_source(p,['/O2']);objects.append(obj);compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
    dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),str(lib)]
    p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((p.stdout+p.stderr).encode());assert p.returncode==0,(p.stdout,p.stderr)
    raw=compare(objects[0],symbol,ORACLE,0x9cd0,144);diagnostic=masked_compare(objects[0],symbol,ORACLE,0x9cd0,144);assert diagnostic['masked_equal']
    co=COFF(objects[1]);storage=[]
    native=pefile.PE(str(ORACLE))
    for i,sym in enumerate(CELLS):
        s=next(x for x in co.symbols.values()if x['name']==sym);assert s['storage']==2
        if s['section']==0:
            assert s['value']==4;kind='COMMON'
        else:
            sec=co.sections[s['section']-1];assert not sec['flags']&0x20 and not sec['relocations'] and s['value']+4<=sec['size']
            actual=sec['data'][s['value']:s['value']+4]if sec['data']else bytes(4);assert actual==bytes(4);kind='defined zero data'
        rva=0x56e61c+4*i;t=next(t for t in native.sections if t.VirtualAddress<=rva< t.VirtualAddress+t.Misc_VirtualSize);assert rva>=t.VirtualAddress+t.SizeOfRawData
        storage.append(dict(symbol=sym,rva=hex(rva),bytes=4,kind=kind+' / native loader zero'))
    assert pins=={p:sha(Path(p))for p in pins}
    return dict(inputs=pins,compiles=compiles,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=maps(mp),raw=raw,diagnostic=diagnostic,storage=storage)
def execute(kind,b,s):
    pe=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=pe.OPTIONAL_HEADER.ImageBase;size=pe.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,pe.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(STACK,0x10000);u.mem_map(TIME,0x1000)
    entry=0x409cd0 if kind=='native'else b['symbols'][symbol];width=144 if kind=='native'else b['raw']['compiled_size']
    cells=[0x96e61c+i*4 for i in range(6)]if kind=='native'else[b['symbols'][x]for x in CELLS]
    play=0x409e60 if kind=='native'else b['symbols']['_host_cd_play_9e60']
    imported=[i for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports if i.name==b'timeGetTime'];assert len(imported)==1
    u.mem_write(imported[0].address,pack(TIME))
    initial=[s['start'],s['end'],s['previous'],0x93a5b7d9,s.get('playing',0x72d4a6b8),0x51c3e5a7]
    for a,v in zip(cells,initial):u.mem_write(a,pack(v))
    u.mem_write(ESP-0x200,bytes([0xa7])*0x300);u.mem_write(ESP,pack(STOP));u.reg_write(UC_X86_REG_ESP,ESP)
    for reg,v in zip(NV,SEEDS):u.reg_write(reg,v)
    events=[];illegal=[];writes=[];instructions=0
    def ret(sp,extra,result):
        u.reg_write(UC_X86_REG_EAX,result&0xffffffff);u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,read32(u,sp))
    def on_code(machine,a,w,user):
        nonlocal instructions
        instructions+=1
        if instructions>300:illegal.append('instruction cap');machine.emu_stop();return
        if a==STOP:machine.emu_stop();return
        sp=machine.reg_read(UC_X86_REG_ESP)
        if a==TIME:
            events.append(dict(call='timeGetTime'))
            for key,index in [('mutate_start',0),('mutate_end',1),('mutate_previous',2)]:
                if key in s:machine.mem_write(cells[index],pack(s[key]))
            ret(sp,0,s['now']);return
        if a==play:
            events.append(dict(call='play',start=read32(machine,sp+4),end=read32(machine,sp+8),playing_at_entry=read32(machine,cells[4])))
            # Deliberately visible effects verify ordering and tail preservation;
            # they are fixture behavior, not a recovered 9E60 implementation.
            machine.mem_write(cells[4],pack(0x12345678));machine.mem_write(cells[2],pack(0xfedcba98));ret(sp,0,0x89abcdef);return
        if not entry<=a<entry+width:illegal.append(hex(a));machine.emu_stop()
    def on_write(machine,access,a,w,v,user):
        if not (ESP-0x200<=a and a+w<=ESP+4 or a==cells[4] and w==4):writes.append((hex(a),w));machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(entry,STOP,count=301)
    expected=initial[:];expected_events=[]
    if initial[0] and initial[1]:
        expected_events.append(dict(call='timeGetTime'))
        for key,index in [('mutate_start',0),('mutate_end',1),('mutate_previous',2)]:
            if key in s:expected[index]=s[key]
        ms=lambda x:((x&255)+((x>>8)&255)*60)*1000
        duration=(ms(initial[1])-ms(initial[0]))&0xffffffff;elapsed=(s['now']-expected[2])&0xffffffff
        if elapsed>=duration:
            expected_events.append(dict(call='play',start=expected[0],end=expected[1],playing_at_entry=0));expected[4]=0x12345678;expected[2]=0xfedcba98
    actual=[read32(u,a)for a in cells]
    checks=dict(all_six_cells_equal=actual==expected,event_order_arguments_equal=events==expected_events,sentinel_return=u.reg_read(UC_X86_REG_EIP)==STOP,stack_balance=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(NV,SEEDS)),stack_guards=bytes(u.mem_read(ESP-0x200,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4,32))==bytes([0xa7])*32,execution_whitelist=not illegal,write_whitelist=not writes)
    assert all(checks.values()),(kind,s,checks,events,actual,expected)
    return dict(cells=actual,events=events,checks=checks,instructions=instructions)
def main():
    b=build();cases=[]
    scenarios=[dict(name='zero_start',start=0,end=0x102,previous=0,now=100000),dict(name='zero_end',start=0x101,end=0,previous=0,now=100000),dict(name='both_zero',start=0,end=0,previous=0,now=0xffffffff)]
    for name,start,end,previous,values in [('one_second',0x101,0x102,100,[999,1000,1001]),('minute_boundary',0x3b,0x100,0,[999,1000,1001]),('zero_interval',0x101,0x101,0,[0,1]),('reversed_unsigned',0x102,0x101,0,[0,0xfffffc17,0xfffffc18,0xffffffff]),('wrapped_clock',0x101,0x102,0xffffff00,[999,1000,1001]),('high_bytes',0xaabb0101,0xccdd0102,19,[999,1000]),('max_fields',0x1,0xffff,0,[15553999,15554000,15554001])]:
        for elapsed in values:scenarios.append(dict(name=name+'_'+str(elapsed),start=start,end=end,previous=previous,now=(previous+elapsed)&0xffffffff))
    scenarios.extend([dict(name='live_restart_reload',start=0x101,end=0x102,previous=0,now=1000,mutate_start=0x88880304,mutate_end=0x99990506),dict(name='live_tick_reload',start=0x101,end=0x102,previous=0,now=1000,mutate_previous=1000),dict(name='snapshot_threshold_live_arguments',start=0x101,end=0x102,previous=0,now=1000,mutate_start=0,mutate_end=0),dict(name='playing_zero_does_not_disable',start=0x101,end=0x102,previous=0,now=1000,playing=0)])
    for s in scenarios:
        n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'}
        cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual original/fresh source144-byte CD interval callback. API tick and restart9E60 effects modeled offline; no real process/API/renderer or whole gameplay.',build=b,cases=cases,case_count=len(cases),all_equal=True)
    p=OUT/'host_cd_timer_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,report=str(p)),indent=2))
if __name__=='__main__':main()
