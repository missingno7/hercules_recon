"""Fail-closed definition extent regressions for Round 5."""

from pathlib import Path
import unittest
from unittest.mock import patch

import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_consistency_cv4 as cv
import test_consistency_review as fixtures
from test_consistency import M, c, ref
from test_consistency_review import rel, sym


class Round5(unittest.TestCase):
    setUp = fixtures.Reproductions.setUp
    row = fixtures.Reproductions.row
    grade = fixtures.Reproductions.grade

    def compile_fixture(self, ordinary, sidecar, listing):
        with (patch.object(c, 'compile_source', side_effect=[(Path('ordinary.obj'), []), (Path('debug.obj'), [])]),
              patch.object(c, 'COFF', side_effect=[ordinary, sidecar]),
              patch.object(c.Path, 'read_text', return_value=listing)):
            return c.compile_unit('a.c', ('/O2',))

    def check_unsupported(self, compiled, native=b'ABCDEFGHnew!'):
        definitions = c.unit_definitions(M, 'a.c', ('/O2',), compiled)
        self.assertIsNone(definitions[M, '_g'][0]['refs'])
        self.ev.definitions[M, '_g'] = definitions[M, '_g']
        self.image.data[0x3010:0x3010 + len(native)] = native
        self.row('user', ref('_g', 0x3010))
        result = self.grade('user', None)
        self.assertIn('X.DLL | _g | unsupported definition', result['defects'])

    def test_disagreeing_exact_sizes_fail_in_both_directions(self):
        ordinary = cv.coff(b'ABCDEFGHold!', sym('_g', 1))
        for sidecar_size, listing_bytes in ((8, 'ABCDEFGHold!'), (12, 'ABCDEFGH')):
            with self.subTest(sidecar_size=sidecar_size):
                sidecar = cv.add_debug(cv.coff(bytes(12), sym('_g', 1)), cv.array(0x20, sidecar_size))
                cv.bind(sidecar, '_g', 0x1000)
                listing = f"_DATA SEGMENT\n_g DB '{listing_bytes}'\n_DATA ENDS"
                self.assertEqual(c.listing_sizes(listing, exact=True)['_g'], len(listing_bytes))
                self.check_unsupported(self.compile_fixture(ordinary, sidecar, listing))

    def test_trailing_org_does_not_compete_with_codeview(self):
        ordinary = cv.coff(b'ABCDEFGH' + bytes(4), sym('_g', 1))
        sidecar = cv.add_debug(cv.coff(bytes(8), sym('_g', 1)), cv.array(0x20, 8))
        cv.bind(sidecar, '_g', 0x1000)
        listing = "_DATA SEGMENT\n_g DB 'ABCDEFGH'\n\tORG $+4\n_DATA ENDS"
        self.assertIsNone(c.listing_sizes(listing, exact=True)['_g'])
        compiled = self.compile_fixture(ordinary, sidecar, listing)
        definition = c.define(M, 'a.c', ('/O2',), compiled, compiled.unique('_g'))
        self.assertEqual(definition, dict(size=8, payload=b'ABCDEFGH', refs=[]))

    def test_unassignable_debug_record_cannot_fall_back_to_listing(self):
        ordinary = cv.coff(b'ABCDEFGHold!', sym('_g', 1))
        listing = "_DATA SEGMENT\n_g DB 'ABCDEFGH'\n_DATA ENDS"
        for damage in ('missing', 'shifted', 'wrong-kind'):
            with self.subTest(damage=damage):
                sidecar = cv.add_debug(cv.coff(bytes(12), sym('_g', 1)), cv.array(0x20, 12))
                cv.bind(sidecar, '_g', 0x1000)
                fields = sidecar.sections[1]['relocations']
                if damage == 'missing':
                    fields.clear()
                elif damage == 'shifted':
                    for field in fields:
                        field['offset'] += 1
                else:
                    for field in fields:
                        field['type'] = c.DIR32
                with self.assertRaisesRegex(ValueError, 'unassignable CodeView data record relocations'):
                    cv.data_sizes(sidecar, ordinary)
                compiled = self.compile_fixture(ordinary, sidecar, listing)
                self.assertIn('unassignable CodeView', compiled.debug_error)
                self.check_unsupported(compiled)

    def test_partially_bound_record_stays_unsupported_but_recordless_symbol_can_fall_back(self):
        symbols = (sym('_g', 1), sym('_other', 1, 8))
        ordinary = cv.coff(b'ABCDEFGHIJKL', *symbols)
        sidecar = cv.add_debug(cv.coff(bytes(12), *symbols), cv.array(0x20, 8))
        cv.bind(sidecar, '_g', 0x1000)
        sidecar.sections[1]['relocations'].pop()
        self.assertEqual(cv.data_sizes(sidecar, ordinary), {'_g': None})
        listing = "_DATA SEGMENT\n_g DB 'ABCDEFGH'\n_other DB 'IJKL'\n_DATA ENDS"
        compiled = self.compile_fixture(ordinary, sidecar, listing)
        self.check_unsupported(compiled)
        other = c.define(M, 'a.c', ('/O2',), compiled, compiled.unique('_other'))
        self.assertEqual(other, dict(size=4, payload=b'IJKL', refs=[]))

    def test_ordinary_relocation_failure_survives_both_size_sources(self):
        ordinary = cv.coff(b'ABCDEFGH' + bytes(4), sym('_g', 1), sym('_target', 0))
        ordinary.sections[0]['relocations'].append(rel(6, '_target'))
        listing = "_DATA SEGMENT\n_g DB 'ABCDEFGH'\n_DATA ENDS"
        for with_codeview in (True, False):
            with self.subTest(with_codeview=with_codeview):
                sidecar = cv.add_debug(cv.coff(bytes(12), sym('_g', 1)), cv.array(0x20, 8))
                if with_codeview:
                    cv.bind(sidecar, '_g', 0x1000)
                compiled = self.compile_fixture(ordinary, sidecar, listing)
                self.assertEqual(compiled.extent(compiled.unique('_g')), 8)
                self.check_unsupported(compiled)


if __name__ == '__main__':
    unittest.main()
