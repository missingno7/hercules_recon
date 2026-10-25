"""Fresh CD playback/timer with actual cleanup/readiness; modeled API/CRT boundaries."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from coff import COFF
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare,_compare_section
from pe import PE
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_cd_play_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_cd_play.c';ORACLE=args.native_exe.resolve()
ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={'play':('_host_cd_play_9e60',0x9e60,256),'timer':('_host_timing_service_409cd0',0x9cd0,144),'cleanup':('_cleanup_native_9e10',0x9e10,80),'readiness':('_cleanup_native_9c30',0x9c30,160)}
CELLS=['_g_exe_96e61c','_g_exe_96e620','_g_exe_96e624','_g_exe_96e628','_g_exe_96e62c','_g_exe_96e630']
STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000;HOOKS=0x64000000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x11223344,0x22334455,0x33445566,0x44556677]
def read32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def readstr(u,a):
    result=bytearray()
    for i in range(256):
        x=bytes(u.mem_read(a+i,1))
        if x==b'\0':return result.decode('ascii')
        result.extend(x)
    raise AssertionError('fixture string exceeds256 bytes')
def maps(p):
    pattern=re.compile(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})')
    return {m.group(1):int(m.group(2),16)for line in p.read_text(errors='replace').splitlines()if(m:=pattern.match(line))}
def members(data):
    assert data[:8]==b'!<arch>\n';position=8;index=0
    while position<len(data):
        header=data[position:position+60];assert len(header)==60 and header[58:]==b'`\n'
        size=int(header[48:58]);start=position+60;payload=data[start:start+size];assert len(payload)==size
        yield index,payload;position=start+size+(size&1);index+=1
def build():
    assert sha(ORACLE)==ORACLE_SHA;tool=verify_toolchain(DEFAULT_TOOLCHAIN)
    files=[SOURCE,*[ROOT/p for p in ['src/pc/host_cd_timer.c','src/pc/mci_storage.c','src/pc/audio_cleanup_dependencies.c','src/pc/mci_readiness.cpp','src/shared/registry_storage.c','src/pc/graphics_storage.cpp','src/pc/sound_bank_storage.cpp']]]
    libraries=[DEFAULT_TOOLCHAIN/'lib'/p for p in ['libcmt.lib','winmm.lib','kernel32.lib','user32.lib']]
    map_path=Path('D:/Prog/hercules_recon/work/library_map/library_map.json')
    watched=[*files,*libraries,ORACLE,map_path,Path(__file__),*[ROOT/p for p in ['scripts/coff.py','scripts/match.py','scripts/masked.py','scripts/pe.py','scripts/env.py','toolchains/manifest.json']],DEFAULT_TOOLCHAIN/'include/mmsystem.h']
    pins={str(p):sha(p)for p in watched}
    assert sha(libraries[0])=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    library_map=json.loads(map_path.read_bytes())['contributions']['HERCULES.EXE'];contracts=[]
    for index,payload in members(libraries[0].read_bytes()):
        if index not in [419,460]:continue
        symbol,rva,width=('_sprintf',0x10f40,112)if index==419 else('__stricmp',0x1c270,208)
        row=next(r for r in library_map if int(r['rva'],0)==rva);assert row['member']==index and row['size']==width and symbol in row['names']
        path=OUT/('crt_member'+str(index)+'.obj');path.write_bytes(payload);co=COFF(path)
        number,section=next((n,s)for n,s in enumerate(co.sections,1)if s['flags']&0x20 and s['size']);assert section['size']==width
        diagnostic=_compare_section(co,section,number,PE(ORACLE),rva,width);assert diagnostic['masked_equal']
        contracts.append(dict(symbol=symbol,member=index,rva=hex(rva),size=width,member_sha256=sha(path),complete_code_diagnostic=diagnostic,scope='Unmodified identified libcmt member; private whole-section diagnostic permits the existing alias names. No function/layout acceptance; CRT body is not executed in these cases.'))
    fixture=OUT/'fixture.c';fixture.write_bytes(b'''#include <windows.h>
extern void __cdecl host_cd_play_9e60(DWORD,DWORD);
extern void __cdecl host_timing_service_409cd0(void);
void __cdecl cleanup_native_f710(unsigned int index) { for (;;) {} }
int __cdecl main(void) { for (;;) {} }
__declspec(dllexport) void __cdecl play_fixture_anchor(void) { host_cd_play_9e60(0,0); }
__declspec(dllexport) void __cdecl timer_fixture_anchor(void) { host_timing_service_409cd0(); }
''');files.append(fixture);objects=[];compiles=[];defs={}
    for p in files:
        obj,cmd=compile_source(p,['/O2']);objects.append(obj);co=COFF(obj);compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=cmd))
        for s in co.symbols.values():
            if s['storage']==2 and s['section']>0 and s['type']&0x20:
                assert s['name']not in defs;defs[s['name']]=(obj,co.function(s['name'])['size'])
    dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),*map(str,libraries)]
    p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((p.stdout+p.stderr).encode())
    if p.returncode:
        (OUT/'failed_link.json').write_bytes((json.dumps(dict(inputs=pins,compiles=compiles,command=cmd,returncode=p.returncode,output=p.stdout+p.stderr),indent=2)+'\n').encode())
    assert p.returncode==0,(p.stdout,p.stderr)
    symbols=maps(mp);comparisons=[]
    for role,(symbol,rva,width)in PROFILE.items():
        obj,size=defs[symbol];raw=compare(obj,symbol,ORACLE,rva,width);diagnostic=masked_compare(obj,symbol,ORACLE,rva,width)
        if role!='play':assert diagnostic['masked_equal']
        comparisons.append(dict(role=role,symbol=symbol,source_size=size,native_size=width,raw=raw,diagnostic=diagnostic))
    needed=[r[0]for r in PROFILE.values()]+CELLS+['_g_registry_config','_g_exe_91c46c','_sprintf','__stricmp'];assert all(s in symbols for s in needed)
    native=pefile.PE(str(ORACLE));data=[]
    for symbol,owner,rva,width in [(sym,'mci_storage.c',0x56e61c+4*i,4)for i,sym in enumerate(CELLS)]+[('_g_registry_config','registry_storage.c',0x79e858,72),('_g_exe_91c46c','graphics_storage.cpp',0x51c46c,4)]:
        providers=[]
        for receipt in compiles:
            co=COFF(receipt['object'])
            for s in co.symbols.values():
                if s['name']==symbol and s['storage']==2 and (s['section']>0 or s['section']==0 and s['value']>0):providers.append((receipt,co,s))
        assert len(providers)==1;receipt,co,s=providers[0];assert Path(receipt['source']).name==owner
        if s['section']==0:assert s['value']==width;actual=bytes(width);kind='COMMON'
        else:
            sec=co.sections[s['section']-1];assert not sec['flags']&0x20 and not sec['relocations'] and s['value']+width<=sec['size']
            actual=sec['data'][s['value']:s['value']+width]if sec['data']else bytes(width);kind='BSS'if sec['flags']&0x80 else'initialized'
        t=next(t for t in native.sections if t.VirtualAddress<=rva and rva+width<=t.VirtualAddress+t.Misc_VirtualSize)
        if rva>=t.VirtualAddress+t.SizeOfRawData:assert actual==bytes(width);backing='virtual_loader_zero'
        else:assert actual==native.get_data(rva,width);backing='initialized'
        data.append(dict(symbol=symbol,owner=str(Path(receipt['source']).relative_to(ROOT)),rva=hex(rva),bytes=width,coff_kind=kind,native_backing=backing))
    assert pins=={p:sha(Path(p))for p in pins}
    return dict(inputs=pins,toolchain=tool,compiles=compiles,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),symbols=symbols,comparisons=comparisons,data=data,crt_contracts=contracts)

class Protocol:
    """Scripted OS/CRT effects shared by emulator and independent state reference."""
    def __init__(self,scenario,get,setvalue):self.s=scenario;self.get=get;self.set=setvalue;self.events=[];self.counts={}
    def action(self,role,default):
        i=self.counts.get(role,0);self.counts[role]=i+1;items=self.s.get(role,[]);a=items[i]if i<len(items)else default
        for key,value in a.get('mutate',{}).items():self.set(key,value)
        return a
    def mci(self,command,width):
        roles={'open cdaudio shareable wait':'open','status cdaudio media present':'media','status cdaudio ready':'ready','stop cdaudio wait':'stop','set cdaudio time format tmsf':'set_format'}
        role=roles.get(command,'play');assert role!='play'or command.startswith('play cdaudio from ')
        a=self.action(role,dict(code=0,response='true'))
        self.events.append(dict(call='mci',role=role,command=command,width=width,code=a.get('code',0),response=a.get('response','true')if width else None))
        return a.get('code',0),a.get('response','true')
    def format(self,fmt,a,b):
        assert fmt=='play cdaudio from %d to %d';value=fmt%(a,b);self.action('format',{})
        self.events.append(dict(call='sprintf',format=fmt,values=[a,b],text=value));return value
    def compare(self,a,b):
        # Fixture domain is ASCII MCI true/false strings only; locale is not proved.
        result=0 if a.lower()==b.lower()else 1;self.events.append(dict(call='stricmp',a=a,b=b,result=result));return result
    def sleep(self,ms):
        assert ms==100;self.action('sleep',{});self.events.append(dict(call='Sleep',milliseconds=ms))
    def time(self):
        a=self.action('time',dict(value=self.s.get('now',0x87654321)));v=a.get('value',self.s.get('now',0x87654321));self.events.append(dict(call='timeGetTime',value=v));return v

def initial(s):
    cells=[0x11223344,0x55667788,0x91a2b3c4,0x93a5b7d9,s.get('playing',0),s.get('opened',1)]
    if s.get('entry')=='timer':cells[:3]=[s.get('start',0x030101),s.get('end',0x030102),s.get('previous',0)]
    registry=bytearray((i*13+7)&255 for i in range(72));registry[13]=s.get('enabled',1)
    return cells,registry,0xabcdef01
def reference(s):
    cells,registry,reset=initial(s);state=dict(zip(['start','end','previous','paused','playing','opened'],cells));state['enabled']=registry[13];state['reset']=reset
    get=lambda k:state[k]
    def put(k,v):assert k in state;state[k]=v&0xffffffff
    p=Protocol(s,get,put)
    def ready():
        if not state['opened']:state['opened']=int(p.mci('open cdaudio shareable wait',0)[0]==0)
        if not state['opened']:return False
        code,text=p.mci('status cdaudio media present',256)
        if code or p.compare(text,'true'):return False
        code,text=p.mci('status cdaudio ready',256)
        return not code and p.compare(text,'true')==0
    def play(start,end):
        if state['playing'] and ready():p.mci('stop cdaudio wait',0)
        for k in ['playing','end','start','paused','previous']:state[k]=0
        state['start']=start;state['end']=end
        for attempt in range(2):
            if state['enabled']!=1 or not ready()or state['playing']:break
            p.mci('set cdaudio time format tmsf',0)
            start,end=state['start'],state['end'];track=(start>>16)&255
            from_value=track|((start>>8)&255)<<8|(start&255)<<16
            to_value=track|((end>>8)&255)<<8|(end&255)<<16
            command=p.format('play cdaudio from %d to %d',from_value,to_value)
            state['playing']=int(p.mci(command,0)[0]==0)
            if not state['playing']:p.sleep(100)
        state['previous']=p.time();state['reset']=1
    if s.get('entry')=='timer':
        start,end=state['start'],state['end']
        if start and end:
            ms=lambda x:((x&255)+((x>>8)&255)*60)*1000
            duration=(ms(end)-ms(start))&0xffffffff;now=p.time()
            if ((now-state['previous'])&0xffffffff)>=duration:
                state['playing']=0;play(state['start'],state['end'])
    else:play(s.get('start',0x030101),s.get('end',0x070202))
    registry[13]=state['enabled']&255
    return dict(cells=[state[k]for k in ['start','end','previous','paused','playing','opened']],registry_hex=registry.hex(),reset=state['reset'],events=p.events)

def execute(kind,b,s):
    pe=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=pe.OPTIONAL_HEADER.ImageBase;size=pe.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,pe.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(STACK,0x10000);u.mem_map(HOOKS,0x1000)
    addresses={role:0x400000+rva if kind=='native'else b['symbols'][sym]for role,(sym,rva,n)in PROFILE.items()}
    widths={r:c['source_size']for c in b['comparisons']for r in[c['role']]}
    code=[(addresses[r],addresses[r]+(n if kind=='native'else widths[r]))for r,(sym,rva,n)in PROFILE.items()]
    cells=[0x96e61c+i*4 for i in range(6)]if kind=='native'else[b['symbols'][sym]for sym in CELLS]
    registry,reset=(0xb9e858,0x91c46c)if kind=='native'else(b['symbols']['_g_registry_config'],b['symbols']['_g_exe_91c46c'])
    endpoints={};api_addresses={}
    for i,name in enumerate(['mciSendStringA','Sleep','timeGetTime']):
        address=HOOKS+i*16;api_addresses[name]=address;endpoints[address]=name
        imports=[x for d in pe.DIRECTORY_ENTRY_IMPORT for x in d.imports if x.name==name.encode()];assert len(imports)==1;u.mem_write(imports[0].address,pack(address))
    endpoints[0x410f40 if kind=='native'else b['symbols']['_sprintf']]='sprintf'
    endpoints[0x41c270 if kind=='native'else b['symbols']['__stricmp']]='stricmp'
    vals,reg,reset_value=initial(s)
    for a,v in zip(cells,vals):u.mem_write(a,pack(v))
    u.mem_write(registry,bytes(reg));u.mem_write(reset,pack(reset_value));u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP))
    if s.get('entry')!='timer':u.mem_write(ESP+4,pack(s.get('start',0x030101))+pack(s.get('end',0x070202)))
    u.reg_write(UC_X86_REG_ESP,ESP)
    for r,v in zip(NV,SEEDS):u.reg_write(r,v)
    keys=dict(zip(['start','end','previous','paused','playing','opened'],cells));keys['reset']=reset
    def get(k):return u.mem_read(registry+13,1)[0]if k=='enabled'else read32(u,keys[k])
    def put(k,v):
        assert k=='enabled'or k in keys
        u.mem_write(registry+13,bytes([v&255]))if k=='enabled'else u.mem_write(keys[k],pack(v))
    p=Protocol(s,get,put);instructions=0;illegal=[];writes=[];roles=[]
    def ret(sp,extra,value):
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,read32(u,sp))
    def on_code(machine,a,w,user):
        nonlocal instructions
        instructions+=1
        if instructions>3000:illegal.append('instruction cap');machine.emu_stop();return
        if a==STOP:machine.emu_stop();return
        if a in addresses.values():roles.append(next(r for r,v in addresses.items()if v==a))
        sp=machine.reg_read(UC_X86_REG_ESP);role=endpoints.get(a)
        if role=='mciSendStringA':
            command=readstr(machine,read32(machine,sp+4));target=read32(machine,sp+8);width=read32(machine,sp+12);assert read32(machine,sp+16)==0
            assert width in[0,256]and bool(target)==bool(width);result,text=p.mci(command,width)
            if width:
                assert ESP-0x1000<=target and target+width<=ESP;payload=text.encode()+b'\0';assert len(payload)<=width;machine.mem_write(target,payload)
            ret(sp,16,result);return
        if role=='sprintf':
            target=read32(machine,sp+4);fmt=readstr(machine,read32(machine,sp+8));a,b=read32(machine,sp+12),read32(machine,sp+16)
            assert a<0x1000000 and b<0x1000000;value=p.format(fmt,a,b);payload=value.encode()+b'\0';assert ESP-0x1000<=target and target+len(payload)<=ESP
            machine.mem_write(target,payload);ret(sp,0,len(value));return
        if role=='stricmp':ret(sp,0,p.compare(readstr(machine,read32(machine,sp+4)),readstr(machine,read32(machine,sp+8))));return
        if role=='Sleep':p.sleep(read32(machine,sp+4));ret(sp,4,0x11112222);return
        if role=='timeGetTime':ret(sp,0,p.time());return
        if not any(lo<=a<hi for lo,hi in code):illegal.append(hex(a));machine.emu_stop()
    def on_write(machine,access,a,w,v,user):
        allowed=[(ESP-0x1000,ESP),(reset,reset+4),*[(a,a+4)for a in cells]]
        if not any(lo<=a and a+w<=hi for lo,hi in allowed):writes.append((hex(a),w));machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
    entry=addresses['timer'if s.get('entry')=='timer'else'play'];u.emu_start(entry,STOP,count=3001)
    actual=dict(cells=[read32(u,a)for a in cells],registry_hex=bytes(u.mem_read(registry,72)).hex(),reset=read32(u,reset),events=p.events)
    expected=reference(s);checks=dict(full_state_equal=actual==expected,sentinel_return=u.reg_read(UC_X86_REG_EIP)==STOP,stack_balance=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(r)==v for r,v in zip(NV,SEEDS)),stack_guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+12,32))==bytes([0xa7])*32,arguments_preserved=s.get('entry')=='timer'or bytes(u.mem_read(ESP+4,8))==pack(s.get('start',0x030101))+pack(s.get('end',0x070202)),execution_whitelist=not illegal,write_whitelist=not writes)
    assert all(checks.values()),(kind,s,checks,actual,expected,illegal,writes)
    return dict(state=actual,checks=checks,executed_provider_entries=roles,instructions=instructions)

def scenarios():
    cases=[dict(name='enabled_'+str(v),enabled=v)for v in[0,1,2,0x7f,0x80,0xff]]
    cases.extend([dict(name='second_try_success',play=[dict(code=7),dict(code=0)]),dict(name='both_attempts_fail',play=[dict(code=7),dict(code=0x80000000)]),dict(name='open_success',opened=0),dict(name='open_failure',opened=0,open=[dict(code=5)]),dict(name='media_api_failure',media=[dict(code=1)]),dict(name='media_false',media=[dict(code=0,response='false')]),dict(name='ready_api_failure',ready=[dict(code=2)]),dict(name='ready_false',ready=[dict(code=0,response='false')]),dict(name='uppercase_true',media=[dict(code=0,response='TRUE')],ready=[dict(code=0,response='TrUe')]),dict(name='cleanup_then_play',playing=0xffffffff),dict(name='cleanup_open_retry',playing=1,opened=0,open=[dict(code=5),dict(code=0)]),dict(name='cleanup_stop_failure_ignored',playing=7,stop=[dict(code=0xffffffff)]),dict(name='set_format_failure_ignored',set_format=[dict(code=4)]),dict(name='distinct_endpoint_tracks',start=0xaa7b0102,end=0xcc990304),dict(name='maximum_packed_bytes',start=0xffffffff,end=0x00010203),dict(name='zero_endpoints',start=0,end=0),dict(name='disable_during_sleep',play=[dict(code=3)],sleep=[dict(mutate=dict(enabled=0))]),dict(name='second_ready_failure_after_success',ready=[dict(code=0),dict(code=8)]),dict(name='ready_sets_playing',ready=[dict(code=0,mutate=dict(playing=0x80000000))]),dict(name='set_format_live_endpoints',set_format=[dict(code=0,mutate=dict(start=0x88fe0708,end=0x99220304))]),dict(name='format_mutates_next_retry_endpoints',play=[dict(code=7),dict(code=0)],format=[dict(mutate=dict(start=0x00090102,end=0x000a0304))]),dict(name='clock_mutation_final_store',time=[dict(value=0xffffffff,mutate=dict(previous=13,reset=0,start=0x12345678,paused=77))])])
    cases.extend([dict(name='timer_before_threshold',entry='timer',start=0x030101,end=0x030102,previous=100,time=[dict(value=1099)]),dict(name='timer_threshold_to_play',entry='timer',start=0x030101,end=0x070102,previous=100,time=[dict(value=1100),dict(value=5678)]),dict(name='timer_wrap_to_retry',entry='timer',start=0x030101,end=0x070102,previous=0xffffff00,time=[dict(value=744),dict(value=9999)],play=[dict(code=5),dict(code=0)]),dict(name='timer_live_endpoint_reload',entry='timer',start=0x030101,end=0x070102,previous=0,time=[dict(value=1000,mutate=dict(start=0x090304,end=0x0a0506)),dict(value=8888)]),dict(name='timer_live_previous_reload',entry='timer',start=0x030101,end=0x070102,previous=0,time=[dict(value=1000,mutate=dict(previous=1000))]),dict(name='timer_zero_start',entry='timer',start=0,end=0x070102),dict(name='timer_zero_end',entry='timer',start=0x030101,end=0)])
    return cases
def main():
    b=build();cases=[]
    for s in scenarios():
        n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},s
        cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual fresh/native CD play9E60, cleanup9E10, readiness9C30 and timer9CD0; synthetic OS MCI/tick/Sleep and identified CRT sprintf/ASCII stricmp boundaries. No real process/API/CRT execution or startup/gameplay acceptance.',build=b,cases=cases,case_count=len(cases),all_equal=True)
    p=OUT/'host_cd_play_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,report=str(p)),indent=2))
if __name__=='__main__':main()
