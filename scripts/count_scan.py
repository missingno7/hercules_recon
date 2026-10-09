"""Diagnostic: is a near-miss reproduced by a translation-unit symbol-count shift? (never acceptance)

blood2_recon (docs/object_residue.md) measured that VC5 register choice, commutative operand order
and scheduling depend on the number of symbols a translation unit declares, periodically in the
count. This recompiles SOURCE with N dummy declarations (--kind) prepended (N in a range)
and reports, per N, the relocation-masked difference against the original function. A hit
classifies the residue as TU context; the padded source is never staged or promoted.
"""
import argparse
import json
import sys
from pathlib import Path

from env import ROOT
from match import compile_source
from masked import masked_compare
from coff import COFF

TARGETS = {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL'}


KINDS = {
    'extern': 'extern int count_pad_{i};',
    'global': 'int count_pad_{i};',
    'typedef': 'typedef int count_pad_t{i};',
    'proto': 'int count_pad_f{i}(int a);',
    'struct': 'struct count_pad_s{i};',
}


def scan(module, rva, size, symbol, source, counts, flags, kind='extern'):
    text = Path(source).read_text()
    results = {}
    for n in counts:
        # Same directory as the source so relative includes keep resolving.
        padded = Path(source).resolve().parent / f'_count_scan_{Path(source).stem}{Path(source).suffix}'
        padded.write_text(''.join(KINDS[kind].format(i=i) + '\n' for i in range(n)) + text)
        try:
            obj, _ = compile_source(padded, flags)
            m = masked_compare(obj, symbol, ROOT / TARGETS[module], rva, size)
            results[n] = 0 if m['masked_equal'] else m['differing_bytes']
            if COFF(obj).function(symbol)['data'].__len__() != size:
                results[n] = f"size {len(COFF(obj).function(symbol)['data'])}"
        finally:
            padded.unlink(missing_ok=True)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module')
    parser.add_argument('rva')
    parser.add_argument('size', type=int)
    parser.add_argument('symbol')
    parser.add_argument('source')
    parser.add_argument('--max', type=int, default=256, help='scan N = 0 .. max-1')
    parser.add_argument('--tp', action='store_true', help='compile as C++ (/TP)')
    parser.add_argument('--kind', choices=sorted(KINDS), default='extern', help='declaration used for padding')
    parser.add_argument('--min', type=int, default=0)
    args = parser.parse_args()
    if not Path(args.source).resolve().is_relative_to(ROOT / 'candidates'):
        raise SystemExit('count scans run on scratch copies under candidates/ only')
    flags = ['/O2'] + (['/TP'] if args.tp else [])
    results = scan(args.module, int(args.rva, 16), args.size, args.symbol, args.source, range(args.min, args.max), flags, args.kind)
    values = sorted({v for v in results.values()}, key=str)
    hits = [n for n, v in results.items() if v == 0]
    print(json.dumps(dict(function=args.rva, base=results[min(results)], hits=hits[:64], hit_count=len(hits),
                          distinct=values[:20]), indent=1))
    by_value = {}
    for n, v in results.items():
        by_value.setdefault(str(v), []).append(n)
    for v, ns in sorted(by_value.items(), key=lambda kv: len(kv[1]), reverse=True)[:8]:
        print(f'  {v:>10}: {len(ns)} counts, e.g. {ns[:12]}')


if __name__ == '__main__':
    sys.exit(main())
