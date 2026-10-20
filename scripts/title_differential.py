"""Build and run the private TITLE original-RVA differential fixture."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from pe import PE


LANE = ROOT / 'work/title_behavioral/differential'
HARNESS = ROOT / 'tests/title_oracle_differential.c'
RUNNER = Path(__file__).resolve()
UNIT = ROOT / 'calibration/title/u197f0.c'
SOURCES = [ROOT / 'src/title/pilot.c']
ORACLE = ROOT / 'assets/pc/HERCULES/TITLE.DLL'
RECOVERY = ROOT / 'recovery.json'
MANIFEST = ROOT / 'toolchains/manifest.json'
INVENTORY = ROOT / 'docs/inventory.md'


def contained(path, parent):
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def extract_unit_digits(unit_text):
    """Exactly the canonical DigitSprite definition and set_decimal_digits body from their unit (no edits)."""
    def block(start):
        depth, i = 0, unit_text.index('{', start)
        while True:
            depth += {'{': 1, '}': -1}.get(unit_text[i], 0)
            if depth == 0:
                return unit_text[start:i + 1]
            i += 1
    struct = block(unit_text.index('struct DigitSprite {')) + ';\n'
    start = unit_text.index('void set_decimal_digits(')
    while unit_text.index(';', start) < unit_text.index('{', start):   # skip prototypes
        start = unit_text.index('void set_decimal_digits(', start + 1)
    body = block(start)
    return ('/* Extracted verbatim from calibration/title/u197f0.c for the behavioral fixture. */\n'
            + struct + '\n' + body + '\n')


def run(output):
    output = output.resolve()
    if not contained(output, LANE.resolve()):
        raise ValueError('Output must remain under ' + str(LANE))
    output.mkdir(parents=True, exist_ok=True)
    toolchain_id = verify_toolchain(DEFAULT_TOOLCHAIN)
    recovery_bytes = RECOVERY.read_bytes()
    recovery = json.loads(recovery_bytes)
    manifest_bytes = MANIFEST.read_bytes()
    manifest = json.loads(manifest_bytes)
    toolchain_entry = next(entry for entry in manifest['toolchains']
                           if entry['directory'] == DEFAULT_TOOLCHAIN.name)
    rows = {row['id']: row for row in recovery['functions']
            if row.get('id') in ('title.decimal_digits', 'title.make_colour', 'title.sentinel_count')}
    expected = {
        'title.decimal_digits': ('calibration/title/u197f0.c', '_set_decimal_digits', 0x1a2c0),
        'title.make_colour': ('src/title/pilot.c', '_make_colour', 0x189a0),
        'title.sentinel_count': ('src/title/pilot.c', '_sentinel_count', 0xcee0),
    }
    for name, (source, symbol, rva) in expected.items():
        row = rows.get(name)
        if not row or row['source'] != source or row['symbol'] != symbol or int(row['rva'], 0) != rva:
            raise ValueError('Unexpected recovery identity for ' + name)
        if row['target'] != 'assets/pc/HERCULES/TITLE.DLL':
            raise ValueError('Unexpected oracle target for ' + name)
    oracle_sha = digest(ORACLE.read_bytes())
    if any(row['target_sha256'] != oracle_sha for row in rows.values()):
        raise ValueError('TITLE.DLL changed from the recovery-pinned oracle')
    metadata = PE(ORACLE).metadata()
    if metadata['machine'] != 0x14c:
        raise ValueError('Oracle is not x86 PE32')

    digits = output / 'decimal_digits_from_unit.c'
    digits.write_bytes(extract_unit_digits(UNIT.read_text()).encode())
    inputs = [RUNNER, HARNESS, UNIT, *SOURCES, RECOVERY, MANIFEST, INVENTORY, ORACLE]
    input_hashes = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in inputs}
    environment = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        environment.pop(key, None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + environment.get('PATH', ''),
                       INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    executable = output / 'oracle_diff.exe'
    object_dir = str(output) + os.sep
    compile_command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/ML', '/W3',
                       '/Fo' + object_dir, '/Fe' + str(executable), str(HARNESS),
                       str(digits), *map(str, SOURCES), '/link', '/INCREMENTAL:NO']
    compile_result = subprocess.run(compile_command, cwd=output, env=environment,
                                    capture_output=True, text=True, timeout=120)
    compile_text = compile_result.stdout + compile_result.stderr
    (output / 'compile.log').write_text(compile_text, encoding='utf-8')
    fixture_command = [str(executable), str(ORACLE)]
    runtime_result = None
    runtime_text = ''
    if compile_result.returncode == 0 and executable.is_file():
        runtime_result = subprocess.run(fixture_command, cwd=output, env=environment,
                                        capture_output=True, text=True, timeout=120)
        runtime_text = runtime_result.stdout + runtime_result.stderr
    (output / 'runtime.log').write_text(runtime_text, encoding='utf-8')

    changed_inputs = [path for path, value in input_hashes.items()
                      if digest((ROOT / path).read_bytes()) != value]
    if changed_inputs:
        raise RuntimeError('Fixture input changed during build/run: ' + ', '.join(changed_inputs))
    final_oracle_sha = digest(ORACLE.read_bytes())
    if final_oracle_sha != oracle_sha:
        raise RuntimeError('Original oracle changed during the differential run')

    fixture_metadata = PE(executable).metadata() if executable.is_file() else None
    if fixture_metadata and fixture_metadata['machine'] != 0x14c:
        raise RuntimeError('Pinned fixture unexpectedly produced a non-x86 executable')
    report = {
        'scope': ('Behavioral differential of selected reconstructed TITLE routines, called statically from their repository sources, '
                  'against original functions reached by loaded module base plus recovery-pinned RVA. Not byte acceptance or whole-DLL equivalence.'),
        'toolchain': toolchain_id,
        'toolchain_path': str(DEFAULT_TOOLCHAIN),
        'toolchain_manifest_sha256': digest(manifest_bytes),
        'toolchain_provenance': {
            'repository': toolchain_entry['repository'],
            'commit': toolchain_entry['commit'],
            'archive_sha256': toolchain_entry['archive_sha256'],
            'installed_tree': toolchain_entry['installed_tree'],
            'key_file_sha256': {item['path']: item['sha256'] for item in toolchain_entry['files']
                                if item['path'] in ('bin/cl.exe', 'bin/c1.dll', 'bin/c2.exe', 'bin/link.exe')},
        },
        'oracle_provenance': {
            'documentation': str(INVENTORY.relative_to(ROOT)),
            'documentation_sha256': input_hashes[str(INVENTORY.relative_to(ROOT))],
            'note': ('Repository inventory identifies assets/pc/HERCULES as the immutable matching targets and reports '
                     'the installer DLLs byte-identical. It also notes the source PC disc image is a later repack, '
                     'so an independently authenticated retail disc remains unverified.'),
        },
        'compile_command': compile_command,
        'fixture_command': fixture_command,
        'fixture_architecture': 'x86 PE32' if fixture_metadata else None,
        'original_exe_launched': False,
        'resolution_method': 'loaded module base + reviewed RVA; no GetProcAddress',
        'compile_exit_code': compile_result.returncode,
        'fixture_exit_code': runtime_result.returncode if runtime_result else None,
        'compile_inputs_sha256': input_hashes,
        'executable_sha256': digest(executable.read_bytes()) if executable.is_file() else None,
        'original_oracle': {
            'path': str(ORACLE.relative_to(ROOT)),
            'sha256': oracle_sha,
            'machine': 'x86 PE32',
            'image_base': hex(metadata['image_base']),
            'pe_timestamp_utc': metadata['timestamp_utc'],
            'linker_version': metadata['linker_version'],
            'exports': [{'name': item['name'], 'ordinal': item['ordinal'], 'rva': hex(item['rva'])}
                        for item in metadata['exports']],
            'imports': [{'dll': item['dll'], 'count': len(item['symbols'])}
                        for item in metadata['imports']],
            'load_method': 'LoadLibraryA; fixture calls selected functions only at loaded_base + recovery RVA',
            'file_modified': final_oracle_sha != oracle_sha,
            'functions': {name: {'symbol': row['symbol'], 'rva': row['rva'], 'size': row['size'],
                                 'recovery_status': row['status']}
                          for name, row in rows.items()},
        },
        'fixture_output': runtime_text,
        'logs': {'compile': str((output / 'compile.log').relative_to(ROOT)),
                 'runtime': str((output / 'runtime.log').relative_to(ROOT))},
    }
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(runtime_text.strip() or compile_text.strip())
    print('report:', output / 'report.json')
    return compile_result.returncode if compile_result.returncode else (
        runtime_result.returncode if runtime_result else 1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=LANE)
    arguments = parser.parse_args()
    raise SystemExit(run(arguments.output))
