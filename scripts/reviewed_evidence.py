"""Opt-in historical evidence reads; never redirects live compiler inputs."""
from pathlib import Path


def snapshot_path(root, name, expected, snapshots):
    if name not in snapshots:
        return None
    original = Path(name)
    if original.is_absolute() or '..' in original.parts or not original.parts or original.parts[0] not in ('src', 'evidence'):
        raise ValueError('Only repository source/review provenance may be snapshotted')
    required = 'evidence/repin_snapshots/' + expected + '.txt'
    if snapshots[name] != required:
        raise ValueError('Historical evidence must use its exact content address')
    path = (root / required).resolve()
    if not path.is_relative_to(root.resolve() / 'evidence/repin_snapshots'):
        raise ValueError('Historical evidence escapes repository snapshots')
    return path
