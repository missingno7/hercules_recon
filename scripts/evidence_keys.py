"""One canonical spelling of evidence_files keys, and OS-identity confinement of what they read.

Accepted binding manifests pin repository files under relative keys and, historically, under
exactly two absolute spellings of the original checkout: forward slashes throughout, or Windows
backslashes throughout. Both are pure-string aliases of their relative key. Nothing about an alias
is resolved against the filesystem, and an alias never names the current checkout's location: every
consumer reads the LOCAL repository file of the canonical relative key. Any other absolute key is not
a repository key (pinned SDK inputs keep their own, exact-prefix handling).

Confinement of the file a key reads is decided separately and never by comparing path strings or
case: every existing component below a trusted base must be the directory entry itself, not a
symlink, junction or mount point, with the same volume serial and file index whether or not it is
followed (confined_path).
"""
import json
import os
from pathlib import Path
import re
import stat

# The decoded root spellings recorded in accepted evidence_files keys, each with its only separator.
RECORDED_ROOTS = (('D:/Prog/hercules_recon', '/'), ('D:\\Prog\\hercules_recon', '\\'))
ABSOLUTE_TOPS = ('src', 'evidence')
COMPONENT = re.compile(r'[A-Za-z0-9_.-]+')
RESERVED = frozenset(['CON', 'PRN', 'AUX', 'NUL'] + ['COM%d' % i for i in range(1, 10)] +
                     ['LPT%d' % i for i in range(1, 10)])
HEX = re.compile(r'[0-9a-f]{64}')
NAME_SURROGATE = 0x20000000  # reparse tags that name another file: symlinks, junctions, mount points


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate JSON object key: %r' % (key,))
        result[key] = value
    return result


def load_json(data):
    """json.loads for evidence, ledger, review and plan documents; refuses any repeated decoded key.

    Plain json.loads keeps the last of repeated keys (including escaped spellings of one key), which
    would hide a conflicting pin or redirect from every later check.
    """
    return json.loads(data, object_pairs_hook=_unique_object)


def _components(text, separator, name):
    """Strict ASCII components: no empty, '.', '..', trailing dot or space, stream or device names."""
    parts = text.split(separator)
    for part in parts:
        if (not COMPONENT.fullmatch(part) or part.endswith('.') or
                part.split('.')[0].upper() in RESERVED):
            raise ValueError('Invalid evidence key: %r' % (name,))
    return parts


def _absolute(name):
    return name.startswith(('/', '\\')) or re.match(r'[A-Za-z]:', name) is not None


def repository_key(name):
    """The canonical relative '/' key of one evidence_files key, or None for a non-repository key.

    Relative keys use one separator kind ('/' or the legacy '\\'). An absolute key is a repository
    key only when it is exactly a recorded root, its separator and a src/ or evidence/ path in that
    same separator. Malformed repository keys are refused, never repaired.
    """
    if not isinstance(name, str):
        raise ValueError('Invalid evidence key: %r' % (name,))
    for root, separator in RECORDED_ROOTS:
        if name.startswith(root + separator):
            parts = _components(name[len(root) + 1:], separator, name)
            if len(parts) < 2 or parts[0] not in ABSOLUTE_TOPS:
                raise ValueError('Unsupported absolute repository evidence key (src/ or evidence/ only): ' + name)
            return '/'.join(parts)
    if _absolute(name):
        return None
    return '/'.join(_components(name, '\\' if '\\' in name else '/', name))


def key_identity(relative):
    """Windows file-name identity of a canonical key: its components are ASCII, so ASCII case only."""
    return relative.lower()


def canonical_keys(files):
    """{key: canonical relative key or None} of one evidence_files map; refuse ambiguous aliases.

    Two keys alias one file when their canonical keys have the same identity. Only the recorded
    legacy duplicate, an identical canonical key pinning an identical hash, denotes a single pin;
    any other alias pair (case variants, or different pinned hashes) is ambiguous and refused.
    """
    result, seen = {}, {}
    for name, expected in files.items():
        relative = result[name] = repository_key(name)
        if relative is None:
            continue
        first = seen.setdefault(key_identity(relative), (name, relative, expected))
        if first[0] != name and (first[1] != relative or first[2] != expected):
            raise ValueError('Ambiguous evidence keys: %s and %s' % (first[0], name))
    return result


def equivalent_keys(files, relative):
    """Every key of one evidence_files map that names the canonical repository key `relative`."""
    identity = key_identity(relative)
    return [name for name, other in canonical_keys(files).items()
            if other is not None and key_identity(other) == identity]


def external_parts(name, base):
    """Components of an absolute key spelled exactly as `base` + one separator kind, else None."""
    base = Path(base)
    for prefix, separator in ((str(base), '\\'), (base.as_posix(), '/')):
        if name.startswith(prefix + separator):
            return _components(name[len(prefix) + 1:], separator, name)
    return None


def checked_path(base, parts, verified=None):
    """base/parts (unresolved), refused unless every existing component below `base` is itself.

    Each existing component must be neither a symlink nor a name-surrogate reparse point (junction,
    mount point), and must have the same volume serial and file index unfollowed (lstat) and
    followed (stat). Intermediate components must be directories. A missing tail is allowed: reads
    of it fail on their own and writes create real directories. `verified` is an optional set owned
    by one caller's single read-only pass, holding the components this function already accepted.
    """
    path = Path(base)
    for index, part in enumerate(parts):
        if not isinstance(part, str) or not COMPONENT.fullmatch(part) or part.endswith('.'):
            raise ValueError('Invalid evidence path component: %r' % (part,))
        path = path / part
        if verified is not None and str(path) in verified:
            continue
        try:
            own = os.lstat(path)
        except (FileNotFoundError, NotADirectoryError):
            break
        if (stat.S_ISLNK(own.st_mode) or getattr(own, 'st_reparse_tag', 0) & NAME_SURROGATE):
            raise ValueError('Evidence path leaves its base through a link: ' + str(path))
        followed = os.stat(path)
        if (followed.st_dev, followed.st_ino) != (own.st_dev, own.st_ino):
            raise ValueError('Evidence path leaves its base through a link: ' + str(path))
        if index < len(parts) - 1 and not stat.S_ISDIR(own.st_mode):
            raise ValueError('Evidence path crosses a non-directory: ' + str(path))
        if verified is not None:
            verified.add(str(path))
    return Path(base).joinpath(*parts)


def confined_path(base, parts):
    """The resolved path of checked_path: no link below `base` can change what it names."""
    return checked_path(base, parts).resolve()
