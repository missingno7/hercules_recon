"""Adversarial tests of within-run compile reuse, using synthetic objects."""
import copy
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
from test_matching import make_coff, make_pe, CODE, workspace_temporary_directory


class GroupedMatchingTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('grouped-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'toolchains').mkdir()
        (self.root / 'toolchains/manifest.json').write_bytes(b'fixture')
        self.source = self.root / 'source.c'
        self.source.write_bytes(b'int sample(void) { return 0; }\n')
        self.target = self.root / 'sample.dll'
        self.obj = self.root / 'sample.obj'
        make_pe(self.target)
        make_coff(self.obj)
        self.row = dict(id='first', status='FUNCTION_MATCH', source='source.c', target='sample.dll',
                        target_sha256=match.digest(self.target.read_bytes()), flags=['/O2'],
                        proof={'source_sha256': match.digest(self.source.read_bytes())},
                        symbol='_sample', rva='0x1000', size=len(CODE))
        self.rows = [self.row, dict(copy.deepcopy(self.row), id='second')]
        self.addCleanup(patch.stopall)
        patch.object(match, 'ROOT', self.root).start()
        self.compiler = patch.object(match, 'compile_source', return_value=(self.obj, ['synthetic'])).start()
        patch.object(match, 'verify_toolchain', Mock(return_value='fixture')).start()

    def run_rows(self):
        return match.verify_rows(self.rows, self.root / 'toolchain')

    def test_one_compile_all_comparisons_and_no_cross_run_reuse(self):
        for _ in range(2):
            results, stats = self.run_rows()
            self.assertEqual(stats['compiler_calls'], 1)
            self.assertEqual([r['id'] for r in results], ['first', 'second'])
            self.assertTrue(all(r['exact'] for r in results))
        self.assertEqual(self.compiler.call_count, 2)

    def test_second_row_cannot_hide_a_mismatch(self):
        self.rows[1]['size'] = 3
        results, _ = self.run_rows()
        self.assertTrue(results[0]['exact'])
        self.assertFalse(results[1]['exact'])

    def test_flags_and_paths_separate_groups(self):
        self.rows[1]['flags'] = ['/O1']
        self.assertEqual(self.run_rows()[1]['compiler_calls'], 2)
        other = self.root / 'other.c'
        other.write_bytes(self.source.read_bytes())
        self.rows[1].update(source='other.c', flags=['/O2'])
        self.assertEqual(self.run_rows()[1]['compiler_calls'], 2)

    def test_each_row_source_and_oracle_proof_is_checked(self):
        for field in ('source', 'target'):
            with self.subTest(field=field):
                rows = copy.deepcopy(self.rows)
                if field == 'source':
                    rows[1]['proof']['source_sha256'] = 'bad'
                else:
                    rows[1]['target_sha256'] = 'bad'
                with self.assertRaises(ValueError):
                    match.verify_rows(rows, self.root / 'toolchain')
        self.compiler.assert_not_called()

    def add_headers(self):
        self.source.write_bytes(b'#include"first.h"\nint sample(void) { return 0; }\n')
        (self.root / 'first.h').write_bytes(b'#include "second.h"\n')
        (self.root / 'second.h').write_bytes(b'#define VALUE 0\n')
        for row in self.rows:
            row['proof']['source_sha256'] = match.digest(self.source.read_bytes())

    def test_transitive_headers_are_recorded(self):
        self.add_headers()
        result = self.run_rows()[0][0]
        self.assertEqual(len(result['compile_inputs']), 3)

    def test_change_between_proof_check_and_input_snapshot_rejected(self):
        original = match.compile_inputs
        def mutate(*args):
            self.source.write_bytes(b'int sample(void) { return 1; }\n')
            return original(*args)
        with patch.object(match, 'compile_inputs', side_effect=mutate):
            with self.assertRaisesRegex(ValueError, 'freezing compile inputs'):
                self.run_rows()
        self.compiler.assert_not_called()

    def test_source_header_or_oracle_mutation_during_compile_rejected(self):
        self.add_headers()
        for path in (self.source, self.root / 'second.h', self.target, self.root / 'toolchains/manifest.json'):
            before = path.read_bytes()
            def mutate(*args, **kwargs):
                path.write_bytes(before + b'changed')
                return self.obj, ['synthetic']
            with self.subTest(path=path.name), patch.object(match, 'compile_source', side_effect=mutate):
                with self.assertRaises(ValueError):
                    self.run_rows()
            path.write_bytes(before)

    def test_object_mutation_between_row_checks_rejected(self):
        original = match.compare
        def mutate(*args):
            result = original(*args)
            self.obj.write_bytes(self.obj.read_bytes() + b'x')
            return result
        with patch.object(match, 'compare', side_effect=mutate):
            with self.assertRaisesRegex(ValueError, 'Object changed'):
                self.run_rows()

    def test_macro_include_and_forced_input_modes_fail_closed(self):
        self.source.write_bytes(b'#define HEADER "first.h"\n#include HEADER\n')
        with self.assertRaisesRegex(ValueError, 'Unsupported include'):
            match.compile_inputs(self.source, ['/O2'], self.root)
        self.source.write_bytes(b'int x;\n')
        for flag in ('@flags.rsp', '/FIhidden.h', '/Yucommon.h', '/Irelative'):
            with self.subTest(flag=flag), self.assertRaises(ValueError):
                match.compile_inputs(self.source, [flag], self.root)


if __name__ == '__main__':
    unittest.main()
