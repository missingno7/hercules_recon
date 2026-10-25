"""Finite original/source query/steering comparisons; Unicorn only, no OS DLL load.

The bounds producer is a modeled service. Worklist fixtures establish only the
tested slots, never a production capacity or loader-zero matching acceptance.
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
from coff import COFF
from match import DEFAULT_TOOLCHAIN,compile_source,digest,verify_toolchain
from pe import PE
from title_behavioral import context_files
from title_link import read_map

sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,
 UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
SOURCES=[ROOT/'src/eng1/behavioral/nearest_actor_2aea0.c',ROOT/'src/eng1/behavioral/target_actor_motion.c',ROOT/'src/eng1/behavioral/actor_pipeline_storage.c']
SPANS={0x2aea0:224,0x2b5e0:960}
DATA=0x61000000;P=DATA+0x100;T=DATA+0x500
ROWS=[DATA+0x1000+i*0x100 for i in range(8)]
STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
SEEDS=[0x12345678,0x23456789,0x3456789a,0x456789ab]
pack=lambda x:struct.pack('<I',x&0xffffffff)


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    inputs={ORACLE,Path(__file__).resolve(),runtime,ROOT/'toolchains/manifest.json'}
    for s in SOURCES:inputs.update(context_files(s))
    pins={str(p):digest(p.read_bytes()) for p in inputs}
    out.mkdir(parents=True,exist_ok=True)
    service=out/'services.c';service.write_text('/* Intercepted fixture only. */\nint __cdecl actor_bounds_2a880(void*a,void*b,unsigned long m){for(;;){}}\n')
    objects=[];commands=[];sizes={}
    for source in [*SOURCES,service]:
        obj,command=compile_source(source,['/O2']);objects.append(obj);commands.append(command)
        if source in SOURCES[:2]:
            entry=0x2aea0 if source==SOURCES[0] else 0x2b5e0
            sizes[entry]=COFF(obj).function(f'_eng1_native_code_target_00{entry:05x}')['size']
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),*map(str,objects)]
    env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    result=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,commands=[*commands,command],objects=[dict(source=str(s),object=str(o),object_sha256=digest(o.read_bytes())) for s,o in zip([*SOURCES,service],objects)],compiled_sizes=sizes,image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)


def execute(pe,symbols,sizes,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.image_base;extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,extent);u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(DATA,0x4000);u.mem_map(STACK,0x10000)
    u.mem_write(DATA,bytes((i*13+31)&255 for i in range(0x4000)))
    entry=base+case['entry'] if original else symbols[f'_eng1_native_code_target_00{case["entry"]:05x}']
    size=SPANS[case['entry']] if original else sizes[case['entry']]
    service=base+0x2a880 if original else symbols['_actor_bounds_2a880']
    worklist=base+0x71580 if original else symbols['_g_actor_pipeline_storage_71440']+0x140
    def w32(at,x):u.mem_write(at,pack(x))
    def position(at,values):u.mem_write(at,b''.join(pack(v) for v in values))
    position(P,case.get('point',[0x100000,0x200000,0x300000]))
    position(T,case.get('target',[0x400000,0x500000,0x600000]))
    target=0 if case.get('null_target') else P if case.get('same_target') else T
    if case['entry']==0x2aea0:
        records=case.get('rows',[]);w32(worklist,case.get('count',len(records)))
        for at,values in zip(ROWS,records):position(at,values)
        for n in range(len(records)):w32(worklist+4+4*n,ROWS[n])
        args=[P,case.get('depth',100),case.get('planar',100)]
    else:
        u.mem_write(P+0x94,bytes(case.get('directions',[0,0,0])))
        values=case.get('axis',[0x10000,0x50000,0x1000,0x2000])
        for offset in [0xa8,0xb8,0xc8]:u.mem_write(P+offset,b''.join(pack(v) for v in values))
        args=[P,target,case.get('flags',0)]
    frame=pack(STOP)+b''.join(pack(v) for v in args)+bytes([0x93])*24
    u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
    for r,v in zip(NV,SEEDS):u.reg_write(r,v)
    calls=[]
    def get32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def code(uc,address,n,user):
        if address==STOP:uc.emu_stop();return
        if address==service:
            assert case['entry']==0x2b5e0
            sp=uc.reg_read(UC_X86_REG_ESP);actor,out,mode=[get32(sp+4+i*4) for i in range(3)]
            assert actor==target and mode==0 and STACK<=out and out+12<=ESP
            calls.append([actor,'bounds',mode])
            if case.get('bounds_result',1):uc.mem_write(out,struct.pack('<6h',*case.get('bounds',[-20,-30,-40,21,32,43])))
            if case.get('mutate'):
                position(P,[0x7fffffff,0x80000000,0xffffffff]);position(target,[0x12345678,0x87654321,0xabcdef01])
                uc.mem_write(P+0x94,b'\xff\x10\x01')
            uc.reg_write(UC_X86_REG_EAX,case.get('bounds_result',1)&0xffffffff)
            uc.reg_write(UC_X86_REG_ECX,0xabcdef12);uc.reg_write(UC_X86_REG_EDX,0x3456789a)
            uc.reg_write(UC_X86_REG_EIP,get32(sp));uc.reg_write(UC_X86_REG_ESP,sp+4);return
        assert entry<=address<entry+size,hex(address)
    allowed={P+n:(4 if n in [0,4,8,0xa8,0xb8,0xc8] else 1) for n in [0,4,8,0xa8,0xb8,0xc8,0x94,0x95,0x96]}
    def write(uc,access,address,n,value,user):
        if STACK<=address and address+n<=ESP:return
        # The nonmatching query may reuse cdecl argument-copy slots. Its ABI
        # guarantees return address/ESP, callee-saved values and caller guards,
        # not retention of these copies. This never qualifies as RAW equality.
        if case['entry']==0x2aea0 and ESP+4<=address and address+n<=ESP+4+4*len(args):return
        assert case['entry']==0x2b5e0 and allowed.get(address)==n,hex(address)
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.emu_start(entry,0,count=10000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4
    assert [u.reg_read(r) for r in NV]==SEEDS
    if case['entry']==0x2aea0:
        assert bytes(u.mem_read(ESP,4))==frame[:4]
        guard=4+4*len(args)
        assert bytes(u.mem_read(ESP+guard,len(frame)-guard))==frame[guard:]
    else:assert bytes(u.mem_read(ESP,len(frame)))==frame
    result=u.reg_read(UC_X86_REG_EAX)
    return dict(result=result if case['entry']==0x2aea0 else result&255,calls=calls,data_sha256=digest(u.mem_read(DATA,0x4000)),worklist_sha256=digest(u.mem_read(worklist,40)))


def cases():
    for flags,null,result,directions in itertools.product(range(256),(False,True),(0,1),([0,0,0],[255,16,1])):
        if null and result:continue
        yield dict(entry=0x2b5e0,flags=flags,null_target=null,bounds_result=result,directions=directions)
    for flags,axis in itertools.product((1,2,4,7,8,16,32,56,64,128,192,255),([0,0,1,1],[-1,-1,0x7fffffff,1],[0x7fffffff,0x80000000,1,0xffffffff],[0x80000000,0xffffffff,0xffffffff,0x80000000])):
        yield dict(entry=0x2b5e0,flags=flags,axis=axis,directions=[255,0,1])
    for result,flags,alias,mutate in itertools.product((0,1,0xffffffff),(0,7,255),(False,True),(False,True)):
        yield dict(entry=0x2b5e0,flags=flags,bounds_result=result,same_target=alias,mutate=mutate,bounds=[-32768,32767,-1,32767,-32768,0])
    for flags,null in itertools.product((0x12340000,0x12340001,0x123400ff),(False,True)):
        yield dict(entry=0x2b5e0,flags=flags,null_target=null)
    for count in (0,0xffffffff,0x80000000):yield dict(entry=0x2aea0,count=count)
    coords=[-32768,-32767,-101,-100,-1,0,1,99,100,101,32767]
    for x,y,z,depth,planar in itertools.product(coords,(-32768,0,32767),(-32768,0,32767),(0,1,100),(0,1,100)):
        yield dict(entry=0x2aea0,point=[0,0,0],rows=[[x<<16,y<<16,z<<16],[0,0,0]],depth=depth,planar=planar)
    for values in ([[0,0,0],[0,0,0]],[[1<<16,0,0],[-1<<16,0,0]],[[10<<16,0,0],[1<<16,0,0],[1<<16,0,0]]):
        yield dict(entry=0x2aea0,point=[0,0,0],rows=values,depth=100,planar=100)
    yield dict(entry=0x2aea0,point=[0,0,0],rows=[[n<<16,0,0] for n in range(8,0,-1)],depth=100,planar=100)
    rng=random.Random(0x2aea0)
    for n in range(150):
        yield dict(entry=0x2aea0,point=[rng.getrandbits(32) for i in range(3)],rows=[[rng.getrandbits(32) for i in range(3)] for j in range(n%8)],depth=rng.choice([-1,0,0x7fffffff,32768]),planar=rng.choice([-1,0,0x7fffffff,32768]))


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/spatial_queries_2026_10_25/differential');args=parser.parse_args()
    out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    built,candidate,symbols=build(out);native=PE(ORACLE);count=0;by_entry={};results=[]
    for case in cases():
        a=execute(native,symbols,built['compiled_sizes'],case,True);b=execute(candidate,symbols,built['compiled_sizes'],case,False)
        assert a==b,dict(case=case,native=a,source=b)
        count+=1;by_entry[hex(case['entry'])]=by_entry.get(hex(case['entry']),0)+1;results.append(dict(case=case,result=a))
    assert all(digest(Path(p).read_bytes())==h for p,h in built['input_sha256'].items())
    report=dict(scope='Finite unpatched original/source query and steering cases, modeled bounds producer and synthetic0..8-entry worklists; no game execution or list-capacity acceptance.',case_count=count,by_entry=by_entry,all_equal=True,build=built,cases_sha256=digest(json.dumps(results,sort_keys=True).encode()),limitations=['Bounds behavior modeled; no resource chain or whole-game claim.','Worklist prefix and backing aggregate do not establish complete native storage ownership or matching acceptance for2aea0.','Query compiler may reuse incoming cdecl argument-copy slots; their values are excluded from equivalence. Return address,ESP,callee-saved values,caller guards and complete heap/worklist effects are checked. Inputs do not alias those copies.'])
    (out/'cases.json').write_text(json.dumps(results,indent=2)+'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=count,by_entry=by_entry,all_equal=True)))


if __name__=='__main__':main()
