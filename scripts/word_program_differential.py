"""Closed WORD-loop profile and isolated unpatched interpreter/client checks.

Only frame, sound(-40) and restart(-1) streams are certified by this profile.
Actor program/state mutation by other systems and other opcodes remain outside
it. No original OS module or game startup is performed.
"""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

from env import ROOT
from match import DEFAULT_TOOLCHAIN,compile_source,compile_inputs,digest,verify_toolchain
from coff import COFF
from masked import masked_compare
from pe import PE
from refinery_forms import function_text,replace_function
from storage_alias import alias_profile,wrapper_source
from title_link import read_map
sys.path.insert(0,'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL';ORACLE_SHA='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
TABLE_SOURCE='src/eng1/behavioral/flags_word_programs.c';TABLE_HEADER='src/eng1/behavioral/flags_word_programs.h'
INTERPRETER='calibration/eng1/actor_unit_282e0.cpp'
IDS=['eng1.37230','eng1.37300','eng1.37460','eng1.3aef0']
DATA=0x61000000;P=DATA+0x100;G=DATA+0x300;T=DATA+0x500;N=DATA+0x700;A=DATA+0x900;B=DATA+0xb00;C=DATA+0xd00;FRAMES=DATA+0x1200
STACK=0x62000000;SP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x12345678,0x23456789,0x3456789a,0x456789ab]
pack=lambda n:struct.pack('<I',n&0xffffffff)


def loop_profile(words):
    if not isinstance(words,(list,tuple)) or len(words)<3 or any(type(n)is not int or not 0<=n<=65535 for n in words):
        raise ValueError('Expected complete raw WORDs including duration, a frame and restart')
    frames=[];sounds=[];index=1
    while index<len(words):
        op=words[index]
        if op<0x8000:frames.append(index);index+=1
        elif op==0xffd8:
            if index+1>=len(words):raise ValueError('Truncated sound operand')
            sounds.append(dict(opcode=index,operand=index+1));index+=2
        elif op==0xffff:
            if index!=len(words)-1:raise ValueError('Restart does not delimit the complete object')
            if not frames:raise ValueError('Loop would never reach a frame')
            return dict(words=len(words),bytes=2*len(words),duration=words[0] if words[0]<0x8000 else words[0]-65536,frame_indices=frames,sound_indices=sounds,restart_index=index,stable_return_states=[0,*frames])
        else:raise ValueError('Opcode outside the closed frame/sound/restart profile')
    raise ValueError('No terminal restart')


def same_local_targets(original,extracted,name):
    """Fixture-only equality, allowing just renumbered labels in this COMDAT."""
    left=original.function(name);right=extracted.function(name)
    if {k:v for k,v in left.items() if k!='relocations'}!={k:v for k,v in right.items() if k!='relocations'}:
        raise ValueError('Complete function bytes/flags/extent changed')
    a=left['relocations'];b=right['relocations']
    if len(a)!=len(b):raise ValueError('Relocation inventory changed')
    changed=0
    for old,new in zip(a,b):
        if old==new:continue
        if {k:v for k,v in old.items() if k!='symbol'}!={k:v for k,v in new.items() if k!='symbol'}:
            raise ValueError('Relocation offset/type changed')
        values=[]
        for coff,rel in [(original,old),(extracted,new)]:
            if not re.fullmatch(r'\$L\d+',rel['symbol']):raise ValueError('Only compiler-local label numbers may differ')
            fn,=[s for s in coff.symbols.values() if s['name']==name and s['section']>0]
            choices=[s for s in coff.symbols.values() if s['name']==rel['symbol']]
            if len(choices)!=1:raise ValueError('Ambiguous local label')
            sym=choices[0]
            if sym['section']!=fn['section'] or sym['storage']!=6 or sym['type']!=0 or sym['aux'] or not 0<=sym['value']<left['size']:
                raise ValueError('Label must be an unambiguous plain label in this complete function')
            values.append(sym['value'])
        if values[0]!=values[1]:raise ValueError('Local label target changed')
        changed+=1
    return changed


def audit_programs():
    verify_toolchain(DEFAULT_TOOLCHAIN);pe=PE(ORACLE);assert digest(pe.data)==ORACLE_SHA
    context=compile_inputs(ROOT/TABLE_SOURCE,['/O2'],DEFAULT_TOOLCHAIN)
    obj,command=compile_source(ROOT/TABLE_SOURCE,['/O2']);coff=COFF(obj);programs={};excluded=[]
    declarations=re.findall(r'extern\s+Eng1ProgramWord\s+(\w+)\[(\d+)\]',(ROOT/TABLE_HEADER).read_text())
    assert len(declarations)==15
    for name,count in declarations:
        size=2*int(count);symbol,=[s for s in coff.symbols.values() if s['name']=='_'+name and s['section']>0]
        sec=coff.sections[symbol['section']-1];start=symbol['value'];payload=sec['data'][start:start+size]
        assert len(payload)==size and not [r for r in sec['relocations'] if start<=r['offset']<start+size]
        try:profile=loop_profile(list(struct.unpack('<'+'H'*int(count),payload)))
        except ValueError as exc:excluded.append(dict(symbol='_'+name,reason=str(exc)));continue
        positions=[]
        for section in pe.pe.sections:
            data=section.get_data();at=data.find(payload)
            while at>=0:positions.append(section.VirtualAddress+at);at=data.find(payload,at+1)
        assert len(positions)==1,(name,positions)
        programs['_'+name]=dict(rva=positions[0],size=size,sha256=digest(payload),source=TABLE_SOURCE,profile=profile,unique_native_positions=positions)
    assert len(programs)==10 and sum(p['size'] for p in programs.values())==186
    assert context==compile_inputs(ROOT/TABLE_SOURCE,['/O2'],DEFAULT_TOOLCHAIN)
    return dict(scope=__doc__,oracle_sha256=ORACLE_SHA,programs=programs,excluded=excluded,command=command,object_sha256=digest(obj.read_bytes()),compile_inputs={str(p):h for p,h in context.items()})


def build(out,binding_path,table_audit):
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    bindings=json.loads(binding_path.read_text());state=json.loads((ROOT/'recovery.json').read_text())['functions']
    rows={i:next(r for r in state if r['id']==i) for i in IDS};known={n:r for f in bindings['functions'].values() for n,r in f['symbols'].items()}
    inputs={Path(__file__).resolve(),runtime,ORACLE,binding_path,ROOT/'toolchains/manifest.json'}
    for source in {r['source'] for r in rows.values()}|{INTERPRETER,TABLE_SOURCE,'calibration/eng1/actor_middle_init.c','src/eng1/behavioral/actor_base_init.c','src/shared/frame_state.c'}:
        inputs.update(compile_inputs(ROOT/source,['/O2'],DEFAULT_TOOLCHAIN))
    owner_profiles={}
    for name,rec in known.items():
        if rec['kind']!='bss' or name=='_g_frame_global_734f4':continue
        owner=rec['owner'];key=(owner['source'],tuple(owner['flags']),rec.get('source_alias',{}).get('header'));owner_profiles.setdefault(key,(name,rec))
        inputs.update(compile_inputs(ROOT/owner['source'],owner['flags'],DEFAULT_TOOLCHAIN))
        if 'source_alias'in rec:header,_=alias_profile(rec,name,ROOT);inputs.add(header)
    pins={str(p):digest(p.read_bytes()) for p in inputs};out.mkdir(parents=True,exist_ok=True);objects=[];commands=[];checks=[]
    for source in sorted({r['source'] for r in rows.values()}):
        selected=[r for r in rows.values() if r['source']==source];full,cmd=compile_source(ROOT/source,['/O2']);commands.append(cmd);coff=COFF(full)
        text=(ROOT/source).read_text();keep={r['symbol'] for r in selected}
        for sym in coff.symbols.values():
            if sym['section']<=0 or not sym['type']&0x20 or sym['name'] in keep:continue
            name=sym['name'][1:];body=function_text(text,name);text=replace_function(text,name,body.split('{',1)[0].rstrip()+';')
        fixture=out/Path(source).name;fixture.write_text(text);pins[str(fixture)]=digest(fixture.read_bytes())
        obj,cmd=compile_source(fixture,['/O2']);objects.append(obj);commands.append(cmd)
        for row in selected:
            assert COFF(obj).function(row['symbol'])==coff.function(row['symbol']),row['id']
            check=masked_compare(obj,row['symbol'],ORACLE,int(row['rva'],0),row['size']);assert check['masked_equal']
            checks.append(dict(id=row['id'],complete_canonical_contribution_unchanged=True,diagnostic=check))
    for (source,flags,header_name),(name,rec) in owner_profiles.items():
        source=ROOT/source;flags=list(flags)
        if header_name:
            header,_=alias_profile(rec,name,ROOT);source=wrapper_source(source.resolve(),header,ROOT,pointer_types=rec['source_alias']['pointer_types']);flags+=['/WX'];pins[str(source)]=digest(source.read_bytes())
        obj,cmd=compile_source(source,flags);objects.append(obj);commands.append(cmd)
    real={'_eng1_interpret_word_stream':(0x28820,640),'_reset_frame_state':(0x29260,16),'_select_frame':(0x292f0,48),'_initialize_middle_actor':(0x27a50,176),'_initialize_alternate_actor':(0x27fe0,272)}
    canonical_frames,_=compile_source(ROOT/'src/shared/frame_state.c',['/O2'])
    for source in [INTERPRETER,TABLE_SOURCE,'calibration/eng1/actor_middle_init.c','src/eng1/behavioral/actor_base_init.c']:
        obj,cmd=compile_source(ROOT/source,['/O2']);commands.append(cmd);coff=COFF(obj)
        if source==INTERPRETER:
            original_coff=coff;keep={'_eng1_interpret_word_stream','_reset_frame_state','_select_frame'}
            text=(ROOT/source).read_text()
            for sym in coff.symbols.values():
                if sym['section']<=0 or not sym['type']&0x20 or sym['name']in keep:continue
                name=sym['name'][1:];body=function_text(text,name);text=replace_function(text,name,body.split('{',1)[0].rstrip()+';')
            extracted=out/'word_interpreter.cpp';extracted.write_text(text);pins[str(extracted)]=digest(extracted.read_bytes())
            obj,cmd=compile_source(extracted,['/O2']);commands.append(cmd);coff=COFF(obj)
            for name in keep:
                count=same_local_targets(original_coff,coff,name)
                checks.append(dict(id=name,complete_canonical_contribution_unchanged=True,renumbered_local_labels_with_equal_targets=count))
        objects.append(obj)
        for name,(rva,size) in real.items():
            if any(s['name']==name and s['section']>0 for s in coff.symbols.values()):
                check=masked_compare(obj,name,ORACLE,rva,size);assert check['masked_equal'],(name,check)
                if name in ('_reset_frame_state','_select_frame'):assert coff.function(name)==COFF(canonical_frames).function(name)
    fixture=out/'services.c'
    fixture.write_text('/* Intercepted fixture services and isolated mirror cell, never production. */\nunsigned long g_frame_global_734f4;\nvoid *g_configuration_dword_71384;\nint eng1_interpret_signed_pairs(void*p){for(;;){}}\nvoid play_sound(int s,void*p){for(;;){}}\nvoid mark_actor_resource_for_release(void*p){for(;;){}}\nint remove_actor(void*p){for(;;){}}\nvoid remove_actor_preserving_resource(void*p){for(;;){}}\nvoid stop_actor_sounds(void*p){for(;;){}}\nvoid title_17ad0(int x,int y,int z,int k,int g){for(;;){}}\n')
    pins[str(fixture)]=digest(fixture.read_bytes());obj,cmd=compile_source(fixture,['/O2']);objects.append(obj);commands.append(cmd)
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile),'/EXPORT:eng1_interpret_word_stream']+['/EXPORT:'+r['symbol'][1:] for r in rows.values()]+list(map(str,objects));commands.append(cmd)
    env=os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    ran=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    symbols=read_map(mapfile);assert len({symbols[n] for n in ['_play_sound','_eng1_interpret_signed_pairs','_remove_actor']})==3
    assert all(digest(Path(p).read_bytes())==sha for p,sha in pins.items())
    return dict(input_sha256=pins,commands=commands,extraction_checks=checks,image_sha256=digest(image.read_bytes()),object_sha256=[digest(p.read_bytes()) for p in objects]),PE(image),symbols,rows,known,real


def execute(pe,symbols,rows,known,real,programs,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'));u.mem_map(DATA,0x2000);u.mem_map(STACK,0x10000)
    u.mem_write(DATA,bytes((i*13+31)&255 for i in range(0x2000)))
    program_at={name:pe.image_base+p['rva'] if original else symbols[name] for name,p in programs.items()}
    def norm(value):
        for name,at in program_at.items():
            if at<=value<at+programs[name]['size']:return [name,value-at]
        return value
    cells={name:pe.image_base+known[name]['rva'] if original else symbols[name] for name in known if known[name]['kind']=='bss'};cell_at={at:name for name,at in cells.items()}
    def w32(at,n):u.mem_write(at,pack(n))
    def w16(at,n):u.mem_write(at,struct.pack('<H',n&0xffff))
    def r32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    for name,at in cells.items():w32(at,{'_g_frame_owner_72860':N,'_g_native_pointer_7217c':N,'_g_player_glow_707dc':G,'_g_player_trail_707fc':T,'_g_burst_origin_707e4':A,'_g_frame_global_734f4':B,'_g_player_flag_707f8':0x76543210}.get(name,0))
    actors=[P,G,T,N,A,B,C]
    chosen=case.get('program','_g_eng1_flag_program_063318')
    for index,at in enumerate(actors):
        for off,val in [(0,0x100000+index*0x10000),(4,0x200000-index*0x10000),(8,0x300000+index*0x10000),(0x64,program_at[chosen]),(0x110,FRAMES)]:w32(at+off,val)
        w16(at+0x30,case.get('ticks',0));w16(at+0x32,case.get('state',0));u.mem_write(at+0x94,bytes([case.get('direction',0)]))
    for index in range(32):w32(FRAMES+4*index,DATA+0x1600+index*8)
    if case.get('actor')=='glow':argument=G
    elif case.get('actor')=='trail':argument=T
    else:argument=P
    at_real={pe.image_base+rva if original else symbols[name]:name for name,(rva,size) in real.items()}
    pair=pe.image_base+0x283e0 if original else symbols['_eng1_interpret_signed_pairs'];sound=pe.image_base+0x6da0 if original else symbols['_play_sound']
    client_entries={fid:pe.image_base+int(row['rva'],0) if original else symbols[row['symbol']] for fid,row in rows.items()}
    spans=[(at,real[name][1]) for at,name in at_real.items()]+[(client_entries[fid],row['size']) for fid,row in rows.items()]
    trace=[];writes=[];reads=[];returns=[];current=None
    def ret(value=0):
        sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,r32(sp));u.reg_write(UC_X86_REG_ESP,sp+4)
        u.reg_write(UC_X86_REG_EAX,value);u.reg_write(UC_X86_REG_ECX,0xaabbccdd);u.reg_write(UC_X86_REG_EDX,0x99887766)
    def code(uc,at,size,user):
        if at==STOP:uc.emu_stop();return
        sp=uc.reg_read(UC_X86_REG_ESP)
        if at==pair:
            actor=r32(sp+4);assert actor in actors;trace.append(['signed_pairs',actor])
            if case.get('mutate_roots'):
                for name,value in [('_g_frame_owner_72860',C),('_g_player_glow_707dc',A),('_g_player_trail_707fc',B)]:w32(cells[name],value)
            ret();return
        if at==sound:
            sound_id=r32(sp+4)&0xffff;actor=r32(sp+8);assert actor in actors and sound_id in (0x627e,0x627f)
            trace.append(['sound',sound_id,actor]);ret();return
        if at in at_real:
            actor=r32(sp+4);assert actor in actors
            if at_real[at]!='_eng1_interpret_word_stream':trace.append([at_real[at],actor,*([r32(sp+8)] if at_real[at]=='_select_frame' else [])])
        assert any(start<=at and at+size<=start+length for start,length in spans),(current,hex(at))
    def read(uc,access,at,size,value,user):
        if DATA<=at and at+size<=DATA+0x2000 or STACK<=at and at+size<=STACK+0x10000:return
        if at in cell_at and size==4:reads.append([cell_at[at],norm(r32(at))]);return
        for name,start in program_at.items():
            if start<=at and at+size<=start+programs[name]['size']:
                assert size==2;reads.append([name,at-start,struct.unpack('<H',uc.mem_read(at,2))[0]]);return
        word=pe.image_base+0x28820 if original else symbols['_eng1_interpret_word_stream']
        if size in (1,4) and word<=at and at+size<=word+640:
            raw=bytes(uc.mem_read(at,size))
            value=struct.unpack('<I',raw)[0] if size==4 else raw[0]
            if size==4:
                assert word<=value<word+640
                value-=word
            reads.append(['word-switch-table',at-word,size,value]);return
        raise AssertionError((current,'unguarded read',hex(at),size))
    def write(uc,access,at,size,value,user):
        assert DATA<=at and at+size<=DATA+0x2000 or STACK<=at and at+size<=SP or at in cell_at and size==4,(current,hex(at),size)
        if DATA<=at<DATA+0x2000:writes.append([at-DATA,size,norm(value&((1<<(size*8))-1)) if size==4 else value&((1<<(size*8))-1)])
        elif at in cell_at:writes.append([cell_at[at],size,norm(value&0xffffffff)])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    for fid in case['entries']:
        current=fid;frame=pack(STOP)+(b'' if fid in ('eng1.37230','eng1.37460') else pack(argument))+b'\x93'*24
        u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
        for reg,seed in zip(NV,SEEDS):u.reg_write(reg,seed)
        entry=pe.image_base+0x28820 if original and fid=='word' else symbols['_eng1_interpret_word_stream'] if fid=='word' else client_entries[fid]
        u.emu_start(entry,STOP+1,count=3000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4
        assert bytes(u.mem_read(SP,len(frame)))==frame and [u.reg_read(reg) for reg in NV]==SEEDS
        if fid=='word':returns.append(u.reg_read(UC_X86_REG_EAX))
    memory=bytearray(u.mem_read(DATA,0x2000));images=[]
    for at in actors:
        value=r32(at+0x64);images.append([at,norm(value)])
        if isinstance(norm(value),list):memory[at-DATA+0x64:at-DATA+0x68]=b'\0'*4
    return dict(data_sha256=digest(memory),images=images,cells={n:r32(at) for n,at in cells.items()},calls=trace,stores=writes,reads=reads,returns=returns)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--audit-only',action='store_true')
    parser.add_argument('--bindings',type=Path,default=ROOT/'evidence/player_programs_bindings_2026_10_25.json');parser.add_argument('--output',type=Path,default=ROOT/'work/player_programs_2026_10_25/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work');out.mkdir(parents=True,exist_ok=True)
    audit=audit_programs();(out/'programs.json').write_text(json.dumps(audit,indent=2)+'\n')
    if args.audit_only:print(json.dumps(dict(programs=len(audit['programs']),complete_bytes=186,excluded=len(audit['excluded']))));return
    built,source,symbols,rows,known,real=build(out,args.bindings.resolve(),audit);original=PE(ORACLE);programs=audit['programs']
    records=[];counts=Counter();covered={name:set() for name in programs};word_invocations=0
    cases=[]
    for name,program in programs.items():
        profile=program['profile'];duration=max(1,profile['duration']+1);length=2*(duration*len(profile['frame_indices'])+1)
        cases.append(dict(program=name,entries=['word']*length))
        for state in profile['stable_return_states']:
            for ticks in [-32768,-1,0,1,32767]:cases.append(dict(program=name,state=state,ticks=ticks,entries=['word']*3))
    for fid in IDS:
        for actor in ['self','glow','trail']:
            for direction in [0,255]:
                for mutate in [False,True]:cases.append(dict(entries=[fid,'word','word'],actor=actor,direction=direction,mutate_roots=mutate))
    for case in cases:
        expected=execute(original,{},rows,known,real,programs,case,True);actual=execute(source,symbols,rows,known,real,programs,case,False)
        assert actual==expected,dict(case=case,original=expected,source=actual)
        for entry in case['entries']:counts[entry]+=1
        word_invocations+=case['entries'].count('word')
        for read in actual['reads']:
            if read[0] in covered:covered[read[0]].add(read[1]//2)
        records.append(dict(case=case,result=actual))
    for name,program in programs.items():assert covered[name]==set(range(program['profile']['words'])),(name,covered[name])
    assert all(digest(Path(p).read_bytes())==h for p,h in built['input_sha256'].items())
    report=dict(scope=__doc__,all_equal=True,cases=len(records),word_invocations=word_invocations,entry_invocations=sum(counts.values()),by_entry=dict(counts),program_audit=audit,read_word_coverage={n:sorted(v) for n,v in covered.items()},build=built,cases_sha256=digest(json.dumps(records,sort_keys=True).encode()),limits=['Only the closed frame/sound/restart profile with stable program/state metadata is covered. Sound and signed-pair services are modeled; unsupported interpreter paths fail the guard.','Mirror fixture uses an isolated four-byte cell; the actual full frame_data.c owner is separately compiled by the reference gate. No broader capacity, all-opcode or whole-game claim.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['all_equal','cases','word_invocations','entry_invocations','by_entry']},indent=2))


if __name__=='__main__':main()
