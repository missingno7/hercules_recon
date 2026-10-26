import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import checkout_pins  # noqa: E402


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


class CheckTree(unittest.TestCase):
    def setUp(self):
        self.tree = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tree)
        write(self.tree / 'src/a.c', b'int a;\n')
        write(self.tree / 'evidence/old.json', b'{"x": 1}\r\n')
        self.old = checkout_pins.digest(self.tree / 'evidence/old.json')
        document = dict(functions={}, evidence_files={'evidence/old.json': self.old})
        write(self.tree / 'evidence/b.json', json.dumps(document).encode())
        self.row = dict(id='m.a', status='FUNCTION_MATCH_RELOCATED', source='src/a.c',
                        proof=dict(source_sha256=checkout_pins.digest(self.tree / 'src/a.c'),
                                   bindings_path='evidence/b.json'))
        self.save()

    def save(self):
        write(self.tree / 'recovery.json', json.dumps(dict(functions=[self.row])).encode())

    def test_clean_tree_passes(self):
        counts, problems = checkout_pins.check_tree(self.tree)
        self.assertEqual(problems, [])
        self.assertEqual((counts['rows'], counts['pins']), (1, 1))

    def test_normalized_crlf_evidence_is_detected(self):
        write(self.tree / 'evidence/old.json', b'{"x": 1}\n')  # what an LF-normalizing checkout produces
        _, problems = checkout_pins.check_tree(self.tree)
        self.assertEqual(problems, [('pin hash', 'm.a', 'evidence/old.json')])

    def test_snapshot_redirect_restores_exact_bytes(self):
        write(self.tree / 'evidence/old.json', b'{"x": 1}\n')
        address = f'{checkout_pins.SNAPSHOTS}/{self.old}.txt'
        write(self.tree / address, b'{"x": 1}\r\n')
        self.row['proof']['reference_snapshots'] = {'evidence/old.json': address}
        self.save()
        counts, problems = checkout_pins.check_tree(self.tree)
        self.assertEqual(problems, [])
        self.assertEqual(counts['snapshots'], 1)

    def test_misaddressed_snapshot_and_source_drift_are_detected(self):
        write(self.tree / f'{checkout_pins.SNAPSHOTS}/{"0" * 64}.txt', b'other')
        write(self.tree / 'src/a.c', b'int a;\r\n')
        _, problems = checkout_pins.check_tree(self.tree)
        self.assertIn(('snapshot address', '0' * 64 + '.txt'), problems)
        self.assertIn(('source', 'm.a', 'src/a.c'), problems)

    def test_uncommitted_originals_are_left_to_the_verifier(self):
        document = dict(functions={}, evidence_files={'assets/pc/X.DLL': '0' * 64, 'work/discs/a': '0' * 64})
        write(self.tree / 'evidence/b.json', json.dumps(document).encode())
        counts, problems = checkout_pins.check_tree(self.tree)
        self.assertEqual((problems, counts['pins']), ([], 0))


@unittest.skipUnless(shutil.which('git'), 'git required')
class CleanCheckout(unittest.TestCase):
    def test_committed_head_reproduces_every_pin_under_both_eol_settings(self):
        for autocrlf, (counts, problems) in checkout_pins.clean_checkouts('HEAD').items():
            self.assertGreater(counts['rows'], 0, autocrlf)
            self.assertEqual(problems, [], autocrlf)


if __name__ == '__main__':
    unittest.main()
