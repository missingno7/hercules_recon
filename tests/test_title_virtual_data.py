"""Loader-zero data diagnostics must not manufacture missing file bytes."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from title_link import virtual_data


class Image:
    sections = [dict(virtual_address=0x1000, virtual_size=12, raw_size=4)]

    def read_rva(self, rva, size):
        if rva < 0x1000 or rva + size > 0x1004:
            raise ValueError('not file-backed')
        return b'abcd'[rva - 0x1000:rva - 0x1000 + size]


class VirtualDataTests(unittest.TestCase):
    def test_file_bytes_are_preserved(self):
        self.assertEqual(virtual_data(Image(), 0x1001, 3), (b'bcd', 0))

    def test_only_virtual_tail_is_zero_filled(self):
        self.assertEqual(virtual_data(Image(), 0x1002, 5), (b'cd\0\0\0', 3))
        self.assertEqual(virtual_data(Image(), 0x1008, 4), (bytes(4), 4))

    def test_ranges_outside_section_are_rejected(self):
        for rva, size in [(0x0fff, 1), (0x100a, 3), (0x100c, 1), (0x1000, -1)]:
            with self.subTest(rva=rva, size=size), self.assertRaises(ValueError):
                virtual_data(Image(), rva, size)

    def test_truncated_file_is_not_zero_filled(self):
        image = Image()
        image.read_rva = lambda rva, size: b''
        with self.assertRaises(ValueError):
            virtual_data(image, 0x1002, 5)


if __name__ == '__main__':
    unittest.main()
