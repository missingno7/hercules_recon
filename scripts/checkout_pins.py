"""Clean-checkout reproducibility of accepted-proof pins (compiler-free).

Clones a commit into a temporary directory with a fresh checkout under each
core.autocrlf setting and checks, in those checked-out bytes only, that every
RAW/RELOCATED proof's canonical source hash, every evidence_files pin of its
binding document (through any reference_snapshots redirect), and every
evidence/repin_snapshots content address still hold. The developer worktree's
own bytes are never read. Hash-pinned originals (assets/, work/discs) are not
committed and are checked by the fresh verifier with --original-root instead.

    python scripts/checkout_pins.py [--commit HEAD]
"""
import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from env import ROOT
from evidence_keys import canonical_keys, checked_path, external_parts, load_json
import match
from reviewed_evidence import snapshot_path

ACCEPTED = ('FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED')
SNAPSHOTS = 'evidence/repin_snapshots'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_tree(tree):
    """Return (counts, problems) for one checked-out tree."""
    rows = load_json((tree / 'recovery.json').read_bytes())['functions']
    problems, counts = [], dict(rows=0, sources=0, pins=0, snapshots=0)
    verified = set()  # components already confined during this one read-only pass
    for snapshot in sorted((tree / SNAPSHOTS).glob('*.txt')):
        counts['snapshots'] += 1
        if digest(snapshot) != snapshot.stem:
            problems.append(('snapshot address', snapshot.name))
    for row in rows:
        if row['status'] not in ACCEPTED:
            continue
        counts['rows'] += 1
        proof = row['proof']
        counts['sources'] += 1
        if digest(tree / row['source']) != proof['source_sha256']:
            problems.append(('source', row['id'], row['source']))
        path = proof.get('bindings_path')
        if not path:
            continue
        document = load_json((tree / path).read_text(encoding='utf-8'))
        redirects = proof.get('reference_snapshots', {})
        files = document.get('evidence_files') or {}
        try:
            keys = canonical_keys(files)  # the verifier's one shared spelling; ambiguous aliases refused
        except ValueError as error:
            problems.append(('invalid pin key', row['id'], str(error)))
            continue
        for name, expected in files.items():
            relative = keys[name]
            if relative is None:
                # Not a repository key: only pinned SDK inputs (exact prefix), verified by match.py.
                if name in redirects:
                    problems.append(('invalid snapshot redirect', row['id'], name))
                elif external_parts(name, match.DEFAULT_TOOLCHAIN) is None:
                    problems.append(('invalid absolute pin', row['id'], name))
                continue
            if relative.startswith(('assets/', 'work/discs/')):
                if name in redirects:
                    problems.append(('invalid snapshot redirect', row['id'], name))
                continue  # uncommitted originals: verified by match.py --original-root
            counts['pins'] += 1
            try:
                # Provenance confinement first, then the exact content address of any redirect.
                item = checked_path(tree, relative.split('/'), verified)
                item = snapshot_path(tree, name, expected, redirects) or item
            except ValueError as error:
                problems.append(('invalid pin path', row['id'], name, str(error)))
                continue
            if not item.exists():
                problems.append(('missing pin', row['id'], name))
            elif digest(item) != expected:
                problems.append(('pin hash', row['id'], name))
    return counts, problems


def clean_checkouts(commit='HEAD', settings=('true', 'false')):
    results = {}
    with tempfile.TemporaryDirectory(prefix='checkout_pins_') as scratch:
        for autocrlf in settings:
            tree = Path(scratch) / f'autocrlf_{autocrlf}'
            subprocess.run(['git', '-c', f'core.autocrlf={autocrlf}', 'clone', '-q', '--shared', '--no-checkout',
                            str(ROOT), str(tree)], check=True)
            subprocess.run(['git', '-C', str(tree), 'config', 'core.autocrlf', autocrlf], check=True)
            subprocess.run(['git', '-C', str(tree), '-c', 'advice.detachedHead=false', 'checkout', '-q', commit],
                           check=True)
            results[autocrlf] = check_tree(tree)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--commit', default='HEAD')
    args = parser.parse_args()
    failed = False
    for autocrlf, (counts, problems) in clean_checkouts(args.commit).items():
        print(f'autocrlf={autocrlf}: {counts}; problems: {len(problems)}')
        for problem in problems[:20]:
            print('  ', *problem)
        failed |= bool(problems)
    raise SystemExit(1 if failed else 0)


if __name__ == '__main__':
    main()
