"""Fresh constructor flow with actual sample decoder and bank release."""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_sound_bank_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args()
OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve())
OUT.mkdir(parents=True,exist_ok=True)
sys.path[:0] = [str(ROOT/'scripts'),'C:/tools/hercules/python',
                'C:/tools/hercules/renderer-oracle-python']
from match import compile_source, compare, verify_toolchain, DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ORACLE = args.native_exe.resolve()
sha = lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
pack = lambda v:struct.pack('<I',v&0xffffffff)
STACK,ESP,STOP = 0x62000000,0x62008000,0x6200f000
API,HEAP,HEAP_SIZE = 0x65000000,0x66000000,0x30000
DEVICE,DEVICE_VT,BUFFER_VT = HEAP,HEAP+0x1000,HEAP+0x2000
BUFFER_BASE,BLOCK1,BLOCK2 = HEAP+0x10000,HEAP+0x20000,HEAP+0x21000
FIELDS = {
    'header':('_g_host_sound_header_699838',0x699838,16),
    'counter':('_g_host_sound_bank_counters_698ed8',0x698ed8,2400),
    'scratch':('_g_host_sound_scratch_699848',0x699848,0x100000),
    'loops':('_g_host_sound_loop_flags_698a18',0x698a18,1200),
    'bank':('_g_exe_799908',0x799908,19200),
    'device':('_g_exe_79e440',0x79e440,4),
    'release_flag':('_g_audio_cleanup_iteration_flag',0x79e450,1),
}
PROFILE = {
    'constructor':('_host_sound_bank_initialize_f3d0',0xf3d0,832),
    'decoder':('_host_sound_decode_2410',0x2410,288),
    'release':('_cleanup_native_f710',0xf710,80),
}
METHODS = {
    'Create':(0x0c,12),'Duplicate':(0x14,8),'Stop':(0x48,0),
    'Release':(8,0),'Position':(0x34,4),'Frequency':(0x44,4),
    'Pan':(0x40,4),'Volume':(0x3c,4),'Lock':(0x2c,28),'Unlock':(0x4c,16),
}


def build():
    assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    verify_toolchain(DEFAULT_TOOLCHAIN)
    files=[ROOT/'src/pc/host_sound_bank.cpp',ROOT/'calibration/host_sound_bank_storage.c',
           *[ROOT/p for p in ['src/pc/host_sound_decoder.c',
            'calibration/host_sound_decoder_tables.c','calibration/audio_bank_release.cpp',
            'calibration/host_sound_channel_storage.cpp','calibration/sound_bank_storage.cpp',
            'calibration/archive_storage.c']]]
    libraries=[DEFAULT_TOOLCHAIN/'lib'/p for p in ['libcmt.lib','kernel32.lib','user32.lib']]
    pins={str(p):sha(p)for p in [*files,*libraries,ORACLE,Path(__file__),
          ROOT/'calibration/host_sound_bank.h',ROOT/'calibration/host_sound_decoder_tables.h',
          ROOT/'calibration/host_sound_channel_storage.h',ROOT/'toolchains/manifest.json',
          DEFAULT_TOOLCHAIN/'include/dsound.h',DEFAULT_TOOLCHAIN/'include/mmsystem.h',
          *[ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py']]]}
    fixture=OUT/'fixture.c'
    fixture.write_bytes(b'''#include <stddef.h>
short g_host_sound_scratch_699848[524288];
int __cdecl archive_open(const char*a,const char*b) { for (;;) {} }
size_t __cdecl archive_read(void*a,size_t b,size_t c,int d) { for (;;) {} }
int __cdecl archive_close(int a) { for (;;) {} }
void __cdecl archive_fatal(const char*a) { for (;;) {} }
int __cdecl main(void) { for (;;) {} }
extern void __cdecl host_sound_bank_initialize_f3d0(unsigned int);
__declspec(dllexport) void anchor(void) { host_sound_bank_initialize_f3d0(0); }
''')
    objects,compiles,definitions,owners=[],[],{},{}
    for source in [*files,fixture]:
        obj,command=compile_source(source,['/O2'])
        objects.append(obj)
        compiles.append(dict(source=str(source),source_sha256=sha(source),object=str(obj),
                             object_sha256=sha(obj),command=command))
        co=COFF(obj)
        for symbol in co.symbols.values():
            if symbol['storage']==2 and symbol['section']>0:
                owners.setdefault(symbol['name'],[]).append((co,symbol,source))
            if symbol['storage']==2 and symbol['section']>0 and symbol['type']&0x20:
                assert symbol['name']not in definitions
                definitions[symbol['name']]=(obj,co.function(symbol['name'])['size'])
    dll,mp=OUT/'fixture.dll',OUT/'fixture.map'
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY',
             '/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF',
             '/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),*map(str,libraries)]
    env=os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:env.pop(k,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    process=subprocess.run(command,env=env,capture_output=True,text=True)
    (OUT/'link.log').write_bytes((process.stdout+process.stderr).encode())
    assert process.returncode==0,process.stdout+process.stderr
    regex=re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
    symbols={m[1]:int(m[2],16)for line in mp.read_text().splitlines()if(m:=regex.match(line))}
    comparisons=[]
    for role,(symbol,rva,width)in PROFILE.items():
        obj,size=definitions[symbol]
        comparisons.append(dict(role=role,symbol=symbol,source_size=size,
            raw=compare(obj,symbol,ORACLE,rva,width),
            diagnostic=masked_compare(obj,symbol,ORACLE,rva,width)))
    # Literal identity is independent of format modeling and caller similarity.
    original,linked=pefile.PE(str(ORACLE)),pefile.PE(str(dll))
    for symbol,rva,width in [('_g_host_sound_filename_format',0x3a510,len(b'm:\\language\\sonix\\pc_%ld.esf')+1),
                             ('_g_host_sound_create_error',0x3a500,len(b'PC_CreateSFX(1)')+1)]:
        assert linked.get_data(symbols[symbol]-linked.OPTIONAL_HEADER.ImageBase,width)==original.get_data(rva,width),(symbol,width)
    data=[]
    release_proof=next(c for c in comparisons if c['role']=='release')
    assert int(release_proof['diagnostic']['symbols']['_g_audio_cleanup_iteration_flag'],0)==FIELDS['release_flag'][1]
    for role,(name,rva,width)in FIELDS.items():
        if role=='scratch':continue
        assert len(owners[name])==1
        co,symbol,provider=owners[name][0];section=co.sections[symbol['section']-1]
        assert not section['relocations']and symbol['value']+width<=section['size']
        value=section['data'][symbol['value']:symbol['value']+width]if section['data']else bytes(width)
        target=next(t for t in original.sections if t.VirtualAddress<=rva and rva+width<=t.VirtualAddress+t.Misc_VirtualSize)
        assert rva>=target.VirtualAddress+target.SizeOfRawData and value==bytes(width)
        data.append(dict(role=role,symbol=name,rva=hex(rva),bytes=width,provider=str(provider),source_kind='BSS',native_backing='virtual_loader_zero'))
    for name,rva,width in [('_g_host_sound_index_deltas_35e08',0x35e08,64),('_g_host_sound_steps_35e48',0x35e48,356)]:
        assert len(owners[name])==1
        co,symbol,provider=owners[name][0];section=co.sections[symbol['section']-1]
        assert not section['relocations']
        value=section['data'][symbol['value']:symbol['value']+width]
        assert value==original.get_data(rva,width)
        data.append(dict(role='decoder_table',symbol=name,rva=hex(rva),bytes=width,provider=str(provider),raw_initialized_equal=True))
    assert pins=={p:sha(Path(p))for p in pins}
    return dict(inputs=pins,compiles=compiles,comparisons=comparisons,symbols=symbols,
                dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),link_command=command,data=data)


def execute(kind,b,s):
    native=kind=='native'
    image=pefile.PE(str(ORACLE if native else b['dll']))
    base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    u.mem_map(base,(size+4095)&~4095)
    u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
    u.mem_map(STACK,0x10000);u.mem_map(API,0x1000);u.mem_map(HEAP,HEAP_SIZE)
    fields={k:(0x400000+rva if native else b['symbols'][name],width)
            for k,(name,rva,width)in FIELDS.items()}
    get32=lambda a:int.from_bytes(u.mem_read(a,4),'little')
    def string(a):
        return bytes(u.mem_read(a,256)).split(b'\0')[0].decode('latin1')
    for name,(a,width)in fields.items():
        u.mem_write(a,bytes((i*29+(i>>8)*17+43)&255 for i in range(width)))
    u.mem_write(fields['bank'][0],bytes(19200))
    u.mem_write(fields['device'][0],pack(0 if s.get('no_device')else DEVICE))
    u.mem_write(fields['release_flag'][0],bytes([s.get('release_flag',0)]))
    index=s.get('index',1)
    u.mem_write(HEAP,bytes([0x91])*HEAP_SIZE)
    u.mem_write(DEVICE,pack(DEVICE_VT))
    for token in range(8):u.mem_write(BUFFER_BASE+token*16,pack(BUFFER_VT)+pack(token)+bytes(8))
    endpoints={}
    for i,(name,(offset,extra))in enumerate(METHODS.items()):
        address=API+i*16;endpoints[address]=name
        u.mem_write((DEVICE_VT if name in ['Create','Duplicate']else BUFFER_VT)+offset,pack(address))
    if s.get('loaded')and index<1200:u.mem_write(fields['bank'][0]+index*16,pack(BUFFER_BASE))
    functions={role:0x400000+rva if native else b['symbols'][name]
               for role,(name,rva,width)in PROFILE.items()}
    widths={x['role']:x['source_size']for x in b['comparisons']}
    code_ranges=[(functions[r],functions[r]+(width if native else widths[r]))
                 for r,(name,rva,width)in PROFILE.items()]
    for name,rva,symbol in [('format',0x10f40,'_sprintf'),('open',0x5130,'_archive_open'),
                            ('read',0x5210,'_archive_read'),('close',0x51f0,'_archive_close'),
                            ('fatal',0x10460,'_archive_fatal')]:
        endpoints[0x400000+rva if native else b['symbols'][symbol]]=name
    header=struct.pack('<4I',s.get('magic',0x03465345),s.get('sample_bytes',32),
                       s.get('rate_word',0x12345622),s.get('flags',1))
    packed=bytes.fromhex(s.get('packed','0123456789abcdef'))
    events,errors,counts,entries=[],[],{},[]
    fatal=False
    u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100)
    u.mem_write(ESP,pack(STOP)+pack(index))
    u.reg_write(UC_X86_REG_ESP,ESP)
    nv=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
    seeds=[0x11223344,0x22334455,0x33445566,0x44556677]
    for reg,value in zip(nv,seeds):u.reg_write(reg,value)
    def mutation(role):
        i=counts.get(role,0);counts[role]=i+1
        items=s.get('mutations',{}).get(role,[]);action=items[i]if i<len(items)else{}
        if 'flags'in action:u.mem_write(fields['header'][0]+12,pack(action['flags']))
        if 'sample_bytes'in action:u.mem_write(fields['header'][0]+4,pack(action['sample_bytes']))
        if 'first_token'in action:u.mem_write(fields['bank'][0]+index*16,pack(BUFFER_BASE+16*action['first_token']))
        return action
    def ret(sp,extra,value):
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xdeadbeef)
        u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,get32(sp))
    def on_code(machine,a,width,_):
        nonlocal fatal
        if a==STOP:machine.emu_stop();return
        if a in functions.values():entries.append(next(r for r,v in functions.items()if v==a))
        role=endpoints.get(a)
        if not role:
            if not any(lo<=a<hi for lo,hi in code_ranges):errors.append(['execution',hex(a)]);machine.emu_stop()
            return
        sp=u.reg_read(UC_X86_REG_ESP)
        arg=lambda n:get32(sp+4*n)
        if role=='format':
            fmt=string(arg(2));assert fmt=='m:\\language\\sonix\\pc_%ld.esf',fmt
            text=fmt.replace('%ld',str(arg(3)));target=arg(1)
            assert ESP-0x1000<=target and target+len(text)+1<=ESP
            u.mem_write(target,text.encode()+b'\0');events.append(['format',arg(3),text]);mutation(role);ret(sp,0,len(text));return
        if role=='open':
            events.append(['open',string(arg(1)),string(arg(2))]);mutation(role);ret(sp,0,3);return
        if role=='read':
            target,size,count,handle=arg(1),arg(2),arg(3),arg(4);assert handle==3 and count==1
            if counts.get('read',0)==0:
                assert target==fields['header'][0]and size==16
                payload=header
                where='header'
            else:
                offset=target-fields['scratch'][0]
                assert 0<=offset and offset+size<=0x100000 and size<=len(packed)
                payload=packed[:size];where=offset
            u.mem_write(target,payload);events.append(['read',where,size,count,handle]);mutation(role);ret(sp,0,1);return
        if role=='close':events.append(['close',arg(1)]);mutation(role);ret(sp,0,0);return
        if role=='fatal':
            events.append(['fatal',string(arg(1))]);fatal=True;machine.emu_stop();return
        receiver=arg(1)
        action=mutation(role)
        token=(receiver-BUFFER_BASE)//16
        if role in ['Create','Duplicate']:assert receiver==DEVICE
        else:assert 0<=token<8 and receiver==BUFFER_BASE+token*16
        if role=='Create':
            desc,target,outer=arg(2),arg(3),arg(4);assert outer==0 and target==fields['bank'][0]+index*16
            assert ESP-0x1000<=desc and desc+20<=ESP
            raw=bytearray(u.mem_read(desc,20));pcm=get32(desc+16)
            assert ESP-0x1000<=pcm and pcm+16<=ESP
            raw[16:20]=bytes(4);events.append(['Create',raw.hex(),bytes(u.mem_read(pcm,16)).hex()])
            if not s.get('create_null'):u.mem_write(target,pack(BUFFER_BASE))
        elif role=='Duplicate':
            source,target=arg(2),arg(3);slot=(target-fields['bank'][0])//4
            lane=slot-index*4;assert lane in [1,2,3]
            assert BUFFER_BASE<=source<BUFFER_BASE+128
            events.append(['Duplicate',(source-BUFFER_BASE)//16,lane])
            u.mem_write(target,pack(0 if lane==s.get('duplicate_null')else BUFFER_BASE+lane*16))
        elif role=='Lock':
            assert arg(2)==0
            first,bytes1,second,bytes2=arg(4),arg(5),arg(6),arg(7)
            assert arg(8)==0
            length=arg(3)
            for pointer in [first,bytes1,second,bytes2]:assert ESP-0x1000<=pointer and pointer+4<=ESP
            u.mem_write(first,pack(0 if s.get('null_block')else BLOCK1))
            u.mem_write(bytes1,pack(s.get('lock_bytes',length)))
            u.mem_write(second,pack(BLOCK2 if s.get('split')else 0))
            u.mem_write(bytes2,pack(s.get('split',0)))
            events.append(['Lock',token,length,s.get('lock_bytes',length),s.get('split',0)])
        elif role=='Unlock':
            assert arg(2)==BLOCK1
            events.append(['Unlock',token,arg(3),arg(4)==BLOCK2,arg(5)])
        elif role in ['Position','Frequency','Pan','Volume']:
            assert arg(2)==0;events.append([role,token,0])
        else:events.append([role,token])
        ret(sp,METHODS[role][1]+4,action.get('result',s.get('com_result',0)))
    def on_write(machine,access,a,width,value,_):
        allowed=[(ESP-0x1000,ESP+8),(BLOCK1,BLOCK1+256)]
        allowed.extend((lo,lo+n)for k,(lo,n)in fields.items()if k not in ['device'])
        if not any(lo<=a and a+width<=hi for lo,hi in allowed):errors.append(['write',hex(a),width]);machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,on_code);u.hook_add(UC_HOOK_MEM_WRITE,on_write)
    u.emu_start(functions['constructor'],STOP,count=1000000)
    checks=dict(terminal=fatal==s.get('fatal',False)and(fatal or u.reg_read(UC_X86_REG_EIP)==STOP),
                stack=fatal or u.reg_read(UC_X86_REG_ESP)==ESP+4,
                nonvolatiles=fatal or all(u.reg_read(reg)==v for reg,v in zip(nv,seeds)),
                guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+8,32))==bytes([0xa7])*32,
                whitelist=not errors)
    assert all(checks.values()),(kind,s,checks,errors)
    # Normalize only pointer-bank values to synthetic COM tokens; they are already identical.
    state={k:hashlib.sha256(bytes(u.mem_read(a,n))).hexdigest()for k,(a,n)in fields.items()if k!='device'}
    result=dict(state=state,heap_sha256=hashlib.sha256(bytes(u.mem_read(HEAP,HEAP_SIZE))).hexdigest(),
                events=events,entries=entries,checks=checks,fatal=fatal)
    if s.get('expect_no_calls'):assert not events
    return result


def scenarios():
    result=[dict(name='normal16'),dict(name='normal8',flags=0x0101),
            dict(name='null_device',no_device=True),dict(name='null_created_buffer',create_null=True),
            dict(name='already_loaded',loaded=True,expect_no_calls=True),
            dict(name='bad_magic',magic=0,fatal=True),dict(name='empty_lock',lock_bytes=0),
            dict(name='null_lock',null_block=True),dict(name='split_lock',split=7),
            dict(name='ignored_com_errors',com_result=0x80004005),
            dict(name='header_upper_bits',rate_word=0xffff5622,flags=0xabcd0001),
            dict(name='partial_odd_sample',sample_bytes=6),
            dict(name='position_retargets_buffer',mutations={'Position':[{'first_token':4}]}),
            dict(name='create_mutates_header',mutations={'Create':[{'flags':0x0107,'sample_bytes':16}]}),
            dict(name='lock_mutates_pcm_mode',mutations={'Lock':[{'flags':0x0101}]}),
            dict(name='unlock_mutates_metadata',mutations={'Unlock':[{'flags':0x0080}]})]
    for index in [0,824,999,1000,1199,1200,0xffffffff]:
        result.append(dict(name='index_'+str(index),index=index,expect_no_calls=index>=1200))
    for lane in [1,2,3]:
        result.append(dict(name='duplicate_fail_'+str(lane),duplicate_null=lane))
    for flag in [0,1,128]:
        result.append(dict(name='protected_duplicate_fail_'+str(flag),index=824,duplicate_null=2,release_flag=flag))
    return result


def main():
    b,cases=build(),[]
    for s in scenarios():
        n=execute('native',b,s);c=execute('source',b,s)
        assert n==c,(s,n,c)
        cases.append(dict(scenario=s,native=n,source=c,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual original/fresh constructor,decoder and bankrelease;archive,format,fatal and COMeffects explicitly modeled.1MiB scratch is fixture backing only,not a production capacity.',
                build=b,cases=cases,case_count=len(cases),all_equal=True)
    p=OUT/'sound_bank_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(case_count=len(cases),all_equal=True,report=str(p))))


if __name__=='__main__':main()
