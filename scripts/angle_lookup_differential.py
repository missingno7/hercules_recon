"""Finite unpatched PC angle-search fixtures and complete initialized-table audit.

Runs instructions only inside the selected contribution in Unicorn, never through
an OS loader or a module initializer. Full matching remains a separate gate.
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
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

SOURCE = ROOT / 'src/shared/angle_lookup.c'
OWNER = ROOT / 'src/title/title_sine.c'
MODULES = {'TITLE.DLL': 'b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b',
           'ENG1.DLL': '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757',
           'ENG3.DLL': 'aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7'}
ENTRIES = [('ENG1.DLL', 0x2bf30, '_lookup_tangent_angle'),
           ('ENG3.DLL', 0x1f7b0, '_lookup_tangent_angle'),
           ('ENG1.DLL', 0x2bfb0, '_lookup_cosine_phase')]
STACK = 0x62000000
SP = STACK + 0x8000
STOP = STACK + 0xf000
NV = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]


def execute(pe, entry, args):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(pe.image_base, extent)
    u.mem_write(pe.image_base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(STACK, 0x10000)
    frame = struct.pack('<III', STOP, *(a & 0xffffffff for a in args)) + b'\x93' * 24
    u.mem_write(SP, frame)
    u.reg_write(UC_X86_REG_ESP, SP)
    u.reg_write(UC_X86_REG_EAX, 0x93847560)
    for reg, seed in zip(NV, SEEDS):
        u.reg_write(reg, seed)

    def code(uc, address, size, user):
        if address == STOP:
            uc.emu_stop()
            return
        assert entry <= address and address + size <= entry + 128, hex(address)

    def write(uc, access, address, size, value, user):
        assert STACK <= address and address + size <= SP, hex(address)

    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    fault = None
    try:
        u.emu_start(entry, STOP + 1, count=2000)
    except UcError as error:
        fault = error.errno
    if fault is None:
        assert u.reg_read(UC_X86_REG_EIP) == STOP, 'instruction limit reached'
        assert u.reg_read(UC_X86_REG_ESP) == SP + 4
        assert bytes(u.mem_read(SP, len(frame))) == frame
        assert [u.reg_read(r) for r in NV] == SEEDS
        return dict(result=u.reg_read(UC_X86_REG_EAX))
    # Compare physical divide-fault location as well as exception class.
    return dict(fault=fault, offset=u.reg_read(UC_X86_REG_EIP) - entry)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/angle_lookup_2026_10_25/differential')
    out = parser.parse_args().output.resolve()
    assert out.is_relative_to(ROOT / 'work')
    out.mkdir(parents=True, exist_ok=True)
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime = Path(unicorn.__file__).parent / 'lib/unicorn.dll'
    assert unicorn.__version__ == '2.1.4'
    assert digest(runtime.read_bytes()) == '010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    originals = {m: PE(ROOT / 'assets/pc/HERCULES' / m) for m in MODULES}
    assert all(digest(p.data) == MODULES[m] for m, p in originals.items())
    inputs = {Path(__file__).resolve(), runtime, ROOT / 'toolchains/manifest.json'}
    for source in [SOURCE, OWNER]:
        inputs.update(context_files(source))
    pins = {str(p): digest(p.read_bytes()) for p in inputs}
    objects, commands = [], []
    for source in [SOURCE, OWNER]:
        obj, cmd = compile_source(source, ['/O2'])
        objects.append(obj)
        commands.append(cmd)
    owner = COFF(objects[1])
    tables = {}
    for name, size in [('_g_243e0', 2048), ('_g_tangent_quarter_degrees', 1440)]:
        sym, = [s for s in owner.symbols.values() if s['name'] == name]
        sec = owner.sections[sym['section'] - 1]
        pos = sym['value']
        payload = sec['data'][pos:pos + size]
        assert len(payload) == size and not sec['flags'] & 0x20
        assert not any(r['offset'] < pos + size and r['offset'] + 4 > pos for r in sec['relocations'])
        positions = {}
        for module, pe in originals.items():
            found, start = [], 0
            while True:
                at = pe.data.find(payload, start)
                if at < 0:
                    break
                start = at + 1
                rva = pe.offset_to_rva(at)
                if any(s['virtual_address'] <= rva and rva + size <= s['virtual_address'] + s['raw_size']
                       and not s['characteristics'] & 0x20000000 for s in pe.sections):
                    found.append(rva)
            assert len(found) == 1, (name, module, found)
            positions[module] = hex(found[0])
        tables[name] = dict(size=size, sha256=digest(payload), unique_native_positions=positions)
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
    candidate, symbols = PE(image), read_map(mapfile)
    results, counts = [], {}
    edge = [0, 1, 2, 255, 256, 32767, 32768, 65535, 0x7fffffff, 0x80000000, 0xffffffff]
    rng = random.Random(0x2bf30)
    cases = list(itertools.product(edge, repeat=2))
    cases += [(rng.getrandbits(32), rng.getrandbits(32)) for _ in range(180)]
    # Exercise table subdivisions, signed quotient wrap, and divide overflow.
    cases += [(i, 65536) for i in range(512)]
    cases += [(65535, i * 183) for i in range(360)]
    cases += [(65536, 0xffffffff)]  # cosine: INT_MIN / -1 traps physically
    for module, rva, symbol in ENTRIES:
        assert COFF(objects[0]).function(symbol)['size'] == 128
        key = module + ':' + hex(rva)
        for args in cases:
            native = execute(originals[module], originals[module].image_base + rva, args)
            rebuilt = execute(candidate, symbols[symbol], args)
            assert native == rebuilt, (key, args, native, rebuilt)
            results.append(dict(entry=key, args=args, result=native))
        counts[key] = len(cases)
    assert all(digest(Path(p).read_bytes()) == h for p, h in pins.items())
    report = dict(scope='Complete initialized-table audit and finite isolated unpatched angle-search cases; no module initialization or game execution.',
                  all_equal=True, case_count=len(results), by_entry=counts, tables=tables,
                  input_sha256=pins, original_sha256=MODULES, commands=commands + [command],
                  object_sha256=[digest(p.read_bytes()) for p in objects], image_sha256=digest(image.read_bytes()),
                  cases_sha256=digest(json.dumps(results, sort_keys=True).encode()),
                  faults=sum('fault' in row['result'] for row in results))
    (out / 'cases.json').write_text(json.dumps(results, indent=2) + '\n')
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(cases=len(results), all_equal=True, faults=report['faults'], tables=tables)))


if __name__ == '__main__':
    main()
