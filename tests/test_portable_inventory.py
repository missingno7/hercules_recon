import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match  # noqa: E402


def aux(text):
    data = text.encode('ascii') + b'\0'
    data = data.ljust((len(data) + 17) // 18 * 18, b'\0')
    return [data[i:i + 18] for i in range(0, len(data), 18)]


class FakeCoff:
    def __init__(self, path, payload=b'\x90'):
        self.sections = [dict(name='.text', data=payload)]
        self.symbols = {0: dict(name='.file', storage=103, aux=aux(path)),
                        2: dict(name='_f', storage=2, aux=[])}


class PortableInventory(unittest.TestCase):
    def portable(self, root, coff):
        with mock.patch.object(match, 'ROOT', Path(root)), mock.patch.object(match, 'COFF', lambda obj: coff):
            return match.portable_unit_inventory(None)

    def test_checkout_location_does_not_change_the_hash(self):
        short = self.portable('C:/r', FakeCoff('C:\\r\\src\\a.c'))
        long = self.portable('D:/some/much/longer/clone', FakeCoff('D:\\some\\much\\longer\\clone\\src\\a.c'))
        self.assertEqual(short, long)

    def test_content_and_relative_path_still_count(self):
        base = self.portable('C:/r', FakeCoff('C:\\r\\src\\a.c'))
        self.assertNotEqual(base, self.portable('C:/r', FakeCoff('C:\\r\\src\\a.c', payload=b'\xc3')))
        self.assertNotEqual(base, self.portable('C:/r', FakeCoff('C:\\r\\src\\b.c')))

    def test_file_record_outside_repository_refuses(self):
        with self.assertRaises(ValueError):
            self.portable('C:/r', FakeCoff('C:\\elsewhere\\a.c'))

    def test_recorded_portable_hash_is_what_validation_checks(self):
        row = dict(id='m.a', proof=dict(unit_inventory_sha256='0' * 64, context_sha256={'x': 'y'},
                                        unit_inventory_portable_sha256='p' * 64))
        with mock.patch.object(match, 'context_proof', lambda c: {'x': 'y'}), \
                mock.patch.object(match, 'portable_unit_inventory', lambda obj: 'p' * 64), \
                mock.patch.object(match, 'unit_inventory', side_effect=AssertionError('absolute hash not used')):
            match.validate_unit_proof(row, {}, obj=object())
        with mock.patch.object(match, 'context_proof', lambda c: {'x': 'y'}), \
                mock.patch.object(match, 'portable_unit_inventory', lambda obj: 'q' * 64):
            with self.assertRaises(ValueError):
                match.validate_unit_proof(row, {}, obj=object())


if __name__ == '__main__':
    unittest.main()
