"""Strict natural-link acceptance: LINKED_MATCH (owner decision 2026-10-20).

A LINKED_MATCH function is raw byte-identical over its complete recorded extent at its original
RVA in a fresh, unresolved-free natural link of the module's canonical sources: the pinned VC5 RTM
compiler, linker and libraries, the reviewed link plan, and nothing else (no /FORCE, stubs,
aliases, placement controls or patching). FUNCTION_MATCH (scripts/match.py, scripts/promote.py)
is unchanged; this is a separate status with its own proof.

build MODULE           fresh compile of every plan source, natural link, refuse on any unresolved
                       symbol, duplicate definition or linker warning; writes work/linked/<module>/
                       (image, map, log, objects, report.json).
candidates MODULE      rows not yet strict whose linked RVA equals the original RVA and whose linked
                       bytes equal the original over the full row extent.
promote MODULE ID...   rebuild fresh, re-check, record LINKED_MATCH with its proof.
verify [MODULE]        rebuild and re-check every LINKED_MATCH row.

Link plan (calibration/<module stem>/link_plan.json): module, target, target_sha256, toolchain,
output_name, objects [{source, flags, role, evidence}] in command-line order, libraries
[{name, sha256}] from the pinned toolchain lib directory, link_flags (allowlisted).
Linked equality says nothing about module layout beyond the recorded extent, source identity,
link order of the original or whole-image equality.
"""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from pe import PE

STATE = ROOT / 'recovery.json'
STATUS = 'LINKED_MATCH'
STRICT = {'FUNCTION_MATCH', STATUS}
SCOPE = ('complete raw function extent at its original RVA in a fresh unresolved-free natural link of the '
         'canonical sources under the reviewed link plan')
MASKED_SCOPE = 'relocation-masked diagnostic equality with consistent implied symbol map; not acceptance'
SOURCE_DIRS = ('calibration', 'src')
LINK_FLAGS = {'/DLL', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:REF', '/OPT:NOREF', '/OPT:ICF', '/OPT:NOICF'}
EXPORT_FLAG = re.compile(r'^/EXPORT:[A-Za-z_@?$][\w@?$]*(=[A-Za-z_@?$][\w@?$]*)?$')
COMPILE_FLAG = re.compile(r'^/(O[12xdsgtiby-]+|Og-|Ob[012]|Oy-|G[3456]|Gs|Gf|Gy|Gz|Gd|Gr|M[TDL]d?|TP|TC|W[0-4]|Za|Ze|J|'
                          r'D[A-Za-z_]\w*(=\w*)?)$')
PRAGMA = re.compile(rb'^\s*#\s*pragma\s+(\w+)', re.M)
PRAGMA_ALLOWED = {b'once', b'pack', b'warning'}
FORBIDDEN_TOKENS = re.compile(rb'\b(__asm|_asm|__emit|_emit)\b|__declspec\s*\(\s*(naked|allocate|dllexport|selectany)')
QUOTED_INCLUDE = re.compile(rb'^\s*#\s*include\s*"([^"\r\n]+)"')
SECTIONS = {'.text', '.data', '.bss', '.rdata', '.drectve', '.CRT$XCU'}
DIRECTIVE = re.compile(r'^-defaultlib:"?[\w.]+"?$', re.I)
UNRESOLVED = re.compile(r'(LNK2001|LNK2019|LNK1120)')
DUPLICATE = re.compile(r'(LNK2005|LNK4006|LNK1169|LNK4088)')
WARNING = re.compile(r'warning LNK\d+')
MAP_LINE = re.compile(r'^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+(f\s+)?(i\s+)?(\S+)?\s*$')


class LinkRefused(ValueError):
    pass


def jdigest(value):
    return digest(json.dumps(value, sort_keys=True, separators=(',', ':')).encode('utf-8'))


def plan_path(module):
    return ROOT / 'calibration' / Path(module).stem.lower() / 'link_plan.json'


def _relative_inside(root, value, dirs):
    p = Path(value)
    if p.is_absolute() or '..' in p.parts or '\\' in value:
        raise LinkRefused(f'Plan path must be repository-relative POSIX: {value}')
    full = (root / p).resolve()
    if not any(full.is_relative_to((root / d).resolve()) for d in dirs):
        raise LinkRefused(f'Plan source outside {dirs}: {value}')
    return full


def load_plan(module, path=None):
    path = Path(path) if path else plan_path(module)
    raw = path.read_bytes()
    plan = json.loads(raw)
    if plan.get('module', '').upper() != module.upper():
        raise LinkRefused(f'Plan {path} is for {plan.get("module")}, not {module}')
    return plan, raw


def validate_plan(plan, root=None):
    root = root or ROOT
    for key in ('module', 'target', 'target_sha256', 'toolchain', 'output_name', 'objects', 'libraries', 'link_flags'):
        if key not in plan:
            raise LinkRefused(f'Plan lacks {key}')
    if plan['toolchain'] != verify_toolchain(DEFAULT_TOOLCHAIN):
        raise LinkRefused('Plan toolchain is not the pinned toolchain')
    if not re.fullmatch(r'[\w.-]+\.(dll|exe)', plan['output_name'], re.I):
        raise LinkRefused('Output name must be a plain file name')
    seen = set()
    for o in plan['objects']:
        src = o['source']
        if src in seen:
            raise LinkRefused(f'Duplicate plan object: {src}')
        seen.add(src)
        full = _relative_inside(root, src, SOURCE_DIRS)
        if full.suffix.lower() not in ('.c', '.cpp') or not full.is_file():
            raise LinkRefused(f'Plan object must be an existing .c/.cpp source: {src}')
        for f in o.get('flags', []):
            if not COMPILE_FLAG.match(f):
                raise LinkRefused(f'Compiler flag not allowed: {src}: {f}')
    for f in plan['link_flags']:
        if f.upper() not in LINK_FLAGS and not EXPORT_FLAG.match(f):
            raise LinkRefused(f'Link flag not allowed: {f}')
    if not plan['objects']:
        raise LinkRefused('Plan has no objects')
    for lib in plan['libraries']:
        if not re.fullmatch(r'[\w]+\.lib', lib['name'], re.I) or not re.fullmatch(r'[0-9a-f]{64}', lib.get('sha256', '')):
            raise LinkRefused(f'Library must be a pinned toolchain library name with sha256: {lib}')


def repository_headers(source, root):
    """Transitive literal-quoted repository headers (same rule as promote.quoted_repository_headers)."""
    include_dirs = [(DEFAULT_TOOLCHAIN / 'include').resolve()]
    pending, visited, headers = [Path(source).resolve()], set(), {}
    while pending:
        current = pending.pop()
        if current in visited or not current.is_file():
            continue
        visited.add(current)
        for line in current.read_bytes().splitlines():
            m = QUOTED_INCLUDE.match(line)
            if not m:
                continue
            name = os.fsdecode(m.group(1))
            found = next((c.resolve() for c in [current.parent / name, *(d / name for d in include_dirs)]
                          if c.is_file()), None)
            if found is None or not found.is_relative_to(root.resolve()):
                continue
            if found not in headers:
                headers[found] = found.read_bytes()
                pending.append(found)
    return headers


def scan_source(path, data):
    """Refuse placement/steering constructs in repository text (sources and quoted headers)."""
    for m in PRAGMA.finditer(data):
        if m.group(1) not in PRAGMA_ALLOWED:
            raise LinkRefused(f'#pragma {m.group(1).decode()} is not allowed in a natural link: {path}')
    m = FORBIDDEN_TOKENS.search(data)
    if m:
        raise LinkRefused(f'Forbidden construct {m.group(0).decode()!r}: {path}')


def snapshot(plan, root, extra=()):
    """Bytes of every input that decides the link: sources, quoted headers, libraries, state."""
    files = {}
    for o in plan['objects']:
        src = root / o['source']
        files[src.resolve()] = src.read_bytes()
        files.update(repository_headers(src, root))
    for p in extra:
        files[Path(p).resolve()] = Path(p).read_bytes()
    for p, data in files.items():
        if p.suffix.lower() in ('.c', '.cpp', '.h', '.hpp', '.inc'):
            scan_source(p, data)
    return files


def assert_unchanged(files, phase):
    for p, before in files.items():
        if not p.is_file() or p.read_bytes() != before:
            raise LinkRefused(f'Concurrent edit during {phase}: {p}')


def source_hashes(files, root):
    out = {}
    for p, data in files.items():
        if p.suffix.lower() in ('.c', '.cpp', '.h', '.hpp', '.inc'):
            out[p.relative_to(root.resolve()).as_posix()] = digest(data)
    return dict(sorted(out.items()))


def check_object(obj, source):
    coff = COFF(obj)
    for s in coff.sections:
        name = s['name']
        if name not in SECTIONS and not name.startswith('.debug$'):
            raise LinkRefused(f'{source}: section {name} is not a natural compiler contribution')
        if name == '.drectve' and s['size']:
            for token in s['data'].decode('latin-1').replace('\0', ' ').split():
                if not DIRECTIVE.match(token):
                    raise LinkRefused(f'{source}: linker directive {token} is not allowed')
    return coff


def libraries(plan):
    out = []
    for lib in plan['libraries']:
        path = DEFAULT_TOOLCHAIN / 'lib' / lib['name']
        if not path.is_file():
            raise LinkRefused(f'Library missing from the pinned toolchain: {lib["name"]}')
        data = path.read_bytes()
        if digest(data) != lib['sha256']:
            raise LinkRefused(f'Library identity changed: {lib["name"]}')
        out.append((path, data))
    return out


def read_map(path):
    base, symbols, dupes = None, {}, set()
    for line in path.read_text(errors='ignore').splitlines():
        m = re.match(r'\s*Preferred load address is ([0-9a-fA-F]+)', line)
        if m:
            base = int(m.group(1), 16)
            continue
        m = MAP_LINE.match(line)
        if m:
            name = m.group(3)
            entry = dict(section=int(m.group(1), 16), address=int(m.group(4), 16), object=m.group(7) or '')
            if name in symbols and symbols[name] != entry:
                dupes.add(name)
            symbols[name] = entry
    for name in dupes:  # ambiguous (e.g. same-named statics): never a placement witness
        symbols.pop(name)
    return base, symbols


def normalized_image(data):
    """Image bytes with the link timestamps and checksum zeroed (reproducibility digest only)."""
    import pefile
    p = pefile.PE(data=bytes(data), fast_load=False)
    buf = bytearray(data)
    off = p.DOS_HEADER.e_lfanew
    buf[off + 8:off + 12] = b'\0' * 4  # FILE_HEADER.TimeDateStamp
    buf[off + 24 + 64:off + 24 + 68] = b'\0' * 4  # OPTIONAL_HEADER.CheckSum
    if hasattr(p, 'DIRECTORY_ENTRY_EXPORT'):
        e = p.get_offset_from_rva(p.OPTIONAL_HEADER.DATA_DIRECTORY[0].VirtualAddress)
        buf[e + 4:e + 8] = b'\0' * 4
    return bytes(buf)


def file_backed_text(pe, rva, size):
    return any(s['characteristics'] & 0x20000000 and s['virtual_address'] <= rva and
               rva + size <= s['virtual_address'] + min(s['virtual_size'], s['raw_size']) for s in pe.sections)


def module_rows(state, module):
    return [r for r in state['functions'] if Path(r['target']).name.upper() == module.upper()]


def build(module, plan_file=None, out=None, original_root=None, quiet=False):
    """Fresh compile + natural link. Returns the report; raises LinkRefused on any gate failure."""
    plan_file = Path(plan_file) if plan_file else plan_path(module)
    plan, plan_raw = load_plan(module, plan_file)
    validate_plan(plan)
    target = (Path(original_root) if original_root else ROOT) / plan['target']
    target_bytes = target.read_bytes()
    if digest(target_bytes) != plan['target_sha256']:
        raise LinkRefused(f'Target hash mismatch: {target}')
    files = snapshot(plan, ROOT, extra=[plan_file, STATE])
    libs = libraries(plan)
    out = Path(out) if out else ROOT / 'work/linked' / Path(module).stem.lower()
    if out.exists():
        shutil.rmtree(out)
    (out / 'objects').mkdir(parents=True)
    objects, listing = [], []
    for i, o in enumerate(plan['objects']):
        src = ROOT / o['source']
        before = src.read_bytes()
        obj, command = compile_source(src, list(o.get('flags') or ['/O2']))
        if src.read_bytes() != before:
            raise LinkRefused(f'Source changed during compilation: {o["source"]}')
        check_object(obj, o['source'])
        dst = out / 'objects' / f'{i:02d}_{src.stem}.obj'
        shutil.copyfile(obj, dst)
        objects.append(dst)
        listing.append(dict(index=i, source=o['source'], flags=list(o.get('flags') or ['/O2']),
                            object=dst.name, source_sha256=digest(before), object_sha256=digest(dst.read_bytes())))
    env = {k: v for k, v in os.environ.items() if k.upper() not in ('CL', '_CL_', 'LINK', '_LINK_', 'LIB', 'INCLUDE')}
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    env['LIB'] = str(DEFAULT_TOOLCHAIN / 'lib')
    image, mapfile = out / plan['output_name'], out / 'link.map'
    command = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/nologo', *plan['link_flags'],
               f'/OUT:{image}', f'/MAP:{mapfile}', *map(str, objects), *(str(p) for p, _ in libs)]
    r = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=600)
    log = r.stdout + r.stderr
    (out / 'link.log').write_text(log)
    problems = dict(unresolved=sorted(set(l.strip() for l in log.splitlines() if UNRESOLVED.search(l))),
                    duplicates=sorted(set(l.strip() for l in log.splitlines() if DUPLICATE.search(l))),
                    warnings=sorted(set(l.strip() for l in log.splitlines() if WARNING.search(l))))
    if r.returncode or any(problems.values()) or not image.is_file() or not mapfile.is_file():
        (out / 'report.json').write_text(json.dumps(dict(refused=True, exit=r.returncode, **problems), indent=1))
        raise LinkRefused(f'Natural link refused (exit {r.returncode}): ' +
                          '; '.join(f'{k}: {len(v)}' for k, v in problems.items() if v) + f'; see {out / "link.log"}')
    for p, data in libs:
        if p.read_bytes() != data:
            raise LinkRefused(f'Library changed during link: {p}')
    assert_unchanged(files, 'build')
    if digest(target.read_bytes()) != plan['target_sha256']:
        raise LinkRefused('Target changed during build')
    state = json.loads(files[STATE.resolve()])
    image_bytes = image.read_bytes()
    image_pe = PE(image)
    base, symbols = read_map(mapfile)
    if base != image_pe.image_base:
        raise LinkRefused('Map load address differs from the image base')
    by_object = {o['object']: o['source'] for o in listing}
    recorded = module_rows(state, module)
    original = PE(target) if recorded else None
    rows = [check_row(row, plan, symbols, by_object, image_pe, original) for row in recorded]
    report = dict(scope='Natural-link build for LINKED_MATCH; not whole-module equality.',
                  module=module, plan=plan_file.relative_to(ROOT).as_posix() if plan_file.is_relative_to(ROOT) else str(plan_file),
                  link_plan_sha256=digest(plan_raw), toolchain=plan['toolchain'],
                  target=plan['target'], target_sha256=plan['target_sha256'],
                  command=[Path(command[0]).name, *command[1:]], objects=listing,
                  libraries=[dict(name=Path(p).name, sha256=digest(d)) for p, d in libs],
                  sources=source_hashes(files, ROOT), sources_sha256=jdigest(source_hashes(files, ROOT)),
                  state_sha256=digest(files[STATE.resolve()]),
                  image=image.relative_to(ROOT).as_posix() if image.is_relative_to(ROOT) else str(image),
                  image_sha256=digest(image_bytes), image_normalized_sha256=digest(normalized_image(image_bytes)),
                  map_sha256=digest(mapfile.read_bytes()), rows=rows,
                  summary=dict(rows=len(rows), placed=sum(r['placed'] for r in rows),
                               linked_equal=sum(r['equal'] for r in rows),
                               linked_equal_bytes=sum(r['size'] for r in rows if r['equal']),
                               candidates=sum(r['equal'] and r['status'] not in STRICT for r in rows)))
    (out / 'report.json').write_text(json.dumps(report, indent=1) + '\n')
    if not quiet:
        s = report['summary']
        print(f"{module}: linked {len(listing)} objects; rows {s['rows']}, placed {s['placed']}, "
              f"raw equal {s['linked_equal']} ({s['linked_equal_bytes']} bytes), candidates {s['candidates']}")
    return report


def check_row(row, plan, symbols, by_object, linked, original):
    rva, size = int(row['rva'], 16), row['size']
    res = dict(id=row['id'], rva=row['rva'], size=size, symbol=row['symbol'], status=row['status'],
               source=row.get('source'), linked_rva=None, placed=False, equal=False, reason=None)
    plan_flags = {o['source']: list(o.get('flags') or ['/O2']) for o in plan['objects']}
    sym = symbols.get(row['symbol'])
    if row.get('source') not in plan_flags:
        res['reason'] = 'row source is not a plan object'
    elif list(row.get('flags') or ['/O2']) != plan_flags[row['source']]:
        res['reason'] = 'row flags differ from the plan flags of its source'
    elif sym is None:
        res['reason'] = 'symbol absent or ambiguous in the map'
    elif by_object.get(sym['object']) != row['source']:
        res['reason'] = f"symbol defined by {sym['object']}, not the row source"
    else:
        linked_rva = sym['address'] - linked.image_base
        res['linked_rva'] = hex(linked_rva)
        res['placed'] = linked_rva == rva
        if not res['placed']:
            res['reason'] = 'not at the original RVA'
        elif not (file_backed_text(linked, rva, size) and file_backed_text(original, rva, size)):
            res['reason'] = 'extent not wholly inside file-backed code of both images'
        else:
            a, b = linked.read_rva(rva, size), original.read_rva(rva, size)
            res['equal'] = a == b
            res['function_bytes_sha256'] = digest(a)
            if not res['equal']:
                res['differing_bytes'] = sum(x != y for x, y in zip(a, b))
                res['reason'] = 'bytes differ'
    return res


def candidates(module, **kw):
    report = build(module, quiet=True, **kw)
    found = [r for r in report['rows'] if r['equal'] and r['status'] not in STRICT]
    for r in found:
        print(f"{r['id']} {r['rva']} {r['size']} {r['status']}")
    print(f'{len(found)} LINKED_MATCH candidates ({sum(r["size"] for r in found)} bytes) of {len(report["rows"])} rows')
    return found


@contextmanager
def lock():
    """Same lock file as scripts/promote.py: promotions of either kind serialize."""
    path = ROOT / 'work/promotion.lock'
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a+b') as stream:
        stream.seek(0, 2)
        if stream.tell() == 0:
            stream.write(b'0')
            stream.flush()
        stream.seek(0)
        if os.name == 'nt':
            import msvcrt
            msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield
        finally:
            stream.seek(0)
            if os.name == 'nt':
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def write_state(state):
    temporary = STATE.with_name('recovery.json.linked-tmp')
    temporary.write_bytes((json.dumps(state, indent=2) + '\n').encode('utf-8'))
    for attempt in range(20):
        try:
            os.replace(temporary, STATE)
            return
        except OSError:
            # Windows refuses the replace while another process holds the file open for reading.
            if attempt == 19:
                raise
            time.sleep(0.5)


def proof(row_result, report):
    return dict(scope=SCOPE, link_plan=report['plan'], link_plan_sha256=report['link_plan_sha256'],
                source_sha256=report['sources'][row_result['source']], sources_sha256=report['sources_sha256'],
                image_sha256=report['image_sha256'], image_normalized_sha256=report['image_normalized_sha256'],
                function_bytes_sha256=row_result['function_bytes_sha256'], toolchain=report['toolchain'],
                libraries={l['name']: l['sha256'] for l in report['libraries']})


def promote(module, ids, **kw):
    if (ROOT / 'work/promotion-journal.json').exists():
        raise LinkRefused('Interrupted promotion: run scripts/promote.py --recover first')
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    rows = {r['id']: r for r in module_rows(state, module)}
    missing = [i for i in ids if i not in rows]
    if missing:
        raise LinkRefused(f'Unknown {module} rows: {missing}')
    if any(rows[i]['status'] == 'FUNCTION_MATCH' for i in ids):
        raise LinkRefused('FUNCTION_MATCH rows keep their standalone proof; LINKED_MATCH is not a replacement')
    report = build(module, quiet=True, **kw)
    if report['state_sha256'] != digest(state_before):
        raise LinkRefused('recovery.json changed before the build')
    results = {r['id']: r for r in report['rows']}
    bad = [i for i in ids if not results[i]['equal']]
    if bad:
        raise LinkRefused('Not linked-equal: ' + ', '.join(f"{i} ({results[i]['reason']})" for i in bad))
    regressed = [r['id'] for r in report['rows'] if r['status'] == STATUS and not r['equal']]
    if regressed:
        raise LinkRefused(f'Existing LINKED_MATCH rows regressed: {regressed}')
    promoted = {}
    for i in ids:
        row = rows[i]
        row['status'] = STATUS
        row['proof'] = proof(results[i], report)
        promoted.setdefault(row['source'], set()).add(row['symbol'])
    # Masked siblings pin every other function their source defines (scripts/similar.py verify): a
    # promoted symbol becomes co-defined unless a remaining masked row (e.g. an ENG1/ENG3 twin of the
    # same source) still checks it.
    masked = [r for r in state['functions'] if r['status'] == 'CODEGEN_SIMILAR' and
              r.get('proof', {}).get('scope') == MASKED_SCOPE and r.get('source') in promoted]
    for r in masked:
        if r['proof']['source_sha256'] != report['sources'][r['source']]:
            raise LinkRefused(f"{r['id']}: masked proof is stale; restage {r['source']} first")
    for source, symbols in promoted.items():
        siblings = [r for r in masked if r['source'] == source]
        remaining = {r['symbol'] for r in siblings}
        for r in siblings:
            co = (set(r['proof'].get('co_defined_functions', [])) | symbols) - remaining
            r['proof']['co_defined_functions'] = sorted(co)
    if STATE.read_bytes() != state_before:
        raise LinkRefused('recovery.json changed during promotion')
    write_state(state)
    print(f'{STATUS}: ' + ', '.join(ids))
    return report


def verify(module=None, strict_provenance=False, **kw):
    state = json.loads(STATE.read_text())
    modules = sorted({Path(r['target']).name.upper() for r in state['functions'] if r['status'] == STATUS})
    if module:
        modules = [m for m in modules if m == module.upper()]
    failures, drift, count = [], [], 0
    for m in modules:
        try:
            report = build(m, quiet=True, **kw)
        except LinkRefused as error:
            failures.append(f'{m}: {error}')
            continue
        results = {r['id']: r for r in report['rows']}
        for row in module_rows(state, m):
            if row['status'] != STATUS:
                continue
            count += 1
            res, p = results[row['id']], row['proof']
            if not res['equal']:
                failures.append(f"{row['id']}: {res['reason']}")
            elif res['function_bytes_sha256'] != p['function_bytes_sha256']:
                failures.append(f"{row['id']}: function bytes differ from the proof")
            elif report['sources'].get(row['source']) != p['source_sha256']:
                failures.append(f"{row['id']}: source differs from the proof; review and promote again")
            for key in ('link_plan_sha256', 'sources_sha256', 'image_normalized_sha256'):
                if p.get(key) != report[key]:
                    drift.append(f"{row['id']}: {key}")
    print(f'{count} {STATUS} rows verified in {len(modules)} modules; {len(failures)} failures; '
          f'{len(drift)} provenance drifts')
    for f in failures:
        print('  ' + f)
    for d in drift[:20]:
        print('  drift ' + d)
    if failures or (strict_provenance and drift):
        raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--original-root', type=Path,
                        help='Read hash-pinned originals from this checkout; sources, plan and state stay local')
    sub = parser.add_subparsers(dest='command', required=True)
    for name in ('build', 'candidates'):
        s = sub.add_parser(name)
        s.add_argument('module')
        s.add_argument('--out', type=Path)
    s = sub.add_parser('promote')
    s.add_argument('module')
    s.add_argument('ids', nargs='+')
    s = sub.add_parser('verify')
    s.add_argument('module', nargs='?')
    s.add_argument('--strict-provenance', action='store_true')
    args = parser.parse_args()
    kw = dict(original_root=args.original_root)
    try:
        if args.command == 'build':
            build(args.module, out=args.out, **kw)
        elif args.command == 'candidates':
            candidates(args.module, out=args.out, **kw)
        elif args.command == 'promote':
            with lock():
                promote(args.module, args.ids, **kw)
        else:
            verify(args.module, args.strict_provenance, **kw)
    except LinkRefused as error:
        raise SystemExit(f'refused: {error}')


if __name__ == '__main__':
    main()
