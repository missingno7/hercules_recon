"""Build and run the focused ENG1 source-binding fixture with pinned MSVC 5.0."""
import json
import os
from pathlib import Path
import subprocess
import sys

from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'work/eng1_mode_bindings_check'
ADAPTER = ROOT / 'src/eng1/mode_handler_bindings.c'
FIXTURE = ROOT / 'tests/eng1_mode_handler_bindings.c'
EVIDENCE = ROOT / 'evidence/eng1_mode_handler_bindings.json'


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    env = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    env['INCLUDE'] = str(DEFAULT_TOOLCHAIN / 'include')
    env['LIB'] = str(DEFAULT_TOOLCHAIN / 'lib')
    exe = OUT / 'eng1_mode_handler_bindings.exe'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2',
               '/Fe' + str(exe), str(FIXTURE), str(ADAPTER)]
    built = subprocess.run(command, cwd=OUT, env=env, capture_output=True,
                           text=True)
    ran = None
    if built.returncode == 0:
        ran = subprocess.run([str(exe)], cwd=OUT, env=env, capture_output=True,
                             text=True)
    report = {
        'scope': 'ENG1 source-binding fixture only; no function recovery or match claim.',
        'toolchain': toolchain,
        'adapter_source': str(ADAPTER.relative_to(ROOT)).replace('\\', '/'),
        'adapter_sha256': digest(ADAPTER.read_bytes()),
        'fixture_source': str(FIXTURE.relative_to(ROOT)).replace('\\', '/'),
        'fixture_sha256': digest(FIXTURE.read_bytes()),
        'dispatch_evidence': 'evidence/eng1_setup_source.json: dispatch at 0x1555b is cdecl, zero physical arguments, return discarded.',
        'command': command,
        'compile_exit_code': built.returncode,
        'compile_stdout': built.stdout.strip(),
        'compile_stderr': built.stderr.strip(),
        'run_exit_code': None if ran is None else ran.returncode,
        'run_stdout': '' if ran is None else ran.stdout.strip(),
        'run_stderr': '' if ran is None else ran.stderr.strip(),
        'passed': built.returncode == 0 and ran is not None and ran.returncode == 0,
    }
    report_path = OUT / 'report.json'
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'report': str(report_path), 'passed': report['passed'],
                      'run_stdout': report['run_stdout']}))
    if not report['passed']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
