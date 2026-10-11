"""Differential the linked behavioral TITLE DLL against the original in memory."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from pe import PE
from title_link import read_map


LANE = ROOT / 'work/title_behavioral/differential/newDLL'
FIXTURE = ROOT / 'tests/title_linked_differential.c'
RUNNER = Path(__file__).resolve()
ORIGINAL = ROOT / 'assets/pc/HERCULES/TITLE.DLL'
BUILD_DIR = ROOT / 'work/title_behavioral/build'
BUILT_DLL = BUILD_DIR / 'TITLE.DLL'
BUILT_MAP = BUILD_DIR / 'TITLE.map'
BUILD_REPORT = BUILD_DIR / 'report.json'
STAGED_DLL = LANE / 'TITLE_RECON.DLL'
RECOVERY = ROOT / 'recovery.json'
BUILD_CONFIG = ROOT / 'evidence/title_behavioral_build.json'
DATA_CONFIG = ROOT / 'evidence/title_behavioral_data.json'
NATIVE_CELL_DATA_RVA = 0x2d340
CELL_PREFIX_BYTES = 3072
INTERFACE_BYTES = 1000
WRITABLE = 0x80000000
EXECUTABLE = 0x20000000


def hash_context(build_report):
    rows = build_report.get('context_sha256', {})
    changed = []
    for relpath, expected in rows.items():
        path = ROOT / relpath
        if not path.is_file() or digest(path.read_bytes()) != expected:
            changed.append(relpath)
    if changed:
        raise ValueError('Behavioral build context changed after link: ' + ', '.join(changed[:12]))
    return rows


def section_for(metadata, rva, size, characteristics):
    for section in metadata['sections']:
        start = section['virtual_address']
        end = start + section['virtual_size']
        if start <= rva and rva + size <= end and section['characteristics'] & characteristics == characteristics:
            return section['name']
    raise ValueError('RVA range is not in a mapped section with required permissions: ' + hex(rva))


def compile_fixture(output):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'HERCULES_TITLE_TRACE'):
        environment.pop(key, None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + environment.get('PATH', ''),
                       INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    executable = output / 'dll_diff.exe'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/ML', '/W3',
               '/Fo' + str(output) + os.sep, '/Fe' + str(executable), str(FIXTURE),
               '/link', '/INCREMENTAL:NO']
    result = subprocess.run(command, cwd=output, env=environment,
                            capture_output=True, text=True, timeout=120)
    compile_text = result.stdout + result.stderr
    (output / 'compile.log').write_text(compile_text, encoding='utf-8')
    if result.returncode or not executable.is_file():
        raise RuntimeError('Differential fixture compile failed; see ' + str(output / 'compile.log'))
    return executable, command, compile_text, environment


def report_build():
    missing = [str(path) for path in (BUILT_DLL, BUILT_MAP, BUILD_REPORT)
               if not path.is_file() or path.stat().st_size == 0]
    if missing:
        raise RuntimeError('Behavioral TITLE link is not ready; missing/empty: ' + ', '.join(missing))
    report_bytes = BUILD_REPORT.read_bytes()
    build_report = json.loads(report_bytes)
    if build_report.get('link_exit_code') != 0 or not build_report.get('dll_sha256'):
        raise RuntimeError('Behavioral build report does not record a successful natural DLL link')
    if digest(BUILT_DLL.read_bytes()) != build_report['dll_sha256']:
        raise RuntimeError('Built TITLE.DLL differs from its reported linked image')
    if build_report.get('recovery_sha256') != digest(RECOVERY.read_bytes()):
        raise RuntimeError('recovery.json changed after the behavioral DLL was linked')
    if build_report.get('config_sha256') != digest(BUILD_CONFIG.read_bytes()):
        raise RuntimeError('Behavioral build configuration changed after the DLL was linked')
    context = hash_context(build_report)
    return build_report, report_bytes, context


def metadata(path):
    result = PE(path)
    info = result.metadata()
    if info['machine'] != 0x14c:
        raise ValueError('Expected x86 PE32 DLL: ' + str(path))
    return result, info


def address_rva(symbols, name, image_base):
    if name not in symbols:
        raise ValueError('Symbol missing from fresh TITLE.map: ' + name)
    va = symbols[name]
    if va < image_base:
        raise ValueError('Mapped symbol precedes image base: ' + name)
    return va - image_base


def verify_target_rows(recovery, oracle_sha):
    build = json.loads(BUILD_CONFIG.read_bytes())
    rows = {row['symbol']: row for row in build.get('functions', [])}
    for name, rva, symbol, fragment in (
        ('object reset', 0x94c0, '_title_094c0', 'src/title/behavioral/object_setup.inc'),
        ('cell initializer', 0x4fe0, '_title_04fe0', 'src/title/behavioral/cell_init.c'),
    ):
        row = rows.get(symbol)
        if not row or int(row['rva'], 0) != rva or row.get('source_fragment') != fragment:
            raise ValueError('Behavioral build config identity changed for ' + name)
    targets = {row['target_sha256'] for row in recovery['functions']
               if row.get('target') == 'assets/pc/HERCULES/TITLE.DLL'}
    if targets and oracle_sha not in targets:
        raise ValueError('Original TITLE oracle differs from recovery target identity')


def run(output):
    output = output.resolve()
    if not output.is_relative_to(LANE.resolve()):
        raise ValueError('Output must remain under ' + str(LANE))
    output.mkdir(parents=True, exist_ok=True)
    executable, compile_command, compile_text, environment = compile_fixture(output)
    build_report, build_report_bytes, build_context = report_build()
    recovery_bytes = RECOVERY.read_bytes()
    recovery = json.loads(recovery_bytes)
    oracle_sha = digest(ORIGINAL.read_bytes())
    verify_target_rows(recovery, oracle_sha)
    oracle, oracle_info = metadata(ORIGINAL)
    candidate_sha = digest(BUILT_DLL.read_bytes())
    shutil.copyfile(BUILT_DLL, STAGED_DLL)
    if digest(STAGED_DLL.read_bytes()) != candidate_sha:
        raise RuntimeError('Staged newDLL image differs from the linked source-built DLL')
    candidate, candidate_info = metadata(STAGED_DLL)
    symbols = read_map(BUILT_MAP)
    reset_rva = address_rva(symbols, '_title_094c0', candidate.image_base)
    cell_fn_rva = address_rva(symbols, '_title_04fe0', candidate.image_base)
    cell_data_rva = address_rva(symbols, '_g_2d340', candidate.image_base)
    interface_rva = address_rva(symbols, '_g_engine_interface', candidate.image_base)
    flag_rva = address_rva(symbols, '_g_257e8', candidate.image_base)
    exports = {item['name']: item for item in candidate_info['exports']}
    entry_export = exports.get('PC_DLLEngineMain')
    if entry_export is None:
        raise ValueError('New TITLE DLL is missing PC_DLLEngineMain export')
    entry_rva = entry_export['rva']
    mapped_entry = symbols.get('_PC_DLLEngineMain')
    if mapped_entry is not None and mapped_entry - candidate.image_base != entry_rva:
        raise ValueError('Fresh map entry address does not agree with the built DLL export')
    if 'PC_DLLEngineMain' not in {item['name'] for item in oracle_info['exports']} or \
            next(item['rva'] for item in oracle_info['exports'] if item['name'] == 'PC_DLLEngineMain') != 0xc1f0:
        raise ValueError('Original entry export no longer matches its reviewed RVA')

    section_rows = {
        'candidate_reset_code': section_for(candidate_info, reset_rva, 1, EXECUTABLE),
        'candidate_cell_init_code': section_for(candidate_info, cell_fn_rva, 1, EXECUTABLE),
        'candidate_cell_prefix_data': section_for(candidate_info, cell_data_rva, CELL_PREFIX_BYTES, WRITABLE),
        'candidate_interface_data': section_for(candidate_info, interface_rva, INTERFACE_BYTES, WRITABLE),
        'candidate_first_use_data': section_for(candidate_info, flag_rva, 4, WRITABLE),
        'original_cell_prefix_data': section_for(oracle_info, NATIVE_CELL_DATA_RVA, CELL_PREFIX_BYTES, WRITABLE),
        'original_interface_data': section_for(oracle_info, 0x2bb40, INTERFACE_BYTES, WRITABLE),
        'original_first_use_data': section_for(oracle_info, 0x257e8, 4, WRITABLE),
    }
    runtime_command = [str(executable), str(ORIGINAL), str(STAGED_DLL), hex(reset_rva),
                       hex(cell_fn_rva), hex(cell_data_rva), hex(interface_rva),
                       hex(flag_rva), hex(entry_rva)]
    runtime_result = subprocess.run(runtime_command, cwd=output, env=environment,
                                    capture_output=True, text=True, timeout=120)
    runtime_text = runtime_result.stdout + runtime_result.stderr
    (output / 'runtime.log').write_text(runtime_text, encoding='utf-8')

    compiled_source_rows = [
        {key: row.get(key) for key in ('source', 'source_sha256', 'compiler_input_sha256',
                                       'extensions', 'adaptations')}
        for row in build_report.get('compiled_sources', [])
    ]
    input_paths = [RUNNER, FIXTURE, ORIGINAL, BUILT_DLL, BUILT_MAP, BUILD_REPORT,
                   RECOVERY, BUILD_CONFIG, DATA_CONFIG]
    input_hashes = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in input_paths}
    for relpath, sha in build_context.items():
        input_hashes[relpath] = sha
    if digest(ORIGINAL.read_bytes()) != oracle_sha:
        raise RuntimeError('Original TITLE.DLL file changed during the in-memory test')
    if digest(BUILT_DLL.read_bytes()) != candidate_sha or digest(STAGED_DLL.read_bytes()) != candidate_sha:
        raise RuntimeError('Source-built candidate DLL file changed during the in-memory test')
    if RECOVERY.read_bytes() != recovery_bytes or BUILD_REPORT.read_bytes() != build_report_bytes:
        raise RuntimeError('Recovery or source-built DLL report changed during the test')
    for relpath, sha in build_context.items():
        if digest((ROOT / relpath).read_bytes()) != sha:
            raise RuntimeError('Build source/header changed during the in-memory test: ' + relpath)
    report = {
        'scope': ('Behavioral differential of a freshly source-built TITLE DLL against the original DLL loaded side by side. '
                  'Runtime writes are limited to loaded-module data and test buffers; no DLL file or recovery state is modified. '
                  'This is not byte acceptance or whole-game equivalence.'),
        'toolchain': verify_toolchain(DEFAULT_TOOLCHAIN),
        'toolchain_path': str(DEFAULT_TOOLCHAIN),
        'compile_command': compile_command,
        'fixture_command': runtime_command,
        'compile_exit_code': 0,
        'runtime_exit_code': runtime_result.returncode,
        'fixture_architecture': 'x86 PE32',
        'fixture_sha256': digest(executable.read_bytes()),
        'fixture_source_sha256': digest(FIXTURE.read_bytes()),
        'source_built_dll': {
            'path': str(BUILT_DLL.relative_to(ROOT)),
            'sha256': candidate_sha,
            'map_path': str(BUILT_MAP.relative_to(ROOT)),
            'map_sha256': digest(BUILT_MAP.read_bytes()),
            'build_report_path': str(BUILD_REPORT.relative_to(ROOT)),
            'build_report_sha256': digest(build_report_bytes),
            'compiled_sources': compiled_source_rows,
            'context_sha256': build_context,
            'staged_as': str(STAGED_DLL.relative_to(ROOT)),
            'staged_sha256': digest(STAGED_DLL.read_bytes()),
        },
        'original_oracle': {
            'path': str(ORIGINAL.relative_to(ROOT)), 'sha256': oracle_sha,
            'timestamp_utc': oracle_info['timestamp_utc'], 'image_base': hex(oracle_info['image_base']),
            'exports': oracle_info['exports'],
        },
        'loaded_addressing': {
            'original_rvas': {'object_reset': hex(0x94c0), 'cell_initializer': hex(0x4fe0),
                              'g_2d340': hex(NATIVE_CELL_DATA_RVA), 'interface': hex(0x2bb40),
                              'first_use_flag': hex(0x257e8), 'entry': hex(0xc1f0)},
            'candidate_rvas_from_fresh_map': {'object_reset': hex(reset_rva),
                                              'cell_initializer': hex(cell_fn_rva),
                                              'g_2d340': hex(cell_data_rva),
                                              'interface': hex(interface_rva),
                                              'first_use_flag': hex(flag_rva),
                                              'entry_export': hex(entry_rva)},
            'candidate_sections': section_rows,
            'full_path_loads': [str(ORIGINAL), str(STAGED_DLL)],
            'distinct_module_handles_asserted_by_fixture': True,
        },
        'input_sha256': input_hashes,
        'fixture_output': runtime_text,
        'logs': {'compile': str((output / 'compile.log').relative_to(ROOT)),
                 'runtime': str((output / 'runtime.log').relative_to(ROOT))},
    }
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(runtime_text.strip() or compile_text.strip())
    print('report:', output / 'report.json')
    return runtime_result.returncode


def prepare(output):
    output = output.resolve()
    if not output.is_relative_to(LANE.resolve()):
        raise ValueError('Output must remain under ' + str(LANE))
    output.mkdir(parents=True, exist_ok=True)
    executable, command, compile_text, _ = compile_fixture(output)
    record = {'scope': 'Compile-only preparation of the loaded-DLL differential fixture; no oracle or built DLL loaded.',
              'toolchain': verify_toolchain(DEFAULT_TOOLCHAIN), 'compile_command': command,
              'compile_exit_code': 0, 'fixture_sha256': digest(executable.read_bytes()),
              'fixture_source_sha256': digest(FIXTURE.read_bytes()), 'compiler_output': compile_text}
    (output / 'prepare.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    print('fixture ready:', executable)
    print('prepare report:', output / 'prepare.json')
    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=LANE)
    parser.add_argument('--prepare-only', action='store_true',
                        help='Compile the fixture without requiring/loading either TITLE DLL.')
    args = parser.parse_args()
    raise SystemExit(prepare(args.output) if args.prepare_only else run(args.output))
