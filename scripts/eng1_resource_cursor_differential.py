"""Compare source packed-resource parsing with unpatched original x86 execution."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from title_behavioral import context_files

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def run():
    out = ROOT / 'work/eng1_behavioral/differential/resource_cursor'
    out.mkdir(parents=True, exist_ok=True)
    evidence = ROOT / 'evidence/eng1_forest_actor_init.json'
    identity = json.loads(evidence.read_bytes())
    forest = identity['forest_producer']
    archive = ROOT / 'assets/pc/HERCULES/HERCULES.FS'
    oracle = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
    if sha(archive) != identity['archive_sha256']:
        raise ValueError('Game archive identity changed')
    if sha(oracle) != '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Native ENG1 oracle identity changed')
    with archive.open('rb') as stream:
        stream.seek(forest['archive_offset'])
        data = stream.read(forest['archive_length'])
    if hashlib.sha256(data).hexdigest() != forest['sha256']:
        raise ValueError('FOREST map identity changed')
    mapfile = out / 'forest_map.bin'
    mapfile.write_bytes(data)
    source = ROOT / 'tests/eng1_resource_cursor_differential.c'
    paths = context_files(source) | {oracle, archive, evidence, Path(__file__).resolve()}
    before = {p.relative_to(ROOT).as_posix(): sha(p) for p in paths}
    tool = verify_toolchain(DEFAULT_TOOLCHAIN)
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    exe = out / 'fixture.exe'
    exe.unlink(missing_ok=True)
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/MT',
               '/Fo' + str(out) + os.sep, '/Fe' + str(exe), str(source),
               '/link', '/INCREMENTAL:NO', 'kernel32.lib']
    built = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
    (out / 'compile.log').write_bytes((built.stdout + built.stderr).encode())
    report = dict(scope='Finite native20670+20830 differential on the full shipped FOREST map. Original DLL execution exists only in this fixture; no original-code dispatch in replacement ENG1.',
                  toolchain=tool, input_sha256=before, forest_map_sha256=forest['sha256'],
                  command=command, compile_exit_code=built.returncode,
                  limits='One selected group0 state, context level0 lookup. Source scalar/prefix views; not all group states or physical shared aggregate behavior. No FUNCTION_MATCH or universal equivalence claim.')
    if built.returncode == 0:
        tested = subprocess.run([str(exe), str(oracle), str(mapfile)], cwd=out, env=env,
                                capture_output=True, text=True, timeout=30)
        report.update(run_exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr,
                      executable_sha256=sha(exe))
        match = re.search(r'checks=(\d+) failures=(\d+)', tested.stdout)
        if match:
            report.update(checks=int(match[1]), failures=int(match[2]))
    report['inputs_unchanged'] = all(sha(ROOT / p) == value for p, value in before.items())
    report['result'] = 'PASS' if report.get('run_exit_code') == 0 and report.get('failures') == 0 and report['inputs_unchanged'] else 'FAIL'
    (out / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(report.get('stdout', built.stdout + built.stderr))
    return 0 if report['result'] == 'PASS' else 1

if __name__ == '__main__':
    raise SystemExit(run())
