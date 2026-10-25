"""Bounded fresh-VC5/original host delivery comparison; all APIs are synthetic."""
import argparse
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'scripts'), r'C:\tools\hercules\renderer-oracle-python']
from coff import COFF
from match import compile_source, compare, digest, verify_toolchain, DEFAULT_TOOLCHAIN
from masked import masked_compare
from pe import PE
from eng1_data_evidence import native_bytes
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ORACLE = ROOT / 'work/discs/pc_install/HERCULES.EXE'
SRC = ROOT / 'src/pc/host_warm_delivery.cpp'
OUT = ROOT / 'work/host_warm_differential'
FUNCTIONS = [('host_warm_delivery_bcf0', 0xbcf0, 64),
             ('host_register_input_packet_bd30', 0xbd30, 16),
             ('host_poll_input_alias_bd50', 0xbd50, 16),
             ('host_notify_secondary_be40', 0xbe40, 32),
             ('host_audio_poll_f820', 0xf820, 176)]
POLL, ONE, SECOND, ALT = 0x5000100, 0x5000200, 0x5000300, 0x5000400
STATUS, POSITION, STOP = 0x5000500, 0x5000600, 0x5000f00
BUFFER_A, BUFFER_B, VTABLE = 0x5002000, 0x5002010, 0x5003000
PACKET_A, PACKET_B = 0x5004000, 0x5004020
IDS, CURSORS, BANK, CONFIG = 0xb99848, 0xb998a8, 0xb99908, 0xb9e858
PREFIX, PACKET_PTR, SECOND_PTR, SECOND_GATE = 0xa8e780, 0xa8e78c, 0xa8e90c, 0xa8e910
STACK_TOP = 0x2010000

STORAGE_FIELDS = [('_g_host_input_packet_68e78c', 0x68e78c, 4),
                  ('_g_host_secondary_callback_68e90c', 0x68e90c, 4),
                  ('_g_host_secondary_delivery_68e910', 0x68e910, 4),
                  ('_g_host_audio_channel_ids_799848', 0x799848, 96),
                  ('_g_host_audio_channel_cursors_7998a8', 0x7998a8, 96)]

def verify_storage():
    source = ROOT / 'src/pc/host_warm_delivery_storage.cpp'
    obj, command = compile_source(source, ['/O2'])
    coff, image = COFF(obj), PE(ORACLE)
    fields = []
    public = {s['name']: s for s in coff.symbols.values() if s['section'] > 0 and s['storage'] == 2}
    assert set(public) == {name for name, _, _ in STORAGE_FIELDS}
    assert not any(s['flags'] & 0x20 and s['size'] for s in coff.sections)
    for name, rva, size in STORAGE_FIELDS:
        symbol = public[name]
        section = coff.sections[symbol['section']-1]
        assert section['flags'] & 0x80 and not section['relocations']
        assert symbol['value'] + size <= section['size']
        actual = bytes(size) if not section['data'] else section['data'][symbol['value']:symbol['value']+size]
        expected, origin, native_section = native_bytes(image, {'rva':rva, 'size':size})
        assert origin == 'loader_zero' and expected == actual
        fields.append({'symbol':name,'rva':hex(rva),'size':size,'exact_initial_bytes':True,
                       'initial_sha256':digest(actual),'origin':origin,
                       'coff_section':section['name'],'coff_offset':symbol['value']})
    return {'source':source.relative_to(ROOT).as_posix(),'source_sha256':digest(source.read_bytes()),
            'object':str(obj),'object_sha256':digest(obj.read_bytes()),'command':command,'fields':fields,
            'limit':'Measured separate prefixes only; no original aggregate/TU/declaration/placement claim. Fixture binds known field identities, not a native linker layout.'}

def get(uc, addr):
    return struct.unpack('<I', uc.mem_read(addr, 4))[0]

def put(uc, addr, value):
    uc.mem_write(addr, struct.pack('<I', value & 0xffffffff))

def compile_inputs():
    from title_behavioral import context_files
    source_paths = [SRC, ROOT / 'src/pc/host_callback_protocol.c', ROOT / 'src/pc/host_warm_delivery_storage.cpp', ROOT / 'src/pc/sound_bank_storage.cpp', ROOT / 'src/shared/registry_storage.c']
    paths = {p for source in source_paths for p in context_files(source)} | {ORACLE, Path(__file__).resolve(), ROOT / 'toolchains/manifest.json', ROOT / 'scripts/eng1_data_evidence.py'}
    before = {p: digest(p.read_bytes()) for p in sorted(paths)}
    assert before[ORACLE] == '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    obj, cmd = compile_source(SRC, ['/O2'])
    peer, peer_cmd = compile_source(ROOT / 'src/pc/host_callback_protocol.c', ['/O2'])
    assert all(digest(p.read_bytes()) == h for p, h in before.items())
    comparisons = []
    for name, rva, size in FUNCTIONS:
        comparisons.append(dict(name=name, rva=hex(rva), raw=compare(obj, '_'+name, ORACLE, rva, size), diagnostic=masked_compare(obj, '_'+name, ORACLE, rva, size)))
    return [obj, peer], [cmd, peer_cmd], before, comparisons

def linked_functions(objects):
    # Only test memory is relocated. No production code or original bytes change.
    external = {'_g_host_input_packet_68e78c': PACKET_PTR,
                '_g_host_secondary_callback_68e90c': SECOND_PTR,
                '_g_host_secondary_delivery_68e910': SECOND_GATE,
                '_g_host_callback_prefix': PREFIX,
                '_g_registry_config': CONFIG,
                '_g_host_audio_channel_ids_799848': IDS,
                '_g_host_audio_channel_cursors_7998a8': CURSORS,
                '_g_exe_799908': BANK, '_host_input_collect_8040': POLL}
    sections, symbols = [], dict(external)
    next_code = 0x7000000
    for obj in objects:
        coff = COFF(obj)
        for name in [s['name'] for s in coff.symbols.values() if s['section'] > 0 and s['type'] & 0x20]:
            fn = coff.function(name)
            assert name not in symbols
            symbols[name] = next_code
            sections.append((next_code, fn))
            next_code += 0x1000
    loads = []
    for address, fn in sections:
        code = bytearray(fn['data'])
        for r in fn['relocations']:
            assert r['symbol'] in symbols, r
            addend = struct.unpack_from('<I', code, r['offset'])[0]
            value = symbols[r['symbol']] + addend
            if r['type'] == 0x14:
                value -= address + r['offset'] + 4
            else:
                assert r['type'] == 6, r
            struct.pack_into('<I', code, r['offset'], value & 0xffffffff)
        loads.append((address, code))
    return symbols, loads

def emulator(pe, loads):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(pe.image_base, (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff)
    for s in pe.sections:
        if s['raw_size']:
            uc.mem_write(pe.image_base+s['virtual_address'], pe.data[s['raw_offset']:s['raw_offset']+s['raw_size']])
    uc.mem_map(0x2000000, 0x20000)
    uc.mem_map(0x5000000, 0x10000)
    uc.mem_map(0x7000000, 0x20000)
    for address, data in loads:
        uc.mem_write(address, bytes(data))
    for pointer in (BUFFER_A, BUFFER_B):
        put(uc, pointer, VTABLE)
    put(uc, VTABLE+0x24, STATUS)
    put(uc, VTABLE+0x10, POSITION)
    for address in (POLL, ONE, SECOND, ALT, STATUS, POSITION, STOP):
        uc.mem_write(address, b'\xc3')
    return uc

def run(uc, source, symbols, cfg):
    trace = []
    uc.mem_write(CONFIG, bytes(72))
    uc.mem_write(CONFIG+0x0e, bytes([cfg.get('sound', 1)]))
    for slot in range(24):
        put(uc, IDS+slot*4, 0 if cfg.get('initial_zero_prefixes') else 0xffffffff)
        put(uc, CURSORS+slot*4, 0 if cfg.get('initial_zero_prefixes') else 0xa5000000+slot)
    put(uc, BANK, 0 if cfg.get('initial_zero_prefixes') else BUFFER_A)
    put(uc, BANK+4799*4, 0 if cfg.get('initial_zero_prefixes') else BUFFER_B)
    for slot, value in cfg.get('ids', {}).items():
        put(uc, IDS+slot*4, value)
    put(uc, PREFIX, 0)
    put(uc, PREFIX+4, ONE if cfg.get('callbacks') else 0)
    put(uc, PREFIX+8, cfg.get('gate', 1))
    put(uc, SECOND_PTR, SECOND if cfg.get('callbacks') else 0)
    put(uc, SECOND_GATE, cfg.get('secondary_gate', 1))
    uc.mem_write(PACKET_A, bytes([0xa5])*16)
    uc.mem_write(PACKET_B, bytes([0x5a])*16)
    put(uc, PACKET_PTR, PACKET_A if cfg.get('packet') else 0)
    initial_nonvolatile = {UC_X86_REG_EBX:0x11223344, UC_X86_REG_EBP:0x22334455, UC_X86_REG_ESI:0x33445566, UC_X86_REG_EDI:0x44556677}
    for reg, value in initial_nonvolatile.items():
        uc.reg_write(reg, value)
    put(uc, STACK_TOP-4, STOP)
    if cfg['function'] == 'host_register_input_packet_bd30':
        put(uc, STACK_TOP, cfg.get('arg', 0))
    uc.reg_write(UC_X86_REG_ESP, STACK_TOP-4)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    entry = symbols['_'+cfg['function']] if source else pe.image_base+next(r for n,r,_ in FUNCTIONS if n==cfg['function'])
    visits = 0
    def hook(engine, address, size, data):
        nonlocal visits
        visits += 1
        assert visits < 50000, 'instruction budget exhausted'
        if address == STOP:
            engine.emu_stop()
            return
        if address not in (POLL, 0x408040, ONE, SECOND, ALT, STATUS, POSITION):
            return
        esp = engine.reg_read(UC_X86_REG_ESP)
        pop = 0
        if address in (POLL, 0x408040):
            trace.append(['input', cfg.get('input', 0x1234a5a5)])
            if cfg.get('swap_packet'):
                put(engine, PACKET_PTR, PACKET_B)
            engine.reg_write(UC_X86_REG_EAX, cfg.get('input', 0x1234a5a5))
        elif address == STATUS:
            buffer = get(engine, esp+4)
            assert buffer in (BUFFER_A, BUFFER_B)
            trace.append(['GetStatus', 'A' if buffer==BUFFER_A else 'B'])
            put(engine, get(engine, esp+8), cfg.get('status', 1))
            if cfg.get('swap_audio'):
                put(engine, IDS, (1199 << 16)|3)
            if cfg.get('activate_last'):
                put(engine, IDS+23*4, (1199 << 16)|3)
                put(engine, PREFIX+4, ONE)
                put(engine, SECOND_PTR, SECOND)
            engine.reg_write(UC_X86_REG_EAX, cfg.get('HRESULT', 0))
            pop = 8
        elif address == POSITION:
            buffer = get(engine, esp+4)
            assert buffer in (BUFFER_A, BUFFER_B)
            cursor_ptr = get(engine, esp+8)
            assert CURSORS <= cursor_ptr < CURSORS+24*4
            trace.append(['GetCurrentPosition', 'A' if buffer==BUFFER_A else 'B', (cursor_ptr-CURSORS)//4])
            put(engine, cursor_ptr, 0x1111 if buffer==BUFFER_A else 0x2222)
            put(engine, get(engine, esp+12), 0x3333)
            engine.reg_write(UC_X86_REG_EAX, 0x80004005)
            pop = 12
        else:
            trace.append(['callback', {ONE:'one', SECOND:'secondary', ALT:'changed'}[address]])
            if address==ONE and cfg.get('change_secondary'):
                put(engine, SECOND_PTR, ALT)
            if address==ONE and cfg.get('disable_after_one'):
                put(engine, PREFIX+8, 0)
            engine.reg_write(UC_X86_REG_EAX, 0x89abcdef)
        engine.reg_write(UC_X86_REG_EIP, get(engine, esp))
        engine.reg_write(UC_X86_REG_ESP, esp+4+pop)
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(entry, STOP+1, count=50000)
    assert uc.reg_read(UC_X86_REG_EIP)==STOP
    assert uc.reg_read(UC_X86_REG_ESP)==STACK_TOP
    assert all(uc.reg_read(reg)==value for reg,value in initial_nonvolatile.items())
    state = {'trace':trace, 'channels':list(struct.unpack('<24I',uc.mem_read(IDS,96))),
             'cursors':list(struct.unpack('<24I',uc.mem_read(CURSORS,96))),
             'packets':bytes(uc.mem_read(PACKET_A,16)).hex()+bytes(uc.mem_read(PACKET_B,16)).hex(),
             'packet_ptr':get(uc,PACKET_PTR), 'gate':get(uc,PREFIX+8),
             'callback_one':get(uc,PREFIX+4), 'callback_secondary':get(uc,SECOND_PTR)}
    if cfg['function'] in ('host_register_input_packet_bd30','host_poll_input_alias_bd50'):
        state['eax'] = uc.reg_read(UC_X86_REG_EAX)
    return state

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native-exe', type=Path, default=ROOT/'work/discs/pc_install/HERCULES.EXE')
parser.add_argument('--output', type=Path, default=OUT)
args = parser.parse_args()
ORACLE, OUT = args.native_exe.resolve(), args.output.resolve()
if not OUT.is_relative_to(ROOT/'work'):
    parser.error('Generated objects/receipts must remain under ignored work/')
OUT.mkdir(parents=True, exist_ok=True)
objects, commands, inputs, comparisons = compile_inputs()
storage = verify_storage()
symbols, loads = linked_functions(objects)
pe = PE(ORACLE)
base = {'function':'host_warm_delivery_bcf0'}
cases = [dict(base, name='warm_null_idle'),
         dict(base, name='initial_zero_prefixes', initial_zero_prefixes=True, sound=0, gate=0, secondary_gate=0),
         dict(base,name='warm_packet_callbacks',packet=True,callbacks=True),
         dict(base,name='input_mutates_packet',packet=True,swap_packet=True),
         dict(base,name='channel_null_bank',ids={0:1},sound=0),
         dict(base,name='disabled_sound',ids={0:0},sound=0),
         dict(base,name='stopped_status',ids={0:0},status=0,HRESULT=0x80004005),
         dict(base,name='playing_status_high_flags',ids={0:0},status=0x80000001),
         dict(base,name='status_mutates_channel_and_bank_selection',ids={0:0},swap_audio=True),
         dict(base,name='audio_mutates_later_slot_and_delivery',ids={0:0},activate_last=True),
         dict(base,name='channel_23_bank_edge',ids={23:(1199<<16)|3}),
         dict(base,name='callback_changes_secondary',callbacks=True,change_secondary=True),
         dict(base,name='callback_disables_secondary',callbacks=True,disable_after_one=True),
         dict(base,name='gate_zero',callbacks=True,gate=0),
         dict(base,name='secondary_gate_zero',callbacks=True,secondary_gate=0),
         dict(name='register_null',function='host_register_input_packet_bd30',arg=0),
         dict(name='register_packet',function='host_register_input_packet_bd30',arg=PACKET_B)]
cases += [dict(name='alias_'+hex(v),function='host_poll_input_alias_bd50',input=v) for v in (0,0x1234abcd,0xffffffff)]
results = []
for cfg in cases:
    original = run(emulator(pe, []), False, symbols, cfg)
    candidate = run(emulator(pe, loads), True, symbols, cfg)
    assert original == candidate, (cfg['name'],original,candidate)
    results.append(dict(name=cfg['name'],pass_equal=True,observation=original))
assert all(digest(p.read_bytes())==h for p,h in inputs.items())
report = {'scope':'20 finite interpreted-original/fresh-VC5 cluster cases; actual canonical callback-one source, fixture input sampler and SDK COM endpoints. No real APIs/game/DLL startup or strict matching acceptance.',
          'toolchain':verify_toolchain(DEFAULT_TOOLCHAIN),'commands':commands,
          'inputs':{str(p):h for p,h in inputs.items()},
          'objects':[{'path':str(p),'sha256':digest(p.read_bytes())} for p in objects],
          'comparisons':comparisons, 'cases':results, 'case_count':len(results), 'failures':0,
          'storage':storage,
          'limits':'GetStatus synthetic endpoint always writes status, including failure HRESULT. No output on failed status call remains outside finite proof. Bank IDs are supported row0..1199/slot0..3 only. Void incidental EAX is not compared. Callback/C++ spelling and storage/TU/module layout are unproved.'}
(OUT/'differential_report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
print(json.dumps({'cases':len(results),'failures':0,'comparisons':[{'name':r['name'],'raw_exact':r['raw']['exact'],'diagnostic_equal':r['diagnostic']['masked_equal'],'diagnostic_residuals':r['diagnostic']['differing_bytes']} for r in comparisons]},indent=2))
