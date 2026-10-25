"""Fresh native/source DirectSound channel experiment; only COM and F3D0 modeled."""
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
parser.add_argument('--output',type=Path,default=ROOT/'work/host_sound_channels_public')
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
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
pack = lambda v: struct.pack('<I', v & 0xffffffff)
PROFILE = {
    'stop_all': ('_audio_native_10010', 0x10010, 32),
    'restore': ('_host_sound_bank_restore_f760', 0xf760, 96),
    'stop': ('_audio_native_fa10', 0xfa10, 256),
    'pause': ('_host_sound_pause_ffb0', 0xffb0, 96),
    'resume': ('_host_sound_resume_10030', 0x10030, 160),
    'poll': ('_host_audio_poll_f820', 0xf820, 176),
    'release': ('_cleanup_native_f710', 0xf710, 80),
}
DATA = {
    'ids': ('_g_host_audio_channel_ids_799848', 0x799848, 96),
    'cursors': ('_g_host_audio_channel_cursors_7998a8', 0x7998a8, 96),
    'bank': ('_g_exe_799908', 0x799908, 19200),
    'registry': ('_g_registry_config', 0x79e858, 72),
    'paused': ('_g_host_sound_paused_79e44c', 0x79e44c, 4),
    'loops': ('_g_host_sound_loop_flags_698a18', 0x698a18, 1200),
    'gain_first': ('_g_exe_79e438', 0x79e438, 2),
    'gain_second': ('_g_exe_79e43c', 0x79e43c, 2),
    'release_flag': ('_g_audio_cleanup_iteration_flag', 0x79e450, 1),
}
STACK, ESP, STOP = 0x62000000, 0x62008000, 0x6200f000
METHODS, OBJECTS, OBJECT_SIZE = 0x65000000, 0x66000000, 0x20000
VTABLE = OBJECTS + OBJECT_SIZE - 0x100
SLOTS = {'Release': (8, 0), 'GetCurrentPosition': (0x10, 8),
         'GetStatus': (0x24, 4), 'Play': (0x30, 12),
         'SetCurrentPosition': (0x34, 4), 'Stop': (0x48, 0), 'Restore': (0x50, 0)}


def build():
    assert sha(ORACLE) == '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    verify_toolchain(DEFAULT_TOOLCHAIN)
    source = ROOT/'src/pc/host_sound_channels.cpp'
    storage = ROOT/'src/pc/host_sound_channel_storage.cpp'
    files = [source, storage, *[ROOT/p for p in [
        'src/pc/host_warm_delivery.cpp', 'src/pc/host_warm_delivery_storage.cpp',
        'src/pc/host_callback_storage.c', 'src/pc/sound_bank_storage.cpp',
        'src/shared/registry_storage.c', 'src/pc/audio_bank_release.cpp',
        'src/pc/audio_small_helpers.c', 'src/pc/audio_gain_storage.c']]]
    watched = [*files, ORACLE, Path(__file__), ROOT/'src/pc/host_warm_delivery.h',
               ROOT/'src/pc/host_callback_protocol_externs.h',
               ROOT/'src/pc/host_sound_channel_storage.h', ROOT/'src/pc/host_sound_bank.h',
               DEFAULT_TOOLCHAIN/'include/dsound.h', DEFAULT_TOOLCHAIN/'include/windows.h',
               ROOT/'toolchains/manifest.json', *[ROOT/'scripts'/p for p in
               ['match.py', 'coff.py', 'pe.py', 'masked.py', 'env.py']]]
    pins = {str(p): sha(p) for p in watched}
    fixture = OUT/'fixture.cpp'
    fixture.write_bytes(b'''#include <windows.h>
extern "C" {
void __cdecl host_sound_bank_initialize_f3d0(unsigned int) { for (;;) {} }
DWORD __cdecl host_input_collect_8040(void) { for (;;) {} return 0; }
void __cdecl host_invoke_callback_one(void) { for (;;) {} }
void __cdecl audio_native_fa10(int,int);
void __cdecl audio_native_10010(void);
void __cdecl host_sound_pause_ffb0(void);
void __cdecl host_sound_resume_10030(void);
void __cdecl host_sound_bank_restore_f760(void);
__declspec(dllexport) void anchor(void) {
 audio_native_10010(); audio_native_fa10(0,0); host_sound_pause_ffb0(); host_sound_resume_10030();
 host_sound_bank_restore_f760();
}
}
''')
    objects, receipts, definitions, owners = [], [], {}, {}
    for p in [*files, fixture]:
        obj, cmd = compile_source(p, ['/O2'])
        objects.append(obj)
        receipts.append(dict(source=str(p), source_sha256=sha(p), object=str(obj),
                             object_sha256=sha(obj), command=cmd))
        co = COFF(obj)
        for s in co.symbols.values():
            if s['storage'] == 2 and s['section'] > 0:
                owners.setdefault(s['name'], []).append((obj, co, s, p))
                if s['type'] & 0x20:
                    assert s['name'] not in definitions, s['name']
                    definitions[s['name']] = (obj, co.function(s['name'])['size'])
    dll, mp = OUT/'fixture.dll', OUT/'fixture.map'
    command = [str(DEFAULT_TOOLCHAIN/'bin/link.exe'), '/nologo', '/DLL', '/NOENTRY',
               '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:REF', '/OPT:NOICF',
               '/OUT:'+str(dll), '/MAP:'+str(mp), *map(str, objects)]
    env = os.environ.copy()
    for k in ['CL', '_CL_', 'LINK', '_LINK_']:
        env.pop(k, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN/'bin') + os.pathsep + env.get('PATH', '')
    process = subprocess.run(command, env=env, capture_output=True, text=True)
    (OUT/'link.log').write_bytes((process.stdout+process.stderr).encode())
    assert process.returncode == 0, process.stdout+process.stderr
    regex = re.compile(r'^\s*[\da-fA-F]{4}:[\da-fA-F]{8}\s+(\S+)\s+([\da-fA-F]{8})')
    symbols = {m[1]: int(m[2], 16) for line in mp.read_text().splitlines()
               if (m := regex.match(line))}
    measurements = []
    for role, (symbol, rva, size) in PROFILE.items():
        obj, extent = definitions[symbol]
        raw = compare(obj, symbol, ORACLE, rva, size)
        diagnostic = masked_compare(obj, symbol, ORACLE, rva, size)
        if role != 'stop':
            assert diagnostic['masked_equal'], (role, diagnostic)
        measurements.append(dict(role=role, symbol=symbol, source_size=extent,
                                 raw=raw, diagnostic=diagnostic))
    release_proof = next(m for m in measurements if m['role'] == 'release')
    assert release_proof['diagnostic']['symbols']['_g_audio_cleanup_iteration_flag'] == hex(DATA['release_flag'][1])
    native, data = pefile.PE(str(ORACLE)), []
    for key, (symbol, rva, width) in DATA.items():
        assert len(owners[symbol]) == 1
        obj, co, s, provider = owners[symbol][0]
        section = co.sections[s['section']-1]
        assert not section['relocations'] and s['value']+width <= section['size']
        actual = section['data'][s['value']:s['value']+width] if section['data'] else bytes(width)
        target = next(t for t in native.sections if t.VirtualAddress <= rva and
                      rva+width <= t.VirtualAddress+t.Misc_VirtualSize)
        assert rva >= target.VirtualAddress+target.SizeOfRawData
        assert actual == bytes(width)
        data.append(dict(role=key, symbol=symbol, provider=str(provider), bytes=width,
                         rva=hex(rva), source_kind='BSS', native_backing='virtual_loader_zero'))
    assert pins == {p: sha(Path(p)) for p in pins}
    return dict(inputs=pins, compiles=receipts, dll=str(dll), dll_sha256=sha(dll),
                link_command=command, map_sha256=sha(mp), symbols=symbols,
                measurements=measurements, data=data)


def execute(kind, build, scenario):
    native = kind == 'native'
    pe = pefile.PE(str(ORACLE if native else build['dll']))
    base, size = pe.OPTIONAL_HEADER.ImageBase, pe.OPTIONAL_HEADER.SizeOfImage
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(base, (size+4095)&~4095)
    u.mem_write(base, pe.get_memory_mapped_image().ljust(size, b'\0'))
    u.mem_map(STACK, 0x10000)
    u.mem_map(METHODS, 0x1000)
    u.mem_map(OBJECTS, OBJECT_SIZE)
    fields = {k: (0x400000+rva if native else build['symbols'][sym], width)
              for k, (sym, rva, width) in DATA.items()}
    get32 = lambda a: int.from_bytes(u.mem_read(a, 4), 'little')
    def put(key, index, value, width=4):
        a, length = fields[key]
        assert 0 <= index*width and index*width+width <= length
        u.mem_write(a+index*width, pack(value)[:width])
    def object_address(token):
        assert 0 <= token < 6000
        return OBJECTS+token*16
    for token in range(6000):
        u.mem_write(object_address(token), pack(VTABLE)+pack(1)+pack(123+token)+pack(0))
    endpoints = {}
    for i, (name, (offset, _)) in enumerate(SLOTS.items()):
        address = METHODS+16*i
        endpoints[address] = name
        u.mem_write(VTABLE+offset, pack(address))
    for key, (address, width) in fields.items():
        u.mem_write(address, bytes(width))
    for i in range(24):
        put('ids', i, 0xffffffff)
        put('cursors', i, 0xa1b2c300+i)
    u.mem_write(fields['registry'][0], bytes((i*13+7)&255 for i in range(72)))
    put('registry', 14, scenario.get('enabled', 1), 1)
    put('paused', 0, scenario.get('paused', 1))
    put('release_flag', 0, scenario.get('release_flag', 0), 1)
    for bank in scenario.get('banks', [0, 1, 2]):
        for lane in range(4):
            put('bank', bank*4+lane, object_address(bank*4+lane))
    for bank, flag in scenario.get('loops', {0: 0, 1: 1, 2: 128}).items():
        put('loops', int(bank), flag, 1)
    for slot, value in scenario.get('ids', {0: 0, 3: 0x10002, 23: 0x20003}).items():
        put('ids', int(slot), value)
    for token, value in scenario.get('status', {}).items():
        u.mem_write(object_address(int(token))+4, pack(value))
    for index, token in scenario.get('bank_tokens', {}).items():
        put('bank', int(index), 0 if token is None else object_address(token))
    initial_obj = bytes(u.mem_read(OBJECTS, OBJECT_SIZE))
    addresses = {role: 0x400000+rva if native else build['symbols'][sym]
                 for role, (sym, rva, width) in PROFILE.items()}
    widths = {m['role']: m['source_size'] for m in build['measurements']}
    code_ranges = [(addresses[k], addresses[k]+(size if native else widths[k]))
                   for k, (sym, rva, size) in PROFILE.items()]
    initializer = 0x40f3d0 if native else build['symbols']['_host_sound_bank_initialize_f3d0']
    events, errors, counts, entries = [], [], {}, []
    u.mem_write(ESP-0x1000, bytes([0xa7])*0x1100)
    u.mem_write(ESP, pack(STOP)+pack(scenario.get('value', -1))+pack(scenario.get('slot', 3)))
    u.reg_write(UC_X86_REG_ESP, ESP)
    nv = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
    seeds = [0x11223344, 0x22334455, 0x33445566, 0x44556677]
    for reg, value in zip(nv, seeds):
        u.reg_write(reg, value)
    def mutation(name):
        count = counts.get(name, 0)
        counts[name] = count+1
        actions = scenario.get('mutations', {}).get(name, [])
        action = actions[count] if count < len(actions) else {}
        for field, values in action.items():
            if field in ['result', 'status', 'cursor', 'refill']:
                continue
            for index, value in values.items():
                if field == 'bank':
                    value = 0 if value is None else object_address(value)
                put(field, int(index), value, 1 if field in ['registry', 'loops', 'release_flag'] else 4)
        return action
    def ret(sp, extra, result):
        u.reg_write(UC_X86_REG_EAX, result&0xffffffff)
        u.reg_write(UC_X86_REG_ECX, 0xdeadbeef)
        u.reg_write(UC_X86_REG_EDX, 0xdeadbeef)
        u.reg_write(UC_X86_REG_ESP, sp+4+extra)
        u.reg_write(UC_X86_REG_EIP, get32(sp))
    def on_code(machine, address, width, _):
        if address == STOP:
            machine.emu_stop()
            return
        if address in addresses.values():
            entries.append(next(k for k, v in addresses.items() if v == address))
        if address == initializer:
            sp = u.reg_read(UC_X86_REG_ESP)
            bank = get32(sp+4)
            assert bank < 1200
            action = mutation('Initialize')
            events.append(['Initialize', bank])
            if action.get('refill', True):
                for lane in range(4):
                    put('bank', bank*4+lane, object_address(bank*4+lane))
            ret(sp, 0, action.get('result', 1))
            return
        if address in endpoints:
            method = endpoints[address]
            sp = u.reg_read(UC_X86_REG_ESP)
            receiver = get32(sp+4)
            assert OBJECTS <= receiver < OBJECTS+6000*16 and (receiver-OBJECTS)%16 == 0
            token = (receiver-OBJECTS)//16
            action = mutation(method)
            arguments = []
            if method == 'GetStatus':
                target = get32(sp+8)
                assert ESP-0x1000 <= target and target+4 <= ESP
                value = action.get('status', get32(receiver+4))
                u.mem_write(target, pack(value))
                arguments = [value]
            elif method == 'GetCurrentPosition':
                first, second = get32(sp+8), get32(sp+12)
                assert fields['cursors'][0] <= first < fields['cursors'][0]+96
                assert ESP-0x1000 <= second and second+4 <= ESP
                value = action.get('cursor', get32(receiver+8))
                u.mem_write(first, pack(value))
                u.mem_write(second, pack(value+17))
                arguments = [value]
            elif method == 'SetCurrentPosition':
                value = get32(sp+8)
                u.mem_write(receiver+8, pack(value))
                arguments = [value]
            elif method == 'Play':
                arguments = [get32(sp+8), get32(sp+12), get32(sp+16)]
                assert arguments[:2] == [0, 0] and arguments[2] in [0, 1]
                u.mem_write(receiver+4, pack(get32(receiver+4)|1))
            elif method == 'Stop':
                u.mem_write(receiver+4, pack(get32(receiver+4)&~1))
            elif method == 'Restore':
                u.mem_write(receiver+4, pack(get32(receiver+4)&~2))
            elif method == 'Release':
                u.mem_write(receiver+12, pack(1))
            events.append([method, token, *arguments])
            ret(sp, SLOTS[method][1]+4, action.get('result', 0))
            return
        if not any(a <= address < end for a, end in code_ranges):
            errors.append(['execution', hex(address)])
            machine.emu_stop()
    def on_write(machine, access, address, width, value, _):
        allowed = [(a, a+n) for a, n in fields.values()]+[(ESP-0x1000, ESP+12)]
        if not any(a <= address and address+width <= end for a, end in allowed):
            errors.append(['write', hex(address), width])
            machine.emu_stop()
    u.hook_add(UC_HOOK_CODE, on_code)
    u.hook_add(UC_HOOK_MEM_WRITE, on_write)
    entry = addresses[scenario['entry']]
    u.emu_start(entry, STOP, count=200000)
    checks = dict(sentinel=u.reg_read(UC_X86_REG_EIP)==STOP,
                  stack=u.reg_read(UC_X86_REG_ESP)==ESP+4,
                  nonvolatiles=all(u.reg_read(reg)==v for reg, v in zip(nv, seeds)),
                  guards=bytes(u.mem_read(ESP-0x1000,32))==bytes([0xa7])*32 and
                  bytes(u.mem_read(ESP+12,32))==bytes([0xa7])*32,
                  args=bytes(u.mem_read(ESP+4,8))==pack(scenario.get('value',-1))+pack(scenario.get('slot',3)),
                  whitelist=not errors)
    assert all(checks.values()), (kind, scenario, checks, errors)
    objects_after = bytes(u.mem_read(OBJECTS, OBJECT_SIZE))
    result = dict(state={k: bytes(u.mem_read(a,n)).hex() for k,(a,n) in fields.items()},
                  objects_sha256=hashlib.sha256(objects_after).hexdigest(), events=events,
                  entries=entries, checks=checks)
    if scenario.get('expect_no_calls'):
        assert not events and objects_after == initial_obj
    if 'expect_releases' in scenario:
        count = sum(e[0] == 'Release' for e in events)
        assert count == scenario['expect_releases'], ('release_count', kind, count, scenario)
    if 'expect_initializations' in scenario:
        assert [e[1] for e in events if e[0]=='Initialize'] == scenario['expect_initializations']
    return result


def scenarios():
    result = [dict(name='release_special_protected',entry='release',value=824,banks=[824],release_flag=0,expect_releases=0),
              dict(name='release_special_enabled',entry='release',value=824,banks=[824],release_flag=1,expect_releases=4),
              dict(name='release_special_highbit',entry='release',value=824,banks=[824],release_flag=128,expect_releases=4),
              dict(name='restore_special_protected',entry='restore',banks=[824],status={3296:2},release_flag=0,expect_releases=0,expect_initializations=[824]),
              dict(name='restore_special_enabled',entry='restore',banks=[824],status={3296:2},release_flag=1,expect_releases=4,expect_initializations=[824]),
              dict(name='stop_all',entry='stop_all'),
              dict(name='stop_all_reload_later',entry='stop_all',mutations={'Stop':[{'ids':{23:0x10001}}]}),
              dict(name='stop_all_preserves_tagged',entry='stop_all',ids={3:0x03380000}),
              dict(name='stop_packed',entry='stop'),
              dict(name='stop_group',entry='stop',value=1),
              dict(name='stop_tagged_packed',entry='stop',ids={3:0x03380000},expect_no_calls=True),
              dict(name='stop_dead_packed',entry='stop',ids={3:0xffffffff},expect_no_calls=True),
              dict(name='stop_empty_guard',entry='stop',ids={3:0x10002},banks=[],expect_no_calls=True),
              dict(name='stop_slot_truncation',entry='stop',slot=0x7fff0003),
              dict(name='stop_reload_id',entry='stop',mutations={'Stop':[{'ids':{3:0x20001}}]}),
              dict(name='stop_reload_buffer',entry='stop',value=1,mutations={'Stop':[{'bank':{4:100}}]})]
    for value in [-2,824,1200]:
        result.append(dict(name='stop_rejected_group_'+str(value),entry='stop',value=value,expect_no_calls=True))
    for paused in [0,1,0x80000000]:
        result.append(dict(name='pause_guard_'+str(paused),entry='pause',paused=paused,
                           expect_no_calls=bool(paused)))
        result.append(dict(name='resume_guard_'+str(paused),entry='resume',paused=paused,
                           expect_no_calls=not bool(paused)))
    for enabled in [0,1,128,255]:
        result.append(dict(name='pause_enabled_'+str(enabled),entry='pause',paused=0,enabled=enabled))
        result.append(dict(name='resume_enabled_'+str(enabled),entry='resume',enabled=enabled))
    result.extend([
        dict(name='pause_nonplaying',entry='pause',paused=0,status={0:0,6:0,11:0}),
        dict(name='pause_poll_reload',entry='pause',paused=0,mutations={'GetStatus':[{'ids':{0:0x20001}}]}),
        dict(name='resume_reload_id',entry='resume',mutations={'SetCurrentPosition':[{'ids':{0:0x20001}}]}),
        dict(name='resume_disable_in_position',entry='resume',mutations={'SetCurrentPosition':[{'registry':{14:0}}]}),
        dict(name='restore_exact_lost',entry='restore',status={0:2},expect_initializations=[0]),
        dict(name='restore_mixed_status',entry='restore',status={0:6},expect_initializations=[]),
        dict(name='restore_failure_ignored',entry='restore',status={0:2},mutations={'Restore':[{'result':0x80004005}]},expect_initializations=[0]),
        dict(name='restore_live_bank',entry='restore',status={0:2},mutations={'GetStatus':[{'bank':{0:100}}]},expect_initializations=[0]),
        dict(name='resume_restore_then_play',entry='resume',status={0:2},expect_initializations=[0]),
        dict(name='boundary_bank',entry='resume',banks=[1199],ids={23:(1199<<16)|3},loops={1199:128}),
        dict(name='com_failures_ignored',entry='stop',value=1,mutations={'Stop':[{'result':0x80004005}],'SetCurrentPosition':[{'result':0x80004005}]})])
    return result


def main():
    b, cases = build(), []
    for scenario in scenarios():
        native = execute('native', b, scenario)
        source = execute('source', b, scenario)
        assert native == source, (scenario, native, source)
        cases.append(dict(scenario=scenario, native=native, source=source, equal=True))
    # Reproduce the historical fixture defect as a deliberate negative control.
    correct = DATA['release_flag']
    DATA['release_flag'] = (correct[0], 0x79e448, correct[2])
    rejected = False
    try:
        scenario = next(x for x in scenarios() if x['name'] == 'release_special_enabled')
        execute('native', b, scenario)
    except AssertionError as error:
        assert error.args[0][0] == 'release_count', error
        rejected = True
    finally:
        DATA['release_flag'] = correct
    assert rejected, 'Old flag address must fail the bank-824 discriminator.'
    assert b['inputs'] == {p:sha(Path(p)) for p in b['inputs']}
    report = dict(scope='Actual native/fresh channel stop, pause, resume, restore, poll and bank release. Only COM methods and the explicitly unrecovered F3D0 initializer are modeled. No process/device/game execution.',
                  build=b, cases=cases, case_count=len(cases), all_equal=True, old_flag_address_rejected=rejected)
    p = OUT/'sound_channels_differential.json'
    p.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(case_count=len(cases),all_equal=True,report=str(p))))


if __name__ == '__main__':
    main()
