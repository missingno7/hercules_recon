"""Fresh VC5 bounded comparisons against the unpatched ENG1 x86 oracle."""
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
    'payload-dispatch': dict(
        sources=['tests/eng1_payload_dispatch_differential.c',
                 'src/eng1/behavioral/payload_dispatch.c',
                 'src/eng1/behavioral/payload_dispatch_data.c',
                 'calibration/resource_data.c',
                 'calibration/dispatcher_bridges.c',
                 'calibration/engine_interface_storage.c',
                 'src/eng1/behavioral/dispatch_storage.c',
                 'src/eng1/behavioral/frame_commands.c',
                 'src/eng1/behavioral/host_wrappers.c',
                 'src/eng1/behavioral/level_storage.c'],
        spans=[(0x4ca0, 1072)], cases=6,
        scope='Four payload-family controls and two queue-flush cases versus unpatched x86, with actual source queue consumer and controlled host endpoints. Optional pinned captured startup prefix adds one case; its bytes remain private and its seed state is not a complete live snapshot.'),
    'dialog-frame': dict(
        sources=['tests/eng1_dialog_frame_differential.c',
                 'tests/eng1_draw_fixture_state.c',
                 'src/eng1/behavioral/frame_runtime_callees.c',
                 'src/eng1/behavioral/frame_program_data.c',
                 'src/eng1/behavioral/draw_commands.c',
                 'src/eng1/behavioral/cold_start_frame_data.c',
                 'src/eng1/behavioral/viewport_state_data.c',
                 'src/eng1/behavioral/color_storage.c',
                 'src/eng1/behavioral/runtime_storage.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/frame_lifecycle_bridges.c'],
        spans=[(0x1f40, 0x1a1), (0x2450, 0x73), (0x24d0, 0x10),
               (0x2c60, 0x40), (0x2c9b0, 0x10)], cases=147,
        success_marker='2c60, 132 state-value and 6 branch-context 1f40, and 6 direct 2450 native differentials passed; missing-NUL guard trapped at the measured prefix',
        scope='147 finite native comparisons plus a source-only missing-NUL guard discriminator. Actual source dialog/data/command writers; host endpoints are controlled spies. Mutable strings, pointer/operation changes, width/context/frame/cursor callback mutation and DWORD wrap covered. No pixel or gameplay claim.'),
    'simulation-root': dict(
        sources=['tests/eng1_simulation_iteration_differential.c',
                 'src/eng1/behavioral/simulation_storage.c'],
        spans=[(0x156a0, 209)], cases=6,
        scope='Six cases/36 checks for the actual source156a0 body and scalar storage. Standalone test-only wrappers execute original15780 and12620, with bounded zero-row preconditions; production contains separate C dependencies. No fully source simulation or gameplay claim.'),
    'resource-links': dict(
        sources=['tests/eng1_resource_link_differential.c',
                 'src/eng1/behavioral/resource_link_initialization.c',
                 'calibration/resource_data.c'],
        spans=[(0x39b0, 0xa0)], cases=2,
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
        spans=[(0x33420, 0x200), (0x33620, 0xe0)], cases=14,
        scope='Fourteen unpatched-x86 command-writer cases: complete128B output/canaries, callback order/args, chain/head/cursor; truncation, blend gates, unsigned edges and callback cursor mutation. Host endpoints are controlled spies; no pixel or gameplay claim.'),
    'descriptor-reload': dict(
        sources=['tests/eng1_descriptor_reload_differential.c',
                 'src/eng1/behavioral/descriptor_resource_reload.c',
                 'calibration/resource_data.c',
                 'src/eng1/behavioral/resource_data.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/engine_file_bridge.c',
                 'calibration/resource_provider.c',
                 'calibration/resource_callbacks.c',
                 'calibration/resource_destroy.c',
                 'calibration/frame_lifecycle_bridges.c'],
        spans=[(0x33e0, 0x30), (0x3410, 0x60)], cases=3,
        flags={'calibration/resource_provider.c': [
            '/Dresource_allocate=engine_pool_allocate',
            '/Dresource_file_length=engine_file_length',
            '/Dresource_load_async=engine_file_load_async',
            '/Dresource_load_sync=engine_file_load_sync']},
        scope='Three finite synchronous reload scenarios versus unpatched33e0/3410: all81 rows, provider globals, context, buffer contents and host callback order/arguments. Real source resource_provider and resource_load_complete; only host endpoints and valid-list guard counting are spies. Failure/reclaim/async paths and gameplay unverified.'),
}

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def run(profile, oracle, captured_payload=None):
    cfg = dict(PROFILES[profile])
    oracle = oracle.resolve()
    if sha(oracle) != '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Unexpected ENG1 oracle identity')
    out = ROOT / 'work/eng1_behavioral/differential' / profile
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT / p for p in cfg['sources']]
    paths = {p for source in sources for p in context_files(source)} | {Path(__file__).resolve()}
    if captured_payload is not None:
        if profile != 'payload-dispatch':
            raise ValueError('Captured payload is supported only by payload-dispatch')
        captured_payload = captured_payload.resolve()
        if sha(captured_payload) != '8d3fb673b1806e5c5f502df64eb75edc26a3b380eb472e5be30a469d6889f21d':
            raise ValueError('Captured payload differs from the reviewed startup prefix')
        paths.add(captured_payload)
        cfg['cases'] = 7
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
        flags = cfg.get('flags', {}).get(source.relative_to(ROOT).as_posix(), [])
        command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/c', '/O2', '/Gy', '/MT', *flags, '/Fo' + str(obj), str(source)]
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
        arguments = [str(exe), str(oracle)]
        if captured_payload is not None:
            arguments.append(str(captured_payload))
        tested = subprocess.run(arguments, cwd=out, env=env, capture_output=True, text=True, timeout=60)
        report.update(run_exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr, executable_sha256=sha(exe))
        if profile == 'payload-dispatch':
            match = re.search(r'native/source 0x4ca0 cases=(\d+)/(\d+)', tested.stdout)
            if match:
                report.update(cases=int(match[1]), failures=int(match[2])-int(match[1]), checks=76 if captured_payload is not None else 67)
        elif 'success_marker' in cfg:
            if cfg['success_marker'] in tested.stdout:
                report.update(cases=cfg['cases'], failures=0, source_guard_cases=1)
        elif profile != 'command-writers':
            match = re.search(r'checks=(\d+) failures=(\d+)', tested.stdout)
            if match:
                report.update(checks=int(match[1]), failures=int(match[2]), cases=cfg['cases'])
        else:
            report.update(cases=len(re.findall(r'^differential .*: pass ', tested.stdout, re.M)), failures=0 if tested.returncode==0 else None)
    report['inputs_unchanged'] = all(sha(ROOT / p)==value for p,value in before.items()) and sha(oracle)==report['oracle_sha256']
    coverage = report.get('cases') == cfg['cases'] and (profile=='command-writers' or 'success_marker' in cfg or report.get('checks',0)>0)
    report['result'] = 'PASS' if report.get('run_exit_code')==0 and report.get('failures')==0 and report['inputs_unchanged'] and coverage else 'FAIL'
    (out / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(report.get('stdout', ''.join(logs)))
    return 0 if report['result']=='PASS' else 1

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', choices=PROFILES)
    parser.add_argument('--native-eng1', type=Path, default=ROOT/'assets/pc/HERCULES/ENG1.DLL')
    parser.add_argument('--captured-payload', type=Path, help='Optional reviewed private startup input; bytes are never saved in public source')
    args = parser.parse_args()
    raise SystemExit(run(args.profile, args.native_eng1, args.captured_payload))
