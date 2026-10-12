"""Native color differential: fresh pinned compile then isolated execution."""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from title_behavioral import context_files
from pe import PE


def run(output):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Outputs must remain under ignored work/')
    output.mkdir(parents=True, exist_ok=True)
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    oracle = ROOT / 'assets/pc/HERCULES/ENG1.DLL'
    oracle_hash = digest(oracle.read_bytes())
    if oracle_hash != '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Original ENG1 identity changed')
    host = ROOT / 'work/discs/pc_install/HERCULES.EXE'
    host_hash = digest(host.read_bytes())
    if host_hash != '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d':
        raise ValueError('Original host EXE identity changed')
    host_load = PE(host).read_rva(0x70c0, 4)
    if host_load != bytes.fromhex('8a442404'):
        raise ValueError('Original host byte argument load changed')
    sources = [ROOT / path for path in ('tests/eng1_color_differential.c',
        'src/eng1/behavioral/color.c', 'src/eng1/behavioral/color_storage.c',
        'src/eng1/behavioral/host_color_callback.c')]
    contexts = {str(path.relative_to(ROOT)): digest(path.read_bytes())
                for source in sources for path in context_files(source)}
    prediction = ROOT / 'evidence/eng1_color_differential_prediction.json'
    contexts[str(prediction.relative_to(ROOT))] = digest(prediction.read_bytes())
    contexts[str(Path(__file__).relative_to(ROOT))] = digest(Path(__file__).read_bytes())
    environment = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        environment.pop(name, None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + environment.get('PATH', ''),
                       INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    executable = output / 'fixture.exe'
    executable.unlink(missing_ok=True)
    command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/MT',
               '/Fo' + str(output) + os.sep, '/Fe' + str(executable), *map(str, sources),
               '/link', '/INCREMENTAL:NO', 'kernel32.lib']
    built = subprocess.run(command, cwd=output, env=environment, capture_output=True, text=True, timeout=60)
    (output / 'compile.log').write_bytes((built.stdout + built.stderr).encode())
    report = dict(scope='Bounded native original vs canonical source color helper behavior; not linked ENG1 or gameplay validation.',
        toolchain=toolchain, oracle_sha256=oracle_hash, command=command,
        host_low_byte_evidence=dict(path=str(host.relative_to(ROOT)), sha256=host_hash,
            rva='0x70c0', input_load_bytes=host_load.hex(), input_load='mov al, byte ptr [esp+4]',
            scope='Read-only exact native instruction corroborates existing host-body evidence; host code is not executed by this fixture.',
            existing_body_evidence='evidence/eng1_runtime_color.json'),
        context_sha256=contexts, compile_exit_code=built.returncode,
        native_rvas=['0x5a60', '0x5d50', '0x5dc0', '0x5e00', '0x2c830'],
        native_execution='Standalone x86 DONT_RESOLVE_DLL_REFERENCES, synthetic guarded base/current frame views, copied interface spy.',
        memory_checks='Snapshot every mapped section, allow only seven named output field ranges; restore original writable sections between cases and before unload.',
        callbacks='Compare low byte, order, all 512 DWORDs, both complete guarded frames and scalar state at invocation. Capture full DWORD and separately report fade upper residue differences. Direct wrapper probes require full DWORD equality.',
        limitations=['Finite input states, not universal equivalence.', 'No instruction patches or game process.',
                     'No FUNCTION_MATCH, source identity, module integration, callback reentrancy or gameplay claim.'])
    if built.returncode == 0:
        tested = subprocess.run([str(executable), str(oracle)], cwd=output, env=environment,
                                capture_output=True, text=True, timeout=60)
        report.update(exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr,
                      executable_sha256=digest(executable.read_bytes()),
                      object_sha256={p.name: digest(p.read_bytes()) for p in output.glob('*.obj')})
        (output / 'run.log').write_bytes((tested.stdout + tested.stderr).encode())
        summary = re.search(r'native_states=(\d+) checks=(\d+) failures=(\d+) raw_callback_differences=(\d+)', tested.stdout)
        if summary:
            states, checks, failures, raw_differences = map(int, summary.groups())
            report.update(native_states=states, checks=checks, semantic_failures=failures,
                low_byte_semantic_status='PASS' if tested.returncode == 0 and failures == 0 else 'FAIL',
                raw_fade_callback_dword_status='DIAGNOSTIC_DISAGREEMENT' if raw_differences else 'NO_OBSERVED_DISAGREEMENT',
                raw_fade_callback_dword_differences=raw_differences,
                raw_callback_observations=[line for line in tested.stdout.splitlines() if line.startswith('RAW_CALLBACK ')],
                original_code_and_data_neighbors_preserved=tested.returncode == 0 and failures == 0)
    else:
        report.update(exit_code=None, stdout='', stderr=built.stderr)
    report['inputs_unchanged'] = all(digest((ROOT / path).read_bytes()) == value for path, value in contexts.items())
    report['oracle_unchanged'] = digest(oracle.read_bytes()) == oracle_hash
    report['host_oracle_unchanged'] = digest(host.read_bytes()) == host_hash
    (output / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(report['stdout'], end='')
    if built.returncode:
        print(built.stdout + built.stderr)
    return 0 if report['exit_code'] == 0 and report['inputs_unchanged'] and report['oracle_unchanged'] and report['host_oracle_unchanged'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_behavioral/native_color_validation')
    args = parser.parse_args()
    raise SystemExit(run(args.output))
