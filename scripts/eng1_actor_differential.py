"""Compare the bounded ENG1 actor-pipeline path with native x86 in an isolated process."""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from title_behavioral import context_files


def run(output, native_eng1=None):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Fixture outputs must remain under ignored work/')
    output.mkdir(parents=True, exist_ok=True)
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    oracle = native_eng1.resolve() if native_eng1 is not None else ROOT / 'assets/pc/HERCULES/ENG1.DLL'
    oracle_hash = digest(oracle.read_bytes())
    if oracle_hash != '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Original ENG1 identity changed')
    sources = [ROOT / path for path in ('tests/eng1_actor_pipeline_differential.c',
               'src/eng1/sine_table.c')]
    contexts = {str(path.relative_to(ROOT)): digest(path.read_bytes())
                for source in sources for path in context_files(source)}
    environment = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        environment.pop(name, None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + environment.get('PATH', ''),
                       INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    executable = output / 'fixture.exe'
    executable.unlink(missing_ok=True)
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/MT',
               '/Fo' + str(output) + os.sep, '/Fe' + str(executable), *map(str, sources),
               '/link', '/INCREMENTAL:NO']
    built = subprocess.run(command, cwd=output, env=environment, capture_output=True, text=True, timeout=60)
    (output / 'compile.log').write_bytes((built.stdout + built.stderr).encode())
    report = dict(scope='512 empty-pool tick/Z states: native258e0/nested26480 owner-null versus source26480spy. Four additional25d00 level9 empty-pool paths use native2a880/type-zero extent rejection versus source bounds spies, checking consumed color-list reset and complete8c0aggregate. Not gameplay validation.',
                  toolchain=toolchain, oracle_sha256=oracle_hash, command=command,
                  context_sha256=contexts, compile_exit_code=built.returncode,
                  native_rvas=['0x258e0', '0x26480', '0x25d00', '0x2a880', '0x30e40'],
                  native_execution='DONT_RESOLVE_DLL_REFERENCES in standalone x86 fixture; loaded data cells and fake buffers only.',
                  limitations=['Finite zero-pool bounds and kind_info[0] empty-owner path; not nonzero actors, all nested behavior or universal equivalence.', 'No code-page patches or game process.',
                               'Whole-DLL integration and callback reentrancy remain untested.'])
    if built.returncode == 0:
        tested = subprocess.run([str(executable), str(oracle)], cwd=output, env=environment,
                                capture_output=True, text=True, timeout=30)
        report.update(exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr,
                      executable_sha256=digest(executable.read_bytes()))
        reset = re.search(r'color_reset_native_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
        if reset:
            report.update(color_reset_cases=int(reset[1]),color_reset_checks=int(reset[2]),color_reset_failures=int(reset[3]))
        (output / 'run.log').write_bytes((tested.stdout + tested.stderr).encode())
    else:
        report.update(exit_code=None, stdout='', stderr=built.stderr)
    report['inputs_unchanged'] = all(digest((ROOT / path).read_bytes()) == value for path, value in contexts.items())
    report['oracle_unchanged'] = digest(oracle.read_bytes()) == oracle_hash
    (output / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(report['stdout'], end='')
    if built.returncode:
        print(built.stdout + built.stderr)
    return 0 if report['exit_code'] == 0 and report['inputs_unchanged'] and report['oracle_unchanged'] and report.get('color_reset_cases')==4 and report.get('color_reset_failures')==0 else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_behavioral/differential/actor_pipeline')
    parser.add_argument('--native-eng1', type=Path, help='Read-only hash-pinned ENG1 oracle from another checkout')
    args = parser.parse_args()
    raise SystemExit(run(args.output,args.native_eng1))
