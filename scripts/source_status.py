"""Locate current source and report per-function status from recovery.json."""
import argparse
from collections import Counter
import json
from pathlib import Path

from env import ROOT

ACCEPTED = {'FUNCTION_MATCH', 'FUNCTION_MATCH_RAW', 'FUNCTION_MATCH_RELOCATED', 'LINKED_MATCH'}


def inventory():
    rows = json.loads((ROOT / 'recovery.json').read_text())['functions']
    result = {}
    for path in sorted((ROOT / 'src').rglob('*')):
        if path.is_file() and path.suffix in {'.c', '.cpp', '.h', '.hpp', '.inc', '.def'}:
            source = path.relative_to(ROOT).as_posix()
            members = [r for r in rows if r['source'] == source]
            result[source] = dict(source=source, recorded_rows=len(members),
                                  accepted_rows=sum(r['status'] in ACCEPTED for r in members),
                                  statuses=dict(Counter(r['status'] for r in members)),
                                  functions=[dict(id=r['id'], status=r['status'], target=r['target'],
                                                  rva=r['rva'], size=r['size'], symbol=r['symbol'])
                                             for r in members])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('query', nargs='?', help='Module, source path, filename, or function ID')
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args()
    units = inventory()
    if args.query:
        query = args.query.replace('\\', '/')
        review = ROOT / 'evidence/source_layout_2026_10_25/review.json'
        if review.exists():
            query = json.loads(review.read_text())['moves'].get(query, query)
        module = {'hercules.exe': 'pc', 'title.dll': 'title', 'eng1.dll': 'eng1', 'eng3.dll': 'eng3'}.get(query.lower(), query.lower())
        selected = [v for p, v in units.items() if query.lower() in p.lower()
                    or p.startswith('src/' + module + '/')
                    or any(query.lower() in f['id'].lower() for f in v['functions'])]
        if not selected:
            parser.error('No current source or function matches ' + args.query)
        if args.json:
            print(json.dumps(selected, indent=2))
        else:
            for unit in selected:
                print(unit['source'])
                if not unit['functions']:
                    print('  No function rows; inspect its data/header/build role.')
                for f in unit['functions']:
                    print(f"  {f['status']:26} {f['id']} ({Path(f['target']).name} {f['rva']}, {f['size']} bytes)")
    else:
        summary = []
        for module in ['pc', 'title', 'eng1', 'eng3', 'shared']:
            selected = [v for p, v in units.items() if p.startswith('src/' + module + '/')]
            summary.append(dict(module=module, source_files=len(selected),
                                recorded_rows=sum(v['recorded_rows'] for v in selected),
                                accepted_rows=sum(v['accepted_rows'] for v in selected)))
        if args.json:
            print(json.dumps(summary, indent=2))
        else:
            print('Module  Source files  Accepted / recorded function rows')
            for v in summary:
                print(f"{v['module']:7} {v['source_files']:12}  {v['accepted_rows']:8} / {v['recorded_rows']}")
            print('Counts come from recovery.json; a directory or filename confers no proof status.')


if __name__ == '__main__':
    main()
