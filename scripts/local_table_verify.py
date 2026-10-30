"""Compatibility CLI for local-table reference checks through the normal gate.

No ledger writes or acceptance; all builds and comparisons use match.verify_rows.
"""
import argparse
import json
from pathlib import Path

from match import ROOT, verify_rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bindings', required=True, type=Path)
    parser.add_argument('--original-root', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    state_bytes = (ROOT / 'recovery.json').read_bytes()
    document = json.loads(args.bindings.read_bytes())
    path = args.bindings.resolve().relative_to(ROOT).as_posix()
    ids = set(document['functions'])
    rows = [r for r in json.loads(state_bytes)['functions'] if r['id'] in ids]
    if {r['id'] for r in rows} != ids:
        raise ValueError('Unknown binding IDs')
    results, statistics = verify_rows(rows, original_root=args.original_root,
                                     relocation_candidates={r['id']: path for r in rows})
    if (ROOT / 'recovery.json').read_bytes() != state_bytes:
        raise ValueError('Recovery changed during opt-in verification')
    report = args.report.resolve()
    if not report.is_relative_to(ROOT / 'work'):
        raise ValueError('Reports belong under work/')
    report.write_text(json.dumps(dict(results=results, statistics=statistics), indent=2) + '\n')
    print(json.dumps(dict(exact=sum(r['exact'] for r in results), **statistics)))
    if not all(r['exact'] for r in results):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
