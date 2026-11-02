"""Recorded absolute evidence keys are string aliases; confinement is OS identity, never path strings."""
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import binding_rename
import checkout_pins
import match
import repin
from evidence_keys import (RECORDED_ROOTS, canonical_keys, checked_path, confined_path, equivalent_keys, external_parts,
                           load_json, repository_key)
from original_evidence import original_evidence_path
from reviewed_evidence import snapshot_path
from test_binding_rename import OwnerFixture

FS, BS = RECORDED_ROOTS[0][0] + '/', RECORDED_ROOTS[1][0] + '\\'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def junction(link, target):
    """A directory junction (no privilege needed); the tests needing one skip elsewhere."""
    import _winapi
    link.parent.mkdir(parents=True, exist_ok=True)
    _winapi.CreateJunction(str(target), str(link))


needs_junctions = unittest.skipUnless(os.name == 'nt', 'directory junctions are a Windows feature')


class Names(unittest.TestCase):
    def test_exactly_two_recorded_spellings_alias_their_relative_key(self):
        self.assertEqual(RECORDED_ROOTS, (('D:/Prog/hercules_recon', '/'), ('D:\\Prog\\hercules_recon', '\\')))
        self.assertEqual(repository_key(FS + 'src/title/u02b70.c'), 'src/title/u02b70.c')
        self.assertEqual(repository_key(BS + 'src\\eng1\\frame_data.c'), 'src/eng1/frame_data.c')
        self.assertEqual(repository_key(BS + 'evidence\\a-b_2.json'), 'evidence/a-b_2.json')
        self.assertEqual(repository_key('src/unit.c'), 'src/unit.c')
        self.assertEqual(repository_key('evidence\\x.json'), 'evidence/x.json')  # recorded relative legacy spelling

    def test_strict_component_grammar(self):
        for relative in ('src', 'src/', 'src//unit.c', 'src/./unit.c', 'src/../unit.c', 'src/unit.c.', 'src/dir./u.c',
                         'src/dir /u.c', 'src/ dir/u.c', 'src/\u212a/u.c', 'src/unit.c:stream', 'src/UNIT~1.C',
                         'src/x+y.c', 'src/nul.c', 'src/CON', 'src/com1.h', 'assets/pc/X.DLL', 'work/u.c',
                         'scripts/match.py', 'src/a\n.c'):
            with self.subTest(relative=relative):
                with self.assertRaises(ValueError):
                    repository_key(FS + relative)
                with self.assertRaises(ValueError):
                    repository_key(BS + relative.replace('/', '\\'))
        for name in (FS + 'src/a\\b.c', BS + 'src\\a/b.c', FS + '../x/src/u.c', BS + '..\\x\\src\\u.c'):
            with self.subTest(name=name):
                with self.assertRaises(ValueError):
                    repository_key(name)  # mixed separators or traversal after a recorded root
        for relative in ('src/./u.c', 'src/../u.c', '../src/u.c', 'src//u.c', '', '.', 'src/u.c.', 'src/u.c ',
                         'src/a\\b.c', 'src\\a/b.c', 'src/\u212a.c', 'src/aux.c'):
            with self.subTest(relative=relative):
                with self.assertRaises(ValueError):
                    repository_key(relative)

    def test_other_absolute_spellings_are_not_repository_keys(self):
        for name in ('d:/Prog/hercules_recon/src/u.c', 'D:/PROG/hercules_recon/src/u.c',
                     'D:/Prog/hercules_recon /src/u.c', 'D:/Prog/hercules_recon./src/u.c', 'D:/Prog/hercules_reconX/src/u.c',
                     'D:\\Prog\\hercules_recon/src/u.c', 'D:/Prog/hercules_recon\\src\\u.c', '\\\\?\\D:\\Prog\\hercules_recon\\src\\u.c',
                     '//server/share/src/u.c', 'D:src/u.c', '\\src\\u.c', 'C:/elsewhere/src/u.c'):
            with self.subTest(name=name):
                self.assertIsNone(repository_key(name))

    def test_names_never_touch_the_filesystem(self):
        with patch('os.stat', side_effect=AssertionError('stat')), patch('os.lstat', side_effect=AssertionError('lstat')), \
                patch.object(Path, 'resolve', side_effect=AssertionError('resolve')):
            self.assertEqual(repository_key(BS + 'src\\u.c'), 'src/u.c')
            self.assertIsNone(repository_key('C:/elsewhere/src/u.c'))
            self.assertEqual(canonical_keys({FS + 'src/u.c': 'a', 'src/v.c': 'b'}), {FS + 'src/u.c': 'src/u.c', 'src/v.c': 'src/v.c'})

    def test_sdk_keys_need_the_exact_toolchain_prefix(self):
        base = Path('C:/tools/sdk')
        self.assertEqual(external_parts(str(base) + '\\include\\x.h', base), ['include', 'x.h'])
        self.assertEqual(external_parts(base.as_posix() + '/include/x.h', base), ['include', 'x.h'])
        for name in ('C:/TOOLS/sdk/include/x.h', 'C:/tools/sdk/../x.h', 'C:/tools/sdkX/x.h', 'C:/tools/sdk/include\\x.h'):
            with self.subTest(name=name):
                try:
                    self.assertIsNone(external_parts(name, base))
                except ValueError:
                    pass


class Ambiguity(unittest.TestCase):
    """Finding 5: case, dot-component and backslash aliases never evade the exactly-one policy."""

    def test_aliases_are_refused_or_counted(self):
        h = 'a' * 64
        for pins in ({'src/UNIT.c': h, FS + 'src/unit.c': h}, {'src/unit.c': h, FS + 'src/unit.c': 'b' * 64},
                     {BS + 'src\\Unit.c': h, 'src/unit.c': h}):
            with self.subTest(pins=pins):
                with self.assertRaisesRegex(ValueError, 'Ambiguous'):
                    canonical_keys(pins)
                with self.assertRaisesRegex(ValueError, 'Ambiguous'):
                    binding_rename._owner_pin_key({'evidence_files': pins}, 'src/unit.c')
                with self.assertRaisesRegex(ValueError, 'Ambiguous'):
                    repin.impact({'functions': []}, {'b': {'evidence_files': pins}}, {'src/unit.c'})
                with self.assertRaisesRegex(ValueError, 'Ambiguous'):
                    repin.reference_redirects(pins, {'src/unit.c': b'old'}, {'src/unit.c': sha(b'new')}, {})
        with self.assertRaisesRegex(ValueError, 'Invalid evidence key'):
            binding_rename._owner_pin_key({'evidence_files': {'src/./unit.c': h, FS + 'src/unit.c': h}}, 'src/unit.c')
        # A recorded legacy duplicate (same canonical key, same hash) is one pin to readers, but never
        # a pin an owner revision may move: it needs exactly one key.
        for pins in ({'src\\unit.c': h, FS + 'src/unit.c': h}, {'src/unit.c': h, BS + 'src\\unit.c': h}):
            with self.subTest(pins=pins):
                self.assertEqual(sorted(equivalent_keys(pins, 'src/unit.c')), sorted(pins))
                with self.assertRaisesRegex(ValueError, 'exactly one equivalent'):
                    binding_rename._owner_pin_key({'evidence_files': pins}, 'src/unit.c')
                audit = repin.impact({'functions': []}, {'b': {'evidence_files': pins}}, {'src/unit.c'})
                self.assertEqual(sorted(pin['path'] for pin in audit['direct_pins']), sorted(pins))
                current, mapping = repin.reference_redirects(pins, {'src/unit.c': b'old'}, {'src/unit.c': sha(b'new')}, {})
                self.assertEqual((current, sorted(mapping)), (set(), sorted(pins)))

    def test_case_variant_of_a_changed_source_is_found(self):
        h = 'a' * 64
        audit = repin.impact({'functions': []}, {'b': {'evidence_files': {'src/UNIT.c': h}}}, {'src/unit.c'})
        self.assertEqual([pin['path'] for pin in audit['direct_pins']], ['src/UNIT.c'])
        self.assertEqual(binding_rename._owner_pin_key({'evidence_files': {'src/UNIT.c': h}}, 'src/unit.c'), 'src/UNIT.c')
        self.assertEqual(repin.reference_redirects({'src/UNIT.c': h}, {'src/unit.c': b'old'}, {'src/unit.c': sha(b'n')}, {}),
                         (set(), {'src/UNIT.c': 'evidence/repin_snapshots/' + h + '.txt'}))

    def test_publication_refuses_new_ambiguity(self):
        plan = binding_rename.Plan()
        parent = dict(schema=1, evidence_files={'src/unit.c': 'a' * 64}, functions={'x': {}})
        documents = {'evidence/p.json': (json.dumps(parent).encode(), parent)}
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            binding_rename._manifest(plan, documents, 'evidence/p.json', 'x', {}, 'evidence/r.json', 'c' * 64,
                                     [{'evidence_files': {FS + 'src/unit.c': 'b' * 64}}])


class Fixture(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.base = Path(directory.name).resolve()
        self.root = self.base / 'K'
        self.old, self.new = b'old\n', b'new\n'
        self.h = sha(self.old)
        self.address = 'evidence/repin_snapshots/' + self.h + '.txt'
        write(self.root / 'src/unit.c', self.new)
        self.addCleanup(patch.stopall)
        for module in (match, repin, binding_rename):
            patch.object(module, 'ROOT', self.root).start()

    def fixture(self, name, redirect=None, digest=None):
        document = dict(schema=1, evidence_files={name: digest or self.h}, functions={'x': {'symbols': {}}})
        write(self.root / 'evidence/b.json', json.dumps(document).encode())
        proof = dict(source_sha256=sha(self.new), bindings_path='evidence/b.json')
        if redirect is not None:
            proof['reference_snapshots'] = {name: redirect}
        row = dict(id='x', status=match.RELOCATED, source='src/unit.c', proof=proof)
        write(self.root / 'recovery.json', json.dumps({'functions': [row]}).encode())
        return row

    def outcomes(self, name, redirect):
        """(snapshot_path, ReferenceSnapshot, checkout_pins) refusals for one fixture."""
        row = self.fixture(name, redirect)
        results = []
        for call in (lambda: snapshot_path(self.root, name, self.h, {name: redirect}),
                     lambda: match.ReferenceSnapshot().read('evidence/b.json', row)):
            try:
                call()
                results.append(False)
            except ValueError:
                results.append(True)
        results.append(bool(checkout_pins.check_tree(self.root)[1]))
        return results


class DuplicateJsonKeys(Fixture):
    """A repeated decoded key never reaches a check as only its last value (raw, escaped, nested)."""

    def raw_document(self, pins):
        return ('{"schema": 1, "evidence_files": {%s}, "functions": {"x": {"symbols": {}}}}' % pins).encode()

    def test_decoder_refuses_raw_and_escaped_duplicates(self):
        h, g = 'a' * 64, 'b' * 64
        for pins in ('"src/unit.c": "%s", "src/unit.c": "%s"' % (h, g),
                     '"src/unit.c": "%s", "src/\\u0075nit.c": "%s"' % (h, g),
                     '"src/unit.c": "%s", "src/unit.c": "%s"' % (h, h)):
            with self.subTest(pins=pins):
                with self.assertRaisesRegex(ValueError, 'Duplicate JSON object key'):
                    load_json(self.raw_document(pins))
        self.assertEqual(load_json(self.raw_document('"src/unit.c": "%s"' % h))['evidence_files'], {'src/unit.c': h})

    def test_every_pin_reader_refuses_duplicate_pins(self):
        write(self.root / self.address, self.old)
        row = self.fixture('src/unit.c', self.address)
        for pins in ('"src/unit.c": "%s", "src/unit.c": "%s"' % (sha(self.new), self.h),
                     '"src/unit.c": "%s", "src/\\u0075nit.c": "%s"' % (sha(self.new), self.h)):
            with self.subTest(pins=pins):
                (self.root / 'evidence/b.json').write_bytes(self.raw_document(pins))
                with self.assertRaisesRegex(ValueError, 'Duplicate JSON object key'):
                    match.ReferenceSnapshot().read('evidence/b.json', row)
                with self.assertRaisesRegex(ValueError, 'Duplicate JSON object key'):
                    checkout_pins.check_tree(self.root)
                with self.assertRaisesRegex(ValueError, 'Duplicate JSON object key'):
                    repin.documents()
                review = {'owner_revisions': {'src/unit.c': {'rows': {'x': {'bindings_path': 'evidence/b.json'}}}}}
                with self.assertRaisesRegex(ValueError, 'Duplicate JSON object key'):
                    binding_rename.parent_documents(review)

    def test_duplicate_snapshot_redirect_keys_are_refused(self):
        write(self.root / self.address, self.old)
        self.fixture('src/unit.c', self.address)
        state = (self.root / 'recovery.json').read_text()
        redirect = '"reference_snapshots": {"src/unit.c": "%s"}' % self.address
        duplicated = '"reference_snapshots": {"src/unit.c": "evidence/unaddressed.txt", "src/\\u0075nit.c": "%s"}' % self.address
        self.assertIn(redirect, state)
        (self.root / 'recovery.json').write_text(state.replace(redirect, duplicated))
        with self.assertRaisesRegex(ValueError, 'Duplicate JSON object key'):
            checkout_pins.check_tree(self.root)


class Consumers(Fixture):
    def test_every_consumer_reads_the_local_relative_file(self):
        write(self.root / self.address, self.old)
        for name in ('src/unit.c', 'src\\unit.c', FS + 'src/unit.c', BS + 'src\\unit.c'):
            with self.subTest(name=name):
                row = self.fixture(name, self.address)
                self.assertEqual(original_evidence_path(self.root, name, self.base / 'elsewhere'),
                                 (self.root / 'src/unit.c', False))
                self.assertEqual(snapshot_path(self.root, name, self.h, {name: self.address}), self.root / self.address)
                observed = match.ReferenceSnapshot().read('evidence/b.json', row)[2]
                self.assertIn(self.root / self.address, observed)
                self.assertNotIn(self.root / 'src/unit.c', observed)
                self.assertEqual(checkout_pins.check_tree(self.root)[1], [])
                self.assertEqual(repin.reference_redirects({name: self.h}, {'src/unit.c': self.old},
                                                           {'src/unit.c': sha(self.new)}, {}), (set(), {name: self.address}))
                document = {'evidence_files': {name: self.h}}
                self.assertEqual(repin.impact({'functions': []}, {'b': document}, {'src/unit.c'})['direct_pins'][0]['path'], name)
                self.assertEqual(binding_rename._owner_pin_key(document, 'src/unit.c'), name)

    def test_unrecorded_absolute_keys_refuse(self):
        write(self.root / self.address, self.old)
        for name in (str(self.root / 'src/unit.c'), (self.root / 'src/unit.c').as_posix(), 'D:/Prog/hercules_recon\\src\\unit.c'):
            with self.subTest(name=name):
                row = self.fixture(name, self.address)
                with self.assertRaisesRegex(ValueError, 'Unsupported absolute'):
                    original_evidence_path(self.root, name, self.root)
                with self.assertRaises(ValueError):
                    match.ReferenceSnapshot().read('evidence/b.json', row)
                with self.assertRaises(ValueError):
                    snapshot_path(self.root, name, self.h, {name: self.address})
                self.assertTrue(checkout_pins.check_tree(self.root)[1])
                with self.assertRaisesRegex(ValueError, 'exactly one equivalent'):
                    binding_rename._owner_pin_key({'evidence_files': {name: self.h}}, 'src/unit.c')

    def test_checkout_redirects_need_the_exact_content_address(self):
        """Finding 2: checkout_pins goes through snapshot_path for every redirect."""
        write(self.root / self.address, self.old)
        write(self.root / 'evidence/unaddressed.txt', self.old)
        outside = self.base / 'external.txt'
        write(outside, self.old)
        for name in ('src/unit.c', FS + 'src/unit.c', BS + 'src\\unit.c'):
            for redirect in ('evidence/unaddressed.txt', str(outside), outside.as_posix(), '../external.txt'):
                with self.subTest(name=name, redirect=redirect):
                    self.fixture(name, redirect)
                    self.assertTrue(checkout_pins.check_tree(self.root)[1])
        self.fixture('C:/elsewhere/x.h', self.address)
        self.assertEqual(checkout_pins.check_tree(self.root)[1][0][0], 'invalid snapshot redirect')

    def test_sdk_keys_keep_their_exact_prefix_reads(self):
        sdk = self.base / 'toolchain'
        header = sdk / 'include/header.h'
        write(header, self.old)
        with patch.object(match, 'DEFAULT_TOOLCHAIN', sdk):
            for name in (str(header), header.as_posix()):
                row = self.fixture(name)
                self.assertIn(header, match.ReferenceSnapshot().read('evidence/b.json', row)[2])
                self.assertEqual(checkout_pins.check_tree(self.root)[1], [])
            row = self.fixture(str(sdk).upper() + '\\include\\header.h')
            with self.assertRaisesRegex(ValueError, 'pinned SDK'):
                match.ReferenceSnapshot().read('evidence/b.json', row)

    def test_recovery_state_is_never_evidence(self):
        for name in ('recovery.json', 'RECOVERY.JSON'):
            with self.subTest(name=name):
                row = self.fixture(name)
                with self.assertRaisesRegex(ValueError, 'independent of recovery state'):
                    match.ReferenceSnapshot().read('evidence/b.json', row)

    def test_owner_review_derives_with_both_recorded_spellings(self):
        for index, key in enumerate((FS + 'src/pipe.c', BS + 'src\\pipe.c')):
            with self.subTest(key=key):
                root = self.base / ('owner_%d' % index)
                root.mkdir()
                with patch.object(match, 'ROOT', root), patch.object(repin, 'ROOT', root), \
                        patch.object(binding_rename, 'ROOT', root):
                    fixture = OwnerFixture(root)
                    for location in ('evidence/pipe_a.json', 'evidence/pipe_b.json'):
                        document = json.loads((root / location).read_bytes())
                        document['evidence_files'][key] = document['evidence_files'].pop('src/pipe.c')
                        fixture.write_json(location, document)
                    for row in fixture.state['functions']:
                        document = json.loads((root / row['proof']['bindings_path']).read_bytes())
                        row['proof'].update(binding_rename.proof_fields(document, row['id']))
                    write(root / 'recovery.json', json.dumps(fixture.state).encode())
                    plan = repin.binding_renames(fixture.state, fixture.plan(fixture.review()), {'src/pipe.c'})
                    self.assertEqual(sorted(plan.rows), ['m.p1', 'm.p2', 'm.p3'])
                    for location, (_, descendant) in plan.manifests.items():
                        self.assertEqual(descendant['evidence_files'][key], sha(fixture.PIPE_AFTER))
                        self.assertNotIn('src/pipe.c', descendant['evidence_files'])
                        self.assertEqual(json.loads((root / location).read_bytes())['evidence_files'][key],
                                         sha(fixture.PIPE_BEFORE))


@needs_junctions
class Confinement(Fixture):
    def test_confined_path_refuses_every_link_component(self):
        outside = self.base / 'outside'
        write(outside / 'unit.c', self.old)
        junction(self.root / 'src/escape', outside)
        with self.assertRaisesRegex(ValueError, 'through a link'):
            confined_path(self.root, ['src', 'escape', 'unit.c'])
        self.assertEqual(confined_path(self.root, ['src', 'missing', 'x.c']), self.root / 'src/missing/x.c')
        self.assertEqual(confined_path(self.root, ['src', 'unit.c']), self.root / 'src/unit.c')
        # One pass's cache only remembers accepted components; a link is refused on every call.
        verified = set()
        for _ in range(2):
            self.assertEqual(checked_path(self.root, ['src', 'unit.c'], verified), self.root / 'src/unit.c')
            self.assertEqual(checked_path(self.root, ['src', 'missing', 'x.c'], verified), self.root / 'src/missing/x.c')
            with self.assertRaisesRegex(ValueError, 'through a link'):
                checked_path(self.root, ['src', 'escape', 'unit.c'], verified)
        self.assertEqual(verified, {str(self.root / 'src'), str(self.root / 'src/unit.c')})

    def test_snapshot_junction_into_a_unicode_equivalent_sibling_refuses(self):
        """Finding 3: K and the Kelvin sign are distinct directories that pathlib compares equal."""
        twin = self.base / '\u212a'
        write(twin / self.address, self.old)
        junction(self.root / 'evidence/repin_snapshots', twin / 'evidence/repin_snapshots')
        self.assertFalse((self.root / 'evidence').samefile(twin / 'evidence'))
        for name in ('src/unit.c', FS + 'src/unit.c', BS + 'src\\unit.c'):
            with self.subTest(name=name):
                self.assertEqual(self.outcomes(name, self.address), [True, True, True])

    def test_snapshot_junction_outside_refuses(self):
        outside = self.base / 'outside_snapshots'
        write(outside / (self.h + '.txt'), self.old)
        junction(self.root / 'evidence/repin_snapshots', outside)
        for name in ('src/unit.c', FS + 'src/unit.c'):
            with self.subTest(name=name):
                self.assertEqual(self.outcomes(name, self.address), [True, True, True])

    def test_source_junction_refuses_before_any_redirect(self):
        """Finding 4: provenance confinement is decided before a historical snapshot replaces it."""
        write(self.root / self.address, self.old)
        targets = {'escape': self.base / 'outside_source', 'twin': self.base / '\u212a' / 'src' / 'twin'}
        for folder, target in targets.items():
            write(target / 'unit.c', self.old)
            junction(self.root / 'src' / folder, target)
        for folder in targets:
            for name in ('src/%s/unit.c' % folder, FS + 'src/%s/unit.c' % folder, BS + 'src\\%s\\unit.c' % folder):
                with self.subTest(name=name):
                    # snapshot_path alone only addresses the snapshot; both readers refuse the provenance.
                    self.assertEqual(self.outcomes(name, self.address)[1:], [True, True])
                    row = self.fixture(name)
                    with self.assertRaisesRegex(ValueError, 'through a link'):
                        match.ReferenceSnapshot().read('evidence/b.json', row)

    def test_original_assets_junction_refuses(self):
        original = self.base / 'original'
        write(self.base / 'elsewhere/X.DLL', self.old)
        junction(original / 'assets', self.base / 'elsewhere')
        with self.assertRaises(ValueError):
            original_evidence_path(self.root, 'assets/X.DLL', original)
        (original / 'assets').rmdir()
        write(original / 'assets/X.DLL', self.old)
        self.assertEqual(original_evidence_path(self.root, 'assets/X.DLL', original), (original / 'assets/X.DLL', True))

    def test_changed_and_owner_sources_refuse_links(self):
        write(self.base / 'outside/unit.c', self.old)
        junction(self.root / 'src/escape', self.base / 'outside')
        with self.assertRaises(ValueError):
            repin.canonical_change('src/escape/unit.c')
        with self.assertRaises(ValueError):
            binding_rename._canonical_owner_source('src/escape/unit.c')
        self.assertEqual(repin.canonical_change('src/unit.c'), self.root / 'src/unit.c')


if __name__ == '__main__':
    unittest.main()
