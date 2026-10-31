"""reviewed-binding-rename-v1: derivation refusals and an end-to-end repin.apply() transaction.

No compiler and no original image: tiny real COFF objects stand in for the compiled units and a
fake comparator emulates relocated_compare's name/addend/destination checks against fixed native
destinations, so the transaction, descendant manifests, proofs and preservation run unmodified.
"""
import contextlib
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import binding_rename
import match
import repin
import unit_refresh
from coff import COFF

T = 'assets/pc/HERCULES/ENG1.DLL'
OWNER_SOURCE = b'int owner[8];' + bytes([10])
RELOCATED = 'FUNCTION_MATCH_RELOCATED'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write_coff(path, functions, externals):
    """functions: [(name, code, [(offset, symbol, type)])]; externals: [(name, coff type)]."""
    symbols = []
    for number, (name, code, relocations) in enumerate(functions, 1):
        symbols.append(('.text', 0, number, 0, 3, [struct.pack('<IHHIHB3x', len(code), len(relocations), 0, 0, 0, 1)]))
        symbols.append((name, 0, number, 0x20, 2, []))
    symbols += [(name, 0, 0, kind, 2, []) for name, kind in externals]
    index, position = {}, 0
    for name, *_rest in symbols:
        if name != '.text':
            index[name] = position
        position += 1 + len(_rest[-1])
    header = 20 + 40 * len(functions)
    raw, data = [], bytearray()
    for name, code, relocations in functions:
        data_ptr = header + len(data)
        data += code
        reloc_ptr = header + len(data)
        for offset, symbol, kind in relocations:
            data += struct.pack('<IIH', offset, index[symbol], kind)
        raw.append((len(code), data_ptr, reloc_ptr if relocations else 0, len(relocations)))
    strings, table = bytearray(), []
    for name, value, section, kind, storage, aux in symbols:
        if len(name) <= 8:
            field = name.encode().ljust(8, b'\0')
        else:
            field = struct.pack('<II', 0, 4 + len(strings))
            strings += name.encode() + b'\0'
        table.append(struct.pack('<8sIhHBB', field, value, section, kind, storage, len(aux)) + b''.join(aux))
    symptr = header + len(data)
    out = struct.pack('<HHIIIHH', 0x14c, len(functions), 0, symptr, position, 0, 0)
    for size, data_ptr, reloc_ptr, count in raw:
        out += struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, size, data_ptr, reloc_ptr, 0, count, 0, 0x60501020)
    out += bytes(data) + b''.join(table) + struct.pack('<I', 4 + len(strings)) + bytes(strings)
    Path(path).write_bytes(out)
    return path


def function_record(rva, offsets, evidence='recovery.json#functions[m.callee]'):
    return dict(kind='function', rva=rva, size=16, evidence=[evidence],
                references=[dict(offset=o, type=20, addend=0) for o in offsets])


def owner_record(rva, size, offsets_addends, symbol):
    return dict(kind='bss', rva=rva, size=size, evidence=['evidence/owner_review.json'],
                owner=dict(source='src/owner.c', flags=['/O2'], symbol=symbol),
                identity=dict(rva=0x3000, size=6, operand=1, sha256='a' * 64),
                references=[dict(offset=o, type=6, addend=a) for o, a in offsets_addends])


class Fixture:
    """A miniature repository: callee unit, caller unit, owner user, parent manifests, review."""

    CALLER_BEFORE = b'int caller(void) { old_callee(); return cell; }\n'
    CALLER_AFTER = b'int caller(void) { new_callee(); return owner[4]; }\n'
    CALLEE_BEFORE = b'void old_callee(void) {}\n'
    CALLEE_AFTER = b'void new_callee(void) {}\n'
    OTHER = b'int other(void) { return owner[0]; }\n'
    # Native destinations are fixed facts: 0x1000 callee entry; 0x5010 the cell = owner + 0x10.

    def __init__(self, root):
        self.root = root
        for folder in ('src', 'work/cand', 'evidence/repin_snapshots', 'toolchains', 'objects'):
            (root / folder).mkdir(parents=True, exist_ok=True)
        (root / 'toolchains/manifest.json').write_text('{}')
        self.write('src/owner.c', OWNER_SOURCE)
        self.write('src/caller.c', self.CALLER_BEFORE)
        self.write('src/callee.c', self.CALLEE_BEFORE)
        self.write('src/other.c', self.OTHER)
        self.write('work/cand/caller.c', self.CALLER_AFTER)
        self.write('work/cand/callee.c', self.CALLEE_AFTER)
        caller_code = b'\xe8\0\0\0\0\xa1\0\0\0\0\xc3' + b'\x90' * 5
        caller_after = b'\xe8\0\0\0\0\xa1\x10\0\0\0\xc3' + b'\x90' * 5
        self.objects = {
            sha(self.CALLER_BEFORE): write_coff(root / 'objects/caller_before.obj', [
                ('_caller', caller_code, [(1, '_old_callee', 20), (6, '_cell', 6)])], [('_old_callee', 0x20), ('_cell', 0)]),
            sha(self.CALLER_AFTER): write_coff(root / 'objects/caller_after.obj', [
                ('_caller', caller_after, [(1, '_new_callee', 20), (6, '_owner', 6)])], [('_new_callee', 0x20), ('_owner', 0)]),
            sha(self.CALLEE_BEFORE): write_coff(root / 'objects/callee_before.obj', [('_old_callee', b'\xc3' + b'\x90' * 15, [])], []),
            sha(self.CALLEE_AFTER): write_coff(root / 'objects/callee_after.obj', [('_new_callee', b'\xc3' + b'\x90' * 15, [])], []),
            sha(self.OTHER): write_coff(root / 'objects/other.obj', [
                ('_other', b'\x90\x90\xa1\0\0\0\0\xc3' + b'\x90' * 8, [(3, '_owner', 6)])], [('_owner', 0)]),
        }
        self.NATIVE = {('m.caller', 1): 0x1000, ('m.caller', 6): 0x5010, ('m.other', 3): 0x5000}
        self.parent = dict(schema=1, evidence_files={'src/caller.c': sha(self.CALLER_BEFORE)}, functions={
            'm.caller': dict(symbols={'_old_callee': function_record(0x1000, [1]),
                                      '_cell': owner_record(0x5010, 4, [(6, 0)], '_owner')})})
        # (the cell record above is an alias of the owner in the parent manifest)
        self.parent['functions']['m.caller']['symbols']['_cell']['source_alias'] = dict(kind='compile-renaming',
                                                                                         header='src/alias.h')
        self.owner_doc = dict(schema=1, evidence_files={'src/owner.c': sha(OWNER_SOURCE)}, functions={
            'm.other': dict(symbols={'_owner': owner_record(0x5000, 0x20, [(3, 0)], '_owner')})})
        self.write_json('evidence/parent.json', self.parent)
        self.write_json('evidence/owner.json', self.owner_doc)
        self.state = dict(schema_version=1, functions=[
            self.row('m.callee', '_old_callee', 'src/callee.c', 'FUNCTION_MATCH', 0x1000),
            self.row('m.caller', '_caller', 'src/caller.c', RELOCATED, 0x2000, 'evidence/parent.json'),
            self.row('m.other', '_other', 'src/other.c', RELOCATED, 0x2100, 'evidence/owner.json')])

    def write(self, name, data):
        (self.root / name).write_bytes(data)

    def write_json(self, name, document):
        (self.root / name).write_text(json.dumps(document, indent=1) + '\n')

    def row(self, rid, symbol, source, status, rva, bindings=None):
        proof = dict(scope='s', source_sha256=sha((self.root / source).read_bytes()), toolchain='fixture')
        row = dict(id=rid, status=status, symbol=symbol, source=source, target=T, rva=hex(rva), size=16,
                   flags=['/O2'], proof=proof)
        if bindings:
            document = json.loads((self.root / bindings).read_text())
            proof.update(bindings_path=bindings, **binding_rename.proof_fields(document, rid))
        return row

    def verify(self, rows, original_root=None, relocation_candidates=None, **_):
        """Fake comparator: names, addends, destinations and binding digests, never the compiler."""
        results = []
        for row in rows:
            source = (self.root / row['source']).read_bytes()
            if sha(source) != row['proof']['source_sha256']:
                raise ValueError('Canonical source differs from accepted proof: ' + row['id'])
            obj = self.objects[sha(source)]
            section = COFF(obj).function(row['symbol'])
            result = dict(id=row['id'], exact=True, scope='s', compiled_sha256=sha(section['data']), target_sha256='t',
                          object=str(obj), object_sha256=sha(obj.read_bytes()), compile_inputs={}, command=['cl'],
                          source_sha256=sha(source), toolchain='fixture')
            if row['status'] == RELOCATED:
                binding, evidence, _ = match.reference_inputs(row['proof']['bindings_path'], row)
                if (binding_rename.reference_digest(binding) != row['proof']['bindings_sha256'] or
                        binding_rename.reference_digest(evidence) != row['proof']['reference_evidence_sha256']):
                    raise ValueError('Accepted reference bindings changed: ' + row['id'])
                exact = set(binding['symbols']) == {r['symbol'] for r in section['relocations']}
                receipts = []
                for rel in section['relocations']:
                    record = binding['symbols'].get(rel['symbol'], {})
                    addend = struct.unpack_from('<i', section['data'], rel['offset'])[0]
                    uses = [u for u in record.get('references', []) if u['offset'] == rel['offset']]
                    exact &= uses == [dict(offset=rel['offset'], type=rel['type'], addend=addend)]
                    native = self.NATIVE[(row['id'], rel['offset'])]
                    exact &= record.get('rva', -1) + addend == native and 0 <= addend < record.get('size', 0)
                    receipts.append(dict(offset=rel['offset'], type=rel['type'], symbol=rel['symbol'], addend=addend,
                                         kind=record.get('kind'), destination_rva=record.get('rva', 0) + addend,
                                         target_destination_rva=native, evidence=record.get('evidence')))
                result.update(exact=exact, relocated_equal=exact, references=receipts,
                              bindings_sha256=binding_rename.reference_digest(binding))
            results.append(result)
        return results, dict(compiler_calls=0, rows=len(rows))

    def review(self, **changes):
        parent_sha = sha((self.root / 'evidence/parent.json').read_bytes())
        owner_sha = sha((self.root / 'evidence/owner.json').read_bytes())
        rebind = {'symbol': '_owner', 'from': dict(row='m.other', bindings_path='evidence/owner.json',
                                                   bindings_sha256=owner_sha)}
        review = dict(schema=1, profile=binding_rename.PROFILE, reason='Callers name the owner and the renamed callee.',
                      definitions={'m.callee': dict(before='_old_callee', after='_new_callee')},
                      bindings={'m.caller': dict(bindings_path='evidence/parent.json', bindings_sha256=parent_sha,
                                                 renames={'_old_callee': '_new_callee'}, rebinds={'_cell': rebind})})
        review.update(changes)
        self.write_json('evidence/review.json', review)
        return dict(path='evidence/review.json', sha256=sha((self.root / 'evidence/review.json').read_bytes()))

    def plan(self, review, caller_candidate=None):
        caller = caller_candidate or self.CALLER_AFTER
        self.write('work/cand/caller.c', caller)
        state = (self.root / 'recovery.json').read_bytes()
        return dict(state_sha256=sha(state), promotions=[], binding_review=review, sources={
            'src/caller.c': dict(candidate='work/cand/caller.c', before_sha256=sha(self.CALLER_BEFORE),
                                 candidate_sha256=sha(caller)),
            'src/callee.c': dict(candidate='work/cand/callee.c', before_sha256=sha(self.CALLEE_BEFORE),
                                 candidate_sha256=sha(self.CALLEE_AFTER))})


@contextlib.contextmanager
def no_lock():
    yield


class EndToEndApplyTests(unittest.TestCase):
    def setUp(self):
        base = Path(__file__).resolve().parents[1] / 'work'
        base.mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=base)
        self.root = Path(self.tmp.name).resolve()
        self.fx = Fixture(self.root)
        (self.root / 'recovery.json').write_text(json.dumps(self.fx.state, indent=2) + '\n')
        import promote
        self.patches = [patch.object(repin, 'ROOT', self.root), patch.object(binding_rename, 'ROOT', self.root),
                        patch.object(match, 'ROOT', self.root), patch.object(promote, 'lock', no_lock),
                        patch.object(promote, 'JOURNAL', self.root / 'work/promotion-journal.json'),
                        patch.object(promote, 'assert_verified_inputs', lambda *a, **k: None),
                        patch.object(repin, 'frozen_before', lambda *a, **k: None),
                        patch.object(match, 'verify_rows', self.fx.verify)]
        for p in self.patches:
            p.start()

    def tearDown(self):
        for p in reversed(self.patches):
            p.stop()
        self.tmp.cleanup()

    def before_report(self):
        state = json.loads((self.root / 'recovery.json').read_bytes())
        results, _ = self.fx.verify(match.accepted_rows(state['functions']))
        path = self.root / 'work/before.json'
        path.write_text(json.dumps(dict(results=results, state_sha256=sha((self.root / 'recovery.json').read_bytes()),
                                        historical_evidence={})))
        return path

    def run_apply(self, plan):
        plan_path = self.root / 'work/plan.json'
        plan_path.write_text(json.dumps(plan))
        return repin.apply(plan_path, self.before_report(), self.root, self.root / 'work/repin')

    def test_renamed_callee_and_rebound_member_are_reissued_and_reverified(self):
        receipt = self.run_apply(self.fx.plan(self.fx.review()))
        state = json.loads((self.root / 'recovery.json').read_bytes())
        rows = {r['id']: r for r in state['functions']}
        self.assertEqual(rows['m.callee']['symbol'], '_new_callee')
        self.assertEqual(rows['m.callee']['symbol_history'][0]['before'], '_old_callee')
        caller = rows['m.caller']['proof']
        self.assertTrue(caller['bindings_path'].startswith('evidence/rename_'))
        manifest = json.loads((self.root / caller['bindings_path']).read_bytes())
        binding = manifest['functions']['m.caller']['symbols']
        self.assertEqual(binding['_new_callee'], function_record(0x1000, [1]))  # identical record, new key
        owner = copy.deepcopy(self.fx.owner_doc['functions']['m.other']['symbols']['_owner'])
        owner['references'] = [dict(offset=6, type=6, addend=0x10)]  # same destination 0x5010
        self.assertEqual(binding['_owner'], owner)
        self.assertEqual(manifest['evidence_files']['evidence/parent.json'],
                         sha((self.root / 'evidence/parent.json').read_bytes()))
        self.assertEqual(manifest['evidence_files']['src/owner.c'], sha(OWNER_SOURCE))
        self.assertEqual(json.loads((self.root / 'evidence/parent.json').read_bytes()), self.fx.parent)
        history = rows['m.caller']['reference_rename_history'][0]
        self.assertEqual(history['proof']['bindings_path'], 'evidence/parent.json')
        self.assertEqual(rows['m.other']['proof']['bindings_path'], 'evidence/owner.json')  # untouched
        self.assertEqual((self.root / 'src/caller.c').read_bytes(), Fixture.CALLER_AFTER)
        self.assertEqual(receipt['binding_renames']['rows']['m.caller'],
                         {'_old_callee': ['_new_callee', 0], '_cell': ['_owner', 0x10]})
        caller_section = COFF(self.fx.objects[sha(Fixture.CALLER_AFTER)]).function('_caller')
        self.assertEqual(caller['compiled_sha256'], sha(caller_section['data']))  # only the addend field moved

    def test_stale_callee_reference_rolls_back_everything(self):
        stale = b'int caller(void) { old_callee(); return owner[4]; }\n'
        self.fx.objects[sha(stale)] = write_coff(self.root / 'objects/caller_stale.obj', [
            ('_caller', b'\xe8\0\0\0\0\xa1\x10\0\0\0\xc3' + b'\x90' * 5, [(1, '_old_callee', 20), (6, '_owner', 6)])],
            [('_old_callee', 0x20), ('_owner', 0)])
        before = (self.root / 'recovery.json').read_bytes()
        with self.assertRaises(ValueError):
            self.run_apply(self.fx.plan(self.fx.review(), caller_candidate=stale))
        self.assertEqual((self.root / 'recovery.json').read_bytes(), before)
        self.assertEqual((self.root / 'src/caller.c').read_bytes(), Fixture.CALLER_BEFORE)
        self.assertEqual((self.root / 'src/callee.c').read_bytes(), Fixture.CALLEE_BEFORE)
        self.assertEqual(list((self.root / 'evidence').glob('rename_*.json')), [])

    def test_unreviewed_rename_of_an_accepted_reference_fails_closed(self):
        review = self.fx.review(bindings={})
        with self.assertRaises(ValueError):
            self.run_apply(self.fx.plan(review))
        self.assertEqual((self.root / 'src/caller.c').read_bytes(), Fixture.CALLER_BEFORE)


class DeriveTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name).resolve()
        self.fx = Fixture(self.root)
        self.scope = patch.object(binding_rename, 'ROOT', self.root)
        self.scope.start()

    def tearDown(self):
        self.scope.stop()
        self.tmp.cleanup()

    def set_owner(self, **fields):
        """Change the accepted owner record and keep its accepted row's proof consistent."""
        record = self.fx.owner_doc['functions']['m.other']['symbols']['_owner']
        for key, value in fields.items():
            record[key] = value
        self.fx.write_json('evidence/owner.json', self.fx.owner_doc)
        row = next(r for r in self.fx.state['functions'] if r['id'] == 'm.other')
        row['proof'].update(binding_rename.proof_fields(self.fx.owner_doc, 'm.other'))

    def derive(self, mutate=None, changed=('src/caller.c', 'src/callee.c')):
        review_item = self.fx.review()
        review = json.loads((self.root / review_item['path']).read_bytes())
        if mutate:
            mutate(review)
        documents = binding_rename.parent_documents(review)
        return binding_rename.derive(self.fx.state, review, documents, review_item['path'], 'c' * 64, set(changed))

    def test_owner_proof_must_match_its_manifest(self):
        record = self.fx.owner_doc['functions']['m.other']['symbols']['_owner']
        record['size'] = 0x40
        self.fx.write_json('evidence/owner.json', self.fx.owner_doc)  # proof not updated: not the accepted record
        with self.assertRaisesRegex(ValueError, 'differs from its proof'):
            self.derive()

    def test_valid_review_derives_one_descendant_manifest(self):
        plan = self.derive()
        self.assertEqual(plan.definitions, {'m.callee': '_new_callee'})
        self.assertEqual(plan.rows['m.caller'], {'_old_callee': ('_new_callee', 0), '_cell': ('_owner', 0x10)})
        (path, document), = plan.manifests.values()
        self.assertEqual(document['binding_rename_review'], 'evidence/review.json')

    def test_refusals(self):
        bindings = lambda r: r['bindings']['m.caller']
        cases = {
            'storage rename': lambda r: bindings(r).update(renames={'_cell': '_owner'}, rebinds={}),
            'parent changed': lambda r: bindings(r).update(bindings_sha256='0' * 64),
            'owner outside': None,
            'unchanged source': None,
            'stale definition': lambda r: r['definitions']['m.callee'].update(before='_other_name'),
            'two names': lambda r: bindings(r)['renames'].update({'_old_callee': '_third'}) or
                         r['definitions']['m.callee'].update(after='_new_callee'),
            'empty reason': lambda r: r.update(reason=' '),
            'overlap': lambda r: bindings(r)['renames'].update({'_cell': '_x'}),
            'collision': 'collision',
        }
        for label, mutate in cases.items():
            with self.subTest(label):
                if label == 'owner outside':  # the cell would sit 4 bytes before the owner
                    self.set_owner(rva=0x5014)
                    with self.assertRaisesRegex(ValueError, 'inside its owner'):
                        self.derive()
                    self.set_owner(rva=0x5000)
                    continue
                if label == 'collision':  # two accepted definitions cannot take one name
                    with self.assertRaisesRegex(ValueError, 'Duplicate definition rename'):
                        self.derive(lambda r: r['definitions'].update({'m.other': dict(before='_other', after='_new_callee')}),
                                    changed=('src/caller.c', 'src/callee.c', 'src/other.c'))
                    continue
                if label == 'unchanged source':
                    with self.assertRaises(ValueError):
                        self.derive(changed=('src/callee.c',))
                    continue
                with self.assertRaises(ValueError):
                    self.derive(mutate)

    def test_anchor_inside_the_caller_is_not_independent(self):
        self.set_owner(identity=dict(rva=0x2004, size=6, operand=1, sha256='a' * 64))
        with self.assertRaisesRegex(ValueError, 'anchor lies inside the caller'):
            self.derive()


class ProtectedRenameRefreshTests(unittest.TestCase):
    """unit_refresh admits an accepted definition renamed in place only when it is reviewed."""

    def objects(self, root):
        before = COFF(write_coff(root / 'b.obj', [('_callee', b'\xc3' + b'\x90' * 15, []),
                                                  ('_user', b'\xe8\0\0\0\0\xc3' + b'\x90' * 10, [(1, '_callee', 20)])], []))
        after = COFF(write_coff(root / 'a.obj', [('_renamed', b'\xc3' + b'\x90' * 15, []),
                                                 ('_user', b'\xe8\0\0\0\0\xc3' + b'\x90' * 10, [(1, '_renamed', 20)])], []))
        return before, after

    def test_reviewed_definition_and_reference_rename(self):
        with tempfile.TemporaryDirectory() as directory:
            before, after = self.objects(Path(directory))
            record = lambda coff, name: next(r for r in unit_refresh.symbol_records(coff) if r['name'] == name)
            declaration = dict(profile='reviewed-unit-delta-v2', removed=[], added=[], rebases=[], renames=[],
                               symbol_renames=[dict(before=record(before, '_callee'), after=record(after, '_renamed'))],
                               review='evidence/r.json', review_sha256='0' * 64)
            result = unit_refresh.check_delta(before, after, declaration, {'_callee': '_renamed', '_user': '_user'},
                                              allow_protected_renames=True)
            self.assertEqual(result['protected_renamed_references'][0]['function'], '_user')
            for protected, allow in (({'_callee', '_user'}, True), ({'_callee': '_renamed', '_user': '_user'}, False),
                                     ({'_callee': '_other', '_user': '_user'}, True)):
                with self.assertRaises(ValueError):
                    unit_refresh.check_delta(before, after, declaration, protected, allow_protected_renames=allow)


if __name__ == '__main__':
    unittest.main()
