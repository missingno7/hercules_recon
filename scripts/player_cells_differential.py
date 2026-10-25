"""Guarded original/source player-cell clients and flag/scroll compositions.

Real frame helpers and the empty entry are composed. RNG, animation, removal,
camera bounds and 39d40 remain modeled. No original OS module/game startup.
"""
import argparse
from collections import Counter
import itertools
import json
import os
from pathlib import Path
import random
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

ORACLE=ROOT/'assets/pc/HERCULES/ENG1.DLL'
ORACLE_SHA='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
REAL={'_advance_frame','_select_frame','_reset_frame_state','_eng1_empty_c720'}
MODELED={'_random_limit':1,'_eng1_interpret_signed_pairs':1,'_eng1_interpret_word_stream':1,
         '_remove_actor':1,'_dispatch_local_20ea0':4,'_eng1_39d40':1}
CELL_NAMES=['_g_player_glow_707dc','_g_burst_origin_707e4','_g_player_cruise_707e8','_g_player_flag_707f8','_g_player_trail_707fc','_g_frame_owner_72860']
DATA=0x61000000;P=DATA+0x100;GLOW=DATA+0x300;TRAIL=DATA+0x500;BURST=DATA+0x700;NORMAL=DATA+0x900
ALT_BURST=DATA+0xb00;ALT_TRAIL=DATA+0xd00;ALT_NORMAL=DATA+0xf00;FRAMES=DATA+0x1800
STACK=0x62000000;SP=STACK+0x8000;STOP=STACK+0xf000
NV=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];SEEDS=[0x12345678,0x23456789,0x3456789a,0x456789ab]
pack=lambda n:struct.pack('<I',n&0xffffffff)
signed=lambda n:((n&0xffffffff)^0x80000000)-0x80000000


def build(out,binding_path):
    verify_toolchain(DEFAULT_TOOLCHAIN);runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes())==ORACLE_SHA
    bindings=json.loads(binding_path.read_text());state=json.loads((ROOT/'recovery.json').read_text())['functions']
    rows=[next(r for r in state if r['id']==i) for i in bindings['functions']]
    known={n:r for f in bindings['functions'].values() for n,r in f['symbols'].items()}
    assert len(rows)==26 and sum(r['size'] for r in rows)==2576
    inputs={Path(__file__).resolve(),runtime,ORACLE,binding_path,ROOT/'toolchains/manifest.json'}
    profiles={}
    for row in rows:inputs.update(compile_inputs(ROOT/row['source'],row['flags'],DEFAULT_TOOLCHAIN))
    for name,rec in known.items():
        if rec['kind']!='bss':continue
        owner=rec['owner'];key=(owner['source'],tuple(owner['flags']),rec.get('source_alias',{}).get('header'))
        profiles.setdefault(key,(name,rec));inputs.update(compile_inputs(ROOT/owner['source'],owner['flags'],DEFAULT_TOOLCHAIN))
        if 'source_alias' in rec:
            header,_=alias_profile(rec,name,ROOT);inputs.add(header)
    for source in ('src/shared/frame_state.c','src/shared/empty_entries.c'):inputs.update(compile_inputs(ROOT/source,['/O2'],DEFAULT_TOOLCHAIN))
    pins={str(p):digest(p.read_bytes()) for p in inputs};out.mkdir(parents=True,exist_ok=True)
    objects=[];commands=[];checks=[]
    for source in sorted({r['source'] for r in rows}):
        selected=[r for r in rows if r['source']==source];flags=selected[0]['flags'];assert all(r['flags']==flags for r in selected)
        original,cmd=compile_source(ROOT/source,flags);commands.append(cmd);coff=COFF(original)
        text=(ROOT/source).read_text();keep={r['symbol'] for r in selected}
        for sym in coff.symbols.values():
            if sym['section']<=0 or not sym['type']&0x20 or sym['name'] in keep:continue
            name=sym['name'];assert name.startswith('_') and '@' not in name
            body=function_text(text,name[1:]);text=replace_function(text,name[1:],body.split('{',1)[0].rstrip()+';')
        fixture=out/Path(source).name;fixture.write_text(text);pins[str(fixture)]=digest(fixture.read_bytes())
        obj,cmd=compile_source(fixture,flags);commands.append(cmd);extracted=COFF(obj)
        assert {s['name'] for s in extracted.symbols.values() if s['section']>0 and s['type']&0x20}==keep
        for row in selected:
            assert extracted.function(row['symbol'])==coff.function(row['symbol']),row['id']
            check=masked_compare(obj,row['symbol'],ORACLE,int(row['rva'],0),row['size']);assert check['masked_equal'],check
            checks.append(dict(id=row['id'],complete_canonical_contribution_unchanged=True,diagnostic=check))
        objects.append(obj)
    for (source,flags,header_name),(name,rec) in profiles.items():
        src=ROOT/source;flags=list(flags)
        if header_name:
            header,_=alias_profile(rec,name,ROOT);src=wrapper_source(src.resolve(),header,ROOT,pointer_types=rec['source_alias']['pointer_types'])
            pins[str(src)]=digest(src.read_bytes());flags+=['/WX']
        obj,cmd=compile_source(src,flags);objects.append(obj);commands.append(cmd)
    for source in ('src/shared/frame_state.c','src/shared/empty_entries.c'):
        obj,cmd=compile_source(ROOT/source,['/O2']);objects.append(obj);commands.append(cmd)
        for name in REAL:
            matches=[s for s in COFF(obj).symbols.values() if s['name']==name and s['section']>0]
            if matches:
                rec=known[name];check=masked_compare(obj,name,ORACLE,rec['rva'],rec['size']);assert check['masked_equal'],check
    services=out/'services.c'
    services.write_text('/* Intercepted synthetic fixture services, never production. */\nint random_limit(int n){for(;;){}}\nvoid eng1_interpret_signed_pairs(void*p){for(;;){}}\nvoid eng1_interpret_word_stream(void*p){for(;;){}}\nint remove_actor(void*p){for(;;){}}\nint dispatch_local_20ea0(void*b,int id,int mode,int x){for(;;){}}\nvoid eng1_39d40(void*p){for(;;){}}\n')
    pins[str(services)]=digest(services.read_bytes());obj,cmd=compile_source(services,['/O2']);objects.append(obj);commands.append(cmd)
    image=out/'fixture.dll';mapfile=out/'fixture.map'
    cmd=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF','/BASE:0x45000000','/OUT:'+str(image),'/MAP:'+str(mapfile)]
    cmd += ['/EXPORT:'+r['symbol'][1:] for r in rows]+list(map(str,objects));commands.append(cmd)
    env=os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    ran=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(ran.stdout+ran.stderr);assert ran.returncode==0,ran.stdout+ran.stderr
    symbols=read_map(mapfile);assert len({symbols[name] for name in MODELED})==len(MODELED)
    assert all(digest(Path(p).read_bytes())==sha for p,sha in pins.items())
    return dict(input_sha256=pins,commands=commands,extraction_checks=checks,image_sha256=digest(image.read_bytes()),object_sha256=[digest(p.read_bytes()) for p in objects]),PE(image),symbols,{r['id']:r for r in rows},known


def execute(pe,symbols,rows,known,case,original):
    u=Uc(UC_ARCH_X86,UC_MODE_32);extent=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(pe.image_base,extent);u.mem_write(pe.image_base,pe.pe.get_memory_mapped_image().ljust(extent,b'\0'))
    u.mem_map(DATA,0x3000);u.mem_map(STACK,0x10000);u.mem_write(DATA,bytes((i*13+31)&255 for i in range(0x3000)))
    cells={name:pe.image_base+known[name]['rva'] if original else symbols[name] for name in CELL_NAMES};cell_at={at:name for name,at in cells.items()}
    entries={fid:pe.image_base+int(row['rva'],0) if original else symbols[row['symbol']] for fid,row in rows.items()}
    helpers={pe.image_base+known[name]['rva'] if original else symbols[name]:name for name in REAL|set(MODELED)}
    spans=[(entries[fid],row['size']) for fid,row in rows.items()]+[(at,known[name]['size']) for at,name in helpers.items() if name in REAL]
    def w32(at,n):u.mem_write(at,pack(n))
    def w16(at,n):u.mem_write(at,struct.pack('<H',n&0xffff))
    def r32(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    for name,value in zip(CELL_NAMES,[GLOW,BURST,case.get('cruise',0),case.get('flag',0),TRAIL,NORMAL]):w32(cells[name],value)
    actors=[P,GLOW,TRAIL,BURST,NORMAL,ALT_BURST,ALT_TRAIL,ALT_NORMAL]
    for index,at in enumerate(actors):
        for off,val in [(0,case.get('px',0)),(4,case.get('py',0)),(8,case.get('pz',0)),(0x108,0x11223344),(0x110,FRAMES),(0x120,0),(0x128,0)]:w32(at+off,val)
        for off in [0x70,0x72,0x74]:w16(at+off,case.get('angle',0))
        for off,val in [(0x30,7),(0x32,index*3),(0x114,0),(0x11e,0),(0x2a,0)]:w16(at+off,val)
        for off,key in [(0xa8,'vx'),(0xac,'tx'),(0xb0,'rx'),(0xb8,'vy'),(0xbc,'ty'),(0xc0,'ry'),(0xc8,'vz'),(0xcc,'tz'),(0xd0,'rz')]:w32(at+off,case.get(key,0))
        u.mem_write(at+0x94,bytes([case.get('direction',0)]))
    for at,shift in [(BURST,0),(ALT_BURST,0x10000)]:
        w32(at,case.get('bx',0)+shift);w32(at+4,case.get('by',0)+shift)
    for at in [NORMAL,ALT_NORMAL]:w32(at,case.get('nx',0));w32(at+4,case.get('ny',0))
    for i in range(64):w32(FRAMES+4*i,DATA+0x2000+i*8)
    if case.get('frame_loop'):w32(FRAMES+4,0);w32(FRAMES+8,3)
    calls=[];reads=[];writes=[];rng_index=0;current=None
    def finish(value=0):
        sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,r32(sp));u.reg_write(UC_X86_REG_ESP,sp+4)
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xaabbccdd);u.reg_write(UC_X86_REG_EDX,0x99887766)
    def roots():
        w32(cells['_g_burst_origin_707e4'],ALT_BURST);w32(cells['_g_player_trail_707fc'],ALT_TRAIL);w32(cells['_g_frame_owner_72860'],ALT_NORMAL)
    def code(uc,at,size,user):
        nonlocal rng_index
        if at==STOP:uc.emu_stop();return
        if at in helpers:
            name=helpers[at];sp=uc.reg_read(UC_X86_REG_ESP)
            argc=MODELED.get(name,2 if name=='_select_frame' else 1)
            args=[r32(sp+4+4*n) for n in range(argc)]
            if name=='_dispatch_local_20ea0':
                assert STACK<=args[0] and args[0]+16<=SP and args[1] in (0,1) and args[2]==8
                assert signed(args[3])==signed(r32(P))>>16
                calls.append([name,'stack_bounds',*args[1:]])
            else:calls.append([name,*args])
            if name in REAL:
                assert args[0] in actors
                if name=='_select_frame':assert 0<=args[1]<64
                return
            if name=='_random_limit':
                assert 0<args[0]<=0x10000
                value=(case.get('rng',0)+rng_index*7)%args[0];rng_index+=1
                if case.get('mutate'):
                    roots();w32(cells['_g_player_cruise_707e8'],case.get('cruise',0)+rng_index*0x1111)
                finish(value);return
            if name=='_dispatch_local_20ea0':
                payload=struct.pack('<8h',case.get('ceiling',-256),7,case.get('floor',512),9,11,13,15,17)
                uc.mem_write(args[0],payload)
                if case.get('mutate'):roots();w32(P+4,case.get('bounds_y',0))
                finish(1);return
            assert args[0] in actors
            if case.get('mutate'):
                roots();w32(args[0]+0x108,0x76543210)
                if name=='_remove_actor':w16(args[0]+0x2e,0);w16(args[0]+0x2a,0x44)
                elif name in ('_eng1_interpret_signed_pairs','_eng1_interpret_word_stream'):
                    w32(args[0],r32(args[0])+0x10000);w32(args[0]+4,r32(args[0]+4)-0x20000)
                elif name=='_eng1_39d40':w32(args[0]+0xb8,r32(args[0]+0xb8)^0x1234)
            finish(1 if name=='_remove_actor' else 0);return
        assert any(start<=at and at+size<=start+length for start,length in spans),(current,hex(at))
    def read(uc,access,at,size,value,user):
        assert DATA<=at and at+size<=DATA+0x3000 or STACK<=at and at+size<=STACK+0x10000 or at in cell_at and size==4,(current,hex(at),size)
        if at in cell_at:reads.append([cell_at[at],r32(at)])
    def write(uc,access,at,size,value,user):
        assert DATA<=at and at+size<=DATA+0x3000 or STACK<=at and at+size<=SP or at in cell_at and size==4,(current,hex(at),size)
        if at in cell_at:writes.append([cell_at[at],size,value&0xffffffff])
        elif DATA<=at<DATA+0x3000:writes.append([at-DATA,size,value&((1<<(size*8))-1)])
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    for fid in case['entries']:
        current=fid;frame=pack(STOP)+(b'' if fid=='eng1.372b0' else pack(P))+b'\x93'*32
        u.mem_write(SP,frame);u.reg_write(UC_X86_REG_ESP,SP)
        for reg,seed in zip(NV,SEEDS):u.reg_write(reg,seed)
        u.emu_start(entries[fid],STOP+1,count=3000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==SP+4
        assert bytes(u.mem_read(SP,len(frame)))==frame and [u.reg_read(reg) for reg in NV]==SEEDS
    return dict(data_sha256=digest(bytes(u.mem_read(DATA,0x3000))),cells={n:r32(at) for n,at in cells.items()},calls=calls,ordered_stores=writes,cell_reads=reads)


def cases(rows):
    values=[0,1,-1,0x7fffffff,0x80000000,0xc00000,0x1400000,-0xa00000]
    rng=random.Random(0x707dc)
    for fid in rows:
        for index in range(64):
            yield dict(entries=[fid],px=rng.choice(values),py=rng.choice(values),pz=rng.choice(values),bx=rng.choice(values),by=rng.choice(values),nx=rng.choice(values),ny=rng.choice(values),flag=rng.choice(values),cruise=rng.choice(values),vx=rng.choice(values),vy=rng.choice(values),vz=rng.choice(values),rx=rng.choice(values),ry=rng.choice(values),rz=rng.choice(values),angle=rng.choice([0,-1,32767,-32768]),direction=index%2,rng=index,mutate=bool(index&2),frame_loop=bool(index&4),floor=rng.choice([-32768,-1,0,16,64,32767]),ceiling=rng.choice([-32768,-1,0,32767]))
    for fid in ['eng1.3b050','eng1.3b080','eng1.3b0b0','eng1.3b0e0','eng1.3b110','eng1.3b140']:
        for x,flag,mutate in itertools.product([-1,0,1],[-129,-128,-1,0,1,127,128,0x7fffffff,0x80000000],[False,True]):
            yield dict(entries=[fid,'eng1.3af20','eng1.3af20'],px=x,bx=0,flag=flag,mutate=mutate,cruise=0)
    for dx,dy,flag,cruise in itertools.product([-0xa00001,-0xa00000,-0x9fffff,0x6fffff,0x700000,0x700001],[-1,0,0xa00000,0xa00001],[-2,-1,0,1,2],[0,-1,1]):
        yield dict(entries=['eng1.3af20'],px=dx,py=dy,flag=flag,cruise=cruise,floor=32767)
    for vy,cruise,mutate in itertools.product([-1,0,1,0x7fffffff,0x80000000],[-3,-2,-1,0,1,2,3],[False,True]):
        yield dict(entries=['eng1.3c330'],vy=vy,cruise=cruise,mutate=mutate,floor=64,ceiling=-32)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bindings',type=Path,default=ROOT/'evidence/player_cells_bindings_2026_10_25.json')
    parser.add_argument('--output',type=Path,default=ROOT/'work/player_cells_2026_10_25/differential')
    args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'work')
    built,source,symbols,rows,known=build(out,args.bindings.resolve());original=PE(ORACLE)
    counts=Counter();calls=Counter();records=[];composed=0
    for case in cases(rows):
        expected=execute(original,{},rows,known,case,True);actual=execute(source,symbols,rows,known,case,False)
        assert actual==expected,dict(case=case,original=expected,source=actual)
        composed+=len(case['entries'])>1
        for fid in case['entries']:counts[fid]+=1
        for call in actual['calls']:calls[call[0]]+=1
        records.append(dict(case=case,result=actual))
    assert set(counts)==set(rows) and all(calls[name] for name in REAL|set(MODELED))
    assert all(digest(Path(p).read_bytes())==sha for p,sha in built['input_sha256'].items())
    report=dict(scope=__doc__,all_equal=True,cases=len(records),entry_invocations=sum(counts.values()),composed_cases=composed,by_entry=dict(counts),service_calls=dict(calls),build=built,cases_sha256=digest(json.dumps(records,sort_keys=True).encode()),limits=['Native capacities and full animation/removal/camera/host behavior are unproved. Modeled service effects stress root and state reloads; only the frame helpers and empty entry execute as real dependencies.','Private extraction preserves all 26 complete canonical contributions before linking; no code patching or original OS startup.'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['all_equal','cases','entry_invocations','composed_cases','by_entry','service_calls']},indent=2))


if __name__=='__main__':main()
