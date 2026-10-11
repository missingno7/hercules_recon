"""Adversarial shared-text diagnostics with synthetic, unexecuted fixtures."""
import copy
from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import masked


class UnitDiagnosticTests(unittest.TestCase):
    def setUp(self):
        self.code = b'\xb8\0\0\0\0\xc3' * 2
        self.native = b'\xb8' + struct.pack('<I', 0x402000) + b'\xc3\xb8' + struct.pack('<I', 0x402004) + b'\xc3'
        self.data = b'one\0two\0'
        self.object = SimpleNamespace(sections=[
            dict(name='.text', flags=0x60500020, size=12, data=self.code, relocations=[
                dict(offset=1, type=masked.DIR32, symbol='_literal0'),
                dict(offset=7, type=masked.DIR32, symbol='_literal1')]),
            dict(name='.data', flags=0xc0300040, size=8, data=self.data, relocations=[])
        ], symbols={
            0: dict(name='_first', section=1, value=0, type=0x20),
            1: dict(name='_second', section=1, value=6, type=0x20),
            2: dict(name='_literal0', section=2, value=0, type=0),
            3: dict(name='_literal1', section=2, value=4, type=0)
        })
        self.pe = SimpleNamespace(image_base=0x400000,
            sections=[dict(virtual_address=0x1000, virtual_size=12, raw_size=12, characteristics=0x60000020),
                      dict(virtual_address=0x2000, virtual_size=8, raw_size=8, characteristics=0xc0000040)],
            pe=SimpleNamespace(OPTIONAL_HEADER=SimpleNamespace(DATA_DIRECTORY=[SimpleNamespace(Size=0)] * 6)))
        self.pe.read_rva = lambda rva, size: (self.native if rva == 0x1000 else self.data)[:size]

    def compare(self, **kwargs):
        with patch.object(masked, 'COFF', return_value=self.object), patch.object(masked, 'PE', return_value=self.pe):
            return masked.masked_unit_compare('synthetic.obj', 'synthetic.exe', kwargs.get('rva', 0x1000),
                                              kwargs.get('size', 12), kwargs.get('functions', {'_first': 0, '_second': 6}))

    def test_complete_code_and_literal_run_equal(self):
        result = self.compare()
        self.assertTrue(result['unit_equal'])
        self.assertEqual(result['function_offsets'], {'_first': 0, '_second': 6})
        self.assertEqual(result['mapped_data'][0]['rva'], '0x2000')

    def test_function_offsets_cannot_be_guessed_or_omitted(self):
        for inventory in ({'_first': 0}, {'_first': 0, '_second': 5}, {'_first': 0, '_second': 6, '_extra': 11}):
            with self.assertRaisesRegex(ValueError, 'inventory/offsets'):
                self.compare(functions=inventory)

    def test_extra_code_or_comdat_cannot_be_sliced_away(self):
        baseline = copy.deepcopy(self.object.sections)
        self.object.sections.append(dict(name='.other', flags=0x60500020, size=1, data=b'\xc3', relocations=[]))
        with self.assertRaisesRegex(ValueError, 'non-COMDAT'):
            self.compare()
        self.object.sections = baseline
        self.object.sections[0]['flags'] |= 0x1000
        with self.assertRaisesRegex(ValueError, 'non-COMDAT'):
            self.compare()

    def test_code_field_cannot_extend_outside_contribution(self):
        self.object.sections[0]['relocations'][0]['offset'] = 10
        with self.assertRaisesRegex(ValueError, 'relocation exceeds'):
            self.compare()

    def test_literals_must_share_the_compiled_section_base(self):
        self.native = self.native[:7] + struct.pack('<I', 0x402005) + self.native[11:]
        result = self.compare()
        self.assertTrue(result['masked_equal'])
        self.assertFalse(result['unit_equal'])
        self.assertEqual(result['section_conflicts'][0]['reason'], 'defined section bases differ')

    def test_literal_bytes_and_unmapped_data_are_not_ignored(self):
        self.data = b'changed!'
        self.assertFalse(self.compare()['unit_equal'])
        self.data = b'one\0two\0'
        self.object.sections.append(dict(name='.bss', flags=0xc0400080, size=4, data=b'', relocations=[]))
        result = self.compare()
        self.assertFalse(result['unit_equal'])
        self.assertEqual(result['unmapped_sections'][0]['name'], '.bss')

    def test_target_interval_must_be_complete_file_backed_code(self):
        with self.assertRaisesRegex(ValueError, 'file-backed executable'):
            self.compare(size=13)

    def test_unsupported_relocations_do_not_equal(self):
        self.object.sections[0]['relocations'][0]['type'] = masked.DIR32NB
        result = self.compare()
        self.assertFalse(result['unit_equal'])
        self.assertEqual(len(result['unsupported']), 1)

    def test_internal_reference_must_imply_the_complete_text_base(self):
        self.object.symbols[2].update(section=1, value=6)
        self.native = self.native[:1] + struct.pack('<I', 0x401006) + self.native[5:]
        self.assertTrue(self.compare()['unit_equal'])
        self.native = self.native[:1] + struct.pack('<I', 0x401007) + self.native[5:]
        result = self.compare()
        self.assertFalse(result['unit_equal'])
        self.assertEqual(result['conflicts'][0]['reason'], 'own section')

    def test_native_absolute_relocation_inventory_must_agree(self):
        self.pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size = 8
        self.pe.pe.DIRECTORY_ENTRY_BASERELOC = [SimpleNamespace(entries=[
            SimpleNamespace(rva=0x1001, type=3)])]
        result = self.compare()
        self.assertFalse(result['unit_equal'])
        self.assertEqual(result['conflicts'][0]['reason'],
                         'absolute relocation offsets differ from target base relocations')


if __name__ == '__main__':
    unittest.main()
