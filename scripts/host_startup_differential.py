"""Finite original/source startup and normal-exit control comparison.

Actual shutdown-chain and display-mode source providers execute. Other startup
and cleanup services are explicit synthetic call boundaries, not recovered
game bodies. No original process, DLL initializer, real API or CRT exit runs.
"""
import hashlib,json,struct,sys
from pathlib import Path
SCRIPT=Path(__file__).resolve()
ROOT=SCRIPT.parents[1]
LANE=ROOT/'work/host_startup_differential'
LANE.mkdir(parents=True,exist_ok=True)
sys.path[:0]=[str(ROOT/'scripts'),r'C:\tools\hercules\python',r'C:\tools\hercules\renderer-oracle-python']
from coff import COFF
from match import compile_source,compare,digest,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from pe import PE
from host_dispatch_differential import archive_members
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ORACLE=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
EXPECTED='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
BASE=0x400000;LINK_BASE=0x2000000;LINK_SIZE=0x200000;STACK=0x3000000;STACK_SIZE=0x10000;STOP=0x4000000;HOOK_BASE=0x4100000
CELLS={'entered':('_g_fatal_entered',0xba023c,1),'owner':('_g_fatal_owner_window',0xba0200,4),
       'guard':('_g_startup_apply_guard_rva_51c450',0x91c450,4),
       'gate':('_g_host_input_gate_rva_03a594',0x43a594,1),
       'display':('_g_exe_96e3d0',0x96e3d0,1),'fullscreen':('_g_fatal_suppress_dialog',0xba020c,1)}
FUNCTIONS=[('host_normal_exit.c','_host_normal_exit_104d0',0x104d0,80),
           ('host_startup.c','_host_startup_5e10',0x5e10,128),
           ('host_startup_guard.c','_host_set_config_apply_guard_5750',0x5750,16)]
SERVICES={'_host_registry_read_config':(0x4ce0,0),
    '_archive_initialize':(0x5020,0),'_host_service_72a0':(0x72a0,0),
    '_host_input_initialize_7bb0':(0x7bb0,0),'_host_service_a0b0':(0xa0b0,0),
    '_host_service_fdf0':(0xfdf0,0),'_host_service_1ff0':(0x1ff0,0),
    '_cleanup_native_4ac0':(0x4ac0,0),'_cleanup_native_4da0':(0x4da0,0),
    '_cleanup_native_ff70':(0xff70,0),'_cleanup_native_a120':(0xa120,0),
    '_empty_host_callback':(0xbe30,0),'_cleanup_native_74c0':(0x74c0,0),
    '_archive_shutdown':(0x5110,0)}
APIS={'timeBeginPeriod':('winmm.dll',1),'timeEndPeriod':('winmm.dll',1),
      'MessageBoxA':('user32.dll',4),'SendNotifyMessageA':('user32.dll',4)}
REGS={'eax':UC_X86_REG_EAX,'ebx':UC_X86_REG_EBX,'ecx':UC_X86_REG_ECX,'edx':UC_X86_REG_EDX,
      'esi':UC_X86_REG_ESI,'edi':UC_X86_REG_EDI,'ebp':UC_X86_REG_EBP,'esp':UC_X86_REG_ESP,'eflags':UC_X86_REG_EFLAGS}

def save(p,v):p.write_bytes((json.dumps(v,indent=2)+'\n').encode())
def align(v,n=16):return (v+n-1)&~(n-1)

def build_layout(units):
    cursor=LINK_BASE;sections=[];symbols={};coffs=[]
    for unit in units:
        coff=COFF(Path(unit['object']));coffs.append(coff);addresses=[]
        for section in coff.sections:
            addresses.append(cursor);sections.append((cursor,bytearray(section['data']).ljust(section['size'],b'\0'),section['relocations']))
            cursor=align(cursor+max(section['size'],1))
        for s in coff.symbols.values():
            if s['storage']!=2:continue
            if s['section']>0:
                address=addresses[s['section']-1]+s['value']
                if s['name'] in symbols:assert symbols[s['name']]==address,'duplicate '+s['name']
                symbols[s['name']]=address
            elif s['section']==0 and s['value']:
                assert s['name'] not in symbols,'duplicate COMMON '+s['name']
                symbols[s['name']]=cursor;sections.append((cursor,bytearray(s['value']),[]));cursor=align(cursor+s['value'])
    endpoints={};external={};endpoint_cursor=HOOK_BASE
    for coff in coffs:
        for s in coff.symbols.values():
            if s['storage']!=2 or s['section']!=0 or s['value'] or s['name'] in symbols:continue
            name=s['name'];endpoints[endpoint_cursor]=name
            if name.startswith('__imp_'):
                symbols[name]=cursor;sections.append((cursor,bytearray(struct.pack('<I',endpoint_cursor)),[]));cursor+=4
            else:symbols[name]=endpoint_cursor
            external[name]=endpoint_cursor;endpoint_cursor+=16
    assert cursor<=LINK_BASE+LINK_SIZE
    for address,data,relocs in sections:
        for reloc in relocs:
            off=reloc['offset'];addend=struct.unpack_from('<I',data,off)[0];target=symbols[reloc['symbol']]
            if reloc['type']==6:value=target+addend
            elif reloc['type']==7:value=target+addend-LINK_BASE
            elif reloc['type']==20:value=target+addend-address-off-4
            else:raise ValueError('unsupported COFF relocation '+str(reloc))
            struct.pack_into('<I',data,off,value&0xffffffff)
    return sections,symbols,endpoints,external

def invoke(mode,case,layout,pe):
    sections,symbols,endpoints,external=layout
    uc=Uc(UC_ARCH_X86,UC_MODE_32);uc.mem_map(BASE,align(pe.pe.OPTIONAL_HEADER.SizeOfImage,4096));uc.mem_write(BASE,pe.pe.get_memory_mapped_image())
    uc.mem_map(LINK_BASE,LINK_SIZE);uc.mem_map(STACK,STACK_SIZE);uc.mem_map(STOP,4096);uc.mem_map(HOOK_BASE,4096)
    for address,data,_ in sections:
        if data:uc.mem_write(address,bytes(data))
    cells={k:(native if mode=='original' else symbols[sym],width) for k,(sym,native,width) in CELLS.items()}
    def rd(address,width=4):return int.from_bytes(uc.mem_read(address,width),'little')
    def wr(address,value,width=4):uc.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
    def cell(k,value=None):
        a,w=cells[k]
        if value is not None:wr(a,value,w)
        return rd(a,w)
    def state():return {k:cell(k) for k in cells}
    def string(address):
        result=bytearray()
        for i in range(256):
            b=uc.mem_read(address+i,1)
            if b==b'\0':return result.decode('ascii')
            result.extend(b)
        raise AssertionError('unterminated fixture string')
    original_endpoints={};api_counts={}
    for entry in pe.pe.DIRECTORY_ENTRY_IMPORT:
        for item in entry.imports:
            if not item.name:continue
            name=item.name.decode('ascii')
            if name not in APIS:continue
            assert entry.dll.decode().lower()==APIS[name][0]
            endpoint=HOOK_BASE+0x800+len(original_endpoints)*16
            wr(item.address,endpoint);original_endpoints[endpoint]=name;api_counts[name]=api_counts.get(name,0)+1
    assert set(api_counts)==set(APIS) and all(v==1 for v in api_counts.values())
    events=[];writes=[];boundary=None;returned=False
    hooks={}
    if mode=='original':
        hooks.update({BASE+rva:(name,argc,0) for name,(rva,argc) in SERVICES.items()})
        hooks[BASE+0x11d10]=('_exit',1,0)
        hooks.update({address:(name,APIS[name][1],APIS[name][1]*4) for address,name in original_endpoints.items()})
        startup=BASE+0x5e10;normal=BASE+0x104d0
    else:
        for address,name in endpoints.items():
            if name.startswith('__imp_'):
                api=name.split('__imp__',1)[1].split('@')[0]
                assert api in APIS,'unmapped import '+name
                hooks[address]=(api,APIS[api][1],APIS[api][1]*4)
            elif name in SERVICES:hooks[address]=(name,SERVICES[name][1],0)
            elif name=='_exit':hooks[address]=(name,1,0)
            else:raise AssertionError('unmapped source dependency '+name)
        startup=symbols['_host_startup_5e10'];normal=symbols['_host_normal_exit_104d0']
    actual_guard=BASE+0x5750 if mode=='original' else symbols['_host_set_config_apply_guard_5750']
    actual_chain=BASE+0x5e90 if mode=='original' else symbols['_fatal_shutdown_chain']
    actual_mode=BASE+0x6ec0 if mode=='original' else symbols['_cleanup_native_6ec0']
    def on_write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=STACK+STACK_SIZE:return
        for name,(a,w) in cells.items():
            if a<=address and address+size<=a+w:
                writes.append(dict(cell=name,offset=address-a,bytes=size,value=value&((1<<(size*8))-1)));return
        raise AssertionError('untracked non-stack write '+hex(address))
    def on_code(uc,address,size,user):
        nonlocal boundary,returned
        if address==STOP:returned=True;uc.emu_stop();return
        sp=uc.reg_read(UC_X86_REG_ESP)
        if address==actual_guard:events.append(dict(call='actual_config_guard_setter',args=[rd(sp+4)],state=state()))
        if address==actual_chain:events.append(dict(call='actual_shutdown_chain',state=state()))
        if address==actual_mode:events.append(dict(call='actual_display_mode_setter',args=[rd(sp+4)],state=state()))
        if address==normal:events.append(dict(call='actual_normal_exit',state=state()))
        if address not in hooks:return
        name,argc,pop=hooks[address];args=[rd(sp+4+4*i) for i in range(argc)]
        event=dict(call=name,args=args,state=state())
        if name=='MessageBoxA':
            event['text']=string(args[1]);event['caption']=string(args[2])
            event['args']=[args[0],{'literal':event['text'],'bytes':len(event['text'])+1},
                           {'literal':event['caption'],'bytes':len(event['caption'])+1},args[3]]
        events.append(event)
        if name=='_exit':boundary=dict(call=name,args=args);uc.emu_stop();return
        if name==case.get('mutation_at'):
            for k,v in case.get('mutation',{}).items():cell(k,v)
        if name=='MessageBoxA':
            for k,v in case.get('message_mutation',{}).items():cell(k,v)
        result=case.get('api_result',0x80000001) if name in APIS else 0xa5a5a5a5
        uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ECX,0xc1c1c1c1);uc.reg_write(UC_X86_REG_EDX,0xd2d2d2d2)
        uc.reg_write(UC_X86_REG_ESP,sp+4+pop);uc.reg_write(UC_X86_REG_EIP,rd(sp))
    uc.hook_add(UC_HOOK_CODE,on_code);uc.hook_add(UC_HOOK_MEM_WRITE,on_write)
    for k,v in dict(entered=0,owner=0,guard=0,gate=1,display=0xa5,fullscreen=0).items():cell(k,v)
    for k,v in case.get('initial',{}).items():cell(k,v)
    before=state();sp=STACK+STACK_SIZE-0x100;wr(sp,STOP)
    if case['entry']=='guard':wr(sp+4,case['value'])
    for i,(k,r) in enumerate(REGS.items()):uc.reg_write(r,0x11111111*(i+1)&0xffffffff)
    uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_EFLAGS,0x202)
    saved={k:uc.reg_read(REGS[k]) for k in ('ebx','esi','edi','ebp')}
    entry={'startup':startup,'normal':normal,'guard':actual_guard}[case['entry']]
    uc.emu_start(entry,0,count=20000)
    assert returned or boundary
    if returned:
        assert uc.reg_read(UC_X86_REG_ESP)==sp+4
        assert all(uc.reg_read(REGS[k])==v for k,v in saved.items())
    return dict(before=before,after=state(),events=events,writes=writes,boundary=boundary,normal_return=returned,
        return_ABI_checked=returned)

def main():
    assert digest(ORACLE.read_bytes())==EXPECTED
    paths=[ROOT/'src/pc'/n for n,_,_,_ in FUNCTIONS]+[ROOT/'src/pc'/n for n in ('fatal_cleanup_chain.c','display_mode_setter.c','fatal_storage.c','host_startup_storage.c','graphics_storage.cpp')]
    headers=[ROOT/'src/pc/host_startup_storage.h',DEFAULT_TOOLCHAIN/'include/windows.h',DEFAULT_TOOLCHAIN/'include/mmsystem.h',DEFAULT_TOOLCHAIN/'include/stdlib.h']
    snapshot={str(p):digest(p.read_bytes()) for p in paths+headers+[ORACLE,Path(__file__),ROOT/'scripts/host_dispatch_differential.py',ROOT/'toolchains/manifest.json']}
    units=[]
    for p in paths:
        obj,command=compile_source(p,['/O2']);units.append(dict(source=str(p),source_sha256=digest(p.read_bytes()),object=str(obj),object_sha256=digest(obj.read_bytes()),command=command))
    raw=[dict(symbol=symbol,rva=hex(rva),size=size,raw=compare(Path(units[i]['object']),symbol,ORACLE,rva,size),diagnostic_only=masked_compare(Path(units[i]['object']),symbol,ORACLE,rva,size)) for i,(_,symbol,rva,size) in enumerate(FUNCTIONS)]
    library_map=Path('D:/Prog/hercules_recon/work/library_map/library_map.json');lib=DEFAULT_TOOLCHAIN/'lib/libcmt.lib'
    assert digest(lib.read_bytes())=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    library=json.loads(library_map.read_bytes());record=next(r for r in library['contributions']['HERCULES.EXE'] if r['rva']=='0x11d10')
    assert record['names']==['_exit'] and record['member']==335 and record['size']==32
    member=dict(archive_members(lib.read_bytes()))[335]
    mp=LANE/'exit_member.obj';mp.write_bytes(member);c=COFF(mp)
    symbol=next(s for s in c.symbols.values() if s['name']=='_exit' and s['section']>0)
    contribution=c.sections[symbol['section']-1];assert symbol['value']==0 and contribution['size']==32
    layout=build_layout(units);pe=PE(ORACLE)
    cases=[dict(name='startup_ready',entry='startup',initial=dict(owner=0x12340000)),
        dict(name='startup_initial_failure_becomes_ready',entry='startup',initial=dict(gate=0),mutation_at='_host_service_a0b0',mutation=dict(gate=0xff)),
        dict(name='startup_config_apply_clears_gate',entry='startup',mutation_at='_cleanup_native_4ac0',mutation=dict(gate=0)),
        dict(name='startup_apply_sets_gate',entry='startup',initial=dict(gate=0),mutation_at='_cleanup_native_4ac0',mutation=dict(gate=1)),
        dict(name='startup_missing_media_null_window',entry='startup',initial=dict(gate=0)),
        dict(name='startup_missing_media_message_mutates_owner',entry='startup',initial=dict(gate=0,owner=0x11110000),message_mutation=dict(owner=0x22220000)),
        dict(name='startup_missing_media_shutdown_clears_owner',entry='startup',initial=dict(gate=0,owner=0x11110000),mutation_at='_cleanup_native_a120',mutation=dict(owner=0)),
        dict(name='normal_exit_no_window',entry='normal'),
        dict(name='normal_exit_window',entry='normal',initial=dict(owner=0x12340000)),
        dict(name='normal_exit_repeated',entry='normal',initial=dict(entered=1,owner=0x12340000)),
        dict(name='normal_exit_nonboolean_guard',entry='normal',initial=dict(entered=0xff)),
        dict(name='normal_exit_cleanup_reloads_owner',entry='normal',initial=dict(owner=0x11110000),mutation_at='_cleanup_native_74c0',mutation=dict(owner=0x33330000)),
        dict(name='normal_exit_cleanup_sets_window',entry='normal',mutation_at='_archive_shutdown',mutation=dict(owner=0x44440000)),
        dict(name='normal_exit_cleanup_clears_window',entry='normal',initial=dict(owner=0x11110000),mutation_at='_cleanup_native_4da0',mutation=dict(owner=0))]
    cases.append(dict(name='startup_resets_nonzero_apply_guard',entry='startup',initial=dict(guard=0x12345678)))
    cases.extend(dict(name='guard_value_'+hex(value),entry='guard',value=value,initial=dict(guard=0xa5a5a5a5)) for value in (0,1,0xffffffff,0x80000000,0x12345678))
    results=[]
    for case in cases:
        original=invoke('original',case,layout,pe);candidate=invoke('candidate',case,layout,pe)
        results.append(dict(case=case,original=original,candidate=candidate,equal=original==candidate))
        if original!=candidate:
            save(LANE/'mismatch.json',results[-1]);raise AssertionError('differential mismatch '+case['name'])
    assert snapshot=={str(p):digest(p.read_bytes()) for p in paths+headers+[ORACLE,Path(__file__),ROOT/'scripts/host_dispatch_differential.py',ROOT/'toolchains/manifest.json']}
    report=dict(scope=__doc__,oracle=dict(path=str(ORACLE),sha256=EXPECTED),toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
        frozen_inputs=snapshot,units=units,comparisons=raw,
        CRT=dict(map_sha256=digest(library_map.read_bytes()),archive_sha256=digest(lib.read_bytes()),record=record,
            member_sha256=digest(member),contribution_sha256=digest(contribution['data']),boundary='Intercepted at exit entry. No CRT exit or process termination executes.'),
        synthetic_external_bindings={n:hex(v) for n,v in layout[3].items()},
        cases=results,case_count=len(results),failures=0,all_equal=True,
        limits=['Only startup call-order/state protocol and normal-exit control are compared.',
            'MessageBox literal pointer operands are compared by complete NUL-terminated content and argument role, not equal linked addresses.',
            'Config/input/apply and unresolved startup/cleanup service effects use explicit synthetic boundaries; their complete bodies are outside this fixture.',
            'Actual Config-guard setter, shutdown-chain and requested-mode setter providers execute; other subordinate bodies are intercepted.',
            'Fresh provider storage supplies measured prefixes only, without proving historical object/TU/layout ownership.',
            'No real OS, original process, original DLL initializer, full startup/menu/gameplay or exact function acceptance.'])
    save(LANE/'report.json',report)
    print(json.dumps(dict(case_count=len(results),failures=0,all_equal=True,raw_equal=[x['raw']['exact'] for x in raw]),indent=2))

if __name__=='__main__':main()
