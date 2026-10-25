"""Census source-backed code contributions without changing recovery state.

Default mode surveys existing-but-unrecorded source. ``--recorded`` compiles
every distinct source/ordered-flags profile in recovery.json once and diagnoses
all recorded extents. Neither mode is an acceptance gate.
"""
import argparse
import glob
import json
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parent))
from env import ROOT  # noqa: E402
from coff import COFF  # noqa: E402
from masked import _compare_section, masked_compare  # noqa: E402
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain  # noqa: E402
from pe import PE  # noqa: E402

MODULES = {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL', 'ENG1.DLL': 'assets/pc/HERCULES/ENG1.DLL',
           'ENG3.DLL': 'assets/pc/HERCULES/ENG3.DLL', 'HERCULES.EXE': 'work/discs/pc_install/HERCULES.EXE'}
RECORDED_CATEGORIES = ('raw_equal', 'reference_only', 'nonreference_code_difference',
                       'extent_difference', 'unsupported')


def module_of(target):
    name = Path(target).name.upper()
    return name if name in MODULES else None


def harvest():
    found = set()
    for path in glob.glob(str(ROOT / 'evidence/*.json')):
        try:
            data = json.load(open(path, encoding='utf-8'))
        except Exception:
            continue
        stack = [(data, None, None)]
        while stack:
            node, target, source = stack.pop()
            if isinstance(node, dict):
                target = node.get('target', target) if isinstance(node.get('target'), str) else target
                source = node.get('source', source) if isinstance(node.get('source'), str) else source
                sym, rva = node.get('symbol'), node.get('rva')
                if isinstance(sym, str) and isinstance(rva, str) and target and source:
                    m = module_of(target)
                    if m and source.endswith(('.c', '.cpp')):
                        try:
                            found.add((m, int(rva, 16), sym, source))
                        except ValueError:
                            pass
                if isinstance(node.get('symbol_rvas'), dict) and target and source:
                    m = module_of(target)
                    if m and source.endswith(('.c', '.cpp')):
                        for s, r in node['symbol_rvas'].items():
                            try:
                                found.add((m, int(r, 16), s, source))
                            except (ValueError, TypeError):
                                pass
                for v in node.values():
                    stack.append((v, target, source))
            elif isinstance(node, list):
                for v in node:
                    stack.append((v, target, source))
    config = json.load(open(ROOT / 'config/eng1/sources.json'))
    for row in config['functions']:
        found.add(('ENG1.DLL', int(row['rva'], 16), row['symbol'], row['source']))
    return found


def recorded_fields(row, coff, section, target):
    """Classify complete-row differences using the existing relocation comparator."""
    rva, target_size = int(row['rva'], 16), row['size']
    actual = section['data']
    if target_size <= 0 or not any(
            s['characteristics'] & 0x20000000 and
            s['virtual_address'] <= rva and
            rva + target_size <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])
            for s in target.sections):
        return dict(category='unsupported', reason='recorded target extent is not file-backed executable code',
                    compiled_size=len(actual), target_size=target_size, relocations=len(section['relocations']))
    try:
        expected = target.read_rva(rva, target_size)
        number = coff.sections.index(section) + 1
        comparison = _compare_section(coff, section, number, target, rva, target_size)
    except Exception as error:
        return dict(category='unsupported', reason='target read failed: ' + str(error)[:200],
                    compiled_size=len(actual), target_size=target_size, relocations=len(section['relocations']))
    extent_difference = len(actual) != target_size
    target_relocations = [entry for block in getattr(target.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                          for entry in block.entries if entry.type and
                          entry.rva < rva + target_size and entry.rva + 4 > rva]
    raw_equal = (not extent_difference and actual == expected and not section['relocations']
                 and not target_relocations)
    if comparison['unsupported']:
        category = 'unsupported'
    elif extent_difference:
        category = 'extent_difference'
    elif raw_equal:
        category = 'raw_equal'
    elif comparison['differing_bytes'] == 0:
        category = 'reference_only'
    else:
        category = 'nonreference_code_difference'
    reason = ('unsupported COFF relocations' if comparison['unsupported'] else None)
    return dict(category=category, reason=reason, compiled_size=comparison['compiled_size'],
                target_size=comparison['target_size'], raw_equal=raw_equal,
                masked_equal=comparison['masked_equal'], differing_bytes=comparison['differing_bytes'],
                conflicts=len(comparison['conflicts']), unsupported_relocations=len(comparison['unsupported']),
                relocation_count=comparison['relocations'])


def output_path(value, default):
    path = Path(value) if value else default
    path = path if path.is_absolute() else ROOT / path
    resolved_work = (ROOT / 'work').resolve()
    resolved = path.resolve()
    if not resolved.is_relative_to(resolved_work):
        raise ValueError('Census output must stay under ignored work/')
    resolved.parent.mkdir(parents=True, exist_ok=True)
    return resolved


def recorded_census(state, state_sha256):
    """Freshly compile every recorded source/ordered-flags profile once."""
    expected_targets = defaultdict(set)
    groups = defaultdict(list)
    for row in state['functions']:
        groups[(row['source'], tuple(row['flags']))].append(row)
        expected_targets[(ROOT / row['target']).resolve()].add(row['target_sha256'])

    target_images, target_load_errors = {}, {}
    for path, hashes in expected_targets.items():
        try:
            if len(hashes) != 1:
                raise ValueError('conflicting target hashes in recovery rows')
            actual_hash = digest(path.read_bytes())
            expected_hash = next(iter(hashes))
            if actual_hash != expected_hash:
                raise ValueError('target hash differs from recovery.json')
            target_images[path] = PE(path)
        except Exception as error:
            target_load_errors[path] = str(error)[:200]

    def check_targets(members):
        errors = {}
        by_path = defaultdict(set)
        for row in members:
            by_path[(ROOT / row['target']).resolve()].add(row['target_sha256'])
        for path, hashes in by_path.items():
            if len(hashes) != 1:
                errors[str(path)] = 'conflicting target hashes in source profile'
                continue
            expected = next(iter(hashes))
            try:
                actual = digest(path.read_bytes())
            except Exception as error:
                errors[str(path)] = 'target unavailable: ' + str(error)[:160]
                continue
            if actual != expected:
                errors[str(path)] = 'target hash changed or differs from recovery.json'
        return errors

    def unsupported_row(row, reason):
        return dict(id=row['id'], module=module_of(row['target']), rva=row['rva'], size=row['size'],
                    symbol=row['symbol'], source=row['source'], flags=row['flags'],
                    target_sha256=row['target_sha256'], status=row['status'],
                    category='unsupported', reason=reason)

    def hash_after_group(source, members):
        try:
            source_after = digest(source.read_bytes())
            source_error = None
        except Exception as error:
            source_after = None
            source_error = str(error)[:180]
        return source_after, source_error, check_targets(members)

    def work(item):
        (source_text, flags), members = item
        source = (ROOT / source_text).resolve()
        proof_hashes = sorted({r.get('proof', {}).get('source_sha256')
                               for r in members if r.get('proof', {}).get('source_sha256')})
        expected_target_hashes = {}
        for row in members:
            expected_target_hashes[str((ROOT / row['target']).resolve())] = row['target_sha256']
        group = dict(source=source_text, flags=list(flags), rows=len(members),
                     recovery_source_sha256=proof_hashes, target_sha256=expected_target_hashes)
        records = []
        try:
            source_before = digest(source.read_bytes())
        except Exception as error:
            group.update(status='unsupported', error='source unavailable: ' + str(error)[:180])
            group['target_hashes_before'] = check_targets(members)
            group['target_hashes_after'] = check_targets(members)
            return group, [unsupported_row(r, 'source unavailable') for r in members]
        group['source_sha256'] = source_before
        group['matches_recorded_source_sha256'] = not proof_hashes or proof_hashes == [source_before]
        target_errors_before = check_targets(members)
        group['target_hashes_before'] = target_errors_before
        try:
            obj, _ = compile_source(source, list(flags), DEFAULT_TOOLCHAIN)
            coff = COFF(obj)
        except Exception as error:
            group.update(status='unsupported', error='compile/COFF failure: ' + str(error)[:180])
            source_after, source_error, target_errors_after = hash_after_group(source, members)
            group['source_sha256_after'] = source_after
            group['source_hash_after_error'] = source_error
            group['target_hashes_after'] = target_errors_after
            return group, [unsupported_row(r, group['error']) for r in members]

        for row in members:
            module = module_of(row['target'])
            target_path = (ROOT / row['target']).resolve()
            if module is None:
                records.append(unsupported_row(row, 'unrecognized target module'))
                continue
            if target_path in target_load_errors:
                records.append(unsupported_row(row, target_load_errors[target_path]))
                continue
            try:
                section = coff.function(row['symbol'])
                comparison = recorded_fields(row, coff, section, target_images[target_path])
                records.append(dict(id=row['id'], module=module, rva=row['rva'], size=row['size'],
                                    symbol=row['symbol'], source=source_text, flags=list(flags),
                                    target_sha256=row['target_sha256'], status=row['status'], **comparison))
            except Exception as error:
                records.append(unsupported_row(row, str(error)[:200]))

        source_after, source_error, target_errors_after = hash_after_group(source, members)
        group['source_sha256_after'] = source_after
        if source_error:
            group['source_hash_after_error'] = source_error
        group['target_hashes_after'] = target_errors_after
        changed = source_after != source_before or target_errors_before or target_errors_after
        if changed:
            reason = 'source or target hash guard failed during source-profile group'
            group.update(status='unsupported', error=reason)
            for record in records:
                record.update(category='unsupported', reason=reason)
        else:
            group['status'] = 'compiled'
        return group, records

    verify_toolchain(DEFAULT_TOOLCHAIN)
    ordered_groups = sorted(groups.items(), key=lambda item: (item[0][0], item[0][1]))
    group_results, results = [], []
    with ThreadPoolExecutor(8) as pool:
        futures = [pool.submit(work, item) for item in ordered_groups]
        for index, future in enumerate(as_completed(futures), 1):
            group, rows = future.result()
            group_results.append(group)
            results.extend(rows)
            if index % 25 == 0 or index == len(futures):
                print(f'recorded census: {index}/{len(futures)} source profiles', flush=True)

    if digest((ROOT / 'recovery.json').read_bytes()) != state_sha256:
        raise RuntimeError('recovery.json changed during census; refusing to write a mixed-state report')
    if len(results) != len(state['functions']):
        raise RuntimeError('recorded census did not classify every recovery row')
    group_results.sort(key=lambda r: (r['source'], tuple(r['flags'])))
    results.sort(key=lambda r: (r.get('module') or '', int(r['rva'], 16), r['id']))
    by_module = {}
    for module in MODULES:
        rows = [r for r in results if r.get('module') == module]
        categories = {}
        for category in RECORDED_CATEGORIES:
            selected = [r for r in rows if r.get('category') == category]
            categories[category] = dict(functions=len(selected), recorded_bytes=sum(r['size'] for r in selected),
                                        compiled_bytes=sum(r.get('compiled_size', 0) for r in selected))
        by_module[module] = dict(functions=len(rows), recorded_bytes=sum(r['size'] for r in rows),
                                 categories=categories,
                                 differing_bytes=sum(r.get('differing_bytes', 0) for r in rows),
                                 conflicts=sum(r.get('conflicts', 0) for r in rows),
                                 unsupported_relocations=sum(r.get('unsupported_relocations', 0) for r in rows))
    return dict(scope=('Fresh recorded-source COFF byte comparison; reference_only means all byte differences '
                       'fall within supported object relocation fields. No destination bindings or acceptance.'),
                recovery_sha256=state_sha256, toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
                compile_group_count=len(ordered_groups), compile_group_results=group_results,
                summary=dict(functions=len(results), recorded_bytes=sum(r['size'] for r in results),
                             categories={category: dict(functions=sum(r.get('category') == category for r in results),
                                                       recorded_bytes=sum(r['size'] for r in results
                                                                          if r.get('category') == category))
                                         for category in RECORDED_CATEGORIES},
                             by_module=by_module),
                rows=results)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recorded', action='store_true',
                        help='compile all recovery source/ordered-flags profiles and compare recorded extents')
    parser.add_argument('--output', help='JSON output under ignored work/')
    args = parser.parse_args()
    state_raw = (ROOT / 'recovery.json').read_bytes()
    state = json.loads(state_raw)
    state_sha256 = digest(state_raw)
    if args.recorded:
        report = recorded_census(state, state_sha256)
        path = output_path(args.output, ROOT / 'work/census/recorded.json')
        path.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
        print('recorded rows:', report['summary']['functions'], 'groups:', report['compile_group_count'])
        for module, summary in report['summary']['by_module'].items():
            print(module, summary['functions'], summary['recorded_bytes'], summary['categories'])
        print('report:', path)
        return

    recorded = {(module_of(r['target']), int(r['rva'], 16)) for r in state['functions']}
    found = [f for f in harvest() if (f[0], f[1]) not in recorded and (ROOT / f[3]).exists()]
    by_source = defaultdict(list)
    for f in found:
        by_source[f[3]].append(f)
    print(len(found), 'unrecorded mappings in', len(by_source), 'sources', flush=True)

    def work(source):
        try:
            obj, _ = compile_source(ROOT / source, ['/O2'])
        except Exception as e:
            return [dict(source=source, error=str(e)[:200])]
        coff = COFF(obj)
        out = []
        for module, rva, sym, _ in by_source[source]:
            try:
                section = coff.function(sym)
            except Exception:
                out.append(dict(module=module, rva=hex(rva), symbol=sym, source=source, error='symbol not defined'))
                continue
            size = len(section['data'])
            try:
                m = masked_compare(obj, sym, ROOT / MODULES[module], rva, size)
            except Exception as e:
                out.append(dict(module=module, rva=hex(rva), symbol=sym, source=source, error=str(e)[:200]))
                continue
            out.append(dict(module=module, rva=hex(rva), symbol=sym, source=source, size=size,
                            masked_equal=m['masked_equal'], differing=m['differing_bytes'],
                            conflicts=len(m['conflicts']), relocations=m['relocations']))
        return out

    results = []
    with ThreadPoolExecutor(8) as pool:
        for out in pool.map(work, sorted(by_source)):
            results.extend(out)
    path = output_path(args.output, ROOT / 'work/census/census.json')
    path.write_text(json.dumps(results, indent=1) + '\n', encoding='utf-8')
    ok = [r for r in results if r.get('masked_equal')]
    near = sorted([r for r in results if 'differing' in r and not r['masked_equal']], key=lambda r: r['differing'])
    print('masked equal unrecorded:', len(ok), sum(r['size'] for r in ok))
    for r in ok:
        print('  EQ', r['module'], r['rva'], r['size'], r['symbol'], r['source'])
    print('near (<=16 differing):', len([r for r in near if r['differing'] <= 16]))
    for r in near[:60]:
        print('  ', r['module'], r['rva'], r['size'], r['differing'], r['conflicts'], r['symbol'], r['source'])
    errors = [r for r in results if 'error' in r]
    print('errors:', len(errors))
    for r in errors[:30]:
        print('  ERR', r)


if __name__ == '__main__':
    main()
