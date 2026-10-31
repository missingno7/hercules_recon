"""Reviewed re-pin of a canonical header: contexts re-pinned, pins redirected, other files refused."""
import hashlib
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import repin  # noqa: E402  (this checkout's scripts/repin.py, loaded before the shared fixture)
import test_binding_rename as base  # noqa: E402

OLD, NEW = b'/* declarations, old names */\n', b'/* declarations, new names */\n'


def sha(data):
    return hashlib.sha256(data).hexdigest()


class HeaderRepinTests(unittest.TestCase):
    """Runs the real repin.apply() transaction on the shared miniature repository fixture."""

    setUp = base.EndToEndApplyTests.setUp
    tearDown = base.EndToEndApplyTests.tearDown
    before_report = base.EndToEndApplyTests.before_report
    run_apply = base.EndToEndApplyTests.run_apply

    def install_header(self, pin_in_parent=True):
        (self.root / 'src/unit.h').write_bytes(OLD)
        (self.root / 'work/cand/unit.h').write_bytes(NEW)
        if pin_in_parent:  # a binding manifest pins the header as historical provenance
            parent = json.loads((self.root / 'evidence/parent.json').read_bytes())
            parent['evidence_files']['src/unit.h'] = sha(OLD)
            (self.root / 'evidence/parent.json').write_text(json.dumps(parent, indent=1) + '\n')
        state = json.loads((self.root / 'recovery.json').read_bytes())
        for row in state['functions']:
            row['proof']['context_sha256'] = {row['source']: row['proof']['source_sha256'], 'src/unit.h': sha(OLD)}
            if row['proof'].get('bindings_path') == 'evidence/parent.json':
                document = json.loads((self.root / 'evidence/parent.json').read_bytes())
                row['proof'].update(base.binding_rename.proof_fields(document, row['id']))
        (self.root / 'recovery.json').write_text(json.dumps(state, indent=2) + '\n')

    def plan(self, name='src/unit.h', candidate='work/cand/unit.h'):
        state = (self.root / 'recovery.json').read_bytes()
        return dict(state_sha256=sha(state), promotions=[], sources={name: dict(
            candidate=candidate, before_sha256=sha((self.root / name).read_bytes()),
            candidate_sha256=sha((self.root / candidate).read_bytes()))})

    def test_header_change_repins_every_including_context_and_redirects_its_pins(self):
        self.install_header()
        receipt = self.run_apply(self.plan())
        state = json.loads((self.root / 'recovery.json').read_bytes())
        for row in state['functions']:
            self.assertEqual(row['proof']['context_sha256']['src/unit.h'], sha(NEW), row['id'])
        caller = next(r for r in state['functions'] if r['id'] == 'm.caller')
        snapshot = caller['proof']['reference_snapshots']['src/unit.h']
        self.assertEqual((self.root / snapshot).read_bytes(), OLD)  # the pinned provenance stays readable
        self.assertEqual((self.root / 'src/unit.h').read_bytes(), NEW)
        self.assertEqual(receipt['promoted'], [])
        self.assertIn('m.caller', receipt['affected'])

    def test_other_files_and_outside_paths_cannot_change(self):
        (self.root / 'src/notes.txt').write_bytes(b'x')
        with base.patch.object(repin, 'ROOT', self.root):
            self.assertEqual(repin.canonical_change('src/caller.c'), (self.root / 'src/caller.c').resolve())
            for name in ('src/notes.txt', 'evidence/parent.json', '../outside.h', 'src/missing.h'):
                with self.assertRaises(ValueError, msg=name):
                    repin.canonical_change(name)


if __name__ == '__main__':
    unittest.main()
