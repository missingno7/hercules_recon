"""Finite unpatched original/source spatial predicate comparisons in Unicorn.

The actor-bounds service is modeled explicitly. No DLL is loaded into an OS
process, no original host/game starts, and no instruction bytes are modified.
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
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,
 UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

SOURCE=ROOT/'src/eng1/behavioral/actor_spatial_predicates.cpp'
ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
SPANS={0x2a710:144,0x2aa30:128}
DATA=0x61000000;ACTOR=DATA+0x100;AREA=DATA+0x400
STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
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
    fixture.write_text('/* Intercepted synthetic service/storage, never production code. */\nunsigned short g_pipeline_word_71376;\nint __cdecl actor_bounds_2a880(void *a,void *b,unsigned long c) { for (;;) {} }\n')
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


def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.image_base
    extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,extent);u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(DATA,0x1000);u.mem_map(STACK,0x10000)
    u.mem_write(DATA,bytes((i*13+31)&255 for i in range(0x1000)))
    entry=base+case['entry'] if original else symbols[f'_eng1_native_code_target_00{case["entry"]:05x}']
    service=base+0x2a880 if original else symbols['_actor_bounds_2a880']
    timer=base+0x71376 if original else symbols['_g_pipeline_word_71376']
    u.mem_write(timer,struct.pack('<H',case.get('timer',0)))
    u.mem_write(ACTOR+0xc,struct.pack('<hh',case.get('x',0),case.get('y',0)))
    u.mem_write(ACTOR+0x1f,bytes([case.get('flags',1)]))
    u.mem_write(AREA,struct.pack('<6h',*case.get('area',[-10,-20,-30,10,20,30])))
    expected_data=bytearray(u.mem_read(DATA,0x1000))
    args=[AREA,ACTOR] if case['entry']==0x2a710 else [0 if case.get('null_actor') else ACTOR,case.get('xm',0),case.get('ym',0)]
    frame=pack(STOP)+b''.join(pack(n) for n in args)+bytes([0x93])*24
    u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
    for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
    transcript=[];writes=[]
    def get32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        if address==service:
            assert case['entry']==0x2a710
            sp=uc.reg_read(UC_X86_REG_ESP);a,b,mode=[get32(sp+4+4*n) for n in range(3)]
            assert a==ACTOR and mode==0 and STACK<=b and b+12<=ESP
            transcript.append([a,'stack_bounds',mode])
            if case.get('result',1):uc.mem_write(b,struct.pack('<6h',*case.get('bounds',[-10,-20,-30,10,20,30])))
            if 'mutate_area' in case:
                payload=struct.pack('<6h',*case['mutate_area']);uc.mem_write(AREA,payload)
                expected_data[AREA-DATA:AREA-DATA+12]=payload
            uc.reg_write(UC_X86_REG_EAX,case.get('result',1)&0xffffffff)
            uc.reg_write(UC_X86_REG_ECX,0xaabbccdd);uc.reg_write(UC_X86_REG_EDX,0x99887766)
            uc.reg_write(UC_X86_REG_EIP,get32(sp));uc.reg_write(UC_X86_REG_ESP,sp+4)
            return
        assert entry<=address<entry+SPANS[case['entry']],hex(address)
    def write(uc,access,address,size,value,user):
        assert STACK<=address and address+size<=ESP,hex(address)
        writes.append([address,size])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.emu_start(entry,0,count=1000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP
    assert u.reg_read(UC_X86_REG_ESP)==ESP+4
    assert [u.reg_read(r) for r in NV]==list(SEEDS)
    assert bytes(u.mem_read(DATA,0x1000))==bytes(expected_data)
    assert bytes(u.mem_read(ESP,len(frame)))==frame
    assert struct.unpack('<H',u.mem_read(timer,2))[0]==case.get('timer',0)
    return dict(eax=u.reg_read(UC_X86_REG_EAX),calls=transcript,data_sha256=digest(expected_data))


def cases():
    area=[-10,-20,-30,10,20,30]
    for axis in range(3):
        for lo,hi in [(-32768,-32768),(-31,-30),(-21,-20),(-11,-10),(-10,-10),(-1,1),(10,10),(10,11),(20,21),(30,31),(32767,32767),(20,-20)]:
            b=area.copy();b[axis]=lo;b[axis+3]=hi
            for result in (0,1,0x80000000):yield dict(entry=0x2a710,area=area,bounds=b,result=result)
    rng=random.Random(0x2a710)
    for n in range(120):
        a=[rng.randrange(-32768,32768) for _ in range(6)]
        b=[rng.randrange(-32768,32768) for _ in range(6)]
        yield dict(entry=0x2a710,area=a,bounds=b,result=1)
    for result,mutation in itertools.product((0,1,2),([100]*6,[-32768]*6,[32767]*6)):
        yield dict(entry=0x2a710,area=area,bounds=area,result=result,mutate_area=mutation)
    coordinates=[(-32768,0),(-201,0),(-200,0),(-1,0),(0,0),(319,239),(320,240),(32767,32767),(0,-201),(0,-200),(0,-1),(0,239),(0,240),(200,200)]
    for (x,y),(xm,ym),flags,timer in itertools.product(coordinates,[(0,0),(200,200),(0xffff,1),(0x100c8,0x123400c8),(0xffffffff,0xffffffff)],(0,1,3,0xfd),(0,1,0xffff)):
        yield dict(entry=0x2aa30,x=x,y=y,xm=xm,ym=ym,flags=flags,timer=timer)
    for timer in (1,0xffff):yield dict(entry=0x2aa30,timer=timer,null_actor=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/spatial_predicates_2026_10_25/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out);native=PE(ORACLE)
    count=0;by_entry={};checks=[]
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False)
        assert a==b,dict(case=case,native=a,source=b)
        checks.append(dict(case=case,result=a));count+=1;by_entry[hex(case['entry'])]=by_entry.get(hex(case['entry']),0)+1
    assert all(digest(Path(p).read_bytes())==h for p,h in build_report['input_sha256'].items())
    report=dict(scope='Finite isolated spatial predicates, modeled actor_bounds service, exact argument transcript, guards/inputs/stack/callee-saved state; not actual extent behavior or game execution.',case_count=count,by_entry=by_entry,all_equal=True,build=build_report,cases_sha256=digest(json.dumps(checks,sort_keys=True).encode()),limitations=['2a710 original and source invoke a modeled2a880 producer; no claim of that producer or resource closure.','2aa30 uses synthetic timer storage; no production owner acceptance follows from fixture storage.'])
    (out/'cases.json').write_text(json.dumps(checks,indent=2)+'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=count,by_entry=by_entry,all_equal=True)))


if __name__=='__main__':main()
