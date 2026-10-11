import sys
import unittest
from unittest.mock import patch
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from residual import instruction_rows, mismatch_islands
import leaf

SCRATCH = Path(__file__).resolve().parents[1] / 'work/test_residual'


class ResidualTests(unittest.TestCase):
    def test_relocated_address_does_not_mask_other_immediate(self):
        a = bytes.fromhex('c7050000000010000000')
        b = bytes.fromhex('c7057856341211000000')
        ka = instruction_rows(a, 0, {2})[0][1]
        kb = instruction_rows(b, 0, {2})[0][1]
        self.assertIn('[R], 0x10', ka)
        self.assertIn('[R], 0x11', kb)
        self.assertNotEqual(ka, kb)

    def test_zero_displacement_keeps_register_and_index_scale(self):
        a = bytes.fromhex('8a140500000000')  # [eax + R], scale one
        b = bytes.fromhex('8a148500000000')  # [eax*4 + R]
        c = bytes.fromhex('8a148578563412')
        self.assertNotEqual(instruction_rows(a, 0, {3})[0][1], instruction_rows(b, 0, {3})[0][1])
        self.assertEqual(instruction_rows(b, 0, {3})[0][1], instruction_rows(c, 0, {3})[0][1])

    def test_omitted_zero_displacement_normalizes_to_same_shape(self):
        a = bytes.fromhex('8a8000000000')
        b = bytes.fromhex('8a8078563412')
        ka = instruction_rows(a, 0, {2})[0][1]
        self.assertEqual(ka, instruction_rows(b, 0, {2})[0][1])
        self.assertIn('eax + R', ka)
        self.assertNotIn('\x01', ka)

    def test_constant_load_reorder_is_explained(self):
        a = bytes.fromhex('31ffbe0f000000')
        b = bytes.fromhex('be0f00000031ff')
        r = mismatch_islands(a, b, [[0, 4], [5, 7]])
        self.assertEqual(r['total_islands'], 1)
        self.assertEqual(r['islands'][0]['kind'], 'instruction_order_difference')
        self.assertEqual(r['islands'][0]['decoded_span'], [0, 7])

    def test_register_width_is_preserved(self):
        a = bytes.fromhex('b806000000')
        b = bytes.fromhex('b906000000')
        self.assertEqual(mismatch_islands(a, b, [[0, 1]])['islands'][0]['kind'], 'register_operand_difference')
        c = bytes.fromhex('b006909090')
        self.assertNotEqual(mismatch_islands(a, c, [[0, 5]])['islands'][0]['kind'], 'register_operand_difference')

    def test_undecoded_bytes_never_classified_as_a_reorder(self):
        r = mismatch_islands(b'\x0f', b'\x90', [[0, 1]])
        self.assertEqual(r['islands'][0]['kind'], 'partial_or_undecoded_window')

    def test_output_limit_reports_omissions(self):
        a = bytes.fromhex('b806000000') + b'\x90'*20 + bytes.fromhex('b806000000')
        b = bytes.fromhex('b906000000') + b'\x90'*20 + bytes.fromhex('b906000000')
        r = mismatch_islands(a, b, [[0, 1], [25, 26]], limit=1)
        self.assertEqual(r['omitted_islands'], 1)
        with self.assertRaises(ValueError):
            mismatch_islands(a, b, [[0, 1]], limit=0)

    def test_source_mutation_during_compile_is_refused(self):
        SCRATCH.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=SCRATCH) as directory:
            self.assertTrue(Path(directory).resolve().is_relative_to(SCRATCH.resolve()))
            source = Path(directory) / 'input.c'
            target = Path(directory) / 'target.dll'
            source.write_text('before')
            target.write_bytes(b'oracle')
            def compile_changed(*args):
                source.write_text('after')
                return Path(directory) / 'input.obj', ['compiler']
            with patch.object(leaf.sqlite3, 'connect'), patch.object(leaf, 'module_path', return_value=target), \
                 patch.object(leaf, 'compile_source', side_effect=compile_changed), patch.object(leaf, 'compare', return_value={}):
                with self.assertRaisesRegex(ValueError, 'Source or oracle changed'):
                    leaf.check('TITLE.DLL', 0, 16, '_probe', source, ['/O2'])

    def test_oracle_mutation_during_compile_is_refused(self):
        SCRATCH.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=SCRATCH) as directory:
            self.assertTrue(Path(directory).resolve().is_relative_to(SCRATCH.resolve()))
            source = Path(directory) / 'input.c'
            target = Path(directory) / 'target.dll'
            source.write_text('source')
            target.write_bytes(b'before')
            def compile_changed(*args):
                target.write_bytes(b'after')
                return Path(directory) / 'input.obj', ['compiler']
            with patch.object(leaf.sqlite3, 'connect'), patch.object(leaf, 'module_path', return_value=target), \
                 patch.object(leaf, 'compile_source', side_effect=compile_changed), patch.object(leaf, 'compare', return_value={}):
                with self.assertRaisesRegex(ValueError, 'Source or oracle changed'):
                    leaf.check('TITLE.DLL', 0, 16, '_probe', source, ['/O2'])


if __name__ == '__main__':
    unittest.main()
