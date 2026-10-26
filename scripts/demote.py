"""Reviewed withdrawal of FUNCTION_MATCH_RELOCATED acceptances (status correction only).

A relocated row whose independent reference identities a review found missing,
ambiguous, circular or contradicted returns to its own prior masked proof
(CODEGEN_SIMILAR). Nothing is deleted: the withdrawn proof, its binding path
and hash, the reason and the pinned review are kept in the row's
`withdrawn_acceptance` history, so STRICT can be restored later with
`promote.py --relocated <id> --bindings <withdrawn bindings_path>` once the
missing evidence exists. No verifier rule, binding document or other row's
proof changes; the co-defined inventories of masked rows in the affected units
are recomputed from a fresh compile because the demoted symbol becomes masked.

Gates (all fresh, before any write): every remaining accepted RAW/RELOCATED row
verifies exactly, the LINKED check passes, and every masked row of each affected
unit is masked-equal with its recorded masked identity and symbol map.

    python scripts/demote.py <ids...> --review evidence/<review>.json [--verify-only] [--report work/...]

The review is a repository evidence document whose `rows` map each ID to a
record with `verdict: "DEMOTE"` and a non-empty `missing_link`.
"""
import argparse
import base64
import copy
import datetime
import json
from pathlib import Path

from env import ROOT
from match import RELOCATED, accepted_rows, compile_source, digest, verify_rows
from promote import JOURNAL, STATE, atomic_bytes, lock, recover
from similar import SCOPE, check_rows, defined_functions


def withdraw(state, ids, review_path, review_bytes, date):
    """Pure ledger transformation; returns the affected (source, flags) groups."""
    review = json.loads(review_bytes)
    records = review.get('rows')
    if not isinstance(records, dict):
        raise ValueError('Review must map row IDs under "rows"')
    wanted = list(ids)
    if not wanted or len(set(wanted)) != len(wanted):
        raise ValueError('Provide unique recovery IDs')
    rows = {r['id']: r for r in state['functions']}
    groups = set()
    for fid in wanted:
        row = rows.get(fid)
        if row is None or row['status'] != RELOCATED:
            raise ValueError('Only accepted FUNCTION_MATCH_RELOCATED rows can be withdrawn: ' + fid)
        prior = row.get('prior_masked_proof')
        if not isinstance(prior, dict) or prior.get('scope') != SCOPE or not prior.get('masked_sha256'):
            raise ValueError('Withdrawal needs the row\'s own prior masked proof: ' + fid)
        if prior.get('source_sha256') != row['proof']['source_sha256']:
            raise ValueError('Prior masked proof was made from another source: ' + fid)
        record = records.get(fid)
        if not isinstance(record, dict) or record.get('verdict') != 'DEMOTE' or not record.get('missing_link'):
            raise ValueError('Review has no DEMOTE verdict with a missing link for ' + fid)
        history = list(row.get('withdrawn_acceptance', []))
        history.append(dict(status=row['status'], proof=row['proof'], withdrawn_on=date,
                            review_path=review_path, review_sha256=digest(review_bytes),
                            missing_link=record['missing_link'],
                            restore=f"promote.py --relocated {fid} --bindings {row['proof'].get('bindings_path')}"))
        row['withdrawn_acceptance'] = history
        row['status'] = 'CODEGEN_SIMILAR'
        row['proof'] = copy.deepcopy(prior)
        del row['prior_masked_proof']
        groups.add((row['source'], tuple(row['flags'])))
    return groups


def masked_group(state, source, flags):
    return [r for r in state['functions'] if r['status'] == 'CODEGEN_SIMILAR' and r.get('source') == source
            and tuple(r.get('flags') or ['/O2']) == flags and r.get('proof', {}).get('scope') == SCOPE]


def demote(ids, review, verify_only=False, report=None, date=None):
    if JOURNAL.exists():
        raise ValueError('Interrupted promotion: run scripts/promote.py --recover first')
    review_path = Path(review).resolve()
    if not review_path.is_relative_to((ROOT / 'evidence').resolve()):
        raise ValueError('The withdrawal review must be repository evidence')
    review_rel = review_path.relative_to(ROOT.resolve()).as_posix()
    review_bytes = review_path.read_bytes()
    before = STATE.read_bytes()
    state = json.loads(before)
    groups = withdraw(state, ids, review_rel, review_bytes, date or datetime.date.today().isoformat())
    # Fresh gate 1: every remaining acceptance, unchanged.
    remaining = accepted_rows(state['functions'])
    results, statistics = verify_rows(remaining)
    if not all(r['exact'] for r in results):
        raise ValueError('Withdrawal rejected: a remaining accepted row does not verify')
    if any(r['status'] == 'LINKED_MATCH' for r in state['functions']):
        from linked import verify
        verify()
    # Fresh gate 2: the affected units' masked inventories, with recomputed co-defined sets.
    masked = {}
    for source, flags in sorted(groups):
        members = masked_group(state, source, flags)
        obj, _ = compile_source(ROOT / source, list(flags))
        co_defined = sorted(defined_functions(obj) - {r['symbol'] for r in members})
        for row in members:
            row['proof']['co_defined_functions'] = co_defined
        raw = [r for r in state['functions'] if r.get('source') == source and r['status'] == 'FUNCTION_MATCH'
               and tuple(r.get('flags') or ['/O2']) == flags]
        checked = check_rows(members + raw, source, set(co_defined) - {r['symbol'] for r in raw})
        for row in members:
            result = checked[row['id']]
            if result['masked_sha256'] != row['proof']['masked_sha256'] or result['symbols'] != row['link_symbols']:
                raise ValueError('Masked identity or symbol map changed: ' + row['id'])
        masked[source] = dict(rows=len(members), co_defined=len(co_defined))
    receipt = dict(withdrawn=sorted(ids), review=review_rel, review_sha256=digest(review_bytes),
                   remaining_verified=len(results), statistics=statistics, masked_units=masked,
                   state_before_sha256=digest(before), verify_only=verify_only)
    after = (json.dumps(state, indent=2) + '\n').encode()
    receipt['state_after_sha256'] = digest(after)
    if report:
        report = Path(report).resolve()
        if not any(report.is_relative_to(ROOT / d) for d in ('work', 'build')):
            raise ValueError('Reports belong under work/ or build/')
        report.parent.mkdir(parents=True, exist_ok=True)
        report.write_text(json.dumps(receipt, indent=2) + '\n')
    if STATE.read_bytes() != before:
        raise ValueError('Recovery state changed during withdrawal')
    if not verify_only:
        journal = [dict(path='recovery.json', before=base64.b64encode(before).decode(),
                        before_sha256=digest(before), after_sha256=digest(after))]
        atomic_bytes(JOURNAL, json.dumps(journal).encode())
        try:
            atomic_bytes(STATE, after)
        except BaseException:
            recover()
            raise
        JOURNAL.unlink()
    print(f"{'Verified' if verify_only else 'Withdrew'} {len(ids)} acceptances; "
          f"{len(results)} remaining accepted rows verified; {len(masked)} masked units rechecked")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('ids', nargs='+')
    parser.add_argument('--review', required=True)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    with lock():
        demote(args.ids, args.review, args.verify_only, args.report)


if __name__ == '__main__':
    main()
