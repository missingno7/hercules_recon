"""Census of existing-but-unrecorded source: compile every mapped function and masked-compare it.

Diagnostic only (never acceptance). Run: python scripts/census.py; output work/census/census.json.
ENG1 behavioral build configuration, skips rows already in recovery.json, compiles each source
once with the pinned toolchain, and masked-compares the compiled /Gy contribution at the mapped RVA.
"""
import glob
import json
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parent))
from env import ROOT  # noqa: E402
from coff import COFF  # noqa: E402
from masked import masked_compare  # noqa: E402
from match import compile_source  # noqa: E402

MODULES = {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL', 'ENG1.DLL': 'assets/pc/HERCULES/ENG1.DLL',
           'ENG3.DLL': 'assets/pc/HERCULES/ENG3.DLL', 'HERCULES.EXE': 'work/discs/pc_install/HERCULES.EXE'}


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
    config = json.load(open(ROOT / 'evidence/eng1_behavioral_build.json'))
    for row in config['functions']:
        found.add(('ENG1.DLL', int(row['rva'], 16), row['symbol'], row['source']))
    return found


def main():
    state = json.load(open(ROOT / 'recovery.json'))
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
    (ROOT / 'work/census').mkdir(parents=True, exist_ok=True)
    (ROOT / 'work/census/census.json').write_text(json.dumps(results, indent=1))
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
