"""Masked comparisons must retain whole-oracle identity and fresh input guards."""
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / 'scripts'))
import similar


class MaskedInputTests(unittest.TestCase):
    def setUp(self):
        parent = (PROJECT_ROOT / 'work/test-fixtures').resolve()
        parent.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix='masked-input-', dir=parent)
        self.root = Path(temporary.name).resolve()
        if not self.root.is_relative_to(parent):
            raise ValueError('Fixture escaped workspace')
        self.addCleanup(temporary.cleanup)
        self.addCleanup(patch.stopall)
        patch.object(similar, 'ROOT', self.root).start()
        self.source = self.root / 'candidate.c'
        self.source.write_text('int sample(void) { return 0; }\n')
        self.target = self.root / 'TEST.DLL'
        self.target.write_bytes(b'synthetic oracle identity, no game code')
        self.row = dict(id='test.sample', target='TEST.DLL', target_sha256=hashlib.sha256(self.target.read_bytes()).hexdigest(),
                        rva='0x1000', size=16, symbol='_sample')

    def test_changed_whole_oracle_rejected_before_compile(self):
        self.target.write_bytes(b'changed outside the function')
        with patch.object(similar, 'compile_source') as compile_mock, self.assertRaisesRegex(ValueError, 'Target hash mismatch'):
            similar.check_rows([self.row], 'candidate.c')
        compile_mock.assert_not_called()

    def test_source_changed_by_compile_rejected(self):
        def mutate(*args):
            self.source.write_text('int changed;\n')
            return self.root / 'fake.obj', []
        with patch.object(similar, 'compile_source', side_effect=mutate), self.assertRaisesRegex(ValueError, 'Source changed during compilation'):
            similar.check_rows([self.row], 'candidate.c')

    def test_conflicting_recorded_oracle_hashes_rejected(self):
        other = dict(self.row, target_sha256='wrong identity')
        with patch.object(similar, 'compile_source') as compile_mock, self.assertRaisesRegex(ValueError, 'Conflicting oracle'):
            similar.check_rows([self.row, other], 'candidate.c')
        compile_mock.assert_not_called()

    def test_summary_rejects_other_modules_map(self):
        import json
        with patch.object(similar, 'STATE', self.root / 'recovery.json'):
            similar.STATE.write_text(json.dumps(dict(functions=[])))
            mapping = self.root / 'map.json'
            mapping.write_text(json.dumps(dict(module='OTHER.DLL', functions=[])))
            with self.assertRaisesRegex(ValueError, 'different module'):
                similar.summary('TEST.DLL', mapping)

    def test_oracle_changed_after_comparison_rejected(self):
        def mutate(*args):
            self.target.write_bytes(b'changed during comparison')
            return dict(masked_equal=True)
        with patch.object(similar, 'compile_source', return_value=(self.root / 'fake.obj', [])), \
             patch.object(similar.sqlite3, 'connect'), \
             patch.object(similar, 'defined_functions', return_value={'_sample'}), \
             patch.object(similar, 'interior_entries', return_value=0), \
             patch.object(similar, 'masked_compare', side_effect=mutate), \
             patch.object(similar, 'masked_identity', return_value='diagnostic'), \
             self.assertRaisesRegex(ValueError, 'Target changed during verification'):
            similar.check_rows([self.row], 'candidate.c')


if __name__ == '__main__':
    unittest.main()
