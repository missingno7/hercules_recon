"""Guarded grid scans composed with the real native/source 20990 decoder.

Synthetic readable rows, headers and tag-zero programs establish finite behavior,
never native allocation capacities. No OS module load or game startup occurs.
"""
import argparse
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
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

ORACLE = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
ORACLE_SHA = '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
SOURCES = ['src/eng1/behavioral/grid_query_20b50.c', 'src/eng1/behavioral/query_20990.c',
           'src/eng1/behavioral/grid_query_data.c']
DATA = 0x61000000
HEADER = DATA + 0x100
ROWS = DATA + 0x2000
STACK = 0x62000000
SP = STACK + 0x8000
STOP = STACK + 0xf000
NV = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]
pack = lambda x: struct.pack('<I', x & 0xffffffff)


def build(out, extra):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime = Path(unicorn.__file__).parent / 'lib/unicorn.dll'
    assert unicorn.__version__ == '2.1.4'
    assert digest(runtime.read_bytes()) == '010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes()) == ORACLE_SHA
    sources = [ROOT / s for s in SOURCES] + ([extra] if extra else [])
    inputs = {Path(__file__).resolve(), runtime, ORACLE, ROOT / 'toolchains/manifest.json'}
    for source in sources:
        inputs.update(context_files(source))
    pins = {str(p): digest(p.read_bytes()) for p in inputs}
    out.mkdir(parents=True, exist_ok=True)
    globals_file = out / 'globals.c'
    globals_file.write_text('''/* Fixture storage only; never a native-owner claim. */
signed char g_active_row_70d01, g_group_count_70ece;
short g_element_width_70d14;
unsigned short g_rebuilt_row_offsets_70e0c[32];
unsigned char *g_rebuilt_row_pointers_70dcc[16];
unsigned char *g_header_pointer_70d34;
unsigned char g_actor_group_sequence_5e138[112];
void eng1_incomplete_behavior(unsigned long rva,const char *why){for(;;){}}
''')
    objects, commands, functions = [], [], {}
    for source in [*sources, globals_file]:
        obj, command = compile_source(source, ['/O2'])
        objects.append(obj)
        commands.append(command)
        coff = COFF(obj)
        for symbol in coff.symbols.values():
            if symbol['section'] > 0 and symbol['type'] & 0x20:
                functions[symbol['name']] = coff.function(symbol['name'])['size']
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
    symbols = read_map(mapfile)
    allowed = [(symbols[name], symbols[name] + size) for name, size in functions.items() if name in symbols]
    return dict(input_sha256=pins, commands=commands + [command], functions=functions,
                object_sha256=[digest(p.read_bytes()) for p in objects], image_sha256=digest(image.read_bytes())), PE(image), symbols, allowed


def execute(pe, symbols, allowed, case, entry_name=None):
    native = entry_name is None
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(pe.image_base, extent)
    u.mem_write(pe.image_base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(DATA, 0x13000)
    u.mem_map(STACK, 0x10000)
    addresses = {name: pe.image_base + rva if native else symbols['_' + name] for name, rva in
                 [('g_active_row_70d01', 0x70d01), ('g_group_count_70ece', 0x70ece),
                  ('g_element_width_70d14', 0x70d14), ('g_rebuilt_row_offsets_70e0c', 0x70e0c),
                  ('g_rebuilt_row_pointers_70dcc', 0x70dcc), ('g_header_pointer_70d34', 0x70d34)]}
    u.mem_write(addresses['g_active_row_70d01'], bytes([case.get('active', 0) & 255]))
    u.mem_write(addresses['g_group_count_70ece'], bytes([case['groups']]))
    u.mem_write(addresses['g_element_width_70d14'], struct.pack('<h', case['width']))
    u.mem_write(addresses['g_rebuilt_row_offsets_70e0c'], struct.pack('<32H', *([case['extent']] * 16 + [case['base']] * 16)))
    u.mem_write(addresses['g_rebuilt_row_pointers_70dcc'], b''.join(pack(ROWS + i * 0x1000) for i in range(16)))
    u.mem_write(addresses['g_header_pointer_70d34'], pack(HEADER))
    # Tags1..3: constant/sentinel or linear tag-zero decoder programs; tag4 has
    # relative offset zero. All addresses are fixture-owned, not original assets.
    for tag, program in enumerate(case['programs'], 1):
        at = HEADER + tag * 4 - 2
        target = HEADER + (-0x80 if case.get('negative_relative') else 0x100) + tag * 16
        u.mem_write(at, struct.pack('<h', target - at))
        u.mem_write(target, bytes(program))
    row = b''.join(pack(v) for v in case['cells'])
    for i in range(16):
        u.mem_write(ROWS + i * 0x1000, row)
    args = case['args']
    frame = pack(STOP) + b''.join(pack(v) for v in args) + b'\x93' * 24
    u.mem_write(SP, frame)
    u.reg_write(UC_X86_REG_ESP, SP)
    for reg, seed in zip(NV, SEEDS):
        u.reg_write(reg, seed)
    entry = pe.image_base + 0x20b50 if native else symbols[entry_name]
    decoder = pe.image_base + 0x20990 if native else symbols['_query_decode_20990']
    ranges = [(pe.image_base + 0x20b50, pe.image_base + 0x20d30),
              (pe.image_base + 0x20990, pe.image_base + 0x20b50)] if native else allowed
    calls, reads = [], []

    def get32(at):
        return struct.unpack('<I', u.mem_read(at, 4))[0]

    def code(uc, address, size, user):
        if address == STOP:
            uc.emu_stop()
            return
        if not native:
            assert address != symbols['_eng1_incomplete_behavior'], 'source guard reached in supported fixture'
        assert any(lo <= address and address + size <= hi for lo, hi in ranges), hex(address)
        if address == decoder:
            esp = uc.reg_read(UC_X86_REG_ESP)
            calls.append([get32(esp + 4), get32(esp + 8), get32(esp + 12)])

    def read(uc, access, address, size, value, user):
        if ROWS <= address < ROWS + 0x10000:
            assert (address - ROWS) % 0x1000 + size <= len(row)
            reads.append([address - ROWS, size])

    def write(uc, access, address, size, value, user):
        assert (STACK <= address and address + size <= SP or
                SP + 4 <= address and address + size <= SP + 20), hex(address)

    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_READ, read)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.emu_start(entry, STOP + 1, count=10000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    assert u.reg_read(UC_X86_REG_ESP) == SP + 4
    assert bytes(u.mem_read(SP, 4)) == pack(STOP)
    assert bytes(u.mem_read(SP + 20, 24)) == b'\x93' * 24
    assert [u.reg_read(r) for r in NV] == SEEDS
    # Incoming coordinate copies may be reused; their values are not outputs.
    return dict(result=u.reg_read(UC_X86_REG_EAX), decoder_calls=calls, row_reads=reads,
                data_sha256=digest(bytes(u.mem_read(DATA, 0x13000))))


def cases():
    rng = random.Random(0x20b50)
    for n in range(720):
        groups = 1 + n % 16
        width = 1 + n % 8
        extent = 1 + n % 4
        base = n % 7
        xc = rng.randrange(extent + 1)
        yc = rng.randrange(width + 1)
        x = base * 64 + xc * 64 + rng.randrange(64)
        y = yc * 64 + 32 + rng.randrange(64)
        z = rng.choice([0, 63, 64, 127, 128, 2047, 65535, 0x7fffffff, 0x80000000, rng.getrandbits(32)])
        programs = [[0, rng.choice([64, 65, 95, 127, 255])], [0, 20, 50], [0, 50, 20]]
        cells = [rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]) for _ in range(128)]
        if n % 17 == 0:
            cells = [0] * 128
        yield dict(groups=groups, width=width, extent=extent, base=base, args=[x + (n % 3) * 65536, y + (n % 5) * 65536, z, rng.getrandbits(32)],
                   active=-1 if n % 29 == 0 else n % 127, programs=programs, cells=cells, negative_relative=n % 11 == 0)
    # Discriminators for a forward hit, reverse seed, empty scan and one-past read.
    for cells in [[0] * 128, [2] * 128, [3] * 128, [0] * 4 + [2] + [0] * 123]:
        yield dict(groups=1, width=4, extent=1, base=0, args=[0, 287, 0, 0], active=0,
                   programs=[[0, 65], [0, 20, 50], [0, 50, 20]], cells=cells,
                   expected_result=0x7fff7fff if not any(cells) else 0x17fff)
    yield dict(groups=1, width=4, extent=1, base=0, args=[0, 96, 0, 0], active=0,
               programs=[[0, 64], [0, 20, 50], [0, 50, 20]], cells=[4, 2] + [0] * 126,
               expected_result=0, discriminator='forward-hit reverse seed is y-adjusted minus cursor')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, help='Optional ignored private source defining grid_scan_exact')
    parser.add_argument('--output', type=Path, default=ROOT / 'work/grid_scan_2026_10_25/differential')
    args = parser.parse_args()
    out = args.output.resolve()
    assert out.is_relative_to(ROOT / 'work')
    extra = args.candidate.resolve() if args.candidate else None
    if extra:
        assert any(extra.is_relative_to(ROOT / folder) for folder in ['candidates', 'work'])
    built, source, symbols, allowed = build(out, extra)
    native = PE(ORACLE)
    names = ['_grid_scan_20b50'] + (['_grid_scan_exact'] if extra else [])
    results = []
    for number, case in enumerate(cases()):
        expected = execute(native, symbols, allowed, case)
        if 'expected_result' in case:
            assert expected['result'] == case['expected_result'], (number, case, expected)
        for name in names:
            actual = execute(source, symbols, allowed, case, name)
            assert expected == actual, dict(number=number, entry=name, case=case, native=expected, source=actual)
        results.append(dict(case=case, result=expected))
    assert all(digest(Path(p).read_bytes()) == h for p, h in built['input_sha256'].items())
    report = dict(scope=__doc__, all_equal=True, native_cases=len(results), source_entries=names,
                  comparisons=len(results) * len(names), real_decoder=True, build=built,
                  cases_sha256=digest(json.dumps(results, sort_keys=True).encode()),
                  limits=['Synthetic16-layer readable storage and tag-zero programs only; no allocation-capacity proof.',
                          'No null/nonpositive-width or nonpositive-group-count domain claim; guarded canonical behavior remains distinct.',
                          'Argument-copy values excluded; return address, ESP, callee-saved registers and caller guards checked.',
                          'Finite compositions and similar compiled extent do not grant matching acceptance.'])
    (out / 'cases.json').write_text(json.dumps(results, indent=2) + '\n')
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(native_cases=len(results), comparisons=report['comparisons'], all_equal=True)))


if __name__ == '__main__':
    main()
