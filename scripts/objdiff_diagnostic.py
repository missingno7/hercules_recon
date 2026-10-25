"""Bounded candidate-object diagnostics with the optional pinned objdiff CLI.

This never supplies an original target, grants acceptance, or writes recovery state.
"""
import argparse
import json
from pathlib import Path
import subprocess

from env import ROOT
from coff import COFF
from match import digest

TOOL = Path(r'C:\tools\objdiff-3.7.0\objdiff-cli.exe')
TOOL_SHA256 = '91f473c5753c7593253b53a9dbda839bb4d395fdabf449a7250940d3c4de05b4'


def summarize(data, symbol, limit):
    result = {}
    for side in ('left', 'right'):
        matches = [s for s in data[side]['symbols'] if s.get('name') == symbol]
        if len(matches) != 1:
            raise ValueError('Expected one objdiff function on each side: ' + symbol)
        function = matches[0]
        changes = []
        for row in function.get('instructions', []):
            if not row.get('diff_kind'):
                continue
            instruction = row.get('instruction')
            changes.append(dict(
                offset=hex(int(instruction.get('address', 0))) if instruction else None,
                kind=row['diff_kind'],
                instruction=instruction['formatted'] if instruction else '<alignment gap>'))
        result[side] = dict(inferred_body_size=int(function['size']),
                            match_percent=function.get('match_percent'),
                            changes=changes[:limit], omitted_changes=max(0, len(changes) - limit))
    return result


def compare_objects(baseline, candidate, symbol, limit=8):
    if not 1 <= limit <= 64:
        raise ValueError('Diagnostic instruction limit must be between 1 and 64')
    if digest(TOOL.read_bytes()) != TOOL_SHA256:
        raise ValueError('Optional objdiff 3.7.0 CLI identity differs')
    baseline, candidate = Path(baseline).resolve(), Path(candidate).resolve()
    frozen = {path: digest(path.read_bytes()) for path in (baseline, candidate)}
    left, right = COFF(baseline).function(symbol), COFF(candidate).function(symbol)
    command = [str(TOOL), 'diff', '-1', str(baseline), '-2', str(candidate),
               '-o', '-', '--format', 'json', '-c', 'functionRelocDiffs=all', symbol]
    output = subprocess.run(command, capture_output=True, text=True, check=True)
    result = summarize(json.loads(output.stdout), symbol, limit)
    if any(digest(path.read_bytes()) != expected for path, expected in frozen.items()):
        raise ValueError('Candidate object changed during objdiff analysis')
    if digest(TOOL.read_bytes()) != TOOL_SHA256:
        raise ValueError('Optional objdiff CLI changed during analysis')
    result.update(
        scope='Candidate objects only; percentages and inferred body sizes never grant matching acceptance.',
        symbol=symbol, tool_sha256=TOOL_SHA256, command=command,
        object_sha256={str(path): expected for path, expected in frozen.items()},
        complete_contribution_sizes=[len(left['data']), len(right['data'])],
        complete_byte_differences=sum(left['data'][n:n+1] != right['data'][n:n+1]
                                     for n in range(max(len(left['data']), len(right['data'])))),
        ordered_relocations_equal=left['relocations'] == right['relocations'])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('symbol')
    parser.add_argument('--limit', type=int, default=8)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.report and not any(args.report.resolve().is_relative_to(ROOT / d) for d in ('work', 'build')):
        parser.error('Generated diagnostics belong under work/ or build/')
    result = compare_objects(args.baseline, args.candidate, args.symbol, args.limit)
    rendered = json.dumps(result, indent=2) + '\n'
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(rendered, encoding='utf-8')
    print(rendered)


if __name__ == '__main__':
    main()
