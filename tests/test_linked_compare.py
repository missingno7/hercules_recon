"""Actual linked-byte reporting must never turn masking or prefixes into equality."""
import copy
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from linked_compare import compare_function, image_comparison
from pe import PE
from test_matching import make_pe, CODE, workspace_temporary_directory


class LinkedComparisonTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('linked-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.native_path = self.root / 'original.dll'
        self.linked_path = self.root / 'linked.dll'
        make_pe(self.native_path)
        make_pe(self.linked_path)
        self.section = dict(size=len(CODE), data=CODE, relocations=[])

    def run_comparison(self, rva=0x1000, size=16, next_entry=None):
        return compare_function(self.section, PE(self.linked_path), PE(self.native_path), rva, 0x1000, size, next_entry)

    def test_equal_complete_bytes_at_original_rva(self):
        result = self.run_comparison()
        self.assertTrue(result['raw_equal_at_original_rva'])

    def test_displaced_equal_bytes_are_not_original_layout(self):
        data = bytearray(self.linked_path.read_bytes()); data[0x420:0x430] = CODE
        self.linked_path.write_bytes(data)
        result = self.run_comparison(rva=0x1020)
        self.assertTrue(result['raw_equal'])
        self.assertFalse(result['raw_equal_at_original_rva'])

    def test_target_prefix_cannot_hide_extra_contribution(self):
        result = self.run_comparison(size=3)
        self.assertFalse(result['raw_equal'])
        self.assertEqual(result['category'], 'size differs')
        self.assertEqual(result['different_bytes'], 13)

    def test_reference_only_difference_stays_raw_mismatch(self):
        code = b'\xa1' + struct.pack('<I', 0x402000) + b'\xc3' + b'\x90' * 10
        for path, address in ((self.native_path, 0x402000), (self.linked_path, 0x402010)):
            data = bytearray(path.read_bytes()); data[0x400:0x410] = code
            struct.pack_into('<I', data, 0x401, address); path.write_bytes(data)
        self.section = dict(size=16, data=code, relocations=[dict(offset=1, type=6, symbol='_data')])
        result = self.run_comparison()
        self.assertFalse(result['raw_equal'])
        self.assertEqual(result['category'], 'reference operands only')
        self.assertEqual(result['different_nonreference_bytes'], 0)
        self.assertEqual(result['references'][0]['destination_delta'], 16)

    def test_bad_mapping_or_changed_linked_body_is_rejected(self):
        data = bytearray(self.linked_path.read_bytes()); data[0x400] ^= 1
        self.linked_path.write_bytes(data)
        with self.assertRaisesRegex(ValueError, 'outside relocations'):
            self.run_comparison()

    def test_extent_crossing_next_function_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'overlaps another'):
            self.run_comparison(next_entry=0x1008)

    def test_section_report_keeps_file_padding_differences(self):
        data = bytearray(self.linked_path.read_bytes()); data[0x5ff] ^= 1
        self.linked_path.write_bytes(data)
        report = image_comparison(PE(self.linked_path), PE(self.native_path))
        self.assertFalse(report['raw_file_equal'])
        text = next(r for r in report['sections'] if r['name'] == '.text')
        self.assertEqual(text['different_bytes'], 1)


if __name__ == '__main__':
    unittest.main()
