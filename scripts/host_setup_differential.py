"""Fresh original/source setup family with real pool/callback/file code."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
from pathlib import Path
ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/host_setup_public');parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'));args=parser.parse_args()
OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_setup.c';STORAGE=ROOT/'calibration/host_setup_storage.c';HEADER=ROOT/'calibration/host_setup_storage.h'
ORACLE=args.native_exe.resolve();ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
PROFILE={'setup':('_host_service_10690',0x10690,208),'reset':('_host_resource_reset_10760',0x10760,96),'slots':('_host_setup_slots_reset_a1a0',0xa1a0,112),'notify':('_host_frame_notify_zero',0xac00,16),'initialize':('_initialize_pool',0x4510,96),'identity':('_identity_value',0xbce0,16),'zero':('_host_zero_callback',0xa340,16),'empty':('_empty_host_callback',0xbe30,16),'enable':('_host_enable_clear_callbacks',0xbdf0,32),'packets':('_host_input_packets_initialize_4180',0x4180,32),'register':('_host_register_input_packet_bd30',0xbd30,16),'one':('_host_return_one',0xbd40,16),'invoke':('_host_invoke_callback_zero',0xbd80,32),'diagnostic':('_host_diagnostic',0x63c0,16),'handler':('_virtual_file_close',0xa350,80),'archive_close':('_archive_close',0x51f0,32)}
CTX_SIZE=0x1b34;DATA=0x60000000;CTXS=[DATA+0x10020,DATA+0x20020];HEAP=0x61000020;STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
REGS={'ebx':UC_X86_REG_EBX,'esi':UC_X86_REG_ESI,'edi':UC_X86_REG_EDI,'ebp':UC_X86_REG_EBP};SEEDS=dict(ebx=0x11223344,esi=0x22334455,edi=0x33445566,ebp=0x44556677)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
def read32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def maps(path):
    pat=re.compile(r'^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})');return {m.group(1):int(m.group(2),16)for l in path.read_text(errors='replace').splitlines()if (m:=pat.match(l))}
def build():
    assert sha(ORACLE)==ORACLE_SHA;verify_toolchain(DEFAULT_TOOLCHAIN)
    files=[SOURCE,STORAGE,*[ROOT/p for p in ['calibration/host_pool_repaired.c','calibration/host_pool_storage.c','src/pc/exe_helpers.c','src/pc/host_zero_callback.c','src/pc/interface_callbacks.c','src/host_return_one.c','calibration/host_callback_protocol.c','calibration/host_callback_storage.c','calibration/host_warm_delivery.cpp','calibration/host_warm_delivery_storage.cpp','calibration/sound_bank_storage.cpp','calibration/registry_storage.c','src/pc/host_diagnostic.c','calibration/host_pool_formats.c','calibration/virtual_file.c','calibration/archive_positions.c','calibration/archive_array_prefixes.c','calibration/archive_storage.c','calibration/public_file_handles.c']]]
    watched=[*files,HEADER,*[ROOT/p for p in ['calibration/engine_interface.h','calibration/resource_context.h','calibration/host_callback_protocol_externs.h','calibration/host_warm_delivery.h','toolchains/manifest.json','scripts/coff.py','scripts/match.py','scripts/masked.py']],ORACLE,Path(__file__).resolve()]
    pins={str(p):sha(p)for p in watched};h=os.path.relpath(ROOT/'calibration/engine_interface.h',OUT).replace('\\','/')
    fixture=OUT/'fixture.c';fixture.write_bytes(('''#include "%s"
EngineInterface g_host_engine_interface = {0};
#include <stddef.h>
int __cdecl archive_open(const char *path,const char *mode) { for (;;) {} }
int __cdecl archive_seek(int handle,long offset,int origin) { for (;;) {} }
size_t __cdecl archive_read(void *target,size_t size,size_t count,int handle) { for (;;) {} }
void __cdecl archive_fatal(const char *message) { for (;;) {} }
unsigned long __cdecl host_input_collect_8040(void) { for (;;) {} }
unsigned char g_host_setup_input_buffer0_7a2a00[34] = {0};
unsigned char g_host_setup_input_buffer1_7a2a40[34] = {0};
extern void __cdecl host_service_10690(void);
__declspec(dllexport) void __cdecl setup_fixture_anchor(void) { host_service_10690(); }
/* Unexecuted references from other pool functions are link boundaries only. */
'''%h).encode());files.append(fixture)
    objects=[];compiles=[];defs={}
    for s in files:
        obj,cmd=compile_source(s,['/O2']);objects.append(obj);co=COFF(obj);compiles.append(dict(source=str(s),source_sha256=sha(s),object=str(obj),object_sha256=sha(obj),command=cmd))
        for sym in co.symbols.values():
            if sym['storage']==2 and sym['section']>0 and sym['type']&0x20:assert sym['name']not in defs;defs[sym['name']]=(obj,co.function(sym['name'])['size'])
    dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*[str(p)for p in objects]]
    p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((p.stdout+p.stderr).encode());assert p.returncode==0,(p.stdout,p.stderr)
    symbols=maps(mp);needed=[v[0]for v in PROFILE.values()]+['_g_host_engine_interface','_g_host_pool','_g_host_callback_prefix','_g_host_input_packet_68e78c','_g_host_setup_input_buffer0_7a2a00','_g_host_setup_input_buffer1_7a2a40','_g_host_setup_prefix_3a630','_g_host_setup_slots_first_3744c','_g_public_file_count','_g_public_file_handles','_g_archive_logical_positions'];assert all(s in symbols for s in needed)
    comps=[]
    for role,(sym,rva,size)in PROFILE.items():
        obj,width=defs[sym];comps.append(dict(role=role,symbol=sym,source_size=width,native_size=size,raw=compare(obj,sym,ORACLE,rva,size),diagnostic=masked_compare(obj,sym,ORACLE,rva,size)))
    assert all(next(c for c in comps if c['role']==r)['diagnostic']['masked_equal']for r in ['setup','reset','notify','packets','handler','archive_close'])
    assert all(next(c for c in comps if c['role']==r)['raw']['exact']for r in ['identity','zero','empty','one','diagnostic'])
    co=COFF(objects[1]);fields={'_g_host_setup_prefix_3a630':(0x3a630,32,False),'_g_host_setup_slots_first_3744c':(0x3744c,4,False)}
    defined=[s for s in co.symbols.values()if s['storage']==2 and s['section']>0];assert {s['name']for s in defined}==set(fields)
    native=pefile.PE(str(ORACLE));storage_fields=[]
    for s in defined:
        rv,width,zero=fields[s['name']];sec=co.sections[s['section']-1];assert not sec['flags']&0x20 and not sec['relocations'] and s['value']+width<=sec['size']
        if zero:
            target=next(t for t in native.sections if t.VirtualAddress<=rv and rv+width<=t.VirtualAddress+t.Misc_VirtualSize);assert rv>=target.VirtualAddress+target.SizeOfRawData and sec['flags']&0x80 and not sec['data'];actual=bytes(width)
        else:
            actual=sec['data'][s['value']:s['value']+width];assert actual==native.get_data(rv,width)
        storage_fields.append(dict(symbol=s['name'],rva=hex(rv),bytes=width,kind='loader_zero'if zero else'initialized',sha256=hashlib.sha256(actual).hexdigest()))
    # Existing public-file storage is shared with setup, not defined again here.
    public_fields=[]
    for symbol,owner,rva,width in [('_g_public_file_count','archive_storage.c',0x56e638,4),('_g_public_file_handles','public_file_handles.c',0x56e640,64),('_g_archive_logical_positions','archive_array_prefixes.c',0x51a7d0,4096)]:
        providers=[]
        for receipt in compiles:
            c=COFF(receipt['object'])
            for s in c.symbols.values():
                if s['name']==symbol and s['storage']==2 and (s['section']>0 or s['section']==0 and s['value']>0):
                    providers.append((receipt,c,s))
        assert len(providers)==1,(symbol,providers)
        receipt,c,s=providers[0];assert Path(receipt['source']).name==owner
        if s['section']==0:
            assert s['value']==width;actual=bytes(width);kind='COMMON'
        else:
            sec=c.sections[s['section']-1];assert not sec['flags']&0x20 and not sec['relocations'] and s['value']+width<=sec['size']
            actual=sec['data'][s['value']:s['value']+width]if sec['data']else bytes(width);kind='data_prefix'
        target=next(t for t in native.sections if t.VirtualAddress<=rva and rva+width<=t.VirtualAddress+t.Misc_VirtualSize)
        if rva>=target.VirtualAddress+target.SizeOfRawData:
            assert actual==bytes(width);backing='virtual_loader_zero'
        else:
            assert rva+width<=target.VirtualAddress+target.SizeOfRawData and actual==native.get_data(rva,width);backing='initialized'
        public_fields.append(dict(symbol=symbol,owner=str(Path(receipt['source']).relative_to(ROOT)),rva=hex(rva),bytes=width,coff_kind=kind,native_backing=backing,sha256=hashlib.sha256(actual).hexdigest()))
    assert pins=={str(p):sha(p)for p in watched}
    return dict(inputs=pins,compiles=compiles,link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map=str(mp),map_sha256=sha(mp),symbols=symbols,source_sizes={s:n for s,(o,n)in defs.items()},comparisons=comps,storage_fields=storage_fields,shared_file_storage_fields=public_fields)
def execute(kind,b,scenario):
    pe=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=pe.OPTIONAL_HEADER.ImageBase;size=pe.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,pe.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(DATA,0x40000);u.mem_map(HEAP-32,0x2000);u.mem_map(STACK,0x10000)
    addresses={role:0x400000+rva if kind=='native'else b['symbols'][sym]for role,(sym,rva,n)in PROFILE.items()}
    if kind=='native':table,pool,callback,packet_cell,buf0,buf1,record,first,current,slots,positions=0x4361b8,0xba29c0,0xa8e780,0xa8e78c,0xba2a00,0xba2a40,0x43a630,0x43744c,0x96e638,0x96e640,0x91a7d0
    else:table,pool,callback,packet_cell,buf0,buf1,record,first,current,slots,positions=(b['symbols'][s]for s in ['_g_host_engine_interface','_g_host_pool','_g_host_callback_prefix','_g_host_input_packet_68e78c','_g_host_setup_input_buffer0_7a2a00','_g_host_setup_input_buffer1_7a2a40','_g_host_setup_prefix_3a630','_g_host_setup_slots_first_3744c','_g_public_file_count','_g_public_file_handles','_g_archive_logical_positions'])
    expected=[bytearray(bytes([0xa1+i])*CTX_SIZE)for i in range(2)];expected[0][9]=scenario['byte9']
    for a,x in zip(CTXS,expected):u.mem_write(a-32,bytes([0xc1])*32+x+bytes([0xc2])*32)
    tab=bytearray(bytes([0x2d])*1000);tab[4:8]=pack(CTXS[0]);u.mem_write(table,bytes(tab));pool_before=bytearray(bytes([0x3d])*48);pool_before[32:36]=pack(HEAP);pool_before[36:40]=pack(128);u.mem_write(pool,bytes(pool_before))
    u.mem_write(callback,pack(0x11112222)+pack(0x33334444)+pack(7));u.mem_write(packet_cell,pack(0));u.mem_write(buf0,bytes([0xf1])*34);u.mem_write(buf1,bytes([0xf2])*34)
    rec_before=bytes(u.mem_read(record,32));defaults=bytearray(rec_before[16:32]);assert rec_before.hex()=='0000000002040002000000004001000200800040002000100004000800010002'
    initial_count=scenario.get('count',16)
    u.mem_write(first,pack(scenario['first']));u.mem_write(current,pack(initial_count));slot_values=[0]*16
    if scenario.get('pending'):slot_values[0]=1;slot_values[7]=512;slot_values[15]=1024
    if scenario.get('dense'):slot_values=[1,512,1024]+[0]*13
    position_values=[0x20000000+i*4 for i in range(1024)];u.mem_write(positions,b''.join(pack(v)for v in position_values))
    u.mem_write(slots,b''.join(pack(v)for v in slot_values));u.mem_write(HEAP-32,bytes([0xe1])*32+bytes([0x6d])*128+bytes([0xe2])*64)
    u.mem_write(ESP-0x500,bytes([0xa7])*0x600);u.mem_write(ESP,pack(STOP));u.reg_write(UC_X86_REG_ESP,ESP)
    for k,v in SEEDS.items():u.reg_write(REGS[k],v)
    events=[];illegal=[];writes=[];count=0
    ranges=[(addresses[r],addresses[r]+(n if kind=='native'else b['source_sizes'][sym]))for r,(sym,rv,n)in PROFILE.items()]
    write_ranges=[(a,a+CTX_SIZE)for a in CTXS]+[(pool,pool+48),(callback,callback+12),(packet_cell,packet_cell+4),(HEAP,HEAP+128),(first,first+4),(current,current+4),(slots,slots+64),(positions,positions+4096),(ESP-0x500,ESP+4)]
    entries={v:k for k,v in addresses.items()}
    def on_write(machine,access,a,w,v,user):
        if not any(lo<=a and a+w<=hi for lo,hi in write_ranges):writes.append((hex(a),w));machine.emu_stop()
    def on_code(machine,a,w,user):
        nonlocal count,defaults
        count+=1
        if count>20000:illegal.append('instruction cap');machine.emu_stop();return
        if a==STOP:machine.emu_stop();return
        role=entries.get(a);sp=machine.reg_read(UC_X86_REG_ESP)
        if role=='handler':events.append(dict(call=role,id=read32(machine,sp+4)))
        elif role=='archive_close':events.append(dict(call=role,archive_id=read32(machine,sp+4)))
        if role=='initialize':events.append(dict(call=role,arena=hex(read32(machine,sp+4)),bytes=read32(machine,sp+8),id=read32(machine,sp+12)))
        elif role=='identity':events.append(dict(call=role,value=read32(machine,sp+4)))
        elif role=='zero':
            offset=read32(machine,sp)-addresses['setup'];assert offset in [0x45,0x5e,0xbc]
            events.append(dict(call=role,arg=read32(machine,sp+4)if offset!=0xbc else None))
        elif role=='empty':
            caller=read32(machine,sp);direct=addresses['setup']<=caller<addresses['setup']+208
            events.append(dict(call=role,arg=read32(machine,sp+4)if direct else None))
        elif role=='notify':events.append(dict(call=role,argument_is_record=read32(machine,sp+4)==record,args=[read32(machine,sp+i)for i in [8,12,16]]))
        elif role=='register':events.append(dict(call=role,arg0_is_buffer1=read32(machine,sp+4)==buf1,arg1=read32(machine,sp+8),arg2_is_buffer0=read32(machine,sp+12)==buf0,arg3=read32(machine,sp+16)))
        elif role=='one':
            if scenario.get('swap'):
                machine.mem_write(table+4,pack(CTXS[1]));defaults=bytearray(bytes.fromhex('1032547698badcfe0123456789abcdef'));machine.mem_write(record+16,bytes(defaults))
            events.append(dict(call=role))
        elif role in ['enable','packets','reset','slots','invoke']:events.append(dict(call=role,arg=read32(machine,sp+4)if role=='reset'else None))
        if not any(lo<=a<hi for lo,hi in ranges):illegal.append(hex(a));machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
    u.emu_start(addresses['setup'],STOP,count=20001)
    handled=[e['id']for e in events if e['call']=='handler']
    closed=[];expected_handlers=[]
    if scenario['first']:slot_values=[0]*16
    signed_count=initial_count-0x100000000 if initial_count&0x80000000 else initial_count
    for i,value in enumerate(slot_values):
        if value:
            expected_handlers.append(i+1)
            if i+1<=signed_count:
                position_values[value-1]=0xffffffff;closed.append(value)
                if signed_count>0:signed_count-=1
                slot_values[i]=0
    index=1 if scenario.get('swap')else 0
    x=expected[index];x[0x6c:0x7c]=defaults;x[0x7c:0x8c]=defaults;x[0x9c:0x9e]=bytes(2);x[0xdf]=0;x[0xdc:0xde]=bytes(2);x[15]=x[14]
    pool_expected=bytearray(pool_before);pool_expected[:4]=pack(HEAP);pool_expected[16:20]=pack(HEAP+120)
    heap_expected=bytearray(bytes([0x6d])*128);heap_expected[:4]=pack(0x8000001e)
    for i in range(2,31):heap_expected[i*4:i*4+4]=pack(1)
    checks=dict(contexts_equal=all(bytes(u.mem_read(a,CTX_SIZE))==bytes(x)for a,x in zip(CTXS,expected)),context_guards=all(bytes(u.mem_read(a-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(a+CTX_SIZE,32))==bytes([0xc2])*32 for a in CTXS),heap_equal=bytes(u.mem_read(HEAP,128))==bytes(heap_expected),heap_guards=bytes(u.mem_read(HEAP-32,32))==bytes([0xe1])*32 and bytes(u.mem_read(HEAP+128,64))==bytes([0xe2])*64,pool_equal=bytes(u.mem_read(pool,48))==bytes(pool_expected),callback_equal=bytes(u.mem_read(callback,12))==bytes(8)+pack(1),packet_points_to_buffer1=read32(u,packet_cell)==buf1,buffers_unchanged=bytes(u.mem_read(buf0,34))==bytes([0xf1])*34 and bytes(u.mem_read(buf1,34))==bytes([0xf2])*34,current_slot_zero=read32(u,current)==0,first_flag_zero=read32(u,first)==0,slot_table_equal=bytes(u.mem_read(slots,64))==b''.join(pack(v)for v in slot_values),handler_order=handled==expected_handlers,return_sentinel=u.reg_read(UC_X86_REG_EIP)==STOP,stack_balance=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatiles=all(u.reg_read(REGS[k])==v for k,v in SEEDS.items()),unused_stack_clear=bytes(u.mem_read(ESP-32,32))==bytes([0xa7])+bytes(31),execution_whitelist=not illegal,write_whitelist=not writes)
    checks['archive_positions_complete_equal']=bytes(u.mem_read(positions,4096))==b''.join(pack(v)for v in position_values)
    checks['actual_archive_close_order']=[e['archive_id']for e in events if e['call']=='archive_close']==closed
    assert all(checks.values()),(kind,scenario,checks,events,illegal,writes)
    assert next(e for e in events if e['call']=='identity')['value']==scenario['byte9']
    return dict(events=events,checks=checks,context_hashes=[hashlib.sha256(bytes(u.mem_read(a,CTX_SIZE))).hexdigest()for a in CTXS],pool_sha256=hashlib.sha256(bytes(u.mem_read(pool,48))).hexdigest(),heap_sha256=hashlib.sha256(bytes(u.mem_read(HEAP,128))).hexdigest(),record_bytes=bytes(u.mem_read(record,32)).hex(),slot_values=slot_values,archive_positions_sha256=hashlib.sha256(bytes(u.mem_read(positions,4096))).hexdigest(),closed_archive_ids=closed,instructions=count)
def main():
    b=build();scenarios=[dict(name='byte9_'+str(v),byte9=v,first=1,pending=True)for v in [0,1,0x7f,0x80,0xff]]+[dict(name='sparse_shrinking_count',byte9=7,first=0,pending=True),dict(name='dense_shrinking_count',byte9=9,first=0,dense=True,count=3),dict(name='negative_count',byte9=3,first=0,pending=True,count=0xffffffff),dict(name='zero_count',byte9=4,first=0,pending=True,count=0),dict(name='first_highbit',byte9=3,first=0x80000000,pending=True),dict(name='live_context_defaults',byte9=5,first=0,pending=True,swap=True)]
    cases=[]
    for s in scenarios:
        n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},(s,n,c);cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Setup/reset/notification/public-file reset and actual virtual/archive close code. No handler or close return/effects modeled. Synthetic table/context/arena/public-handle preconditions and one post-leaf context/default intervention; no game process/API/full startup.',build=b,cases=cases,case_count=len(cases),all_equal=True,oracle_sha256=sha(ORACLE),runner_sha256=sha(Path(__file__)))
    dest=OUT/'host_setup_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,actual_setup_family_execution=True,report=str(dest)),indent=2))
if __name__=='__main__':main()
