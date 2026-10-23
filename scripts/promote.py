"""Serialize source promotion; freshly verify every accepted function before writing."""
import argparse
import base64
import copy
from contextlib import contextmanager
import json
import os
from pathlib import Path

from env import ROOT
from match import (check, digest, verify_rows, compile_inputs, context_proof,
                   unit_inventory, DEFAULT_TOOLCHAIN)
from coff import COFF

STATE = ROOT / "recovery.json"
JOURNAL = ROOT / "work/promotion-journal.json"


def atomic_bytes(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".promotion-tmp")
    temporary.write_bytes(content)
    os.replace(temporary, path)


@contextmanager
def lock():
    path = ROOT / "work/promotion.lock"
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as stream:
        stream.seek(0, 2)
        if stream.tell() == 0:
            stream.write(b"0")
            stream.flush()
        stream.seek(0)
        if os.name == "nt":
            import msvcrt
            msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield
        finally:
            stream.seek(0)
            if os.name == "nt":
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def recover():
    if not JOURNAL.exists():
        print("No interrupted promotion.")
        return
    journal = json.loads(JOURNAL.read_text())
    for row in journal:
        path = ROOT / row["path"]
        if path.resolve() != STATE.resolve() and not path.resolve().is_relative_to((ROOT / "src").resolve()):
            raise ValueError("Journal destination outside canonical source/state")
        current = digest(path.read_bytes()) if path.exists() else None
        if current not in {row["before_sha256"], row["after_sha256"]}:
            raise ValueError(f"Concurrent edit after interrupted promotion: {path}")
    for row in journal:
        path = ROOT / row["path"]
        if row["before"] is None:
            path.unlink(missing_ok=True)
        else:
            atomic_bytes(path, base64.b64decode(row["before"]))
    JOURNAL.unlink()
    print("Restored pre-promotion source and state.")


def validate_candidate_object(obj, allowed_symbols):
    coff = COFF(obj)
    functions = {s["name"] for s in coff.symbols.values() if s["section"] > 0 and s["type"] & 0x20}
    if functions != set(allowed_symbols):
        raise ValueError(f"Candidate functions must exactly cover accepted/requested rows: {sorted(functions)}")
    for name in functions:
        coff.function(name)  # require independently bounded /Gy code
    for section in coff.sections:
        if section["flags"] & (0x40 | 0x80) and section["size"]:
            if section["name"] not in {".drectve", ".debug$S", ".debug$T", ".debug$F"}:
                raise ValueError(f"Pilot gate does not accept unreviewed data contributions: {section['name']}")


def promote(ids, candidate, verify_only=False):
    if JOURNAL.exists():
        raise ValueError("Interrupted promotion: run scripts/promote.py --recover first")
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    wanted = set(ids)
    rows = [r for r in state["functions"] if r["id"] in wanted]
    if not rows or {r["id"] for r in rows} != wanted:
        raise ValueError("Every requested function must already have reviewed target context in recovery.json")
    sources = {r["source"] for r in rows}
    if len(sources) != 1:
        raise ValueError("One source file per promotion")
    destination = ROOT / next(iter(sources))
    if not destination.resolve().is_relative_to((ROOT / "src").resolve()):
        raise ValueError("Promotion destination must be under src/")
    content = candidate.read_bytes()
    if destination.suffix.lower() != ".c":
        raise ValueError("Initial promotion gate supports self-contained C translation units only")
    observed = {STATE: state_before}
    for row in state["functions"]:
        if row["status"] == "FUNCTION_MATCH" or row["id"] in wanted:
            path = ROOT / row["source"]
            observed[path] = path.read_bytes() if path.exists() else None
    allowed_symbols = {r["symbol"] for r in state["functions"] if r.get("source") in sources and
                       (r["status"] == "FUNCTION_MATCH" or r["id"] in wanted)}
    results = {}
    # Freeze candidate bytes for the compile and transaction to avoid a concurrent
    # scratch writer changing the candidate after verification.
    frozen = ROOT / "work/promotion-candidate.c"
    frozen.write_bytes(content)
    try:
        for row in state["functions"]:
            if row["status"] != "FUNCTION_MATCH" and row["id"] not in wanted:
                continue
            result = check(row, frozen if row.get("source") in sources else None)
            if not result["exact"]:
                raise ValueError(f"Promotion rejected: {row['id']} is not exact: {result}")
            results[row["id"]] = result
            if row.get("source") in sources:
                validate_candidate_object(result["object"], allowed_symbols)
        for path, before in observed.items():
            if (path.read_bytes() if path.exists() else None) != before:
                raise ValueError(f"Concurrent canonical edit during verification: {path}")
        print(f"Fresh byte verification passed for {len(results)} functions.")
        if verify_only:
            return
        for row in state["functions"]:
            if row["id"] in results:
                result = results[row["id"]]
                row["status"] = "FUNCTION_MATCH"
                row["proof"] = {k: result[k] for k in ("scope", "source_sha256", "compiled_sha256", "target_sha256", "toolchain")}
        state_content = (json.dumps(state, indent=2) + "\n").encode("utf-8")
        journal = []
        for path, after in ((destination, content), (STATE, state_content)):
            before = observed[path]
            journal.append({"path": path.relative_to(ROOT).as_posix(),
                            "before": base64.b64encode(before).decode("ascii") if before is not None else None,
                            "before_sha256": digest(before) if before is not None else None,
                            "after_sha256": digest(after)})
        atomic_bytes(JOURNAL, json.dumps(journal).encode("utf-8"))
        try:
            atomic_bytes(destination, content)
            atomic_bytes(STATE, state_content)
        except BaseException:
            recover()
            raise
        JOURNAL.unlink()
        print("Promoted: " + ", ".join(ids))
    finally:
        frozen.unlink(missing_ok=True)


def promote_existing_units(ids, verify_only=False, report=None):
    """Status-only adoption of raw functions in unchanged, fully inventoried units.

    No source is moved, extracted, rewritten or replaced. Other code/data remains
    at its existing proof level. The pilot source-replacement gate stays intact.
    """
    from similar import SCOPE, masked_identity
    from masked import masked_compare
    if JOURNAL.exists():
        raise ValueError('Interrupted promotion: recover it first')
    if report is not None and not any(report.resolve().is_relative_to(ROOT / d) for d in ('work', 'build')):
        raise ValueError('Promotion reports belong under work/ or build/')
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    wanted = set(ids)
    selected = [r for r in state['functions'] if r['id'] in wanted]
    if not wanted or len(selected) != len(wanted) or any(
            r['status'] != 'CODEGEN_SIMILAR' or r.get('proof', {}).get('scope') != SCOPE for r in selected):
        raise ValueError('Existing-unit adoption requires unique, reviewed masked rows')
    sources = {r['source'] for r in selected}
    peers = [r for r in state['functions'] if r.get('source') in sources]
    for row in peers:
        path = (ROOT / row['source']).resolve()
        if not any(path.is_relative_to(ROOT / d) for d in ('src', 'calibration')):
            raise ValueError('Existing units must be repository source')
        if row['status'] != 'FUNCTION_MATCH' and not (
                row['status'] == 'CODEGEN_SIMILAR' and row.get('proof', {}).get('scope') == SCOPE):
            raise ValueError('Every existing-unit peer needs a supported prior proof')
        if digest(path.read_bytes()) != row['proof']['source_sha256']:
            raise ValueError('Existing source differs from recorded proof: ' + row['id'])
    checked = [copy.deepcopy(r) for r in state['functions']
               if r['status'] == 'FUNCTION_MATCH' or r['id'] in wanted]
    for row in checked:
        row['status'] = 'FUNCTION_MATCH'  # a request to the raw gate, not state publication
    observed = {STATE: state_before, ROOT / 'toolchains/manifest.json': (ROOT / 'toolchains/manifest.json').read_bytes()}
    for row in checked + peers:
        observed[ROOT / row['target']] = (ROOT / row['target']).read_bytes()
        for path in compile_inputs((ROOT / row['source']).resolve(), row['flags'], DEFAULT_TOOLCHAIN):
            observed[path] = path.read_bytes()
    results, statistics = verify_rows(checked)
    if not all(r['exact'] for r in results):
        raise ValueError('Existing-unit promotion rejected by complete raw comparison')
    by_id = {r['id']: r for r in results}
    units = {}
    for source in sorted(sources):
        members = [r for r in peers if r['source'] == source]
        if len({tuple(r['flags']) for r in members}) != 1:
            raise ValueError('Unit peers disagree about compiler flags')
        result = by_id[next(r['id'] for r in members if r['id'] in wanted)]
        obj = Path(result['object'])
        observed[obj] = obj.read_bytes()
        coff = COFF(obj)
        defined = [s['name'] for s in coff.symbols.values() if s['section'] > 0 and s['type'] & 0x20]
        if len(defined) != len(members) or set(defined) != {r['symbol'] for r in members}:
            raise ValueError('Complete emitted function inventory differs from recorded unit')
        for row in members:
            coff.function(row['symbol'])  # full independently bounded COMDAT, never slices
            if row['status'] == 'CODEGEN_SIMILAR':
                measured = masked_compare(obj, row['symbol'], ROOT / row['target'], int(row['rva'], 0), row['size'])
                if (not measured['masked_equal'] or measured['symbols'] != row['link_symbols'] or
                        masked_identity(obj, row['symbol']) != row['proof']['masked_sha256']):
                    raise ValueError('Recorded masked peer regressed: ' + row['id'])
        context = compile_inputs((ROOT / source).resolve(), members[0]['flags'], DEFAULT_TOOLCHAIN)
        units[source] = dict(unit_inventory_sha256=unit_inventory(obj), context_sha256=context_proof(context),
                             functions=defined, sections=[dict(name=s['name'], size=s['size'], flags=s['flags'],
                             payload_sha256=digest(s['data']), relocation_count=len(s['relocations'])) for s in coff.sections],
                             scope='Inventory preservation only; non-selected code and data are not accepted')
    for path, before in observed.items():
        if path.read_bytes() != before:
            raise ValueError('Input/object changed during promotion: ' + str(path))
    for row in selected:
        result, unit = by_id[row['id']], units[row['source']]
        row['prior_masked_proof'] = row['proof']
        row['status'] = 'FUNCTION_MATCH'
        row['proof'] = {k: result[k] for k in ('scope', 'source_sha256', 'compiled_sha256', 'target_sha256', 'toolchain')}
        row['proof'].update({k: unit[k] for k in ('unit_inventory_sha256', 'context_sha256')})
    receipt = dict(scope='Fresh raw function acceptance in unchanged mixed units; no unit/data/layout acceptance',
                   promoted=sorted(wanted), state_before_sha256=digest(state_before),
                   statistics=statistics, results=results, units=units, verify_only=verify_only)
    if report is not None:
        report.parent.mkdir(parents=True, exist_ok=True)
        report.write_bytes((json.dumps(receipt, indent=2) + '\n').encode())
    if not verify_only:
        after = (json.dumps(state, indent=2) + '\n').encode()
        if STATE.read_bytes() != state_before:
            raise ValueError('Recovery state changed before publication')
        journal = [dict(path=STATE.relative_to(ROOT).as_posix(), before=base64.b64encode(state_before).decode(),
                        before_sha256=digest(state_before), after_sha256=digest(after))]
        atomic_bytes(JOURNAL, json.dumps(journal).encode())
        try:
            atomic_bytes(STATE, after)
        except BaseException:
            recover()
            raise
        JOURNAL.unlink()
    print(f"{'Verified' if verify_only else 'Promoted'} existing units: {', '.join(sorted(wanted))}; "
          f"{len(results)} raw rows, {len(peers)} unit peers preserved")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ids", nargs="*")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--recover", action="store_true")
    parser.add_argument('--existing-unit', action='store_true', help='accept raw functions in unchanged recorded units; status only')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    with lock():
        if args.recover:
            recover()
        elif args.existing_unit:
            if not args.ids or args.source is not None:
                parser.error('--existing-unit requires IDs and forbids --source')
            promote_existing_units(args.ids, args.verify_only, args.report)
        elif not args.ids or args.source is None:
            parser.error("provide function ids and --source, or --recover")
        else:
            promote(args.ids, args.source, args.verify_only)


if __name__ == "__main__":
    main()
