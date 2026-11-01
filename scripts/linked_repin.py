"""Explicit reviewed source-only re-pin through complete link-plan object identity.

Only IMAGE_FILE_HEADER.TimeDateStamp (bytes [4:8]) is zeroed on both sides.
Every other byte, including metadata, directives and auxiliary records, must agree.
The fresh natural link is the normal linked.verify acceptance check, plus unchanged
plan/toolchain/libraries and unchanged sources outside the reviewed replacements.
"""
import copy
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time
import uuid

import linked
from env import ROOT

OBJECT_IDENTITY = 'coff-complete-bytes-zero-timestamp-v1'


def normalized_object(data):
    """Plain i386 COFF bytes with exactly its four timestamp bytes zeroed.

    Preserve length and every byte outside [4:8], including uninterpreted bytes.
    This does not serialize the parsed COFF inventory or normalize PE images.
    """
    if len(data) < 20:
        raise ValueError('Expected a complete plain i386 COFF object header')
    machine, _, _, _, _, optional_size, _ = struct.unpack_from('<HHIIIHH', data)
    if machine != 0x14c or optional_size:
        raise ValueError('Expected a plain i386 COFF object')
    return data[:4] + b'\0' * 4 + data[8:]


def equal(a, b, label):
    if a != b:
        raise ValueError('Linked preservation failed: ' + label)


def unique_paths(names, label):
    """Reject aliases before any path-string keyed discovery or object inventory."""
    seen = {}
    for name in names:
        path = (ROOT / name).resolve()
        key = str(path).casefold()
        if key in seen:
            raise ValueError('Duplicate canonical paths in ' + label + ': ' + seen[key] + ', ' + name)
        seen[key] = name


def tree_files(source_root):
    source_root = Path(source_root).resolve()
    if not source_root.is_dir():
        raise ValueError('Missing dependency root: ' + str(source_root))
    files = set()
    for path in source_root.rglob('*'):
        resolved = path.resolve()
        if resolved != Path(os.path.abspath(path)):
            raise ValueError('Aliased dependency tree path is unverifiable: ' + str(path))
        if not path.is_file():
            continue
        if not resolved.is_relative_to(source_root):
            raise ValueError('Dependency root escape: ' + str(path))
        files.add(resolved)
    return files


def tree_inputs(source_root):
    """First-read hashes, with membership frozen as well as contents."""
    return {p: linked.digest(p.read_bytes()) for p in tree_files(source_root)}


def repository_inputs():
    return tree_inputs(ROOT / 'src')


def allowed_dependency(path):
    path = Path(path).resolve()
    if not any(path.is_relative_to(root.resolve()) for root in (ROOT / 'src', linked.DEFAULT_TOOLCHAIN)):
        raise ValueError('Compiler dependency outside src/ and pinned toolchain: ' + str(path))
    return path


MARKER = re.compile(rb'^[ \t]*#(?:line[ \t]+|[ \t]+)\d+[ \t]+"([^"\r\n]+)"[ \t]*$')


def compiler_dependencies(source, flags):
    """Pinned VC5 /E, exact TU profile; actual #line dependencies, never a cache.

    The repository's host_config/host_frame_precreate probes use this VC5 trace
    format. Snapshotting and publication use repin's shared frozen-input guard.
    """
    for flag in flags:
        if not linked.COMPILE_FLAG.fullmatch(flag):
            raise ValueError('Compiler flag not allowed: ' + flag)
    toolchain = linked.DEFAULT_TOOLCHAIN
    toolchain_id = linked.verify_toolchain(toolchain)
    parent = ROOT / 'build/match'
    parent.mkdir(parents=True, exist_ok=True)
    cwd = Path(tempfile.mkdtemp(prefix='dependencies-', dir=parent))
    command = [str(toolchain / 'bin/cl.exe'), '/nologo', '/E', '/Gy', *flags, str(Path(source).resolve())]
    env = {k: v for k, v in os.environ.items() if k.upper() not in ('CL', '_CL_', 'INCLUDE', 'LIB')}
    env['PATH'] = str(toolchain / 'bin') + os.pathsep + env.get('PATH', '')
    env['INCLUDE'], env['LIB'] = str(toolchain / 'include'), str(toolchain / 'lib')
    record = dict(kind='preprocess', command=command, source_path=str(Path(source).resolve()),
                  source_sha256=linked.digest(Path(source).read_bytes()), flags=list(flags), toolchain_id=toolchain_id)
    started = time.monotonic()
    try:
        result = subprocess.run(command, cwd=cwd, env=env, capture_output=True, timeout=90)
        record.update(returncode=result.returncode, stdout_sha256=linked.digest(result.stdout),
                      stderr_sha256=linked.digest(result.stderr))
        if result.returncode:
            raise ValueError('Pinned compiler dependency scan failed: ' + str(source))
        paths = set()
        for line in result.stdout.splitlines():
            marker = MARKER.fullmatch(line)
            if marker:
                spelling = marker[1].decode('ascii').replace('\\\\', '\\')
                path = Path(spelling)
                if not path.is_absolute():
                    raise ValueError('Relative compiler dependency marker: ' + spelling)
                path = allowed_dependency(path)
                if not path.is_file():
                    raise ValueError('Missing compiler dependency: ' + str(path))
                paths.add(path)
            elif re.match(rb'^[ \t]*#[ \t]*line\b', line):
                raise ValueError('Unparsed compiler dependency marker')
        if Path(source).resolve() not in paths:
            raise ValueError('Compiler dependency scan omitted its source: ' + str(source))
        record['inputs'] = sorted(map(str, paths))
        return paths, record
    except BaseException as error:
        record['error'] = str(error)
        raise
    finally:
        record['elapsed_seconds'] = time.monotonic() - started
        (cwd / 'dependencies.json').write_text(json.dumps(record, indent=2) + '\n')
        metrics = os.environ.get('HERCULES_COMPILE_METRICS_DIR')
        if metrics:
            repo_root = Path(__file__).resolve().parents[1]
            requested = Path(metrics)
            directory = (requested if requested.is_absolute() else repo_root / requested).resolve()
            if not any(directory.is_relative_to((repo_root / n).resolve()) for n in ('work', 'build')):
                raise ValueError('Compile metrics must be under work/ or build/')
            directory.mkdir(parents=True, exist_ok=True)
            (directory / (uuid.uuid4().hex + '.json')).write_text(json.dumps(record) + '\n')


TOKEN = re.compile(rb'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\r\n]*')
DIRECTIVE = re.compile(rb'^[ \t]*#[ \t]*([A-Za-z_][A-Za-z_0-9]*)(?:[ \t]+(.*?))?[ \t]*$')
QUOTED_INCLUDE = re.compile(rb'^[ \t]*#[ \t]*include[ \t]*"([^"\r\n]+)"[ \t]*$')
IDENTIFIER = re.compile(rb'[A-Za-z_][A-Za-z_0-9]*')
INTEGER = rb'(?:0[xX][0-9A-Fa-f]+|0[0-7]*|[1-9][0-9]*)(?:[uU][lL]?|[lL][uU]?)?'
DEFINE = re.compile(rb'[A-Za-z_][A-Za-z_0-9]*(?:[ \t]+' + INTEGER + rb')?')
LINE_SPLICE = re.compile(rb'\\(?:[\n\r\v\f\x85]|\xe2\x80[\xa8\xa9])')


def classify_source(data):
    """Classify raw bytes BEFORE looking for dependencies; LF is the only plain newline.

    Parse complete physical directive lines, not a list of interesting directive
    prefixes. Unknown syntax is opaque even when it does not spell 'include'.
    Header guards and integer object macros are parsed without evaluating them;
    discovery follows every literal branch, so condition evaluation is unnecessary.
    """
    opaque, includes = [], []
    if b'\r' in data:
        opaque.append(dict(reason='CR byte (unsupported newline convention)'))
    if LINE_SPLICE.search(data):
        opaque.append(dict(reason='backslash-newline line splice'))
    if b'??' in data or b'%:' in data:
        opaque.append(dict(reason='trigraph or alternate directive spelling'))
    if any((c < 32 and c not in (9, 10, 13)) or c >= 127 for c in data):
        opaque.append(dict(reason='unexamined source encoding or control byte'))

    def uncomment(match):
        token = match.group()
        if token.startswith((b'/*', b'//')):
            # A comment spanning physical lines may splice a directive even
            # without a backslash: '# /* ... LF ... */ include "header.h"'.
            if b'\n' in token and b'#' in data[:match.start()].rsplit(b'\n', 1)[-1]:
                opaque.append(dict(reason='multiline comment inside possible directive'))
            return bytes(c if c in (10, 13) else 32 for c in token)
        if b'\n' in token:
            opaque.append(dict(reason='multiline quoted token'))
        return token

    text = TOKEN.sub(uncomment, data)
    # splitlines() would silently reinterpret CR and other line separators.
    for number, line in enumerate(text.split(b'\n'), 1):
        if not line.lstrip(b' \t').startswith(b'#'):
            continue
        form = dict(line=number, form=line.decode('latin-1'))
        literal = QUOTED_INCLUDE.fullmatch(line)
        if literal:
            includes.append((literal[1].decode('latin-1'), form))
            continue
        if re.fullmatch(rb'[ \t]*#[ \t]*', line):  # null directive
            continue
        directive = DIRECTIVE.fullmatch(line)
        if directive:
            name, body = directive[1], (directive[2] or b'').strip(b' \t')
            if name in (b'ifdef', b'ifndef', b'undef') and IDENTIFIER.fullmatch(body):
                continue
            if name in (b'else', b'endif') and not body:
                continue
            if name == b'define' and DEFINE.fullmatch(body):
                continue
            if name == b'pragma' and body == b'once':
                continue
        opaque.append(dict(reason='unparsed preprocessor directive', **form))
    return includes, opaque


def include_inputs(plan):
    """Literal repository closure plus an explicit conservative uncertainty flag.

    This is not a C preprocessor. Every unexamined include (including system
    headers), line splice, alternate directive spelling or encoding is opaque.
    With any opaque form, ANY canonical change could affect this module.
    """
    pending = [(ROOT / o['source']).resolve() for o in plan['objects']]
    files, opaque = {}, []
    while pending:
        path = pending.pop()
        if path in files:
            continue
        data = path.read_bytes()
        files[path] = data
        includes, findings = classify_source(data)
        opaque.extend(dict(path=path.relative_to(ROOT).as_posix(), **finding) for finding in findings)
        for name, form in includes:
            allowed_dependency(path.parent / name)
            found = next((p.resolve() for p in (path.parent / name, linked.DEFAULT_TOOLCHAIN / 'include' / name)
                          if p.is_file()), None)
            if found is not None:
                allowed_dependency(found)
            if found is not None and found.is_relative_to((ROOT / 'src').resolve()):
                pending.append(found)
            else:
                opaque.append(dict(path=path.relative_to(ROOT).as_posix(),
                                   reason='unresolved or unexamined include', **form))
    return files, opaque


class Gate:
    def __init__(self):
        self.guard = {}
        self.modules = {}
        self.repository = {}
        self.toolchain = {}
        self.inputs = {}
        self.enabled = False

    def capture(self, files):
        from repin import merge_frozen_inputs
        merge_frozen_inputs(self.inputs, {p: linked.digest(data) for p, data in files.items()})

    def dependencies(self, plan, candidates=None):
        from repin import assert_frozen_inputs
        receipts = []
        for item in plan['objects']:
            paths, record = compiler_dependencies(ROOT / item['source'], list(item.get('flags') or ['/O2']))
            for path in paths:
                path = allowed_dependency(path)
                if path not in self.inputs:
                    raise ValueError('Compiler dependency absent from initial frozen inventory: ' + str(path))
            assert_frozen_inputs({p: self.inputs[p] for p in paths}, candidates)
            record['compile_inputs'] = {str(p): (linked.digest(candidates[p]) if candidates and p in candidates else self.inputs[p])
                                        for p in paths}
            receipts.append(record)
        return receipts

    def assert_after(self, candidates):
        if not self.enabled:
            return
        from repin import assert_frozen_inputs
        equal(tree_files(ROOT / 'src'), set(self.repository), 'repository input file inventory')
        equal(tree_files(linked.DEFAULT_TOOLCHAIN), set(self.toolchain), 'toolchain input file inventory')
        assert_frozen_inputs(self.inputs, candidates)

    def finish(self, state, candidates, original_root):
        """Called inside repin's recoverable transaction; raises before publication."""
        receipts = []
        self.assert_after(candidates)
        for module, before in self.modules.items():
            linked.verify_toolchain.cache_clear()
            dependencies_after = self.dependencies(before['plan'], candidates)
            self.assert_after(candidates)
            report = linked.build(module, out=before['out'] / 'after',
                                  original_root=original_root, quiet=True)
            equal(report['link_plan_sha256'], before['plan_sha256'], 'link plan')
            equal(report['toolchain'], before['toolchain'], 'toolchain')
            equal({l['name']: l['sha256'] for l in report['libraries']}, before['libraries'], 'libraries')
            sources = linked.source_hashes(linked.snapshot(before['plan'], ROOT), ROOT)
            equal(report['sources'], sources, 'fresh source inventory')
            expected = dict(before['sources'])
            for name in before['changed']:
                if name in expected:
                    expected[name] = linked.digest(candidates[ROOT / name])
            equal(sources, expected, 'only declared sources changed')
            objects = {o['source']: o for o in report['objects']}
            unique_paths([o['source'] for o in report['objects']], 'fresh linked objects')
            equal(set(objects), set(before['objects']), 'complete link-plan object inventory')
            checked = []
            for name, old in before['objects'].items():
                obj = before['out'] / 'after' / 'objects' / objects[name]['object']
                data = obj.read_bytes()
                normalized_before, normalized_after = normalized_object(old), normalized_object(data)
                equal(normalized_after, normalized_before, 'complete object bytes except timestamp ' + module + ':' + name)
                checked.append(dict(source=name, flags=objects[name]['flags'], size=len(data),
                                    before_sha256=linked.digest(old), after_sha256=linked.digest(data),
                                    before_normalized_sha256=linked.digest(normalized_before),
                                    after_normalized_sha256=linked.digest(normalized_after)))
            results = {r['id']: r for r in report['rows']}
            current = {r['id']: r for r in linked.module_rows(state, module) if r['status'] == linked.STATUS}
            equal(set(current), set(before['proofs']), 'linked ID inventory')
            # Equivalent to linked.verify's failure checks, using this fresh build.
            # Existing provenance drifts stay historical; never replace image pins.
            for rid, row in current.items():
                result, proof = results[rid], row['proof']
                equal(proof, before['proofs'][rid], 'linked proof changed before gate ' + rid)
                equal(result['equal'], True, 'fresh linked equality ' + rid)
                equal(result['function_bytes_sha256'], proof['function_bytes_sha256'], 'function proof ' + rid)
                proof['source_sha256'] = sources[row['source']]
                proof['sources_sha256'] = report['sources_sha256']
            receipts.append(dict(module=module, identity=OBJECT_IDENTITY,
                                 objects=checked, sources_before=before['sources'], sources_after=sources,
                                 opaque_includes=before['opaque'],
                                 dependencies_before=before['dependencies'], dependencies_after=dependencies_after,
                                 proofs_before=before['proofs'], report=report))
        self.assert_after(candidates)
        return receipts


def prepare(state, plan, directory, original_root):
    """Discover ALL linked modules using any changed input, then require exact opt-in.

    Opaque include forms require explicit coverage of every changed input. Every
    opted-in module's complete plan object inventory is compiled and preserved.
    """
    gate = Gate()
    changed = set(plan['sources'])
    unique_paths(plan['sources'], 'plan sources')
    for name in changed:
        equal((ROOT / name).resolve().relative_to(ROOT).as_posix(), name, 'canonical changed source path')
    declared = plan.get('linked_object_unchanged', {})
    if not isinstance(declared, dict):
        raise ValueError('linked_object_unchanged must map changed sources to module lists')
    for name, modules in declared.items():
        if (name not in changed or not isinstance(modules, list) or not modules or
                any(not isinstance(m, str) for m in modules) or len(set(modules)) != len(modules)):
            raise ValueError('linked_object_unchanged must name changed sources and distinct modules')
    required, discovered = {}, {}
    modules = sorted({Path(r['target']).name.upper() for r in state['functions'] if r['status'] == linked.STATUS})
    if modules:
        from repin import merge_frozen_inputs
        gate.enabled = True
        gate.repository, gate.toolchain = repository_inputs(), tree_inputs(linked.DEFAULT_TOOLCHAIN)
        merge_frozen_inputs(gate.inputs, gate.repository)
        merge_frozen_inputs(gate.inputs, gate.toolchain)
        gate.capture({ROOT / 'toolchains/manifest.json': (ROOT / 'toolchains/manifest.json').read_bytes()})
        for name, item in plan['sources'].items():
            if 'before_sha256' in item:
                equal(gate.repository[(ROOT / name).resolve()], item['before_sha256'], 'source base ' + name)
    for module in modules:
        path = linked.plan_path(module)
        link_plan, raw = linked.load_plan(module, path)
        gate.capture({path: raw})
        unique_paths([o['source'] for o in link_plan['objects']], module + ' link plan')
        linked.validate_plan(link_plan)
        closure, opaque = include_inputs(link_plan)
        gate.capture(closure)
        files = linked.snapshot(link_plan, ROOT)
        gate.capture(files)
        sources = linked.source_hashes(files, ROOT)
        files.update(closure)
        dependencies = gate.dependencies(link_plan)
        actual = {Path(p).relative_to(ROOT).as_posix() for r in dependencies for p in r['compile_inputs']
                  if Path(p).is_relative_to(ROOT / 'src')}
        touched = changed if opaque else changed & ({p.relative_to(ROOT).as_posix() for p in closure} | actual)
        # A malformed plan must never hide an accepted changed source from discovery.
        direct = {r['source'] for r in linked.module_rows(state, module)
                  if r['status'] == linked.STATUS} & changed
        equal(direct - set(sources), set(), 'linked row source absent from plan')
        if not touched:
            continue
        for name in touched:
            required.setdefault(name, []).append(module)
        discovered[module] = (path, link_plan, raw, files, sources, touched, opaque, dependencies)
    equal({n: sorted(ms) for n, ms in declared.items()}, required,
          'explicit linked_object_unchanged source/module opt-in required (all affected modules)')
    for module, (path, link_plan, raw, files, sources, touched, opaque, dependencies) in discovered.items():
        linked.verify_toolchain.cache_clear()
        linked.validate_plan(link_plan)
        objects = {o['source']: o for o in link_plan['objects']}
        libs = linked.libraries(link_plan)
        lib_hashes = {p.name: linked.digest(data) for p, data in libs}
        target = (Path(original_root) if original_root else ROOT) / link_plan['target']
        target_bytes = target.read_bytes()
        equal(linked.digest(target_bytes), link_plan['target_sha256'], 'original target')
        proofs = {}
        for row in linked.module_rows(state, module):
            if row['status'] != linked.STATUS:
                continue
            proof = row['proof']
            equal(proof['scope'], linked.SCOPE, 'linked scope ' + row['id'])
            equal(proof['source_sha256'], sources[row['source']], 'before source pin ' + row['id'])
            equal(proof['link_plan'], path.relative_to(ROOT).as_posix(), 'proof link plan')
            equal(proof['link_plan_sha256'], linked.digest(raw), 'proof link plan pin')
            equal(proof['toolchain'], link_plan['toolchain'], 'proof toolchain')
            equal(proof['libraries'], lib_hashes, 'proof libraries')
            proofs[row['id']] = copy.deepcopy(proof)
        files[path.resolve()] = raw
        files[target.resolve()] = target_bytes
        files[(ROOT / 'toolchains/manifest.json').resolve()] = (ROOT / 'toolchains/manifest.json').read_bytes()
        files.update({p.resolve(): data for p, data in libs})
        gate.capture(files)
        out = Path(directory) / 'linked_object_unchanged' / Path(module).stem.lower()
        out.mkdir(parents=True, exist_ok=True)
        old_objects = {}
        for index, item in enumerate(link_plan['objects']):
            name = item['source']
            obj, _ = linked.compile_source(ROOT / name, list(objects[name].get('flags') or ['/O2']))
            linked.check_object(obj, name)
            old_objects[name] = obj.read_bytes()
            (out / (str(index).zfill(2) + '_' + Path(name).name + '.before.obj')).write_bytes(old_objects[name])
        linked.assert_unchanged(files, 'linked re-pin before compilation')
        gate.modules[module] = dict(plan=link_plan, plan_sha256=linked.digest(raw), sources=sources,
                                    changed=touched, objects=old_objects, proofs=proofs, out=out,
                                    toolchain=link_plan['toolchain'], libraries=lib_hashes, opaque=opaque,
                                    dependencies=dependencies)
    gate.assert_after({})
    gate.guard = {p: h for p, h in gate.inputs.items() if p not in {ROOT / n for n in changed}}
    return gate
