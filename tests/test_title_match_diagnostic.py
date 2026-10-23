"""Mixed-unit diagnostics retain failures and reject stale build inputs."""
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import title_match_diagnostic as diagnostic
from match import digest
from test_matching import make_coff, make_pe, CODE, workspace_temporary_directory


class TitleMatchDiagnosticTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('title-diagnostic-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / 'unit.c'
        self.source.write_bytes(b'int sample(void) { return 0; }\n')
        self.obj = self.root / 'unit.obj'
        make_coff(self.obj, extra_function=True, extra_data=True)
        self.target = self.root / 'TITLE.DLL'
        make_pe(self.target)
        self.row = dict(id='title.sample', source='unit.c', target='TITLE.DLL',
                        target_sha256=digest(self.target.read_bytes()), flags=['/O2'],
                        rva='0x1000', size=len(CODE), symbol='_sample', status='FUNCTION_MATCH',
                        proof={'source_sha256': digest(self.source.read_bytes())})
        (self.root / 'recovery.json').write_text(json.dumps({'functions': [self.row]}))
        self.report = dict(functions=[dict(source='unit.c', symbol='_sample')],
                           compiled_sources=[dict(source='unit.c', command=['cl', '/Fo'+str(self.obj), str(self.source)],
                                                  object_sha256=digest(self.obj.read_bytes()),
                                                  compiler_input_sha256=digest(self.source.read_bytes()))],
                           dll_sha256='synthetic-linked-image')
        self.report_path = self.root / 'report.json'
        self.report_path.write_text(json.dumps(self.report))
        self.addCleanup(patch.stopall)
        patch.object(diagnostic, 'ROOT', self.root).start()
        patch.object(diagnostic, 'load_report', return_value=(self.report, self.target, '')).start()

    def test_inventories_extra_members_and_data_without_promoting(self):
        before = (self.root / 'recovery.json').read_bytes()
        result, _ = diagnostic.audit(self.report_path)
        self.assertEqual(result['summary']['raw_equal'], 1)
        self.assertEqual(result['summary']['emitted_function_symbols'], 2)
        self.assertEqual(len(result['units'][0]['sections']), 3)
        self.assertEqual((self.root / 'recovery.json').read_bytes(), before)

    def test_failed_member_is_reported_not_excluded(self):
        self.row['size'] = 3
        (self.root / 'recovery.json').write_text(json.dumps({'functions': [self.row]}))
        result, _ = diagnostic.audit(self.report_path)
        self.assertEqual(result['summary']['raw_regressions'], ['title.sample'])
        self.assertEqual(len(result['units'][0]['outcomes']), 1)

    def test_unrecorded_member_retains_provisional_extent(self):
        self.report['functions'].append(dict(source='unit.c', symbol='_extra', rva='0x1000', inventory_size=len(CODE)))
        result, _ = diagnostic.audit(self.report_path)
        self.assertEqual(result['summary']['unrecorded_leads'], 1)
        lead = result['units'][0]['unrecorded_leads'][0]
        self.assertTrue(lead['masked_equal'])
        self.assertTrue(lead['target_extent_provisional'])

    def test_stale_object_source_and_oracle_are_rejected(self):
        for path in (self.obj, self.source, self.target):
            before = path.read_bytes()
            path.write_bytes(before + b'changed')
            with self.subTest(path=path.name), self.assertRaises(ValueError):
                diagnostic.audit(self.report_path)
            path.write_bytes(before)


if __name__ == '__main__':
    unittest.main()
