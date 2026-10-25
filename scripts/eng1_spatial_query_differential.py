"""Finite original/source ENG1 spatial queries. Diagnostic only; never matching acceptance."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from coff import COFF
from env import ROOT as SCRIPT_ROOT
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from masked import masked_compare
from pe import PE
from title_link import read_map

assert ROOT == SCRIPT_ROOT
sys.path.insert(0, 'C:/tools/hercules/renderer-oracle-python')
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX,
    UC_X86_REG_EIP, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_ESP)

ORACLE = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
SOURCE = ROOT / 'src/eng1/spatial_query_family_1a9b0.c'
OUT = ROOT / 'work/velocity/query_canonical_semantics'
FIXTURE_OBJECT = OUT / 'fixture.obj'
EXTENTS = {'eng1_1a9b0': (0x1a9b0, 48), 'eng1_1a9e0': (0x1a9e0, 288),
           'eng1_1ab00': (0x1ab00, 80), 'eng1_26de0': (0x26de0, 176)}
DATA, STACK = 0x61000000, 0x62000000
HIT_ACTOR, MISS_ACTOR = DATA + 0x400, DATA + 0x500
ESP, STOP = STACK + 0x8000, STACK + 0xf000
SAVED = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]
pack32 = lambda value: struct.pack('<I', value & 0xffffffff)


def fixture_source(path):
    path.write_text(f'''/* Private differential fixture only: one external native service is intercepted. */
#include "{(ROOT / 'src/eng1/actor_pipeline_storage.c').as_posix()}"
#include "{(ROOT / 'src/eng1/setup_shared_state.c').as_posix()}"
#include "{(ROOT / 'src/shared/engine_interface_storage.c').as_posix()}"
#include "{(ROOT / 'src/eng1/point_in_box_2a810.c').as_posix()}"
#include "{(ROOT / 'src/shared/bounds.c').as_posix()}"
#include "{(ROOT / 'src/eng1/actor_bounds_2a880.h').as_posix()}"
unsigned char g_71ee0[0x600]; /* Synthetic rows; not a recovered storage owner/capacity. */
int __cdecl actor_bounds_2a880(void *actor, Query2a170Bounds *out, unsigned long mode)
{{ (void)actor; (void)out; (void)mode; return 0; }} /* Unicorn intercepts this entry. */
''', encoding='ascii')


def build():
    verify_toolchain(DEFAULT_TOOLCHAIN)
    runtime = Path(unicorn.__file__).parent / 'lib/unicorn.dll'
    assert unicorn.__version__ == '2.1.4'
    assert digest(runtime.read_bytes()) == '010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1'
    assert digest(ORACLE.read_bytes()) == '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757'
    OUT.mkdir(parents=True, exist_ok=True)
    fixture = OUT / 'fixture.c'
    fixture_source(fixture)
    candidate, cand_cmd = compile_source(SOURCE, ['/O2'])
    services, serv_cmd = compile_source(fixture, ['/O2', '/I' + str(ROOT / 'src/eng1'),
        '/I' + str(ROOT / 'src/shared')])
    image, mapfile = OUT / 'fixture.dll', OUT / 'fixture.map'
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    command = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/NOLOGO', '/DLL', '/NOENTRY',
        '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF', '/BASE:0x45000000',
        '/OUT:' + str(image), '/MAP:' + str(mapfile), str(candidate), str(services)]
    result = subprocess.run(command, cwd=OUT, env=env, capture_output=True, text=True, timeout=60)
    (OUT / 'link.log').write_text(result.stdout + result.stderr, encoding='utf-8')
    assert result.returncode == 0, result.stdout + result.stderr
    pe, symbols = PE(image), read_map(mapfile)
    names = ['eng1_1a9b0', 'eng1_1a9e0', 'eng1_1ab00', 'eng1_26de0',
             'actor_bounds_2a880', 'point_in_box_2a810', 'bounds_overlap',
             'g_71ee0', 'g_eng1_setup_pointer_72018', 'g_eng1_setup_pointer_72020',
             'g_engine_interface', 'g_actor_pipeline_storage_71440', 'g_pipeline_word_6fec4']
    assert all('_' + n in symbols for n in names), [n for n in names if '_' + n not in symbols]
    sizes = {n: COFF(candidate).function('_' + n)['size'] for n in EXTENTS}
    metrics = {n: masked_compare(candidate, '_' + n, ORACLE, *EXTENTS[n]) for n in EXTENTS}
    return dict(candidate=candidate, services=services, pe=pe, symbols=symbols,
        compiled_sizes=sizes, masked=metrics,
        commands=[cand_cmd, serv_cmd, command], object_hashes=[digest(candidate.read_bytes()), digest(services.read_bytes())],
        image_sha256=digest(image.read_bytes())), pe, symbols


def signed_words(u, address):
    return list(struct.unpack('<6h', u.mem_read(address, 12)))


def run(pe, symbols, case, original):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    base = pe.image_base
    extent = (pe.pe.OPTIONAL_HEADER.SizeOfImage + 0xfff) & ~0xfff
    u.mem_map(base, extent)
    u.mem_write(base, pe.pe.get_memory_mapped_image().ljust(extent, b'\0'))
    u.mem_map(DATA, 0x4000)
    u.mem_map(STACK, 0x10000)
    u.mem_write(DATA, bytes(0x4000))

    owner = DATA + 0x100
    context_actor = DATA + 0x200
    query_actor = DATA + 0x300
    hit_actor = DATA + 0x400
    miss_actor = DATA + 0x500
    query = DATA + 0x600
    for actor in (owner, context_actor, query_actor):
        u.mem_write(actor + 0x2e, struct.pack('<H', 1 if case.get('actor_gate', True) else 0))
    mute = case.get('mute', 9)
    u.mem_write(context_actor, bytes([mute & 0xff]))
    qbox = case.get('query_box', [-10, -10, -10, 10, 10, 10])
    u.mem_write(query, struct.pack('<6h', *qbox))

    def global_addr(name, rva=None):
        return base + rva if original else symbols['_' + name]
    owner18 = global_addr('g_eng1_setup_pointer_72018', 0x72018)
    owner20 = global_addr('g_eng1_setup_pointer_72020', 0x72020)
    interface = (base + 0x70f20) if original else symbols['_g_engine_interface']
    storage = (base + 0x71440) if original else symbols['_g_actor_pipeline_storage_71440']
    table = (base + 0x71ee0) if original else symbols['_g_71ee0']
    word = (base + 0x6fec4) if original else symbols['_g_pipeline_word_6fec4']
    worklist = (base + 0x71900) if original else storage + 0x4c0
    u.mem_write(owner18, pack32(context_actor))
    u.mem_write(owner20, pack32(owner))
    u.mem_write(interface + 4, pack32(context_actor))
    rows = case.get('rows', [])
    u.mem_write(table, bytes(max(12, len(rows) * 12)))
    u.mem_write(owner + 0x36, struct.pack('<H', len(rows)))
    for i, row in enumerate(rows):
        u.mem_write(table + 12 * i, struct.pack('<6h', *row))
    u.mem_write(worklist, bytes(32))
    u.mem_write(worklist, pack32(case.get('worklist_count', len(case.get('worklist', [])))))
    actor_positions = {hit_actor: [0x00050000, 0x00050000, 0x00050000],
                       miss_actor: [0x00300000, 0x00300000, 0x00300000]}
    for i, ptr in enumerate(case.get('worklist', []), 1):
        u.mem_write(worklist + i * 4, pack32(ptr))
    for ptr, xyz in actor_positions.items():
        u.mem_write(ptr, b''.join(pack32(x) for x in xyz))
    pipeline_box = case.get('pipeline_box', [-10, -10, -10, 10, 10, 10])
    u.mem_write(storage, struct.pack('<6h', *pipeline_box))
    u.mem_write(word, struct.pack('<H', case.get('pipeline_enabled', 0)))

    if original:
        entries = {base + rva: name for name, (rva, _) in EXTENTS.items()}
        service = base + 0x2a880
        point = base + 0x2a810
        overlap = base + 0x2a7a0
    else:
        entries = {symbols['_' + name]: name for name in EXTENTS}
        service = symbols['_actor_bounds_2a880']
        point = symbols['_point_in_box_2a810']
        overlap = symbols['_bounds_overlap']
    entry = base + case['rva'] if original else symbols['_' + case['name']]
    args = [query_actor] if case['name'] == 'eng1_26de0' else [query]
    frame = pack32(STOP) + b''.join(pack32(x) for x in args) + bytes([0xa5]) * 20
    u.mem_write(ESP, frame)
    u.reg_write(UC_X86_REG_ESP, ESP)
    for reg, value in zip(SAVED, [0x12345678, 0x23456789, 0x3456789a, 0x456789ab]):
        u.reg_write(reg, value)
    events, writes = [], []
    stop_ranges = [(STACK, STACK + 0x10000)]

    def read32(address):
        return struct.unpack('<I', u.mem_read(address, 4))[0]

    def code(uc, address, size, user):
        if address == STOP:
            uc.emu_stop()
            return
        if address in entries:
            events.append(['call', entries[address]])
        if address == service:
            sp = uc.reg_read(UC_X86_REG_ESP)
            actor, out, mode = read32(sp + 4), read32(sp + 8), read32(sp + 12)
            values = case.get('query_bounds' if mode == 1 else 'context_bounds', qbox)
            result = case.get('query_result' if mode == 1 else 'context_result', 1)
            events.append(['bounds', actor, mode, result, list(values) if result else None])
            if result:
                uc.mem_write(out, struct.pack('<6h', *values))
            uc.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
            uc.reg_write(UC_X86_REG_EIP, read32(sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 4)
            return
        if address == point:
            sp = uc.reg_read(UC_X86_REG_ESP)
            x, y, z, box = [read32(sp + 4 + 4 * i) for i in range(4)]
            events.append(['point', x, y, z, signed_words(uc, box)])
        if address == overlap:
            sp = uc.reg_read(UC_X86_REG_ESP)
            left, right = read32(sp + 4), read32(sp + 8)
            events.append(['overlap', signed_words(uc, left), signed_words(uc, right)])

    def normalize(address, n, value):
        if DATA <= address and address + n <= DATA + 0x4000:
            return ['object', address - DATA, n, value]
        if STACK <= address and address + n <= STACK + 0x10000:
            return None
        if worklist <= address and address + n <= worklist + 0x100:
            return ['worklist', address - worklist, n, value]
        if storage <= address and address + n <= storage + 0x8c0:
            return ['pipeline', address - storage, n, value]
        if table <= address and address + n <= table + 0x600:
            return ['rows', address - table, n, value]
        if address in (owner18, owner20, interface + 4, word):
            return ['global', {owner18:'owner18', owner20:'owner20', interface+4:'interface.context', word:'pipeline.enabled'}[address], n, value]
        return ['other', hex(address), n, value]

    def write(uc, access, address, size, value, user):
        event = normalize(address, size, value)
        if event is not None:
            writes.append(event)

    u.hook_add(UC_HOOK_CODE, code)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.emu_start(entry, STOP, count=30000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP and u.reg_read(UC_X86_REG_ESP) == ESP + 4
    assert [u.reg_read(reg) for reg in SAVED] == SEEDS
    guard = 4 + 4 * len(args)
    assert bytes(u.mem_read(ESP, 4)) == frame[:4]
    assert bytes(u.mem_read(ESP + guard, len(frame) - guard)) == frame[guard:]
    state = dict(owner_selection=struct.unpack('<H', u.mem_read(owner + 0x36, 2))[0],
        worklist=bytes(u.mem_read(worklist, 32)).hex(), pipeline=bytes(u.mem_read(storage, 12)).hex(),
        pipeline_enabled=struct.unpack('<H', u.mem_read(word, 2))[0],
        rows=bytes(u.mem_read(table, max(12, len(rows) * 12))).hex())
    return dict(result=u.reg_read(UC_X86_REG_EAX), events=events, writes=writes, state=state)


def cases():
    row = [0, 0, 0, 0, 0, 0]
    miss = [20, 20, 20, 30, 30, 30]
    yield dict(name='eng1_1a9e0', rva=0x1a9e0, actor_gate=False, rows=[row])
    yield dict(name='eng1_1a9e0', rva=0x1a9e0, rows=[])
    yield dict(name='eng1_1a9e0', rva=0x1a9e0, rows=[miss, row])
    yield dict(name='eng1_1a9e0', rva=0x1a9e0, rows=[miss])
    yield dict(name='eng1_1ab00', rva=0x1ab00, actor_gate=False)
    yield dict(name='eng1_1ab00', rva=0x1ab00, context_result=0)
    yield dict(name='eng1_1ab00', rva=0x1ab00, context_bounds=[30, 30, 30, 40, 40, 40])
    yield dict(name='eng1_1ab00', rva=0x1ab00, query_box=[0, 0, 0, 10, 10, 10],
               context_bounds=[-10, -10, -10, 0, 0, 0]) # touching is overlap
    yield dict(name='eng1_1a9b0', rva=0x1a9b0, rows=[row], mute=9)
    yield dict(name='eng1_1a9b0', rva=0x1a9b0, rows=[miss], mute=9)
    yield dict(name='eng1_1a9b0', rva=0x1a9b0, rows=[miss], mute=0)
    yield dict(name='eng1_26de0', rva=0x26de0, query_result=0, mute=9)
    yield dict(name='eng1_26de0', rva=0x26de0, rows=[row], worklist=[HIT_ACTOR], mute=9)
    yield dict(name='eng1_26de0', rva=0x26de0, worklist=[HIT_ACTOR, MISS_ACTOR], mute=9)
    yield dict(name='eng1_26de0', rva=0x26de0, worklist=[MISS_ACTOR], mute=9,
               pipeline_enabled=1, pipeline_box=[-10, -10, -10, 10, 10, 10])
    yield dict(name='eng1_26de0', rva=0x26de0, worklist=[MISS_ACTOR], mute=9,
               pipeline_enabled=1, pipeline_box=[30, 30, 30, 40, 40, 40])
    yield dict(name='eng1_26de0', rva=0x26de0, worklist=[MISS_ACTOR], mute=9,
               pipeline_enabled=0, pipeline_box=[-10, -10, -10, 10, 10, 10])
    yield dict(name='eng1_26de0', rva=0x26de0, worklist=[MISS_ACTOR], worklist_count=-1, mute=9,
               pipeline_enabled=0, pipeline_box=[-10, -10, -10, 10, 10, 10])


def main():
    built, pe, symbols = build()
    original = PE(ORACLE)
    results = []
    for case in cases():
        native = run(original, {}, case, True)
        source = run(pe, symbols, case, False)
        assert native == source, dict(case=case, native=native, source=source)
        results.append(dict(case=case, result=native))
    report = dict(scope='Finite original/source query-family comparison under Unicorn; actor_bounds_2a880 is a shared fixture service model; no game execution.',
        oracle_sha256=digest(ORACLE.read_bytes()), source_sha256=digest(SOURCE.read_bytes()),
        candidate_sha256=built['object_hashes'][0], fixture_sha256=built['object_hashes'][1],
        fixture_image_sha256=built['image_sha256'], cases=len(results), all_equal=True,
        per_function_masked=built['masked'], toolchain='pinned msvc5_rtm /O2; two CL invocations',
        limitations=['The actor-bounds producer is modeled identically at the service boundary; the comparison does not prove its semantics.',
                     'Cases populate at most two rows in synthetic g_71ee0 fixture memory; its buffer span claims no production owner, capacity, or out-of-prefix behavior.',
                     'Incoming cdecl argument words may be reused as scratch; return address, ESP/callee-saved registers, and post-argument caller guards are checked. Query/worklist/pipeline state, service-call order and observed non-stack writes are compared.'])
    (OUT / 'cases.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    (OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(cases=len(results), all_equal=True, masked=built['masked'])))


if __name__ == '__main__':
    main()
