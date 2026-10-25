"""Middle first-fit allocator and factory through real middle/base initializers.

Unpatched native/source instructions run in Unicorn with synthetic readable
pools and descriptor records. Repeated factory calls reserve slots by kind;
allocator-only calls do not. Optional root interventions and descriptor/actor
aliases test reload order. No original OS module or game starts.
"""
import argparse
from collections import Counter
import json,os,struct,subprocess,sys
from pathlib import Path
from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,compile_inputs,digest
from masked import masked_compare
from pe import PE
from title_link import read_map
from coordinate_factories_differential import build as build_factories,ORACLE,NV,SEEDS,pack,STACK,SP,STOP
from actor_pool_allocators_differential import cases as allocator_cases,expected
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
SOURCE='src/eng1/middle_actor_factory.cpp'
REAL={'_allocate_middle_actor':(0x275d0,336),'_spawn_middle_277e0':(0x277e0,128),'_initialize_middle_actor':(0x27a50,176),'_initialize_alternate_actor':(0x27fe0,272)}
ROOTS=[0x713ac,0x71432,0x713a0,0x713b8,0x713aa];POOL=0x61001000;A=0x63000000;B=0x63100000;STRIDE=0xec


def build(out):
    out.mkdir(parents=True,exist_ok=True);prior,_,_=build_factories(out/'helpers');pins=dict(prior['input_sha256'])
    pins[str(Path(__file__).resolve())]=digest(Path(__file__).read_bytes());pins[str(ROOT/'scripts/actor_pool_allocators_differential.py')]=digest((ROOT/'scripts/actor_pool_allocators_differential.py').read_bytes())
    pins.update({str(p):h for p,h in compile_inputs(ROOT/SOURCE,['/O2'],DEFAULT_TOOLCHAIN).items()})
    obj,command=compile_source(ROOT/SOURCE,['/O2']);checks={}
    for name,rva,size in [('_allocate_middle_actor',0x275d0,336),('_spawn_middle_277e0',0x277e0,128)]:
        checks[name]=masked_compare(obj,name,ORACLE,rva,size);assert checks[name]['masked_equal'],checks[name]
    fixture=out/'middle_cells.c';fixture.write_text('/* Complete synthetic fixture objects only; never production owners. */\nvoid *g_middle_actor_pool;\nunsigned long g_configuration_dword_71390;\n'+''.join('unsigned short g_configuration_word_%x;\n'%r for r in ROOTS))
    pins[str(fixture)]=digest(fixture.read_bytes());cells,cellcmd=compile_source(fixture,['/O2']);objects=[p for p in prior['commands'][-1] if p.lower().endswith('.obj')]+[str(obj),str(cells)]
    image=out/'fixture.dll';mapfile=out/'fixture.map';link=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),'/EXPORT:allocate_middle_actor','/EXPORT:spawn_middle_277e0',*objects]
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');ran=subprocess.run(link,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,helper_build=prior,commands=[command,cellcmd,link],checks=checks,image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)


def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    count=case['readable_slots'];bank_size=max(0x1000,(count*STRIDE+0x200+0xfff)&~0xfff);u.mem_map(POOL-0x1000,bank_size+0x1000);u.mem_map(A,0x200000);u.mem_map(STACK,0x10000)
    payload=bytearray(bytes(range(256))*(bank_size//256));free=set(case['free'])
    for n in range(count):struct.pack_into('<H',payload,n*STRIDE+0x2e,0 if n in free else case.get('occupied_kind',0x202f))
    u.mem_write(POOL,bytes(payload));u.mem_write(POOL-0x1000,b'\xab'*0x1000)
    real={name:pe.image_base+rva if original else symbols[name] for name,(rva,size) in REAL.items()};entry='_allocate_middle_actor' if case['entry']=='allocator' else '_spawn_middle_277e0'
    cells={};values={};configuration=list(case['config'])
    for j,rva in enumerate(ROOTS):
        at=pe.image_base+rva if original else symbols['_g_configuration_word_%x'%rva];cells[at]=('word.%x'%rva,2);values[at]=struct.pack('<H',configuration[j])
    pointer=pe.image_base+0x73494 if original else symbols['_g_middle_actor_pool'];cells[pointer]=('pool',4);values[pointer]=pack(0 if case.get('null') else POOL)
    root=pe.image_base+0x71390 if original else symbols['_g_configuration_dword_71390'];cells[root]=('descriptors',4);values[root]=pack(A)
    for at,raw in values.items():u.mem_write(at,raw)
    kind=case['flags'];index=kind&0xfff
    records={A+index*8:struct.pack('<I',0x31415926)+bytes([0xa5,0x5a,0x6d,0xff]),B+index*8:struct.pack('<I',0x27182818)+bytes([0x5a,0xa5,0x4c,0xff])}
    for at,raw in records.items():u.mem_write(at,raw)
    reads=[];globals_read=[];writes=[];interventions=[];entered=Counter();turns=[];hit=None;slot=None
    def r32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        for name,at in real.items():
            if address==at:entered[name]+=1
        assert any(at<=address and address+size<=at+REAL[name][1] for name,at in real.items()),hex(address)
    def read(uc,access,address,size,value,user):
        if STACK<=address and address+size<=STACK+0x10000:return
        if address in cells:
            name,width=cells[address];assert size==width;globals_read.append([name,size,int.from_bytes(uc.mem_read(address,size),'little')]);return
        pc=uc.reg_read(UC_X86_REG_EIP)
        if entry=='_spawn_middle_277e0' and real[entry]<=pc<real[entry]+128:
            at=r32(root)+index*8;assert (address-at,size) in [(0,4),(6,1)]
            location=['actor',address-slot] if case.get('alias_descriptor_actor') else ['A' if A<=address<A+0x100000 else 'B',address-(A if A<=address<A+0x100000 else B)-index*8]
            reads.append(['descriptor',location,size,int.from_bytes(uc.mem_read(address,size),'little')]);return
        assert POOL+0x2e<=address and address+size<=POOL+count*STRIDE and size==2 and (address-POOL-0x2e)%STRIDE==0,(hex(address),size)
        reads.append(['kind',(address-POOL-0x2e)//STRIDE])
    def write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=SP:return
        if address in cells:
            name,width=cells[address];assert hit is not None and name=='word.%x'%ROOTS[0 if kind&0x8000 else 3] and size==2 and value==hit;writes.append([name,size,value]);return
        assert entry=='_spawn_middle_277e0' and slot is not None and slot<=address and address+size<=slot+STRIDE,(hex(address),size)
        off=address-slot;writes.append(['actor',off,size,value]);pc=uc.reg_read(UC_X86_REG_EIP)
        if case.get('rotate') and real[entry]<=pc<real[entry]+128 and off==0x22:
            target=B if r32(root)==A else A;uc.mem_write(root,pack(target));interventions.append([off,target])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    for turn in range(case.get('repeat',1)):
        reads.clear();globals_read.clear();writes.clear();interventions.clear();calls_before=Counter(entered)
        hit,want_reads=expected(dict(case,config=configuration,free=list(free)));assert all(0<=n<count for n in want_reads);slot=None if hit is None else POOL+hit*STRIDE
        if case.get('null'):assert hit is None
        if case.get('alias_descriptor_actor') and slot is not None:u.mem_write(root,pack(slot+0x9c-index*8))
        baseline=bytes(u.mem_read(POOL,bank_size));coords=[(0x80000000+turn)&0xffffffff,(0xffffffff-turn)&0xffffffff,(0x7fffffff+turn)&0xffffffff]
        frame=pack(STOP)+b''.join(pack(n) for n in ([kind] if entry=='_allocate_middle_actor' else [*coords,kind]))+b'\x93'*24;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
        for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
        u.emu_start(real[entry],0,count=max(1000,24*len(want_reads)+2000))
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4 and [u.reg_read(r) for r in NV]==SEEDS and u.reg_read(UC_X86_REG_EAX)==(0 if slot is None else slot)
        assert bytes(u.mem_read(SP,len(frame)))==frame and bytes(u.mem_read(POOL-0x1000,0x1000))==b'\xab'*0x1000
        actual_reads=[r[1] for r in reads if r[0]=='kind'];assert actual_reads==want_reads
        cursor='word.%x'%ROOTS[0 if kind&0x8000 else 3];assert [w for w in writes if w[0]!='actor']==([] if hit is None else [[cursor,2,hit]])
        final=bytes(u.mem_read(POOL,bank_size))
        if slot is None or entry=='_allocate_middle_actor':assert final==baseline and not any(w[0]=='actor' for w in writes)
        else:
            assert final[:hit*STRIDE]==baseline[:hit*STRIDE] and final[(hit+1)*STRIDE:]==baseline[(hit+1)*STRIDE:]
            assert struct.unpack_from('<H',final,hit*STRIDE+0x2e)[0]==((kind|0x4000)&0xffff)
            assert final[hit*STRIDE:hit*STRIDE+12]==b''.join(pack(n) for n in coords) and final[hit*STRIDE+0x9c:hit*STRIDE+0xa8]==b''.join(pack(n) for n in coords)
            assert len([r for r in reads if r[0]=='descriptor'])==2;free.discard(hit)
        if hit is not None:configuration[0 if kind&0x8000 else 3]=hit
        for j,rva in enumerate(ROOTS):
            at=next(at for at,v in cells.items() if v[0]=='word.%x'%rva);assert bytes(u.mem_read(at,2))==struct.pack('<H',configuration[j])
        assert bytes(u.mem_read(pointer,4))==values[pointer]
        for at,raw in records.items():assert bytes(u.mem_read(at,len(raw)))==raw
        delta=entered-calls_before
        if slot is None or entry=='_allocate_middle_actor':assert sum(delta.values())==(1 if entry=='_allocate_middle_actor' else 2)
        else:assert delta==Counter({name:1 for name in REAL})
        turns.append(dict(hit=hit,kind_reads=len(actual_reads),kind_read_sha256=digest(json.dumps(actual_reads).encode()),descriptor_reads=[r for r in reads if r[0]=='descriptor'],cell_reads=list(globals_read),writes=list(writes),interventions=list(interventions),entered=dict(delta),pool_sha256=digest(final)))
    return turns


def cases():
    for case in allocator_cases():
        if case['pool']=='normal':yield dict(case,pool='middle',entry='allocator')
    for kind in [0,1,0xfff,0x1000,0x4000,0x7fff,0x8000,0xc001,0xffff,0x12340000,0xdeadc001,0xffffffff]:
        for free in [[0],[1],[2],[3],[],[0,2]]:
            for rotate in [False,True]:yield dict(pool='middle',entry='factory',flags=kind,config=[3,4,2,1,0],free=free,readable_slots=4,rotate=rotate,repeat=3)
    for kind in [0x4000,0x4001,0xc000,0xc001]:yield dict(pool='middle',entry='factory',flags=kind,config=[3,4,2,1,0],free=[0,1,2,3],readable_slots=4,alias_descriptor_actor=True,repeat=3)
    for kind in [0,0xffff]:yield dict(pool='middle',entry='factory',flags=kind,config=[0]*5,free=[],readable_slots=0,null=True,repeat=2)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/middle_factory_2026_10_25/differential');args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    built,candidate,symbols=build(out);native=PE(ORACLE);checks=[];calls=Counter()
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False);assert a==b,dict(case=case,native=a,source=b);checks.append(dict(case=case,result=a))
        for turn in a:calls.update(turn['entered'])
    assert all(digest(Path(p).read_bytes())==h for p,h in built['input_sha256'].items())
    raw=json.dumps(checks,sort_keys=True).encode();(out/'cases.json').write_bytes(raw+b'\n')
    report=dict(scope=__doc__,cases=len(checks),all_equal=True,by_entry=dict(Counter(c['case']['entry'] for c in checks)),top_level_invocations=sum(len(c['result']) for c in checks),real_invocations=dict(calls),alias_cases=sum(bool(c['case'].get('alias_descriptor_actor')) for c in checks),root_intervention_invocations=sum(bool(t['interventions']) for c in checks for t in c['result']),kind_reads=sum(t['kind_reads'] for c in checks for t in c['result']),cases_sha256=digest(raw),build=built,limits=['Readable pool/table capacities are synthetic, including65535-slot stress banks; no native allocation capacity is inferred.','External root rotations and descriptor/actor aliases are explicit fixture states, not a claim that real helpers mutate the root.','Complete function matching and independent scalar ownership remain separate acceptance gates; no original OS module starts.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k in ['cases','all_equal','by_entry','top_level_invocations','real_invocations','alias_cases','root_intervention_invocations','kind_reads']}))


if __name__=='__main__':main()
