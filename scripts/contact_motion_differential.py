"""Guarded finite motion/contact and fixed-angle fixtures; no OS module load.

The original motion routine executes normally. The two contact services are
modeled at their entry, including live actor mutations. Their internal scene
and collision chains are outside this fixture's scope.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from pe import PE
from title_behavioral import context_files
from title_link import read_map

sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

SOURCES = ['src/eng1/behavioral/contact_motion.c', 'src/shared/fixed_angle.c',
           'src/shared/motion.c', 'calibration/title/title_sine.c']
ENTRIES = {0x2bc50: ('_damped_contact_motion', 208), 0x2bd20: ('_contact_motion', 416),
           0x2bec0: ('_rounded_sine_angle', 48), 0x2bef0: ('_cosine_angle', 64)}
ORACLE = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
ORACLE_SHA = '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
DATA = 0x61000000
P = DATA + 0x100
STACK = 0x62000000
SP = STACK + 0x8000
STOP = STACK + 0xf000
NV = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]
pack = lambda x: struct.pack('<I', x & 0xffffffff)


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime = Path(unicorn.__file__).parent / 'lib/unicorn.dll'
    assert unicorn.__version__ == '2.1.4'
    assert digest(runtime.read_bytes()) == '010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes()) == ORACLE_SHA
    inputs = {Path(__file__).resolve(), runtime, ORACLE, ROOT / 'toolchains/manifest.json'}
    for source in SOURCES:
        inputs.update(context_files(ROOT / source))
    pins = {str(p): digest(p.read_bytes()) for p in inputs}
    out.mkdir(parents=True, exist_ok=True)
    services = out / 'services.c'
    services.write_text('/* Intercepted fixture entries only. */\nint eng1_29e20(void*p){for(;;){}}\nint eng1_29f50(void*p){for(;;){}}\n')
    objects, commands, sizes = [], [], {}
    for source in [*(ROOT / s for s in SOURCES), services]:
        obj, command = compile_source(source, ['/O2'])
        objects.append(obj)
        commands.append(command)
        coff = COFF(obj)
        for rva, (symbol, size) in ENTRIES.items():
            if any(s['name'] == symbol and s['section'] > 0 for s in coff.symbols.values()):
                sizes[rva] = coff.function(symbol)['size']
                assert sizes[rva] == size
    image, mapfile = out / 'fixture.dll', out / 'fixture.map'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/NOLOGO', '/DLL', '/NOENTRY',
               '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF', '/BASE:0x45000000',
               '/OUT:' + str(image), '/MAP:' + str(mapfile), *map(str, objects)]
    env = os.environ.copy()
    for key in ['CL', '_CL_', 'LINK', '_LINK_']:
        env.pop(key, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    result = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
    (out / 'link.log').write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
    return dict(input_sha256=pins, commands=commands + [command],
                object_sha256=[digest(p.read_bytes()) for p in objects],
                image_sha256=digest(image.read_bytes())), PE(image), read_map(mapfile)


def execute(pe, symbols, case, original):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(pe.image_base, extent)
    u.mem_write(pe.image_base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(DATA, 0x1000)
    u.mem_map(STACK, 0x10000)
    u.mem_write(DATA, bytes((i * 13 + 31) & 255 for i in range(0x1000)))
    rva = case['entry']
    symbol, size = ENTRIES[rva]
    entry = pe.image_base + rva if original else symbols[symbol]
    motion = pe.image_base + 0x2b4c0 if original else symbols['_advance_object_motion']
    services = {pe.image_base + r if original else symbols['_eng1_' + hex(r)[2:]]: r for r in [0x29e20, 0x29f50]}
    contact = rva in [0x2bc50, 0x2bd20]
    if contact:
        u.mem_write(P, b''.join(pack(v) for v in case.get('position', [0x7fffffff, 0x80000000, 0xffffffff])))
        u.mem_write(P + 0x94, bytes(case['directions']))
        for index, axis in enumerate(case['axes']):
            u.mem_write(P + 0xa8 + index * 16, b''.join(pack(v) for v in axis))
        argument = P
    else:
        argument = case['angle']
    frame = pack(STOP) + pack(argument) + b'\x93' * 24
    u.mem_write(SP, frame)
    u.reg_write(UC_X86_REG_ESP, SP)
    u.reg_write(UC_X86_REG_EAX, 0x93847560)
    for reg, seed in zip(NV, SEEDS):
        u.reg_write(reg, seed)
    calls = []

    def get32(at):
        return struct.unpack('<I', u.mem_read(at, 4))[0]

    def code(uc, address, length, user):
        if address == STOP:
            uc.emu_stop()
            return
        if address in services:
            service = services[address]
            assert contact and service == (0x29e20 if rva == 0x2bc50 else 0x29f50)
            esp = uc.reg_read(UC_X86_REG_ESP)
            assert get32(esp + 4) == P
            calls.append(dict(service=hex(service), actor_sha256=digest(bytes(uc.mem_read(P, 0x200)))))
            if case.get('mutate'):
                for offset, value in [(4, 0x12345678), (0xac, 0x76543210), (0xb0, 0x123),
                                      (0xb4, 0x456), (0xb8, case['callback_speed']),
                                      (0xc0, 0x789), (0xc4, 0xabc), (0xcc, 0x80000001)]:
                    uc.mem_write(P + offset, pack(value))
                uc.mem_write(P + 0x95, b'\x80')
            uc.reg_write(UC_X86_REG_EAX, case['result'] & 0xffffffff)
            uc.reg_write(UC_X86_REG_EIP, get32(esp))
            uc.reg_write(UC_X86_REG_ESP, esp + 4)
            return
        assert (entry <= address and address + length <= entry + size or
                rva == 0x2bc50 and motion <= address and address + length <= motion + 288), hex(address)

    def write(uc, access, address, size, value, user):
        assert (STACK <= address and address + size <= SP or
                contact and P <= address and address + size <= P + 0x200), hex(address)

    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.emu_start(entry, STOP + 1, count=2000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    assert u.reg_read(UC_X86_REG_ESP) == SP + 4
    assert bytes(u.mem_read(SP, len(frame))) == frame
    assert [u.reg_read(r) for r in NV] == SEEDS
    return dict(result=u.reg_read(UC_X86_REG_EAX), calls=calls,
                data_sha256=digest(bytes(u.mem_read(DATA, 0x1000))))


def cases():
    rng = random.Random(0x2bc50)
    for entry in [0x2bc50, 0x2bd20]:
        for direction, speed, result, mutate in itertools.product([0, 1, 255], [0, 1, 2, -1, 0x7fffffff, 0x80000000], [0, 1, -1], [False, True]):
            yield dict(entry=entry, directions=[255, direction, 1],
                       axes=[[0x7fffffff, -1, 1, 0], [speed, 0x7fffffff, 0, 0], [-1, 0x80000001, 0x7fffffff, 1]],
                       result=result, mutate=mutate, callback_speed=speed)
        for n in range(160):
            yield dict(entry=entry, directions=[rng.randrange(256) for _ in range(3)],
                       axes=[[rng.getrandbits(32) for _ in range(4)] for _ in range(3)],
                       result=rng.choice([0, 1, -1]), mutate=bool(n % 2), callback_speed=rng.getrandbits(32))
    angles = [0, 1, -1, 44, 45, 46, -44, -45, -46, 0x7fffffff, 0x80000000, 0xffffffff]
    angles += [n * 65536 + d for n in range(-360, 361, 15) for d in [-23041, -1, 0, 1, 23039]]
    angles += [rng.getrandbits(32) for _ in range(250)]
    for entry in [0x2bec0, 0x2bef0]:
        for angle in angles:
            yield dict(entry=entry, angle=angle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/motion_contacts_2026_10_25/differential')
    out = parser.parse_args().output.resolve()
    assert out.is_relative_to(ROOT / 'work')
    built, candidate, symbols = build(out)
    native, results, counts = PE(ORACLE), [], {}
    for case in cases():
        a, b = execute(native, symbols, case, True), execute(candidate, symbols, case, False)
        assert a == b, dict(case=case, native=a, source=b)
        results.append(dict(case=case, result=a))
        key = hex(case['entry'])
        counts[key] = counts.get(key, 0) + 1
    assert all(digest(Path(p).read_bytes()) == h for p, h in built['input_sha256'].items())
    report = dict(scope=__doc__, all_equal=True, case_count=len(results), by_entry=counts,
                  build=built, cases_sha256=digest(json.dumps(results, sort_keys=True).encode()),
                  limitations=['Contact services modeled, including actor mutations; scene/grid/resource chains not executed.',
                               'Finite cases do not establish universal equivalence or whole-game completeness.'])
    (out / 'cases.json').write_text(json.dumps(results, indent=2) + '\n')
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(cases=len(results), by_entry=counts, all_equal=True)))


if __name__ == '__main__':
    main()
