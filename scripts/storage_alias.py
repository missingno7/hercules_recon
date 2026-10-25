"""Explicit scalar aliases, proved by fresh compiler renaming.

This does not infer identities from caller addresses or accept weak aliases,
defined-object merging, pointer views, indexed BSS or arbitrary flag adapters.
"""
from pathlib import Path
import re
import tempfile

from coff import COFF


def alias_names(record, name):
    alias = record.get('source_alias')
    owner_name = record.get('owner', {}).get('symbol')
    if not isinstance(alias, dict):
        raise ValueError('Loader-zero owner aliases require an explicit compiler-renaming proof')
    kind = alias.get('kind')
    keys = {'kind', 'header', 'renames'} | ({'pointer_types'} if kind == 'owner-compile-renaming' else set())
    if set(alias) != keys:
        raise ValueError('Loader-zero owner aliases require an explicit compiler-renaming proof')
    if kind == 'compile-renaming' and (type(record.get('size')) is not int or record['size'] != 2):
        raise ValueError('Compiler-renaming aliases support complete WORD scalars only')
    if kind not in ('compile-renaming', 'owner-compile-renaming'):
        raise ValueError('Unsupported compiler-renaming alias kind')
    if kind == 'owner-compile-renaming' and (type(record.get('size')) is not int or record['size'] != 4):
        raise ValueError('Owner renaming supports complete four-byte pointer cells only')
    renames = alias['renames']
    identifier = lambda n: isinstance(n, str) and re.fullmatch(r'[A-Za-z_]\w*', n, re.ASCII)
    if (not isinstance(renames, dict) or not 1 <= len(renames) <= 16 or
            any(not identifier(a) or not identifier(b) or a == b for a, b in renames.items()) or
            len(set(renames.values())) != len(renames) or set(renames) & set(renames.values())):
        raise ValueError('Compiler-renaming aliases need distinct nonchained C identifiers')
    names = {'_' + a: '_' + b for a, b in renames.items()}
    if name not in names or names[name] != owner_name:
        raise ValueError('Compiler-renaming aliases disagree with the exact caller/owner names')
    if kind == 'owner-compile-renaming':
        types = alias['pointer_types']
        if (len(renames) != 1 or not isinstance(types, dict) or set(types) != set(renames) or
                any(value not in ('void', 'byte') for value in types.values())):
            raise ValueError('Owner renaming needs one explicit void/byte pointer type')
    if not isinstance(alias['header'], str):
        raise ValueError('Compiler-renaming aliases need a canonical header')
    return names


def alias_profile(record, name, root):
    names = alias_names(record, name)
    header = (root / record['source_alias']['header']).resolve()
    if not header.is_relative_to((root / 'src').resolve()) or header.suffix != '.h' or not header.is_file():
        raise ValueError('Compiler-renaming header must be existing canonical source')
    if record['source_alias']['kind'] == 'owner-compile-renaming':
        # The generated type declaration must not be defeated by arbitrary
        # header macros/includes/conditionals. Permit exactly the inverse map.
        content = re.sub(r'/\*.*?\*/', '', header.read_text(), flags=re.S)
        definitions = []
        for line in content.splitlines():
            if not line.strip():
                continue
            match = re.fullmatch(r'\s*#\s*define\s+([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*', line, re.ASCII)
            if match is None:
                raise ValueError('Owner-renaming header must contain only its exact inverse macro map')
            definitions.append(match.groups())
        expected = [(owner[1:], caller[1:]) for caller, owner in names.items()]
        if sorted(definitions) != sorted(expected):
            raise ValueError('Owner-renaming header differs from its exact inverse macro map')
    return header, names


def wrapper_source(source, header, root, pointer_types=None):
    """Keep the generated include wrapper under ignored build/, never canonical."""
    directory = root / 'build/match'
    directory.mkdir(parents=True, exist_ok=True)
    if any('"' in str(p) or '\n' in str(p) or '\r' in str(p) for p in (source, header)):
        raise ValueError('Unsupported compiler-renaming include path')
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', newline='\n',
                                     suffix=source.suffix, prefix='alias-', dir=directory, delete=False) as out:
        out.write('#include "' + header.as_posix() + '"\n')
        if pointer_types is not None:
            if (source.suffix != '.c' or not isinstance(pointer_types, dict) or len(pointer_types) != 1 or
                    any(not re.fullmatch(r'[A-Za-z_]\w*', name, re.ASCII) or typ not in ('void', 'byte')
                        for name, typ in pointer_types.items())):
                raise ValueError('Owner pointer type checks require one C void/byte declaration')
            for name, typ in pointer_types.items():
                out.write('extern ' + ('void' if typ == 'void' else 'unsigned char') + ' *' + name + ';\n')
        out.write('#include "' + source.as_posix() + '"\n')
        if pointer_types is not None:
            out.write('typedef char storage_alias_pointer_is_four[(sizeof(void *) == 4) ? 1 : -1];\n')
        return Path(out.name)


def verify_owner_alias_object(caller, original, adapted, names):
    """One original undefined DATA name, exact COMMON rename, data-only owner.

    match.py additionally generates and freshly compiles a checked pointer
    declaration under /WX. This COFF check cannot itself recover C types.
    """
    if len(names) != 1 or set(names) & set(names.values()):
        raise ValueError('Owner renaming requires one distinct scalar name')
    source, left, right = COFF(caller), COFF(original), COFF(adapted)
    before, after = next(iter(names.items()))  # caller -> actual owner
    choices = [s for s in source.symbols.values() if s['name'] == before]
    if (len(choices) != 1 or choices[0]['section'] or choices[0]['value'] or
            choices[0]['storage'] != 2 or choices[0]['type'] & 0x20 or choices[0].get('aux')):
        raise ValueError('Owner alias caller must be undefined strong DATA, never COMMON/function')
    if any(s['name'] == after for s in source.symbols.values()):
        raise ValueError('Owner renaming cannot coalesce a pre-existing caller symbol')
    if any(s['storage'] == 105 for obj in (source, left, right) for s in obj.symbols.values()):
        raise ValueError('Owner renaming cannot contain weak symbols')
    from zero_storage import verify_common
    verify_common(original, after, 4)
    verify_common(adapted, before, 4)
    if (any(s['name'] == before for s in left.symbols.values()) or
            any(s['name'] == after for s in right.symbols.values())):
        raise ValueError('Owner renaming cannot merge pre-existing COMMON/storage names')
    for obj in (left, right):
        if (any(s['section'] > 0 and s['type'] & 0x20 for s in obj.symbols.values()) or
                any(s['size'] and s['flags'] & 0x20000020 for s in obj.sections)):
            raise ValueError('Owner renaming requires a data-only unit without executable storage')
        if any(s['name'] == '.drectve' and b'alternatename:' in s['data'].lower() for s in obj.sections):
            raise ValueError('Owner renaming cannot contain weak linker alias directives')
    rename = {after: before}
    def sections(obj, mapping):
        return [dict(s, relocations=[dict(r, symbol=mapping.get(r['symbol'], r['symbol']))
                                    for r in s['relocations']]) for s in obj.sections]
    def symbols(obj, mapping):
        # FILE auxiliaries describe the fresh include wrapper, not storage.
        if any(s['storage'] == 103 and (s['name'] != '.file' or s['section'] != -2 or s['value'] or s['type'])
               for s in obj.symbols.values()):
            raise ValueError('Owner renaming cannot hide storage in malformed FILE symbols')
        return sorted((mapping.get(s['name'], s['name']), s['section'], s['value'], s['type'],
                       s['storage'], tuple(s.get('aux', ())))
                      for s in obj.symbols.values() if s['storage'] != 103)
    if sections(left, rename) != sections(right, {}):
        raise ValueError('Owner renaming changed complete data/directive sections or relocations')
    if symbols(left, rename) != symbols(right, {}):
        raise ValueError('Owner renaming changed the complete storage/symbol inventory')
    return dict(scope='Fresh exact data-only owner sections and complete non-FILE symbol inventory after one COMMON-name rename.',
                functions=0, size=4, symbols=names)


def verify_alias_object(original, adapted, symbol, names):
    """Every function/data contribution must be equal after ONLY named renaming."""
    left, right = COFF(original), COFF(adapted)
    if (not names or len(set(names.values())) != len(names) or set(names) & set(names.values())):
        raise ValueError('Ambiguous compiler-renaming map')
    def unique(obj, name):
        symbols = [s for s in obj.symbols.values() if s['name'] == name]
        if len(symbols) != 1:
            raise ValueError('Compiler-renaming symbol is missing or ambiguous: ' + name)
        item = symbols[0]
        if item['section'] or item['value'] or item['storage'] != 2 or item['type'] & 0x20 or item.get('aux'):
            raise ValueError('Compiler-renaming aliases require undefined strong data, never object merging')
    for before, after in names.items():
        unique(left, before)
        unique(right, after)
        if any(s['name'] == after for s in left.symbols.values()) or any(s['name'] == before for s in right.symbols.values()):
            raise ValueError('Compiler-renaming aliases cannot merge pre-existing symbols')
    if any(s['storage'] == 105 for obj in (left, right) for s in obj.symbols.values()):
        raise ValueError('Compiler-renaming aliases cannot contain weak symbols')
    def section(sec, rename):
        return dict(size=sec['size'], flags=sec['flags'], data=sec['data'],
                    relocations=[dict(r, symbol=rename.get(r['symbol'], r['symbol'])) for r in sec['relocations']])
    def functions(obj, rename):
        return {s['name']: section(obj.function(s['name']), rename)
                for s in obj.symbols.values() if s['section'] > 0 and s['type'] & 0x20}
    a, b = functions(left, names), functions(right, {})
    if symbol not in a or a != b:
        raise ValueError('Compiler renaming changed a complete function contribution')
    def other_code(obj, rename):
        owned = {s['section'] for s in obj.symbols.values() if s['section'] > 0 and s['type'] & 0x20}
        return [(s['name'], section(s, rename)) for n, s in enumerate(obj.sections, 1)
                if s['flags'] & 0x20 and n not in owned]
    if other_code(left, names) != other_code(right, {}):
        raise ValueError('Compiler renaming changed unclassified executable storage')
    def noncode(obj, rename):
        return [(s['name'], section(s, rename)) for s in obj.sections if not s['flags'] & 0x20]
    if noncode(left, names) != noncode(right, {}):
        raise ValueError('Compiler renaming changed non-code storage or linker directives')
    def externals(obj, rename):
        return sorted((rename.get(s['name'], s['name']), s['section'], s['value'], s['type'], s['storage'])
                      for s in obj.symbols.values() if s['storage'] == 2)
    if externals(left, names) != externals(right, {}):
        raise ValueError('Compiler renaming changed the external symbol inventory')
    return dict(scope='Fresh complete COFF functions, non-code contributions and external inventory equal after explicit data-symbol renaming only.',
                functions=len(a), symbols=names)
