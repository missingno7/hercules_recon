"""Fresh startup CD validation/retry with actual readiness; modeled API/CRT boundaries."""
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
parser.add_argument('--output',type=Path,default=ROOT/'work/host_cd_startup_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_cd_startup.c';ORACLE=args.native_exe.resolve()
ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={'startup':('_host_service_a0b0',0xa0b0,112),'validate':('_host_cd_validate_10520',0x10520,368),'readiness':('_cleanup_native_9c30',0x9c30,160)}
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
    files=[SOURCE,*[ROOT/p for p in ['calibration/mci_storage.c','calibration/mci_readiness.cpp','calibration/host_startup_storage.c','calibration/fatal_storage.c']]]
    libraries=[DEFAULT_TOOLCHAIN/'lib'/p for p in ['libcmt.lib','winmm.lib','kernel32.lib','user32.lib']]
    map_path=Path('D:/Prog/hercules_recon/work/library_map/library_map.json')
    watched=[*files,*libraries,ORACLE,map_path,Path(__file__),*[ROOT/p for p in ['scripts/coff.py','scripts/match.py','scripts/masked.py','scripts/pe.py','scripts/env.py','toolchains/manifest.json']],DEFAULT_TOOLCHAIN/'include/mmsystem.h',ROOT/'calibration/host_startup_storage.h']
    pins={str(p):sha(p)for p in watched}
    assert sha(libraries[0])=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    library_map=json.loads(map_path.read_bytes())['contributions']['HERCULES.EXE'];contracts=[]
    for index,payload in members(libraries[0].read_bytes()):
        if index not in [419,420,460]:continue
        symbol,rva,width={419:('_sprintf',0x10f40,112),420:('_sscanf',0x11e50,80),460:('__stricmp',0x1c270,208)}[index]
        row=next(r for r in library_map if int(r['rva'],0)==rva);assert row['member']==index and row['size']==width and symbol in row['names']
        path=OUT/('crt_member'+str(index)+'.obj');path.write_bytes(payload);co=COFF(path)
        number,section=next((n,s)for n,s in enumerate(co.sections,1)if s['flags']&0x20 and s['size']);assert section['size']==width
        diagnostic=_compare_section(co,section,number,PE(ORACLE),rva,width);assert diagnostic['masked_equal']
        contracts.append(dict(symbol=symbol,member=index,rva=hex(rva),size=width,member_sha256=sha(path),complete_code_diagnostic=diagnostic,scope='Unmodified identified libcmt member; private whole-section diagnostic permits the existing alias names. No function/layout acceptance; CRT body is not executed in these cases.'))
    fixture=OUT/'fixture.c';fixture.write_bytes(b'''#include <windows.h>
extern void __cdecl host_service_a0b0(void);
extern void __cdecl host_cd_validate_10520(void);
int __cdecl main(void) { for (;;) {} }
__declspec(dllexport) void __cdecl startup_fixture_anchor(void) { host_service_a0b0(); }
__declspec(dllexport) void __cdecl validate_fixture_anchor(void) { host_cd_validate_10520(); }
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
        assert diagnostic['masked_equal']
        comparisons.append(dict(role=role,symbol=symbol,source_size=size,native_size=width,raw=raw,diagnostic=diagnostic))
    needed=[r[0]for r in PROFILE.values()]+CELLS+['_g_host_input_gate_rva_03a594','_g_fatal_owner_window','_sprintf','_sscanf','__stricmp'];assert all(s in symbols for s in needed)
    native=pefile.PE(str(ORACLE));data=[]
    for symbol,owner,rva,width in [(sym,'mci_storage.c',0x56e61c+4*i,4)for i,sym in enumerate(CELLS)]+[('_g_host_input_gate_rva_03a594','host_startup_storage.c',0x3a594,1),('_g_fatal_owner_window','fatal_storage.c',0x7a0200,4)]:
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
    def __init__(self,s,get,put):self.s=s;self.get=get;self.put=put;self.counts={};self.events=[]
    def action(self,role,default):
        i=self.counts.get(role,0);self.counts[role]=i+1;items=self.s.get(role,[]);a=items[i]if i<len(items)else default
        for k,v in a.get('mutate',{}).items():self.put(k,v)
        return a
    def mci(self,command,width,same_buffer):
        roles={'open cdaudio shareable wait':('open',''),'status cdaudio media present':('media','true'),'status cdaudio ready':('ready','true'),'status cdaudio number of tracks wait':('tracks','2'),'set cdaudio time format tmsf':('set_format',''),'status cdaudio type track 1 wait':('type1','data'),'status cdaudio type track 2 wait':('type2','audio'),'status cdaudio length track 2 wait':('length2','34:12:08')}
        assert command in roles,command;role,response=roles[command];a=self.action(role,dict(code=0,response=response))
        self.events.append(dict(call='mci',role=role,command=command,width=width,same_buffer=same_buffer,code=a.get('code',0),response=a.get('response',response)if width else None))
        return a.get('code',0),a.get('response',response)
    def format(self,fmt,track):
        assert fmt in['status cdaudio type track %u wait','status cdaudio length track %u wait'];assert track in[1,2]
        text=fmt%track;self.action('format',{});self.events.append(dict(call='sprintf',format=fmt,track=track,text=text));return text
    def compare(self,a,b):
        result=0 if a.lower()==b.lower()else 1;self.events.append(dict(call='stricmp',a=a,b=b,result=result));return result
    def scan(self,text,fmt):
        assert fmt in['%u','%u:%u:%u'];values=[];position=0
        for i in range(1 if fmt=='%u'else 3):
            if i:
                if position>=len(text)or text[position]!=':':break
                position+=1
            m=re.match(r'\s*([+-]?\d+)',text[position:])
            if not m:break
            value=int(m.group(1));assert -0xffffffff<=value<=0xffffffff
            values.append(value&0xffffffff);position+=m.end()
        result=len(values)if values or text.strip()else -1
        a=self.action('scan',{});assert not a,'Only concrete bounded ASCII scan effects are modeled.'
        self.events.append(dict(call='sscanf',text=text,format=fmt,values=values,result=result));return result,values
    def message(self,owner,text,title,flags):
        assert text=='To play Hercules you must close all other CD applications and insert the game CD.'and title=='Hercules Notice'and flags==0x35
        a=self.action('message',dict(result=2));result=a.get('result',2)
        self.events.append(dict(call='MessageBoxA',owner=owner,text=text,title=title,flags=flags,result=result));return result

def initial(s):return [0x11223344,0x55667788,0x91a2b3c4,0x93a5b7d9,0x72d4a6b8,s.get('opened',1)],s.get('gate',0xff),0x13572468
def reference(s):
    cells,gate,owner=initial(s);state=dict(zip(['start','end','previous','paused','playing','opened'],cells));state.update(gate=gate,owner=owner)
    def put(k,v):assert k in state;state[k]=v& (255 if k=='gate'else 0xffffffff)
    p=Protocol(s,lambda k:state[k],put)
    def ready():
        if not state['opened']:state['opened']=int(p.mci('open cdaudio shareable wait',0,False)[0]==0)
        if not state['opened']:return False
        code,text=p.mci('status cdaudio media present',256,False)
        if code or p.compare(text,'true'):return False
        code,text=p.mci('status cdaudio ready',256,False)
        return not code and p.compare(text,'true')==0
    def validate():
        state['gate']=0
        if not ready():return
        code,text=p.mci('status cdaudio number of tracks wait',256,False)
        if code:return
        count,values=p.scan(text,'%u')
        if count!=1 or values[0]!=2:return
        for track in[1,2]:
            if p.mci('set cdaudio time format tmsf',0,False)[0]:return
            command=p.format('status cdaudio type track %u wait',track);code,text=p.mci(command,256,True)
            if code:return
            audio=p.compare(text,'audio')==0
            if audio!=(track!=1):return
            if track==2:
                command=p.format('status cdaudio length track %u wait',track);code,text=p.mci(command,256,True)
                if code:return
                count,values=p.scan(text,'%u:%u:%u')
                if count!=3 or values[0]!=34:return
        state['gate']=1
    if s.get('entry','validate')=='startup':
        for k in['playing','paused','previous','end','start','opened']:state[k]=0
        for iteration in range(8):
            previous_ready=ready();validate()
            if previous_ready and state['gate']:break
            if p.message(state['owner'],'To play Hercules you must close all other CD applications and insert the game CD.','Hercules Notice',0x35)==2:break
        else:raise AssertionError('unbounded synthetic retry script')
    else:validate()
    return dict(cells=[state[k]for k in['start','end','previous','paused','playing','opened']],gate=state['gate'],owner=state['owner'],events=p.events)

def execute(kind,b,s):
    pe=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=pe.OPTIONAL_HEADER.ImageBase;size=pe.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,pe.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(STACK,0x10000);u.mem_map(HOOKS,0x1000)
    addresses={role:0x400000+rva if kind=='native'else b['symbols'][sym]for role,(sym,rva,n)in PROFILE.items()};widths={c['role']:c['source_size']for c in b['comparisons']}
    code=[(addresses[r],addresses[r]+(n if kind=='native'else widths[r]))for r,(sym,rva,n)in PROFILE.items()]
    cells=[0x96e61c+i*4 for i in range(6)]if kind=='native'else[b['symbols'][sym]for sym in CELLS]
    gate,owner=(0x43a594,0xba0200)if kind=='native'else(b['symbols']['_g_host_input_gate_rva_03a594'],b['symbols']['_g_fatal_owner_window'])
    endpoints={}
    for i,name in enumerate(['mciSendStringA','MessageBoxA']):
        address=HOOKS+i*16;endpoints[address]=name;imports=[x for d in pe.DIRECTORY_ENTRY_IMPORT for x in d.imports if x.name==name.encode()];assert len(imports)==1;u.mem_write(imports[0].address,pack(address))
    for symbol,native,name in[('_sprintf',0x410f40,'sprintf'),('_sscanf',0x411e50,'sscanf'),('__stricmp',0x41c270,'stricmp')]:endpoints[native if kind=='native'else b['symbols'][symbol]]=name
    vals,g,o=initial(s)
    for a,v in zip(cells,vals):u.mem_write(a,pack(v))
    u.mem_write(gate,bytes([g]));u.mem_write(owner,pack(o));u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP));u.reg_write(UC_X86_REG_ESP,ESP)
    for r,v in zip(NV,SEEDS):u.reg_write(r,v)
    keys=dict(zip(['start','end','previous','paused','playing','opened'],cells));keys['owner']=owner
    def get(k):return u.mem_read(gate,1)[0]if k=='gate'else read32(u,keys[k])
    def put(k,v):
        assert k=='gate'or k in keys
        u.mem_write(gate,bytes([v&255]))if k=='gate'else u.mem_write(keys[k],pack(v))
    p=Protocol(s,get,put);instructions=0;illegal=[];writes=[];roles=[]
    def ret(sp,extra,value):
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,read32(u,sp))
    def on_code(machine,a,w,user):
        nonlocal instructions
        instructions+=1
        if instructions>10000:illegal.append('instruction cap');machine.emu_stop();return
        if a==STOP:machine.emu_stop();return
        if a in addresses.values():roles.append(next(r for r,v in addresses.items()if v==a))
        sp=machine.reg_read(UC_X86_REG_ESP);role=endpoints.get(a)
        if role=='mciSendStringA':
            cmd=read32(machine,sp+4);command=readstr(machine,cmd);target=read32(machine,sp+8);width=read32(machine,sp+12);assert read32(machine,sp+16)==0
            assert width in[0,256]and bool(target)==bool(width);result,text=p.mci(command,width,cmd==target)
            if width:
                assert ESP-0x1000<=target and target+width<=ESP;payload=text.encode()+b'\0';assert len(payload)<=width;machine.mem_write(target,payload)
            ret(sp,16,result);return
        if role=='sprintf':
            target=read32(machine,sp+4);fmt=readstr(machine,read32(machine,sp+8));value=p.format(fmt,read32(machine,sp+12));payload=value.encode()+b'\0';assert ESP-0x1000<=target and target+len(payload)<=ESP
            machine.mem_write(target,payload);ret(sp,0,len(value));return
        if role=='sscanf':
            text=readstr(machine,read32(machine,sp+4));fmt=readstr(machine,read32(machine,sp+8));result,values=p.scan(text,fmt)
            for i,v in enumerate(values):
                target=read32(machine,sp+12+4*i);assert ESP-0x1000<=target and target+4<=ESP;machine.mem_write(target,pack(v))
            ret(sp,0,result);return
        if role=='stricmp':ret(sp,0,p.compare(readstr(machine,read32(machine,sp+4)),readstr(machine,read32(machine,sp+8))));return
        if role=='MessageBoxA':
            result=p.message(read32(machine,sp+4),readstr(machine,read32(machine,sp+8)),readstr(machine,read32(machine,sp+12)),read32(machine,sp+16));ret(sp,16,result);return
        if not any(lo<=a<hi for lo,hi in code):illegal.append(hex(a));machine.emu_stop()
    def on_write(machine,access,a,w,v,user):
        allowed=[(ESP-0x1000,ESP),(gate,gate+1),*[(a,a+4)for a in cells]]
        if not any(lo<=a and a+w<=hi for lo,hi in allowed):writes.append((hex(a),w));machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);entry=addresses[s.get('entry','validate')];u.emu_start(entry,STOP,count=10001)
    actual=dict(cells=[read32(u,a)for a in cells],gate=u.mem_read(gate,1)[0],owner=read32(u,owner),events=p.events);expected=reference(s)
    checks=dict(full_selected_state_equal=actual==expected,sentinel_return=u.reg_read(UC_X86_REG_EIP)==STOP,stack_balance=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(r)==v for r,v in zip(NV,SEEDS)),stack_guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+4,32))==bytes([0xa7])*32,execution_whitelist=not illegal,write_whitelist=not writes)
    assert all(checks.values()),(kind,s,checks,actual,expected,illegal,writes)
    return dict(state=actual,checks=checks,executed_provider_entries=roles,instructions=instructions)

def scenarios():
    cases=[dict(name='valid_gate_'+str(v),gate=v)for v in[0,1,0x80,0xff]]
    cases.extend([dict(name='readiness_open_success',opened=0),dict(name='readiness_open_failure',opened=0,open=[dict(code=5)]),dict(name='readiness_media_error',media=[dict(code=7)]),dict(name='readiness_media_false',media=[dict(response='false')]),dict(name='readiness_ready_error',ready=[dict(code=8)]),dict(name='readiness_ready_false',ready=[dict(response='false')]),dict(name='track_count_api_error',tracks=[dict(code=6)]),dict(name='track_count_scan_empty',tracks=[dict(response='')]),dict(name='track_count_scan_invalid',tracks=[dict(response='two')]),dict(name='track_count_zero',tracks=[dict(response='0')]),dict(name='track_count_one',tracks=[dict(response='1')]),dict(name='track_count_three',tracks=[dict(response='3')]),dict(name='track_count_trailing_text',tracks=[dict(response='2 ignored')]),dict(name='first_format_error',set_format=[dict(code=4)]),dict(name='second_format_error',set_format=[dict(code=0),dict(code=3)]),dict(name='first_type_api_error',type1=[dict(code=9)]),dict(name='second_type_api_error',type2=[dict(code=10)]),dict(name='first_track_audio',type1=[dict(response='audio')]),dict(name='first_track_any_non_audio',type1=[dict(response='anything')]),dict(name='second_track_non_audio',type2=[dict(response='data')]),dict(name='second_track_casefold',type2=[dict(response='AuDiO')]),dict(name='length_api_error',length2=[dict(code=11)]),dict(name='length_parse_two_fields',length2=[dict(response='34:12')]),dict(name='length_parse_one_field',length2=[dict(response='34')]),dict(name='length_parse_invalid',length2=[dict(response='invalid')]),dict(name='minutes_33',length2=[dict(response='33:59:74')]),dict(name='minutes_35',length2=[dict(response='35:0:0')]),dict(name='unchecked_seconds_frames',length2=[dict(response='34:999:777')]),dict(name='length_trailing_text',length2=[dict(response='34:12:8 trailing')]),dict(name='track_count_failure_clears_old_gate',gate=0xff,tracks=[dict(response='3')])])
    cases.extend([dict(name='startup_success',entry='startup'),dict(name='startup_cancel',entry='startup',tracks=[dict(response='3')],message=[dict(result=2)]),dict(name='startup_retry_then_success',entry='startup',tracks=[dict(response='3')],message=[dict(result=4)]),dict(name='startup_first_ready_false_probe_valid_cancel',entry='startup',media=[dict(response='false')],message=[dict(result=2)]),dict(name='startup_probe_valid_no_ready_recheck',entry='startup',tracks=[dict(mutate=dict(opened=0))]),dict(name='startup_live_owner',entry='startup',tracks=[dict(response='3',mutate=dict(owner=0xfedcba98))]),dict(name='startup_cancel_keeps_callback_gate',entry='startup',tracks=[dict(response='3')],message=[dict(result=2,mutate=dict(gate=0x80))]),dict(name='startup_open_retry_within_probe',entry='startup',open=[dict(code=5),dict(code=0)],message=[dict(result=2)])])
    for result in[0,1,3,5,6,7,0xffffffff]:cases.append(dict(name='startup_non_cancel_'+str(result),entry='startup',tracks=[dict(response='3')],message=[dict(result=result)]))
    return cases
def main():
    b=build();cases=[]
    for s in scenarios():
        n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},s
        cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual fresh/native startup CD checkA0B0,validation10520 and readiness9C30. MCI/dialog and identified CRT formatting/scanning/ASCII comparison are modeled boundaries. No original process/API/CRT or full startup/gameplay acceptance.',build=b,cases=cases,case_count=len(cases),all_equal=True)
    p=OUT/'host_cd_startup_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,report=str(p)),indent=2))
if __name__=='__main__':main()
