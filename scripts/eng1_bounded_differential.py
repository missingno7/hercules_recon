"""Fresh VC5 comparisons for resource-link initialization and command writers."""
import argparse
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
from pe import PE
from title_behavioral import context_files

PROFILES = {
    'resource-links': dict(
        sources=['tests/eng1_resource_link_differential.c',
                 'src/eng1/behavioral/resource_link_initialization.c',
                 'calibration/resource_data.c'],
        spans=[(0x39b0, 0xa0)],
        scope='Two seeded full256-row resource-link prefixes versus unpatched x86; masks/descriptors and preserved successor keys. No wider capacity, gameplay or universal equivalence claim.'),
    'command-writers': dict(
        sources=['tests/eng1_draw_commands_differential.c',
                 'tests/eng1_draw_fixture_state.c',
                 'src/eng1/behavioral/draw_commands.c',
                 'src/eng1/behavioral/cold_start_frame_data.c',
                 'src/eng1/behavioral/viewport_state_data.c',
                 'src/eng1/behavioral/color_storage.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/frame_lifecycle_bridges.c'],
        spans=[(0x33420, 0x200), (0x33620, 0xe0)],
        scope='Fourteen unpatched-x86 command-writer cases: complete128B output/canaries, callback order/args, chain/head/cursor; truncation, blend gates, unsigned edges and callback cursor mutation. Host endpoints are controlled spies; no pixel or gameplay claim.'),
}

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def run(profile, oracle):
    cfg = PROFILES[profile]
    oracle = oracle.resolve()
    if sha(oracle) != '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Unexpected ENG1 oracle identity')
    out = ROOT / 'work/eng1_behavioral/differential' / profile
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT / p for p in cfg['sources']]
    paths = {p for source in sources for p in context_files(source)} | {Path(__file__).resolve()}
    before = {p.relative_to(ROOT).as_posix(): sha(p) for p in paths}
    tool = verify_toolchain(DEFAULT_TOOLCHAIN)
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    exe = out / 'fixture.exe'
    exe.unlink(missing_ok=True)
    commands, objects, logs = [], [], []
    for index, source in enumerate(sources):
        obj = out / ('unit_' + str(index) + '.obj')
        obj.unlink(missing_ok=True)
        command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/c', '/O2', '/Gy', '/MT', '/Fo' + str(obj), str(source)]
        commands.append(command)
        built = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
        logs.append(built.stdout + built.stderr)
        if built.returncode:
            break
        objects.append(obj)
    if built.returncode == 0:
        command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/Fe' + str(exe), *map(str, objects), '/link', '/INCREMENTAL:NO', '/OPT:REF', 'kernel32.lib']
        commands.append(command)
        built = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
        logs.append(built.stdout + built.stderr)
    (out / 'compile.log').write_bytes(''.join(logs).encode())
    pe = PE(oracle)
    report = dict(scope=cfg['scope'], toolchain=tool, input_sha256=before,
                  oracle_sha256=sha(oracle), native_spans={hex(rva):dict(size=size,sha256=hashlib.sha256(pe.read_rva(rva,size)).hexdigest()) for rva,size in cfg['spans']},
                  commands=commands, compile_exit_code=built.returncode)
    if built.returncode == 0:
        tested = subprocess.run([str(exe), str(oracle)], cwd=out, env=env, capture_output=True, text=True, timeout=60)
        report.update(run_exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr, executable_sha256=sha(exe))
        if profile == 'resource-links':
            match = re.search(r'checks=(\d+) failures=(\d+)', tested.stdout)
            if match:
                report.update(checks=int(match[1]), failures=int(match[2]), cases=2)
        else:
            report.update(cases=len(re.findall(r'^differential .*: pass ', tested.stdout, re.M)), failures=0 if tested.returncode==0 else None)
    report['inputs_unchanged'] = all(sha(ROOT / p)==value for p,value in before.items()) and sha(oracle)==report['oracle_sha256']
    coverage = report.get('cases') == (14 if profile=='command-writers' else 2) and (profile!='resource-links' or report.get('checks',0)>0)
    report['result'] = 'PASS' if report.get('run_exit_code')==0 and report.get('failures')==0 and report['inputs_unchanged'] and coverage else 'FAIL'
    (out / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(report.get('stdout', ''.join(logs)))
    return 0 if report['result']=='PASS' else 1

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', choices=PROFILES)
    parser.add_argument('--native-eng1', type=Path, default=ROOT/'assets/pc/HERCULES/ENG1.DLL')
    args = parser.parse_args()
    raise SystemExit(run(args.profile, args.native_eng1))
