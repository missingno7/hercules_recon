"""Explicit read-only original evidence routing for isolated worktrees."""
from pathlib import Path

from evidence_keys import confined_path, repository_key


def inventory_with_original_paths(coff, root, original_root):
    """Relocate only COFF FILE provenance, then require the OLD inventory hash.

    No object is written; every section and every other symbol/aux byte remains
    in the inventory. Absolute paths are the sole checkout-dependent metadata.
    """
    import copy
    symbols = copy.deepcopy(coff.symbols)
    for symbol in symbols.values():
        if symbol['storage'] != 103:
            continue
        payload = b''.join(symbol['aux'])
        name = payload.rstrip(b'\0').decode('ascii')
        path = Path(name)
        if not path.is_absolute() or not path.is_relative_to(root.resolve()):
            raise ValueError('Unexpected COFF FILE provenance')
        replacement = str(Path(original_root).resolve() / path.relative_to(root.resolve())).encode('ascii') + b'\0'
        # Equal-size checkout prefixes avoid changing symbol/aux indices.
        padded = replacement.ljust(len(payload), b'\0')
        if len(padded) != len(payload) or (len(replacement) + 17) // 18 != len(symbol['aux']):
            raise ValueError('COFF FILE rebase would alter symbol indices')
        symbol['aux'] = [padded[i:i+18] for i in range(0, len(padded), 18)]
    return dict(sections=coff.sections, symbols=symbols)


def original_evidence_path(root, name, original_root=None):
    """(path, is_original) of one evidence_files key; SDK keys are routed by match.py.

    Every repository key, including the two recorded absolute aliases, names its LOCAL file, except
    hash-pinned originals (assets/, work/discs/) which an explicit original_root supplies. Confinement
    below either base is OS identity of every component (evidence_keys.confined_path).
    """
    relative = repository_key(name)
    if relative is None:
        raise ValueError('Unsupported absolute repository evidence key: ' + name)
    parts = relative.split('/')
    if original_root is not None and (parts[:1] == ['assets'] or parts[:2] == ['work', 'discs']):
        base = Path(original_root).resolve()
        result = confined_path(base, parts)
        if not any(result.is_relative_to(base / p) for p in ('assets', 'work/discs')):
            raise ValueError('Original evidence escapes original assets root')
        return result, True
    return confined_path(root, parts), False
