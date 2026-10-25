"""Guarded pool startup and composed shutdown/reset comparisons in Unicorn.

Shutdown executes all three recovered reset loops. Lifecycle services are
explicitly modeled; storage and readable capacities are synthetic. No original
OS module load, instruction patch or host/game startup occurs.
"""
import argparse,itertools,json,os,struct,subprocess,sys
from pathlib import Path
from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN,compile_source,digest,verify_toolchain,compile_inputs
from masked import masked_compare
from pe import PE
from refinery_forms import function_text,replace_function
from title_link import read_map
sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
PIPELINE=ROOT/'src/eng1/actor_pipeline.c';STARTUP=ROOT/'src/eng1/pool_startup.c'
RESET=ROOT/'src/shared/actor_pool_reset_repaired.c';ALIAS=ROOT/'src/eng1/actor_pool_limit_aliases.h'
DATA=0x61000000;STACK=0x62000000;SP=STACK+0x8000;STOP=STACK+0xf000
NV=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP);SEEDS=(0x12345678,0x23456789,0x3456789a,0x456789ab)
SPECS={
 'normal':dict(stride=0x134,pointer=0x72860,limit=0x713b4,start=0x71378,cursor=0x713c4,reset=0x27490,startup=0x274e0,remove=('remove_actor',0x27260),init=('initialize_normal_actor',0x27520)),
 'middle':dict(stride=0xec,pointer=0x73494,limit=0x71432,start=0x713aa,cursor=0x713b8,reset=0x279a0,startup=0x279f0,remove=('remove_middle_actor',0x27860),init=('initialize_middle_actor',0x27a50)),
 'alternate':dict(stride=0x94,pointer=0x734e0,limit=0x713ba,start=0x7138c,cursor=0x71430,reset=0x27f50,startup=0x27fa0,remove=('release_actor',0x27e20),init=('initialize_alternate_actor',0x27fe0)),
}
GROUPS=[DATA+0xe100+n*0x40 for n in range(4)]
pack=lambda n:struct.pack('<I',n&0xffffffff)

def extract(source,keep,obj):
    text=source.read_text()
    for symbol in COFF(obj).symbols.values():
        if symbol['section']>0 and symbol['type']&0x20 and symbol['name'][1:] not in keep:
            name=symbol['name'][1:];body=function_text(text,name)
            text=replace_function(text,name,body[:body.index('{')].rstrip()+';')
    return text

def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN);runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins={str(p):digest(p.read_bytes()) for p in [Path(__file__).resolve(),ORACLE,ALIAS,runtime,ROOT/'toolchains/manifest.json']}
    for src in [PIPELINE,STARTUP,RESET]:pins.update({str(p):h for p,h in compile_inputs(src,['/O2'],DEFAULT_TOOLCHAIN).items()})
    out.mkdir(parents=True,exist_ok=True);objects=[];commands=[]
    pipeline,cmd=compile_source(PIPELINE,['/O2']);commands.append(cmd)
    reset,cmd=compile_source(RESET,['/O2']);commands.append(cmd)
    shutdown=out/'shutdown.c';shutdown.write_text(extract(PIPELINE,{'engine_shutdown'},pipeline))
    resetters=out/'resetters.c';resetters.write_text('#include "'+ALIAS.as_posix()+'"\n'+extract(RESET,{'reset_'+f+'_actor_pool' for f in SPECS},reset))
    services=out/'services.c'
    names=['g_configuration_word_%x'%s[k] for s in SPECS.values() for k in ('limit','start','cursor')]
    services.write_text('/* Synthetic fixture only; all lifecycle services intercepted. */\n#include "'+(ROOT/'src/eng1/resource_runtime_data.h').as_posix()+'"\n'+''.join('void *g_'+f+'_actor_pool;\n' for f in SPECS)+'unsigned short '+','.join(names+['g_configuration_word_71380'])+';\nPendingActionGroup *pending_action_group_table[20];\n'+''.join('int '+s['remove'][0]+'(void *p){for(;;){}}\nvoid '+s['init'][0]+'(void *p){for(;;){}}\n' for s in SPECS.values())+'void clear_actor(void *p){for(;;){}}\n')
    for source,flags in [(shutdown,['/O2','/I'+str(PIPELINE.parent)]),(resetters,['/O2']),(STARTUP,['/O2']),(services,['/O2'])]:
        obj,cmd=compile_source(source,flags);objects.append(obj);commands.append(cmd)
    assert masked_compare(objects[0],'_engine_shutdown',ORACLE,0x256a0,64)['masked_equal']
    for f,spec in SPECS.items():
        assert masked_compare(objects[1],'_reset_'+f+'_actor_pool',ORACLE,spec['reset'],80)['masked_equal']
        assert masked_compare(objects[2],'_eng1_call_%x'%spec['startup'],ORACLE,spec['startup'],96 if f=='middle' else 64)['masked_equal']
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    link=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),*map(str,objects)]
    env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    ran=subprocess.run(link,cwd=out,env=env,capture_output=True,text=True,timeout=60);(out/'link.log').write_text(ran.stdout+ran.stderr)
    assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,commands=commands+[link],object_sha256=[digest(p.read_bytes()) for p in objects],image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)

def execute(pe,symbols,case,native):
    u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.image_base;extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,extent);u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'));u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000)
    payload=bytearray(bytes((i*13+31)&255 for i in range(0x10000)));u.mem_write(DATA,bytes(payload))
    cells={};banks={};helpers={};services={};live={};starts={};cursor_initial={}
    def address(rva,name):return base+rva if native else symbols['_'+name]
    for n,(family,spec) in enumerate(SPECS.items()):
        bank=[DATA+0x100+n*0x4000,DATA+0x2100+n*0x4000];banks[family]=bank
        cells[family]={key:address(spec[key],'g_'+family+'_actor_pool' if key=='pointer' else 'g_configuration_word_%x'%spec[key]) for key in ('pointer','limit','start','cursor')}
        live[family]=dict(bank=0,count=case.get('count',2) if case['entry']=='shutdown' or case['entry']==family else 0,removes=0,finishes=0)
        starts[family]=(case.get('start_seed',0x1122)+n*0x2211)&0xffff;cursor_initial[family]=0x9000+n
        u.mem_write(cells[family]['pointer'],pack(bank[0]));u.mem_write(cells[family]['limit'],struct.pack('<H',live[family]['count']))
        u.mem_write(cells[family]['start'],struct.pack('<H',starts[family]));u.mem_write(cells[family]['cursor'],struct.pack('<H',cursor_initial[family]))
        reset_at=address(spec['reset'],'reset_'+family+'_actor_pool');helpers[reset_at]=(family,80)
        services[address(spec['remove'][1],spec['remove'][0])]=(family,'remove')
        services[address(spec['init'][1],spec['init'][0])]=(family,'init')
    clear=address(0x55b0,'clear_actor');services[clear]=(None,'clear')
    group_count=address(0x71380,'g_configuration_word_71380');table=address(0x713e0,'pending_action_group_table')
    group_limit=case.get('groups',2);u.mem_write(group_count,struct.pack('<H',group_limit))
    group_ptrs=GROUPS.copy()
    for n,ptr in enumerate(group_ptrs):u.mem_write(table+n*4,pack(ptr))
    u.mem_write(table+16,b'\0'*64)
    if case['entry']=='shutdown':entry=address(0x256a0,'engine_shutdown');entry_size=64
    else:
        spec=SPECS[case['entry']];entry=address(spec['startup'],'eng1_call_%x'%spec['startup']);entry_size=96 if case['entry']=='middle' else 64
    frame=pack(STOP)+b'\x93'*32;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
    for r,v in zip(NV,SEEDS):u.reg_write(r,v)
    transcript=[];reset_entries=[];cursor_writes=[];group_writes=[];pending={}
    def get32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def mark(actor,role):
        at=actor-DATA+0x10;value=(len(transcript)*37+(1 if role=='remove' else 2))&255
        payload[at:at+4]=bytes([value])*4;u.mem_write(DATA+at,bytes([value])*4)
    def mutate(family,stage):
        nonlocal group_limit,group_ptrs
        info=live[family];target=case.get('mutate_stage')
        if target!=stage or info[stage+'s' if stage=='remove' else 'finishes']!=1:return
        if 'new_count' in case:
            info['count']=case['new_count'];u.mem_write(cells[family]['limit'],struct.pack('<H',info['count']))
        if case.get('switch'):
            info['bank']=1;u.mem_write(cells[family]['pointer'],pack(banks[family][1]))
        if case['entry']=='middle' and 'new_groups' in case:
            group_limit=case['new_groups'];u.mem_write(group_count,struct.pack('<H',group_limit))
            group_ptrs.reverse()
            for n,ptr in enumerate(group_ptrs):u.mem_write(table+n*4,pack(ptr))
    def code(uc,at,size,user):
        if at==STOP:uc.emu_stop();return
        if at in services:
            family,role=services[at];sp=uc.reg_read(UC_X86_REG_ESP);actor=get32(sp+4)
            if role=='clear':family=pending['family']
            spec=SPECS[family];valid=[]
            for b,bank in enumerate(banks[family]):
                if bank<=actor<bank+4*spec['stride'] and (actor-bank)%spec['stride']==0:valid.append((b,(actor-bank)//spec['stride']))
            b,index=valid[0] if len(valid)==1 else (None,None);assert b is not None,(family,role,hex(actor))
            info=live[family]
            if role=='remove':
                assert case['entry']=='shutdown';info['removes']+=1
                assert actor==banks[family][info['bank']]+(info['removes']-1)*spec['stride']
                pending.update(family=family,actor=actor)
            else:
                info['finishes']+=1
                if case['entry']=='shutdown':assert actor==pending['actor']
                else:assert actor==banks[family][info['bank']]+(info['finishes']-1)*spec['stride']
            transcript.append([family,role,b,index]);mark(actor,role)
            # Every modeled service may alter all cursor starts. Shutdown must
            # use the final values, after every reset has returned.
            for j,name in enumerate(SPECS):
                starts[name]=(starts[name]+0x1234+j)&0xffff;u.mem_write(cells[name]['start'],struct.pack('<H',starts[name]))
            mutate(family,'remove' if role=='remove' else 'finish')
            result=case.get('result',1) if role=='remove' else 0x91a2b3c4
            uc.reg_write(UC_X86_REG_EAX,result&0xffffffff);uc.reg_write(UC_X86_REG_ECX,0xdeadbeef);uc.reg_write(UC_X86_REG_EDX,0x87654321)
            uc.reg_write(UC_X86_REG_EIP,get32(sp));uc.reg_write(UC_X86_REG_ESP,sp+4);return
        if at in helpers:
            assert case['entry']=='shutdown';reset_entries.append(helpers[at][0])
        allowed=entry<=at<entry+entry_size or case['entry']=='shutdown' and any(start<=at<start+span for start,(f,span) in helpers.items())
        assert allowed,hex(at)
    def write(uc,access,at,size,value,user):
        if STACK<=at and at+size<=SP:return
        if case['entry']=='shutdown':
            family,=[f for f in SPECS if cells[f]['cursor']==at]
            assert reset_entries==list(SPECS) and size==2 and value==starts[family]
            assert entry<=uc.reg_read(UC_X86_REG_EIP)<entry+entry_size
            cursor_writes.append([family,value]);return
        assert case['entry']=='middle' and at in group_ptrs[:group_limit] and size==4 and value==0xffffffff
        group_writes.append(group_ptrs.index(at));payload[at-DATA:at-DATA+4]=b'\xff'*4
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.emu_start(entry,0,count=5000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4
    assert [u.reg_read(r) for r in NV]==list(SEEDS) and bytes(u.mem_read(SP,len(frame)))==frame
    assert bytes(u.mem_read(DATA,0x10000))==bytes(payload)
    assert reset_entries==(list(SPECS) if case['entry']=='shutdown' else [])
    assert [f for f,v in cursor_writes]==(list(SPECS) if case['entry']=='shutdown' else [])
    assert group_writes==(list(range(group_limit)) if case['entry']=='middle' else [])
    for family in SPECS:
        info=live[family]
        assert get32(cells[family]['pointer'])==banks[family][info['bank']]
        assert bytes(u.mem_read(cells[family]['limit'],2))==struct.pack('<H',info['count'])
        assert bytes(u.mem_read(cells[family]['start'],2))==struct.pack('<H',starts[family])
        assert bytes(u.mem_read(cells[family]['cursor'],2))==struct.pack('<H',starts[family] if case['entry']=='shutdown' else cursor_initial[family])
        initial_count=case.get('count',2) if case['entry']=='shutdown' or case['entry']==family else 0
        expected_finishes=0 if not initial_count else max(1,info['count'])
        assert info['finishes']==expected_finishes,info
    assert bytes(u.mem_read(group_count,2))==struct.pack('<H',group_limit)
    assert bytes(u.mem_read(table,80))==b''.join(pack(p) for p in group_ptrs)+b'\0'*64
    return dict(calls=transcript,reset_order=reset_entries,cursor_writes=cursor_writes,group_writes=group_writes,data_sha256=digest(payload),final_counts={f:i['count'] for f,i in live.items()})

def cases():
    for entry in ['shutdown',*SPECS]:
        for count in (0,1,2,4):
            for groups in ((0,1,4) if entry=='middle' else (0,)):
                yield dict(entry=entry,count=count,groups=groups)
        stages=('remove','finish') if entry=='shutdown' else ('finish',)
        for stage,count,new_count,switch in itertools.product(stages,(1,2),(0,1,3,4),(False,True)):
            for result in ((0,1,0xffffffff) if entry=='shutdown' else (1,)):
                yield dict(entry=entry,count=count,mutate_stage=stage,new_count=new_count,switch=switch,result=result,groups=2,new_groups=4 if switch else 0,start_seed=0xfffe)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/pool_callers_2026_10_25/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out);native=PE(ORACLE);checks=[]
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False);assert a==b,dict(case=case,native=a,source=b)
        checks.append(dict(case=case,result=a))
    assert all(digest(Path(p).read_bytes())==h for p,h in build_report['input_sha256'].items())
    report=dict(scope=__doc__,cases=len(checks),by_entry={e:sum(r['case']['entry']==e for r in checks) for e in ['shutdown',*SPECS]},all_equal=True,build=build_report,cases_sha256=digest(json.dumps(checks,sort_keys=True).encode()),limits=['All lifecycle calls are modeled. Shutdown composes real reset loops; complete removal/init lifecycle and actual native capacities are not established.','Middle startup uses four readable synthetic group pointers; the existing20-pointer source prefix does not prove native table ownership/capacity or matching acceptance.'])
    (out/'cases.json').write_text(json.dumps(checks,indent=2)+'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['cases','by_entry','all_equal']}))

if __name__=='__main__':main()
