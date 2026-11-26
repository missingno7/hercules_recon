"""CodeView extent and build separation regressions for the consistency tier."""
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from struct import pack
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from test_consistency import M, c
import test_consistency_review as fixtures
from test_consistency_review import ledger_row, section, sym, unit
from cv4_sizes import C11, data_sizes, type_sizes


def record(kind, payload):
    return pack('<HH', len(payload) + 2, kind) + payload


def name(text):
    return bytes([len(text)]) + text


def array(element, length):
    numeric = pack('<H', length) if length < 0x8000 else pack('<HI', 0x8004, length)
    return record(0x1003, pack('<II', element, 0x11) + numeric + name(b''))


def coff(payload, *symbols, flags=0x40):
    return SimpleNamespace(sections=[section(payload, flags=flags)], symbols=dict(enumerate(symbols)))


def add_debug(obj, types=b''):
    obj.sections.extend([section(C11, name='.debug$S'),
                         section(C11 + types, name='.debug$T')])
    return obj


def bind(obj, link, index, kind=0x1008, stream=1, section_name=None, addend=0):
    target = obj.sections[stream]
    start = len(target['data']) + 4
    target['data'] += record(kind, pack('<IIH', index, addend, 0) + name(b'display_name'))
    target['relocations'].extend([
        dict(offset=start + 4, type=11, symbol=link),
        dict(offset=start + 8, type=10, symbol=section_name or link),
    ])


class CodeViewTypes(unittest.TestCase):
    def test_scalar_primitive_pointers_and_qualified_types(self):
        stream = record(0x1001, pack('<IH', 0x41, 2) + b'\xf2\xf1')
        resolve = type_sizes(C11 + stream)
        expected = {0x20: 1, 0x31: 2, 0x74: 4, 0x41: 8, 0x403: 4, 0x57a: None, 0x1000: 8}
        self.assertEqual({index: resolve(index) for index in expected}, expected)

    def test_struct_class_union_enum_and_function_pointer(self):
        records = [
            record(0x1005, pack('<HHIIIH', 2, 0, 0x2000, 0, 0, 16) + name(b'S') + b'\xf2\xf1'),
            record(0x1004, pack('<HHIIIH', 1, 0, 0, 0, 0, 12) + name(b'C')),
            record(0x1006, pack('<HHIH', 2, 0, 0x2000, 8) + name(b'U')),
            record(0x1007, pack('<HHII', 1, 0, 0x22, 0x2000) + name(b'E')),
            record(0x1002, pack('<II', 0x2001, 10)),
        ]
        resolve = type_sizes(C11 + b''.join(records))
        self.assertEqual([resolve(0x1000 + i) for i in range(len(records))], [16, 12, 8, 4, 4])

    def test_nested_and_large_arrays(self):
        resolve = type_sizes(C11 + array(0x11, 6) + array(0x1000, 12) + array(0x20, 40000))
        self.assertEqual([resolve(i) for i in range(0x1000, 0x1003)], [6, 12, 40000])

    def test_array_over_forward_struct_uses_its_explicit_total(self):
        forward = record(0x1005, pack('<HHIIIH', 0, 0x80, 0, 0, 0, 0) + name(b'Pair'))
        resolver = type_sizes(C11 + forward + array(0x1000, 76) + array(0x1001, 152))
        self.assertIsNone(resolver(0x1000))
        self.assertEqual((resolver(0x1001), resolver(0x1002)), (76, 152))

    def test_malformed_or_unresolved_types_fail_closed(self):
        invalid = [
            array(0x1000, 4),
            array(0x11, 3),
            array(0x1001, 4),
            record(0x1003, pack('<IIH', 0x20, 0x11, 0x8004)),
            record(0x1003, pack('<IIHh', 0x20, 0x11, 0x8001, -2) + name(b'')),
            record(0x1005, pack('<HHIIIH', 0, 0x80, 0, 0, 0, 8) + name(b'F')),
            record(0x1002, pack('<II', 0x20, 10 | (8 << 13))),
            record(0x1002, pack('<IIIH', 0x74, 10 | (2 << 5), 0x1001, 0)),
            record(0x2000, b''),
        ]
        for fragment in invalid:
            with self.subTest(fragment=fragment.hex()):
                self.assertIsNone(type_sizes(C11 + fragment)(0x1000))
        with self.assertRaisesRegex(ValueError, 'record length'):
            type_sizes(C11 + b'\x0a\x00\x04\x10')


class DeclaredExtents(unittest.TestCase):
    def test_identical_listing_preserves_padding_or_owned_zeros(self):
        listing = "_DATA SEGMENT\n_array DB 'abc', 00H\n  ORG $+4\n_DATA ENDS"
        self.assertEqual(c.listing_sizes(listing), {'_array': 8})
        original = coff(b'abc\0' + bytes(4), sym('_array', 1))
        for size in (4, 8):
            with self.subTest(size=size):
                debug = add_debug(coff(b'not read', sym('_array', 1)), array(0x70, size))
                bind(debug, '_array', 0x1000)
                unit = c.Unit(original, data_sizes(debug, original))
                found = c.define('X.DLL', 'a.c', (), unit, unit.unique('_array'))
                self.assertEqual((found['size'], found['payload']), (size, original.sections[0]['data'][:size]))

    def test_global_and_function_local_use_relocated_coff_names(self):
        syms = [sym('?global@@3HA', 1), sym('_local$S7', 1, 4, storage=c.STATIC)]
        original = coff(bytes(8), *syms)
        debug = add_debug(coff(bytes(8), *syms))
        debug.sections.append(section(b'', name='.debug$S'))
        bind(debug, '?global@@3HA', 0x74)
        bind(debug, '_local$S7', 0x74, kind=0x1007, stream=3)
        self.assertEqual(data_sizes(debug, original), {'?global@@3HA': 4, '_local$S7': 4})

    def test_ambiguous_or_misbound_debug_records_fail_closed(self):
        original = coff(bytes(8), sym('_g', 1), sym('_other', 1, 4))
        for mode in ('second-section', 'duplicate', 'wrong-target', 'nonzero-addend', 'wrong-kind', 'bad-type'):
            debug = add_debug(coff(bytes(8), *original.symbols.values()))
            if mode == 'second-section':
                debug.sections.append(section(C11, name='.debug$S'))
                with self.assertRaisesRegex(ValueError, 'one primary'):
                    data_sizes(debug, original)
                continue
            kwargs = {'wrong-target': dict(section_name='_other'), 'nonzero-addend': dict(addend=4),
                      'wrong-kind': dict(kind=0x1007)}.get(mode, {})
            bind(debug, '_g', 0x1000 if mode == 'bad-type' else 0x74, **kwargs)
            if mode == 'duplicate':
                bind(debug, '_g', 0x74)
            with self.subTest(mode=mode):
                self.assertIsNone(data_sizes(debug, original)['_g'])

    def test_capacity_and_missing_metadata_fail_closed_for_all_storage(self):
        original = coff(bytes(8), sym('_g', 1), sym('_next', 1, 4), sym('_common', 0, 8))
        for name_, size in (('_g', 5), ('_next', 100), ('_next', None), ('_common', 4), ('_common', None)):
            with self.subTest(symbol=name_, size=size):
                unit = c.Unit(original, {name_: size})
                found = c.define('X.DLL', 'a.c', (), unit, unit.unique(name_))
                self.assertIsNone(found['refs'])
        unit = c.Unit(original, {'_common': 8})
        self.assertEqual(c.define('X.DLL', 'a.c', (), unit, unit.unique('_common')),
                         dict(size=8, payload=None, refs=[]))

    def test_debug_object_size_preserves_ordinary_bytes_and_pointer_fields(self):
        regular, debug = Path('ordinary.obj'), Path('debug.obj')
        original = coff(b'\x04\0\0\0', sym('_p', 1), sym('_target', 0))
        original.sections[0]['relocations'] = [dict(offset=0, type=c.DIR32, symbol='_target')]
        sidecar = add_debug(coff(b'xxxx', sym('_p', 1)))
        bind(sidecar, '_p', 0x474)
        with (patch.object(c, 'compile_source', side_effect=[(regular, []), (debug, [])]) as compile_,
              patch.object(c, 'COFF', side_effect=[original, sidecar]),
              patch.object(c.Path, 'read_text', return_value='')):
            unit = c.compile_unit('a.c', ('/O2',))
        self.assertEqual([call.args[1] for call in compile_.call_args_list], [['/O2', '/Falisting.asm'], ['/O2', '/Z7']])
        self.assertIs(unit.coff, original)
        found = c.define('X.DLL', 'a.c', (), unit, unit.unique('_p'))
        self.assertEqual(found['payload'], b'\x04\0\0\0')
        self.assertEqual(found['refs'][0]['name'], '_target')

    def test_listing_fallback_uses_only_recognized_extents_without_trailing_org(self):
        original = coff(bytes(20), sym('_cv', 1), sym('_str', 1, 4), sym('_padded', 1, 8),
                        sym('_bad', 1, 12), sym('_unresolved', 1, 16))
        sidecar = add_debug(coff(b'', *original.symbols.values()))
        bind(sidecar, '_cv', 0x74)
        bind(sidecar, '_unresolved', 0x1000)
        listing = """_DATA SEGMENT
_cv DD 0
_str DB 'abc', 0
_padded DB 0
    ORG $+3
_bad DD 0
    DB 2 DUP(unknown @ form)
_unresolved DD 0
_DATA ENDS"""
        with (patch.object(c, 'compile_source', side_effect=[(Path('ordinary.obj'), []), (Path('debug.obj'), [])]),
              patch.object(c, 'COFF', side_effect=[original, sidecar]),
              patch.object(c.Path, 'read_text', return_value=listing)):
            compiled = c.compile_unit('a.c', ())
        def extent(name):
            return compiled.extent(compiled.unique(name))
        self.assertEqual([extent(name) for name in ('_cv', '_str', '_padded', '_bad', '_unresolved')],
                         [4, 4, None, None, None])

    def test_failed_debug_build_keeps_comparison_object_and_fails_definition(self):
        original = coff(bytes(4), sym('_g', 1))
        with (patch.object(c, 'compile_source', side_effect=[(Path('ordinary.obj'), []), RuntimeError('VC5')]),
              patch.object(c, 'COFF', return_value=original),
              patch.object(c.Path, 'read_text', return_value='')):
            unit = c.compile_unit('a.c', ())
        self.assertEqual(unit.debug_error, 'VC5')
        self.assertIs(unit.coff, original)
        self.assertIsNone(c.define('X.DLL', 'a.c', (), unit, unit.unique('_g'))['refs'])


class ProfileReuse(unittest.TestCase):
    setUp = fixtures.Reproductions.setUp
    prepare_code = fixtures.Reproductions.prepare_code

    def test_one_profile_shared_by_code_and_data_owner(self):
        row = ledger_row('fn', '_fn', 0x1000, 1, flags=('/O2',))
        self.prepare_code(row, b'\xc3', [])
        obj = unit([sym('_fn', 1, function=True), sym('_g', 2)], b'\xc3',
                   data_sections=[section(bytes(4))], sizes={'_g': 4})
        with (patch.object(c.Path, 'read_bytes', return_value=json.dumps({'functions': [row]}).encode()),
              patch.object(c, 'Image', return_value=self.image),
              patch.object(c, 'data_units', return_value=[(M, 'a.c', ('/O2',), 'owner')]),
              patch.object(c, 'compile_unit', return_value=obj) as compile_):
            evidence = c.gather(jobs=1)
        compile_.assert_called_once_with('a.c', ('/O2',))
        self.assertIn('fn', evidence.rows)
        self.assertEqual(evidence.definitions[M, '_g'],
                         [dict(size=4, payload=bytes(4), refs=[])])


if __name__ == '__main__':
    unittest.main()
