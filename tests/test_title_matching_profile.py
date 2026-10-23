"""The matching diagnostic may remove tagged tracing only, never ownership fixes."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from title_behavioral import selected_adaptations, selected_additional_sources, ordered_functions, order_file_bytes, build


class TitleMatchingProfileTests(unittest.TestCase):
    def test_order_file_obeys_measured_vc5_decoration_convention(self):
        self.assertEqual(order_file_bytes(['_title_01000', '__helper']), b'title_01000\n_helper\n')
        with self.assertRaises(ValueError):
            order_file_bytes(['?unmeasured_cpp'])

    def test_order_uses_numeric_native_starts_and_includes_extra_providers(self):
        rows = [dict(rva='0x100', symbol='_later'), dict(rva='0x20', symbol='_first')]
        config = {'functions': [dict(rva='0x30', symbol='_middle')]}
        self.assertEqual(ordered_functions(rows, config), ['_first', '_middle', '_later'])

    def test_order_rejects_ambiguous_or_unsafe_names(self):
        for rows, additions in [
                ([dict(rva='0x20', symbol='_a'), dict(rva='0x20', symbol='_b')], []),
                ([dict(rva='0x20', symbol='_a')], [dict(rva='0x20', symbol='_b')]),
                ([dict(rva='0x20', symbol='_a'), dict(rva='0x30', symbol='_a')], []),
                ([dict(rva='0x20', symbol='_a\n_b')], [])]:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                ordered_functions(rows, {'functions': additions})

    def test_order_cannot_silently_change_behavioral_profile(self):
        with self.assertRaisesRegex(ValueError, 'matching diagnostic'):
            build(Path(__file__).resolve().parents[1] / 'work/unused_order_test', False, True)

    def test_default_preserves_trace_and_matching_profile_preserves_real_edits(self):
        config = {'source_adaptations': {'unit.c': [
            {'before': 'old', 'after': 'correct ownership'},
            {'before': 'call', 'after': 'trace; call', 'diagnostic_only': True},
            {'before': 'trace_name', 'after': 'unrelated trace declaration'}]}}
        before = copy.deepcopy(config)
        self.assertEqual(selected_adaptations(config), config['source_adaptations'])
        self.assertEqual(selected_adaptations(config, True)['unit.c'],
                         [config['source_adaptations']['unit.c'][i] for i in (0, 2)])
        self.assertEqual(config, before)

    def test_only_tagged_support_is_omitted_and_default_is_unchanged(self):
        config = {'additional_sources': [
            {'source': 'game.c'},
            {'source': 'trace.c', 'diagnostic_only': True},
            {'source': 'trace_named_game.c', 'diagnostic_only': False}]}
        before = copy.deepcopy(config)
        self.assertEqual(selected_additional_sources(config), config['additional_sources'])
        self.assertEqual(selected_additional_sources(config, True),
                         [config['additional_sources'][i] for i in (0, 2)])
        self.assertEqual(config, before)


if __name__ == '__main__':
    unittest.main()
