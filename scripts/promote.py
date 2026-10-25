"""Serialize source promotion; freshly verify every accepted function before writing."""
import argparse
import base64
import copy
from contextlib import contextmanager
import json
import os
from pathlib import Path
import re

from env import ROOT
from match import (check, digest, verify_rows, compile_inputs, context_proof,
                   unit_inventory, DEFAULT_TOOLCHAIN)
from coff import COFF

STATE = ROOT / "recovery.json"
JOURNAL = ROOT / "work/promotion-journal.json"
QUOTED_INCLUDE = re.compile(rb'^\s*#\s*include\s*"([^"\r\n]+)"')


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


def _include_directories(flags):
    result = []
    index = 0
    while index < len(flags):
        flag = str(flags[index])
        if flag.lower() == "/i" and index + 1 < len(flags):
            index += 1
            value = str(flags[index]).strip('"')
        elif flag[:2].lower() == "/i" and len(flag) > 2:
            value = flag[2:].strip('"')
        else:
            index += 1
            continue
        path = Path(value)
        if not path.is_absolute():
            path = ROOT / path
        result.append(path.resolve())
        index += 1
    result.append((DEFAULT_TOOLCHAIN / "include").resolve())
    return result


def quoted_repository_headers(source, flags):
    """Snapshot transitive literal-quoted headers resolved in ordinary order.

    Angle-bracket SDK/MFC headers are pinned by the existing toolchain manifest.
    This conservative scanner reads quoted directives regardless of conditional
    compilation and records only resolved files inside this repository.
    """
    repository = ROOT.resolve()
    include_dirs = _include_directories(flags)
    pending = [Path(source).resolve()]
    visited = set()
    headers = {}
    while pending:
        current = pending.pop()
        if current in visited or not current.is_file():
            continue
        visited.add(current)
        for line in current.read_bytes().splitlines():
            match = QUOTED_INCLUDE.match(line)
            if not match:
                continue
            included = os.fsdecode(match.group(1))
            candidates = [current.parent / included, *(directory / included for directory in include_dirs)]
            resolved = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
            if resolved is None or not resolved.is_relative_to(repository):
                continue
            if resolved not in headers:
                data = resolved.read_bytes()
                headers[resolved] = data
                pending.append(resolved)
    return headers


def _assert_headers_unchanged(headers, phase):
    for path, before in headers.items():
        if not path.is_file() or path.read_bytes() != before:
            raise ValueError(f"Concurrent quoted-header edit during {phase}: {path}")


def promote(ids, candidate, verify_only=False, original_root=None):
    if JOURNAL.exists():
        raise ValueError("Interrupted promotion: run scripts/promote.py --recover first")
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    wanted = set(ids)
    rows = [r for r in state["functions"] if r["id"] in wanted]
    if not rows or {r["id"] for r in rows} != wanted:
        raise ValueError("Every requested function must already have reviewed target context in recovery.json")
    sources = {r["source"] for r in rows}
    if any(r['status'] == 'FUNCTION_MATCH_RELOCATED' and
           (r.get('source') in sources or sources & set(r.get('proof', {}).get('context_sha256', {})))
           for r in state['functions']):
        raise ValueError('Source replacement would touch accepted relocated peers; use reviewed in-place adoption')
    relocated_peers = [r for r in state['functions'] if r['status'] == 'FUNCTION_MATCH_RELOCATED']
    if relocated_peers and not all(r['exact'] for r in verify_rows(relocated_peers, original_root=original_root)[0]):
        raise ValueError('Previously accepted relocated peer regressed')
    if len(sources) != 1:
        raise ValueError("One source file per promotion")
    destination = ROOT / next(iter(sources))
    if not destination.resolve().is_relative_to((ROOT / "src").resolve()):
        raise ValueError("Promotion destination must be under src/")
    candidate = Path(candidate).resolve()
    content = candidate.read_bytes()
    suffix = destination.suffix.lower()
    if suffix not in {".c", ".cpp"}:
        raise ValueError("Promotion destination must be a .c or .cpp source under src/")
    if candidate.suffix.lower() != suffix:
        raise ValueError("Candidate and promotion destination must use the same source language suffix")
    if suffix == ".cpp" and candidate != destination.resolve():
        raise ValueError("C++ candidates must be staged at their canonical src/*.cpp path so quoted-header lookup is checked in place")
    observed = {STATE: state_before}
    observed[candidate] = content
    header_observed = {}
    for row in state["functions"]:
        if row["status"] == "FUNCTION_MATCH" or row["id"] in wanted:
            path = ROOT / row["source"]
            observed[path] = path.read_bytes() if path.exists() else None
            header_observed.update(quoted_repository_headers(path, row["flags"]))
    observed.update(header_observed)
    allowed_symbols = {r["symbol"] for r in state["functions"] if r.get("source") in sources and
                       (r["status"] == "FUNCTION_MATCH" or r["id"] in wanted)}
    results = {}
    # Freeze candidate bytes for the compile and transaction to avoid a concurrent
    # scratch writer changing the candidate after verification.
    frozen = ROOT / "work/promotion-candidate-stage" / f"promotion-candidate{suffix}"
    frozen.parent.mkdir(parents=True, exist_ok=True)
    frozen.write_bytes(content)
    try:
        for row in state["functions"]:
            if row["status"] != "FUNCTION_MATCH" and row["id"] not in wanted:
                continue
            if row.get("source") in sources and suffix == ".cpp":
                # The frozen source remains C++; prepend the installed source
                # parent to ordinary /I lookup so quoted repository headers
                # resolve exactly where they will for the canonical path.
                frozen_row = dict(row)
                frozen_row["flags"] = ["/I" + str(destination.parent.resolve()), *row["flags"]]
                staged_result = check(frozen_row, frozen, original_root=original_root)
                if not staged_result["exact"]:
                    raise ValueError(f"Promotion rejected: frozen C++ candidate {row['id']} is not exact: {staged_result}")
                validate_candidate_object(staged_result["object"], allowed_symbols)

                # Independently compile the real canonical source-parent path
                # with only the flags recorded in recovery.json. The result
                # below, not the frozen compile, supplies the stored proof.
                result = check(row, destination, original_root=original_root)
            else:
                result = check(row, frozen if row.get("source") in sources else None,
                               original_root=original_root)
            if not result["exact"]:
                raise ValueError(f"Promotion rejected: {row['id']} is not exact: {result}")
            if row.get("source") in sources and result["source_sha256"] != digest(content):
                raise ValueError(f"Candidate changed before the exact source bytes were compiled: {row['id']}")
            results[row["id"]] = result
            if row.get("source") in sources:
                validate_candidate_object(result["object"], allowed_symbols)
        for path, before in observed.items():
            if (path.read_bytes() if path.exists() else None) != before:
                if path in header_observed:
                    raise ValueError(f"Concurrent quoted-header edit during verification: {path}")
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
            _assert_headers_unchanged(header_observed, "promotion commit")
            atomic_bytes(destination, content)
            _assert_headers_unchanged(header_observed, "source replacement")
            atomic_bytes(STATE, state_content)
            _assert_headers_unchanged(header_observed, "state replacement")
            _assert_headers_unchanged(header_observed, "journal removal")
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
    from match import accepted_rows, RELOCATED
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
        if not path.is_relative_to(ROOT / 'src'):
            raise ValueError('Existing units must be repository source')
        if row['status'] not in ('FUNCTION_MATCH', RELOCATED) and not (
                row['status'] == 'CODEGEN_SIMILAR' and row.get('proof', {}).get('scope') == SCOPE):
            raise ValueError('Every existing-unit peer needs a supported prior proof')
        if digest(path.read_bytes()) != row['proof']['source_sha256']:
            raise ValueError('Existing source differs from recorded proof: ' + row['id'])
    checked = copy.deepcopy(accepted_rows(state['functions']) + selected)
    for row in checked:
        if row['id'] in wanted:
            row['status'] = 'FUNCTION_MATCH'  # a request to the raw gate, not state publication
    observed = {STATE: state_before, ROOT / 'toolchains/manifest.json': (ROOT / 'toolchains/manifest.json').read_bytes()}
    for row in checked + peers:
        observed[ROOT / row['target']] = (ROOT / row['target']).read_bytes()
        for path in compile_inputs((ROOT / row['source']).resolve(), row['flags'], DEFAULT_TOOLCHAIN):
            observed[path] = path.read_bytes()
    results, statistics = verify_rows(checked)
    if not all(r['exact'] for r in results):
        raise ValueError('Existing-unit promotion rejected by complete raw comparison')
    if any(r['status'] == 'LINKED_MATCH' for r in state['functions']):
        from linked import verify
        verify()
    by_id = {r['id']: r for r in results}
    units = {}
    for source in sorted(sources):
        members = [r for r in peers if r['source'] == source]
        if len({tuple(r['flags']) for r in members}) != 1:
            raise ValueError('Unit peers disagree about compiler flags')
        if len({r['id'] for r in members}) != len(members):
            raise ValueError('Duplicate existing-unit peer records')
        mapped_names = [r.get('symbol') for r in members]
        if any(not isinstance(symbol, str) or not symbol for symbol in mapped_names):
            raise ValueError('Existing-unit peer has an invalid function symbol')
        mapped = set(mapped_names)
        # The same source function can be recorded against different target
        # modules. It still denotes one emitted COFF definition, and cannot be
        # claimed twice within the same target.
        symbol_targets = {}
        for row in members:
            target = (ROOT / row['target']).resolve().as_posix().casefold()
            if target in symbol_targets.setdefault(row['symbol'], set()):
                raise ValueError('Function inventory maps one symbol more than once per target')
            symbol_targets[row['symbol']].add(target)

        masked_members = [r for r in members if r['status'] == 'CODEGEN_SIMILAR']
        declarations = set()
        for row in masked_members:
            names = row.get('proof', {}).get('co_defined_functions', [])
            if (not isinstance(names, list) or
                    any(not isinstance(name, str) or not name for name in names) or
                    len(names) != len(set(names))):
                raise ValueError('Invalid pinned co-defined function inventory: ' + row['id'])
            declarations.add(frozenset(names))
        if len(declarations) != 1:
            raise ValueError('Unit peers disagree about pinned co-defined functions')
        pinned_co_defined = set(next(iter(declarations)))
        # Relocated peers may appear in masked rows' pinned inventory because
        # the masked verifier treats them as co-defined. They remain accepted
        # only through their unchanged, independently checked relocated proof.
        relocated_symbols = {r['symbol'] for r in members if r['status'] == RELOCATED}
        if (pinned_co_defined & mapped) - relocated_symbols:
            raise ValueError('Pinned co-defined function overlaps an unsupported peer')
        co_defined = pinned_co_defined - mapped
        result = by_id[next(r['id'] for r in members if r['id'] in wanted)]
        obj = Path(result['object'])
        observed[obj] = obj.read_bytes()
        coff = COFF(obj)
        defined = [s['name'] for s in coff.symbols.values() if s['section'] > 0 and s['type'] & 0x20]
        expected = mapped | co_defined
        if len(defined) != len(set(defined)) or set(defined) != expected:
            raise ValueError('Complete emitted function inventory differs from recorded unit')
        for symbol in sorted(expected):
            coff.function(symbol)  # complete independently bounded COMDAT, never slices
        for row in members:
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
    assert_verified_inputs(checked, results)
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
          f"{len(results)} accepted/candidate rows, {len(peers)} unit peers preserved")
    return receipt


def assert_verified_inputs(rows, results, original_root=None):
    """Recheck frozen compiler, reference and oracle inputs before publication."""
    from match import RELOCATED, ReferenceSnapshot, reference_inputs, reference_digest
    by_id = {r['id']: r for r in results}
    # Publication gets its own snapshot, separate from the verification pass.
    # Repeated bindings can share evidence files, but all of those inputs are
    # freshly rehashed once more at the end of this phase.
    reference_snapshot = ReferenceSnapshot()
    for row in rows:
        result = by_id[row['id']]
        if digest(Path(result['object']).read_bytes()) != result['object_sha256']:
            raise ValueError('Compiler object changed before publication')
        for storage in result.get('storage_objects', {}).values():
            if digest(Path(storage['object']).read_bytes()) != storage['object_sha256']:
                raise ValueError('Loader-zero owner object changed before publication')
            for path, expected in storage['compile_inputs'].items():
                if digest(Path(path).read_bytes()) != expected:
                    raise ValueError('Loader-zero owner input changed before publication')
        for adapted in result.get('storage_alias_objects', {}).values():
            if digest(Path(adapted['object']).read_bytes()) != adapted['object_sha256']:
                raise ValueError('Compiler-renaming object changed before publication')
            for path, expected in adapted['compile_inputs'].items():
                if digest(Path(path).read_bytes()) != expected:
                    raise ValueError('Compiler-renaming input changed before publication')
        for path, expected in result['compile_inputs'].items():
            if digest(Path(path).read_bytes()) != expected:
                raise ValueError('Compiler input changed before publication')
        if digest(((original_root or ROOT) / row['target']).read_bytes()) != row['target_sha256']:
            raise ValueError('Oracle changed before publication')
        if result.get('proof_category') == RELOCATED:
            binding, evidence, _ = reference_inputs(result['bindings_path'], row,
                                                     snapshot=reference_snapshot)
            if reference_digest(binding) != result['bindings_sha256'] or reference_digest(evidence) != result['reference_evidence_sha256']:
                raise ValueError('Reference binding changed before publication')
    reference_snapshot.assert_unchanged()


def _relocation_bindings(ids, bindings):
    """Map every requested ID to exactly one repository evidence document."""
    from match import ROOT as MATCH_ROOT

    supplied = list(bindings) if isinstance(bindings, (list, tuple)) else [bindings]
    if not supplied or any(not isinstance(path, (str, Path)) for path in supplied):
        raise ValueError('Provide one or more reference binding documents')
    root = MATCH_ROOT.resolve()
    documents = []
    seen_paths = set()
    for item in supplied:
        path = Path(item).resolve()
        if path in seen_paths:
            raise ValueError('Duplicate reference binding document path')
        seen_paths.add(path)
        if not path.is_relative_to(root):
            raise ValueError('Reference bindings must be repository evidence')
        try:
            document = json.loads(path.read_text(encoding='utf-8'))
        except (OSError, UnicodeError, json.JSONDecodeError) as exc:
            raise ValueError('Cannot read reference binding document: ' + str(path)) from exc
        if not isinstance(document, dict):
            raise ValueError('Reference bindings need a versioned function inventory')
        functions = document.get('functions')
        if document.get('schema') != 1 or not isinstance(functions, dict):
            raise ValueError('Reference bindings need a versioned function inventory')
        documents.append((path.relative_to(root).as_posix(), set(functions)))

    owners = {}
    for function_id in ids:
        paths = [relative for relative, functions in documents if function_id in functions]
        if not paths:
            raise ValueError('No reference binding document covers: ' + function_id)
        if len(paths) != 1:
            raise ValueError('Multiple reference binding documents cover: ' + function_id)
        owners[function_id] = paths[0]
    used_paths = set(owners.values())
    if used_paths != {relative for relative, _ in documents}:
        raise ValueError('Every supplied reference binding document must cover a requested function')
    return owners


def promote_relocated(ids, bindings, verify_only=False, report=None, original_root=None):
    """Status-only acceptance with independently reviewed references and full regression."""
    from match import RELOCATED, accepted_rows
    if JOURNAL.exists():
        raise ValueError('Interrupted promotion: recover it first')
    before = STATE.read_bytes()
    state = json.loads(before)
    selected = [r for r in state['functions'] if r['id'] in set(ids)]
    if not ids or len(ids) != len(set(ids)) or len(selected) != len(ids):
        raise ValueError('Provide unique existing recovery IDs')
    if any(r['status'] != 'CODEGEN_SIMILAR' for r in selected):
        raise ValueError('Relocated adoption requires reviewed CODEGEN_SIMILAR rows; preserves existing acceptance')
    pending = _relocation_bindings([r['id'] for r in selected], bindings)
    rows = accepted_rows(state['functions']) + selected
    results, statistics = verify_rows(rows, original_root=original_root, relocation_candidates=pending)
    if not all(r['exact'] for r in results):
        raise ValueError('Promotion rejected by complete function/reference regression')
    # Existing linked acceptance remains independently verified, never downgraded.
    if any(r['status'] == 'LINKED_MATCH' for r in state['functions']):
        from linked import verify
        verify(original_root=original_root)
    by_id = {r['id']: r for r in results}
    for row in selected:
        result = by_id[row['id']]
        row['prior_masked_proof'] = row['proof']
        row['status'] = RELOCATED
        row['proof'] = {k: result[k] for k in (
            'scope', 'source_sha256', 'compiled_sha256', 'target_sha256', 'toolchain',
            'bindings_path', 'bindings_sha256', 'reference_evidence_sha256', 'context_sha256')}
    # The diagnostic verifier inventories whole translation units. Accepted peers
    # remain present but are now checked by match.py rather than by the masked gate.
    promoted_sources = {r['source'] for r in selected}
    for row in state['functions']:
        if row['status'] == 'CODEGEN_SIMILAR' and row.get('source') in promoted_sources:
            peers = {r['symbol'] for r in selected if r['source'] == row['source'] and r['flags'] == row['flags']}
            remaining = {r['symbol'] for r in state['functions'] if r['status'] == 'CODEGEN_SIMILAR'
                         and r.get('source') == row['source'] and r['flags'] == row['flags']}
            row['proof']['co_defined_functions'] = sorted((set(row['proof'].get('co_defined_functions', [])) | peers) - remaining)
    receipt = dict(promoted=sorted(ids), statistics=statistics, results=results, verify_only=verify_only)
    if report:
        if not any(report.resolve().is_relative_to(ROOT / d) for d in ('work', 'build')):
            raise ValueError('Generated promotion reports belong under work/ or build/')
        report.parent.mkdir(parents=True, exist_ok=True)
        report.write_text(json.dumps(receipt, indent=2) + '\n')
    if STATE.read_bytes() != before:
        raise ValueError('Recovery state changed during promotion')
    assert_verified_inputs(rows, results, original_root)
    if not verify_only:
        after = (json.dumps(state, indent=2) + '\n').encode()
        journal = [dict(path='recovery.json', before=base64.b64encode(before).decode(),
                        before_sha256=digest(before), after_sha256=digest(after))]
        atomic_bytes(JOURNAL, json.dumps(journal).encode())
        try:
            atomic_bytes(STATE, after)
        except BaseException:
            recover()
            raise
        JOURNAL.unlink()
    print(f"{'Verified' if verify_only else 'Promoted'} {len(selected)} relocated functions, "
          f"{sum(r['size'] for r in selected)} bytes; {len(results) - len(selected)} accepted peers preserved")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ids", nargs="*")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--recover", action="store_true")
    parser.add_argument("--original-root", type=Path,
                        help="Read hash-pinned originals from this checkout while keeping local source and recovery state")
    parser.add_argument('--existing-unit', action='store_true', help='accept raw functions in unchanged recorded units; status only')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--relocated', action='store_true', help='status-only acceptance with reviewed references')
    parser.add_argument('--bindings', type=Path, action='append',
                        help='independent reference evidence document (repeat for multiple documents)')
    args = parser.parse_args()
    with lock():
        if args.recover:
            recover()
        elif args.relocated:
            if not args.ids or not args.bindings or args.source or args.existing_unit:
                parser.error('--relocated requires IDs and --bindings, forbids --source/--existing-unit')
            promote_relocated(args.ids, args.bindings, args.verify_only, args.report, args.original_root)
        elif args.existing_unit:
            if not args.ids or args.source is not None:
                parser.error('--existing-unit requires IDs and forbids --source')
            promote_existing_units(args.ids, args.verify_only, args.report)
        elif not args.ids or args.source is None:
            parser.error("provide function ids and --source, or --recover")
        else:
            promote(args.ids, args.source, args.verify_only, args.original_root)


if __name__ == "__main__":
    main()
