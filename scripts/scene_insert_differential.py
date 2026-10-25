"""Isolated unpatched scene insertion in both PC engines; no module startup."""
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
from masked import masked_compare
from pe import PE
from refinery_forms import function_text
from title_link import read_map

sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

SOURCE = ROOT / 'calibration/macro_actor.c'
OWNER = ROOT / 'src/eng1/behavioral/actor_storage.c'
MODULES = {'ENG1.DLL': (0x295a0, 0x6fed8, '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'),
           'ENG3.DLL': (0x1e440, 0x3fbc0, 'aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7')}
DATA = 0x61000000
NODES = [DATA + 0x100 + i * 0x200 for i in range(4)]
P, A, B, C = NODES
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
    inputs = {SOURCE, OWNER, Path(__file__).resolve(), runtime, ROOT / 'toolchains/manifest.json'}
    pins = {str(p): digest(p.read_bytes()) for p in inputs}
    out.mkdir(parents=True, exist_ok=True)
    text = SOURCE.read_text()
    fixture = out / 'insert.c'
    fixture.write_text(text[:text.index('void apply_frame_motion_2d(')] + function_text(text, 'insert_scene') + '\n')
    objects, commands = [], []
    for source in [fixture, OWNER]:
        obj, cmd = compile_source(source, ['/O2'])
        objects.append(obj)
        commands.append(cmd)
    for module, (entry, cell, sha) in MODULES.items():
        target = ROOT / 'assets/pc/HERCULES' / module
        assert digest(target.read_bytes()) == sha
        check = masked_compare(objects[0], '_insert_scene', target, entry, 128)
        assert check['masked_equal'] and check['symbols'] == {'_g_scene_anchor': hex(cell)}, check
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
                image_sha256=digest(image.read_bytes()), object_sha256=[digest(p.read_bytes()) for p in objects]), PE(image), read_map(mapfile)


def execute(pe, entry, cell, case):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(pe.image_base, extent)
    u.mem_write(pe.image_base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(DATA, 0x1000)
    u.mem_map(STACK, 0x10000)
    u.mem_write(DATA, bytes((i * 13 + 31) & 255 for i in range(0x1000)))
    for n, at in enumerate(NODES):
        flags = case['flags'] if n == 0 else (0x40000 if case['chain_flags'] & (1 << (n - 1)) else 0)
        u.mem_write(at + 0x50, pack(flags))
        u.mem_write(at + 0xe4, pack(NODES[n - 1] if n > 1 else 0))
        u.mem_write(at + 0xe8, pack(NODES[n + 1] if 0 < n < 3 else 0))
    u.mem_write(cell, pack(case['anchor']))
    frame = pack(STOP) + pack(P) + pack(case['after']) + b'\x93' * 24
    u.mem_write(SP, frame)
    u.reg_write(UC_X86_REG_ESP, SP)
    for reg, seed in zip(NV, SEEDS):
        u.reg_write(reg, seed)

    def code(uc, address, size, user):
        if address == STOP:
            uc.emu_stop()
            return
        assert entry <= address and address + size <= entry + 128, hex(address)

    def write(uc, access, address, size, value, user):
        assert (DATA <= address and address + size <= DATA + 0x1000 or
                STACK <= address and address + size <= SP), hex(address)

    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.emu_start(entry, STOP + 1, count=1000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    assert u.reg_read(UC_X86_REG_ESP) == SP + 4
    assert bytes(u.mem_read(SP, len(frame))) == frame
    assert [u.reg_read(r) for r in NV] == SEEDS
    result = u.reg_read(UC_X86_REG_EAX)
    assert result == (0 if case['flags'] & 0x80000 else case['after'] or case['anchor'])
    return dict(result=result, data_sha256=digest(bytes(u.mem_read(DATA, 0x1000))),
                anchor=struct.unpack('<I', u.mem_read(cell, 4))[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/scene_cell_2026_10_25/differential')
    out = parser.parse_args().output.resolve()
    assert out.is_relative_to(ROOT / 'work')
    built, source, symbols = build(out)
    originals = {m: PE(ROOT / 'assets/pc/HERCULES' / m) for m in MODULES}
    results = []
    for flags, after, anchor, chain in itertools.product([0, 0x40000, 0x80000, 0xc0000, 0x40001, 0x80001, 0xffffffff], [0, A, B, P], [0, A, B, P], range(8)):
        case = dict(flags=flags, after=after, anchor=anchor, chain_flags=chain)
        expected = execute(source, symbols['_insert_scene'], symbols['_g_scene_anchor'], case)
        for module, (entry, cell, sha) in MODULES.items():
            pe = originals[module]
            actual = execute(pe, pe.image_base + entry, pe.image_base + cell, case)
            assert actual == expected, dict(module=module, case=case, native=actual, source=expected)
        results.append(dict(case=case, result=expected))
    assert all(digest(Path(p).read_bytes()) == h for p, h in built['input_sha256'].items())
    report = dict(scope=__doc__, all_equal=True, cases_per_module=len(results), comparisons=len(results) * len(MODULES),
                  build=built, cases_sha256=digest(json.dumps(results, sort_keys=True).encode()),
                  limits=['Synthetic acyclic0..3-node chains, null/default/explicit/self anchors and flags only; no whole-game state or native allocation-capacity claim.',
                          'Identical source COMMON declaration represents each separate native cell for matching; no ENG3 source-graph integration or cross-module shared runtime cell claim.'])
    (out / 'cases.json').write_text(json.dumps(results, indent=2) + '\n')
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(cases_per_module=len(results), comparisons=report['comparisons'], all_equal=True)))


if __name__ == '__main__':
    main()
