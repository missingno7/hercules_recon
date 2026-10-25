"""Multi-document reference binding and publication snapshot regressions."""
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / 'scripts'))
import linked
import match
import promote


class MultiBindingPromotionTests(unittest.TestCase):
    def setUp(self):
        # Reuse the established synthetic PE/COFF/compiler fixture without
        # inheriting and rerunning its full test suite.
        from test_relocated_acceptance import RelocatedAcceptanceTests
        self.fixture = RelocatedAcceptanceTests('test_verify_only_does_not_publish_a_relocated_status')
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)

    def _document(self, function_ids, destinations=None):
        document = self.fixture.write_bindings(destinations)
        document = copy.deepcopy(document)
        original = document['functions']
        document['functions'] = {}
        for function_id in function_ids:
            document['functions'][function_id] = copy.deepcopy(
                original.get(function_id, next(iter(original.values())))
            )
        return document

    def _write_document(self, path, function_ids, destinations=None):
        document = self._document(function_ids, destinations)
        path.write_text(json.dumps(document, indent=2) + '\n', encoding='ascii')
        return path

    def _two_candidates_and_documents(self):
        alpha = self.fixture.add_relocated_candidate('alpha', 0x1000, 0x1100)
        beta = self.fixture.add_relocated_candidate('beta', 0x1010, 0x1180)
        second_target = self.fixture.root / 'originals/relocated-peer.dll'
        second_target.write_bytes(self.fixture.target.read_bytes())
        beta['target'] = second_target.relative_to(self.fixture.root).as_posix()
        beta['target_sha256'] = match.digest(second_target.read_bytes())
        self.fixture.write_state([alpha, beta])
        alpha_doc = self.fixture.root / 'evidence/alpha-bindings.json'
        beta_doc = self.fixture.root / 'evidence/beta-bindings.json'
        self._write_document(alpha_doc, ['alpha'])
        self._write_document(beta_doc, ['beta'])
        return alpha, beta, alpha_doc, beta_doc

    def test_multi_document_promotion_keeps_paths_and_full_gates(self):
        alpha, beta, alpha_doc, beta_doc = self._two_candidates_and_documents()
        raw = self.fixture.add_raw_peer()
        linked_row = {'id': 'linked-sentinel', 'status': 'LINKED_MATCH', 'proof': {}}
        self.fixture.write_state([raw, alpha, beta, linked_row])
        atomic_bytes = promote.atomic_bytes
        journal_writes = []

        def record_atomic_bytes(path, content):
            journal_writes.append(Path(path).resolve())
            return atomic_bytes(path, content)

        with patch.object(promote, 'verify_rows', wraps=match.verify_rows) as gate, \
             patch.object(promote, 'atomic_bytes', side_effect=record_atomic_bytes), \
             patch.object(linked, 'verify') as linked_gate:
            receipt = promote.promote_relocated(['alpha', 'beta'], [alpha_doc, beta_doc])

        self.assertTrue(all(result['exact'] for result in receipt['results']))
        self.assertEqual({row['id'] for row in gate.call_args.args[0]},
                         {'raw-peer', 'alpha', 'beta'})
        self.assertEqual(gate.call_args.kwargs['relocation_candidates'], {
            'alpha': alpha_doc.relative_to(self.fixture.root).as_posix(),
            'beta': beta_doc.relative_to(self.fixture.root).as_posix(),
        })
        linked_gate.assert_called_once_with(original_root=None)
        self.assertIn(self.fixture.journal.resolve(), journal_writes)
        self.assertFalse(self.fixture.journal.exists())
        accepted = {row['id']: row for row in json.loads(self.fixture.state.read_text())['functions']}
        self.assertEqual(accepted['alpha']['proof']['bindings_path'],
                         alpha_doc.relative_to(self.fixture.root).as_posix())
        self.assertEqual(accepted['beta']['proof']['bindings_path'],
                         beta_doc.relative_to(self.fixture.root).as_posix())
        self.assertNotEqual(accepted['alpha']['target'], accepted['beta']['target'])
        self.assertEqual(accepted['linked-sentinel'], linked_row)

    def test_missing_ambiguous_duplicate_and_unused_documents_reject_before_verification(self):
        _, _, alpha_doc, beta_doc = self._two_candidates_and_documents()
        duplicate = self.fixture.root / 'evidence/duplicate.json'
        unused = self.fixture.root / 'evidence/unused.json'
        self._write_document(duplicate, ['alpha', 'beta'])
        self._write_document(unused, ['unrequested'])

        invalid_calls = [
            (['alpha', 'beta'], [alpha_doc], 'No reference binding document covers'),
            (['alpha', 'beta'], [duplicate, beta_doc], 'Multiple reference binding documents cover'),
            (['alpha'], [alpha_doc, alpha_doc], 'Duplicate reference binding document path'),
            (['alpha', 'beta'], [alpha_doc, beta_doc, unused],
             'Every supplied reference binding document must cover'),
        ]
        with patch.object(promote, 'verify_rows') as gate:
            for ids, documents, message in invalid_calls:
                with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                    promote.promote_relocated(ids, documents)
            gate.assert_not_called()
        self.fixture.compiler.assert_not_called()

    def test_publication_snapshot_rejects_mutation_after_reference_reads(self):
        _, _, alpha_doc, beta_doc = self._two_candidates_and_documents()
        original_assert = match.ReferenceSnapshot.assert_unchanged
        note = self.fixture.reference_note

        def mutate_then_rehash(snapshot):
            note.write_bytes(b'mutated after publication reference reads\n')
            return original_assert(snapshot)

        before = self.fixture.state.read_bytes()
        with patch.object(match.ReferenceSnapshot, 'assert_unchanged', mutate_then_rehash):
            with self.assertRaisesRegex(ValueError, 'Independent reference input changed'):
                promote.promote_relocated(['alpha', 'beta'], [alpha_doc, beta_doc])
        self.assertEqual(self.fixture.state.read_bytes(), before)
        self.assertFalse(self.fixture.journal.exists())

    def test_single_binding_path_remains_backward_compatible(self):
        alpha = self.fixture.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.fixture.write_state([alpha])
        self.fixture.write_bindings()

        promote.promote_relocated(['alpha'], self.fixture.bindings_path)

        accepted = self.fixture.accepted_row('alpha')
        self.assertEqual(accepted['status'], 'FUNCTION_MATCH_RELOCATED')
        self.assertEqual(accepted['proof']['bindings_path'],
                         self.fixture.bindings_path.relative_to(self.fixture.root).as_posix())


if __name__ == '__main__':
    unittest.main()
