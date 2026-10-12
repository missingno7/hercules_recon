"""Permission and loader-boundary regressions for measured ENG1 data."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from eng1_data_evidence import CheckError, native_bytes


class Image:
    def __init__(self, flags=0xc0000040, raw=None):
        self.raw = bytes(32) if raw is None else raw
        self.sections = [dict(virtual_address=0x1000, virtual_size=64,
                              raw_size=32, characteristics=flags)]

    def read_rva(self, rva, size):
        return self.raw[rva-0x1000:rva-0x1000+size]


class NativeDataContract(unittest.TestCase):
    def field(self, **options):
        return dict(rva='0x1018', size=16, **options)

    def test_mixed_storage_requires_explicit_zero_equivalence(self):
        with self.assertRaises(CheckError):
            native_bytes(Image(), self.field())

    def test_mixed_zero_storage_has_exact_loaded_width(self):
        value, kind, _ = native_bytes(Image(), self.field(allow_compiled_zero_fill=True))
        self.assertEqual((value, kind), (bytes(16), 'mixed_zero'))

    def test_mixed_nonzero_storage_cannot_be_common_zero(self):
        with self.assertRaises(CheckError):
            native_bytes(Image(raw=bytes(31)+b'\x01'), self.field(allow_compiled_zero_fill=True))

    def test_readonly_data_requires_explicit_classification(self):
        with self.assertRaises(CheckError):
            native_bytes(Image(flags=0x60000020), dict(rva='0x1000', size=16))

    def test_readonly_native_table_is_data_not_copied_code(self):
        value, kind, _ = native_bytes(Image(flags=0x60000020),
            dict(rva='0x1000', size=16, storage='read_only_table'))
        self.assertEqual((value, kind), (bytes(16), 'initialized'))

    def test_readonly_classification_rejects_writable_native(self):
        with self.assertRaises(CheckError):
            native_bytes(Image(), dict(rva='0x1000', size=16, storage='read_only_table'))

    def test_readonly_classification_requires_read_permission(self):
        with self.assertRaises(CheckError):
            native_bytes(Image(flags=0x20000020),
                dict(rva='0x1000', size=16, storage='read_only_table'))


if __name__ == '__main__':
    unittest.main()
