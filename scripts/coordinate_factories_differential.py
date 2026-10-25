"""Bounded coordinate-factory composition using real allocators/initializers.

Unpatched native/source instructions run in Unicorn, with synthetic four-slot
pools and descriptor banks. Optional fixture interventions change the descriptor
root between actor stores to expose reload order. No original OS module starts.
"""
import argparse
from collections import Counter
import json,os,re,struct,subprocess,sys
from pathlib import Path
from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,compile_inputs,digest,verify_toolchain
from coff import COFF
from masked import masked_compare
from pe import PE
from refinery_forms import function_text,replace_function
from title_link import read_map
from actor_pool_allocators_differential import SPECS
sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP
ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
SOURCE='src/eng1/coordinate_factories.cpp'
REAL={'_spawn_normal':(0x271d0,144),'_spawn_kind':(0x27db0,112),'_allocate_normal_actor':(0x26f70,336),'_allocate_alternate_actor':(0x27b00,320),'_initialize_normal_actor':(0x27520,176),'_initialize_middle_actor':(0x27a50,176),'_initialize_alternate_actor':(0x27fe0,272)}
POOL=0x61001000;A=0x61100000;B=0x61200000;STACK=0x65000000;SP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x12345678,0x23456789,0x3456789a,0x456789ab]
pack=lambda n:struct.pack('<I',n&0xffffffff)


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    inputs={ORACLE,runtime,Path(__file__).resolve(),ROOT/'scripts/actor_pool_allocators_differential.py',ROOT/'toolchains/manifest.json'}
    sources=[SOURCE,'src/eng1/actor_pool_allocators.c','src/eng1/actor_middle_init.c','src/eng1/actor_base_init.c','src/title/u06bb0.cpp']
    for source in sources:inputs.update(compile_inputs(ROOT/source,['/O2'],DEFAULT_TOOLCHAIN))
    pins={str(p):digest(p.read_bytes()) for p in inputs};objects=[];commands=[];checks=[];out.mkdir(parents=True,exist_ok=True)
    for source in sources:
        obj,cmd=compile_source(ROOT/source,['/O2']);commands.append(cmd)
        if source=='src/title/u06bb0.cpp':
            original=COFF(obj);text=(ROOT/source).read_text()
            for sym in original.symbols.values():
                if sym['section']<=0 or not sym['type']&0x20 or sym['name']=='_title_08cd0':continue
                name=sym['name'][1:];body=function_text(text,name);text=replace_function(text,name,body.split('{',1)[0].rstrip()+';')
            text=re.sub(r'#include "([^"]+)"',lambda m:'#include "'+(ROOT/source).parent.joinpath(m[1]).as_posix()+'"',text)
            fixture=out/'normal_initializer.cpp';fixture.write_text(text);pins[str(fixture)]=digest(fixture.read_bytes())
            obj,cmd=compile_source(fixture,['/O2','/Dtitle_08cd0=initialize_normal_actor','/Dtitle_090f0=initialize_middle_actor']);commands.append(cmd)
            a=original.function('_title_08cd0');b=COFF(obj).function('_initialize_normal_actor')
            assert a['data']==b['data'] and [{**r,'symbol':'_initialize_middle_actor'} for r in a['relocations']]==b['relocations']
        objects.append(obj)
        for name,(rva,size) in REAL.items():
            if name not in {s['name'] for s in COFF(obj).symbols.values() if s['section']>0 and s['type']&0x20}:continue
            check=masked_compare(obj,name,ORACLE,rva,size);assert check['masked_equal'],(name,check)
            checks.append(dict(symbol=name,diagnostic=check))
    assert {c['symbol'] for c in checks}==REAL.keys()
    fixture=out/'storage.c'
    fixture.write_text('/* Synthetic complete scalar objects only; never production owners. */\nvoid *g_normal_actor_pool;\nvoid *g_alternate_actor_pool;\nunsigned long g_configuration_dword_71388,g_configuration_dword_713a4;\n'+''.join('unsigned short g_configuration_word_%x;\n'%r for spec in SPECS.values() for r in spec['roots']))
    pins[str(fixture)]=digest(fixture.read_bytes());obj,cmd=compile_source(fixture,['/O2']);objects.append(obj);commands.append(cmd)
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),'/EXPORT:spawn_normal','/EXPORT:spawn_kind',*map(str,objects)];commands.append(cmd)
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');ran=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,commands=commands,checks=checks,normal_initializer_private_extraction_complete_contribution_preserved=True,image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)


def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(POOL-0x1000,0x4000);u.mem_map(A,0x200000);u.mem_map(STACK,0x10000)
    spec=SPECS[case['pool']];stride=spec['stride'];kind=case['kind'];index=kind&(0xffff if case['pool']=='normal' else 0xfff);rowstride=16 if case['pool']=='normal' else 8
    payload=bytearray(bytes(range(256))*0x30)
    for n in range(4):struct.pack_into('<H',payload,n*stride+0x2e,0 if n in case['free'] else 0x201f)
    u.mem_write(POOL,bytes(payload));u.mem_write(POOL-0x1000,b'\xab'*0x1000)
    descriptors={}
    for bank,seed in [(A,0x31415926),(B,0x27182818)]:
        raw=struct.pack('<III',seed,index^seed,(seed+index)&0xffffffff)+bytes([seed&255,0xa5,0x5a,0xff]) if rowstride==16 else struct.pack('<I',seed)+bytes([0xa5,0x5a,seed&255,0xff])
        u.mem_write(bank+index*rowstride,raw);descriptors[bank+index*rowstride]=raw
    globals_by_address={};initial={};root=0
    for family,other in SPECS.items():
        pointer=pe.image_base+other['pointer'] if original else symbols['_g_'+family+'_actor_pool'];initial[pointer]=pack(POOL if family==case['pool'] else 0);globals_by_address[pointer]=(family+'.pool',4)
        for n,rva in enumerate(other['roots']):
            address=pe.image_base+rva if original else symbols['_g_configuration_word_%x'%rva]
            initial[address]=struct.pack('<H',case['config'][n] if family==case['pool'] else 0xc123+n);globals_by_address[address]=(family+'.%x'%rva,2)
        rva=0x71388 if family=='normal' else 0x713a4;address=pe.image_base+rva if original else symbols['_g_configuration_dword_%x'%rva]
        initial[address]=pack(A if family==case['pool'] else 0);globals_by_address[address]=(family+'.descriptors',4)
        if family==case['pool']:root=address
    for address,data in initial.items():u.mem_write(address,data)
    entry='_spawn_normal' if case['pool']=='normal' else '_spawn_kind';real={name:pe.image_base+rva if original else symbols[name] for name,(rva,size) in REAL.items()}
    cp,limit,split,ca,start=case['config'];cursor,end,wrap=(cp,limit,split) if kind&0x8000 else (ca,split,start)
    order=list(range(cursor,end))+list(range(wrap,cursor));hit=next((n for n in order if n in case['free']),None);slot=None if hit is None else POOL+hit*stride
    reads=[];writes=[];entered=Counter();interventions=[]
    def r32(address):return struct.unpack('<I',u.mem_read(address,4))[0]
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        for name,at in real.items():
            if address==at:entered[name]+=1
        assert any(at<=address and address+size<=at+REAL[name][1] for name,at in real.items()),hex(address)
    def read(uc,access,address,size,value,user):
        if STACK<=address and address+size<=STACK+0x10000:return
        if address in globals_by_address:
            name,width=globals_by_address[address];assert size==width and name.startswith(case['pool']+'.');reads.append([name,size,int.from_bytes(uc.mem_read(address,size),'little')]);return
        if POOL<=address and address+size<=POOL+4*stride:
            assert size==2 and (address-POOL-0x2e)%stride==0;reads.append(['kind',(address-POOL-0x2e)//stride]);return
        for at,raw in descriptors.items():
            if at<=address and address+size<=at+rowstride:
                off=address-at;assert (off,size) in ([(0,4),(4,4),(8,4),(12,1)] if rowstride==16 else [(0,4),(6,1)])
                reads.append(['descriptor','A' if at<A+0x100000 else 'B',off,size]);return
        raise AssertionError(('read',hex(address),size))
    def write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=SP:return
        if address in globals_by_address:
            name,width=globals_by_address[address];assert name==case['pool']+'.%x'%spec['roots'][0 if kind&0x8000 else 3] and size==2 and value==hit
            writes.append([name,size,value]);return
        assert slot is not None and slot<=address and address+size<=slot+stride,(hex(address),size)
        off=address-slot;writes.append(['actor',off,size,value])
        # Explicit external fixture intervention, after the factory's category
        # write (and then image/frame writes); never attributed to a native helper.
        if case['rotate'] and uc.reg_read(UC_X86_REG_EIP)>=real[entry] and uc.reg_read(UC_X86_REG_EIP)<real[entry]+REAL[entry][1] and off in ([0x22,0x64,0x110] if rowstride==16 else [0x22]):
            target=B if len(interventions)%2==0 else A;uc.mem_write(root,pack(target));interventions.append([off,target])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    frame=pack(STOP)+b''.join(pack(n) for n in [*case['coords'],kind])+b'\x93'*24;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
    for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
    u.emu_start(real[entry],0,count=10000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4 and [u.reg_read(r) for r in NV]==SEEDS
    assert u.reg_read(UC_X86_REG_EAX)==(0 if slot is None else slot)
    assert bytes(u.mem_read(SP,len(frame)))==frame and bytes(u.mem_read(POOL-0x1000,0x1000))==b'\xab'*0x1000
    final=bytes(u.mem_read(POOL,len(payload)))
    if slot is None:assert final==bytes(payload) and not interventions and sum(entered.values())==2
    else:
        assert final[:hit*stride]==payload[:hit*stride] and final[(hit+1)*stride:]==payload[(hit+1)*stride:]
        assert struct.unpack_from('<H',final,hit*stride+0x2e)[0]==((kind|(0x2000 if rowstride==8 else 0))&0xffff)
        assert final[hit*stride:hit*stride+12]==b''.join(pack(n) for n in case['coords'])
    for at,raw in descriptors.items():assert bytes(u.mem_read(at,len(raw)))==raw
    return dict(hit=hit,reads=reads,writes=writes,entered=dict(entered),interventions=interventions,pool_sha256=digest(final),globals={name:int.from_bytes(u.mem_read(at,width),'little') for at,(name,width) in globals_by_address.items()})


def cases():
    for family in SPECS:
        for kind in [0,1,0x0fff,0x1000,0x7fff,0x8000,0x8001,0xffff,0x12340000,0xdead8001,0xffffffff]:
            for free in [[0],[1],[2],[3],[],[0,2]]:
                for rotate in [False,True]:
                    yield dict(pool=family,kind=kind,config=[3,4,2,1,0],free=free,rotate=rotate,coords=[0x80000000,0xffffffff,0x7fffffff])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/factory_twin_2026_10_25/differential');args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out);native=PE(ORACLE);checks=[];calls=Counter()
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False);assert a==b,dict(case=case,native=a,source=b)
        checks.append(dict(case=case,result=a));calls.update(a['entered'])
    assert all(digest(Path(p).read_bytes())==h for p,h in build_report['input_sha256'].items())
    raw=json.dumps(checks,sort_keys=True).encode();(out/'cases.json').write_bytes(raw+b'\n')
    report=dict(scope=__doc__,cases=len(checks),all_equal=True,real_invocations=dict(calls),cases_sha256=digest(raw),root_intervention_cases=sum(bool(c['result']['interventions']) for c in checks),build=build_report,limits=['Synthetic four-slot pools and readable descriptor banks establish no original allocation capacities.','Optional root changes are explicit external interventions to test reload order, not a claim that real initializer/helpers change roots.','Exact complete contributions and independently bound references remain the separate acceptance gate.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k in ['cases','all_equal','real_invocations','root_intervention_cases']}))


if __name__=='__main__':main()
