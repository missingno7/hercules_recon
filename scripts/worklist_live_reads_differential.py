"""Worklist factory live asset reads against unpatched native instructions.

The real allocator/initializer chain executes in Unicorn. Descriptor banks,
record/actor aliases and root/kind interventions are synthetic. Source prefix
guards are explicit read-only models for these unregistered banks and group0;
callbacks are models. No original OS module is loaded or started.
"""
import argparse
from collections import Counter
import json,os,struct,subprocess,sys
from pathlib import Path
from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,compile_inputs,digest
from coff import COFF
from masked import masked_compare
from pe import PE
from title_link import read_map
from coordinate_factories_differential import build as build_helpers,ORACLE,NV,SEEDS,pack,STACK,SP,STOP
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
SOURCE='src/eng1/worklist_factories.c';MIDDLE='src/eng1/middle_actor_factory.cpp'
HELPERS={'_allocate_normal_actor':(0x26f70,336),'_allocate_middle_actor':(0x275d0,336),'_allocate_alternate_actor':(0x27b00,320),'_initialize_normal_actor':(0x27520,176),'_initialize_middle_actor':(0x27a50,176),'_initialize_alternate_actor':(0x27fe0,272)}
SPECS={'normal':dict(entry=0x270c0,size=272,stride=0x134,pointer=0x72860,roots=[0x71308,0x713b4,0x713a8,0x713c4,0x71378],assets=0x71388,callbacks=0x71370,row=16,callback_stride=12),'middle':dict(entry=0x27720,size=192,stride=0xec,pointer=0x73494,roots=[0x713ac,0x71432,0x713a0,0x713b8,0x713aa],assets=0x71390,callbacks=0x7137c,row=8,callback_stride=8),'alternate':dict(entry=0x27c40,size=144,stride=0x94,pointer=0x734e0,roots=[0x71374,0x713ba,0x713b6,0x71430,0x7138c],assets=0x713a4,callbacks=0x7139c,row=8,callback_stride=8)}
POOL=0x61001000;RECORD=POOL+0x2000;OWNER=POOL+0x3000;A=0x63000000;B=0x63100000;CBTABLE=0x63200000;CALLBACK=0x63400000


def build(out,source):
    out.mkdir(parents=True,exist_ok=True);prior,_,_=build_helpers(out/'helpers');pins=dict(prior['input_sha256']);pins[str(Path(__file__).resolve())]=digest(Path(__file__).read_bytes());objects=[p for p in prior['commands'][-1] if p.lower().endswith('.obj')];commands=[]
    for src in [ROOT/MIDDLE,source]:
        flags=['/O2','/I'+str((ROOT/SOURCE).parent)]
        pins.update({str(p):h for p,h in compile_inputs(src,flags,DEFAULT_TOOLCHAIN).items()});obj,cmd=compile_source(src,flags);objects.append(str(obj));commands.append(cmd)
        if src==source:source_coff=COFF(obj)
        else:assert masked_compare(obj,'_allocate_middle_actor',ORACLE,0x275d0,336)['masked_equal']
    fixture=out/'cells_and_models.c';fixture.write_text('#include "'+(ROOT/'src/eng1/resource_runtime_data.h').as_posix()+'"\n/* Private complete fixture objects and intercepted model endpoints only. */\nResourceKindInfo g_kind_info[1];\nvoid *g_middle_actor_pool;\nunsigned long g_configuration_dword_71390,g_configuration_dword_71370,g_configuration_dword_7137c,g_configuration_dword_7139c;\n'+''.join('unsigned short g_configuration_word_%x;\n'%r for r in SPECS['middle']['roots'])+'void eng1_guard_worklist_table_read(const void*p,unsigned long i,unsigned long s,unsigned long w,const char*n){for(;;){}}\nvoid eng1_guard_worklist_group_owner(unsigned long i){for(;;){}}\n')
    pins[str(fixture)]=digest(fixture.read_bytes());obj,cmd=compile_source(fixture,['/O2']);objects.append(str(obj));commands.append(cmd)
    image=out/'fixture.dll';mapfile=out/'fixture.map';cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),*['/EXPORT:eng1_call_%x'%s['entry'] for s in SPECS.values()],*objects];commands.append(cmd)
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');run=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True,timeout=60);(out/'link.log').write_text(run.stdout+run.stderr);assert run.returncode==0,run.stdout+run.stderr
    symbols=read_map(mapfile);source_ranges={s['name']:source_coff.function(s['name'])['size'] for s in source_coff.symbols.values() if s['section']>0 and s['type']&0x20 and s['name'] in symbols}
    assert all('_eng1_call_%x'%s['entry'] in source_ranges for s in SPECS.values());assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,helper_build=prior,commands=commands,image_sha256=digest(image.read_bytes()),source_ranges=source_ranges),PE(image),symbols


def source_guard_fixture(out,source):
    """Execute only newly compiled source/spies, with the real prefix guards."""
    from eng1_data_evidence import scan_quoted_includes
    out.mkdir(parents=True,exist_ok=True)
    sources=[ROOT/'tests/eng1_worklist_factory_spies.c',source,ROOT/'src/eng1/worklist_read_guards.c',ROOT/'src/shared/actor_pool_reset_repaired.c'];inputs=sources+scan_quoted_includes([sources[0],ROOT/SOURCE,*sources[2:]])
    pins={str(p.resolve()):digest(p.read_bytes()) for p in inputs};image=out/'source_guard_fixture.exe'
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH',''),INCLUDE=str(DEFAULT_TOOLCHAIN/'include'),LIB=str(DEFAULT_TOOLCHAIN/'lib'))
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/cl.exe'),'/nologo','/O2','/Gy','/MT','/DENG1_WORKLIST_GUARD_FIXTURE','/I'+str((ROOT/SOURCE).parent),'/Fo'+str(out)+os.sep,'/Fe'+str(image),*map(str,sources),'/link','/INCREMENTAL:NO','/OPT:REF','kernel32.lib']
    build=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True,timeout=60);(out/'compile.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
    run=subprocess.run([str(image)],cwd=out,env=env,capture_output=True,text=True,timeout=30);(out/'run.log').write_text(run.stdout+run.stderr);assert run.returncode==0 and 'worklist_factory_cases=9 checks=9 failures=0' in run.stdout,run.stdout+run.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(scope='Fresh source-only spies and actual registered-prefix guards; no original file/module is supplied or loaded.',cases=9,all_pass=True,command=cmd,input_sha256=pins,executable_sha256=digest(image.read_bytes()),stdout=run.stdout)


def execute(pe,symbols,source_ranges,case,original):
    spec=SPECS[case['family']];stride=spec['stride'];kind=case['kind'];hit=None if case['fail'] else (2 if kind&0x8000 else 0);slot=None if hit is None else POOL+hit*stride
    record=RECORD if case['alias'] is None else slot+case['alias'];initial=bytearray(bytes(range(256))*0x40)
    for n in range(4):struct.pack_into('<H',initial,n*stride+0x2e,0 if n in ([0,2] if not case['fail'] else []) else 0x202f)
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff;u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'));u.mem_map(POOL-0x1000,0x5000);u.mem_write(POOL,bytes(initial));u.mem_write(POOL-0x1000,b'\xab'*0x1000);u.mem_map(A,0x300000);u.mem_map(CALLBACK,0x1000);u.mem_map(STACK,0x10000)
    u.mem_write(record,struct.pack('<IIIHHHH',0x12345678,0xfedcba98,0x76543210,case['flags'],0x2345,kind,0x6172))
    for bank,seed in [(A,0x31000000),(B,0x32000000)]:
        raw=bytearray(0x100000)
        for index in range(65536 if spec['row']==16 else 4096):
            off=index*spec['row'];struct.pack_into('<I',raw,off,seed|((index+1)&7))
            if spec['row']==16:struct.pack_into('<II',raw,off+4,0x42000000|((index+2)&7),0x53000000|index);raw[off+12]=(index+3)&255
            else:raw[off+6]=(index+3)&255
        u.mem_write(bank,bytes(raw))
    cached_index=kind if spec['row']==16 else kind&0xfff;u.mem_write(CBTABLE+cached_index*spec['callback_stride'],pack(CALLBACK))
    cells={};values={}
    for family,other in SPECS.items():
        at=pe.image_base+other['pointer'] if original else symbols['_g_'+family+'_actor_pool'];cells[at]=(family+'.pool',4);values[at]=pack(POOL if family==case['family'] else 0)
        for j,rva in enumerate(other['roots']):
            at=pe.image_base+rva if original else symbols['_g_configuration_word_%x'%rva];cells[at]=(family+'.%x'%rva,2);values[at]=struct.pack('<H',[3,4,2,1,0][j] if family==case['family'] else 0)
        for rva,role,value in [(other['assets'],'assets',A),(other['callbacks'],'callbacks',CBTABLE)]:
            at=pe.image_base+rva if original else symbols['_g_configuration_dword_%x'%rva];cells[at]=(family+'.'+role,4);values[at]=pack(value if family==case['family'] else 0)
    info=pe.image_base+0x6fee8 if original else symbols['_g_kind_info'];cells[info]=('group.owner',4);values[info]=pack(OWNER);u.mem_write(info+4,pack(0x12345678))
    for at,raw in values.items():u.mem_write(at,raw)
    root=next(at for at,v in cells.items() if v[0]==case['family']+'.assets');real={n:pe.image_base+r if original else symbols[n] for n,(r,z) in HELPERS.items()};entry=pe.image_base+spec['entry'] if original else symbols['_eng1_call_%x'%spec['entry']]
    allowed=[(v,HELPERS[n][1]) for n,v in real.items()]+([(entry,spec['size'])] if original else [(symbols[n],z) for n,z in source_ranges.items()]);guards={} if original else {symbols['_eng1_guard_worklist_table_read']:'table',symbols['_eng1_guard_worklist_group_owner']:'group'}
    writes=[];descriptors=[];callbacks=[];guard_events=[];interventions=[];counts=Counter();entered=Counter()
    def r32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def model_return():
        esp=u.reg_read(UC_X86_REG_ESP);destination=r32(esp)
        for reg,val in [(UC_X86_REG_EAX,0xface0000),(UC_X86_REG_ECX,0xabad1dea),(UC_X86_REG_EDX,0xdeadc0de)]:u.reg_write(reg,val)
        u.reg_write(UC_X86_REG_ESP,esp+4);u.reg_write(UC_X86_REG_EIP,destination)
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        if address in guards:
            esp=uc.reg_read(UC_X86_REG_ESP)
            if guards[address]=='group':assert r32(esp+4)==0;guard_events.append(['group',0])
            else:
                base,index,row,width=[r32(esp+4+j*4) for j in range(4)];assert row in (8,12,16) and 0<width<=row and (A<=base<B+0x100000 or base==CBTABLE);guard_events.append(['table',base,index,row,width])
            model_return();return
        if address==CALLBACK:
            esp=uc.reg_read(UC_X86_REG_ESP);assert slot is not None and r32(esp+4)==slot;callbacks.append(['callback',hit]);model_return();return
        for name,at in real.items():
            if address==at:entered[name]+=1
        assert any(at<=address and address+size<=at+length for at,length in allowed),hex(address)
    def read(uc,access,address,size,value,user):
        if STACK<=address and address+size<=STACK+0x10000:return
        if address in cells:assert size==cells[address][1];return
        if POOL<=address and address+size<=POOL+0x4000:return
        if A<=address and address+size<=B+0x100000:
            bank=A if address<B else B;offset=address-bank;index,field=divmod(offset,spec['row']);assert (field,size) in ([(12,1),(0,4),(4,4),(8,4)] if spec['row']==16 else [(6,1),(0,4)])
            descriptors.append(['A' if bank==A else 'B',index,field,size,int.from_bytes(uc.mem_read(address,size),'little')]);return
        assert address==CBTABLE+cached_index*spec['callback_stride'] and size==4,(hex(address),size)
    def write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=SP:return
        # A C compiler may reuse the callee's two incoming argument homes.
        # They are excluded from semantic equality; return/caller guards are not.
        if SP+4<=address and address+size<=SP+12:return
        if address in cells:
            assert slot is not None and cells[address][0]==case['family']+'.%x'%spec['roots'][0 if kind&0x8000 else 3] and size==2 and value==hit;writes.append(['cursor',size,value]);return
        assert record<=address and address+size<=record+20 or slot is not None and slot<=address and address+size<=slot+stride,(hex(address),size)
        writes.append([address-POOL,size,value])
        if slot is not None and address in [slot+0x22,slot+0x64,slot+0x110]:
            off=address-slot;counts[off]+=1
            if counts[off]==2:
                if case['rotate'] and (off==0x22 or spec['row']==16):
                    target=B if r32(root)==A else A;uc.mem_write(root,pack(target));interventions.append(['root',off,target])
                if case['kind_live']:
                    value={0x22:2,0x64:3,0x110:4}[off];uc.mem_write(record+0x10,struct.pack('<H',value));interventions.append(['kind',off,value])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    frame=pack(STOP)+pack(record)+pack(0)+b'\x93'*24;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
    for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
    u.emu_start(entry,0,count=30000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4 and [u.reg_read(r) for r in NV]==SEEDS and u.reg_read(UC_X86_REG_EAX)==(0 if slot is None else slot)
    assert bytes(u.mem_read(SP,4))==frame[:4] and bytes(u.mem_read(SP+12,len(frame)-12))==frame[12:] and bytes(u.mem_read(POOL-0x1000,0x1000))==b'\xab'*0x1000 and len(callbacks)==(0 if slot is None else 1)
    return dict(pool_sha256=digest(bytes(u.mem_read(POOL,0x4000))),writes=writes,descriptors=descriptors,callbacks=callbacks,interventions=interventions,real_entries=dict(entered)),guard_events


def cases():
    for family in SPECS:
        scenarios=[dict(alias=None,rotate=False,kind_live=False),dict(alias=None,rotate=True,kind_live=False),dict(alias=0x54,rotate=False,kind_live=False),dict(alias=0x12,rotate=False,kind_live=False)]
        if family=='normal':scenarios += [dict(alias=None,rotate=False,kind_live=True),dict(alias=None,rotate=True,kind_live=True),dict(alias=0x100,rotate=False,kind_live=False),dict(alias=0x108,rotate=False,kind_live=False)]
        for kind in [1,0x8001]:
            for flags in [0,0x1000,0xffff]:
                for scenario in scenarios:yield dict(family=family,kind=kind,flags=flags,fail=False,**scenario)
            for rotate in [False,True]:yield dict(family=family,kind=kind,flags=0x1000,fail=True,alias=None,rotate=rotate,kind_live=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/worklist_recovery_2026_10_25/differential');parser.add_argument('--source',type=Path,default=ROOT/SOURCE);parser.add_argument('--diagnose',action='store_true');args=parser.parse_args();out=args.output.resolve();source=args.source.resolve();assert out.is_relative_to(ROOT/'work') and (source==ROOT/SOURCE or source.is_relative_to(ROOT/'work'))
    built,candidate,symbols=build(out,source);native=PE(ORACLE);checks=[];failures=[]
    for case in cases():
        a,_=execute(native,symbols,built['source_ranges'],case,True);b,guards=execute(candidate,symbols,built['source_ranges'],case,False);different=[k for k in a if a[k]!=b[k]]
        checks.append(dict(case=case,native=a,source=b,source_guard_events=guards,equal=not different))
        if different:failures.append(dict(case=case,differing_fields=different))
    assert all(digest(Path(p).read_bytes())==h for p,h in built['input_sha256'].items());raw=json.dumps(checks,sort_keys=True).encode();(out/'cases.json').write_bytes(raw+b'\n')
    guards=source_guard_fixture(out/'source_guards',source)
    report=dict(scope=__doc__,cases=len(checks),all_equal=not failures,failed_cases=len(failures),failures=failures,by_family={n:sum(c['case']['family']==n for c in checks) for n in SPECS},source_guard_invocations=sum(len(c['source_guard_events']) for c in checks),source_prefix_guards=guards,cases_sha256=digest(raw),build=built,limits=['Only group0 and synthetic unregistered asset/callback banks are covered by the native comparison; nine source-only tests separately retain actual prefix-boundary failures.','Descriptor/kind interventions and record/actor aliases test specific reload boundaries, not actual native capacities or callback bodies.','Callee argument homes may differ as ordinary C scratch space; return address, caller guards, ESP and preserved registers are checked.','No instruction matching, complete indexed-owner proof, original OS loading or full gameplay claim follows.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k in ['cases','all_equal','failed_cases','by_family','source_guard_invocations']}))
    if failures and not args.diagnose:raise SystemExit(1)


if __name__=='__main__':main()
