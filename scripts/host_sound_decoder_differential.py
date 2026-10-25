"""Original/fresh nibble-decoder comparison with no modeled game or CRT calls."""
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
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'work/host_sound_decoder_public')
parser.add_argument('--native-exe',type=Path,default=Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE'))
args = parser.parse_args()
OUT = args.output.resolve()
assert OUT.is_relative_to((ROOT/'work').resolve())
OUT.mkdir(parents=True,exist_ok=True)
sys.path[:0] = [str(ROOT/'scripts'), 'C:/tools/hercules/python',
                'C:/tools/hercules/renderer-oracle-python']
from match import compile_source, compare, verify_toolchain, DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

ORACLE = args.native_exe.resolve()
SOURCE = ROOT/'src/pc/host_sound_decoder.c'
TABLE_SOURCE = ROOT/'src/pc/host_sound_decoder_tables.c'
TABLE_HEADER = ROOT/'src/pc/host_sound_decoder_tables.h'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
pack = lambda v: struct.pack('<I', v & 0xffffffff)
ARENA, ARENA_SIZE, STACK = 0x61000000, 0x20000, 0x62000000
ESP, STOP = STACK+0x8000, STACK+0xf000
TABLES = [('_g_host_sound_index_deltas_35e08', 0x35e08, 64),
          ('_g_host_sound_steps_35e48', 0x35e48, 356)]


def build():
    assert sha(ORACLE) == '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    verify_toolchain(DEFAULT_TOOLCHAIN)
    watched = [ORACLE, SOURCE, TABLE_SOURCE, TABLE_HEADER, Path(__file__),
               ROOT/'toolchains/manifest.json', *[ROOT/'scripts'/p for p in
               ['match.py', 'coff.py', 'masked.py', 'pe.py', 'env.py']]]
    pins = {str(p): sha(p) for p in watched}
    fixture = OUT/'fixture.c'
    fixture.write_bytes(b'''extern void __cdecl host_sound_decode_2410(const signed char*,short*,int,int*);
__declspec(dllexport) void anchor(void) { host_sound_decode_2410(0,0,0,0); }
''')
    objects, receipts = [], []
    for p in [SOURCE, TABLE_SOURCE, fixture]:
        obj, command = compile_source(p, ['/O2'])
        objects.append(obj)
        receipts.append(dict(source=str(p),source_sha256=sha(p),command=command,
                             object=str(obj),object_sha256=sha(obj)))
    dll, mp = OUT/'fixture.dll', OUT/'fixture.map'
    command = [str(DEFAULT_TOOLCHAIN/'bin/link.exe'), '/nologo', '/DLL', '/NOENTRY',
               '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:REF', '/OPT:NOICF',
               '/OUT:'+str(dll), '/MAP:'+str(mp), *map(str,objects)]
    env = os.environ.copy()
    for k in ['CL','_CL_','LINK','_LINK_']:
        env.pop(k,None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN/'bin')+os.pathsep+env.get('PATH','')
    process = subprocess.run(command,env=env,capture_output=True,text=True)
    (OUT/'link.log').write_bytes((process.stdout+process.stderr).encode())
    assert process.returncode == 0, process.stdout+process.stderr
    regex = re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
    symbols = {m[1]:int(m[2],16) for line in mp.read_text().splitlines()
               if (m:=regex.match(line))}
    original, linked, co = pefile.PE(str(ORACLE)), pefile.PE(str(dll)), COFF(objects[1])
    data = []
    for name,rva,width in TABLES:
        matches = [s for s in co.symbols.values() if s['name']==name and s['storage']==2 and s['section']>0]
        assert len(matches)==1
        symbol = matches[0]
        section = co.sections[symbol['section']-1]
        assert not section['relocations'] and symbol['value']+width<=section['size']
        value = section['data'][symbol['value']:symbol['value']+width]
        assert value==original.get_data(rva,width)
        assert value==linked.get_data(symbols[name]-linked.OPTIONAL_HEADER.ImageBase,width)
        data.append(dict(symbol=name,rva=hex(rva),bytes=width,raw_initialized_equal=True,
                         sha256=hashlib.sha256(value).hexdigest()))
    raw = compare(objects[0],'_host_sound_decode_2410',ORACLE,0x2410,288)
    diagnostic = masked_compare(objects[0],'_host_sound_decode_2410',ORACLE,0x2410,288)
    assert pins=={p:sha(Path(p)) for p in pins}
    return dict(inputs=pins,compiles=receipts,dll=str(dll),dll_sha256=sha(dll),
                map_sha256=sha(mp),link_command=command,symbols=symbols,
                raw=raw,diagnostic=diagnostic,data=data)


def execute(kind,build,scenario):
    original = pefile.PE(str(ORACLE))
    image = original if kind=='native' else pefile.PE(build['dll'])
    base,size = image.OPTIONAL_HEADER.ImageBase,image.OPTIONAL_HEADER.SizeOfImage
    u = Uc(UC_ARCH_X86,UC_MODE_32)
    u.mem_map(base,(size+4095)&~4095)
    u.mem_write(base,image.get_memory_mapped_image().ljust(size,b'\0'))
    u.mem_map(ARENA,ARENA_SIZE)
    u.mem_map(STACK,0x10000)
    before = bytearray((i*29+(i>>8)*17+43)&255 for i in range(ARENA_SIZE))
    source,target,state = (scenario.get(k,v) for k,v in
                          [('source',0x1000),('target',0x8000),('state',0x18000)])
    packed = bytes.fromhex(scenario.get('packed','0123456789abcdef'))
    count = scenario.get('count',len(packed)*4)
    sample_count = max(0,count>>1)
    assert sample_count <= 4096
    if source is not None:
        assert source+len(packed)<=ARENA_SIZE
        before[source:source+len(packed)] = packed
    if target is not None:
        assert target+sample_count*2<=ARENA_SIZE
    assert 0 <= state and state+4 <= ARENA_SIZE
    before[state:state+4] = pack(scenario.get('initial_state',0))
    u.mem_write(ARENA,bytes(before))
    expected = bytearray(before)
    value = int.from_bytes(expected[state:state+4],'little')
    index,predictor = value>>16,value&0xffff
    assert index<=88
    deltas = struct.unpack('<16i',original.get_data(0x35e08,64))
    steps = struct.unpack('<89i',original.get_data(0x35e48,356))
    step,half,cached = steps[index],0,0
    assert sample_count==0 or source is not None and target is not None
    for i in range(sample_count):
        if half:
            code=cached
        else:
            cached=expected[source+i//2]
            code=cached>>4
        code &= 15
        half ^= 1
        index=max(0,min(88,index+deltas[code]))
        difference=(step>>3)+(step if code&4 else 0)+((step>>1)if code&2 else 0)+((step>>2)if code&1 else 0)
        predictor=max(-32768,min(32767,predictor+(-difference if code&8 else difference)))
        expected[target+i*2:target+i*2+2]=struct.pack('<h',predictor)
        step=steps[index]
    expected[state:state+4]=pack((index<<16)|predictor)
    args=[0 if source is None else ARENA+source,0 if target is None else ARENA+target,
          count,ARENA+state]
    u.mem_write(ESP-0x1000,bytes([0xa7])*0x1100)
    u.mem_write(ESP,pack(STOP)+b''.join(pack(v)for v in args))
    u.reg_write(UC_X86_REG_ESP,ESP)
    nv=[UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]
    seeds=[0x11223344,0x22334455,0x33445566,0x44556677]
    for reg,value in zip(nv,seeds):
        u.reg_write(reg,value)
    entry=0x402410 if kind=='native' else build['symbols']['_host_sound_decode_2410']
    extent=288 if kind=='native' else build['raw']['compiled_size']
    allowed=[(ARENA+state,ARENA+state+4),(ESP-0x1000,ESP+20)]
    if sample_count:
        allowed.append((ARENA+target,ARENA+target+sample_count*2))
    errors=[]
    def code(machine,address,width,_):
        if address==STOP:
            machine.emu_stop()
        elif not entry<=address<entry+extent:
            errors.append(['execution',hex(address)])
            machine.emu_stop()
    def write(machine,access,address,width,value,_):
        if not any(a<=address and address+width<=end for a,end in allowed):
            errors.append(['write',hex(address),width])
            machine.emu_stop()
    u.hook_add(UC_HOOK_CODE,code)
    u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.emu_start(entry,STOP,count=1000000)
    actual=bytes(u.mem_read(ARENA,ARENA_SIZE))
    checks=dict(whole_arena=actual==bytes(expected),sentinel=u.reg_read(UC_X86_REG_EIP)==STOP,
                stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,
                nonvolatiles=all(u.reg_read(reg)==v for reg,v in zip(nv,seeds)),
                guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and
                bytes(u.mem_read(ESP+20,32))==bytes([0xa7])*32,whitelist=not errors)
    assert all(checks.values()),(kind,scenario,checks,errors)
    return dict(checks=checks,arena_sha256=hashlib.sha256(actual).hexdigest(),
                final_state=int.from_bytes(actual[state:state+4],'little'),
                samples_hex=''if target is None else actual[target:target+sample_count*2].hex())


def scenarios():
    result=[dict(name='all_nibbles'),dict(name='null_zero',source=None,target=None,count=0),
            dict(name='negative_count',source=None,target=None,count=-1)]
    for nibble in range(16):
        result.append(dict(name='nibble_'+str(nibble),packed=('%02x'%(nibble*17))*8))
    for state in [0,0xffff,0x00018000,0x00580000,0x00587fff,0x0058ffff]:
        result.append(dict(name='state_'+hex(state),initial_state=state))
    for count in [0,1,2,3,5,6,7,9,31]:
        result.append(dict(name='count_'+str(count),count=count))
    result.extend([dict(name='forward_alias',target=0x1001),
                   dict(name='backward_alias',source=0x1004,target=0x1000),
                   dict(name='state_output_alias',state=0x8000),
                   dict(name='state_input_alias',state=0x1000,initial_state=0),
                   dict(name='long_sequence',packed='0123456789abcdef'*128)])
    return result


def main():
    b,cases=build(),[]
    for scenario in scenarios():
        native=execute('native',b,scenario)
        source=execute('source',b,scenario)
        assert native==source,scenario
        cases.append(dict(scenario=scenario,native=native,source=source,equal=True))
    assert b['inputs']=={p:sha(Path(p))for p in b['inputs']}
    report=dict(scope='Actual native/fresh nibble decoder and initialized data; no modeled callees, CRT, OS or game process.',
                build=b,cases=cases,case_count=len(cases),all_equal=True)
    p=OUT/'sound_decoder_differential.json'
    p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(case_count=len(cases),all_equal=True,report=str(p))))


if __name__=='__main__':
    main()
