"""Durable review pins for accepted compiler-profile history."""
import hashlib

from evidence_keys import HEX, confined_path, load_json, repository_key


def read_review(root, declaration):
    """Read exactly one canonical, confined evidence key and verify its byte pin."""
    if not isinstance(declaration, dict) or set(declaration) != {'path', 'sha256'}:
        raise ValueError('Invalid compiler profile review declaration')
    name, expected = declaration['path'], declaration['sha256']
    if (not isinstance(name, str) or repository_key(name) != name or
            not name.startswith('evidence/') or not isinstance(expected, str) or not HEX.fullmatch(expected)):
        raise ValueError('Compiler profile reviews require a canonical evidence/ key and SHA-256')
    path = confined_path(root, name.split('/'))
    try:
        raw = path.read_bytes()
    except OSError as error:
        raise ValueError('Missing compiler profile review: ' + name) from error
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ValueError('Compiler profile review hash changed: ' + name)
    document = load_json(raw)
    if not isinstance(document, dict):
        raise ValueError('Compiler profile review must be a JSON object: ' + name)
    return path, expected, document


def history_inputs(row, root):
    """Validate every historical authorization, including reviews superseded later."""
    history = row.get('compiler_profile_history', [])
    if not isinstance(history, list):
        raise ValueError('Invalid compiler profile history: ' + row['id'])
    inputs = {}
    for item in history:
        if not isinstance(item, dict) or set(item) != {'review', 'before', 'after'}:
            raise ValueError('Invalid compiler profile history entry: ' + row['id'])
        path, expected, _ = read_review(root, item['review'])
        if path in inputs and inputs[path] != expected:
            raise ValueError('Conflicting compiler profile review pin: ' + str(path))
        inputs[path] = expected
    return inputs
