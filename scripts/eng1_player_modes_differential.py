"""Bounded original/source player-mode comparison in Unicorn; never loads a DLL.

The selected handler and optionally its two axis-motion bodies execute. Other
external calls are explicit modeled services with an ordered transcript. The
fixture is linked normally by VC5; no original or candidate instruction is
modified, and no original DLL is loaded into an OS process.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

from env import ROOT
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from pe import PE
from title_link import read_map
from title_behavioral import context_files

sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

SOURCE = ROOT/'src/eng1/player_modes_remaining.c'
AXIS_SOURCE = ROOT/'src/eng1/player_axis_motion.c'
BOUNDARY_SOURCE = ROOT/'src/eng1/player_boundary_19c30.c'
ORACLE = ROOT/'assets/pc/HERCULES/ENG1.DLL'
SPANS = {0x18a10:256,0x18ba0:16,0x18c30:32,0x18ed0:144,0x19140:32,
         0x191e0:96,0x19260:48,0x19290:352,0x193f0:80,0x19480:464,0x19c20:16,
         0x19a10:144,0x19aa0:160,0x19c30:192,0x19d30:64}
AXES = {0x19a10:'eng1_19a10',0x19aa0:'eng1_19aa0'}
BOUNDARIES = {0x19c30:'eng1_19c30',0x19d30:'eng1_19d30'}
def symbol_for_entry(entry):
    return '_'+{**AXES,**BOUNDARIES}[entry] if entry in AXES or entry in BOUNDARIES else f'_eng1_native_code_target_00{entry:05x}'
SERVICES = {
 'dispatch_local_15d00':(0x15d00,0), 'dispatch_local_16790':(0x16790,0),
 'unlink_0e4':(0x29620,1),'eng1_29c20':(0x29c20,1),'eng1_29e20':(0x29e20,1),
 'title_04ea0':(0x3870,1),'eng1_17340':(0x17340,0),'eng1_176e0':(0x176e0,0),
 'eng1_17ff0':(0x17ff0,0),'eng1_17ba0':(0x17ba0,0),'eng1_16fa0':(0x16fa0,0),
 'eng1_16990':(0x16990,0),'eng1_17d00':(0x17d00,0),'eng1_17a20':(0x17a20,0),
 'eng1_16e30':(0x16e30,0),'eng1_19d70':(0x19d70,0),'eng1_19aa0':(0x19aa0,0),
 'eng1_19a10':(0x19a10,0),'dispatch_local_30010':(0x30010,4),'grid_scan_20b50':(0x20b50,4),
 'eng1_19c30':(0x19c30,0),'eng1_19d30':(0x19d30,0),
}
# Physical cells only; fixture storage does not establish production ownership.
CELLS = {
 'g_normal_actor_pool':(0x72860,4),'g_collision_context':(0x7217c,4),
 'eng1_sim_state_72108':(0x72108,2),'eng1_sim_state_72110':(0x72110,4),
 'eng1_sim_state_6feb4':(0x6feb4,2),'g_eng1_6ef70':(0x6ef70,2),
 'g_eng1_setup_word_6ef80':(0x6ef80,2),'g_eng1_setup_word_6ef84':(0x6ef84,2),
 'g_eng1_setup_word_72158':(0x72158,2),'g_eng1_word_5cf54':(0x5cf54,2),
 'g_eng1_setup_dword_7201c':(0x7201c,4),'g_eng1_setup_dword_72118':(0x72118,4),
 'g_eng1_setup_dword_7211c':(0x7211c,4),
 'g_camera_flags_70d10':(0x70d10,2),'g_active_category_70cfe':(0x70cfe,1),
 'g_root_group_rows_70d5c':(0x70d5c,16),'g_rebuilt_row_offsets_70e0c':(0x70e0c,64),
}
IMAGES = {f'g_eng1_image_{r:x}':r for r in (0x546b0,0x54c30,0x54b50,0x53e80,0x53e58)}
STACK, ESP, STOP = 0x62000000,0x62008000,0x6200f000
DATA = 0x61000000
ACTORS = [DATA+i*0x1000+0x100 for i in range(4)]
CONTEXTS = [DATA+0x5000,DATA+0x6000]
NV = (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
SEEDS = (0x11223344,0x22334455,0x33445566,0x44556677)
pack = lambda value: struct.pack('<I',value & 0xffffffff)

def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    assert digest(ORACLE.read_bytes())=='01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    runtime=Path(unicorn.__file__).parent/'lib/unicorn.dll'
    assert unicorn.__version__=='2.1.4' and digest(runtime.read_bytes())=='010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    sources=[SOURCE,AXIS_SOURCE,BOUNDARY_SOURCE]
    paths={ORACLE,Path(__file__).resolve(),runtime,ROOT/'recovery.json',ROOT/'toolchains/manifest.json'}
    for source in sources:
        paths.update(context_files(source))
    pins={str(p):digest(p.read_bytes()) for p in paths}
    out.mkdir(parents=True,exist_ok=True)
    fixture=out/'services.c'
    lines=['/* Synthetic intercepted services and storage, never production code. */']
    for name in SERVICES:
        if name not in (*AXES.values(),*BOUNDARIES.values()):
            lines.append('void __cdecl '+name+'(void) { for (;;) {} }')
    for name,(_,size) in CELLS.items():
        if size in (1,2,4):
            lines.append({1:'unsigned char ',2:'unsigned short ',4:'unsigned long '}[size]+name+';')
        else:
            lines.append('unsigned char '+name+'['+str(size)+'];')
    lines += ['unsigned long g_engine_interface[250];']
    lines += ['unsigned char '+name+'[1];' for name in IMAGES]
    fixture.write_text('\n'.join(lines)+'\n')
    objects=[];commands=[]
    for source in sources:
        obj,command=compile_source(source,['/O2'])
        objects.append(obj);commands.append(command)
    fixobj,fixcommand=compile_source(fixture,['/O2'])
    image,mapfile=out/'player_modes.dll',out/'player_modes.map'
    link=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/NOLOGO','/DLL','/NOENTRY','/NODEFAULTLIB',
          '/INCREMENTAL:NO','/OPT:NOREF','/BASE:0x20000000','/OUT:'+str(image),'/MAP:'+str(mapfile),*map(str,objects),str(fixobj)]
    env=os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_'):
        env.pop(key,None)
    result=subprocess.run(link,cwd=out,env=env,capture_output=True,text=True,timeout=60)
    (out/'link.log').write_text(result.stdout+result.stderr)
    if result.returncode:
        raise ValueError(result.stdout+result.stderr)
    symbols=read_map(mapfile)
    assert pins=={p:digest(Path(p).read_bytes()) for p in pins}
    return dict(input_sha256=pins,commands=[*commands,fixcommand,link],
        objects=[dict(source=str(source),object=str(obj),object_sha256=digest(obj.read_bytes())) for source,obj in zip(sources,objects)],
        fixture_sha256=digest(fixture.read_bytes()),
        image_sha256=digest(image.read_bytes())), PE(image), symbols

def execute(original,candidate,symbols,case):
    native=PE(ORACLE)
    pe=native if original else candidate
    base=pe.image_base
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    size=(pe.pe.OPTIONAL_HEADER.SizeOfImage+0xfff)&~0xfff
    u.mem_map(base,size)
    u.mem_write(base,pe.pe.get_memory_mapped_image().ljust(size,b'\0'))
    u.mem_map(DATA,0x10000);u.mem_map(STACK,0x10000)
    cells={name:base+rva if original else symbols['_'+name] for name,(rva,n) in CELLS.items()}
    cells['context']=base+0x70f24 if original else symbols['_g_engine_interface']+4
    widths={name:n for name,(r,n) in CELLS.items()};widths['context']=4
    images={name:base+rva if original else symbols['_'+name] for name,rva in IMAGES.items()}
    reverse_images={value:0x10000000+IMAGES[name] for name,value in images.items()}
    services={base+rva if original else symbols['_'+name]:(name,argc) for name,(rva,argc) in SERVICES.items()}
    def w32(address,value):u.mem_write(address,pack(value))
    def get32(address):return struct.unpack('<I',u.mem_read(address,4))[0]
    def put(name,value):u.mem_write(cells[name],(value%(1<<(8*widths[name]))).to_bytes(widths[name],'little'))
    def normalize(value):return reverse_images.get(value,value)
    for i,address in enumerate(ACTORS):
        u.mem_write(address,bytes((j*13+i*37+11)&255 for j in range(0x140)))
        w32(address+0xe4,0)
        w32(address+0x64,images['g_eng1_image_'+case.get('image','54c30')])
        w32(address,0xffff0000 if i==0 else 0x1230000)
        w32(address+4,0x450000);w32(address+8,0xffe00000)
        u.mem_write(address+0x38,b'\x03\x00')
        u.mem_write(address+0x32,struct.pack('<h',case.get('phase',9)))
        u.mem_write(address+0x94,bytes([case.get('dx',0),case.get('dy',0),0,case.get('kind',8)]))
        if case['entry'] in AXES or case['entry'] in BOUNDARIES or case.get('compose_axes'):
            for offset in (0xa8,0xb8):
                for j,key in enumerate(('velocity','target','rate','limit')):
                    w32(address+offset+4*j,case.get(key,0))
            w32(address,case.get('position',0xfffffff0))
            w32(address+4,case.get('position',0xfffffff0))
    chain=case.get('chain',0)
    if chain:
        w32(ACTORS[1]+0xe4,ACTORS[2])
    if chain>1:
        w32(ACTORS[2]+0xe4,ACTORS[3])
    for address in CONTEXTS:
        u.mem_write(address,bytes(0x100))
    # Scenario keys retain the earlier byte-fixture labels at 5c/5d/5f;
    # production input is two WORDs, not three independent BYTE objects.
    u.mem_write(CONTEXTS[0]+0x5c,bytes([case.get('held',0),case.get('pressed',0),0,case.get('released',0)]))
    u.mem_write(CONTEXTS[1]+0x5c,bytes([case.get('held2',0),0x50,0,0]))
    for name in CELLS:put(name,0)
    put('g_normal_actor_pool',ACTORS[0]);put('g_collision_context',ACTORS[1])
    put('g_eng1_setup_dword_72118',ACTORS[0]+4 if case.get('alias_source') else ACTORS[2])
    put('context',CONTEXTS[0]);put('g_eng1_setup_word_72158',case.get('timer',0))
    put('g_eng1_word_5cf54',case.get('word',3));put('g_eng1_setup_word_6ef84',case.get('guard',0))
    put('eng1_sim_state_72110',case.get('active',0));put('g_eng1_setup_dword_7201c',0x12345678)
    put('g_camera_flags_70d10',case.get('camera_flags',0))
    category=case.get('category',0)
    assert 0<=category<4, 'Only the four synthetic header slots are modeled'
    put('g_active_category_70cfe',category)
    for i in range(4):
        header=DATA+0x7000+0x20*i
        u.mem_write(header,bytes(8))
        u.mem_write(header+4,struct.pack('<HH',case.get('width',16),case.get('height',12)))
        w32(cells['g_root_group_rows_70d5c']+4*i,header)
    u.mem_write(cells['g_rebuilt_row_offsets_70e0c'],struct.pack('<h',case.get('columns',10)))
    u.mem_write(cells['g_rebuilt_row_offsets_70e0c']+32,struct.pack('<h',case.get('row',2)))
    w32(ESP,STOP);u.mem_write(ESP+4,bytes([0x91])*32)
    u.reg_write(UC_X86_REG_ESP,ESP);u.reg_write(UC_X86_REG_EFLAGS,0x202)
    for register,value in zip(NV,SEEDS):u.reg_write(register,value)
    events,writes=[],[]
    counters={}
    entry=case['entry']
    start=base+entry if original else symbols[symbol_for_entry(entry)]
    limit=SPANS[entry] if original else 0x2000
    # Candidate execution is bounded by the exact emitted COMDAT extent below.
    if not original:
        limit=case['candidate_size']
    allowed=[(start,start+limit)]
    if case.get('compose_axes'):
        for axis in AXES:
            address=base+axis if original else symbols[symbol_for_entry(axis)]
            extent=SPANS[axis] if original else case['axis_candidate_sizes'][str(axis)]
            allowed.append((address,address+extent))
    if case.get('compose_boundaries'):
        for boundary in BOUNDARIES:
            address=base+boundary if original else symbols[symbol_for_entry(boundary)]
            extent=SPANS[boundary] if original else case['boundary_candidate_sizes'][str(boundary)]
            allowed.append((address,address+extent))
    def on_write(machine,access,address,n,value,user):
        if ESP-0x1000<=address and address+n<=ESP:return
        for name,cell in cells.items():
            if cell<=address and address+n<=cell+widths[name]:
                writes.append(('cell',name,address-cell,n,normalize(value)));return
        for i,actor in enumerate(ACTORS):
            if actor<=address and address+n<=actor+0x140:
                writes.append(('actor',i,address-actor,n,normalize(value)));return
        raise AssertionError(('unexpected write',case,hex(address),n))
    def on_code(machine,address,n,user):
        if address==STOP:
            machine.emu_stop();return
        if address in services and address!=start and not (
                case.get('compose_axes') and services[address][0] in AXES.values()) and not (
                case.get('compose_boundaries') and services[address][0] in BOUNDARIES.values()):
            name,argc=services[address]
            sp=machine.reg_read(UC_X86_REG_ESP)
            arguments=[get32(sp+4+4*i) for i in range(argc)]
            counters[name]=counters.get(name,0)+1
            returns=case.get('returns',{}).get(name,[0])
            result=returns[min(counters[name]-1,len(returns)-1)]
            events.append((name,arguments))
            if case.get('mutate')==name and counters[name]==1:
                put('g_normal_actor_pool',ACTORS[3]);put('g_collision_context',ACTORS[2]);put('context',CONTEXTS[1])
            machine.reg_write(UC_X86_REG_EAX,result&0xffffffff)
            machine.reg_write(UC_X86_REG_ECX,0xc1c1c1c1);machine.reg_write(UC_X86_REG_EDX,0xd2d2d2d2)
            machine.reg_write(UC_X86_REG_ESP,sp+4);machine.reg_write(UC_X86_REG_EIP,get32(sp));return
        assert any(lo<=address and address+n<=hi for lo,hi in allowed),('unexpected execution',case,hex(address))
    u.hook_add(UC_HOOK_MEM_WRITE,on_write);u.hook_add(UC_HOOK_CODE,on_code)
    u.emu_start(start,STOP,count=5000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==ESP+4
    assert all(u.reg_read(r)==v for r,v in zip(NV,SEEDS)) and not u.reg_read(UC_X86_REG_EFLAGS)&0x400
    assert get32(ESP)==STOP and bytes(u.mem_read(ESP+4,32))==bytes([0x91])*32
    snapshots=[]
    for actor in ACTORS:
        raw=bytearray(u.mem_read(actor,0x140))
        struct.pack_into('<I',raw,0x64,normalize(get32(actor+0x64)))
        snapshots.append(raw.hex())
    return dict(events=events,writes=writes,actors=snapshots,
        cells={name:bytes(u.mem_read(address,widths[name])).hex() for name,address in cells.items()},ABI=True)

def scenarios():
    for entry in SPANS:
        yield dict(entry=entry)
        yield dict(entry=entry,returns={'dispatch_local_15d00':[1],'eng1_29c20':[1],'eng1_29e20':[1]})
    for entry in (0x18a10,0x18c30,0x191e0,0x19290,0x193f0):
        for service in ('dispatch_local_15d00','title_04ea0','unlink_0e4','eng1_19d70'):
            yield dict(entry=entry,mutate=service,returns={'dispatch_local_15d00':[1],'eng1_29e20':[1]},chain=2)
    for service in ('eng1_17ff0','eng1_17ba0','eng1_16fa0','eng1_16990','eng1_17d00','eng1_17a20','eng1_16e30'):
        yield dict(entry=0x18ed0,returns={'eng1_29c20':[1],service:[1]})
    for chain,kind,active in itertools.product((0,1,2),(7,8),(0,1)):
        yield dict(entry=0x19290,chain=chain,kind=kind,active=active,returns={'eng1_29e20':[1],'dispatch_local_15d00':[1]})
    for left,right in itertools.product((0,1,0x80000000),(0,1,0x80000000)):
        yield dict(entry=0x19290,returns={'dispatch_local_30010':[left,right]})
    yield dict(entry=0x19260,alias_source=True)
    for phase,word,image in itertools.product((-1,8,9),(2,3),('54c30','53e58')):
        yield dict(entry=0x191e0,phase=phase,word=word,image=image,returns={'dispatch_local_15d00':[1]})
    yield dict(entry=0x18ed0,guard=1,returns={'eng1_29c20':[1],'dispatch_local_15d00':[1]})
    for image in ('53e80','53e58'):
        yield dict(entry=0x193f0,image=image,returns={'dispatch_local_15d00':[1]})
    for timer,held,pressed,released in itertools.product((-32768,-9,-1,0,5,6,32767),(0,0x10,0x40,0x80,0x20,0xf0),(0,3,0x50),(0,3)):
        yield dict(entry=0x19480,timer=timer,held=held,pressed=pressed,released=released)
    for service,held in (('eng1_19aa0',0xf0),('eng1_19a10',0x80)):
        yield dict(entry=0x19480,held=held,mutate=service,held2=0xf0)
    for held,dx,dy in itertools.product((0x10,0x40,0x80,0x20),(0x10,0xff),(1,0xff)):
        yield dict(entry=0x19480,held=held,dx=dx,dy=dy,pressed=0x50)
    for result in (1,0x10000,0x80000000,0x8001):
        yield dict(entry=0x19480,timer=6,returns={'grid_scan_20b50':[result]})
    boundaries=[(0,0,1,1),(0x7fffffff,0x7fffffff,1,1),(-1,-1,1,1),
        (5,3,1,1),(5,3,1,4),(5,3,1,2),(1,3,1,1),(1,3,4,1),(1,3,2,1),
        (-5,-3,1,1),(-5,-3,4,1),(-1,-3,1,4),
        (0x80000000,0,0xffffffff,1),(0x7fffffff,0,1,0xffffffff)]
    for entry,direction,values in itertools.product(AXES,(0,1,0xff),boundaries):
        v,target,rate,limit=values
        yield dict(entry=entry,dx=direction,dy=direction,velocity=v,target=target,rate=rate,limit=limit)
    for held,dx,dy in itertools.product((0x10,0x40,0x80,0x20,0xf0),(0,0xff),(0,1)):
        yield dict(entry=0x19480,compose_axes=True,held=held,pressed=0x53,dx=dx,dy=dy,
            velocity=0x8000,target=0x10000,rate=0x6000,limit=0x4000)
    for service in ('eng1_19c30','eng1_19d30'):
        yield dict(entry=0x19480,compose_axes=True,held=0xf0,pressed=0x53,mutate=service,
            held2=0xf0,velocity=0x8000,target=0x10000,rate=0x6000,limit=0x4000)
    x_positions=(0x80000000,0,0x3fffff,0x400000,0xc00000,0xc00001,
        0x13fffff,0x1400000,0x1400001,0x23fffff,0x2400000,0x2400001,
        0x33fffff,0x3400000,0x3400001,0x3bfffff,0x3c00000,0x3c00001,0x7fffffff)
    for position,flags in itertools.product(x_positions,(0,1)):
        yield dict(entry=0x19c30,position=position,camera_flags=flags)
    for width,position in itertools.product((0,1,2,511,65535),(0x400000,0x7fffffff)):
        yield dict(entry=0x19c30,width=width,position=position)
    for height,position in itertools.product((0,1,12,511,65535),(0,0x3fffff,0x400000,0x3000000,0x3000001,0x80000000,0x7fffffff)):
        yield dict(entry=0x19d30,height=height,position=position)
    for category,position in itertools.product((0,3),(0,0x400000,0x1400000,0x3c00001)):
        yield dict(entry=0x19480,compose_axes=True,compose_boundaries=True,held=0xf0,pressed=0x53,
            category=category,position=position,velocity=0x8000,target=0x10000,rate=0x6000,limit=0x4000)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'work/player_modes_differential_2026_10_25')
    args=parser.parse_args();out=args.output.resolve()
    assert out.is_relative_to(ROOT/'work')
    build_report,candidate,symbols=build(out)
    from coff import COFF
    coffs=[COFF(row['object']) for row in build_report['objects']]
    sizes={}
    for entry in SPANS:
        symbol=symbol_for_entry(entry)
        sections=[coff.function(symbol) for coff in coffs if any(s['name']==symbol and s['section']>0 for s in coff.symbols.values())]
        assert len(sections)==1
        sizes[entry]=sections[0]['size']
    results=[]
    for case in scenarios():
        case['candidate_size']=sizes[case['entry']]
        if case.get('compose_axes'):
            case['axis_candidate_sizes']={str(entry):sizes[entry] for entry in AXES}
        if case.get('compose_boundaries'):
            case['boundary_candidate_sizes']={str(entry):sizes[entry] for entry in BOUNDARIES}
        native=execute(True,candidate,symbols,case)
        source=execute(False,candidate,symbols,case)
        if native!=source:
            (out/'failure.json').write_text(json.dumps(dict(case=case,native=native,source=source),indent=2)+'\n')
            raise AssertionError(('native/source difference',case,sorted(k for k in native if native[k]!=source[k])))
        results.append(dict(case=case,events=len(native['events']),writes=len(native['writes']),equal=True))
    assert build_report['input_sha256']=={p:digest(Path(p).read_bytes()) for p in build_report['input_sha256']}
    report=dict(scope='Finite isolated handlers and explicit controller/axis/boundary compositions over four synthetic header slots. Other services modeled. No full data ownership, whole-game or function-matching acceptance.',
        build=build_report,cases=results,case_count=len(results),all_equal=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(results),all_equal=True,report=str(out/'report.json'))))

if __name__=='__main__':main()
