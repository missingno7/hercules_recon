"""Fresh original graphics startup and mode callback with modeled platform boundaries."""
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
parser.add_argument('--output',type=Path,default=ROOT/'work/host_graphics_startup_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_graphics_startup.cpp';ORACLE=args.native_exe.resolve()
ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
PROFILE={'startup':('_host_service_72a0',0x72a0,544),'mode':('_host_graphics_mode_callback_7170@8',0x7170,48),'lock':('_graphics_native_06ff0',0x6ff0,208)}
FIELDS=[('dc','_g_exe_96e400',0x96e400,4,'graphics_storage.cpp'),('palette_ptr','_g_exe_96e3f8',0x96e3f8,4,'graphics_storage.cpp'),('info_ptr','_g_exe_96e3f4',0x96e3f4,4,'graphics_storage.cpp'),('bits','_g_exe_96e3d8',0x96e3d8,4,'graphics_storage.cpp'),('bitmap','_g_exe_96e404',0x96e404,4,'graphics_storage.cpp'),('palette','_g_exe_96e3fc',0x96e3fc,4,'graphics_storage.cpp'),('old_palette','_g_exe_96e3dc',0x96e3dc,4,'graphics_storage.cpp'),('draw','_g_exe_96e3e4',0x96e3e4,4,'graphics_storage.cpp'),('capability','_g_exe_92c488',0x92c488,1,'host_system_key_storage.c'),('owner','_g_fatal_owner_window',0xba0200,4,'fatal_storage.c'),('fullscreen','_g_fatal_suppress_dialog',0xba020c,1,'fatal_storage.c'),('locked','_g_exe_ba01f0',0xba01f0,1,'graphics_lock_storage.c'),('pitch','_g_exe_ba0214',0xba0214,4,'graphics_storage.cpp'),('render_bits','_g_exe_ba0210',0xba0210,4,'graphics_storage.cpp')]
RECIP_SYMBOL='_g_host_graphics_reciprocal_68e918'

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
    files=[SOURCE,*[ROOT/p for p in ['calibration/host_graphics_startup_storage.c','calibration/graphics_storage.cpp','calibration/graphics_lock.cpp','calibration/graphics_lock_storage.c','calibration/host_system_key_storage.c','calibration/fatal_storage.c']]]
    libraries=[DEFAULT_TOOLCHAIN/'lib'/p for p in ['libcmt.lib','kernel32.lib','user32.lib','gdi32.lib','ddraw.lib']]
    map_path=Path('D:/Prog/hercules_recon/work/library_map/library_map.json')
    watched=[*files,*libraries,ORACLE,map_path,Path(__file__),*[ROOT/p for p in ['scripts/coff.py','scripts/match.py','scripts/masked.py','scripts/pe.py','scripts/env.py','toolchains/manifest.json']],DEFAULT_TOOLCHAIN/'include/ddraw.h',DEFAULT_TOOLCHAIN/'include/wingdi.h']
    pins={str(p):sha(p)for p in watched}
    assert sha(libraries[0])=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    library_map=json.loads(map_path.read_bytes())['contributions']['HERCULES.EXE'];contracts=[]
    for index,payload in members(libraries[0].read_bytes()):
        if index!=134:continue
        symbol,rva,width='_malloc',0x10b20,32
        row=next(r for r in library_map if int(r['rva'],0)==rva);assert row['member']==index and row['size']==width and symbol in row['names']
        path=OUT/('crt_member'+str(index)+'.obj');path.write_bytes(payload);co=COFF(path)
        number,section=next((n,s)for n,s in enumerate(co.sections,1)if s['flags']&0x20 and s['size']);assert section['size']==width
        diagnostic=_compare_section(co,section,number,PE(ORACLE),rva,width);assert diagnostic['masked_equal']
        contracts.append(dict(symbol=symbol,member=index,rva=hex(rva),size=width,member_sha256=sha(path),complete_code_diagnostic=diagnostic,scope='Unmodified identified libcmt member; private whole-section diagnostic permits the existing alias names. No function/layout acceptance; CRT body is not executed in these cases.'))
    fixture=OUT/'fixture.c';fixture.write_bytes(b'''#include <windows.h>
extern void __cdecl host_service_72a0(void);
void __cdecl graphics_native_071a0(void) { for (;;) {} }
void __cdecl archive_fatal(const char *message) { for (;;) {} }
void __cdecl cleanup_native_6bb0(void) { for (;;) {} }
unsigned short __cdecl cleanup_native_6ec0(int value) { for (;;) {} }
int __cdecl main(void) { for (;;) {} }
__declspec(dllexport) void __cdecl graphics_fixture_anchor(void) { host_service_72a0(); }
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
        if role=='mode':assert diagnostic['masked_equal']
        comparisons.append(dict(role=role,symbol=symbol,source_size=size,native_size=width,raw=raw,diagnostic=diagnostic))
    needed=[r[0]for r in PROFILE.values()]+[f[1]for f in FIELDS]+[RECIP_SYMBOL,'_malloc','_archive_fatal','_graphics_native_071a0'];assert all(s in symbols for s in needed)
    native=pefile.PE(str(ORACLE));data=[]
    fields=[(symbol,owner,va-0x400000,width)for role,symbol,va,width,owner in FIELDS]+[(RECIP_SYMBOL,'host_graphics_startup_storage.c',0x68e918,8192)]
    for symbol,owner,rva,width in fields:
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

PAL=0x61000020;INFO=0x61002020;PIXELS=0x61004020;OBJECTS=[0x65000100,0x65000200];DESC=0x65002020;ENUM_AFTER=HOOKS+0x200
def descriptor(mode):
    data=bytearray(bytes([0x5b])*108);data[0:4]=pack(108);data[8:12]=pack(mode[1]);data[12:16]=pack(mode[0]);data[0x54:0x58]=pack(mode[2]);return bytes(data)
def initial():return {role:(0 if role in['fullscreen','locked']else 0xa5 if width==1 else 0x12340000+i*4)for i,(role,sym,va,width,owner)in enumerate(FIELDS)}
class Protocol:
    def __init__(self,s,get,put):self.s=s;self.get=get;self.put=put;self.counts={};self.events=[];self.terminal=None
    def action(self,role,default):
        i=self.counts.get(role,0);self.counts[role]=i+1;items=self.s.get(role,[]);a=items[i]if i<len(items)else default
        for k,v in a.get('mutate',{}).items():self.put(k,v)
        return a
    def api(self,role,arguments,default):
        a=self.action(role,dict(result=default));v=a.get('result',default);self.events.append(dict(call=role,arguments=arguments,result=v));return v,a
    def fatal(self,text):self.events.append(dict(call='archive_fatal',message=text));self.terminal=text
def reference(s):
    state=initial();memory={PAL:bytearray(bytes([0x6d])*1032),INFO:bytearray(bytes([0x7e])*1068)}
    recips=bytes([0x5c])*8192
    def put(k,v):assert k in state;state[k]=v&(255 if next(f[3]for f in FIELDS if f[0]==k)==1 else 0xffffffff)
    p=Protocol(s,lambda k:state[k],put)
    def finish():return dict(state=state,heap_hashes={hex(a):hashlib.sha256(v).hexdigest()for a,v in memory.items()},reciprocal_sha256=hashlib.sha256(recips).hexdigest(),events=p.events,terminal=p.terminal)
    if s.get('entry')=='mode':
        mode=s.get('mode',(320,240,8));result=0 if tuple(mode)==(320,240,8)else 1
        if not result:state['capability']=1
        return dict(finish(),callback_return=result)
    recips=b''.join(pack(0 if i==0 else 0x40000000//i)for i in range(2048))
    result,a=p.api('GetDC',[state['owner']],0xcafe0001);state['dc']=result
    if not result:p.fatal('PC_Mainhdc');return finish()
    result,a=p.api('malloc',[1032],PAL);state['palette_ptr']=result
    if not result:p.fatal('PC_LogPal');return finish()
    assert result==PAL;memory[PAL]=bytearray(1032);memory[PAL][:4]=bytes.fromhex('00030001')
    result,a=p.api('malloc',[1068],INFO);state['info_ptr']=result
    if not result:p.fatal('PC_BitmapInfo');return finish()
    assert result==INFO;memory[INFO]=bytearray(1068);memory[INFO][0:4]=pack(40);memory[INFO][4:8]=pack(320);memory[INFO][8:12]=pack(-240);memory[INFO][12:16]=bytes.fromhex('01000800')
    state['bits']=0
    result,a=p.api('CreateDIBSection',[state['dc'],state['info_ptr'],0,'bits_cell',0,0],0xcafe0002);state['bits']=a.get('bits',PIXELS);state['bitmap']=result
    result,a=p.api('CreatePalette',[state['palette_ptr']],0xcafe0003);state['palette']=result
    if not result:p.fatal('CreatePalette()');return finish()
    result,a=p.api('SelectPalette',[state['dc'],state['palette'],0],0xcafe0004);state['old_palette']=result
    # Normal startup precondition: existing lock6FF0 executes its nonfullscreen arm.
    assert state['fullscreen']==0
    if not state['locked']:state['pitch']=320;state['render_bits']=state['bits'];state['locked']=1
    p.api('tables_71a0',[],0)
    result,a=p.api('DirectDrawCreate',[0,'draw_cell',0],0);state['draw']=a.get('object',OBJECTS[0])
    if result:p.fatal('DirectDrawCreate()');return finish()
    result,a=p.api('SetCooperativeLevel',[state['draw'],state['owner'],0x55],0)
    if result:p.fatal('SetCooperativeLevel()');return finish()
    state['capability']=0
    # Append after callbacks in both runs; callbacks actually execute in emulator.
    obj=state['draw'];a=p.action('EnumDisplayModes',dict(result=0));modes=s.get('modes',[(640,480,16),(320,240,8)])
    callback_results=[]
    for mode in modes:
        result=0 if tuple(mode)==(320,240,8)else 1
        if not result:state['capability']=1
        callback_results.append(result)
        if not result:break
    result=a.get('result',0);p.events.append(dict(call='EnumDisplayModes',arguments=[obj,0,0,0,'mode_callback'],callback_results=callback_results,result=result))
    if result:p.fatal('EnumDisplayModes()');return finish()
    result,a=p.api('SetCooperativeLevel',[state['draw'],state['owner'],0x0c],0)
    if result:p.fatal('SetCooperativeLevel()');return finish()
    return finish()

def execute(kind,b,s):
    pe=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=pe.OPTIONAL_HEADER.ImageBase;size=pe.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,pe.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(STACK,0x10000);u.mem_map(HOOKS,0x1000);u.mem_map(PAL-32,0x10000);u.mem_map(0x65000000,0x10000)
    addresses={role:0x400000+rva if kind=='native'else b['symbols'][sym]for role,(sym,rva,n)in PROFILE.items()};widths={c['role']:c['source_size']for c in b['comparisons']}
    code=[(addresses[r],addresses[r]+(n if kind=='native'else widths[r]))for r,(sym,rva,n)in PROFILE.items()]
    cells={role:va if kind=='native'else b['symbols'][sym]for role,sym,va,width,owner in FIELDS};recip=0xa8e918 if kind=='native'else b['symbols'][RECIP_SYMBOL]
    endpoints={};by_role={}
    for i,name in enumerate(['GetDC','CreateDIBSection','CreatePalette','SelectPalette','DirectDrawCreate']):
        address=HOOKS+i*16;by_role[name]=address;endpoints[address]=name;imports=[x for d in pe.DIRECTORY_ENTRY_IMPORT for x in d.imports if x.name==name.encode()];assert len(imports)==1;u.mem_write(imports[0].address,pack(address))
    # DirectDrawCreate is a normal import-library JMP thunk, not a recovered body.
    thunk=0x410a20 if kind=='native'else b['symbols']['_DirectDrawCreate@12'];code.append((thunk,thunk+6))
    for name,address in[('malloc',0x410b20 if kind=='native'else b['symbols']['_malloc']),('archive_fatal',0x410460 if kind=='native'else b['symbols']['_archive_fatal']),('tables_71a0',0x4071a0 if kind=='native'else b['symbols']['_graphics_native_071a0']),('SetCooperativeLevel',HOOKS+0x100),('EnumDisplayModes',HOOKS+0x110)]:endpoints[address]=name;by_role[name]=address
    for i,obj in enumerate(OBJECTS):
        vt=0x65001000+i*0x100;u.mem_write(obj,pack(vt));u.mem_write(vt+0x50,pack(by_role['SetCooperativeLevel']));u.mem_write(vt+0x20,pack(by_role['EnumDisplayModes']))
    for role,value in initial().items():u.mem_write(cells[role],bytes([value])if next(f[3]for f in FIELDS if f[0]==role)==1 else pack(value))
    u.mem_write(recip,bytes([0x5c])*8192)
    for a,n,seed in[(PAL,1032,0x6d),(INFO,1068,0x7e)]:u.mem_write(a-32,bytes([0xa1])*32+bytes([seed])*n+bytes([0xa2])*32)
    u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP));u.reg_write(UC_X86_REG_ESP,ESP)
    for r,v in zip(NV,SEEDS):u.reg_write(r,v)
    def get(k):return u.mem_read(cells[k],1)[0]if next(f[3]for f in FIELDS if f[0]==k)==1 else read32(u,cells[k])
    def put(k,v):
        assert k in cells;u.mem_write(cells[k],bytes([v&255])if next(f[3]for f in FIELDS if f[0]==k)==1 else pack(v))
    p=Protocol(s,get,put);instructions=0;illegal=[];writes=[];roles=[];pending=None;callback_checks=[]
    def ret(sp,extra,value):u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,read32(u,sp))
    def schedule_callback():
        nonlocal pending
        j=pending['index'];mode=pending['modes'][j];data=descriptor(mode);u.mem_write(DESC-32,bytes([0xb1])*32+data+bytes([0xb2])*32);pending['descriptor']=data;pending['registers']=[u.reg_read(r)for r in NV]
        sp=pending['sp']-12;u.mem_write(sp,pack(ENUM_AFTER)+pack(DESC)+pack(0));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EIP,pending['callback'])
    def finish_enum():
        nonlocal pending
        p.events.append(dict(call='EnumDisplayModes',arguments=pending['arguments'],callback_results=pending['results'],result=pending['result']));ret(pending['sp'],20,pending['result']);pending=None
    def on_code(machine,a,w,user):
        nonlocal instructions,pending
        instructions+=1
        if instructions>40000:illegal.append('instruction cap');machine.emu_stop();return
        if a==STOP:machine.emu_stop();return
        sp=machine.reg_read(UC_X86_REG_ESP)
        if a==ENUM_AFTER:
            assert pending is not None
            valid=sp==pending['sp']and all(machine.reg_read(r)==v for r,v in zip(NV,pending['registers']))and bytes(machine.mem_read(DESC,108))==pending['descriptor']and bytes(machine.mem_read(DESC-32,32))==bytes([0xb1])*32 and bytes(machine.mem_read(DESC+108,32))==bytes([0xb2])*32
            callback_checks.append(valid);assert valid;result=machine.reg_read(UC_X86_REG_EAX);assert result in[0,1];pending['results'].append(result);pending['index']+=1
            if result==0 or pending['index']==len(pending['modes']):finish_enum()
            else:schedule_callback()
            return
        if a in addresses.values():roles.append(next(r for r,v in addresses.items()if v==a))
        role=endpoints.get(a)
        if role=='archive_fatal':p.fatal(readstr(machine,read32(machine,sp+4)));machine.emu_stop();return
        if role=='malloc':
            n=read32(machine,sp+4);assert n in[1032,1068];value,action=p.api(role,[n],PAL if n==1032 else INFO);ret(sp,0,value);return
        if role=='tables_71a0':value,action=p.api(role,[],0);ret(sp,0,value);return
        if role=='GetDC':value,action=p.api(role,[read32(machine,sp+4)],0xcafe0001);ret(sp,4,value);return
        if role=='CreateDIBSection':
            vals=[read32(machine,sp+i)for i in[4,8,12,16,20,24]];assert vals[3]==cells['bits'];value,action=p.api(role,vals[:3]+['bits_cell']+vals[4:],0xcafe0002);put('bits',action.get('bits',PIXELS));ret(sp,24,value);return
        if role=='CreatePalette':value,action=p.api(role,[read32(machine,sp+4)],0xcafe0003);ret(sp,4,value);return
        if role=='SelectPalette':value,action=p.api(role,[read32(machine,sp+i)for i in[4,8,12]],0xcafe0004);ret(sp,12,value);return
        if role=='DirectDrawCreate':
            vals=[read32(machine,sp+i)for i in[4,8,12]];assert vals==[0,cells['draw'],0];value,action=p.api(role,[0,'draw_cell',0],0);put('draw',action.get('object',OBJECTS[0]));ret(sp,12,value);return
        if role=='SetCooperativeLevel':value,action=p.api(role,[read32(machine,sp+i)for i in[4,8,12]],0);ret(sp,12,value);return
        if role=='EnumDisplayModes':
            vals=[read32(machine,sp+i)for i in[4,8,12,16,20]];assert vals[1:4]==[0,0,0]and vals[4]==addresses['mode'];action=p.action(role,dict(result=0));pending=dict(sp=sp,callback=vals[4],arguments=vals[:4]+['mode_callback'],result=action.get('result',0),modes=s.get('modes',[(640,480,16),(320,240,8)]),index=0,results=[])
            if pending['modes']:schedule_callback()
            else:finish_enum()
            return
        if not any(lo<=a<hi for lo,hi in code):illegal.append(hex(a));machine.emu_stop()
    def on_write(machine,access,a,w,v,user):
        allowed=[(ESP-0x1000,ESP),(recip,recip+8192),(PAL,PAL+1032),(INFO,INFO+1068),*[(cells[r],cells[r]+width)for r,sym,va,width,owner in FIELDS]]
        if not any(lo<=a and a+w<=hi for lo,hi in allowed):writes.append((hex(a),w));machine.emu_stop()
    if s.get('entry')=='mode':
        mode=s.get('mode',(320,240,8));u.mem_write(DESC-32,bytes([0xb1])*32+descriptor(mode)+bytes([0xb2])*32);u.mem_write(ESP+4,pack(DESC)+pack(0x87654321));entry=addresses['mode']
    else:entry=addresses['startup']
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.emu_start(entry,STOP,count=40001)
    actual=dict(state={role:get(role)for role,sym,va,width,owner in FIELDS},heap_hashes={hex(a):hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()for a,n in[(PAL,1032),(INFO,1068)]},reciprocal_sha256=hashlib.sha256(bytes(u.mem_read(recip,8192))).hexdigest(),events=p.events,terminal=p.terminal)
    if s.get('entry')=='mode':actual['callback_return']=u.reg_read(UC_X86_REG_EAX)
    expected=reference(s);extra=8 if s.get('entry')=='mode'else 0;returned=p.terminal is None
    checks=dict(full_selected_memory_events_equal=actual==expected,terminal_or_return=bool(p.terminal)or u.reg_read(UC_X86_REG_EIP)==STOP,stack_balance=not returned or u.reg_read(UC_X86_REG_ESP)==ESP+4+extra,nonvolatiles=not returned or all(u.reg_read(r)==v for r,v in zip(NV,SEEDS)),heap_guards=all(bytes(u.mem_read(a-32,32))==bytes([0xa1])*32 and bytes(u.mem_read(a+n,32))==bytes([0xa2])*32 for a,n in[(PAL,1032),(INFO,1068)]),stack_guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+12,32))==bytes([0xa7])*32,callback_abi=all(callback_checks),execution_whitelist=not illegal,write_whitelist=not writes)
    if s.get('entry')=='mode':checks['descriptor_untouched']=bytes(u.mem_read(DESC,108))==descriptor(s.get('mode',(320,240,8)))and bytes(u.mem_read(DESC-32,32))==bytes([0xb1])*32 and bytes(u.mem_read(DESC+108,32))==bytes([0xb2])*32
    assert all(checks.values()),(kind,s,checks,actual,expected,illegal,writes)
    return dict(result=actual,checks=checks,provider_entries=roles,instructions=instructions)

def scenarios():
    cases=[dict(name='normal'),dict(name='no_modes',modes=[]),dict(name='wrong_modes',modes=[(640,480,8),(320,240,16),(320,480,8)]),dict(name='first_mode_matches',modes=[(320,240,8),(640,480,16)]),dict(name='getdc_failure',GetDC=[dict(result=0)]),dict(name='palette_allocation_failure',malloc=[dict(result=0)]),dict(name='bitmap_allocation_failure',malloc=[dict(result=PAL),dict(result=0)]),dict(name='palette_failure',CreatePalette=[dict(result=0)]),dict(name='draw_create_failure',DirectDrawCreate=[dict(result=0x88760001)]),dict(name='exclusive_failure',SetCooperativeLevel=[dict(result=0x80004005)]),dict(name='enum_failure_after_callback',EnumDisplayModes=[dict(result=0x80004005)]),dict(name='normal_coop_failure',SetCooperativeLevel=[dict(result=0),dict(result=0x80004005)]),dict(name='unchecked_dib_failure',CreateDIBSection=[dict(result=0,bits=0)]),dict(name='unchecked_select_palette_failure',SelectPalette=[dict(result=0)]),dict(name='live_owner_and_dc',GetDC=[dict(result=0xabcd1234,mutate=dict(owner=0xfedcba98))]),dict(name='cooperative_live_draw_owner',SetCooperativeLevel=[dict(result=0,mutate=dict(draw=OBJECTS[1],owner=0x89abcdef))]),dict(name='enum_live_draw_owner',EnumDisplayModes=[dict(result=0,mutate=dict(draw=OBJECTS[1],owner=0x76543210))]),dict(name='table_boundary_capability_cleared',tables_71a0=[dict(mutate=dict(capability=0x80))],modes=[]),dict(name='palette_api_dc_reload',CreatePalette=[dict(result=0xabcd4321,mutate=dict(dc=0x87654321))])]
    cases.extend(dict(name='callback_'+str(i),entry='mode',mode=mode)for i,mode in enumerate([(320,240,8),(320,240,0),(320,240,16),(0x100140,240,8),(320,0x100f0,8),(0,240,8),(320,0,8),(0xffffffff,240,8),(320,0xffffffff,8),(320,240,0xffffffff)]))
    return cases
def main():
    b=build();cases=[]
    for s in scenarios():
        n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},s
        cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual startup72A0,mode callback7170 and existing normal-path lock6FF0. Win32/DirectDraw,identified malloc,fatal-entry stop and unresolved table71A0 are offline modeled boundaries. No real platform/allocator/fatal process or complete graphics/game proof.',build=b,cases=cases,case_count=len(cases),all_equal=True)
    path=OUT/'host_graphics_startup_differential.json';path.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,report=str(path)),indent=2))
if __name__=='__main__':main()
