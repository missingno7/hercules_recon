"""Unique player WORD/pair loops and unpatched native/source consumers.

Only positive frame records plus restart are covered by the pair profile.
The real pair/WORD interpreters and reset helper execute. Unsupported services
are fixture traps; no original OS module/game starts.
"""
import argparse
from collections import Counter
import json,os,re
from pathlib import Path
import struct,subprocess,sys
from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,compile_inputs,digest,verify_toolchain
from coff import COFF
from masked import masked_compare
from pe import PE
from refinery_forms import function_text,replace_function
from storage_alias import alias_profile,wrapper_source
from title_link import read_map
from word_program_differential import loop_profile,same_local_targets
sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL';ORACLE_SHA='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
SOURCE='src/eng1/behavioral/player_animation_programs.c';INTERPRETER='calibration/eng1/actor_unit_282e0.cpp'
SPECS={'_g_player_program_633e8':('pair',36),'_g_player_program_634b0':('word',38),'_g_player_program_634e0':('word',18)}
REAL={'_eng1_interpret_signed_pairs':(0x283e0,1088),'_eng1_interpret_word_stream':(0x28820,640),'_reset_frame_state':(0x29260,16)}
DATA=0x61000000;P=DATA+0x100;N=DATA+0x300;G=DATA+0x500;A=DATA+0x700;B=DATA+0x900
STACK=0x62000000;SP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x12345678,0x23456789,0x3456789a,0x456789ab]
pack=lambda n:struct.pack('<I',n&0xffffffff)


def pair_profile(words):
    if not isinstance(words,(list,tuple)) or len(words)<4 or len(words)%2 or any(type(n)is not int or not 0<=n<=65535 for n in words):raise ValueError('Complete fixed-stride pairs required')
    frames=[]
    for index in range(len(words)//2):
        opcode=words[index*2]
        if opcode<0x8000:frames.append(index)
        elif opcode==0xffff:
            if index!=len(words)//2-1 or not frames:raise ValueError('Restart must end a nonempty frame loop')
            return dict(records=len(words)//2,bytes=len(words)*2,frame_indices=frames,restart_index=index,stable_return_states=[-1,*frames],frame_ticks=[words[2*i+1] if words[2*i+1]<0x8000 else words[2*i+1]-65536 for i in frames])
        else:raise ValueError('Pair opcode outside frame/restart profile')
    raise ValueError('No terminal pair restart')


def audit_programs():
    verify_toolchain(DEFAULT_TOOLCHAIN);pe=PE(ORACLE);assert digest(pe.data)==ORACLE_SHA
    context=compile_inputs(ROOT/SOURCE,['/O2'],DEFAULT_TOOLCHAIN);obj,command=compile_source(ROOT/SOURCE,['/O2']);coff=COFF(obj);programs={}
    for name,(mode,size) in SPECS.items():
        sym,=[s for s in coff.symbols.values() if s['name']==name and s['section']>0];sec=coff.sections[sym['section']-1];at=sym['value'];raw=sec['data'][at:at+size]
        assert len(raw)==size and not [r for r in sec['relocations'] if at<=r['offset']<at+size]
        words=list(struct.unpack('<'+'H'*(size//2),raw));profile=pair_profile(words) if mode=='pair' else loop_profile(words)
        positions=[]
        for section in pe.pe.sections:
            data=section.get_data();pos=data.find(raw)
            while pos>=0:positions.append(section.VirtualAddress+pos);pos=data.find(raw,pos+1)
        assert len(positions)==1,(name,positions)
        programs[name]=dict(mode=mode,rva=positions[0],size=size,sha256=digest(raw),profile=profile,unique_native_positions=positions)
    assert context==compile_inputs(ROOT/SOURCE,['/O2'],DEFAULT_TOOLCHAIN)
    return dict(scope=__doc__,oracle_sha256=ORACLE_SHA,programs=programs,command=command,object_sha256=digest(obj.read_bytes()),compile_inputs={str(p):h for p,h in context.items()})


def build(out,binding_path,audit):
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll';assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    bindings=json.loads(binding_path.read_text());state=json.loads((ROOT/'recovery.json').read_text())['functions'];row=next(r for r in state if r['id']=='eng1.37390')
    known=bindings['functions'][row['id']]['symbols'];inputs={Path(__file__).resolve(),ROOT/'scripts/word_program_differential.py',runtime,ORACLE,binding_path,ROOT/'toolchains/manifest.json'}
    owners={}
    for name,rec in known.items():
        if rec['kind']!='bss':continue
        owner=rec['owner'];key=(owner['source'],tuple(owner['flags']),rec.get('source_alias',{}).get('header'));owners.setdefault(key,(name,rec));inputs.update(compile_inputs(ROOT/owner['source'],owner['flags'],DEFAULT_TOOLCHAIN))
        if 'source_alias'in rec:header,_=alias_profile(rec,name,ROOT);inputs.add(header)
    for source in [SOURCE,INTERPRETER,row['source'],'src/shared/frame_state.c']:inputs.update(compile_inputs(ROOT/source,['/O2'],DEFAULT_TOOLCHAIN))
    pins={str(p):digest(p.read_bytes()) for p in inputs};objects=[];commands=[];checks=[];out.mkdir(parents=True,exist_ok=True)
    for source,keep in [(row['source'],{row['symbol']}),(INTERPRETER,{'_eng1_interpret_signed_pairs','_eng1_interpret_word_stream'})]:
        full,cmd=compile_source(ROOT/source,['/O2']);commands.append(cmd);original=COFF(full);text=(ROOT/source).read_text()
        if source==INTERPRETER:
            # Removing other bodies changes pair-interpreter instruction bytes.
            # Keep the original TU; any unrelated linked entry remains forbidden.
            objects.append(full)
            for name,(rva,size) in REAL.items():
                check=masked_compare(full,name,ORACLE,rva,size);assert check['masked_equal'],(name,check)
                checks.append(dict(symbol=name,complete_bytes_and_targets_preserved=True,full_canonical_unit=True,diagnostic=check))
            continue
        for sym in original.symbols.values():
            if sym['section']<=0 or not sym['type']&0x20 or sym['name']in keep:continue
            name=sym['name'][1:];body=function_text(text,name);text=replace_function(text,name,body.split('{',1)[0].rstrip()+';')
        fixture=out/Path(source).name;fixture.write_text(text);pins[str(fixture)]=digest(fixture.read_bytes());obj,cmd=compile_source(fixture,['/O2']);objects.append(obj);commands.append(cmd);coff=COFF(obj)
        for name in keep:
            labels=same_local_targets(original,coff,name)
            rva,size=(int(row['rva'],0),row['size']) if name==row['symbol'] else REAL[name]
            diagnostic=masked_compare(obj,name,ORACLE,rva,size);assert diagnostic['masked_equal'],(name,diagnostic)
            checks.append(dict(symbol=name,complete_bytes_and_targets_preserved=True,local_label_renumberings=labels,diagnostic=diagnostic))
    for (source,flags,header_name),(name,rec) in owners.items():
        src=ROOT/source;flags=list(flags)
        if header_name:
            header,_=alias_profile(rec,name,ROOT);src=wrapper_source(src.resolve(),header,ROOT,pointer_types=rec['source_alias']['pointer_types']);pins[str(src)]=digest(src.read_bytes());flags+=['/WX']
        obj,cmd=compile_source(src,flags);objects.append(obj);commands.append(cmd)
    for source in [SOURCE]:
        obj,cmd=compile_source(ROOT/source,['/O2']);objects.append(obj);commands.append(cmd)
    services=out/'services.c'
    services.write_text('/* Unreached fixture traps and storage only, never production. */\nvoid *g_configuration_dword_71384;\nunsigned long g_kind_info;\nvoid play_sound(int s,void*p){for(;;){}}\nint remove_actor(void*p){for(;;){}}\nvoid remove_actor_preserving_resource(void*p){for(;;){}}\nvoid mark_actor_resource_for_release(void*p){for(;;){}}\nvoid stop_actor_sounds(void*p){for(;;){}}\nvoid title_17ad0(int x,int y,int z,int k,int g){for(;;){}}\n')
    pins[str(services)]=digest(services.read_bytes());obj,cmd=compile_source(services,['/O2']);objects.append(obj);commands.append(cmd)
    provided={s['name'] for obj in objects for s in COFF(obj).symbols.values() if s['section']>0 or s['section']==0 and s['value']>0}
    missing={}
    for obj in objects:
        for sym in COFF(obj).symbols.values():
            if sym['storage']==2 and sym['section']==0 and sym['value']==0 and sym['name'] not in provided:
                assert re.fullmatch(r'_[A-Za-z_]\w*',sym['name'])
                kind=bool(sym['type']&0x20)
                assert sym['name'] not in missing or missing[sym['name']]==kind
                missing[sym['name']]=kind
    traps=out/'link_traps.c'
    traps.write_text('/* Unreached linker definitions only. Runtime guards deny every entry/read. */\n'+''.join('void '+n[1:]+'(void){for(;;){}}\n' if fn else 'unsigned long '+n[1:]+';\n' for n,fn in sorted(missing.items())))
    pins[str(traps)]=digest(traps.read_bytes());obj,cmd=compile_source(traps,['/O2']);objects.append(obj);commands.append(cmd)
    image=out/'fixture.dll';mapfile=out/'fixture.map';cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),'/EXPORT:eng1_37390','/EXPORT:eng1_interpret_signed_pairs','/EXPORT:eng1_interpret_word_stream',*map(str,objects)];commands.append(cmd)
    env=os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_']:env.pop(name,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','');ran=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    assert all(digest(Path(p).read_bytes())==h for p,h in pins.items())
    return dict(input_sha256=pins,commands=commands,checks=checks,unreached_link_traps={n:'function' if fn else 'data' for n,fn in missing.items()},image_sha256=digest(image.read_bytes()),object_sha256=[digest(p.read_bytes()) for p in objects]),PE(image),read_map(mapfile),known


def execute(pe,symbols,known,programs,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff;u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'));u.mem_map(DATA,0x2000);u.mem_map(STACK,0x10000)
    u.mem_write(DATA,bytes((i*13+31)&255 for i in range(0x2000)))
    at={name:pe.image_base+p['rva'] if original else symbols[name] for name,p in programs.items()}
    cell_at={name:pe.image_base+rec['rva'] if original else symbols[name] for name,rec in known.items() if rec['kind']=='bss'};cells={v:k for k,v in cell_at.items()}
    real={name:pe.image_base+rva if original else symbols[name] for name,(rva,size) in REAL.items()};client=pe.image_base+0x37390 if original else symbols['_eng1_37390']
    def norm(value):
        for name,start in at.items():
            if start<=value<start+programs[name]['size']:return [name,value-start]
        return value
    def w32(addr,n):u.mem_write(addr,pack(n))
    def w16(addr,n):u.mem_write(addr,struct.pack('<H',n&0xffff))
    def r32(addr):return struct.unpack('<I',u.mem_read(addr,4))[0]
    w32(cell_at['_g_frame_owner_72860'],N);w32(cell_at['_g_player_glow_707dc'],G)
    for index,actor in enumerate([P,N,G,A,B]):
        for off,value in [(0,0x100000+index),(4,0x200000-index),(8,0x300000+index),(0x64,at[case['program']])]:w32(actor+off,value)
        w16(actor+0x30,case.get('ticks',0));w16(actor+0x32,case.get('cursor',0));w16(actor+0x2a,case.get('mode',0));w32(actor+0x50,0x10203040)
    if case.get('alias_glow'):w32(cell_at['_g_player_glow_707dc'],P)
    if case.get('alias_normal'):w32(cell_at['_g_frame_owner_72860'],P)
    reads=[];writes=[];returns=[];entered=Counter()
    def code(uc,address,size,user):
        if address==STOP:uc.emu_stop();return
        for name,start in real.items():
            if address==start:entered[name]+=1
        assert address>=client and address+size<=client+112 or any(start<=address and address+size<=start+REAL[name][1] for name,start in real.items()),hex(address)
    def read(uc,access,address,size,value,user):
        if DATA<=address and address+size<=DATA+0x2000 or STACK<=address and address+size<=STACK+0x10000:return
        if address in cells and size==4:reads.append([cells[address],r32(address)]);return
        for name,start in at.items():
            if start<=address and address+size<=start+programs[name]['size']:
                assert size==2;reads.append([name,address-start,struct.unpack('<H',uc.mem_read(address,2))[0]]);return
        for name,start in real.items():
            if name=='_reset_frame_state':continue
            if size in (1,4) and start<=address and address+size<=start+REAL[name][1]:
                raw=bytes(uc.mem_read(address,size));v=raw[0] if size==1 else struct.unpack('<I',raw)[0]
                if size==4:assert start<=v<start+REAL[name][1];v-=start
                reads.append([name+'-table',address-start,size,v]);return
        raise AssertionError(('unguarded read',hex(address),size))
    def write(uc,access,address,size,value,user):
        assert DATA<=address and address+size<=DATA+0x2000 or STACK<=address and address+size<=SP or address in cells and size==4,hex(address)
        if DATA<=address<DATA+0x2000:writes.append([address-DATA,size,norm(value&0xffffffff) if size==4 else value&((1<<(8*size))-1)])
        elif address in cells:writes.append([cells[address],size,value&0xffffffff])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    for entry in case['entries']:
        frame=pack(STOP)+pack(P)+b'\x93'*24;u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
        for reg,seed in zip(NV,SEEDS):u.reg_write(reg,seed)
        address=client if entry=='client' else real['_eng1_interpret_signed_pairs' if entry=='pair' else '_eng1_interpret_word_stream']
        u.emu_start(address,STOP+1,count=3000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4
        assert bytes(u.mem_read(SP,len(frame)))==frame and [u.reg_read(r) for r in NV]==SEEDS
        if entry!='client':returns.append(u.reg_read(UC_X86_REG_EAX))
    memory=bytearray(u.mem_read(DATA,0x2000));images=[]
    for actor in [P,N,G,A,B]:
        value=r32(actor+0x64);images.append([actor,norm(value)])
        if isinstance(norm(value),list):memory[actor-DATA+0x64:actor-DATA+0x68]=bytes(4)
    return dict(data_sha256=digest(memory),images=images,reads=reads,stores=writes,returns=returns,entered=dict(entered))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--audit-only',action='store_true');p.add_argument('--bindings',type=Path,default=ROOT/'evidence/player_animation_bindings_2026_10_25.json');p.add_argument('--output',type=Path,default=ROOT/'work/player_animation_2026_10_25/differential');args=p.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work');out.mkdir(parents=True,exist_ok=True)
    audit=audit_programs();(out/'programs.json').write_text(json.dumps(audit,indent=2)+'\n')
    if args.audit_only:print(json.dumps(dict(programs=3,complete_bytes=92)));return
    built,source,symbols,known=build(out,args.bindings.resolve(),audit);original=PE(ORACLE);programs=audit['programs'];cases=[]
    for name,prog in programs.items():
        profile=prog['profile'];mode=prog['mode'];states=profile['stable_return_states'];duration=max(profile['frame_ticks'])+1 if mode=='pair' else profile['duration']+1
        cases.append(dict(program=name,cursor=-1 if mode=='pair' else 0,entries=[mode]*(2*(duration*len(profile['frame_indices'])+1))))
        for cursor in states:
            for ticks in [-32768,-1,0,1,32767]:cases.append(dict(program=name,cursor=cursor,ticks=ticks,entries=[mode]*3))
    for program in ['_g_player_program_634b0','_g_player_program_634e0']:
        for mode in [0,0x22,0x23,0xffff]:
            for cursor in [0,programs[program]['profile']['frame_indices'][-1]]:
                for ticks in [-1,0,1,32767]:
                    for alias_glow in [False,True]:cases.append(dict(program=program,mode=mode,cursor=cursor,ticks=ticks,alias_glow=alias_glow,entries=['client','client','client']))
    records=[];coverage={name:set() for name in programs};counts=Counter();real_counts=Counter()
    for case in cases:
        a=execute(original,{},known,programs,case,True);b=execute(source,symbols,known,programs,case,False);assert a==b,dict(case=case,original=a,source=b)
        counts.update(case['entries'])
        real_counts.update(a['entered'])
        for row in a['reads']:
            if row[0] in coverage:coverage[row[0]].add(row[1])
        records.append(dict(case=case,result=b))
    for name,prog in programs.items():
        expected=set(range(0,prog['size'],2))
        if prog['mode']=='pair':expected.remove(prog['size']-2)
        assert coverage[name]==expected,(name,coverage[name],expected)
    assert all(digest(Path(path).read_bytes())==sha for path,sha in built['input_sha256'].items())
    report=dict(scope=__doc__,all_equal=True,cases=len(records),entry_invocations=sum(counts.values()),by_entry=dict(counts),real_consumer_invocations=dict(real_counts),program_audit=audit,read_offsets={n:sorted(v) for n,v in coverage.items()},build=built,cases_sha256=digest(json.dumps(records,sort_keys=True).encode()),limits=['Only positive frames/restart for pairs and closed WORD loops are covered with stable program/state metadata. Other opcodes/services fail the guard.','Pair restart operand belongs to its fixed-width record but is not consumed on the restart path. Its value is verified as part of the complete compiled payload.','No native actor capacities, all-opcode, runtime-completeness or original OS startup claim.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['all_equal','cases','entry_invocations','by_entry']},indent=2))


if __name__=='__main__':main()
