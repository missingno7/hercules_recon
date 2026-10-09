"""VC5 (11.00.7022) in-object order of uninitialized variables, recovered from measurements.

Within one object, the compiler emits file statics, C++ globals and C communals in the order of a
1024-bucket identifier hash table: ascending bucket, and within a bucket the later-defined name
first. The hash is the one c1.dll uses to intern identifiers (loop at 0x1060a47b), folded once:

    h = (h >> 4) + 4*h + c   (32-bit, over the identifier's bytes)
    bucket = (h ^ (h >> 16)) & 0x3ff

Validated exactly on four probe sets (120 random names, digit and mixed sets, a C++ global set and a
C communal group; see evidence/title_closure.json layout_blocker_L1). Definition order matters only
for names that share a bucket. Storage offsets then follow this order with natural alignment.
"""
import argparse


def bucket(name):
    h = 0
    for c in name.encode():
        h = ((h >> 4) + h * 4 + c) & 0xffffffff
    return (h ^ (h >> 16)) & 0x3ff


def emission_order(names):
    """Order in which VC5 lays out these variables of one object (names in definition order)."""
    position = {n: i for i, n in enumerate(names)}
    return sorted(names, key=lambda n: (bucket(n), -position[n]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('names', nargs='+', help='identifiers in definition order')
    args = parser.parse_args()
    for n in emission_order(args.names):
        print(f'{bucket(n):4d}  {n}')


if __name__ == '__main__':
    main()
