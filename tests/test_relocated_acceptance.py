"""Acceptance and publication checks for independently bound relocated functions.

All PE and COFF inputs are generated, synthetic files. The tests exercise the
real comparator and recovery transaction; only the pinned compiler invocation
and its toolchain identity are replaced by fixture objects.
"""
import copy
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import Mock, patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / 'scripts'))
import match
import masked
import promote
from test_matching import CODE, make_coff, make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file


def file_digest(path):
    return match.digest(Path(path).read_bytes())


def call_bytes(rva, destination):
    return b'\xe8' + struct.pack('<i', destination - (rva + 5)) + b'\xc3'


class RelocatedAcceptanceTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('relocated-acceptance-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        for directory in ('src', 'originals', 'evidence', 'toolchains', 'work', 'objects'):
            (self.root / directory).mkdir(parents=True, exist_ok=True)
        (self.root / 'toolchains/manifest.json').write_text('{}\n', encoding='ascii')
        self.header = self.root / 'src/shared.h'
        self.header.write_text('/* synthetic compiler input */\n', encoding='ascii')

        self.target = self.root / 'originals/relocated.dll'
        make_pe(self.target)
        image = bytearray(self.target.read_bytes())
        # Two original call sites and two independently identified callees.
        image[0x400:0x406] = call_bytes(0x1000, 0x1100)
        image[0x410:0x416] = call_bytes(0x1010, 0x1180)
        image[0x500:0x506] = bytes.fromhex('31c0c3') + b'\x90' * 3
        image[0x580:0x586] = bytes.fromhex('31c0c3') + b'\x90' * 3
        self.target.write_bytes(image)

        self.bindings_path = self.root / 'evidence/reference_bindings.json'
        self.reference_note = self.root / 'evidence/reference_map.txt'
        self.reference_note.write_bytes(b'independent synthetic original symbol map\n')
        self.write_bindings()

        self.objects = {}
        self.rows = {}
        self.prior_proofs = {}
        self.addCleanup(patch.stopall)
        patch.object(match, 'ROOT', self.root).start()
        patch.object(promote, 'ROOT', self.root).start()
        self.state = self.root / 'recovery.json'
        self.journal = self.root / 'work/promotion-journal.json'
        patch.object(promote, 'STATE', self.state).start()
        patch.object(promote, 'JOURNAL', self.journal).start()

        toolchain_verifier = Mock(return_value='synthetic-vc5')
        toolchain_verifier.cache_clear = Mock()
        patch.object(match, 'verify_toolchain', toolchain_verifier).start()
        self.compiler = patch.object(match, 'compile_source', side_effect=self.compile_fixture).start()

    def write_bindings(self, destinations=None):
        destinations = destinations or {'alpha': 0x1100, 'beta': 0x1180}
        document = {
            'schema': 1,
            'evidence_files': {
                'evidence/reference_map.txt': file_digest(self.reference_note),
            },
            'functions': {
                function_id: {
                    'symbols': {
                        '_callee': binding('function', destination, 6,
                                           (1, masked.REL32, 0)),
                    },
                }
                for function_id, destination in destinations.items()
            },
        }
        self.binding_document = document
        self.bindings_path.write_text(json.dumps(document, indent=2) + '\n', encoding='ascii')
        return document

    def add_relocated_candidate(self, function_id, rva, destination):
        source = self.root / 'src' / (function_id + '.c')
        source.write_text('#include "shared.h"\nint sample(void) { return 0; }\n', encoding='ascii')
        obj = self.root / 'objects' / (function_id + '.obj')
        code = b'\xe8\0\0\0\0\xc3'
        coff_file(obj, code, [(1, 1, masked.REL32)],
                  [('_sample', 0, 1, 0x20), ('_callee', 0, 0, 0x20)])
        self.objects[source.resolve()] = obj
        proof = {
            'scope': 'synthetic prior masked proof',
            'source_sha256': file_digest(source),
            'masked_sha256': match.digest(('masked-' + function_id).encode('ascii')),
            'toolchain': 'synthetic-vc5',
            'prior_identity': function_id,
        }
        self.prior_proofs[function_id] = copy.deepcopy(proof)
        row = {
            'id': function_id,
            'source': source.relative_to(self.root).as_posix(),
            'symbol': '_sample',
            'target': self.target.relative_to(self.root).as_posix(),
            'target_sha256': file_digest(self.target),
            'rva': hex(rva),
            'size': len(code),
            'flags': [],
            'status': 'CODEGEN_SIMILAR',
            'proof': proof,
        }
        self.rows[function_id] = row
        return row

    def add_raw_peer(self, bad=False):
        source = self.root / 'src/raw.c'
        source.write_text('int raw(void) { return 0; }\n', encoding='ascii')
        obj = self.root / 'objects/raw.obj'
        raw_code = CODE if not bad else b'\xcc' + CODE[1:]
        coff_file(obj, raw_code, [], [('_sample', 0, 1, 0x20)])
        target = self.root / 'originals/raw.dll'
        make_pe(target)
        self.objects[source.resolve()] = obj
        row = {
            'id': 'raw-peer',
            'source': source.relative_to(self.root).as_posix(),
            'symbol': '_sample',
            'target': target.relative_to(self.root).as_posix(),
            'target_sha256': file_digest(target),
            'rva': '0x1000',
            'size': len(CODE),
            'flags': [],
            'status': 'FUNCTION_MATCH',
            'proof': {
                'scope': 'synthetic strict raw proof',
                'source_sha256': file_digest(source),
                'compiled_sha256': match.digest(CODE),
                'target_sha256': match.digest(CODE),
                'toolchain': 'synthetic-vc5',
            },
        }
        return row

    def compile_fixture(self, source, flags, toolchain=match.DEFAULT_TOOLCHAIN):
        path = Path(source).resolve()
        if path not in self.objects:
            raise AssertionError('No synthetic object registered for ' + str(path))
        return self.objects[path], ['synthetic pinned compiler', str(path)]

    def write_state(self, rows):
        self.state.write_text(json.dumps({'functions': rows}, indent=2) + '\n', encoding='ascii')

    def promote(self, ids, **kwargs):
        return promote.promote_relocated(ids, self.bindings_path, **kwargs)

    def accepted_row(self, function_id):
        return next(row for row in json.loads(self.state.read_text(encoding='ascii'))['functions']
                    if row['id'] == function_id)

    def test_each_function_keeps_its_own_prior_proof_and_fresh_reference_proof(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        beta = self.add_relocated_candidate('beta', 0x1010, 0x1180)
        self.write_state([alpha, beta])
        source_before = {
            row['id']: (self.root / row['source']).read_bytes()
            for row in (alpha, beta)
        }

        self.promote(['alpha', 'beta'])

        accepted = {row['id']: row for row in json.loads(self.state.read_text())['functions']}
        for function_id in ('alpha', 'beta'):
            self.assertEqual(accepted[function_id]['status'], 'FUNCTION_MATCH_RELOCATED')
            self.assertEqual(accepted[function_id]['prior_masked_proof'], self.prior_proofs[function_id])
            self.assertEqual(accepted[function_id]['proof']['bindings_path'],
                             self.bindings_path.relative_to(self.root).as_posix())
            self.assertEqual(accepted[function_id]['proof']['bindings_sha256'],
                             match.digest(json.dumps(
                                 self.binding_document['functions'][function_id], sort_keys=True,
                                 separators=(',', ':')).encode()))
            self.assertEqual((self.root / accepted[function_id]['source']).read_bytes(),
                             source_before[function_id])

        results, stats = match.verify_rows(list(accepted.values()))
        self.assertEqual(stats['rows'], 2)
        self.assertTrue(all(result['exact'] for result in results))

    def test_verify_only_does_not_publish_a_relocated_status(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        before = self.state.read_bytes()

        receipt = self.promote(['alpha'], verify_only=True)

        self.assertTrue(receipt['verify_only'])
        self.assertEqual(self.state.read_bytes(), before)
        self.assertEqual(self.accepted_row('alpha')['status'], 'CODEGEN_SIMILAR')
        self.assertFalse(self.journal.exists())

    def test_wrong_independent_callee_binding_blocks_publication(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        self.write_bindings({'alpha': 0x1110})
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'complete function/reference regression'):
            self.promote(['alpha'])

        self.assertEqual(self.state.read_bytes(), before)
        self.assertFalse(self.journal.exists())

    def test_stale_independent_evidence_hash_rejects_an_accepted_row(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        self.promote(['alpha'])
        accepted = self.accepted_row('alpha')
        self.compiler.reset_mock()
        self.reference_note.write_bytes(b'changed independent evidence\n')

        with self.assertRaisesRegex(ValueError, 'Independent reference evidence changed'):
            match.verify_rows([accepted])

        self.compiler.assert_not_called()

    def test_changed_binding_after_acceptance_is_rejected_by_its_digest(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        self.promote(['alpha'])
        accepted = self.accepted_row('alpha')
        self.compiler.reset_mock()
        self.write_bindings({'alpha': 0x1110})

        with self.assertRaisesRegex(ValueError, 'Accepted reference bindings changed: alpha'):
            match.verify_rows([accepted])

        self.compiler.assert_not_called()

    def test_changed_header_after_acceptance_invalidates_compile_context(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        self.promote(['alpha'])
        accepted = self.accepted_row('alpha')
        self.compiler.reset_mock()
        self.header.write_text('/* changed compiler input */\n', encoding='ascii')

        with self.assertRaisesRegex(ValueError, 'Accepted compiler input changed: alpha'):
            match.verify_rows([accepted])

        self.compiler.assert_not_called()

    def test_header_mutated_after_measurement_blocks_state_publication(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        before = self.state.read_bytes()
        real_verify = match.verify_rows

        def mutate_header_after_verification(*args, **kwargs):
            results = real_verify(*args, **kwargs)
            self.header.write_text('/* raced after measurement */\n', encoding='ascii')
            return results

        with patch.object(promote, 'verify_rows', side_effect=mutate_header_after_verification):
            with self.assertRaisesRegex(ValueError, 'Compiler input changed before publication'):
                self.promote(['alpha'])

        self.assertEqual(self.state.read_bytes(), before)
        self.assertFalse(self.journal.exists())

    def test_reference_evidence_mutated_after_measurement_blocks_publication(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        before = self.state.read_bytes()
        real_verify = match.verify_rows

        def mutate_evidence_after_verification(*args, **kwargs):
            results = real_verify(*args, **kwargs)
            self.reference_note.write_bytes(b'raced independent evidence change\n')
            return results

        with patch.object(promote, 'verify_rows', side_effect=mutate_evidence_after_verification):
            with self.assertRaisesRegex(ValueError, 'Independent reference evidence changed'):
                self.promote(['alpha'])

        self.assertEqual(self.state.read_bytes(), before)
        self.assertFalse(self.journal.exists())

    def test_raw_peer_regression_prevents_relocated_publication(self):
        raw = self.add_raw_peer(bad=True)
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([raw, alpha])
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'complete function/reference regression'):
            self.promote(['alpha'])

        self.assertEqual(self.state.read_bytes(), before)
        self.assertFalse(self.journal.exists())

    def prepare_existing_unit_candidate(self):
        import similar
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        self.promote(['alpha'])
        accepted = self.accepted_row('alpha')
        raw = self.add_raw_peer()
        raw['status'] = 'CODEGEN_SIMILAR'
        raw['link_symbols'] = {}
        raw['proof'].update(scope=similar.SCOPE, masked_sha256=match.digest(CODE))
        self.write_state([accepted, raw])
        return accepted

    def test_existing_unit_raw_promotion_also_verifies_relocated_peers(self):
        accepted = self.prepare_existing_unit_candidate()
        receipt = promote.promote_existing_units(['raw-peer'])
        self.assertEqual({r['id'] for r in receipt['results']}, {'raw-peer', 'alpha'})
        self.assertEqual(self.accepted_row('alpha'), accepted)
        self.assertEqual(self.accepted_row('raw-peer')['status'], 'FUNCTION_MATCH')

    def test_stale_relocated_peer_blocks_existing_unit_raw_promotion(self):
        self.prepare_existing_unit_candidate()
        before = self.state.read_bytes()
        self.write_bindings({'alpha': 0x1110})
        with self.assertRaisesRegex(ValueError, 'Accepted reference bindings changed'):
            promote.promote_existing_units(['raw-peer'])
        self.assertEqual(self.state.read_bytes(), before)

    def test_existing_unit_promotion_rechecks_relocated_evidence_before_publication(self):
        self.prepare_existing_unit_candidate()
        before = self.state.read_bytes()
        real_verify = match.verify_rows

        def mutate_evidence_after_verification(*args, **kwargs):
            results = real_verify(*args, **kwargs)
            self.reference_note.write_bytes(b'raced independent evidence change\n')
            return results

        with patch.object(promote, 'verify_rows', side_effect=mutate_evidence_after_verification):
            with self.assertRaisesRegex(ValueError, 'Independent reference evidence changed'):
                promote.promote_existing_units(['raw-peer'])
        self.assertEqual(self.state.read_bytes(), before)

    def test_new_acceptance_does_not_downgrade_raw_or_relocated_peers(self):
        raw = self.add_raw_peer()
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        beta = self.add_relocated_candidate('beta', 0x1010, 0x1180)
        self.write_state([raw, alpha, beta])
        self.promote(['alpha'])
        raw_before = copy.deepcopy(self.accepted_row('raw-peer'))
        alpha_before = copy.deepcopy(self.accepted_row('alpha'))

        self.promote(['beta'])

        self.assertEqual(self.accepted_row('raw-peer'), raw_before)
        self.assertEqual(self.accepted_row('alpha'), alpha_before)
        self.assertEqual(self.accepted_row('beta')['status'], 'FUNCTION_MATCH_RELOCATED')

    def test_failure_after_state_replace_rolls_back_and_removes_journal(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        before = self.state.read_bytes()
        real_atomic_bytes = promote.atomic_bytes
        failed = False

        def fail_once_after_state_replace(path, content):
            nonlocal failed
            if Path(path).resolve() == self.state.resolve() and not failed:
                real_atomic_bytes(path, content)
                failed = True
                raise OSError('synthetic failure after replacement')
            return real_atomic_bytes(path, content)

        with patch.object(promote, 'atomic_bytes', side_effect=fail_once_after_state_replace):
            with self.assertRaisesRegex(OSError, 'after replacement'):
                self.promote(['alpha'])

        self.assertTrue(failed)
        self.assertEqual(self.state.read_bytes(), before)
        self.assertFalse(self.journal.exists())

    def test_recovery_state_changed_during_measurement_is_never_overwritten(self):
        alpha = self.add_relocated_candidate('alpha', 0x1000, 0x1100)
        self.write_state([alpha])
        before = self.state.read_bytes()
        real_verify = match.verify_rows
        external_edit = b'{"functions": [], "external_edit": true}\n'

        def mutate_state_after_verification(*args, **kwargs):
            results = real_verify(*args, **kwargs)
            self.state.write_bytes(external_edit)
            return results

        with patch.object(promote, 'verify_rows', side_effect=mutate_state_after_verification):
            with self.assertRaisesRegex(ValueError, 'Recovery state changed during promotion'):
                self.promote(['alpha'])

        self.assertNotEqual(self.state.read_bytes(), before)
        self.assertEqual(self.state.read_bytes(), external_edit)
        self.assertFalse(self.journal.exists())


if __name__ == '__main__':
    unittest.main()
