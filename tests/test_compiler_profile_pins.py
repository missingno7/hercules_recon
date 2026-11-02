import hashlib
import json
import pathlib
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts'))
import checkout_pins
import match
import profile_history
import promote


class CompilerProfilePins(unittest.TestCase):
    def setUp(self):
        self.root = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        (self.root / 'src').mkdir()
        (self.root / 'evidence').mkdir()
        (self.root / 'toolchains').mkdir()
        (self.root / 'toolchains/manifest.json').write_bytes(b'{}')
        (self.root / 'src/a.c').write_bytes(b'int a;\n')
        self.path = self.root / 'evidence/profile.json'
        self.raw = b'{"schema": 1, "profile": "reviewed-append-define-v1"}\r\n'
        self.path.write_bytes(self.raw)
        self.pin = dict(path='evidence/profile.json', sha256=hashlib.sha256(self.raw).hexdigest())
        self.row = dict(id='m.a', status='FUNCTION_MATCH', source='src/a.c', target='assets/X.DLL',
                        flags=['/O2', '/DMODULE'], target_sha256='a' * 64,
                        proof=dict(source_sha256=hashlib.sha256(b'int a;\n').hexdigest()),
                        compiler_profile_history=[dict(review=self.pin, before=['/O2'], after=['/O2', '/DMODULE'])])

    def checkout(self):
        (self.root / 'recovery.json').write_text(json.dumps(dict(functions=[self.row])))
        return checkout_pins.check_tree(self.root)[1]

    def strict_refuses_before_compile(self):
        with patch.object(match, 'ROOT', self.root), patch.object(match, 'verify_toolchain') as toolchain, \
                patch.object(match, 'compile_source') as compile_source:
            toolchain.return_value = 'fixture'
            with self.assertRaises(ValueError):
                match.verify_rows([self.row])
            compile_source.assert_not_called()

    def test_valid_pin_and_exact_crlf_checkout_bytes(self):
        self.assertEqual(self.checkout(), [])
        self.assertEqual(profile_history.history_inputs(self.row, self.root), {self.path: self.pin['sha256']})

    def test_missing_review_fails_strict_and_checkout(self):
        self.path.unlink()
        self.strict_refuses_before_compile()
        self.assertTrue(self.checkout())

    def test_publication_rechecks_review_pin(self):
        self.path.unlink()
        with patch.object(promote, 'ROOT', self.root):
            with self.assertRaises(ValueError):
                promote.assert_verified_inputs([self.row], [dict(id='m.a')])

    def test_corrupt_and_line_normalized_review_fail_both_paths(self):
        for data in [self.raw + b' ', self.raw.replace(b'\r\n', b'\n')]:
            with self.subTest(data=data):
                self.path.write_bytes(data)
                self.strict_refuses_before_compile()
                self.assertTrue(self.checkout())

    def test_duplicate_json_keys_fail_even_with_updated_hash(self):
        self.path.write_bytes(b'{"schema":1,"schema":1}')
        self.pin['sha256'] = hashlib.sha256(self.path.read_bytes()).hexdigest()
        self.strict_refuses_before_compile()
        self.assertTrue(self.checkout())

    def test_noncanonical_or_escaping_keys_and_bad_hash_fail_both_paths(self):
        for name in ['evidence/../profile.json', 'evidence//profile.json', 'evidence\\profile.json',
                     'D:/Prog/hercules_recon/evidence/profile.json', 'src/a.c', '/tmp/profile.json']:
            with self.subTest(name=name):
                self.pin['path'] = name
                self.strict_refuses_before_compile()
                self.assertTrue(self.checkout())
        self.pin['path'] = 'evidence/profile.json'
        self.pin['sha256'] = 'bad'
        self.strict_refuses_before_compile()
        self.assertTrue(self.checkout())

    def test_every_older_history_entry_is_checked(self):
        self.row['compiler_profile_history'].append(dict(review=dict(self.pin), before=['/O2'], after=['/O2', '/DOTHER']))
        self.row['compiler_profile_history'][0]['review'] = dict(path='evidence/missing.json', sha256='b' * 64)
        self.strict_refuses_before_compile()
        self.assertTrue(self.checkout())

    def test_no_history_is_backward_compatible(self):
        del self.row['compiler_profile_history']
        self.assertEqual(self.checkout(), [])
        self.assertEqual(profile_history.history_inputs(self.row, self.root), {})

    def test_malformed_history_fails_both_paths(self):
        self.row['compiler_profile_history'] = {'review': self.pin}
        self.strict_refuses_before_compile()
        self.assertTrue(self.checkout())


if __name__ == '__main__':
    unittest.main()
