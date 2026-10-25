"""Bounded checks for the recorded census diagnostic object guard."""
import hashlib
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / 'scripts'))
sys.path.insert(0, str(PROJECT_ROOT / 'tests'))
import census
from test_matching import CODE, make_coff, make_pe, workspace_temporary_directory


def digest(data):
    return hashlib.sha256(data).hexdigest()


class RecordedCensusTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('recorded-census-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.addCleanup(patch.stopall)
        patch.object(census, 'ROOT', self.root).start()
        patch.object(census, 'verify_toolchain', return_value='fixture-toolchain').start()

        self.source = self.root / 'source.c'
        self.source.write_bytes(b'int sample(void) { return 0; }\n')
        self.target = self.root / 'HERCULES.EXE'
        make_pe(self.target)
        self.obj = self.root / 'candidate.obj'
        make_coff(self.obj)
        self.command = ['cl.exe', '/nologo', '/c', str(self.source)]
        patch.object(census, 'compile_source', return_value=(self.obj, self.command)).start()
        self.row = dict(id='sample', status='FUNCTION_MATCH', source='source.c',
                        target='HERCULES.EXE', target_sha256=digest(self.target.read_bytes()),
                        flags=['/O2'], proof={'source_sha256': digest(self.source.read_bytes())},
                        symbol='_sample', rva='0x1000', size=len(CODE))
        self.state = {'functions': [self.row]}
        recovery = b'{"functions":["fixture"]}\n'
        (self.root / 'recovery.json').write_bytes(recovery)
        self.state_sha256 = digest(recovery)

    def run_census(self):
        return census.recorded_census(self.state, self.state_sha256, jobs=1)

    def test_successful_group_retains_stable_object_diagnostics(self):
        report = self.run_census()
        group = report['compile_group_results'][0]
        self.assertEqual(group['object_path'], str(self.obj.resolve()))
        self.assertEqual(group['object_sha256'], digest(self.obj.read_bytes()))
        self.assertEqual(group['object_sha256_after'], group['object_sha256'])
        self.assertTrue(group['object_stable'])
        self.assertEqual(group['command'], self.command)
        self.assertEqual(group['status'], 'compiled')
        self.assertIn('fresh strict match gate', report['scope'])

    def test_object_mutation_during_row_comparison_fails_closed(self):
        original = census.recorded_fields

        def mutate_after_comparison(*args):
            result = original(*args)
            self.obj.write_bytes(self.obj.read_bytes() + b'changed')
            return result

        with patch.object(census, 'recorded_fields', side_effect=mutate_after_comparison):
            report = self.run_census()
        group = report['compile_group_results'][0]
        self.assertEqual(group['status'], 'unsupported')
        self.assertFalse(group['object_stable'])
        self.assertNotEqual(group['object_sha256_after'], group['object_sha256'])
        self.assertEqual(report['rows'][0]['category'], 'unsupported')

    def test_jobs_value_must_be_positive_integer(self):
        self.assertEqual(census.positive_int('2'), 2)
        for value in ('0', '-1', 'many'):
            with self.subTest(value=value), self.assertRaises(census.argparse.ArgumentTypeError):
                census.positive_int(value)


if __name__ == '__main__':
    unittest.main()
