"""Guarded alternate-pool pressure helper with a modeled release service.

Original and freshly linked source instructions run only inside Unicorn. No OS
module load, instruction patching, native allocation-capacity or lifecycle claim.
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

SOURCE=ROOT/'src/eng1/behavioral/alternate_pool_pressure.c'
ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
BANK=0x61001000;STACK=0x63000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
SEEDS=(0x12345678,0x23456789,0x3456789a,0x456789ab)
pack=lambda n:struct.pack('<I',n&0xffffffff)


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins={str(p):digest(p.read_bytes()) for p in [SOURCE,ORACLE,Path(__file__).resolve(),runtime,ROOT/'toolchains/manifest.json']}
    out.mkdir(parents=True,exist_ok=True)
    fixture=out/'services.c'
    fixture.write_text('/* Synthetic fixture only; release entry is intercepted. */\nvoid *g_alternate_actor_pool;\nunsigned short g_configuration_word_713ba;\nint __cdecl release_actor(void *actor) { for (;;) {} }\n')
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
    kinds=case['kinds'];kind=case['kind']
    if 0 in kinds:return [],list(range(kinds.index(0)+1))
    chosen=kinds.index(kind) if kind in kinds else None
    return ([] if chosen is None else [chosen]),list(range(len(kinds)))+list(range(len(kinds) if chosen is None else chosen+1))


def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.image_base
    extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,extent);u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    kinds=case['kinds'];count=len(kinds);bank_size=max(0x1000,((count*0x94+0x200+0xfff)&~0xfff))
    u.mem_map(BANK-0x1000,bank_size+0x1000);u.mem_map(STACK,0x10000)
    payload=bytearray(bytes(range(256))*(bank_size//256))
    for n,kind in enumerate(kinds):struct.pack_into('<H',payload,n*0x94+0x2e,kind)
    u.mem_write(BANK,bytes(payload));u.mem_write(BANK-0x1000,b'\xab'*0x1000)
    entry=base+0x1b1d0 if original else symbols['_eng1_native_code_target_001b1d0']
    service=base+0x27e20 if original else symbols['_release_actor']
    pointer=base+0x734e0 if original else symbols['_g_alternate_actor_pool']
    limit=base+0x713ba if original else symbols['_g_configuration_word_713ba']
    # A null zero-count pool must not be dereferenced.
    pool=0 if not count and case.get('null') else BANK
    u.mem_write(pointer,pack(pool));u.mem_write(limit,struct.pack('<H',count))
    frame=pack(STOP)+pack(case['kind'])+b'\x93'*24
    u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
    for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
    reads=[];global_reads=[];calls=[];write_count=0
    want_calls,want_reads=expected(case)
    def get32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        if address==service:
            sp=uc.reg_read(UC_X86_REG_ESP);actor=get32(sp+4)
            assert actor>=BANK and (actor-BANK)%0x94==0
            index=(actor-BANK)//0x94;calls.append(index)
            assert calls==want_calls
            if case.get('mutate'):
                # Model arbitrary release side effects, including changed globals.
                start=index*0x94+0x20;payload[start:start+16]=b'\x5c'*16
                uc.mem_write(BANK+start,b'\x5c'*16)
                uc.mem_write(pointer,pack(BANK+0x94));uc.mem_write(limit,b'\0\0')
            uc.reg_write(UC_X86_REG_EAX,case.get('result',0)&0xffffffff)
            uc.reg_write(UC_X86_REG_ECX,0x8796a5b4);uc.reg_write(UC_X86_REG_EDX,0xdeadbeef)
            uc.reg_write(UC_X86_REG_EIP,get32(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
            return
        assert entry<=address<entry+96,hex(address)
    def read(uc,access,address,size,value,user):
        if address==pointer:
            assert size==4;global_reads.append('pool');return
        if address==limit:
            assert size==2;global_reads.append('count');return
        if STACK<=address and address+size<=STACK+0x10000:return
        assert address>=BANK+0x2e and size==2 and (address-BANK-0x2e)%0x94==0,hex(address)
        index=(address-BANK-0x2e)//0x94
        assert 0<=index<count
        reads.append(index)
    def write(uc,access,address,size,value,user):
        nonlocal write_count
        assert STACK<=address and address+size<=ESP,hex(address)
        write_count+=1
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.emu_start(entry,0,count=max(1000,24*count+1000))
    assert u.reg_read(UC_X86_REG_EIP)==STOP
    assert u.reg_read(UC_X86_REG_ESP)==ESP+4
    assert [u.reg_read(r) for r in NV]==list(SEEDS)
    assert bytes(u.mem_read(ESP,len(frame)))==frame
    assert bytes(u.mem_read(BANK,bank_size))==bytes(payload)
    assert bytes(u.mem_read(BANK-0x1000,0x1000))==b'\xab'*0x1000
    assert calls==want_calls and reads==want_reads
    assert global_reads==['pool','count']
    mutated=bool(calls and case.get('mutate'))
    assert get32(pointer)==(BANK+0x94 if mutated else pool)
    assert bytes(u.mem_read(limit,2))==struct.pack('<H',0 if mutated else count)
    # Void helper: incidental EAX after the no-call or release paths is excluded.
    return dict(calls=calls,reads_sha256=digest(json.dumps(reads).encode()),read_count=len(reads),global_reads=global_reads,data_sha256=digest(payload),stack_writes=write_count)


def cases():
    for kind in (0,1,0x202f,0xffff,0x10000,0x1202f,0x7fffffff,0x80000000,0xffffffff):
        yield dict(kinds=[],kind=kind,null=True)
    for size in (1,2,3,8):
        for requested in (0,1,0x202f,0xffff,0x10000,0x1202f,0xffffffff):
            for values in ([1]*size,[0x202f]*size,[0xffff]*size):
                yield dict(kinds=values,kind=requested)
            for at in range(size):
                values=[0x202f]*size;values[at]=0
                yield dict(kinds=values,kind=requested)
                values=[1]*size;values[at]=0x202f
                yield dict(kinds=values,kind=requested)
    for values,result in itertools.product(([0x202f],[1,0x202f,0x202f],[0x202f,0,0x202f]),(0,1,0xffffffff)):
        yield dict(kinds=values,kind=0x202f,mutate=True,result=result)
    rng=random.Random(0x1b1d0)
    for _ in range(80):
        values=[rng.choice((0,1,0x202f,0xffff)) for _ in range(rng.randrange(1,17))]
        yield dict(kinds=values,kind=rng.choice((0,1,0x202f,0xffff,0x1202f,0xffffffff)))
    # Full WORD count is readable only in this synthetic bank; native capacity is unknown.
    yield dict(kinds=[0]+[1]*65534,kind=1)
    yield dict(kinds=[1]*65534+[0],kind=1)
    yield dict(kinds=[1]*65534+[0x202f],kind=0x202f,mutate=True,result=0xffffffff)
    yield dict(kinds=[1]*65535,kind=0x1202f)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/pool_pressure_2026_10_25/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out);native=PE(ORACLE);checks=[]
    for number,case in enumerate(cases()):
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False)
        assert a==b,dict(case=case,native=a,source=b)
        checks.append(dict(case=case,result=a))
    assert all(digest(Path(p).read_bytes())==h for p,h in build_report['input_sha256'].items())
    # Cases stay ignored; the report carries a compact digest of all inputs/results.
    serialized=json.dumps(checks,sort_keys=True).encode()
    report=dict(scope=__doc__,cases=len(checks),all_equal=True,build=build_report,cases_sha256=digest(serialized),kind_reads=sum(r['result']['read_count'] for r in checks),release_cases=sum(bool(r['result']['calls']) for r in checks),maximum_count=max(len(r['case']['kinds']) for r in checks),limits=['Release service and storage are synthetic. No complete callee, native capacity, allocator reservation or source-graph closure follows.','All five observed callers discard the result before their next cdecl call. This fixture compares void behavior and excludes incidental EAX.'])
    (out/'cases.json').write_bytes(serialized+b'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['cases','all_equal','kind_reads','release_cases','maximum_count']}))


if __name__=='__main__':main()
