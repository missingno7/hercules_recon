"""Finite original/source first-fit/wrap allocator comparisons in Unicorn.

Both allocators are leaf routines. All storage and readable capacities are
synthetic; no original OS module load, instruction patch or game startup occurs.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,digest,verify_toolchain
from pe import PE
from title_link import read_map

sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,
 UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

SOURCE=ROOT/'src/eng1/behavioral/actor_pool_allocators.c'
ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
BANK=0x61001000;STACK=0x65000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
SEEDS=(0x12345678,0x23456789,0x3456789a,0x456789ab)
SPECS={
 'normal':dict(entry=0x26f70,size=336,stride=0x134,pointer=0x72860,roots=[0x71308,0x713b4,0x713a8,0x713c4,0x71378]),
 'alternate':dict(entry=0x27b00,size=320,stride=0x94,pointer=0x734e0,roots=[0x71374,0x713ba,0x713b6,0x71430,0x7138c]),
}
pack=lambda n:struct.pack('<I',n&0xffffffff)


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins={str(p):digest(p.read_bytes()) for p in [SOURCE,ORACLE,Path(__file__).resolve(),runtime,ROOT/'toolchains/manifest.json']}
    out.mkdir(parents=True,exist_ok=True)
    fixture=out/'storage.c'
    fixture.write_text('/* Complete scalar fixture objects only; never production owners. */\nvoid *g_normal_actor_pool;\nvoid *g_alternate_actor_pool;\n'+''.join('unsigned short g_configuration_word_%x;\n'%r for spec in SPECS.values() for r in spec['roots']))
    obj,command=compile_source(SOURCE,['/O2']);fix,fixcommand=compile_source(fixture,['/O2'])
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    link=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),str(obj),str(fix)]
    env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    ran=subprocess.run(link,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr)
    assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,commands=[command,fixcommand,link],object=str(obj),object_sha256=digest(obj.read_bytes()),image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)


def expected(case):
    cp,limit,split,ca,start=case['config']
    cursor,end,wrap=(cp,limit,split) if case['flags']&0x8000 else (ca,split,start)
    order=list(range(cursor,end))+list(range(wrap,cursor))
    free=set(case['free'])
    hit=next((n for n in order if n in free),None)
    return hit,order if hit is None else order[:order.index(hit)+1]


def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.image_base
    extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,extent);u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    spec=SPECS[case['pool']];stride=spec['stride'];count=case['readable_slots']
    bank_size=max(0x1000,(count*stride+0x200+0xfff)&~0xfff)
    u.mem_map(BANK-0x1000,bank_size+0x1000);u.mem_map(STACK,0x10000)
    payload=bytearray(bytes(range(256))*(bank_size//256))
    free=set(case['free'])
    for n in range(count):struct.pack_into('<H',payload,n*stride+0x2e,0 if n in free else case.get('occupied_kind',0x202f))
    u.mem_write(BANK,bytes(payload));u.mem_write(BANK-0x1000,b'\xab'*0x1000)
    entry=base+spec['entry'] if original else symbols['_allocate_'+case['pool']+'_actor']
    globals_by_address={};initial={};active=[]
    for family,other in SPECS.items():
        pointer=base+other['pointer'] if original else symbols['_g_'+family+'_actor_pool']
        pointer_value=BANK if family==case['pool'] and not case.get('null') else 0
        initial[pointer]=pack(pointer_value);globals_by_address[pointer]=(family+'.pool',4)
        for n,root in enumerate(other['roots']):
            at=base+root if original else symbols['_g_configuration_word_%x'%root]
            value=case['config'][n] if family==case['pool'] else 0xc123+n
            initial[at]=struct.pack('<H',value);globals_by_address[at]=(family+'.%x'%root,2)
            if family==case['pool']:active.append(at)
    for at,data in initial.items():u.mem_write(at,data)
    frame=pack(STOP)+pack(case['flags'])+b'\x93'*24
    reads=[];global_reads=[];writes=[];turns=[]
    hit,want_reads=expected(case);want_cursor=0 if case['flags']&0x8000 else 3
    assert all(0<=n<count for n in want_reads)
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        assert entry<=address<entry+spec['size'],hex(address)
    def read(uc,access,address,size,value,user):
        if address in globals_by_address:
            name,width=globals_by_address[address];assert size==width and name.startswith(case['pool']+'.')
            global_reads.append(name);return
        if STACK<=address and address+size<=STACK+0x10000:return
        assert address>=BANK+0x2e and size==2 and (address-BANK-0x2e)%stride==0,hex(address)
        n=(address-BANK-0x2e)//stride;assert 0<=n<count;reads.append(n)
    def write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=ESP:return
        assert hit is not None and address==active[want_cursor] and size==2 and value==hit,(hex(address),size,value)
        writes.append([globals_by_address[address][0],size,value])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    for _ in range(case.get('repeat',1)):
        reads.clear();global_reads.clear();writes.clear()
        u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
        for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
        u.emu_start(entry,0,count=max(1000,24*len(want_reads)+1000))
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4
        assert [u.reg_read(r) for r in NV]==list(SEEDS)
        eax=u.reg_read(UC_X86_REG_EAX);assert eax==(0 if hit is None else BANK+hit*stride)
        assert reads==want_reads and len(writes)==(0 if hit is None else 1)
        for at,data in initial.items():
            expected_data=struct.pack('<H',hit) if hit is not None and at==active[want_cursor] else data
            assert bytes(u.mem_read(at,len(data)))==expected_data
        assert bytes(u.mem_read(ESP,len(frame)))==frame
        assert bytes(u.mem_read(BANK,bank_size))==bytes(payload)
        assert bytes(u.mem_read(BANK-0x1000,0x1000))==b'\xab'*0x1000
        turns.append(dict(result_index=hit,reads_sha256=digest(json.dumps(reads).encode()),read_count=len(reads),global_reads=list(global_reads),cursor_writes=list(writes),data_sha256=digest(payload)))
        # A successful return does not reserve or advance: another call returns
        # the same slot, but now begins directly at its saved cursor.
        if hit is not None:
            case_config=list(case['config']);case_config[want_cursor]=hit
            repeat_case=dict(case,config=case_config);_,want_reads=expected(repeat_case)
            initial[active[want_cursor]]=struct.pack('<H',hit)
    return turns


def cases():
    flags=(0,0x7fff,0x8000,0xffff,0x12340000,0x12348000)
    for pool in SPECS:
        for cp,ca,mask,flag in itertools.product((2,3,4),(0,1,2),range(16),flags):
            yield dict(pool=pool,config=[cp,4,2,ca,0],free=[n for n in range(4) if mask&(1<<n)],readable_slots=4,flags=flag)
        for flag in flags:yield dict(pool=pool,config=[0]*5,free=[],readable_slots=0,flags=flag,null=True)
        for flag in (0,0x8000):
            yield dict(pool=pool,config=[3,4,2,1,0],free=[0,2],readable_slots=4,flags=flag,repeat=2)
        rng=random.Random(0x26f70 if pool=='normal' else 0x27b00)
        for _ in range(80):
            n=rng.randrange(1,17);start,split,limit=sorted(rng.randrange(n+1) for _ in range(3))
            yield dict(pool=pool,config=[rng.randrange(split,limit+1),limit,split,rng.randrange(start,split+1),start],free=[i for i in range(n) if rng.randrange(4)==0],readable_slots=n,flags=rng.choice(flags),occupied_kind=rng.choice((1,0x8000,0xffff)))
        # Readable synthetic capacity only; actual native pool capacities unknown.
        yield dict(pool=pool,config=[0,65535,0,0,0],free=[65534],readable_slots=65535,flags=0x8000)
        yield dict(pool=pool,config=[32767,65535,0,0,0],free=[],readable_slots=65535,flags=0x8000)
        yield dict(pool=pool,config=[65535,65535,0,0,0],free=[65534],readable_slots=65535,flags=0x8000)
        # Preserve boundary behavior even for non-monotonic synthetic parameters.
        for config in ([1,0,3,2,0],[0,1,3,3,2],[3,2,1,2,0]):
            for flag in (0,0x8000):yield dict(pool=pool,config=config,free=[0,2],readable_slots=4,flags=flag)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/allocator_matching_2026_10_25/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out);native=PE(ORACLE);checks=[]
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False)
        assert a==b,dict(case=case,native=a,source=b)
        checks.append(dict(case=case,result=a))
    assert all(digest(Path(p).read_bytes())==h for p,h in build_report['input_sha256'].items())
    serialized=json.dumps(checks,sort_keys=True).encode()
    report=dict(scope=__doc__,cases=len(checks),all_equal=True,build=build_report,cases_sha256=digest(serialized),kind_reads=sum(t['read_count'] for r in checks for t in r['result']),by_pool={p:sum(r['case']['pool']==p for r in checks) for p in SPECS},limits=['Readable capacities and globals are synthetic; independent actual owner proof is separate. No native capacity or complete factory lifecycle follows.','Neither routine reserves a returned slot; cursor remains at the found index. Repeated-call controls preserve this behavior.'])
    (out/'cases.json').write_bytes(serialized+b'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['cases','all_equal','kind_reads','by_pool']}))


if __name__=='__main__':main()
