"""Compiler-profile dependencies on the legacy source-replacing RAW publisher."""
import copy
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import promote


def sha(data):
    return hashlib.sha256(data).hexdigest()


class RawPublisherProfilePins(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        for name in ['src', 'work', 'evidence']:
            (self.root / name).mkdir()
        self.source = self.root / 'src/a.c'
        self.old = b'int a(void) { return 1; }\n'
        self.source.write_bytes(self.old)
        (self.root / 'src/b.c').write_bytes(b'int b(void) { return 2; }\n')
        self.candidate = self.root / 'work/a.c'
        self.new = self.old + b'/* source-neutral publication */\n'
        self.candidate.write_bytes(self.new)
        self.review = self.root / 'evidence/profile.json'
        self.raw = b'{"schema":1,"profile":"reviewed-append-define-v1"}\n'
        self.review.write_bytes(self.raw)
        history = [dict(review=dict(path='evidence/profile.json', sha256=sha(self.raw)),
                        before=['/O2'], after=['/O2', '/DPROFILE'])]
        self.rows = [dict(id='m.' + name, source='src/' + name + '.c', symbol='_' + name,
                         flags=['/O2', '/DPROFILE'], status='FUNCTION_MATCH',
                         target='work/oracle', target_sha256=sha(b'oracle'), rva='0x1000', size=1,
                         proof=dict(source_sha256=sha((self.root / ('src/' + name + '.c')).read_bytes())),
                         compiler_profile_history=copy.deepcopy(history)) for name in ['a', 'b']]
        self.state = self.root / 'recovery.json'
        self.save_state()
        self.journal = self.root / 'work/promotion-journal.json'
        (self.root / 'work/oracle').write_bytes(b'oracle')
        for module in [match, promote]:
            active = patch.object(module, 'ROOT', self.root)
            active.start(); self.addCleanup(active.stop)
        for field, value in [('STATE', self.state), ('JOURNAL', self.journal)]:
            active = patch.object(promote, field, value)
            active.start(); self.addCleanup(active.stop)
        active = patch.object(promote, 'validate_candidate_object')
        active.start(); self.addCleanup(active.stop)
        active = patch.object(promote, 'check', side_effect=self.exact_check)
        self.check = active.start(); self.addCleanup(active.stop)

    def save_state(self):
        self.saved = (json.dumps(dict(functions=self.rows), indent=2) + '\n').encode()
        self.state.write_bytes(self.saved)

    def exact_check(self, row, source=None, **kwargs):
        # This publisher-boundary fixture deliberately bypasses check()'s own
        # protection: the publisher must independently freeze/recheck the pins.
        source = Path(source) if source is not None else self.root / row['source']
        return dict(exact=True, scope='fixture', source_sha256=sha(source.read_bytes()),
                    compiled_sha256='a' * 64, target_sha256='b' * 64,
                    toolchain='fixture', object='fixture.obj')

    def bad_review(self, mode):
        if mode == 'missing':
            self.review.unlink(missing_ok=True)
        else:
            self.review.write_bytes(self.raw + b' ')

    def unchanged(self):
        self.assertEqual(self.source.read_bytes(), self.old)
        self.assertEqual(self.state.read_bytes(), self.saved)
        self.assertFalse(self.journal.exists())
        self.assertFalse((self.root / 'work/promotion-candidate-stage/promotion-candidate.c').exists())

    def test_valid_reviews_publish_and_retain_history_for_all_raw_rows(self):
        promote.promote(['m.a'], self.candidate)
        self.assertEqual(self.source.read_bytes(), self.new)
        state = json.loads(self.state.read_bytes())
        self.assertEqual(self.check.call_count, 2)
        self.assertTrue(all(r['compiler_profile_history'] == self.rows[i]['compiler_profile_history']
                            for i, r in enumerate(state['functions'])))
        self.assertEqual(state['functions'][0]['proof']['source_sha256'], sha(self.new))
        self.assertFalse(self.journal.exists())

    def test_missing_or_corrupt_reviews_refuse_publisher_before_compilation(self):
        for mode in ['missing', 'corrupt']:
            with self.subTest(mode=mode):
                self.review.write_bytes(self.raw); self.bad_review(mode)
                with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
                    promote.promote(['m.a'], self.candidate)
                self.check.assert_not_called()
                self.unchanged()

    def test_unselected_raw_peer_review_is_required(self):
        del self.rows[0]['compiler_profile_history']
        self.save_state()
        self.review.unlink()
        with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
            promote.promote(['m.a'], self.candidate)
        self.check.assert_not_called()
        self.unchanged()

    def test_review_mutation_after_compilation_refuses_publication(self):
        for mode in ['missing', 'corrupt']:
            with self.subTest(mode=mode):
                self.review.write_bytes(self.raw)
                def checked(row, source=None, **kwargs):
                    result = self.exact_check(row, source)
                    if row['id'] == 'm.b':
                        self.bad_review(mode)
                    return result
                self.check.side_effect = checked
                with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
                    promote.promote(['m.a'], self.candidate)
                self.unchanged()

    def test_mutation_at_source_or_state_write_rolls_back_both(self):
        atomic = promote.atomic_bytes
        for mode in ['missing', 'corrupt']:
            for destination in [self.source, self.state]:
                with self.subTest(mode=mode, destination=destination.name):
                    self.review.write_bytes(self.raw)
                    injected = []
                    def write(path, data):
                        atomic(path, data)
                        if Path(path) == destination and not injected:
                            injected.append(True)
                            self.bad_review(mode)
                    with patch.object(promote, 'atomic_bytes', side_effect=write):
                        with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
                            promote.promote(['m.a'], self.candidate)
                    self.assertEqual(injected, [True])
                    self.unchanged()

    def test_verify_only_also_refuses_post_compile_mutation(self):
        def checked(row, source=None, **kwargs):
            result = self.exact_check(row, source)
            if row['id'] == 'm.b':
                self.review.unlink()
            return result
        self.check.side_effect = checked
        with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
            promote.promote(['m.a'], self.candidate, verify_only=True)
        self.unchanged()

    def test_direct_check_refuses_missing_or_corrupt_review_before_compilation(self):
        for mode in ['missing', 'corrupt']:
            with self.subTest(mode=mode):
                self.review.write_bytes(self.raw); self.bad_review(mode)
                with patch.object(match, 'compile_source') as compile_source:
                    with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
                        match.check(self.rows[0])
                    compile_source.assert_not_called()

    def test_direct_check_rechecks_review_after_compilation(self):
        def compile_source(*args):
            self.review.unlink()
            return self.root / 'work/object.obj', ['fixture']
        with patch.object(match, 'compile_source', side_effect=compile_source), \
                patch.object(match, 'compare', return_value=dict(exact=True)):
            with self.assertRaisesRegex(ValueError, '(?i)compiler profile review'):
                match.check(self.rows[0])


if __name__ == '__main__':
    unittest.main()
