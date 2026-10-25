"""Diagnostic summaries must retain differences concealed by a percentage."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from objdiff_diagnostic import summarize


class ObjdiffDiagnosticTests(unittest.TestCase):
    def fixture(self):
        function = dict(name='_sample', size='56', match_percent=100.0,
                        instructions=[dict(diff_kind='DIFF_ARG_MISMATCH',
                                           instruction=dict(address='39', formatted='call wrong_target'))])
        return dict(left=dict(symbols=[function]), right=dict(symbols=[copy.deepcopy(function)]))

    def test_hundred_percent_does_not_hide_changed_reference(self):
        summary = summarize(self.fixture(), '_sample', 8)
        self.assertEqual(summary['left']['match_percent'], 100.0)
        self.assertEqual(summary['left']['changes'][0]['instruction'], 'call wrong_target')
        self.assertEqual(summary['left']['changes'][0]['offset'], '0x27')
        self.assertEqual(summary['left']['inferred_body_size'], 56)

    def test_bounds_both_sides_and_reports_omissions(self):
        data = self.fixture()
        for side in ('left', 'right'):
            data[side]['symbols'][0]['instructions'] *= 20
        summary = summarize(data, '_sample', 2)
        for side in ('left', 'right'):
            self.assertEqual(len(summary[side]['changes']), 2)
            self.assertEqual(summary[side]['omitted_changes'], 18)

    def test_missing_or_ambiguous_symbol_is_rejected(self):
        for symbols in ([], self.fixture()['left']['symbols'] * 2):
            data = self.fixture()
            data['right']['symbols'] = symbols
            with self.assertRaisesRegex(ValueError, 'one objdiff function'):
                summarize(data, '_sample', 8)

    def test_inserted_instruction_alignment_gap_is_explicit(self):
        data = self.fixture()
        data['left']['symbols'][0]['instructions'] = [dict(diff_kind='DIFF_INSERT')]
        row = summarize(data, '_sample', 8)['left']['changes'][0]
        self.assertIsNone(row['offset'])
        self.assertEqual(row['instruction'], '<alignment gap>')


if __name__ == '__main__':
    unittest.main()
