"""Phase-local evidence reuse must retain fresh mutation checks."""
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
from test_matching import workspace_temporary_directory


class ReferenceSnapshotTests(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('reference-snapshot-')
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.addCleanup(patch.stopall)
        patch.object(match, 'ROOT', self.root).start()
        self.note = self.root / 'identity.txt'
        self.note.write_bytes(b'independent synthetic identity\n')
        self.path = self.root / 'bindings.json'
        self.document = dict(schema=1, evidence_files={'identity.txt': match.digest(self.note.read_bytes())},
                             functions={'alpha': {'symbols': {}}, 'beta': {'symbols': {}}})
        self.write()

    def write(self, path=None, document=None):
        (path or self.path).write_bytes((json.dumps(document or self.document)+'\n').encode())

    def test_shared_document_and_evidence_read_once_per_phase(self):
        reads = []
        original = Path.read_bytes
        def observe(path):
            reads.append(path.resolve())
            return original(path)
        snapshot = match.ReferenceSnapshot()
        with patch.object(Path, 'read_bytes', observe):
            for name in ('alpha', 'beta', 'alpha'):
                match.reference_inputs(self.path, {'id': name}, snapshot=snapshot)
        self.assertEqual(reads.count(self.path.resolve()), 1)
        self.assertEqual(reads.count(self.note.resolve()), 1)

    def test_evidence_shared_by_documents_is_still_checked_against_each_pin(self):
        other = self.root / 'other.json'
        document = copy.deepcopy(self.document)
        document['evidence_files']['identity.txt'] = '0'*64
        self.write(other, document)
        snapshot = match.ReferenceSnapshot()
        snapshot.read(self.path, {'id': 'alpha'})
        with self.assertRaisesRegex(ValueError, 'evidence changed'):
            snapshot.read(other, {'id': 'beta'})

    def test_returned_records_cannot_modify_frozen_metadata(self):
        snapshot = match.ReferenceSnapshot()
        binding, context, observed = snapshot.read(self.path, {'id': 'alpha'})
        binding['symbols']['fake'] = {}
        context['files']['identity.txt'] = 'bad'
        observed.clear()
        again = snapshot.read(self.path, {'id': 'alpha'})
        self.assertEqual(again[0], {'symbols': {}})
        self.assertEqual(again[1]['files'], self.document['evidence_files'])
        self.assertEqual(len(again[2]), 2)

    def test_final_guard_rehashes_evidence_and_document(self):
        for path in (self.note, self.path):
            with self.subTest(path=path.name):
                before = path.read_bytes()
                snapshot = match.ReferenceSnapshot()
                snapshot.read(self.path, {'id': 'alpha'})
                path.write_bytes(before+b'changed')
                # Reusing the phase snapshot is allowed only with the final guard.
                snapshot.read(self.path, {'id': 'beta'})
                with self.assertRaisesRegex(ValueError, 'input changed'):
                    snapshot.assert_unchanged()
                path.write_bytes(before)

    def test_new_phase_and_default_api_never_reuse_an_old_snapshot(self):
        match.reference_inputs(self.path, {'id': 'alpha'})
        self.note.write_bytes(b'changed')
        for snapshot in (None, match.ReferenceSnapshot()):
            with self.subTest(snapshot=snapshot), self.assertRaisesRegex(ValueError, 'evidence changed'):
                match.reference_inputs(self.path, {'id': 'beta'}, snapshot=snapshot)

    def test_recovery_ledger_still_cannot_supply_independent_reference_evidence(self):
        state = self.root / 'recovery.json'
        state.write_bytes(b'{}')
        self.document['evidence_files'] = {'recovery.json': match.digest(state.read_bytes())}
        self.write()
        with self.assertRaisesRegex(ValueError, 'independent of recovery state'):
            match.reference_inputs(self.path, {'id': 'alpha'})


if __name__ == '__main__':
    unittest.main()
