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
    """Symbol table keyed by raw index, as coff.COFF builds it: aux records advance the index."""

    def __init__(self, path, payload=b'\x90'):
        records = aux(path)
        self.sections = [dict(name='.text', data=payload, relocations=[dict(offset=1, symbol='_f', type=20)])]
        self.symbols = {0: dict(name='.file', storage=103, aux=records),
                        1 + len(records): dict(name='_f', storage=2, aux=[])}


def portable(root, coff):
    with mock.patch.object(match, 'ROOT', Path(root)), mock.patch.object(match, 'COFF', lambda obj: coff):
        return match.portable_unit_inventory(None)


class PortableInventory(unittest.TestCase):
    def test_checkout_location_and_index_shift_do_not_change_the_hash(self):
        short = portable('C:/r', FakeCoff('C:\\r\\src\\a.c'))
        long = portable('D:/a/very/long/clone/path/here', FakeCoff('D:\\a\\very\\long\\clone\\path\\here\\src\\a.c'))
        self.assertEqual(short, long)

    def test_content_and_relative_path_still_count(self):
        base = portable('C:/r', FakeCoff('C:\\r\\src\\a.c'))
        self.assertNotEqual(base, portable('C:/r', FakeCoff('C:\\r\\src\\a.c', payload=b'\xc3')))
        self.assertNotEqual(base, portable('C:/r', FakeCoff('C:\\r\\src\\b.c')))

    def test_file_record_outside_repository_refuses(self):
        with self.assertRaises(ValueError):
            portable('C:/r', FakeCoff('C:\\elsewhere\\a.c'))

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
