"""Explicit read-only original evidence routing for isolated worktrees."""
from pathlib import Path


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
    relative = Path(name)
    if original_root is not None:
        base = Path(original_root).resolve()
        # Some older manifests used absolute paths inside their checkout.
        # Rebase those inputs onto LOCAL source/evidence, never remote source.
        if relative.is_absolute() and relative.is_relative_to(base):
            relative = relative.relative_to(base)
    if (original_root is not None and not relative.is_absolute() and
            (relative.parts[:1] == ('assets',) or relative.parts[:2] == ('work', 'discs'))):
        result = (base / relative).resolve()
        if '..' in relative.parts or not any(result.is_relative_to(base / p) for p in ('assets', 'work/discs')):
            raise ValueError('Original evidence escapes original assets root')
        return result, True
    return (root / relative).resolve(), False
