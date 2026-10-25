"""Guarded full-record/ordered-store comparison of the actor base initializer.

Original and source instructions execute only inside Unicorn, without external
services, OS module loading, instruction changes or game startup.
"""
import argparse,json,os,random,struct,subprocess,sys
from pathlib import Path
from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,digest,verify_toolchain
from pe import PE
from title_link import read_map
sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)
SOURCE=ROOT/'src/eng1/behavioral/actor_base_init.c';ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
DATA=0x61000000;STACK=0x62000000;ESP=STACK+0x8000;STOP=STACK+0xf000
NV=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP);SEEDS=(0x12345678,0x23456789,0x3456789a,0x456789ab)
pack=lambda n:struct.pack('<I',n&0xffffffff)

def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN);runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins={str(p):digest(p.read_bytes()) for p in [SOURCE,ORACLE,Path(__file__).resolve(),runtime,ROOT/'toolchains/manifest.json']}
    out.mkdir(parents=True,exist_ok=True);obj,cmd=compile_source(SOURCE,['/O2'])
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    link=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:NOREF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),str(obj)]
    env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    ran=subprocess.run(link,cwd=out,env=env,capture_output=True,text=True,timeout=60);(out/'link.log').write_text(ran.stdout+ran.stderr)
    assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,commands=[cmd,link],object_sha256=digest(obj.read_bytes()),image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)

def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);base=pe.image_base;extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,extent);u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(DATA,0x1000);u.mem_map(STACK,0x10000)
    rng=random.Random(case['seed']);payload=bytes([case['fill']])*0x1000 if 'fill' in case else bytes(rng.randrange(256) for _ in range(0x1000))
    u.mem_write(DATA,payload);actor=DATA+case['offset'];entry=base+0x27fe0 if original else symbols['_initialize_alternate_actor']
    frame=pack(STOP)+pack(actor)+b'\x93'*24;stores=[]
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        assert entry<=address<entry+272,hex(address)
    def read(uc,access,address,size,value,user):assert STACK<=address and address+size<=STACK+0x10000,hex(address)
    def write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=ESP:return
        assert actor<=address and address+size<=actor+0x94,(hex(address),size)
        stores.append([address-actor,size,value])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    results=[]
    for _ in range(case.get('repeat',1)):
        stores.clear();u.mem_write(ESP,frame);u.reg_write(UC_X86_REG_ESP,ESP)
        for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
        u.emu_start(entry,0,count=1000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4
        assert [u.reg_read(r) for r in NV]==list(SEEDS)
        assert bytes(u.mem_read(ESP,len(frame)))==frame
        result=bytes(u.mem_read(DATA,0x1000))
        assert result[:case['offset']]==payload[:case['offset']] and result[case['offset']+0x94:]==payload[case['offset']+0x94:]
        written={at+n for at,size,value in stores for n in range(size)}
        untouched=sorted(set(range(0x94))-written)
        assert all(result[case['offset']+n]==payload[case['offset']+n] for n in untouched)
        # No broad zero-fill: signed index sentinel and untouched bytes matter.
        assert result[case['offset']+0x32:case['offset']+0x34]==b'\xff\xff'
        results.append(dict(record=result[case['offset']:case['offset']+0x94].hex(),stores=list(stores),untouched=untouched))
        payload=result
    return results

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/allocator_matching_2026_10_25/initializer/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out);native=PE(ORACLE);checks=[]
    for offset in (0x100,0x104,0x120):
        for seed in range(66):
            case=dict(offset=offset,seed=seed,repeat=2 if seed<2 else 1)
            if seed<2:case['fill']=255*seed
            a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False);assert a==b,case
            checks.append(dict(case=case,result=a))
    assert all(digest(Path(p).read_bytes())==h for p,h in build_report['input_sha256'].items())
    report=dict(scope=__doc__,cases=len(checks),all_equal=True,build=build_report,stores_per_call=len(checks[0]['result'][0]['stores']),untouched_byte_offsets=checks[0]['result'][0]['untouched'],cases_sha256=digest(json.dumps(checks,sort_keys=True).encode()),limits=['Only the shared 0x94-byte prefix is measured; no whole actor lifecycle, allocation capacity or original source typedef is established.'])
    assert all(r['untouched']==report['untouched_byte_offsets'] and len(r['stores'])==report['stores_per_call'] for c in checks for r in c['result'])
    (out/'cases.json').write_text(json.dumps(checks,indent=2)+'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['cases','all_equal','stores_per_call','untouched_byte_offsets']}))

if __name__=='__main__':main()
