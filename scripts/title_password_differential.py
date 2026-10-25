"""Compare the linked source TITLE password checker with the PC DLL in memory."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG
from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from pe import PE
from title_link import read_map


LANE = ROOT / 'work/title_behavioral/differential/password'
FIXTURE = ROOT / 'tests/title_password_differential.c'
RUNNER = Path(__file__).resolve()
ORIGINAL = ROOT / 'assets/pc/HERCULES/TITLE.DLL'
BUILD_DIR = ROOT / 'work/title_behavioral/build'
BUILT_DLL = BUILD_DIR / 'TITLE.DLL'
BUILT_MAP = BUILD_DIR / 'TITLE.map'
BUILD_REPORT = BUILD_DIR / 'report.json'
STAGED_DLL = LANE / 'TITLE_PASSWORD_RECON.DLL'
RECOVERY = ROOT / 'recovery.json'
BUILD_CONFIG = ROOT / 'config/title/sources.json'
NATIVE_FUNCTION_RVA = 0x0CD30
NATIVE_TABLE_RVA = 0x25A30
NATIVE_INTERFACE_RVA = 0x2BB40
TABLE_BYTES = 42 * 6 * 4
WRITABLE = 0x80000000
EXECUTABLE = 0x20000000


def section_for(info, rva, size, required):
    for row in info['sections']:
        start = row['virtual_address']
        if start <= rva and rva + size <= start + row['virtual_size'] and \
                row['characteristics'] & required == required:
            return row['name']
    raise ValueError('RVA range is not mapped with expected section flags: ' + hex(rva))


def check_build():
    report_bytes = BUILD_REPORT.read_bytes()
    report = json.loads(report_bytes)
    if report.get('link_exit_code') != 0 or digest(BUILT_DLL.read_bytes()) != report.get('dll_sha256'):
        raise ValueError('Fresh source-built TITLE image does not match its successful link report')
    if report.get('recovery_sha256') != digest(RECOVERY.read_bytes()):
        raise ValueError('recovery.json changed after the source-built DLL was linked')
    if report.get('config_sha256') != digest(BUILD_CONFIG.read_bytes()):
        raise ValueError('Behavioral build configuration changed after link')
    context = report.get('context_sha256', {})
    changed = [path for path, sha in context.items()
               if not (ROOT / path).is_file() or digest((ROOT / path).read_bytes()) != sha]
    if changed:
        raise ValueError('Source-built DLL context changed after link: ' + ', '.join(changed[:10]))
    return report_bytes, report, context


def candidate_private_rvas(image, function_rva, function_size):
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    instructions = list(md.disasm(image.read_rva(function_rva, function_size),
                                  image.image_base + function_rva))
    pointers = []
    flags = []
    for insn in instructions:
        if insn.mnemonic != 'mov' or len(insn.operands) != 2:
            continue
        dest, src = insn.operands
        if dest.type == X86_OP_REG and src.type == X86_OP_MEM and \
                src.mem.base == 0 and src.mem.index == 0 and len(pointers) < 4:
            pointers.append(src.mem.disp - image.image_base)
        if dest.type == X86_OP_MEM and dest.mem.base == 0 and dest.mem.index == 0 and \
                dest.size == 4 and src.type in (X86_OP_IMM, X86_OP_REG):
            flags.append(dest.mem.disp - image.image_base)
    if len(pointers) != 4 or len(flags) != 3:
        raise ValueError('Could not derive four object pointers and three special flags from linked checker')
    return pointers, flags


def compile_fixture():
    verify_toolchain(DEFAULT_TOOLCHAIN)
    env = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_', 'HERCULES_TITLE_TRACE'):
        env.pop(name, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'),
               LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    exe = LANE / 'password_diff.exe'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/ML', '/W3',
               '/Fo' + str(LANE) + os.sep, '/Fe' + str(exe), str(FIXTURE),
               '/link', '/INCREMENTAL:NO']
    result = subprocess.run(command, cwd=LANE, env=env, capture_output=True,
                            text=True, timeout=120)
    log = result.stdout + result.stderr
    (LANE / 'compile.log').write_text(log, encoding='utf-8')
    if result.returncode or not exe.is_file():
        raise RuntimeError('VC5 fixture compile failed; see ' + str(LANE / 'compile.log'))
    return exe, env, command, log


def run():
    LANE.mkdir(parents=True, exist_ok=True)
    exe, env, compile_command, compile_log = compile_fixture()
    build_report_bytes, build_report, build_context = check_build()
    recovery_bytes = RECOVERY.read_bytes()
    recovery = json.loads(recovery_bytes)
    oracle_sha = digest(ORIGINAL.read_bytes())
    row = next((r for r in recovery['functions'] if r.get('id') == 'title.0cd30'), None)
    if not row or int(row['rva'], 0) != NATIVE_FUNCTION_RVA or \
            row.get('target_sha256') != oracle_sha or row.get('source') != 'src/title/u0cc90.c':
        raise ValueError('Original password checker recovery identity/hash does not match the oracle')

    original = PE(ORIGINAL)
    built = PE(BUILT_DLL)
    symbols = read_map(BUILT_MAP)
    required_symbols = ('_title_0cd30', '_g_25a30', '_g_engine_interface')
    if any(name not in symbols for name in required_symbols):
        raise ValueError('Fresh TITLE map is missing a password checker symbol')
    candidate_fn = symbols['_title_0cd30'] - built.image_base
    candidate_table = symbols['_g_25a30'] - built.image_base
    candidate_interface = symbols['_g_engine_interface'] - built.image_base
    private_pointers, private_flags = candidate_private_rvas(
        built, candidate_fn, int(row['size']))
    sections = {
        'original_checker': section_for(original.metadata(), NATIVE_FUNCTION_RVA, int(row['size']), EXECUTABLE),
        'candidate_checker': section_for(built.metadata(), candidate_fn, int(row['size']), EXECUTABLE),
        'original_table': section_for(original.metadata(), NATIVE_TABLE_RVA, TABLE_BYTES, WRITABLE),
        'candidate_table': section_for(built.metadata(), candidate_table, TABLE_BYTES, WRITABLE),
        'candidate_interface': section_for(built.metadata(), candidate_interface, 8, WRITABLE),
    }
    candidate_sha = digest(BUILT_DLL.read_bytes())
    shutil.copyfile(BUILT_DLL, STAGED_DLL)
    if digest(STAGED_DLL.read_bytes()) != candidate_sha:
        raise ValueError('Staged candidate differs from the linked source-built DLL')

    runtime_command = [str(exe), str(ORIGINAL), str(STAGED_DLL), hex(candidate_fn),
                       hex(candidate_table), hex(candidate_interface),
                       *[hex(x) for x in private_pointers], *[hex(x) for x in private_flags]]
    runtime = subprocess.run(runtime_command, cwd=LANE, env=env,
                             capture_output=True, text=True, timeout=120)
    runtime_log = runtime.stdout + runtime.stderr
    (LANE / 'runtime.log').write_text(runtime_log, encoding='utf-8')
    return_line = next((line for line in runtime_log.splitlines()
                        if line.startswith('RETURN_COUNTS')), '')
    return_counts = {int(pair.split(':', 1)[0]): int(pair.split(':', 1)[1])
                     for pair in return_line.split()[1:]}

    if digest(ORIGINAL.read_bytes()) != oracle_sha:
        raise RuntimeError('Original asset hash changed during loaded-module test')
    if digest(BUILT_DLL.read_bytes()) != candidate_sha or digest(STAGED_DLL.read_bytes()) != candidate_sha:
        raise RuntimeError('Linked or staged candidate image changed during test')
    if RECOVERY.read_bytes() != recovery_bytes or BUILD_REPORT.read_bytes() != build_report_bytes:
        raise RuntimeError('Recovery or build report changed during test')
    for path, sha in build_context.items():
        if digest((ROOT / path).read_bytes()) != sha:
            raise RuntimeError('Build context changed during test: ' + path)

    inputs = [FIXTURE, RUNNER, ORIGINAL, BUILT_DLL, BUILT_MAP, BUILD_REPORT,
              RECOVERY, BUILD_CONFIG]
    input_hashes = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in inputs}
    input_hashes.update(build_context)
    report = {
        'scope': ('Native in-memory behavioral comparison of title_0cd30 from the fresh source-built DLL '
                  'against the original TITLE.DLL. The original is only a test oracle; no original game '
                  'process is launched and no asset/recovery/build input is modified.'),
        'result': 'pass' if runtime.returncode == 0 else 'fail',
        'runtime_exit_code': runtime.returncode,
        'toolchain': verify_toolchain(DEFAULT_TOOLCHAIN),
        'toolchain_path': str(DEFAULT_TOOLCHAIN),
        'compile_command': compile_command,
        'fixture_command': runtime_command,
        'fixture_sha256': digest(exe.read_bytes()),
        'fixture_source_sha256': digest(FIXTURE.read_bytes()),
        'original_oracle': {
            'path': str(ORIGINAL.relative_to(ROOT)), 'sha256': oracle_sha,
            'image_base': hex(original.image_base), 'checker_rva': hex(NATIVE_FUNCTION_RVA),
            'table_rva': hex(NATIVE_TABLE_RVA), 'recovery_id': row['id'],
        },
        'source_built_candidate': {
            'path': str(BUILT_DLL.relative_to(ROOT)), 'sha256': candidate_sha,
            'map_path': str(BUILT_MAP.relative_to(ROOT)),
            'map_sha256': digest(BUILT_MAP.read_bytes()),
            'staged_path': str(STAGED_DLL.relative_to(ROOT)),
            'candidate_checker_rva': hex(candidate_fn),
            'candidate_table_rva': hex(candidate_table),
            'candidate_interface_rva': hex(candidate_interface),
            'derived_pointer_global_rvas': [hex(x) for x in private_pointers],
            'derived_special_flag_rvas': [hex(x) for x in private_flags],
        },
        'coverage': {
            'valid_rows': 42,
            'invalid_inputs': 43,
            'table_bytes_compared': TABLE_BYTES,
            'input_encoding': 'four TitleObject words at +0x34 equal row keys plus biases [0x1e3,0x121,0x182,0xc0]',
            'observed_return_counts': return_counts,
            'state_compared': 'four 0x134-byte input objects, 0x40-byte context, and three special-code flags',
            'side_effects_asserted': [
                'ordinary match writes context +0x0e difficulty and +0x12 winning row index',
                'codes 101 and 103 set context dword +0x30 high bit and word +0x2e to 0 or 1',
                'code 105 after 104 sets context dword +0x30 bits 0x80000008 and byte +6 to 3, returns 1',
                'codes 100, 102, and 104 set their matching persistent special flag',
                'input objects remain unchanged; invalid inputs leave context and flags unchanged',
            ],
            'special_codes': [100, 101, 102, 103, 104, 105],
            'rng': 'no RNG calls or callbacks in the checker body',
            'miss_bound': ('The native 43rd loop read remains present. Valid cases hit one of the 42 rows; '
                           'invalid cases use first-key 0 or 0xffff, which cannot equal the 43rd first DWORD '
                           '1734692128 at RVA 0x25e20 (the first word of the adjacent trace string). '
                           'This observes but does not resolve the C out-of-bounds contract.'),
        },
        'password_unit_inventory': [
            {'rva': '0xcc90', 'symbol': '_title_0cc90', 'role': 'input magnitude smoothing; depends on title_0c4c0'},
            {'rva': '0xcce0', 'symbol': '_title_0cce0', 'role': 'finds a free animation slot in a private 64-pointer table'},
            {'rva': '0xcd00', 'symbol': '_title_0cd00', 'role': 'clears matching entries in that private slot table'},
            {'rva': '0xcd30', 'symbol': '_title_0cd30', 'role': 'four-key table validator; compared'},
            {'rva': '0xcf00', 'symbol': '_title_0cf00', 'role': 'allocates and initializes wheel objects; allocator/context dependent'},
            {'rva': '0xd250', 'symbol': '_title_0d250', 'role': 'object setup callback; uses random number generation'},
            {'rva': '0xd290', 'symbol': '_title_0d290', 'role': 'per-frame sprite update and timeout release'},
            {'rva': '0xd380', 'symbol': '_title_0d380', 'role': 'rate-limited particle allocation'},
            {'rva': '0xd440', 'symbol': '_title_0d440', 'role': 'per-frame wheel tick with context dependence'},
            {'rva': '0xd4b0', 'symbol': '_title_0d4b0', 'role': 'pad-input handler with sound and state callbacks'},
            {'rva': '0xd6c0', 'symbol': '_title_0d6c0', 'role': 'cycles selected sample through title_0c4d0'},
            {'rva': '0xd720', 'symbol': '_title_0d720', 'role': 'pad-input handler with sound and state callbacks'},
            {'rva': '0xf460', 'symbol': '_title_0f460', 'role': 'full password-screen workflow/render handler'},
        ],
        'sections': sections,
        'distinct_module_handles_asserted': True,
        'input_sha256': input_hashes,
        'compiler_output': compile_log,
        'fixture_output': runtime_log,
        'logs': {'compile': str((LANE / 'compile.log').relative_to(ROOT)),
                 'runtime': str((LANE / 'runtime.log').relative_to(ROOT))},
    }
    (LANE / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(runtime_log.strip() or compile_log.strip())
    print('report:', LANE / 'report.json')
    return runtime.returncode


if __name__ == '__main__':
    raise SystemExit(run())
