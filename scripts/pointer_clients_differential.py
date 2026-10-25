"""Finite unpatched clamp/animation/sine-client comparisons in Unicorn.

The frame selector is real source. RNG, interpreters, bounds, damage and removal
are explicitly modeled; no OS module or original game startup is performed.
Fixture extraction must preserve every selected complete COFF contribution.
"""
import argparse
from collections import Counter
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

from env import ROOT
from match import DEFAULT_TOOLCHAIN, compile_inputs, compile_source, digest, verify_toolchain
from coff import COFF
from masked import masked_compare
from pe import PE
from refinery_forms import function_text, replace_function
from storage_alias import alias_profile, wrapper_source
from title_link import read_map

sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
ORACLE_SHA='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
IDS=['eng1.native_code_target_0117b0','eng1.34570','eng1.398b0','eng1.3a550']
SERVICES={'_random_limit':(0x2c460,1), '_eng1_interpret_word_stream':(0x28820,1),
          '_eng1_interpret_signed_pairs':(0x283e0,1), '_eng1_2a710':(0x2a710,2),
          '_eng1_37e30':(0x37e30,1), '_remove_actor':(0x27260,1)}
DATA=0x61000000;P=DATA+0x100;A=DATA+0x300;O=DATA+0x500;B=DATA+0x700;C=DATA+0x900;FRAMES=DATA+0xc00
STACK=0x62000000;SP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
SEEDS=[0x12345678,0x23456789,0x3456789a,0x456789ab]
pack=lambda x:struct.pack('<I',x&0xffffffff)
signed=lambda x:((x&0xffffffff)^0x80000000)-0x80000000


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())==ORACLE_SHA
    state=json.loads((ROOT/'recovery.json').read_text())['functions']
    rows=[next(r for r in state if r['id']==i) for i in IDS]
    inputs={Path(__file__).resolve(),runtime,ORACLE,ROOT/'toolchains/manifest.json',ROOT/'evidence/owner_alias_bindings_2026_10_25.json'}
    bindings=json.loads((ROOT/'evidence/owner_alias_bindings_2026_10_25.json').read_text())
    known={n:r for f in bindings['functions'].values() for n,r in f['symbols'].items()}
    for row in rows:inputs.update(compile_inputs(ROOT/row['source'],row['flags'],DEFAULT_TOOLCHAIN))
    for name in ('_g_frame_owner_72860','_g_native_pointer_7217c'):
        record=known[name];owner=record['owner'];header,_=alias_profile(record,name,ROOT)
        inputs.add(header);inputs.update(compile_inputs(ROOT/owner['source'],owner['flags'],DEFAULT_TOOLCHAIN))
    for source,flags in [('calibration/title/title_sine.c',['/O2','/Dg_243e0=g_sine_table_5a848']),('src/shared/frame_state.c',['/O2'])]:
        inputs.update(compile_inputs(ROOT/source,flags,DEFAULT_TOOLCHAIN))
    pins={str(p):digest(p.read_bytes()) for p in inputs}
    out.mkdir(parents=True,exist_ok=True)
    objects=[];commands=[];checks=[]
    for source in sorted({r['source'] for r in rows}):
        selected=[r for r in rows if r['source']==source]
        flags=selected[0]['flags'];assert all(r['flags']==flags for r in selected)
        inputs.update(compile_inputs(ROOT/source,flags,DEFAULT_TOOLCHAIN))
        original,cmd=compile_source(ROOT/source,flags);commands.append(cmd);coff=COFF(original)
        text=(ROOT/source).read_text();keep={r['symbol'] for r in selected}
        for symbol in coff.symbols.values():
            if symbol['section']<=0 or not symbol['type']&0x20 or symbol['name'] in keep:continue
            name=symbol['name'];assert name.startswith('_') and '@' not in name
            body=function_text(text,name[1:]);declaration=body.split('{',1)[0].rstrip()+';'
            text=replace_function(text,name[1:],declaration)
        fixture=out/Path(source).name;fixture.write_text(text)
        pins[str(fixture)]=digest(fixture.read_bytes())
        extracted,cmd=compile_source(fixture,flags);commands.append(cmd);extracted_coff=COFF(extracted)
        assert {s['name'] for s in extracted_coff.symbols.values() if s['section']>0 and s['type']&0x20}==keep
        for row in selected:
            assert extracted_coff.function(row['symbol'])==coff.function(row['symbol']),row['id']
            check=masked_compare(extracted,row['symbol'],ORACLE,int(row['rva'],0),row['size'])
            assert check['masked_equal'],(row['id'],check)
            checks.append(dict(id=row['id'],complete_canonical_contribution_unchanged=True,diagnostic=check))
        objects.append(extracted)
    for name in ('_g_frame_owner_72860','_g_native_pointer_7217c'):
        record=known[name];owner=record['owner'];header,_=alias_profile(record,name,ROOT)
        inputs.add(header);inputs.update(compile_inputs(ROOT/owner['source'],owner['flags'],DEFAULT_TOOLCHAIN))
        wrapper=wrapper_source((ROOT/owner['source']).resolve(),header,ROOT,pointer_types=record['source_alias']['pointer_types'])
        pins[str(wrapper)]=digest(wrapper.read_bytes())
        obj,cmd=compile_source(wrapper,[*owner['flags'],'/WX']);objects.append(obj);commands.append(cmd)
    for source,flags in [('calibration/title/title_sine.c',['/O2','/Dg_243e0=g_sine_table_5a848']),('src/shared/frame_state.c',['/O2'])]:
        inputs.update(compile_inputs(ROOT/source,flags,DEFAULT_TOOLCHAIN))
        obj,cmd=compile_source(ROOT/source,flags);objects.append(obj);commands.append(cmd)
        if source.endswith('frame_state.c'):
            check=masked_compare(obj,'_select_frame',ORACLE,0x292f0,48);assert check['masked_equal']
        else:
            coff=COFF(obj);sym,=[s for s in coff.symbols.values() if s['name']=='_g_sine_table_5a848' and s['section']>0]
            payload=coff.sections[sym['section']-1]['data'][sym['value']:sym['value']+2048]
            assert payload==PE(ORACLE).read_rva(0x5a848,2048)
    services=out/'services.c'
    services.write_text('/* Hooked synthetic fixture services, never production. */\nint random_limit(int n){for(;;){}}\nvoid eng1_interpret_word_stream(void*p){for(;;){}}\nvoid eng1_interpret_signed_pairs(void*p){for(;;){}}\nint eng1_2a710(void*b,void*p){for(;;){}}\nvoid eng1_37e30(int n){for(;;){}}\nint remove_actor(void*p){for(;;){}}\n')
    pins[str(services)]=digest(services.read_bytes())
    obj,cmd=compile_source(services,['/O2']);objects.append(obj);commands.append(cmd)
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile)]
    command += ['/EXPORT:'+r['symbol'][1:] for r in rows]+list(map(str,objects))
    env=os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    ran=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    symbols=read_map(mapfile)
    assert len({symbols[name] for name in SERVICES})==len(SERVICES),'Modeled services must retain distinct entries'
    assert all(str(p) in pins for p in inputs)
    assert all(digest(Path(p).read_bytes())==sha for p,sha in pins.items())
    return dict(input_sha256=pins,commands=commands+[command],extraction_checks=checks,
                object_sha256=[digest(p.read_bytes()) for p in objects],image_sha256=digest(image.read_bytes())),PE(image),symbols,rows


def execute(pe,symbols,row,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(DATA,0x2000);u.mem_map(STACK,0x10000)
    u.mem_write(DATA,bytes((i*13+31)&255 for i in range(0x2000)))
    entry=pe.image_base+int(row['rva'],0) if original else symbols[row['symbol']]
    normal=pe.image_base+0x72860 if original else symbols['_g_frame_owner_72860']
    collision=pe.image_base+0x7217c if original else symbols['_g_native_pointer_7217c']
    table=pe.image_base+0x5a848 if original else symbols.get('_g_sine_table_5a848',0)
    select=pe.image_base+0x292f0 if original else symbols['_select_frame']
    service_at={pe.image_base+rva if original else symbols[name]:name for name,(rva,n) in SERVICES.items()}
    cells={normal:'normal',collision:'collision'}
    def w32(at,value):u.mem_write(at,pack(value))
    def w16(at,value):u.mem_write(at,struct.pack('<H',value&0xffff))
    def r32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    w32(normal,A);w32(collision,A if not case.get('null_collision') else 0)
    for at in (P,A,O,B,C):
        for off,value in [(0,0x100000),(4,0x200000),(8,case.get('z',0)),(0x108,case.get('counter',0)),(0x110,FRAMES),(0x128,O)]:w32(at+off,value)
        w16(at+0xc,case.get('screen_x',21));w16(at+0xe,case.get('screen_y',32));w16(at+0x10,3);w16(at+0x3e,4)
        w16(at+0x2a,case.get('state',0));w16(at+0x72,case.get('owner_angle',0));w16(at+0x74,case.get('angle',0)+8)
        u.mem_write(at+0x1f,bytes([case.get('status',2)]))
    w32(A,case.get('clamp_x',0x100000));w32(A+8,case.get('target_z',0))
    w32(C+8,case.get('target_z',0));w16(C+0xc,99)
    w32(P+0x10c,case.get('script',0));w16(O+0x2e,case.get('kind',0x3e));w32(O+0x108,case.get('magic',0x12345675))
    for i in range(32):w32(FRAMES+4*i,DATA+0x1000+i*16)
    frame=pack(STOP)+pack(P)+b'\x93'*32;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
    for reg,seed in zip(NV,SEEDS):u.reg_write(reg,seed)
    transcript=[];writes=[];reads=[]
    def ret(value=0):
        sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,r32(sp));u.reg_write(UC_X86_REG_ESP,sp+4)
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xaabbccdd);u.reg_write(UC_X86_REG_EDX,0x99887766)
    def code(uc,at,size,user):
        if at==STOP:uc.emu_stop();return
        sp=uc.reg_read(UC_X86_REG_ESP)
        if at==select:
            args=[r32(sp+4),r32(sp+8)];assert args[0]==P and 0<=args[1]<32
            transcript.append(['select_frame',*args]);return
        if at in service_at:
            name=service_at[at];args=[r32(sp+4+4*n) for n in range(SERVICES[name][1])]
            transcript.append([name,*args])
            if name=='_random_limit':
                assert args==[40]
                if case.get('rng_mutate'):
                    w32(collision,C);w16(P+0x2a,3)
                    if case.get('flip_status'):u.mem_write(P+0x1f,bytes([case.get('status',2)^2]))
                ret(case.get('rng',0));return
            if name in ('_eng1_interpret_word_stream','_eng1_interpret_signed_pairs'):
                assert args==[P]
                if case.get('mutate_roots'):w32(normal,B);w32(collision,C)
                if 'after_state' in case:w16(P+0x2a,case['after_state'])
                if 'after_counter' in case:w32(P+0x108,case['after_counter'])
                if 'after_angle' in case:w16(P+0x74,case['after_angle'])
                ret();return
            if name=='_eng1_2a710':
                assert args==[r32(collision)+0x88,P];ret(case.get('bounds',0));return
            if name=='_eng1_37e30':
                assert args==[2]
                if 'damage_counter' in case:w32(P+0x108,case['damage_counter'])
                ret();return
            if name=='_remove_actor':assert args==[P];ret(case.get('remove_result',1));return
        assert entry<=at and at+size<=entry+row['size'] or select<=at and at+size<=select+48,hex(at)
    def read(uc,access,at,size,value,user):
        assert DATA<=at and at+size<=DATA+0x2000 or STACK<=at and at+size<=STACK+0x10000 or at in cells and size==4 or table<=at and at+size<=table+2048,hex(at)
        if at in cells:reads.append([cells[at],size,r32(at)])
        elif table<=at and at+size<=table+2048:reads.append(['sine',at-table,size,bytes(uc.mem_read(at,size)).hex()])
    def write(uc,access,at,size,value,user):
        assert DATA<=at and at+size<=DATA+0x2000 or STACK<=at and at+size<=SP or at in cells and size==4,hex(at)
        if DATA<=at<DATA+0x2000:writes.append([at-DATA,size,value&((1<<(size*8))-1)])
        elif at in cells:writes.append([cells[at],size,value&0xffffffff])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.emu_start(entry,STOP+1,count=2000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4
    assert bytes(u.mem_read(SP,len(frame)))==frame and [u.reg_read(r) for r in NV]==SEEDS
    return dict(data_sha256=digest(bytes(u.mem_read(DATA,0x2000))),normal=r32(normal),collision=r32(collision),
                calls=transcript,ordered_stores=writes,root_and_sine_reads=reads,state=struct.unpack('<H',u.mem_read(P+0x2a,2))[0])


def cases(pe):
    for x in [0x80000000,0xffffffff,0,1,0x29fdffff,0x29fe0000,0x29fe0001,0x7fffffff]:yield IDS[0],dict(clamp_x=x)
    for rng,x,dz,status,mutate in itertools.product([-1,0,9,10,14,15,24,25,27,28,37,38,39,40,0x80000000,0x7fffffff],[20,21,299,300],[-1,32,33],[0,2],[False,True]):
        yield IDS[1],dict(rng=rng,screen_x=x,target_z=dz<<16,status=status,rng_mutate=mutate,flip_status=mutate)
    for status in [0,2,255]:yield IDS[1],dict(state=4,status=status,null_collision=True)
    sine=struct.unpack('<1024h',pe.read_rva(0x5a848,2048))
    trunc=lambda n: (-1 if n<0 else 1)*(abs(n)//256)
    for angle in range(1024):
        sx=(trunc(sine[angle]*0xb400)>>16)+14
        sy=(trunc(-sine[(angle+256)&1023]*0xb400)>>16)+32
        yield IDS[2],dict(angle=angle,screen_x=sx,screen_y=sy,counter=[-1,0,1,0x7fffffff,0x80000000][angle%5],damage_counter=angle%3-1)
    for angle,mutate,count in itertools.product([-32768,-1,0,255,256,1023,32767],[False,True],[-1,0,1,0x7fffffff,0x80000000]):
        yield IDS[2],dict(angle=angle,mutate_roots=mutate,after_counter=count,after_angle=angle+1,screen_x=500,screen_y=-500)
    for owner_angle,script,state,bounds,mutate in itertools.product([-32768,-1,0,255,256,1023,32767],[0,1,0x80000000,0xffffffff],[0,2],[0,1],[False,True]):
        yield IDS[3],dict(owner_angle=owner_angle,script=script,after_state=state,bounds=bounds,mutate_roots=mutate)
    for kind,magic in itertools.product([0,0x3d,0x3e,0x403e,0xffff],[0,0x12345674,0x12345675,0x12345676]):
        yield IDS[3],dict(kind=kind,magic=magic,state=2,bounds=1)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'work/pointer_clients_2026_10_25/differential')
    out=parser.parse_args().output.resolve();assert out.is_relative_to(ROOT/'work')
    built,source,symbols,rows=build(out);original=PE(ORACLE);byid={r['id']:r for r in rows}
    summary=Counter();coverage=Counter();transcripts=[]
    for fid,case in cases(original):
        row=byid[fid];expected=execute(original,{},row,case,True);actual=execute(source,symbols,row,case,False)
        assert actual==expected,dict(id=fid,case=case,original=expected,source=actual)
        summary[fid]+=1
        for call in actual['calls']:coverage[(call[0],call[-1] if call[0]=='select_frame' else 0)]+=1
        transcripts.append(dict(id=fid,case=case,result=actual))
    assert all(coverage[('select_frame',i)] for i in (0,6,11,16))
    assert coverage[('_eng1_37e30',0)] and coverage[('_remove_actor',0)] and coverage[('_eng1_2a710',0)]
    assert all(digest(Path(p).read_bytes())==sha for p,sha in built['input_sha256'].items())
    report=dict(scope=__doc__,all_equal=True,cases=sum(summary.values()),by_entry=dict(summary),
                build=built,coverage={str(k):v for k,v in coverage.items()},cases_sha256=digest(json.dumps(transcripts,sort_keys=True).encode()),
                limits=['Real frame selector composed; RNG, script interpreters, bounds, damage and removal modeled. Guarded finite data is synthetic; native capacities and full dependency behavior remain unproved.','Complete extracted contributions are identical to fresh canonical objects before ordinary linking. No instruction patches or original OS startup.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('all_equal','cases','by_entry','coverage')},indent=2))


if __name__=='__main__':main()
