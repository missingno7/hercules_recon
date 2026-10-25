"""Re-point FUNCTION_MATCH rows at a new canonical source (e.g. their address unit).

usage: move_strict.py ID... --source src/title/u0cc90.c [--dry-run] [--original-root PATH]

The new source must already be installed (LF, under src/, same language suffix as
the row's flags imply nothing else). Each row is freshly compiled from the new source with its
recorded flags and must be raw exact exactly as scripts/match.py check requires, with the same
compiled/target bytes as its existing proof. Only the row's `source` and `proof` change; the proof
fields are the ones scripts/promote.py records (scope, source_sha256, compiled_sha256,
target_sha256, toolchain). Anything else refuses. Rows of other modules that share the old
source are untouched (their source file stays).
"""
import argparse
import json
import os
from pathlib import Path
import re
import time

from env import ROOT
from match import check, digest

STATE = ROOT / 'recovery.json'
PROOF_FIELDS = ('scope', 'source_sha256', 'compiled_sha256', 'target_sha256', 'toolchain')
QUOTED = re.compile(rb'^\s*#\s*include\s*"([^"\r\n]+)"')


def headers(source):
    """Transitive literal-quoted repository headers of the new source (concurrency guard)."""
    pending, seen, out = [Path(source).resolve()], set(), {}
    while pending:
        current = pending.pop()
        if current in seen or not current.is_file():
            continue
        seen.add(current)
        for line in current.read_bytes().splitlines():
            m = QUOTED.match(line)
            if m:
                found = (current.parent / os.fsdecode(m.group(1))).resolve()
                if found.is_file() and found.is_relative_to(ROOT.resolve()) and found not in out:
                    out[found] = found.read_bytes()
                    pending.append(found)
    return out


def write_state(state):
    temporary = STATE.with_name('recovery.json.move-tmp')
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


def move(ids, source, dry_run=False, original_root=None):
    if (ROOT / 'work/promotion-journal.json').exists():
        raise ValueError('Interrupted promotion: run scripts/promote.py --recover first')
    rel = Path(source)
    if rel.is_absolute() or '..' in rel.parts:
        raise ValueError('Source must be repository-relative')
    path = (ROOT / rel).resolve()
    if not path.is_relative_to((ROOT / 'src').resolve()):
        raise ValueError('Strict sources belong under src/')
    if path.suffix.lower() not in ('.c', '.cpp') or not path.is_file():
        raise ValueError('Source must be an existing .c or .cpp file')
    content = path.read_bytes()
    if b'\r' in content:
        raise ValueError('Source must be LF text (proof hashes must survive checkout)')
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    rows = [r for r in state['functions'] if r['id'] in set(ids)]
    if len(rows) != len(set(ids)):
        raise ValueError('Unknown function id')
    observed = headers(path)
    results = {}
    for row in rows:
        if row['status'] != 'FUNCTION_MATCH':
            raise ValueError(f"{row['id']} is {row['status']}; only FUNCTION_MATCH rows move with this tool")
        if row['source'] != rel.as_posix():
            old = check(row, original_root=original_root)  # current canonical proof must still verify
            if not old['exact']:
                raise ValueError(f"{row['id']}: current source no longer verifies")
        # Same source after a code-neutral edit of its unit (e.g. new data definitions): the old source hash
        # cannot verify, so the proof is renewed only if the fresh compile is raw exact AND its compiled bytes
        # are identical to the recorded compiled_sha256 (checked below).
        result = check(row, path, original_root=original_root)
        if not result['exact']:
            raise ValueError(f"{row['id']} is not raw exact in {source}: {result['different_bytes']} bytes, "
                             f"{len(result['relocations'])} relocations")
        if result['source_sha256'] != digest(content):
            raise ValueError(f"{source} changed before its exact bytes were compiled")
        if (result['compiled_sha256'], result['target_sha256']) != (row['proof']['compiled_sha256'],
                                                                    row['proof']['target_sha256']):
            raise ValueError(f"{row['id']}: compiled/target identity differs from the accepted proof")
        results[row['id']] = result
    if path.read_bytes() != content or STATE.read_bytes() != state_before:
        raise ValueError('Concurrent edit of the source or recovery.json during verification')
    for p, before in observed.items():
        if not p.is_file() or p.read_bytes() != before:
            raise ValueError(f'Concurrent quoted-header edit during verification: {p}')
    for row in rows:
        print(f"{row['id']}: {row['source']} -> {rel.as_posix()} (EXACT {results[row['id']]['compiled_size']} bytes)")
    if dry_run:
        return results
    for row in rows:
        row['source'] = rel.as_posix()
        row['proof'] = {k: results[row['id']][k] for k in PROOF_FIELDS}
    if STATE.read_bytes() != state_before:
        raise ValueError('recovery.json changed before the write')
    write_state(state)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('ids', nargs='+')
    parser.add_argument('--source', required=True)
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--original-root', type=Path)
    args = parser.parse_args()
    from promote import lock
    with lock():
        move(args.ids, args.source, args.dry_run, args.original_root)


if __name__ == '__main__':
    main()
