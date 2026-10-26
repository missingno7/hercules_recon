"""Record path-independent unit inventories for accepted RAW proofs (reviewed migration).

Existing-unit RAW proofs pin `unit_inventory_sha256`, which hashes the object's
COFF FILE records and so the absolute checkout path; a clean clone elsewhere
cannot reproduce it. Run in the checkout that recorded those proofs: each unit
is freshly compiled, every row's original `unit_inventory_sha256` and
`context_sha256` must reproduce exactly, and only then the same object's
`unit_inventory_portable_sha256` (match.portable_unit_inventory) is added.
The original hash stays in each proof; nothing else changes. match.py then
checks the portable hash wherever it is recorded.

    python scripts/portable_inventory.py [--verify-only] [--report work/...]
"""
import argparse
import base64
import json
from pathlib import Path

from env import ROOT
from match import (compile_inputs, compile_source, context_proof, digest, portable_unit_inventory,
                   unit_inventory, DEFAULT_TOOLCHAIN)
from promote import JOURNAL, STATE, atomic_bytes, lock, recover


def migrate(verify_only=False, report=None):
    if JOURNAL.exists():
        raise ValueError('Interrupted promotion: run scripts/promote.py --recover first')
    before = STATE.read_bytes()
    state = json.loads(before)
    rows = [r for r in state['functions'] if r['status'] == 'FUNCTION_MATCH'
            and 'unit_inventory_sha256' in r.get('proof', {}) and 'unit_inventory_portable_sha256' not in r['proof']]
    groups = {}
    for row in rows:
        groups.setdefault((row['source'], tuple(row['flags'])), []).append(row)
    receipt = {}
    for (source, flags), members in sorted(groups.items()):
        path = ROOT / source
        source_before = path.read_bytes()
        context = compile_inputs(path.resolve(), list(flags), DEFAULT_TOOLCHAIN)
        obj, _ = compile_source(path, list(flags))
        if path.read_bytes() != source_before:
            raise ValueError('Source changed during compilation: ' + source)
        original, portable = unit_inventory(obj), portable_unit_inventory(obj)
        for row in members:
            proof = row['proof']
            if digest(source_before) != proof['source_sha256'] or context_proof(context) != proof['context_sha256']:
                raise ValueError('Accepted unit source/context differs: ' + row['id'])
            if original != proof['unit_inventory_sha256']:
                raise ValueError('Original unit inventory does not reproduce here: ' + row['id'])
            proof['unit_inventory_portable_sha256'] = portable
        receipt[source] = dict(rows=[r['id'] for r in members], unit_inventory_sha256=original,
                               unit_inventory_portable_sha256=portable)
    after = (json.dumps(state, indent=2) + '\n').encode()
    if report:
        report = Path(report).resolve()
        if not any(report.is_relative_to(ROOT / d) for d in ('work', 'build')):
            raise ValueError('Reports belong under work/ or build/')
        report.parent.mkdir(parents=True, exist_ok=True)
        report.write_text(json.dumps(dict(units=receipt, state_before_sha256=digest(before),
                                          state_after_sha256=digest(after)), indent=2) + '\n')
    if STATE.read_bytes() != before:
        raise ValueError('Recovery state changed during migration')
    if not verify_only and rows:
        journal = [dict(path='recovery.json', before=base64.b64encode(before).decode(),
                        before_sha256=digest(before), after_sha256=digest(after))]
        atomic_bytes(JOURNAL, json.dumps(journal).encode())
        try:
            atomic_bytes(STATE, after)
        except BaseException:
            recover()
            raise
        JOURNAL.unlink()
    print(f"{'Verified' if verify_only else 'Recorded'} portable inventories for {len(rows)} rows in {len(groups)} units")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    with lock():
        migrate(args.verify_only, args.report)


if __name__ == '__main__':
    main()
