"""Guarded original/source reset loops; cleanup and initializer services modeled.

WORD limits and pool pointers can change inside a modeled callback. Synthetic
readable banks do not prove native pool capacities or full lifecycle behavior.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from masked import masked_compare
from pe import PE
from refinery_forms import function_text, replace_function
from title_behavioral import context_files
from title_link import read_map

sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)

SOURCE = ROOT / 'calibration/actor_pool_reset_repaired.c'
ALIAS = ROOT / 'src/eng1/behavioral/actor_pool_limit_aliases.h'
OWNER = ROOT / 'src/eng1/behavioral/actor_storage.c'
ORACLE = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
SPECS = {
    'normal': (0x27490, 0x134, 'g_normal_actor_pool', 0x72860, 'g_configuration_word_713b4', 0x713b4, 'remove_actor', 0x27260, 'initialize_normal_actor', 0x27520),
    'middle': (0x279a0, 0xec, 'g_middle_actor_pool', 0x73494, 'g_configuration_word_71432', 0x71432, 'remove_middle_actor', 0x27860, 'initialize_middle_actor', 0x27a50),
    'alternate': (0x27f50, 0x94, 'g_alternate_actor_pool', 0x734e0, 'g_configuration_word_713ba', 0x713ba, 'release_actor', 0x27e20, 'initialize_alternate_actor', 0x27fe0)}
DATA = 0x61000000
BANKS = [DATA + 0x100, DATA + 0x2100]
STACK = 0x62000000
SP = STACK + 0x8000
STOP = STACK + 0xf000
NV = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]
pack = lambda x: struct.pack('<I', x & 0xffffffff)


def build(out):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime = Path(unicorn.__file__).parent / 'lib/unicorn.dll'
    assert unicorn.__version__ == '2.1.4' and digest(runtime.read_bytes()) == '010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes()) == '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    inputs = {Path(__file__).resolve(), ALIAS, ORACLE, ROOT / 'toolchains/manifest.json', runtime}
    for source in [SOURCE, OWNER]: inputs.update(context_files(source))
    pins = {str(p): digest(p.read_bytes()) for p in inputs}
    out.mkdir(parents=True, exist_ok=True)
    original_obj, original_cmd = compile_source(SOURCE, ['/O2'])
    coff = COFF(original_obj)
    text = SOURCE.read_text()
    keep = {'reset_' + name + '_actor_pool' for name in SPECS}
    for symbol in coff.symbols.values():
        if symbol['section'] > 0 and symbol['type'] & 0x20:
            name = symbol['name'][1:]
            if name not in keep:
                body = function_text(text, name)
                text = replace_function(text, name, body[:body.index('{')].rstrip() + ';')
    fixture = out / 'resetters.c'
    fixture.write_text('#include "' + ALIAS.as_posix() + '"\n' + text)
    services = out / 'services.c'
    services.write_text('/* Fixture-only WORD storage; production owner is independently proved. */\n'
                        'unsigned short g_configuration_word_713b4,g_configuration_word_71432,g_configuration_word_713ba;\n'
                        + '\n'.join(('int ' + name + '(void*p){for(;;){}}') for name in ['remove_actor', 'remove_middle_actor', 'release_actor'])
                        + '\n' + '\n'.join(('void ' + name + '(void*p){for(;;){}}') for name in ['clear_actor', 'initialize_normal_actor', 'initialize_middle_actor', 'initialize_alternate_actor']) + '\n')
    objects, commands = [], [original_cmd]
    for source in [fixture, OWNER, services]:
        obj, cmd = compile_source(source, ['/O2']); objects.append(obj); commands.append(cmd)
    for name, (rva, *_) in SPECS.items():
        result = masked_compare(objects[0], '_reset_' + name + '_actor_pool', ORACLE, rva, 80)
        assert result['masked_equal'], result
    image, mapfile = out / 'fixture.dll', out / 'fixture.map'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/NOLOGO', '/DLL', '/NOENTRY', '/NODEFAULTLIB',
               '/INCREMENTAL:NO', '/OPT:NOREF', '/BASE:0x45000000', '/OUT:' + str(image),
               '/MAP:' + str(mapfile), *map(str, objects)]
    env = os.environ.copy()
    for key in ['CL', '_CL_', 'LINK', '_LINK_']: env.pop(key, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    result = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
    (out / 'link.log').write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
    return dict(input_sha256=pins, commands=commands + [command], image_sha256=digest(image.read_bytes()),
                object_sha256=[digest(p.read_bytes()) for p in objects]), PE(image), read_map(mapfile)


def execute(pe, symbols, case, native):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(pe.image_base, extent); u.mem_write(pe.image_base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(DATA, 0x5000); u.mem_map(STACK, 0x10000)
    u.mem_write(DATA, bytes((i * 13 + 31) & 255 for i in range(0x5000)))
    rva, stride, pool_name, pool_rva, limit_name, limit_rva, remove_name, remove_rva, init_name, init_rva = SPECS[case['pool']]
    entry = pe.image_base + rva if native else symbols['_reset_' + case['pool'] + '_actor_pool']
    pool = pe.image_base + pool_rva if native else symbols['_' + pool_name]
    limit = pe.image_base + limit_rva if native else symbols['_' + limit_name]
    services = {pe.image_base + r if native else symbols['_' + name]: role for name, r, role in
                [(remove_name, remove_rva, 'remove'), ('clear_actor', 0x55b0, 'clear'), (init_name, init_rva, 'init')]}
    u.mem_write(pool, pack(BANKS[0])); u.mem_write(limit, struct.pack('<H', case['count']))
    frame = pack(STOP) + b'\x93' * 24; u.mem_write(SP, frame)
    u.reg_write(UC_X86_REG_ESP, SP)
    for reg, seed in zip(NV, SEEDS): u.reg_write(reg, seed)
    calls, removals, mutated = [], 0, False
    def get32(at): return struct.unpack('<I', u.mem_read(at, 4))[0]
    def code(uc, address, size, user):
        nonlocal removals, mutated
        if address == STOP: uc.emu_stop(); return
        if address in services:
            role = services[address]; esp = uc.reg_read(UC_X86_REG_ESP); actor = get32(esp + 4)
            assert any(bank <= actor < bank + 4 * stride for bank in BANKS), hex(actor)
            calls.append(dict(role=role, actor=actor))
            if role == 'remove':
                result = case['pattern'][removals % len(case['pattern'])]; removals += 1
            else:
                assert len(calls) >= 2 and calls[-2]['role'] == 'remove' and calls[-2]['actor'] == actor
                result = 0x12345678
            if not mutated and (role == 'remove') == (case['stage'] == 'remove'):
                if case['change'] != 'none':
                    u.mem_write(limit, struct.pack('<H', 0 if case['change'] == 'shrink' else 3))
                if case['switch']: u.mem_write(pool, pack(BANKS[1]))
                mutated = True
            u.mem_write(actor + 0x20, pack(0xc1000000 + len(calls)))
            uc.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
            uc.reg_write(UC_X86_REG_EIP, get32(esp)); uc.reg_write(UC_X86_REG_ESP, esp + 4)
            return
        assert entry <= address and address + size <= entry + 80, hex(address)
    def write(uc, access, address, size, value, user):
        assert STACK <= address and address + size <= SP, hex(address)
    u.hook_add(UC_HOOK_CODE, code); u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.emu_start(entry, STOP + 1, count=10000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == SP + 4
    assert bytes(u.mem_read(SP, len(frame))) == frame and [u.reg_read(r) for r in NV] == SEEDS
    return dict(calls=calls, data_sha256=digest(bytes(u.mem_read(DATA, 0x5000))),
                pool=get32(pool), limit=struct.unpack('<H', u.mem_read(limit, 2))[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/pool_limit_alias_2026_10_25/differential')
    out = parser.parse_args().output.resolve(); assert out.is_relative_to(ROOT / 'work')
    built, source, symbols = build(out); native = PE(ORACLE); results = []; counts = {}
    for pool, count, pattern, change, switch, stage in itertools.product(SPECS, [0, 1, 2, 4, 65535], [[0], [1], [-1], [0, 1, -1]], ['none', 'shrink', 'grow'], [False, True], ['remove', 'finish']):
        if count == 65535 and change != 'shrink': continue
        case = dict(pool=pool, count=count, pattern=pattern, change=change, switch=switch, stage=stage)
        a, b = execute(native, symbols, case, True), execute(source, symbols, case, False)
        assert a == b, dict(case=case, native=a, source=b)
        results.append(dict(case=case, result=a)); counts[pool] = counts.get(pool, 0) + 1
    assert all(digest(Path(p).read_bytes()) == h for p, h in built['input_sha256'].items())
    report = dict(scope=__doc__, all_equal=True, cases=len(results), by_pool=counts, build=built,
                  cases_sha256=digest(json.dumps(results, sort_keys=True).encode()),
                  limits=['Cleanup/initializers modeled with cdecl preservation and live globals; no full lifecycle or game execution.',
                          'Four readable slots per synthetic bank; count65535 cases shrink before the next iteration and do not prove capacity.',
                          'Void-return ABI: EAX is not an output; caller frame, ESP, callee-saved values, complete fixture banks, call order and live pointer/count effects checked.'])
    (out / 'cases.json').write_text(json.dumps(results, indent=2) + '\n'); (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(cases=len(results), by_pool=counts, all_equal=True)))


if __name__ == '__main__': main()
