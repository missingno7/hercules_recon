"""Raw instruction equality alone must not invent the named callee's identity."""
import struct
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from linked_compare import verify_direct_call_region


class DirectCallRegionTests(unittest.TestCase):
    def setUp(self):
        self.actual = b'\xe8' + struct.pack('<i', 0x1000-5) + b'\xc3' + b'\x90'*10
        self.section = dict(size=16, data=b'\xe8\0\0\0\0'+self.actual[5:],
                            relocations=[dict(offset=1, type=0x14, symbol='_callee')])

    def check(self, original=None, linked=None, actual=None):
        return verify_direct_call_region(self.section, actual or self.actual, self.actual, 0x1000,
            {'_callee': 0x2000} if original is None else original,
            {'_callee': 0x2000} if linked is None else linked)

    def test_complete_raw_code_and_independent_destinations_agree(self):
        self.assertEqual(self.check(), [dict(offset=1, symbol='_callee', destination_rva='0x2000')])

    def test_identical_bytes_cannot_certify_wrong_or_missing_callee(self):
        for native, linked in [({}, {}), ({'_callee':0x3000},{'_callee':0x2000}),
                               ({'_callee':0x2000},{'_callee':0x3000})]:
            with self.subTest(native=native, linked=linked), self.assertRaises(ValueError):
                self.check(native, linked)

    def test_padding_and_relocation_coverage_are_not_ignored(self):
        with self.assertRaises(ValueError):
            self.check(actual=self.actual[:-1]+b'\xcc')
        self.section['relocations'] = []
        with self.assertRaises(ValueError):
            self.check()

    def test_nonzero_addend_is_not_silently_rebound(self):
        self.section['data'] = b'\xe8\x01\0\0\0'+self.actual[5:]
        with self.assertRaises(ValueError):
            self.check()


if __name__ == '__main__':
    unittest.main()
