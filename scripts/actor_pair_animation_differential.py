"""Isolated actor-pair initialization/update/release with real reset instructions.

WORD interpretation and middle removal are explicit mutation models. No original
OS module starts and no original instruction/file is modified.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN,compile_inputs,compile_source,compare,digest,verify_toolchain
from masked import masked_compare
from pe import PE
from title_link import read_map
import boss_entries_differential as base
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX

SOURCE=ROOT/'src/eng1/behavioral/actor_pair_animation.c'
WORDS=ROOT/'src/eng1/behavioral/frame_data_config_word_pool_55478.c'
REAL={'_eng1_native_code_target_001a280':(0x1a280,48),'_eng1_native_code_target_01a2b0':(0x1a2b0,224),'_eng1_native_code_target_01a390':(0x1a390,80),'_reset_frame_state':(0x29260,16)}
ROOTS={'_g_eng1_setup_pointer_72018':0x72018,'_g_eng1_setup_pointer_72164':0x72164,'_g_collision_context':0x7217c,'_g_normal_actor_pool':0x72860}
ARENA=0x61000000;A=ARENA+0x1000;B=ARENA+0x2000;C=ARENA+0x3000;N=ARENA+0x4000


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN);library=Path(base.unicorn.__file__).parent/'lib/unicorn.dll'
    assert base.unicorn.__version__=='2.1.4' and digest(library.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(base.ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    sources=[SOURCE,ROOT/'src/shared/frame_state.c',WORDS,*[ROOT/'src/eng1/behavioral'/s for s in ['actor_storage.c','actor_control_cells.c','setup_shared_state.c']]]
    pins={str(p):digest(p.read_bytes()) for p in [base.ORACLE,library,Path(__file__).resolve(),Path(base.__file__),ROOT/'toolchains/manifest.json']};objects=[];commands=[];checks={};owners={}
    for source in sources:
        pins.update({str(p):h for p,h in compile_inputs(source,['/O2'],DEFAULT_TOOLCHAIN).items()});obj,command=compile_source(source,['/O2']);objects.append(str(obj));commands.append(command);coff=COFF(obj)
        if source==SOURCE:
            for name,(rva,size) in REAL.items():
                if name=='_reset_frame_state':continue
                checks[name]=masked_compare(obj,name,base.ORACLE,rva,size);assert checks[name]['masked_equal']
        elif source.name=='frame_state.c':assert compare(obj,'_reset_frame_state',base.ORACLE,0x29260,16)['exact']
        elif source==WORDS:
            sym,=[s for s in coff.symbols.values() if s['name']=='_g_eng1_config_word_pool_55478'];sec=coff.sections[sym['section']-1];payload=sec['data'];assert sym['value']==0 and len(payload)==192 and not sec['relocations'];pe=PE(base.ORACLE);assert pe.data.count(payload)==1 and pe.pe.get_rva_from_offset(pe.data.find(payload))==0x55478
        else:
            from zero_storage import verify_common
            for name in ROOTS:
                if any(s['name']==name and s['value']==4 for s in coff.symbols.values()):owners[name]=dict(source=source.relative_to(ROOT).as_posix(),common=verify_common(obj,name,4),object_sha256=digest(obj.read_bytes()))
    assert set(owners)==set(ROOTS)
    model=out/'models.c';model.write_text('/* Interception endpoints only, never production or executed. */\nint eng1_interpret_word_stream(void*p){for(;;){}}\nvoid remove_middle_actor(void*p){for(;;){}}\n');pins[str(model)]=digest(model.read_bytes());obj,command=compile_source(model,['/O2']);objects.append(str(obj));commands.append(command)
    image,mapfile=out/'fixture.dll',out/'fixture.map';command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),*['/EXPORT:'+n[1:] for n in REAL if n!='_reset_frame_state'],*objects];commands.append(command);env=os.environ.copy()
    for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');linked=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True,timeout=60);(out/'link.log').write_text(linked.stdout+linked.stderr);assert linked.returncode==0,linked.stdout+linked.stderr
    return dict(input_sha256=pins,commands=commands,checks=checks,owners=owners,word_payload_sha256=digest(payload),image_sha256=digest(image.read_bytes())),PE(base.ORACLE),PE(image),read_map(mapfile)


def execute(pe,symbols,case,original):
    u=base.Uc(base.UC_ARCH_X86,base.UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff;u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'));u.mem_map(ARENA,0x6000);u.mem_map(base.STACK,0x10000)
    real={name:pe.image_base+rva if original else symbols[name] for name,(rva,_) in REAL.items()};cells={pe.image_base+rva if original else symbols[name]:name for name,rva in ROOTS.items()};at={name:addr for addr,name in cells.items()};word=pe.image_base+0x28820 if original else symbols['_eng1_interpret_word_stream'];remove=pe.image_base+0x27860 if original else symbols['_remove_middle_actor'];words=pe.image_base+0x55478 if original else symbols['_g_eng1_config_word_pool_55478']
    raw=bytearray((i*37+case['seed'])&255 for i in range(0x6000));edges=[0,0x7fffffff,0x80000000]
    for j,actor in enumerate([A,B,C,N]):
        offset=actor-ARENA;struct.pack_into('<III',raw,offset,edges[(j+case['xy'])%3],edges[(j+case['xy']+1)%3],0xf0000000+j);struct.pack_into('<H',raw,offset+0x10,case['w10']);struct.pack_into('<H',raw,offset+0x2e,case['kind']);struct.pack_into('<H',raw,offset+0x34,case['frame']);raw[offset+0x94]=0x17+j
    struct.pack_into('<I',raw,A-ARENA+0x64,words+48);u.mem_write(ARENA,bytes(raw))
    initial={'_g_eng1_setup_pointer_72018':A if case['same'] else B,'_g_eng1_setup_pointer_72164':B,'_g_collision_context':A if case['collision_alias'] else B,'_g_normal_actor_pool':A if case['normal_alias'] else N}
    for name,value in initial.items():u.mem_write(at[name],base.pack(value))
    frame=base.pack(base.STOP)+(base.pack(A) if case['mode']!='release' else b'')+b'\x93'*24;u.mem_write(base.SP,frame);u.reg_write(base.UC_X86_REG_ESP,base.SP)
    for reg,value in zip(base.NV,base.SEEDS):u.reg_write(reg,value)
    calls=[];writes=[];reads=[];root_reads=[];data_reads=[];events=[];removals=[];entry=real[{'release':'_eng1_native_code_target_001a280','init':'_eng1_native_code_target_01a2b0','update':'_eng1_native_code_target_01a390'}[case['mode']]]
    r32=lambda addr:struct.unpack('<I',u.mem_read(addr,4))[0]
    def return_model(result):
        esp=u.reg_read(base.UC_X86_REG_ESP);destination=r32(esp)
        for reg,value in [(base.UC_X86_REG_EAX,result),(UC_X86_REG_ECX,0xabad1dea),(UC_X86_REG_EDX,0xdeadc0de)]:u.reg_write(reg,value)
        u.reg_write(base.UC_X86_REG_ESP,esp+4);u.reg_write(base.UC_X86_REG_EIP,destination)
    def code(uc,address,size,user):
        if address==base.STOP:uc.emu_stop();return
        if address in real.values():calls.append(next(name for name,value in real.items() if value==address))
        if address==word:
            esp=uc.reg_read(base.UC_X86_REG_ESP);assert r32(esp+4)==A;calls.append('word_model')
            if case['mode']=='init':
                expected=words+(48 if case['same'] else 72);assert r32(A+0x64)==expected and bytes(uc.mem_read(A+0x30,4))==b'\0'*4
            else:
                index=min(9,max(0,case['frame']-351));assert int.from_bytes(uc.mem_read(A+0x72,2),'little')==int.from_bytes(uc.mem_read(words+96+index*2,2),'little')
            uc.mem_write(A+0x50,base.pack(r32(A+0x50)^0x40));uc.mem_write(A+0x54,base.pack(r32(A+0x54)^0x8000));uc.mem_write(A+0x34,struct.pack('<H',(case['frame']+1)&0xffff));events.append(['word_mutation',case['result']])
            if case['rotate']:
                uc.mem_write(at['_g_collision_context'],base.pack(C));uc.mem_write(at['_g_eng1_setup_pointer_72018'],base.pack(C));events.append(['rotated_collision_and_first',C-ARENA])
            return_model(case['result']);return
        if address==remove:
            esp=uc.reg_read(base.UC_X86_REG_ESP);actor=r32(esp+4);assert actor in [A,B,C,N];calls.append('remove_model');removals.append(actor-ARENA);uc.mem_write(actor+0x2e,b'\0\0');uc.mem_write(actor+0x50,base.pack(r32(actor+0x50)^0x80));events.append(['removed',actor-ARENA])
            if len(removals)==1:
                uc.mem_write(at['_g_eng1_setup_pointer_72018'],base.pack(N))
                if case['redirect']:uc.mem_write(at['_g_eng1_setup_pointer_72164'],base.pack(C));events.append(['redirected_second',C-ARENA])
            return_model(0x12340000+len(removals));return
        assert any(addr<=address and address+size<=addr+REAL[name][1] for name,addr in real.items()),hex(address)
    def read(uc,access,address,size,value,user):
        if base.STACK<=address and address+size<=base.STACK+0x10000:return
        if address in cells:assert size==4;root_reads.append([cells[address],r32(address)-ARENA]);return
        if ARENA<=address and address+size<=ARENA+0x6000:
            value=int.from_bytes(uc.mem_read(address,size),'little');reads.append([address-ARENA,size,value]);return
        assert words<=address and address+size<=words+192 and size==2;data_reads.append([address-words,size,int.from_bytes(uc.mem_read(address,size),'little')])
    def write(uc,access,address,size,value,user):
        if base.STACK<=address and address+size<=base.SP:return
        assert any(actor<=address and address+size<=actor+0x134 for actor in [A,B,C,N]),hex(address)
        if address==A+0x64:assert value in [words+48,words+72] and size==4;value='left_program' if value==words+48 else 'right_program'
        writes.append([address-ARENA,size,value])
    u.hook_add(base.UC_HOOK_CODE,code);u.hook_add(base.UC_HOOK_MEM_READ,read);u.hook_add(base.UC_HOOK_MEM_WRITE,write);u.emu_start(entry,0,count=3000)
    assert u.reg_read(base.UC_X86_REG_EIP)==base.STOP and u.reg_read(base.UC_X86_REG_ESP)==base.SP+4 and bytes(u.mem_read(base.SP,len(frame)))==frame and [u.reg_read(r) for r in base.NV]==base.SEEDS
    expected_removals=2 if case['kind'] and (case['mode']=='release' or case['mode']=='update' and case['result']==1) else 0;assert len(removals)==expected_removals
    if case['mode']=='update':assert data_reads==[[96+min(9,max(0,case['frame']-351))*2,2,data_reads[0][2]]]
    else:assert not data_reads
    memory=bytearray(u.mem_read(ARENA,0x6000));assert memory[:0x1000]==raw[:0x1000] and memory[0x4134:]==raw[0x4134:]
    pointer=r32(A+0x64);assert pointer in [words+48,words+72];memory[A-ARENA+0x64:A-ARENA+0x68]=base.pack(pointer-words)
    return dict(memory_sha256=digest(memory),ordered_writes=writes,actor_reads=reads,root_reads=root_reads,data_reads=data_reads,calls=calls,model_events=events,removals=removals,roots={name:r32(addr)-ARENA for name,addr in at.items()},eax=u.reg_read(base.UC_X86_REG_EAX))


def cases():
    default=dict(seed=0,xy=0,w10=0,frame=351,kind=1,same=True,collision_alias=False,normal_alias=False,rotate=False,redirect=False,result=0)
    for kind,redirect,seed in itertools.product([0,1,65535],[False,True],[0,79]):yield dict(default,mode='release',kind=kind,redirect=redirect,seed=seed)
    for same,collision,normal,xy,w10,rotate,result,seed in itertools.product([False,True],[False,True],[False,True],range(3),[0,1,65535,65504],[False,True],[0,1,0xffffffff],[0,79]):yield dict(default,mode='init',same=same,collision_alias=collision,normal_alias=normal,xy=xy,w10=w10,rotate=rotate,result=result,seed=seed)
    frames=list(range(351,361))+[0,1,350,361,362,32767,65535]
    for frame,result,kind,rotate,redirect,normal,seed in itertools.product(frames,[0,1,2,0xffffffff],[0,1,65535],[False,True],[False,True],[False,True],[0,79]):yield dict(default,mode='update',frame=frame,result=result,kind=kind,rotate=rotate,redirect=redirect,normal_alias=normal,seed=seed)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'work/actor_pair_animation_differential');args=parser.parse_args();out=args.output.resolve()
    if not out.is_relative_to(ROOT/'work'):parser.error('Fixture output belongs under ignored work/')
    out.mkdir(parents=True,exist_ok=True);built,native,source,symbols=build(out);checks=[]
    for case in cases():
        a=execute(native,symbols,case,True);b=execute(source,symbols,case,False);checks.append(dict(case=case,native=a,source=b,equal=a==b))
    for path,sha in built['input_sha256'].items():assert digest(Path(path).read_bytes())==sha
    raw=json.dumps(checks,sort_keys=True).encode()+b'\n';(out/'cases.json').write_bytes(raw);report=dict(scope=__doc__,cases=len(checks),by_mode={mode:sum(c['case']['mode']==mode for c in checks) for mode in ['release','init','update']},all_equal=all(c['equal'] for c in checks),failed_cases=sum(not c['equal'] for c in checks),cases_sha256=digest(raw),build=built,limits=['WORD/removal models test caller contracts and root reloads, not those helpers complete semantics.','Actor/pool pointers refer to guarded synthetic storage; no original allocation-capacity claim.','Role normalization applies only to the independently verified WORD-program pointer; raw values are checked first.']);(out/'report.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n');print(json.dumps({k:report[k] for k in ['cases','by_mode','all_equal','failed_cases']}))
    if not report['all_equal']:raise SystemExit(1)


if __name__=='__main__':main()
