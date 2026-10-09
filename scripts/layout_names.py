"""Choose identifiers for one object's uninitialized variables so VC5 lays them out in address order.

VC5 orders an object's file statics, C++ globals and C communals by identifier hash bucket
(scripts/symbol_order.py). Given the variables in original address order, each with a list of
meaningful candidate words (most preferred first) or one fixed name, this picks per variable the
most natural candidate such that the buckets strictly increase in address order. The hash mixes
trailing characters weakly, so variation comes from the words and the prefix; numeric suffixes are
the last resort. Chosen names are layout-compatible names, not recovered identifiers: record them.

usage: layout_names.py [--prefix P,P..] SPEC...   (address order)
  SPEC  word1,word2,...   candidate words for one variable
        =name             a name that must stay as is
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from symbol_order import bucket, emission_order  # noqa: E402


def variants(words, prefixes):
    """(cost, name): earlier words and prefixes are preferred; numeric suffixes cost most."""
    for w, word in enumerate(words):
        for p, prefix in enumerate(prefixes):
            yield 10 * w + p, prefix + word
    for w, word in enumerate(words):
        for i in range(2, 10):
            yield 100 + 10 * w + i, f'{prefixes[0]}{word}{i}'


def choose(specs, prefixes):
    """specs: [list of words, or a fixed name str] in address order -> chosen names, or None."""
    best = {-1: (0, [])}                       # last bucket -> (cost, names)
    for spec in specs:
        options = [(0, spec)] if isinstance(spec, str) else list(variants(spec, prefixes))
        nxt = {}
        for last, (cost, names) in best.items():
            for c, name in options:
                b = bucket(name)
                if b > last and (b not in nxt or nxt[b][0] > cost + c):
                    nxt[b] = (cost + c, names + [name])
        if not nxt:
            return None
        best = nxt
    return min(best.values())[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--prefix', default='s_,,', help='comma-separated prefixes, most preferred first')
    parser.add_argument('specs', nargs='+')
    args = parser.parse_args()
    prefixes = args.prefix.split(',')
    specs = [s[1:] if s.startswith('=') else s.split(',') for s in args.specs]
    chosen = choose(specs, prefixes)
    if chosen is None:
        sys.exit('no assignment')
    assert emission_order(chosen) == chosen
    for spec, name in zip(specs, chosen):
        print(f'{bucket(name):4d}  {name}')


if __name__ == '__main__':
    main()
