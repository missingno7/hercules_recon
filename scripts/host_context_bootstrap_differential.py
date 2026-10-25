"""Fresh bounded context/arena bootstrap comparison; never starts a game process."""
import argparse,hashlib,json,os,re,struct,subprocess,sys
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,DEFAULT_TOOLCHAIN,verify_toolchain
from masked import masked_compare
from coff import COFF
from pe import PE
from host_dispatch_differential import archive_members
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_context_bootstrap_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args();OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve());OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'src/pc/host_context_bootstrap.c'
ORACLE=args.native_exe.resolve()
ORACLE_SHA='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
CTX_SIZE=0x1b34;POOL_SIZE=48;TABLE_SIZE=1000
DATA=0x60000000;CONTEXTS=[DATA+0x10020,DATA+0x20020,DATA+0x30020]
ARENA=0x61000020;STACK=0x62000000;ESP=STACK+0x8000;SENTINEL=STACK+0xf000
NAME_A=DATA+0x100;NAME_B=DATA+0x200;CAP=900000
REGS={'ebx':UC_X86_REG_EBX,'esi':UC_X86_REG_ESI,'edi':UC_X86_REG_EDI,'ebp':UC_X86_REG_EBP}
SEEDS={'ebx':0x1a2b3c4d,'esi':0x293a4b5c,'edi':0x394a5b6c,'ebp':0x495a6b7c}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
def u32(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def string(u,a):
    b=bytearray()
    for i in range(512):
        v=u.mem_read(a+i,1)[0]
        if not v:return b.decode('ascii')
        b.append(v)
    raise ValueError('unterminated fixture string')
def native_loaded(path):
    p=pefile.PE(str(path));return p,p.get_memory_mapped_image().ljust(p.OPTIONAL_HEADER.SizeOfImage,b'\0')
def map_symbols(path):
    pattern=re.compile(r'^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})')
    return {m.group(1):int(m.group(2),16)for l in path.read_text(errors='replace').splitlines()if (m:=pattern.match(l))}
def build():
    assert sha(ORACLE)==ORACLE_SHA
    verify_toolchain(DEFAULT_TOOLCHAIN)
    watched=[SOURCE,ROOT/'src/shared/engine_interface.h',ROOT/'src/shared/resource_context.h',ROOT/'src/pc/host_pool_repaired.c',ROOT/'src/pc/host_pool_storage.c',ROOT/'src/pc/host_pool_formats.c',ROOT/'src/pc/host_diagnostic.c',Path(__file__).resolve(),ROOT/'toolchains/manifest.json',ROOT/'scripts/host_dispatch_differential.py',ROOT/'scripts/coff.py',ROOT/'scripts/match.py',ROOT/'scripts/masked.py',ORACLE]
    watched += [DEFAULT_TOOLCHAIN/'include/stdlib.h',DEFAULT_TOOLCHAIN/'include/string.h',ROOT/'src/pc/host_bootstrap_text.c',ROOT/'src/pc/host_bootstrap_text.h']
    archives=[DEFAULT_TOOLCHAIN/'lib'/n for n in ['libcmt.lib','kernel32.lib','user32.lib','advapi32.lib']]
    watched+=archives
    before={str(p):sha(p)for p in watched}
    archive=archives[0]
    assert sha(archive)=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    body=next(body for index,body in archive_members(archive.read_bytes()) if index==307)
    assert hashlib.sha256(body).hexdigest()=='d02d282d74293e0030f86506ed1d5ec224fc115e8d435fe813a012eb47866180'
    crt=OUT/'libcmt_srand_member307.obj';crt.write_bytes(body)
    crt_raw=compare(crt,'_srand',ORACLE,0x10a30,16)
    crt_diagnostic=masked_compare(crt,'_srand',ORACLE,0x10a30,16)
    assert crt_diagnostic['masked_equal'] and crt_diagnostic['symbols']=={'__getptd':'0x12100'}
    header=os.path.relpath(ROOT/'src/shared/engine_interface.h',OUT).replace('\\','/')
    fixture=OUT/'fixture.c'
    fixture.write_bytes(('''#include "%s"
extern void __cdecl host_service_1ff0(void);
EngineInterface g_host_engine_interface = {0};
void __cdecl host_service_10690(void) {}
int __cdecl host_service_4040(const char *a,const char *b) { return 0; }
/* Unexecuted link boundaries; code whitelist rejects every entry. */
int main(void) { for (;;) {} }
__declspec(dllexport) void __cdecl context_bootstrap_fixture_anchor(void) { host_service_1ff0(); }
'''%header).encode())
    sources=[SOURCE,ROOT/'src/pc/host_pool_repaired.c',ROOT/'src/pc/host_pool_storage.c',fixture,ROOT/'src/pc/host_pool_formats.c',ROOT/'src/pc/host_diagnostic.c',ROOT/'src/pc/host_bootstrap_text.c']
    objects=[];compiles=[]
    for s in sources:
        obj,cmd=compile_source(s,['/O2'])
        objects.append(obj);compiles.append(dict(source=str(s),source_sha256=sha(s),object=str(obj),object_sha256=sha(obj),command=cmd))
    dll=OUT/'fixture.dll';mp=OUT/'fixture.map'
    env=os.environ.copy()
    for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OUT:'+str(dll),'/MAP:'+str(mp),*[str(p)for p in objects],*[str(p)for p in archives]]
    p=subprocess.run(command,cwd=OUT,env=env,capture_output=True,text=True)
    (OUT/'link.log').write_bytes((p.stdout+p.stderr).encode())
    assert p.returncode==0,(p.stdout,p.stderr)
    symbols=map_symbols(mp)
    needed=['_host_service_1ff0','_save_arena','_host_service_10690','_host_service_4040','_srand','_g_host_engine_interface','_g_host_pool','_g_host_default_engine_name_cell_35100','_g_host_empty_text_3f74c']
    assert all(n in symbols for n in needed)
    comparisons=[]
    for obj,sym,rva,size in [(objects[0],'_host_service_1ff0',0x1ff0,208),(objects[1],'_save_arena',0x4450,32),(objects[5],'_host_diagnostic',0x63c0,16)]:
        raw=compare(obj,sym,ORACLE,rva,size);diag=masked_compare(obj,sym,ORACLE,rva,size)
        assert diag['masked_equal']
        comparisons.append(dict(symbol=sym,raw=raw,diagnostic=diag,ordered_relocations=COFF(obj).function(sym)['relocations']))
    assert comparisons[2]['raw']['exact']
    strings=[];coff=COFF(objects[4]);native=PE(ORACLE)
    defined=[s for s in coff.symbols.values() if s['storage']==2 and s['section']>0]
    string_specs={'_g_fmt_init_pool':(0x3608c,32),'_g_fmt_alloc':(0x360ac,30),'_g_fmt_alloc_failed':(0x360cc,36),'_g_fmt_free':(0x36110,29)}
    assert {s['name'] for s in defined}==set(string_specs)
    for symbol in defined:
        rva,size=string_specs[symbol['name']];section=coff.sections[symbol['section']-1]
        assert not section['flags']&0x20 and not section['relocations']
        original=native.read_rva(rva,size);actual=section['data'][symbol['value']:symbol['value']+size]
        assert actual==original and actual.endswith(b'\0') and b'\0' not in actual[:-1]
        strings.append(dict(symbol=symbol['name'],rva=hex(rva),bytes=size,sha256=hashlib.sha256(actual).hexdigest(),compiled_offset=symbol['value']))
    assert before=={str(p):sha(p)for p in watched}
    return dict(inputs=before,compiles=compiles,fixture_source_sha256=sha(fixture),link_command=command,link_returncode=p.returncode,dll=str(dll),dll_sha256=sha(dll),map=str(mp),map_sha256=sha(mp),symbols=symbols,comparisons=comparisons,pool_format_fields=strings,crt_srand=dict(member_index=307,member_sha256=sha(crt),raw=crt_raw,diagnostic=crt_diagnostic))
def execute(kind,build,scenario):
    p,image=native_loaded(ORACLE if kind=='native' else Path(build['dll']))
    base=p.OPTIONAL_HEADER.ImageBase;size=p.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095);u.mem_write(base,image)
    u.mem_map(DATA,0x50000);u.mem_map(ARENA-32,0x302000);u.mem_map(STACK,0x10000)
    if kind=='native':
        addresses=dict(entry=0x401ff0,save=0x404450,setup=0x410690,seed=0x410a30,switch=0x404040,table=0x4361b8,pool=0xba29c0,name_cell=0x435100,empty_text=0x43f74c)
    else:
        m=build['symbols'];addresses={k:m[n]for k,n in [('entry','_host_service_1ff0'),('save','_save_arena'),('setup','_host_service_10690'),('seed','_srand'),('switch','_host_service_4040'),('table','_g_host_engine_interface'),('pool','_g_host_pool'),('name_cell','_g_host_default_engine_name_cell_35100'),('empty_text','_g_host_empty_text_3f74c')]}
    table=addresses['table'];pool=addresses['pool'];n=scenario['arena_bytes']
    expected_contexts=[bytearray(bytes([0xa1+i])*CTX_SIZE)for i in range(3)]
    for a,b in zip(CONTEXTS,expected_contexts):u.mem_write(a-32,bytes([0xc1])*32+b+bytes([0xc2])*32)
    u.mem_write(ARENA-32,bytes([0xd1])*32+bytes([0x5d])*n+bytes([0xd2])*64)
    table_before=bytearray(bytes([0x2d])*TABLE_SIZE)
    for offset,value in [(4,CONTEXTS[0]),(8,ARENA),(12,n)]:table_before[offset:offset+4]=pack(value)
    u.mem_write(table,bytes(table_before));u.mem_write(pool,bytes([0x3d])*POOL_SIZE)
    u.mem_write(NAME_A,b'\\TITLE\\ENGINE\\TITLE\0');u.mem_write(NAME_B,b'\\ENGINE1\\ENGINE1\0')
    assert string(u,u32(u,addresses['name_cell'])) == '\\TITLE\\ENGINE\\TITLE'
    assert u.mem_read(addresses['empty_text'],1) == b'\0'
    u.mem_write(ESP,pack(SENTINEL));u.reg_write(UC_X86_REG_ESP,ESP)
    for k,v in SEEDS.items():u.reg_write(REGS[k],v)
    events=[];illegal=[];writes=[];count=0;pending=[]
    allowed=[(addresses['entry'],addresses['entry']+208),(addresses['save'],addresses['save']+32)]
    write_ranges=[(a,a+CTX_SIZE)for a in CONTEXTS]+[(ARENA,ARENA+n),(pool+32,pool+40),(ESP-0x200,ESP+4)]
    def on_write(machine,access,address,width,value,user):
        if not any(lo<=address and address+width<=hi for lo,hi in write_ranges):
            writes.append(dict(address=hex(address),width=width));machine.emu_stop()
    def on_code(machine,address,width,user):
        nonlocal count
        count+=1
        if count>CAP:illegal.append('instruction cap');machine.emu_stop();return
        if address==SENTINEL:machine.emu_stop();return
        if address in [addresses['setup'],addresses['seed'],addresses['switch']]:
            pending.append(address);machine.emu_stop();return
        if address==addresses['save']:
            esp=machine.reg_read(UC_X86_REG_ESP)
            events.append(dict(call='save_arena',arena=hex(u32(machine,esp+4)),bytes=u32(machine,esp+8)))
        if not any(lo<=address<hi for lo,hi in allowed):
            illegal.append(hex(address));machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
    pc=addresses['entry']
    for _ in range(10):
        pending.clear();u.emu_start(pc,SENTINEL,count=CAP+1)
        if illegal or writes or u.reg_read(UC_X86_REG_EIP)==SENTINEL:break
        assert pending,'unexpected bounded stop'
        address=pending[-1];esp=u.reg_read(UC_X86_REG_ESP)
        if address==addresses['setup']:
            c=u32(u,table+4)
            events.append(dict(call='setup_10690',context_index=CONTEXTS.index(c),byte9=u.mem_read(c+9,1)[0],dword30=u32(u,c+0x30),pool_saved_arena=hex(u32(u,pool+32)),pool_saved_bytes=u32(u,pool+36)))
            if scenario.get('setup_swap'):u.mem_write(table+4,pack(CONTEXTS[1]))
        elif address==addresses['seed']:
            c=u32(u,table+4)
            events.append(dict(call='srand',seed=u32(u,esp+4),context_index=CONTEXTS.index(c),byte0=u.mem_read(c,1)[0],byte3=u.mem_read(c+3,1)[0],word12c=bytes(u.mem_read(c+0x12c,2)).hex(),word12e=bytes(u.mem_read(c+0x12e,2)).hex(),magic48=hex(u32(u,c+0x48)),magic4c=hex(u32(u,c+0x4c))))
            if scenario.get('seed_mutation'):
                u.mem_write(table+4,pack(CONTEXTS[2]));u.mem_write(addresses['name_cell'],pack(NAME_B))
        else:
            events.append(dict(call='switch_4040',prefix=string(u,u32(u,esp+4)),suffix=string(u,u32(u,esp+8)),context_index=CONTEXTS.index(u32(u,table+4))))
        pc=u32(u,esp);u.reg_write(UC_X86_REG_ESP,esp+4);u.reg_write(UC_X86_REG_EIP,pc)
        u.reg_write(UC_X86_REG_EAX,0);u.reg_write(UC_X86_REG_ECX,0x12345678);u.reg_write(UC_X86_REG_EDX,0x87654321)
    expected_contexts[0]=bytearray(CTX_SIZE)
    index=1 if scenario.get('setup_swap') else 0
    b=expected_contexts[index];b[0]=0xff;b[3]=0x11;b[0x12c:0x130]=bytes(4);b[0x48:0x4c]=pack(0x13546547);b[0x4c:0x50]=pack(0xecab9ab8)
    expected_table=bytearray(table_before)
    if scenario.get('setup_swap'):expected_table[4:8]=pack(CONTEXTS[1])
    if scenario.get('seed_mutation'):expected_table[4:8]=pack(CONTEXTS[2])
    expected_pool=bytearray(bytes([0x3d])*POOL_SIZE);expected_pool[32:36]=pack(ARENA);expected_pool[36:40]=pack(n)
    checks=dict(contexts_complete_equal=all(bytes(u.mem_read(a,CTX_SIZE))==bytes(b)for a,b in zip(CONTEXTS,expected_contexts)),context_guards_equal=all(bytes(u.mem_read(a-32,32))==bytes([0xc1])*32 and bytes(u.mem_read(a+CTX_SIZE,32))==bytes([0xc2])*32 for a in CONTEXTS),arena_complete_zero=bytes(u.mem_read(ARENA,n))==bytes(n),arena_guards_equal=bytes(u.mem_read(ARENA-32,32))==bytes([0xd1])*32 and bytes(u.mem_read(ARENA+n,64))==bytes([0xd2])*64,table_complete_equal=bytes(u.mem_read(table,TABLE_SIZE))==bytes(expected_table),pool_complete_equal=bytes(u.mem_read(pool,POOL_SIZE))==bytes(expected_pool),returned_to_sentinel=u.reg_read(UC_X86_REG_EIP)==SENTINEL,stack_balanced=u.reg_read(UC_X86_REG_ESP)==ESP+4,nonvolatile_preserved=all(u.reg_read(REGS[k])==v for k,v in SEEDS.items()),execution_whitelist=not illegal,write_whitelist=not writes)
    assert all(checks.values()),(kind,scenario,checks,events,illegal,writes)
    assert [e['call']for e in events]==['save_arena','setup_10690','srand','switch_4040']
    assert events[0]==dict(call='save_arena',arena=hex(ARENA),bytes=n)
    assert events[1]==dict(call='setup_10690',context_index=0,byte9=0,dword30=0,pool_saved_arena=hex(ARENA),pool_saved_bytes=n)
    assert events[2]==dict(call='srand',seed=0x13546547,context_index=index,byte0=0xff,byte3=0x11,word12c='0000',word12e='0000',magic48='0x13546547',magic4c='0xecab9ab8')
    assert events[3]==dict(call='switch_4040',prefix='',suffix='\\ENGINE1\\ENGINE1' if scenario.get('seed_mutation')else '\\TITLE\\ENGINE\\TITLE',context_index=2 if scenario.get('seed_mutation')else index)
    return dict(events=events,checks=checks,context_hashes=[hashlib.sha256(bytes(u.mem_read(a,CTX_SIZE))).hexdigest()for a in CONTEXTS],pool_sha256=hashlib.sha256(bytes(u.mem_read(pool,POOL_SIZE))).hexdigest(),table_sha256=hashlib.sha256(bytes(u.mem_read(table,TABLE_SIZE))).hexdigest(),arena_sha256=hashlib.sha256(bytes(u.mem_read(ARENA,n))).hexdigest(),instructions=count)
def main():
    b=build();cases=[]
    scenarios=[dict(name='remainder_'+str(n),arena_bytes=n)for n in [0,1,2,3,4,5,7,17]]+[dict(name='original_three_mib',arena_bytes=3*1024*1024),dict(name='setup_live_context',arena_bytes=19,setup_swap=True),dict(name='seed_live_name_and_context',arena_bytes=21,seed_mutation=True),dict(name='ordered_two_mutations',arena_bytes=23,setup_swap=True,seed_mutation=True)]
    for scenario in scenarios:
        a=execute('native',b,scenario);c=execute('source',b,scenario)
        assert {k:v for k,v in a.items()if k!='instructions'}=={k:v for k,v in c.items()if k!='instructions'}
        cases.append(dict(scenario=scenario,native=a,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Bounded initializer and actual save_arena code only. Synthetic table/context/arena preconditions and setup/srand/switch endpoint effects; no process/DLL initializer/Windows API/whole startup/game execution.',build=b,cases=cases,case_count=len(cases),all_equal=True,original_context_prefix=6964,original_configured_arena_bytes=3145728,oracle_sha256=sha(ORACLE),runner_sha256=sha(Path(__file__)),limits=['Actual initial default-name cell/string and observed one-byte empty-text provider are supplied. No complete empty-text extent or full host-table initializer is accepted.','Mutating callees are synthetic discriminators of caller reload behavior, not recovered callee implementations.','Complete relocation diagnostics are not FUNCTION_MATCH; real module layout and initialized gameplay remain unproved.'])
    dest=OUT/'host_context_bootstrap_differential.json';dest.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(cases=len(cases),all_equal=True,full_context_bytes=6964,full_original_arena_bytes=3145728,report=str(dest)),indent=2))
if __name__=='__main__':main()
