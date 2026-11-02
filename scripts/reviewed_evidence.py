"""Opt-in historical evidence reads; never redirects live compiler inputs."""
from pathlib import Path

from evidence_keys import HEX, confined_path, repository_key


def snapshot_path(root, name, expected, snapshots):
    if name not in snapshots:
        return None
    relative = repository_key(name)  # the one shared spelling: a recorded absolute key is its relative alias
    if relative is None:
        raise ValueError('Only repository source/review provenance may be snapshotted')
    original = Path(relative)
    if original.is_absolute() or '..' in original.parts or not original.parts or original.parts[0] not in ('src', 'evidence'):
        raise ValueError('Only repository source/review provenance may be snapshotted')
    required = 'evidence/repin_snapshots/' + expected + '.txt'
    if not isinstance(expected, str) or not HEX.fullmatch(expected) or snapshots[name] != required:
        raise ValueError('Historical evidence must use its exact content address')
    # Confinement is OS identity of every component below the root, never a path-string comparison.
    path = confined_path(root, required.split('/'))
    if not path.is_relative_to(Path(root).resolve() / 'evidence/repin_snapshots'):
        raise ValueError('Historical evidence escapes repository snapshots')
    return path
