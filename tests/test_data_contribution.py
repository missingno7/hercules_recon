"""Synthetic controls for the opt-in data-contribution-v1 placement module.

Fixtures are real i386 COFF and PE32 files written by the tests; they are parsed
but never linked or executed. Native TITLE.DLL controls are a separate receipt.
"""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from coff import COFF
from pe import PE
from data_contribution import PlacementError, place, place_object
from test_matching import make_pe, workspace_temporary_directory
from test_verifier_forms import object_file

DATA_FLAGS = 0xc0400040     # initialized, read/write, 8-byte alignment
STRING_FLAGS = 0xc0301040   # initialized COMDAT, read/write, 4-byte alignment
PAYLOAD = struct.pack('<IIII', 0x11223344, 7, 0x55667788, 0) + b'menu\0\0\0\0'


class PlacementTests(unittest.TestCase):
    def setUp(self):
        d = workspace_temporary_directory('data-contribution-'); self.addCleanup(d.cleanup)
        self.obj, self.target = [Path(d.name) / n for n in ('unit.obj', 'native.dll')]
        make_pe(self.target)
        self.native = bytearray(self.target.read_bytes())
        self.native[0x600:0x800] = bytes(0x200)
        self.native[0x608:0x608 + len(PAYLOAD)] = PAYLOAD      # RVA 0x2008, 8-aligned
        self.sections = [('.data', PAYLOAD, DATA_FLAGS, [])]
        self.symbols = [('_g_first', 0, 1, 0, 2, []), ('_s_second$S1', 8, 1, 0, 3, []),
                        ('_g_text', 16, 1, 0, 2, [])]

    def run_place(self, number=1, **kwargs):
        self.target.write_bytes(self.native)
        object_file(self.obj, self.sections, self.symbols)
        return place(COFF(self.obj), number, PE(self.target), **kwargs)

    def test_unique_contribution_binds_allocation_intervals(self):
        result = self.run_place()
        self.assertEqual(result['base_rva'], 0x2008)
        self.assertEqual({k: (v['rva'], v['size']) for k, v in result['symbols'].items()},
                         {'_g_first': (0x2008, 8), '_s_second$S1': (0x2010, 8), '_g_text': (0x2018, 8)})
        self.assertTrue(all(v['extent_kind'] == 'allocation-interval' for v in result['symbols'].values()))

    def test_duplicate_native_pattern_rejects(self):
        self.native[0x700:0x700 + len(PAYLOAD)] = PAYLOAD
        with self.assertRaisesRegex(PlacementError, 'Ambiguous'):
            self.run_place()

    def test_overlapping_duplicate_rejects(self):
        self.sections = [('.data', b'abab', DATA_FLAGS, [])]
        self.symbols = [('_g_x', 0, 1, 0, 2, [])]
        self.native[0x700:0x706] = b'ababab'
        with self.assertRaisesRegex(PlacementError, 'Ambiguous'):
            self.run_place()

    def test_shifted_or_changed_native_bytes_reject(self):
        self.native[0x608:0x608 + len(PAYLOAD)] = bytes(len(PAYLOAD))
        self.native[0x60c:0x60c + len(PAYLOAD)] = PAYLOAD     # only at a 4-aligned RVA now
        with self.assertRaisesRegex(PlacementError, 'alignment'):
            self.run_place()
        self.native[0x60c:0x60c + len(PAYLOAD)] = bytes(len(PAYLOAD))
        changed = bytearray(PAYLOAD); changed[5] ^= 1
        self.native[0x608:0x608 + len(PAYLOAD)] = changed
        with self.assertRaisesRegex(PlacementError, 'absent'):
            self.run_place()

    def test_bss_and_executable_sections_reject(self):
        self.sections = [('.bss', b'', 0xc0400080, [])]
        with self.assertRaisesRegex(PlacementError, 'Uninitialized'):
            self.run_place()
        self.sections = [('.data', PAYLOAD, 0x60500020, [])]
        with self.assertRaisesRegex(PlacementError, 'initialized non-executable'):
            self.run_place()

    def test_low_content_rejects(self):
        self.sections = [('.data', b'*\0', STRING_FLAGS, [])]
        self.symbols = [('??_C@_01FDHG@?$CK?$AA@', 0, 1, 0, 2, [])]
        self.native[0x700:0x702] = b'*\0'
        with self.assertRaisesRegex(PlacementError, 'Too little'):
            self.run_place()

    def test_aliases_are_not_bound(self):
        self.symbols.append(('_g_alias', 8, 1, 0, 2, []))
        result = self.run_place()
        self.assertNotIn('_s_second$S1', result['symbols'])
        self.assertEqual(result['aliases'], ['_g_alias', '_s_second$S1'])
        self.assertEqual(result['symbols']['_g_first']['size'], 8)

    def relocated(self):
        """Pointer at +12 to an independently bound function at RVA 0x1004."""
        raw = bytearray(PAYLOAD); struct.pack_into('<I', raw, 12, 0)
        self.sections = [('.data', bytes(raw), DATA_FLAGS, [(12, 3, 6)])]
        self.symbols.append(('_callee', 0, 0, 32, 2, []))
        struct.pack_into('<I', self.native, 0x608 + 12, 0x401004)

    def test_relocation_needs_an_independent_target(self):
        self.relocated()
        with self.assertRaisesRegex(PlacementError, 'Unresolved'):
            self.run_place()
        result = self.run_place(bound={'_callee': 0x1004})
        self.assertEqual(result['relocations'][0]['basis'], 'independent-binding')
        self.assertEqual(result['symbols']['_s_second$S1']['relocated_fields'], [4])

    def test_wrong_relocation_target_rejects(self):
        self.relocated()
        with self.assertRaisesRegex(PlacementError, 'differs from bound target'):
            self.run_place(bound={'_callee': 0x1008})

    def test_native_highlow_inventory_must_match(self):
        self.relocated()
        reloc = self.native
        struct.pack_into('<II', reloc, 0x98 + 96 + 5 * 8, 0x3000, 12)
        struct.pack_into('<IIHH', reloc, 0x800, 0x2000, 12, 0x3000 | (0x8 + 12), 0)
        self.assertEqual(self.run_place(bound={'_callee': 0x1004})['base_rva'], 0x2008)
        struct.pack_into('<IIHH', reloc, 0x800, 0x2000, 12, 0x3000 | (0x8 + 8), 0)
        with self.assertRaisesRegex(PlacementError, 'HIGHLOW'):
            self.run_place(bound={'_callee': 0x1004})

    def test_partial_and_non_dir32_relocations_reject(self):
        self.symbols.append(('_callee', 0, 0, 32, 2, []))
        for uses in ([(22, 3, 6)], [(12, 3, 0x14)], [(8, 3, 6), (10, 3, 6)]):
            with self.subTest(uses=uses):
                self.sections = [('.data', PAYLOAD, DATA_FLAGS, uses)]
                with self.assertRaisesRegex(PlacementError, 'relocation'):
                    self.run_place(bound={'_callee': 0x1004})

    def test_same_object_string_is_placed_before_its_pointer(self):
        text = b'Hello menu\0'
        raw = bytearray(PAYLOAD); struct.pack_into('<I', raw, 12, 0)
        self.sections = [('.data', bytes(raw), DATA_FLAGS, [(12, 3, 6)]), ('.data', text, STRING_FLAGS, [])]
        self.symbols.append(('??_C@_0L@hello', 0, 2, 0, 2, []))
        self.native[0x680:0x680 + len(text)] = text
        struct.pack_into('<I', self.native, 0x608 + 12, 0x402080)
        self.target.write_bytes(self.native)
        object_file(self.obj, self.sections, self.symbols)
        results = place_object(COFF(self.obj), PE(self.target))
        self.assertEqual([r.get('base_rva') for r in results], [0x2008, 0x2080])
        self.assertEqual(results[0]['relocations'][0]['basis'], 'placed-contribution')
        struct.pack_into('<I', self.native, 0x608 + 12, 0x402084)
        self.target.write_bytes(self.native)
        self.assertIn('differs from bound target', place_object(COFF(self.obj), PE(self.target))[0]['rejected'])


if __name__ == '__main__':
    unittest.main()
