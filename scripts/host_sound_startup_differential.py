"""Sound startup with actual x87/CRT conversion and modeled DirectSound only."""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from decimal import Decimal,localcontext
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_sound_startup_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args=parser.parse_args()
OUT=args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve())
OUT.mkdir(parents=True,exist_ok=True)
sys.path[:0]=[str(ROOT/'scripts'),'C:/tools/hercules/python','C:/tools/hercules/renderer-oracle-python']
from match import compile_source,compare,verify_toolchain,DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
from title_behavioral import context_files
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ORACLE=args.native_exe.resolve()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
pack=lambda v:struct.pack('<I',v&0xffffffff)
STACK,ESP,STOP,API,HEAP=0x62000000,0x62008000,0x6200f000,0x65000000,0x66000000
DEVICES=[HEAP+0x100,HEAP+0x200]
BUFFERS=[HEAP+0x300,HEAP+0x400]
DEV_VT,BUF_VT=HEAP+0x1000,HEAP+0x1100
FIELDS={
 'table':('_g_host_sound_volume_table_690a18',0x690a18,32768),
 'lanes':('_g_host_sound_channel_lanes_79e408',0x79e408,48),
 'next':('_g_host_sound_next_slot_79e448',0x79e448,4),
 'format':('_g_host_sound_primary_format_698ec8',0x698ec8,16),
 'bank':('_g_exe_799908',0x799908,19200),
 'counters':('_g_host_sound_bank_counters_698ed8',0x698ed8,2400),
 'ids':('_g_host_audio_channel_ids_799848',0x799848,96),
 'cursors':('_g_host_audio_channel_cursors_7998a8',0x7998a8,96),
 'loops':('_g_host_sound_loop_flags_698a18',0x698a18,1200),
 'scratch':('_g_host_sound_scratch_699848',0x699848,1048576),
 'paused':('_g_host_sound_paused_79e44c',0x79e44c,4),
 'cleanup_flag':('_g_audio_cleanup_iteration_flag',0x79e450,1),
 'device':('_g_exe_79e440',0x79e440,4),
 'primary':('_g_exe_79e444',0x79e444,4),
 'gain_left':('_g_exe_79e438',0x79e438,2),
 'gain_right':('_g_exe_79e43c',0x79e43c,2),
 'registry':('_g_registry_config',0x79e858,72),
 'window':('_g_fatal_owner_window',0x7a0200,4),
}
PROFILE={'startup':('_host_service_fdf0',0xfdf0,336),
         'format':('_host_sound_primary_format_fd50',0xfd50,128),
         'gain':('_audio_native_fdd0',0xfdd0,32)}


def build():
    assert sha(ORACLE)=='587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    verify_toolchain(DEFAULT_TOOLCHAIN)
    source=ROOT/'src/pc/host_sound_startup.cpp'
    files=[source,ROOT/'calibration/host_sound_startup_storage.c',*[ROOT/p for p in [
        'calibration/sound_bank_storage.cpp','calibration/host_sound_channel_storage.cpp',
        'calibration/host_warm_delivery_storage.cpp','calibration/registry_storage.c',
        'calibration/fatal_storage.c','calibration/host_sound_bank_storage.c',
        'calibration/host_sound_scratch_storage.c','calibration/audio_small_helpers.c',
        'calibration/audio_gain_storage.c']]]
    libraries=[DEFAULT_TOOLCHAIN/'lib'/p for p in ['libcmt.lib','kernel32.lib','user32.lib','dsound.lib']]
    assert sha(libraries[0])=='7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    assert sha(libraries[-1])=='4d6e8e1e0af841da16eb621f52ef40cab455b9bd31d4b9407a549e827d257773'
    watched={ORACLE,Path(__file__),ROOT/'toolchains/manifest.json',*libraries}
    for p in files:watched.update(context_files(p))
    watched.update(ROOT/'scripts'/p for p in ['match.py','coff.py','masked.py','pe.py','env.py','title_behavioral.py'])
    watched.update(DEFAULT_TOOLCHAIN/'include'/p for p in ['dsound.h','mmsystem.h','math.h'])
    pins={str(p):sha(p)for p in watched}
    # Identify the real conversion helper without rewriting or injecting it.
    payload=libraries[0].read_bytes();position,index=8,0
    while position<len(payload):
        header=payload[position:position+60];assert header[58:]==b'`\n'
        size=int(header[48:58]);member=payload[position+60:position+60+size]
        if index==602:break
        position+=60+size+(size&1);index+=1
    assert index==602
    memberpath=OUT/'ftol602.obj';memberpath.write_bytes(member)
    co=COFF(memberpath);code=next(s for s in co.sections if s['flags']&0x20 and s['size'])
    original=pefile.PE(str(ORACLE));assert code['size']==39 and not code['relocations']and code['data']==original.get_data(0x10d90,39)
    fixture=OUT/'fixture.c'
    fixture.write_bytes(b'''void __cdecl audio_native_fa10(int a,int b) { for (;;) {} }
int __cdecl main(void) { for (;;) {} }
extern void __cdecl host_service_fdf0(void);
extern unsigned long __cdecl host_sound_primary_format_fd50(unsigned long);
__declspec(dllexport) void anchor(void) { host_service_fdf0(); host_sound_primary_format_fd50(1); }
''')
    objects,compiles,definitions,owners=[],[],{},{}
    for p in [*files,fixture]:
        obj,command=compile_source(p,['/O2']);objects.append(obj)
        compiles.append(dict(source=str(p),source_sha256=sha(p),object=str(obj),object_sha256=sha(obj),command=command))
        co=COFF(obj)
        for sym in co.symbols.values():
            if sym['storage']==2 and(sym['section']>0 or sym['section']==0 and sym['value']>0):
                owners.setdefault(sym['name'],[]).append((co,sym,p))
                if sym['section']>0 and sym['type']&0x20:
                    assert sym['name']not in definitions
                    definitions[sym['name']]=(obj,co.function(sym['name'])['size'])
    dll,mp=OUT/'fixture.dll',OUT/'fixture.map'
    command=[str(DEFAULT_TOOLCHAIN/'bin/link.exe'),'/nologo','/DLL','/NOENTRY',
             '/NODEFAULTLIB','/INCREMENTAL:NO','/OPT:REF','/OPT:NOICF',
             '/OUT:'+str(dll),'/MAP:'+str(mp),*map(str,objects),*map(str,libraries)]
    env=os.environ.copy()
    for key in ['CL','_CL_','LINK','_LINK_']:env.pop(key,None)
    env['PATH']=str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    process=subprocess.run(command,env=env,capture_output=True,text=True)
    (OUT/'link.log').write_bytes((process.stdout+process.stderr).encode());assert process.returncode==0,process.stdout+process.stderr
    regex=re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
    symbols={m[1]:int(m[2],16)for line in mp.read_text().splitlines()if(m:=regex.match(line))}
    linked=pefile.PE(str(dll));assert linked.get_data(symbols['__ftol']-linked.OPTIONAL_HEADER.ImageBase,39)==code['data']
    assert any(x.name==b'DirectSoundCreate'for group in linked.DIRECTORY_ENTRY_IMPORT for x in group.imports)
    measurements=[]
    for role,(symbol,rva,width)in PROFILE.items():
        obj,size=definitions[symbol];measurements.append(dict(role=role,symbol=symbol,source_size=size,
            raw=compare(obj,symbol,ORACLE,rva,width),diagnostic=masked_compare(obj,symbol,ORACLE,rva,width)))
    data=[]
    for role,(name,rva,width)in FIELDS.items():
        assert len(owners[name])==1,name
        co,sym,provider=owners[name][0]
        if sym['section']==0:assert sym['value']==width;value=bytes(width);kind='COMMON'
        else:
            sec=co.sections[sym['section']-1];assert not sec['relocations']and sym['value']+width<=sec['size']
            value=sec['data'][sym['value']:sym['value']+width]if sec['data']else bytes(width);kind='BSS'if sec['flags']&0x80 else'initialized'
        sec=next(t for t in original.sections if t.VirtualAddress<=rva and rva+width<=t.VirtualAddress+t.Misc_VirtualSize)
        assert rva>=sec.VirtualAddress+sec.SizeOfRawData and value==bytes(width),role
        data.append(dict(role=role,symbol=name,rva=hex(rva),bytes=width,provider=str(provider),kind=kind,native_virtual_zero=True))
    assert pins=={p:sha(Path(p))for p in pins}
    return dict(inputs=pins,compiles=compiles,measurements=measurements,data=data,symbols=symbols,
                dll=str(dll),dll_sha256=sha(dll),map_sha256=sha(mp),link_command=command,
                conversion=dict(member=602,member_sha256=sha(memberpath),bytes=39,raw_native_equal=True,
                                raw_linked_equal=True,code_sha256=hashlib.sha256(code['data']).hexdigest()))


def reference_table():
    values=[-10000];nearest=Decimal(1)
    with localcontext()as ctx:
        ctx.prec=60
        for i in range(1,16384):
            real=(Decimal(i)/Decimal(16384)).log10()*1650
            value=max(Decimal(-10000),min(Decimal(0),real))-200
            values.append(int(value))
            nearest=min(nearest,abs(value-value.to_integral_value()))
    assert nearest>Decimal('1e-10'),str(nearest)
    return b''.join(struct.pack('<h',v)for v in values),str(nearest)


def execute(kind,b,s,reference):
    native=kind=='native';image=pefile.PE(str(ORACLE if native else b['dll']))
    base,size=image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(base,(size+4095)&~4095)
    u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'));u.mem_map(STACK,0x10000)
    u.mem_map(API,0x1000);u.mem_map(HEAP,0x2000)
    fields={k:(0x400000+rva if native else b['symbols'][name],width)for k,(name,rva,width)in FIELDS.items()}
    get32=lambda a:int.from_bytes(u.mem_read(a,4),'little')
    for key,(a,width)in fields.items():u.mem_write(a,bytes((i*29+(i>>8)*17+43)&255 for i in range(width)))
    def setfield(key,value):u.mem_write(fields[key][0],pack(value)[:fields[key][1]])
    setfield('window',0x12345678);setfield('device',DEVICES[0]if s.get('initial_device')else 0)
    setfield('primary',BUFFERS[0]if s.get('initial_primary')else 0);setfield('paused',0x89abcdef)
    setfield('next',0xaabbccdd);setfield('cleanup_flag',0x80)
    u.mem_write(fields['registry'][0]+16,pack(s.get('configured',0xabcd1234)))
    u.mem_write(fields['format'][0],struct.pack('<HHIIHH',7,5,s.get('previous_rate',44100),12345,9,8))
    before={k:bytes(u.mem_read(a,n))for k,(a,n)in fields.items()}
    for d in DEVICES:u.mem_write(d,pack(DEV_VT))
    for p in BUFFERS:u.mem_write(p,pack(BUF_VT))
    endpoints={}
    for i,(name,table,offset)in enumerate([('Cooperate',DEV_VT,0x18),('Create',DEV_VT,0x0c),('SetFormat',BUF_VT,0x38)]):
        address=API+i*16;endpoints[address]=name;u.mem_write(table+offset,pack(address))
    endpoints[0x410a26 if native else b['symbols']['_DirectSoundCreate@12']]='DirectSoundCreate'
    addresses={role:0x400000+rva if native else b['symbols'][name]for role,(name,rva,width)in PROFILE.items()}
    extents={row['role']:row['source_size']for row in b['measurements']}
    ranges=[(addresses[k],addresses[k]+(width if native else extents[k]))for k,(name,rva,width)in PROFILE.items()]
    ftol=0x410d90 if native else b['symbols']['__ftol'];ranges.append((ftol,ftol+39))
    cw=s.get('cw',0x27f);u.reg_write(UC_X86_REG_FPCW,cw);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100);u.mem_write(ESP,pack(STOP)+pack(s.get('rate',22050)))
    u.reg_write(UC_X86_REG_ESP,ESP)
    nv=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP];seeds=[0x11223344,0x22334455,0x33445566,0x44556677]
    for reg,value in zip(nv,seeds):u.reg_write(reg,value)
    events,entries,errors=[],[],[];conversions=0
    def ret(sp,extra,value):
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_ECX,0xdeadbeef);u.reg_write(UC_X86_REG_EDX,0xdeadbeef)
        u.reg_write(UC_X86_REG_ESP,sp+4+extra);u.reg_write(UC_X86_REG_EIP,get32(sp))
    def code(machine,a,width,_):
        nonlocal conversions
        if a==STOP:machine.emu_stop();return
        if a==ftol:conversions+=1
        if a in addresses.values():entries.append(next(k for k,v in addresses.items()if v==a))
        role=endpoints.get(a)
        if role:
            sp=u.reg_read(UC_X86_REG_ESP);arg=lambda n:get32(sp+n*4)
            if role=='DirectSoundCreate':
                assert arg(1)==0 and arg(2)==fields['device'][0]and arg(3)==0
                if not s.get('omit_device_write'):setfield('device',0 if s.get('null_device')else DEVICES[0])
                if s.get('mutate_window'):setfield('window',0x87654321)
                events.append([role,s.get('api_result',0),get32(fields['device'][0])])
                ret(sp,12,s.get('api_result',0));return
            if role=='Cooperate':
                assert arg(1)in DEVICES and arg(2)==get32(fields['window'][0])and arg(3)==3
                events.append([role,DEVICES.index(arg(1)),arg(2),arg(3)])
                if s.get('retarget_device'):setfield('device',DEVICES[1])
                ret(sp,12,s.get('cooperate_result',0));return
            if role=='Create':
                assert arg(1)in DEVICES and arg(3)==fields['primary'][0]and arg(4)==0
                desc=arg(2);assert ESP-0x1000<=desc and desc+20<=ESP
                descriptor=bytes(u.mem_read(desc,20));assert descriptor==struct.pack('<5I',20,1,0,0,0)
                events.append([role,DEVICES.index(arg(1)),descriptor.hex()])
                if not s.get('omit_primary_write'):setfield('primary',0 if s.get('null_primary')else BUFFERS[0])
                ret(sp,16,s.get('create_result',0));return
            assert role=='SetFormat'and arg(1)in BUFFERS and arg(2)==fields['format'][0]
            pcm=bytes(u.mem_read(arg(2),16));events.append([role,BUFFERS.index(arg(1)),pcm.hex()])
            if s.get('mutate_format'):u.mem_write(fields['format'][0]+4,pack(123456))
            ret(sp,8,s.get('format_result',0));return
        if not any(lo<=a<hi for lo,hi in ranges):errors.append(['execution',hex(a)]);machine.emu_stop()
    def write(machine,access,a,width,value,_):
        allowed=[(ESP-0x1000,ESP+8),*[(a,a+n)for a,n in fields.values()]]
        if not any(lo<=a and a+width<=hi for lo,hi in allowed):errors.append(['write',hex(a),width]);machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
    entry=s.get('entry','startup');u.emu_start(addresses[entry],STOP,count=3000000)
    actual={k:bytes(u.mem_read(a,n))for k,(a,n)in fields.items()}
    table_ok=actual['table']==(reference if entry=='startup'else before['table'])
    untouched=['cursors','loops','scratch','cleanup_flag','registry','window']if entry=='startup'else[k for k in fields if k not in ['format']]
    if s.get('mutate_window')and 'window'in untouched:untouched.remove('window')
    checks=dict(returned=u.reg_read(UC_X86_REG_EIP)==STOP,stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,
                nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(nv,seeds)),
                guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and bytes(u.mem_read(ESP+8,32))==bytes([0xa7])*32,
                control_word=u.reg_read(UC_X86_REG_FPCW)==cw,fpu_empty=u.reg_read(UC_X86_REG_FPTAG)==0xffff,
                actual_conversion_count=conversions==(16383 if entry=='startup'else 0),
                table_reference=table_ok,untouched=all(actual[k]==before[k]for k in untouched),whitelist=not errors)
    if entry=='startup':
        checks['clears']=actual['bank']==bytes(19200)and actual['counters']==bytes(2400)and actual['ids']==bytes([255])*96 and actual['lanes']==bytes([255])*48 and actual['paused']==bytes(4)and actual['next']==bytes(4)
        expected=struct.pack('<H',s.get('configured',0xabcd1234)&65535)
        checks['same_low_word_gains']=actual['gain_left']==expected and actual['gain_right']==expected
    else:
        expected=s.get('previous_rate',44100)if s.get('initial_device')and s.get('initial_primary')else 0
        checks['prior_rate_return']=u.reg_read(UC_X86_REG_EAX)==expected
    assert all(checks.values()),(kind,s,checks,errors,events)
    return dict(checks=checks,state={k:hashlib.sha256(v).hexdigest()for k,v in actual.items()},
                format_hex=actual['format'].hex(),events=events,entries=entries,conversions=conversions,
                returned_rate=u.reg_read(UC_X86_REG_EAX)if entry=='format'else None)


def scenarios():
    result=[dict(name='normal'),dict(name='null_device',null_device=True),
            dict(name='api_failure_with_device',api_result=0x80004005),
            dict(name='stale_device_on_failure',api_result=0x80004005,initial_device=True,omit_device_write=True),
            dict(name='null_primary',null_primary=True),
            dict(name='create_failure_with_primary',create_result=0x80004005),
            dict(name='stale_primary_on_failure',create_result=0x80004005,initial_primary=True,omit_primary_write=True),
            dict(name='cooperative_failure_and_retarget',retarget_device=True,cooperate_result=0x80004005),
            dict(name='window_live_reload',mutate_window=True),
            dict(name='format_failure_ignored',format_result=0x80004005),
            dict(name='format_callback_mutation',mutate_format=True),
            dict(name='zero_configured_gain',configured=0),dict(name='gain_high_bits_ignored',configured=0xffff8000)]
    for cw in [0x37f,0x67f,0xa7f,0xe7f]:result.append(dict(name='cw_'+hex(cw),cw=cw))
    for rate in [0,22050,44100,0x40000000,0xffffffff]:result.append(dict(name='format_rate_'+str(rate),entry='format',initial_device=True,initial_primary=True,rate=rate))
    result.extend([dict(name='format_no_device',entry='format',initial_primary=True),
                   dict(name='format_no_primary',entry='format',initial_device=True),
                   dict(name='format_return_snapshot',entry='format',initial_device=True,initial_primary=True,mutate_format=True)])
    return result


def main():
    b=build();reference,margin=reference_table();cases=[]
    for s in scenarios():
        native=execute('native',b,s,reference);source=execute('source',b,s,reference)
        assert native==source,(s,native,source)
        cases.append(dict(scenario=s,native=native,source=source,equal=True))
        print(json.dumps(dict(case_complete=s['name'])),flush=True)
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual startup/primaryformat/gain and raw-identicalCRTftol;x87log/convert not modeled. Only DirectSoundCreate and COM effects modeled;no device/process run.',
                build=b,cases=cases,case_count=len(cases),all_equal=True,
                independent_reference=dict(precision_decimal_digits=60,min_distance_to_integer=margin,table_sha256=hashlib.sha256(reference).hexdigest()))
    p=OUT/'sound_startup_differential.json';p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(case_count=len(cases),all_equal=True,report=str(p))))


if __name__=='__main__':main()
