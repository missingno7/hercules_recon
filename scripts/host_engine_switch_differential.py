"""Bounded original/source engine-switch cleanup with real pool/callback code."""
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
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/host_engine_switch_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args();OUT=args.output.resolve();assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_engine_switch.c'
ORACLE=args.native_exe.resolve();ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
PROFILE={'switch':('_host_service_4040',0x4040,144),'zero':('_host_zero_callback',0xa340,16),'free':('_host_free',0x4750,112),'bulk':('_bulk_free',0x47c0,112),'coalesce':('_coalesce_pool',0x4830,96),'compact_wrapper':('_compact_all_wrapper',0x4890,16),'compact':('_compact_pool',0x48a0,336),'count':('_count_used',0x49f0,64),'reverse':('_reverse_dword_copy',0x43a0,176),'empty':('_empty_host_callback',0xbe30,16),'set_one':('_host_set_callback_one',0xbdd0,16),'diagnostic':('_host_diagnostic',0x63c0,16),'select':('_host_engine_select',0x5cd0,320)}
CTX_SIZE=0x1b34;DATA=0x60000000;CTXS=[DATA+0x10020,DATA+0x20020,DATA+0x30020];HEAP=0x61000020;STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
SLOTS=[DATA+0x400,DATA+0x404];PREFIX=DATA+0x600;SUFFIX=DATA+0x800
REGS={'ebx':UC_X86_REG_EBX,'esi':UC_X86_REG_ESI,'edi':UC_X86_REG_EDI,'ebp':UC_X86_REG_EBP};SEEDS=dict(ebx=0x11223344,esi=0x22334455,edi=0x33445566,ebp=0x44556677)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pack=lambda v:struct.pack('<I',v&0xffffffff)
def read32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def cstring(u,a):
    if not a:return None
    b=bytearray()
    for n in range(512):
        c=u.mem_read(a+n,1)[0]
        if not c:return b.decode('ascii')
        b.append(c)
    raise ValueError('unbounded fixture string')
def maps(path):
    pat=re.compile(r'^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})')
    return {m.group(1):int(m.group(2),16)for l in path.read_text(errors='replace').splitlines()if (m:=pat.match(l))}
def build():
    assert sha(ORACLE)==ORACLE_SHA;verify_toolchain(DEFAULT_TOOLCHAIN)
    files=[SOURCE,*[ROOT/p for p in ['src/pc/host_pool_repaired.c','src/pc/host_pool_lifecycle_repaired.c','src/pc/host_pool_counts.c','src/pc/pool_reverse_copy.c','src/pc/host_pool_storage.c','src/pc/host_callback_protocol.c','src/pc/host_callback_storage.c','src/pc/host_zero_callback.c','src/pc/interface_callbacks.c','src/pc/host_diagnostic.c','src/pc/host_pool_formats.c','src/pc/host_pool_compact_format.c']]]
    watched=[*files,*[ROOT/p for p in ['src/shared/engine_interface.h','src/shared/resource_context.h','src/pc/host_callback_protocol_externs.h','toolchains/manifest.json','scripts/coff.py','scripts/match.py','scripts/masked.py']],ORACLE,Path(__file__).resolve()]
    before={str(p):sha(p)for p in watched}
    h=os.path.relpath(ROOT/'src/shared/engine_interface.h',OUT).replace('\\','/')
    fixture=OUT/'fixture.c';fixture.write_bytes(('''#include "%s"
EngineInterface g_host_engine_interface = {0};
void __cdecl host_engine_select(const char *prefix,const char *suffix) {}
extern int __cdecl host_service_4040(const char *,const char *);
__declspec(dllexport) int __cdecl engine_switch_fixture_anchor(const char *a,const char *b) { return host_service_4040(a,b); }
'''%h).encode());files.append(fixture)
    compiles=[];objects=[];definitions={}
    for path in files:
        obj,cmd=compile_source(path,['/O2']);co=COFF(obj)
        objects.append(obj);compiles.append(dict(source=str(path),source_sha256=sha(path),object=str(obj),object_sha256=sha(obj),command=cmd))
        for sym in co.symbols.values():
            if sym['storage']==2 and sym['section']>0 and sym['type']&0x20:
                assert sym['name']not in definitions
                definitions[sym['name']]=(obj,co.function(sym['name'])['size'])
    dll=OUT/'fixture.dll';mp=OUT/'fixture.map';env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/OUT:'+str(dll),'/MAP:'+str(mp),*[str(p)for p in objects]]
    p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'link.log').write_bytes((p.stdout+p.stderr).encode());assert p.returncode==0,(p.stdout,p.stderr)
    symbols=maps(mp);needed=[v[0]for v in PROFILE.values()]+['_g_host_engine_interface','_g_host_pool','_g_host_callback_prefix'];assert all(s in symbols for s in needed)
    comps=[]
    for name,(symbol,rva,size) in PROFILE.items():
        if name=='select':continue
        obj,width=definitions[symbol];raw=compare(obj,symbol,ORACLE,rva,size);diag=masked_compare(obj,symbol,ORACLE,rva,size)
        comps.append(dict(role=name,symbol=symbol,source_extent=width,native_extent=size,raw=raw,diagnostic=diag))
    assert next(r for r in comps if r['role']=='switch')['diagnostic']['masked_equal']
    assert all(next(r for r in comps if r['role']==name)['raw']['exact']for name in ['zero','empty','diagnostic'])
    format_index=next(i for i,p in enumerate(files) if p==ROOT/'src/pc/host_pool_compact_format.c')
    co=COFF(objects[format_index]);defined=[s for s in co.symbols.values() if s['storage']==2 and s['section']>0]
    assert len(defined)==1 and defined[0]['name']=='_g_fmt_compact'
    symbol=defined[0];section=co.sections[symbol['section']-1]
    assert not section['flags']&0x20 and not section['relocations']
    actual=section['data'][symbol['value']:symbol['value']+37]
    original=pefile.PE(str(ORACLE)).get_data(0x36130,37)
    assert actual==original and actual.endswith(b'\0') and b'\0'not in actual[:-1]
    format_receipt=dict(rva='0x36130',symbol='_g_fmt_compact',bytes=37,sha256=hashlib.sha256(actual).hexdigest(),object_sha256=sha(objects[format_index]))
    assert before=={str(p):sha(p)for p in watched}
    return dict(inputs=before,compiles=compiles,fixture_source_sha256=sha(fixture),link_command=cmd,dll=str(dll),dll_sha256=sha(dll),map=str(mp),map_sha256=sha(mp),symbols=symbols,source_sizes={s:n for s,(o,n)in definitions.items()},comparisons=comps,compact_format=format_receipt)
def execute(kind,b,scenario):
    pe=pefile.PE(str(ORACLE if kind=='native'else Path(b['dll'])));base=pe.OPTIONAL_HEADER.ImageBase;size=pe.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,pe.get_memory_mapped_image().ljust(size,b'\0'))
    u.mem_map(DATA,0x50000);u.mem_map(HEAP-32,0x2000);u.mem_map(STACK,0x10000)
    addresses={role:0x400000+rva if kind=='native'else b['symbols'][symbol]for role,(symbol,rva,n)in PROFILE.items()}
    table,pool,callback=(0x4361b8,0xba29c0,0xa8e780)if kind=='native'else tuple(b['symbols'][s]for s in ['_g_host_engine_interface','_g_host_pool','_g_host_callback_prefix'])
    ctx_expected=[bytearray(bytes([0xa1+i])*CTX_SIZE)for i in range(3)]
    for a,x in zip(CTXS,ctx_expected):u.mem_write(a-32,bytes([0xc1])*32+x+bytes([0xc2])*32)
    table_before=bytearray(bytes([0x2d])*1000);table_before[4:8]=pack(CTXS[0]);u.mem_write(table,bytes(table_before))
    pool_before=bytearray(bytes([0x3d])*48)
    for i in range(4):pool_before[i*4:i*4+4]=pack(HEAP if i==0 else 0);pool_before[16+i*4:20+i*4]=pack(HEAP+128 if i==0 else 0)
    u.mem_write(pool,bytes(pool_before));cb_before=pack(0x11112222)+pack(0x33334444)+pack(5);u.mem_write(callback,cb_before)
    heap=bytearray(bytes([0x6d])*128+pack(1)+bytes([0xe2])*64)
    owners=[0,0];layout=scenario['layout'];direct=0
    if layout=='empty':heap[:4]=pack(0x80000020);first_words=32
    elif layout=='context':
        heap[:4]=pack(8);heap[4:8]=pack(CTXS[0]+0xe4);heap[32:36]=pack(0x80000018);direct=HEAP+8;first_words=32
    elif layout in ['bulk','pinned','zero_owner']:
        heap[:4]=pack(8);heap[4:8]=pack(SLOTS[0]);heap[32:36]=pack((0x40000000 if layout=='pinned'else 0)+8);heap[36:40]=pack(SLOTS[1]);heap[64:68]=pack(0x80000010)
        owners=[0 if layout=='zero_owner'else HEAP+8,HEAP+40];first_words=8
    else:raise ValueError(layout)
    ctx_expected[0][0xe4:0xe8]=pack(direct);u.mem_write(CTXS[0]+0xe4,pack(direct))
    u.mem_write(HEAP-32,bytes([0xe1])*32+heap)
    for a,v in zip(SLOTS,owners):u.mem_write(a,pack(v))
    u.mem_write(PREFIX,b'C:\\switch\0');u.mem_write(SUFFIX,b'\\TITLE\\ENGINE\\TITLE\0')
    prefix_arg=0 if scenario.get('null_prefix')else PREFIX;suffix_arg=0 if scenario.get('null_suffix')else SUFFIX
    expected_stack=pack(STOP)+pack(prefix_arg)+pack(suffix_arg);u.mem_write(ESP,expected_stack);u.reg_write(UC_X86_REG_ESP,ESP)
    for k,v in SEEDS.items():u.reg_write(REGS[k],v)
    events=[];illegal=[];writes=[];count=0;pending=[];last_pool=None
    ranges=[(addresses[role],addresses[role]+(n if kind=='native'else b['source_sizes'][symbol]))for role,(symbol,rva,n)in PROFILE.items()if role!='select']
    write_ranges=[(a,a+CTX_SIZE)for a in CTXS]+[(HEAP,HEAP+132),(pool,pool+48),(callback,callback+12),(SLOTS[0],SLOTS[1]+4),(ESP-0x400,ESP+12)]
    entries={v:k for k,v in addresses.items()}
    def on_write(machine,access,address,width,value,user):
        if not any(lo<=address and address+width<=hi for lo,hi in write_ranges):writes.append((hex(address),width));machine.emu_stop()
    def on_code(machine,address,width,user):
        nonlocal count,last_pool
        count+=1
        if count>20000:illegal.append('instruction cap');machine.emu_stop();return
        if address==STOP:machine.emu_stop();return
        role=entries.get(address);esp=machine.reg_read(UC_X86_REG_ESP)
        if role=='select':
            events.append(dict(call=role,prefix=cstring(machine,read32(machine,esp+4)),suffix=cstring(machine,read32(machine,esp+8)),context_index=CTXS.index(read32(machine,table+4))))
            pending.append(address);machine.emu_stop();return
        if role=='zero':events.append(dict(call=role,arg=read32(machine,esp+4)))
        elif role=='free':events.append(dict(call=role,payload=hex(read32(machine,esp+4))))
        elif role=='bulk':
            if scenario.get('bulk_context'):machine.mem_write(table+4,pack(CTXS[1]))
            events.append(dict(call=role,id=read32(machine,esp+4)))
        elif role=='set_one':
            if scenario.get('setter_context'):machine.mem_write(table+4,pack(CTXS[2]))
            events.append(dict(call=role,value=read32(machine,esp+4)))
        elif role in ['compact_wrapper','count','coalesce']:events.append(dict(call=role,id=read32(machine,esp+4)))
        elif role=='compact':events.append(dict(call=role,id=read32(machine,esp+4),limit=read32(machine,esp+8)))
        elif role=='reverse':events.append(dict(call=role,destination=hex(read32(machine,esp+4)),source=hex(read32(machine,esp+8)),bytes=read32(machine,esp+12)))
        elif role=='diagnostic':events.append(dict(call=role,format=cstring(machine,read32(machine,esp+4)),id=read32(machine,esp+8),payload=hex(read32(machine,esp+12)),bytes=read32(machine,esp+16)))
        elif role=='empty':events.append(dict(call=role))
        if not any(lo<=address<hi for lo,hi in ranges):illegal.append(hex(address));machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
    pc=addresses['switch']
    for _ in range(3):
        pending.clear();u.emu_start(pc,STOP,count=20001)
        if illegal or writes or u.reg_read(UC_X86_REG_EIP)==STOP:break
        assert pending
        esp=u.reg_read(UC_X86_REG_ESP);pc=read32(u,esp);u.reg_write(UC_X86_REG_ESP,esp+4);u.reg_write(UC_X86_REG_EIP,pc);u.reg_write(UC_X86_REG_EAX,0x13572468);u.reg_write(UC_X86_REG_ECX,0x24681357);u.reg_write(UC_X86_REG_EDX,0xabcdef01)
    if direct:ctx_expected[0][0xe4:0xe8]=pack(0)
    callback_ctx=1 if scenario.get('bulk_context')else 0;flag_ctx=2 if scenario.get('setter_context')else callback_ctx
    ctx_expected[callback_ctx][0xb0:0xb8]=bytes(8);ctx_expected[flag_ctx][0x108:0x10c]=pack(0x801ffff0);ctx_expected[flag_ctx][0x10c:0x110]=bytes(4)
    expected_table=bytearray(table_before);expected_table[4:8]=pack(CTXS[flag_ctx]);expected_pool=bytearray(pool_before);expected_pool[40:44]=pack(HEAP);expected_pool[44:48]=pack(first_words)
    checks=dict(contexts_complete_equal=all(bytes(u.mem_read(a,CTX_SIZE))==bytes(x)for a,x in zip(CTXS,ctx_expected)),context_guards_equal=all(bytes(u.mem_read(a-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(a+CTX_SIZE,32))==bytes([0xc2])*32 for a in CTXS),table_complete_equal=bytes(u.mem_read(table,1000))==bytes(expected_table),pool_complete_equal=bytes(u.mem_read(pool,48))==bytes(expected_pool),callback_prefix_equal=bytes(u.mem_read(callback,12))==cb_before[:4]+pack(0)+cb_before[8:],heap_guards_equal=bytes(u.mem_read(HEAP-32,32))==bytes([0xe1])*32 and bytes(u.mem_read(HEAP+132,64))==bytes([0xe2])*64,returned_to_sentinel=u.reg_read(UC_X86_REG_EIP)==STOP,eax_zero=u.reg_read(UC_X86_REG_EAX)==0,stack_balance=u.reg_read(UC_X86_REG_ESP)==ESP+4,caller_arguments_unchanged=bytes(u.mem_read(ESP,12))==expected_stack,nonvolatiles_preserved=all(u.reg_read(REGS[k])==v for k,v in SEEDS.items()),execution_whitelist=not illegal,write_whitelist=not writes)
    assert all(checks.values()),(kind,scenario,checks,events,illegal,writes)
    assert events[0]==dict(call='zero',arg=0)
    assert sum(e['call']=='bulk'for e in events)==1 and sum(e['call']=='set_one'for e in events)==1
    assert events[-3:]==[dict(call='empty'),dict(call='set_one',value=0),dict(call='select',prefix=None if not prefix_arg else 'C:\\switch',suffix=None if not suffix_arg else '\\TITLE\\ENGINE\\TITLE',context_index=flag_ctx)]
    return dict(events=events,checks=checks,heap_bytes=bytes(u.mem_read(HEAP,132)).hex(),owner_values=[hex(read32(u,a))for a in SLOTS],context_hashes=[hashlib.sha256(bytes(u.mem_read(a,CTX_SIZE))).hexdigest()for a in CTXS],pool_sha256=hashlib.sha256(bytes(u.mem_read(pool,48))).hexdigest(),table_sha256=hashlib.sha256(bytes(u.mem_read(table,1000))).hexdigest(),instructions=count)
def main():
    b=build();scenarios=[dict(name=layout,layout=layout)for layout in ['empty','context','bulk','pinned','zero_owner']]+[dict(name='null_prefix',layout='empty',null_prefix=True),dict(name='null_suffix',layout='context',null_suffix=True),dict(name='both_null',layout='bulk',null_prefix=True,null_suffix=True),dict(name='live_after_bulk',layout='context',bulk_context=True),dict(name='live_after_setter',layout='bulk',setter_context=True),dict(name='two_live_contexts',layout='context',bulk_context=True,setter_context=True)]
    cases=[]
    for s in scenarios:
        n=execute('native',b,s);c=execute('source',b,s);assert {k:v for k,v in n.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'},(s,n,c)
        cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Original engine-switch caller and actual pool/callback/diagnostic code under bounded synthetic data. Selector is a hook-modeled return boundary. No Windows process/API/DLL initialization or full engine switching/game execution.',build=b,cases=cases,case_count=len(cases),all_equal=True,oracle_sha256=sha(ORACLE),runner_sha256=sha(Path(__file__)),limits=['One-DWORD zero-leaf callsite view is not proof of its original common source prototype; accepted no-argument leaf source is unchanged.','Table/contexts/heap/owner-slot inputs and callback-time context interventions are synthetic; no full owner/layout/callee mutation claim.','Complete source/native function diagnostics remain separate from finite semantics and strict acceptance.'])
    dest=OUT/'host_engine_switch_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(cases=len(cases),all_equal=True,actual_pool_callback_execution=True,report=str(dest)),indent=2))
if __name__=='__main__':main()
