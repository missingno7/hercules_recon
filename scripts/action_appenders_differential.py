"""Action appenders composed with real coordinate factories and pool helpers.

Native/source instructions run unpatched in Unicorn. Pool/group/table storage
is synthetic. The indirect callback is an explicit model which may replace the
live group count; its behavior and native allocation capacities are not proved.
No original OS module or game starts.
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
from coordinate_factories_differential import build as build_factories,REAL as FACTORIES,SPECS,ORACLE,POOL,A,B,STACK,SP,STOP,NV,SEEDS,pack
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
SOURCE='src/eng1/action_group_writers.c'
REAL={**FACTORIES,'_append_normal_action':(0x2b070,96),'_append_kind_action':(0x2b0d0,96)}
GROUP=POOL+0x1800;CALLTABLE=0x61300000;CALLBACK=0x61400000


def build(out):
    out.mkdir(parents=True,exist_ok=True);prior,_,_=build_factories(out/'factories')
    pins=dict(prior['input_sha256']);pins[str(Path(__file__).resolve())]=digest(Path(__file__).read_bytes())
    pins.update({str(p):h for p,h in compile_inputs(ROOT/SOURCE,['/O2'],DEFAULT_TOOLCHAIN).items()})
    obj,command=compile_source(ROOT/SOURCE,['/O2']);checks={}
    for name,rva in [('_append_normal_action',0x2b070),('_append_kind_action',0x2b0d0)]:
        check=masked_compare(obj,name,ORACLE,rva,96);assert check['masked_equal'],check;checks[name]=check
    fixture=out/'callback_cells.c'
    fixture.write_text('/* Complete synthetic fixture objects only, never production owners. */\nunsigned short g_configuration_word_71394,g_configuration_word_71380;\nunsigned long g_configuration_dword_71370,g_configuration_dword_7139c;\nvoid *pending_action_group_table[1];\n')
    pins[str(fixture)]=digest(fixture.read_bytes());cells,cellcmd=compile_source(fixture,['/O2'])
    objects=[p for p in prior['commands'][-1] if p.lower().endswith('.obj')]+[str(obj),str(cells)]
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    link=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),'/EXPORT:append_normal_action','/EXPORT:append_kind_action',*objects]
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');ran=subprocess.run(link,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,factory_build=prior,commands=[command,cellcmd,link],checks=checks,image_sha256=digest(image.read_bytes())),PE(image),read_map(mapfile)


def execute(pe,symbols,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(POOL-0x1000,0x4000);u.mem_map(A,0x200000);u.mem_map(CALLTABLE,0x10000);u.mem_map(CALLBACK,0x1000);u.mem_map(STACK,0x10000)
    family=case['pool'];spec=SPECS[family];stride=spec['stride'];kind=case['kind'];rowstride=16 if family=='normal' else 8;index=kind&(0xffff if family=='normal' else 0xfff);ci=kind&0x1ff
    payload=bytearray(bytes(range(256))*0x30)
    for n in range(4):struct.pack_into('<H',payload,n*stride+0x2e,0 if n in case['free'] else 0x201f)
    u.mem_write(POOL,bytes(payload));u.mem_write(POOL-0x1000,b'\xab'*0x1000)
    group=POOL+(2 if kind&0x8000 else 0)*stride if case.get('alias_group_actor') else GROUP
    u.mem_write(group-8,b'\xc7'*0x40);u.mem_write(group,pack(case['count']))
    # Alias cases reserve only the selected actor's x/count field, leaving its
    # allocation kind free; the real factory overwrites x before the callback.
    if case.get('alias_group_actor'):u.mem_write(group+0x2e,b'\0\0')
    descriptor=struct.pack('<III',0x31415926,0x12345678,0x87654321)+bytes([0x6d,0xa5,0x5a,0xff]) if rowstride==16 else struct.pack('<I',0x27182818)+bytes([0xa5,0x5a,0x4c,0xff])
    u.mem_write(A+index*rowstride,descriptor)
    callback_stride=12 if family=='normal' else 8;callback_row=CALLTABLE+ci*callback_stride;u.mem_write(callback_row,pack(CALLBACK))
    globals_by_address={};initial={}
    for pool,other in SPECS.items():
        pointer=pe.image_base+other['pointer'] if original else symbols['_g_'+pool+'_actor_pool'];initial[pointer]=pack(POOL if pool==family else 0);globals_by_address[pointer]=(pool+'.pool',4)
        for n,rva in enumerate(other['roots']):
            at=pe.image_base+rva if original else symbols['_g_configuration_word_%x'%rva];initial[at]=struct.pack('<H',[3,4,2,1,0][n] if pool==family else 0xc123+n);globals_by_address[at]=(pool+'.%x'%rva,2)
        for rva,role,value in [(0x71388 if pool=='normal' else 0x713a4,'descriptors',A),(0x71370 if pool=='normal' else 0x7139c,'callbacks',CALLTABLE)]:
            at=pe.image_base+rva if original else symbols['_g_configuration_dword_%x'%rva];initial[at]=pack(value if pool==family else 0);globals_by_address[at]=(pool+'.'+role,4)
    limit=pe.image_base+0x71394 if original else symbols['_g_configuration_word_71394'];initial[limit]=struct.pack('<H',case['limit']);globals_by_address[limit]=('limit',2)
    for at,data in initial.items():u.mem_write(at,data)
    real={name:pe.image_base+rva if original else symbols[name] for name,(rva,size) in REAL.items()};entry='_append_normal_action' if family=='normal' else '_append_kind_action'
    allowed=case['count']<case['limit'];hit=(2 if kind&0x8000 else 0) if case['free'] else None;slot=None if not allowed or hit is None else POOL+hit*stride
    baseline=bytes(u.mem_read(POOL,len(payload)));group_before=bytes(u.mem_read(group-8,0x40));guard_before=bytes(u.mem_read(POOL-0x1000,0x1000));entered=Counter();reads=[];writes=[];events=[]
    def r32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def norm(value):return 'actor' if slot is not None and value==slot else value
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        if address==CALLBACK:
            assert slot is not None;esp=uc.reg_read(UC_X86_REG_ESP);assert r32(esp+4)==slot
            events.append(['callback',norm(slot),case['callback_count']])
            if case['callback_count'] is not None:uc.mem_write(group,pack(case['callback_count']))
            uc.reg_write(UC_X86_REG_EAX,0xface0000);uc.reg_write(UC_X86_REG_ECX,0xabad1dea);uc.reg_write(UC_X86_REG_EDX,0xdeadc0de)
            uc.reg_write(UC_X86_REG_EIP,r32(esp));uc.reg_write(UC_X86_REG_ESP,esp+4);return
        for name,at in real.items():
            if address==at:entered[name]+=1;events.append(['entry',name])
        assert any(at<=address and address+size<=at+REAL[name][1] for name,at in real.items()),hex(address)
    def read(uc,access,address,size,value,user):
        if STACK<=address and address+size<=STACK+0x10000:return
        if address in globals_by_address:
            name,width=globals_by_address[address];assert size==width and (name=='limit' or name.startswith(family+'.'));reads.append([name,size,int.from_bytes(uc.mem_read(address,size),'little')]);return
        if address==group and size==4:reads.append(['group.count',r32(group)]);return
        if POOL<=address and address+size<=POOL+4*stride:
            assert size==2 and (address-POOL-0x2e)%stride==0;reads.append(['kind',(address-POOL-0x2e)//stride]);return
        at=A+index*rowstride
        if at<=address and address+size<=at+rowstride:
            assert (address-at,size) in ([(0,4),(4,4),(8,4),(12,1)] if rowstride==16 else [(0,4),(6,1)]);reads.append(['descriptor',address-at,size]);return
        assert address==callback_row and size==4;reads.append(['callback.slot',ci,callback_stride])
    def write(uc,access,address,size,value,user):
        if STACK<=address and address+size<=SP:return
        if address in globals_by_address:
            name,width=globals_by_address[address];assert slot is not None and name==family+'.%x'%spec['roots'][0 if kind&0x8000 else 3] and size==2 and value==hit;writes.append([name,size,value]);return
        pc=uc.reg_read(UC_X86_REG_EIP)
        if real[entry]<=pc<real[entry]+96:
            assert slot is not None and size==4 and group-4<=address<=group+16;writes.append(['group',address-group,size,norm(value)]);events.append(['group.store',address-group,norm(value)]);return
        assert slot is not None and slot<=address and address+size<=slot+stride,(hex(address),size)
        writes.append(['actor',address-slot,size,value])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    coords=[2,0x80000000,0x7fffffff];frame=pack(STOP)+b''.join(pack(n) for n in [group,*coords,kind])+b'\x93'*24;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
    for reg,value in zip(NV,SEEDS):u.reg_write(reg,value)
    u.emu_start(real[entry],0,count=12000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4 and [u.reg_read(r) for r in NV]==SEEDS and u.reg_read(UC_X86_REG_EAX)==(0 if slot is None else slot)
    assert bytes(u.mem_read(SP,len(frame)))==frame,dict(case=case,frame=frame.hex(),actual=bytes(u.mem_read(SP,len(frame))).hex())
    assert bytes(u.mem_read(POOL-0x1000,0x1000))==guard_before,case
    final=bytes(u.mem_read(POOL,len(payload)));group_after=bytes(u.mem_read(group-8,0x40))
    if slot is None:
        assert final==baseline and group_after==group_before and not any(e[0]=='callback' for e in events) and not any(w[0]=='group' for w in writes)
        assert sum(entered.values())==(2 if allowed else 1)+int(allowed)
    else:
        count=case['callback_count'] if case['callback_count'] is not None else (coords[0] if case.get('alias_group_actor') else case['count']);increment=(count+1)&0xffffffff;offset=((increment*4+0x80000000)&0xffffffff)-0x80000000
        assert [w for w in writes if w[0]=='group']==[['group',0,4,increment],['group',offset,4,'actor']]
        assert r32(group+offset)==slot and r32(group)==(slot if offset==0 else increment)
        assert [e[0] for e in events].count('callback')==1 and events[-3:]==[['callback','actor',case['callback_count']],['group.store',0,increment],['group.store',offset,'actor']]
    for at,raw in initial.items():
        if slot is not None and at in globals_by_address and globals_by_address[at][0]==family+'.%x'%spec['roots'][0 if kind&0x8000 else 3]:assert bytes(u.mem_read(at,2))==struct.pack('<H',hit)
        else:assert bytes(u.mem_read(at,len(raw)))==raw
    assert bytes(u.mem_read(A+index*rowstride,len(descriptor)))==descriptor and r32(callback_row)==CALLBACK
    return dict(hit=None if slot is None else hit,reads=reads,writes=writes,events=events,entered=dict(entered),pool_sha256=digest(final),group_sha256=digest(group_after))


def cases():
    for family in SPECS:
        for kind in [0,0x8001,0xffff,0xdead8001]:
            for count in [-2,-1,0,1,2,0x7fffffff]:
                for limit in [0,2,65535]:
                    yield dict(pool=family,kind=kind,count=count,limit=limit,free=[],callback_count=None)
                    if count>=limit:continue
                    for mutation in [None,-2,-1,0,3,0x7fffffff,-0x80000000]:yield dict(pool=family,kind=kind,count=count,limit=limit,free=[0,2],callback_count=mutation)
        for kind in [1,0x8001]:
            for mutation in [None,-1,3,0x7fffffff]:yield dict(pool=family,kind=kind,count=0,limit=2,free=[0,2],callback_count=mutation,alias_group_actor=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/action_appenders_2026_10_25/differential');args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    built,candidate,symbols=build(out);native=PE(ORACLE);checks=[];calls=Counter()
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(candidate,symbols,case,False);assert a==b,dict(case=case,native=a,source=b);checks.append(dict(case=case,result=a));calls.update(a['entered'])
    assert all(digest(Path(p).read_bytes())==h for p,h in built['input_sha256'].items())
    raw=json.dumps(checks,sort_keys=True).encode();(out/'cases.json').write_bytes(raw+b'\n')
    report=dict(scope=__doc__,cases=len(checks),all_equal=True,real_invocations=dict(calls),modeled_callback_invocations=sum(e[0]=='callback' for c in checks for e in c['result']['events']),group_actor_alias_cases=sum(bool(c['case'].get('alias_group_actor')) for c in checks),cases_sha256=digest(raw),build=built,limits=['Callback execution is modeled and deliberately clobbers volatile registers; real callback bodies and their behavior are not established.','Explicit unsigned increment preserves32-bit wrap, including callback-induced INT_MAX/INT_MIN and count-slot alias cases.','All capacities are synthetic. The unused private retirement table definition never executes and proves no production registry extent.','Fresh complete instruction equality and independent references remain separate acceptance requirements.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k in ['cases','all_equal','real_invocations','modeled_callback_invocations','group_actor_alias_cases']}))


if __name__=='__main__':main()
