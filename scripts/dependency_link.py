"""Fresh natural dependency link. Diagnostic only; never run the image or write recovery."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_source, compare, digest, verify_toolchain
from macro_compare import declared_context_files
from region import contribution_identity


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def run(plan_path, output):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    plan = read(plan_path)
    tc = DEFAULT_TOOLCHAIN
    toolchain = verify_toolchain(tc)
    output.mkdir(parents=True, exist_ok=True)
    lane = Path(tempfile.mkdtemp(prefix='link-', dir=output))
    objects, rows, definitions = [], [], {}
    state_hash = digest((ROOT / 'recovery.json').read_bytes())
    state = read(ROOT / 'recovery.json')
    for unit in plan['units']:
        source = ROOT / unit['source']
        before = digest(source.read_bytes())
        if 'receipt' in unit:
            receipt = read(ROOT / unit['receipt'])
            spec = read(ROOT / unit['spec'])
            if digest((ROOT / unit['spec']).read_bytes()) != receipt['provenance']['spec_sha256']:
                raise ValueError('Stale region specification: ' + unit['source'])
            if digest((ROOT / spec['target']).read_bytes()) != spec['target_sha256']:
                raise ValueError('Region oracle changed')
            expected = {f['symbol']: f['output_id'] for f in receipt['functions']}
            if before != receipt['compiled_source_sha256']:
                raise ValueError('Stale source receipt: ' + unit['source'])
            context = declared_context_files(spec, ROOT)
            if context != receipt['provenance']['context_files']:
                raise ValueError('Stale header receipt: ' + unit['source'])
            flags = spec['flags']
        else:
            accepted = next(f for f in state['functions'] if f['id'] == unit['accepted_id'])
            if accepted['status'] != 'FUNCTION_MATCH' or accepted['source'] != unit['source']:
                raise ValueError('Accepted provider identity changed')
            flags, context, expected = accepted['flags'], {}, {}
        obj, command = compile_source(source, flags)
        if digest(source.read_bytes()) != before:
            raise ValueError('Source changed during compilation')
        if 'receipt' in unit and declared_context_files(spec, ROOT) != context:
            raise ValueError('Header changed during compilation')
        coff = COFF(obj)
        identities = {}
        for sym in coff.symbols.values():
            if sym['storage'] == 2 and sym['section'] > 0:
                definitions.setdefault(sym['name'], []).append(unit['source'])
            if sym['type'] & 0x20 and sym['section'] > 0:
                identities[sym['name']] = contribution_identity(coff.function(sym['name']))
        if any(identities.get(k) != v for k, v in expected.items()):
            raise ValueError('Frozen contribution changed: ' + unit['source'])
        if 'accepted_id' in unit:
            target = ROOT / accepted['target']
            if digest(target.read_bytes()) != accepted['target_sha256']:
                raise ValueError('Accepted provider oracle changed')
            proof = compare(obj, accepted['symbol'], target, int(accepted['rva'], 0), accepted['size'])
            if not proof['exact']:
                raise ValueError('Accepted provider regressed')
        objects.append(obj)
        rows.append(dict(source=unit['source'], source_sha256=before, context_files=context,
                         command=command, object=str(obj.relative_to(ROOT)),
                         object_sha256=digest(obj.read_bytes()), output_ids=identities))
    libraries = []
    for lib in plan['libraries']:
        path = tc / lib['path']
        if digest(path.read_bytes()) != lib['sha256']:
            raise ValueError('Library provider changed')
        libraries.append(path)
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(tc / 'bin') + os.pathsep + env.get('PATH', ''), LIB=str(tc / 'lib'))
    command = [str(tc / 'bin/link.exe'), '/nologo', '/DLL', '/NOENTRY', '/NODEFAULTLIB',
               '/INCREMENTAL:NO', '/OPT:NOREF', '/OUT:' + str(lane / 'diagnostic.dll'),
               '/MAP:' + str(lane / 'diagnostic.map'), *map(str, objects), *map(str, libraries)]
    result = subprocess.run(command, cwd=lane, env=env, capture_output=True, text=True, timeout=60)
    log = result.stdout + result.stderr
    (lane / 'link.log').write_text(log)
    unresolved = sorted(set(re.findall(r'LNK2001: unresolved external symbol (\S+)', log)))
    errors = re.findall(r'(?:fatal )?error (LNK\d+)', log)
    if result.returncode and (not unresolved or any(e not in ('LNK2001', 'LNK1120') for e in errors)):
        raise RuntimeError('Unexpected link failure; inspect ' + str(lane / 'link.log'))
    if digest((ROOT / 'recovery.json').read_bytes()) != state_hash:
        raise ValueError('Recovery changed during diagnostic')
    for row, unit in zip(rows, plan['units']):
        if digest((ROOT / row['source']).read_bytes()) != row['source_sha256']:
            raise ValueError('Source changed after compilation')
        if 'receipt' in unit and declared_context_files(read(ROOT / unit['spec']), ROOT) != row['context_files']:
            raise ValueError('Header changed after compilation')
    report = dict(scope='Natural dependency diagnostic only; no aliases, stubs, placement tricks, original execution or acceptance.',
                  toolchain=toolchain, plan_sha256=digest(plan_path.read_bytes()),
                  recovery_sha256=state_hash, sources=rows, libraries=plan['libraries'],
                  link_command=command, link_exit_code=result.returncode,
                  unresolved=unresolved, unresolved_count=len(unresolved),
                  duplicate_source_definitions={k:v for k,v in definitions.items() if len(v) > 1},
                  log_sha256=digest(log.encode()),
                  limitations='Selected sources only; CRT variant, historical TU ownership and whole-image layout unproved.')
    path = lane / 'report.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(rows)} fresh objects; {len(unresolved)} unresolved symbols; report: {path}')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.plan, args.output)
