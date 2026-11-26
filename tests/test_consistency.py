import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import consistency as c  # noqa: E402

M = 'X.DLL'


class FakeImage:
    """64 KB image: code 0x1000..0x2000, file data 0x3000..0x3100, zero tail 0x3100..0x4000."""

    def __init__(self):
        self.base = 0x10000000
        self.relocated = True
        self.data = bytearray(0x5000)
        self.fixups = set()
        self.imports = {0x3080: 'Sleep'}

    def read(self, rva, size):
        return bytes(self.data[rva:rva + size])

    def zone(self, rva, size):
        if 0x1000 <= rva and rva + size <= 0x2000:
            return 'code'
        if 0x3000 <= rva and rva + size <= 0x3100:
            return 'file'
        if 0x3100 <= rva and rva + size <= 0x4000:
            return 'zero'
        return None


def ref(name, base, use='direct', width=4, addend=0, function=False, key=None):
    return dict(key=key or (M, name), name=name, base=base, addend=addend, use=use, width=width,
                function=function, defined=False, common=0)


class Rules(unittest.TestCase):
    def setUp(self):
        self.image = FakeImage()
        self.ev = c.Evidence({M: self.image})

    def row(self, rid, refs, body=None, accepted=False):
        start = 0x1000 + 0x20 * len(self.ev.rows)   # the fixture places disjoint rows
        self.ev.rows[rid] = dict(refs=refs, body=body or rid, native=body or rid, start=start, size=16, module=M, accepted=accepted)

    def grades(self):
        return c.evaluate(self.ev)

    def test_two_witnesses_give_grade_a_and_one_gives_b(self):
        self.row('a', [ref('_g', 0x3200)])
        self.row('b', [ref('_g', 0x3200)])
        self.row('c', [ref('_h', 0x3300)])
        out = self.grades()
        self.assertEqual(out['grades'], {'a': 'A', 'b': 'A', 'c': 'B'})

    def test_identical_bodies_are_one_witness(self):
        self.row('a', [ref('_g', 0x3200)], body='same')
        self.row('b', [ref('_g', 0x3200)], body='same')
        self.assertEqual(self.grades()['grades'], {'a': 'B', 'b': 'B'})

    def test_one_symbol_at_two_addresses_fails_both(self):
        self.row('a', [ref('_g', 0x3200)])
        self.row('b', [ref('_g', 0x3204)])
        out = self.grades()
        self.assertEqual(out['grades'], {})
        self.assertIn('X.DLL | _g | several addresses', out['conflicts'])

    def test_accepted_anchor_blames_only_the_disagreeing_row(self):
        self.ev.anchors[(M, '_g')] = (0x3200, 4)
        self.row('ok1', [ref('_g', 0x3200)], accepted=True)
        self.row('bad', [ref('_g', 0x3210)])
        self.row('good', [ref('_g', 0x3200)])
        out = self.grades()
        self.assertEqual(out['grades'], {'good': 'A'})
        self.assertEqual(out['accepted_disagreement'], [])

    def test_two_names_at_one_address_fail(self):
        self.row('a', [ref('_g', 0x3200)])
        self.row('b', [ref('_alias', 0x3200)])
        out = self.grades()
        self.assertEqual(out['grades'], {})
        self.assertTrue(any('shares storage' in x for x in out['conflicts']))

    def test_overlapping_store_extents_fail(self):
        self.row('a', [ref('_wide', 0x3200, width=8)])
        self.row('b', [ref('_inner', 0x3204, width=4)])
        self.assertEqual(self.grades()['grades'], {})

    def test_negative_direct_addend_fails(self):
        self.row('a', [ref('_g', 0x3200, addend=-4)])
        self.assertEqual(self.grades()['grades'], {})

    def test_file_data_needs_a_definition_or_anchor(self):
        self.row('a', [ref('_t', 0x3010)])
        self.row('b', [ref('_t', 0x3010)])
        self.assertEqual(self.grades()['grades'], {})
        self.ev.definitions[(M, '_t')].append(dict(size=4, payload=b'\x07\0\0\0', refs=[]))
        self.image.data[0x3010] = 7
        self.assertEqual(set(self.grades()['grades'].values()), {'A'})

    def test_definition_payload_must_equal_image(self):
        self.ev.definitions[(M, '_t')].append(dict(size=4, payload=b'\x07\0\0\0', refs=[]))
        self.image.data[0x3010] = 8
        self.row('a', [ref('_t', 0x3010)])
        self.row('b', [ref('_t', 0x3010)])
        out = self.grades()
        self.assertEqual(out['grades'], {})
        self.assertIn('X.DLL | _t | payload differs', out['defects'])

    def test_payload_pointer_binds_its_target(self):
        pointer = dict(offset=0, name='_target', key=(M, '_target'), function=False, defined=False,
                       section=0, value=0, common=0)
        self.ev.definitions[(M, '_table')].append(dict(size=4, payload=b'\0\0\0\0', refs=[pointer]))
        self.image.data[0x3020:0x3024] = struct.pack('<I', self.image.base + 0x3300)
        self.image.fixups = {0x3020}
        self.row('a', [ref('_table', 0x3020, use='value')])
        self.row('b', [ref('_table', 0x3020, use='value')])
        self.row('c', [ref('_target', 0x3300)])
        # The pointer-only table proves nothing by itself: its target needs two witnesses for A.
        self.assertEqual(self.grades()['grades'], {'a': 'B', 'b': 'B', 'c': 'B'})
        self.row('c2', [ref('_target', 0x3300)])
        self.assertEqual(self.grades()['grades'], {'a': 'A', 'b': 'A', 'c': 'A', 'c2': 'A'})
        self.row('d', [ref('_target', 0x3304)])   # contradicts the table's pointer: every user falls
        out = self.grades()
        self.assertEqual(out['grades'], {})
        self.assertIn('X.DLL | _target | several addresses', out['conflicts'])

    def test_payload_fixups_must_match(self):
        self.ev.definitions[(M, '_t')].append(dict(size=4, payload=b'\x07\0\0\0', refs=[]))
        self.image.data[0x3010] = 7
        self.image.fixups = {0x3010}
        self.row('a', [ref('_t', 0x3010)])
        self.assertEqual(self.grades()['grades'], {})

    def test_callee_must_be_instruction_equal_at_that_address(self):
        self.ev.functions[(M, '_f')] = [(0x1100, 'f', True)]
        self.row('a', [ref('_f', 0x1100, use='branch', function=True)])
        self.assertEqual(self.grades()['grades'], {'a': 'A'})
        self.ev.functions[(M, '_f')] = [(0x1100, 'f', False)]
        self.assertEqual(self.grades()['grades'], {})

    def test_callee_under_another_name_fails(self):
        self.ev.starts[(M, 0x1100)] = 'other'
        self.row('a', [ref('_f', 0x1100, use='branch', function=True)])
        self.assertEqual(self.grades()['grades'], {})

    def test_import_slot_must_name_the_import(self):
        self.row('a', [ref('__imp__Sleep@4', 0x3080, use='indexed')])
        self.assertEqual(self.grades()['grades'], {'a': 'A'})
        self.ev.rows.clear()
        self.row('a', [ref('__imp__Beep@8', 0x3080, use='indexed')])
        self.assertEqual(self.grades()['grades'], {})

    def test_code_zone_data_fails(self):
        self.row('a', [ref('_g', 0x1100)])
        self.row('b', [ref('_g', 0x1100)])
        self.assertEqual(self.grades()['grades'], {})

    def test_weak_rows_fall_together(self):
        # b is B only through _h; a's other symbol is witnessed only by b, so a must also fall to B.
        self.row('a', [ref('_g', 0x3200)])
        self.row('b', [ref('_g', 0x3200), ref('_h', 0x3300)])
        out = self.grades()
        self.assertEqual(out['grades'], {'a': 'B', 'b': 'B'})


class Listing(unittest.TestCase):
    def test_sizes(self):
        text = '\n'.join([
            '_BSS\tSEGMENT',
            '_a DD 01H DUP (?)',
            '_b DB 010H DUP (?)',
            '_BSS\tENDS',
            '_DATA\tSEGMENT',
            "$SG1\tDB\t'it''s', 00H",
            '\tORG $+3',
            '_t\tDD\t01H',
            '\tDD\tFLAT:_x',
            '\tDW\t02H, 03H',
            '_DATA\tENDS',
            'CONST\tSEGMENT',
            "??_C@_03X@abc?$AA@ DB 'abc', 00H\t\t\t; `string'",
            'CONST\tENDS'])
        self.assertEqual(c.listing_sizes(text), {'_a': 4, '_b': 16, '$SG1': 8, '_t': 12, '??_C@_03X@abc?$AA@': 4})

    def test_duplicate_names_are_unknown(self):
        self.assertIsNone(c.listing_sizes('_DATA\tSEGMENT\n_a DD 01H\n_a DD 01H\n_DATA\tENDS')['_a'])

    def test_import_name(self):
        self.assertEqual(c.import_name('__imp__Sleep@4'), 'Sleep')
        self.assertIsNone(c.import_name('__imp_?f@@YAXXZ'))


if __name__ == '__main__':
    unittest.main()
