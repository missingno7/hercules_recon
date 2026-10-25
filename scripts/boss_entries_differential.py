"""Complete Entry-data audit and isolated ENG1 25580/26700 composition.

Only unpatched original/source instructions execute in Unicorn. No original OS
module is loaded. The fixture uses the actual source data owner and helper.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_inputs, compile_source, compare, digest, verify_toolchain
from pe import PE
from refinery_forms import replace_function
from title_link import read_map

sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

ORACLE = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
SOURCE = ROOT / 'calibration/eng1/boss_unit_24090.c'
HELPER = ROOT / 'src/eng1/actor_handlers_mid.c'
OWNER = ROOT / 'src/eng1/behavioral/boss_entry_data.c'
ACTOR, STACK = 0x61001000, 0x65000000
SP, STOP = STACK + 0x8000, STACK + 0xf000
NV = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]
pack = lambda value: struct.pack('<I', value & 0xffffffff)


def locate_payload(pe, payload):
    if len(payload) != 576 or payload[-16:] != b'\xff' * 16:
        raise ValueError('Expected the complete 36-row owner and full sentinel')
    if any(struct.unpack_from('<H', payload, n * 16 + 14)[0] == 0xffff for n in range(35)):
        raise ValueError('Early sentinel changes the recovered table extent')
    positions, start = [], 0
    while True:
        found = pe.data.find(payload, start)
        if found < 0:
            break
        positions.append(pe.pe.get_rva_from_offset(found))
        start = found + 1
    if len(positions) != 1:
        raise ValueError('Complete source payload must have one native occurrence')
    return positions[0]


def build(output):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    library = Path(unicorn.__file__).parent / 'lib/unicorn.dll'
    assert unicorn.__version__ == '2.1.4' and digest(library.read_bytes()) == '010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes()) == '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    pins = {str(p):digest(p.read_bytes()) for p in [ORACLE, library, Path(__file__).resolve(), ROOT/'toolchains/manifest.json']}
    objects, commands = [], []
    for source in [SOURCE, HELPER, OWNER]:
        pins.update({str(p):h for p,h in compile_inputs(source, ['/O2'], DEFAULT_TOOLCHAIN).items()})
        obj, command = compile_source(source, ['/O2'])
        commands.append(command)
        if source == SOURCE:
            original = COFF(obj)
            text = source.read_text()
            for symbol in original.symbols.values():
                if symbol['section'] > 0 and symbol['type'] & 0x20 and symbol['name'] != '_eng1_25580':
                    text = replace_function(text, symbol['name'][1:], '')
            text = re.sub(r'#include "([^"]+)"', lambda m:'#include "'+source.parent.joinpath(m[1]).as_posix()+'"', text)
            fixture = output / 'caller.c'
            fixture.write_text(text)
            pins[str(fixture)] = digest(fixture.read_bytes())
            obj, command = compile_source(fixture, ['/O2'])
            commands.append(command)
            before, after = original.function('_eng1_25580'), COFF(obj).function('_eng1_25580')
            assert before['data'] == after['data'] and before['relocations'] == after['relocations']
        elif source == HELPER:
            assert compare(obj, '_attach_entries', ORACLE, 0x26700, 48)['exact']
        else:
            coff = COFF(obj)
            symbol, = [s for s in coff.symbols.values() if s['name'] == '_g_entries_5a450']
            section = coff.sections[symbol['section'] - 1]
            assert symbol['value'] == 0 and len(section['data']) == 576 and not section['relocations']
            assert {s['name'] for s in coff.symbols.values() if s['storage'] == 2 and s['section'] > 0} == {'_g_entries_5a450'}
            payload = section['data']
        objects.append(str(obj))
    native = PE(ORACLE)
    table_rva = locate_payload(native, payload)
    assert table_rva == 0x5a450
    controls = []
    for name, bad in [('changed_value', bytes([payload[0] ^ 1]) + payload[1:]), ('truncated', payload[:-16])]:
        try:
            locate_payload(native, bad)
        except ValueError:
            controls.append(name)
        else:
            raise AssertionError(name)
    image, mapfile = output/'fixture.dll', output/'fixture.map'
    command = [str(DEFAULT_TOOLCHAIN/'bin/link.exe'), '/NOLOGO', '/DLL', '/NOENTRY', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:REF', '/OPT:NOICF', '/BASE:0x45000000', '/OUT:'+str(image), '/MAP:'+str(mapfile), '/EXPORT:eng1_25580', *objects]
    commands.append(command)
    env = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN/'bin') + os.pathsep + env.get('PATH', '')
    linked = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True, timeout=60)
    (output/'link.log').write_text(linked.stdout + linked.stderr)
    assert linked.returncode == 0, linked.stdout + linked.stderr
    symbols = read_map(mapfile)
    return dict(input_sha256=pins, commands=commands, complete_caller_extraction_equal=True,
                data=dict(size=576, rows=36, usable_rows=35, native_rva=hex(table_rva), payload_sha256=digest(payload), negative_controls=controls),
                helper_raw_exact=True, image_sha256=digest(image.read_bytes())), native, PE(image), symbols


def execute(pe, entry, helper, table, case):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(pe.image_base, extent)
    u.mem_write(pe.image_base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(ACTOR - 0x1000, 0x3000)
    u.mem_map(STACK, 0x10000)
    raw = bytearray((n * 37 + case['seed']) & 255 for n in range(0x3000))
    struct.pack_into('<II', raw, 0x1050, case['flags'], case['render'])
    u.mem_write(ACTOR - 0x1000, bytes(raw))
    u.mem_write(table + case['sentinel'] * 16 + 14, b'\xff\xff')
    frame = pack(STOP) + pack(ACTOR) + b'\x93' * 24
    u.mem_write(SP, frame)
    u.reg_write(UC_X86_REG_ESP, SP)
    u.reg_write(UC_X86_REG_EAX, 0x93847560)
    for reg, seed in zip(NV, SEEDS):
        u.reg_write(reg, seed)
    writes, reads, calls = [], [], []
    def code(uc, address, size, user):
        if address == STOP:
            uc.emu_stop()
            return
        assert entry <= address and address + size <= entry + 64 or helper <= address and address + size <= helper + 48, hex(address)
        if address == helper:
            esp = uc.reg_read(UC_X86_REG_ESP)
            args = struct.unpack('<II', uc.mem_read(esp + 4, 8))
            assert args == (ACTOR, table)
            calls.append('attach_entries')
    def read(uc, access, address, size, value, user):
        if STACK <= address and address + size <= STACK + 0x10000:
            return
        if ACTOR <= address and address + size <= ACTOR + 0x134:
            return
        assert table <= address and address + size <= table + 576 and size == 2 and (address-table) % 16 == 14
        reads.append([(address-table)//16, int.from_bytes(uc.mem_read(address,size),'little')])
    def write(uc, access, address, size, value, user):
        if STACK <= address and address + size <= SP:
            return
        assert ACTOR <= address and address + size <= ACTOR + 0x134, hex(address)
        if address == ACTOR + 0x5c:
            assert size == 4 and value == table
            value = 'entry_table'
        writes.append([address-ACTOR, size, value])
    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_READ, read)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.emu_start(entry, 0, count=2000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == SP + 4
    assert bytes(u.mem_read(SP, len(frame))) == frame and [u.reg_read(r) for r in NV] == SEEDS
    assert calls == ['attach_entries'] and len(reads) == case['sentinel'] + 1
    assert int.from_bytes(u.mem_read(ACTOR + 0x36, 2), 'little') == case['sentinel']
    assert int.from_bytes(u.mem_read(ACTOR + 0x5c, 4), 'little') == table
    memory = bytearray(u.mem_read(ACTOR-0x1000, 0x3000))
    assert memory[:0x1000] == raw[:0x1000] and memory[0x1134:] == raw[0x1134:]
    memory[0x105c:0x1060] = b'\0'*4
    return dict(memory_sha256=digest(memory), ordered_writes=writes, table_tag_reads=reads, calls=calls)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'work/boss_entries_differential')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT/'work'):
        parser.error('Fixture output belongs under ignored work/')
    output.mkdir(parents=True, exist_ok=True)
    built, native, source, symbols = build(output)
    checks = []
    for flags, render, seed, sentinel in itertools.product([0, 0xffffffff, 0x800, 0x12345678], [0, 0xffffffff, 0x20000600, 0x87654321], [0, 79, 255], [0, 1, 7, 34, 35]):
        case = dict(flags=flags, render=render, seed=seed, sentinel=sentinel)
        a = execute(native, native.image_base+0x25580, native.image_base+0x26700, native.image_base+0x5a450, case)
        b = execute(source, symbols['_eng1_25580'], symbols['_attach_entries'], symbols['_g_entries_5a450'], case)
        checks.append(dict(case=case, equal=a==b, native=a, source=b))
    for path, sha in built['input_sha256'].items():
        assert digest(Path(path).read_bytes()) == sha
    traces = json.dumps(checks, sort_keys=True).encode() + b'\n'
    (output/'cases.json').write_bytes(traces)
    report = dict(scope=__doc__, cases=len(checks), all_equal=all(c['equal'] for c in checks), failed_cases=sum(not c['equal'] for c in checks), cases_sha256=digest(traces), build=built,
                  limits=['Entry pointer writes are normalized by the independently verified owner role; raw pointers are checked separately.', 'Early sentinels change data only in isolated memory; no native instruction/file is patched.', 'These cases do not establish coordinate meanings, later rendering or whole-game behavior.'])
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('cases','all_equal','failed_cases')}))
    if not report['all_equal']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
