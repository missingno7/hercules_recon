"""Fresh pinned 32-bit fixtures for the behavioral TITLE branch."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from title_behavioral import context_files


def run_entry(output):
    verify_toolchain(DEFAULT_TOOLCHAIN)
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work') and not output.is_relative_to(ROOT / 'build'):
        raise ValueError('Fixture output must be in ignored work/ or build/')
    output.mkdir(parents=True, exist_ok=True)
    units = [ROOT / 'tests/title_entry_semantics.c', ROOT / 'src/title/behavioral/title_entry.c',
             ROOT / 'src/title/behavioral/title_trace.c']
    inputs = {str(p.relative_to(ROOT)): digest(p.read_bytes())
              for source in units for p in context_files(source)}
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    executable = output / 'entry_fixture.exe'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/ML',
               '/Fo' + str(output) + os.sep, '/Fe' + str(executable), *map(str, units),
               '/link', '/INCREMENTAL:NO']
    built = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True, timeout=60)
    (output / 'compile.log').write_text(built.stdout + built.stderr)
    if built.returncode:
        raise RuntimeError('Fixture compilation failed; see ' + str(output / 'compile.log'))
    result = subprocess.run([str(executable)], cwd=output, env=env, capture_output=True, text=True, timeout=30)
    for source, sha in inputs.items():
        if digest((ROOT / source).read_bytes()) != sha:
            raise RuntimeError('Fixture input changed: ' + source)
    report = dict(scope='Reconstructed source entry protocol with test-only spies; no original DLL loaded; not full startup validation.',
                  toolchain=verify_toolchain(DEFAULT_TOOLCHAIN), command=command,
                  input_sha256=inputs, executable_sha256=digest(executable.read_bytes()),
                  exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(result.stdout.strip())
    print('report:', output / 'report.json')
    return result.returncode


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/title_behavioral/entry_fixture')
    args = parser.parse_args()
    raise SystemExit(run_entry(args.output))
