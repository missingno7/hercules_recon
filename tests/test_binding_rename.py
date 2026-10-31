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


def write_coff(path, functions, externals, labels=()):
    """functions: [(name, code, [(offset, symbol, type)])]; externals: [(name, coff type)];
    labels: [(name, value, section)] as VC5 compiler labels (storage class 6)."""
    symbols = []
    for number, (name, code, relocations) in enumerate(functions, 1):
        symbols.append(('.text', 0, number, 0, 3, [struct.pack('<IHHIHB3x', len(code), len(relocations), 0, 0, 0, 1)]))
        symbols.append((name, 0, number, 0x20, 2, []))
    symbols += [(name, value, section, 0, 6, []) for name, value, section in labels]
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
                # Same provenance fields as match.verify_rows: the manifest path read and its evidence inventory.
                result.update(exact=exact, relocated_equal=exact, references=receipts,
                              bindings_sha256=binding_rename.reference_digest(binding),
                              bindings_path=row['proof']['bindings_path'],
                              reference_evidence_sha256=binding_rename.reference_digest(evidence))
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
        # The fresh gate verified the caller against the descendant manifest named by its new proof.
        fresh = next(r for r in receipt['results'] if r['id'] == 'm.caller')
        self.assertEqual({k: fresh[k] for k in binding_rename.PROVENANCE},
                         {k: caller[k] for k in binding_rename.PROVENANCE})

    def test_fresh_result_must_name_the_reissued_manifest(self):
        # A comparator that still reads the parent manifest for the re-issued caller fails closed.
        real = self.fx.verify

        def parent_reader(rows, **kw):
            results, stats = real(rows, **kw)
            for result in results:
                if result['id'] == 'm.caller' and 'bindings_path' in result:
                    result['bindings_path'] = 'evidence/parent.json'
            return results, stats
        before = (self.root / 'recovery.json').read_bytes()
        plan = self.fx.plan(self.fx.review())
        before_report = self.before_report()
        with patch.object(match, 'verify_rows', parent_reader), self.assertRaisesRegex(ValueError, 'all verified findings m.caller'):
            plan_path = self.root / 'work/plan.json'
            plan_path.write_text(json.dumps(plan))
            repin.apply(plan_path, before_report, self.root, self.root / 'work/repin')
        self.assertEqual((self.root / 'recovery.json').read_bytes(), before)

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

    def test_reissued_findings_compare_descendant_provenance(self):
        # Shape of the first real use (W1, eng3.1a320): only the manifest path, binding digest and
        # evidence inventory move to the descendant; references match after the reviewed rename.
        use = dict(offset=10, type=20, symbol='_old', addend=0, kind='function', destination_rva=0x1000,
                   target_destination_rva=0x1000, evidence=['recovery.json#functions[m.callee]'], one_past=False)
        before = dict(exact=True, scope='s', bindings_path='evidence/parent.json', bindings_sha256='p',
                      reference_evidence_sha256='pe', references=[use])
        provenance = dict(bindings_path='evidence/rename_x.json', bindings_sha256='d', reference_evidence_sha256='de')
        after = dict(before, references=[dict(use, symbol='_new')], **provenance)
        changes = {'_old': ('_new', 0)}
        self.assertEqual(binding_rename.expected_findings(before, changes, None, provenance),
                         binding_rename.observed_findings(after, changes, provenance))
        self.assertNotEqual(binding_rename.expected_findings(before, changes),  # the W1 failure
                            binding_rename.observed_findings(after, changes))
        wrong = dict(provenance, bindings_path='evidence/other.json')
        self.assertNotEqual(binding_rename.expected_findings(before, changes, None, wrong),
                            binding_rename.observed_findings(after, changes, wrong))
        with self.assertRaises(ValueError):
            binding_rename.expected_findings(before, changes, None, dict(bindings_path='evidence/rename_x.json'))

    def test_anchor_inside_the_caller_is_not_independent(self):
        self.set_owner(identity=dict(rva=0x2004, size=6, operand=1, sha256='a' * 64))
        with self.assertRaisesRegex(ValueError, 'anchor lies inside the caller'):
            self.derive()


class LocalLabelRenameTests(unittest.TestCase):
    """VC5 numbers a jump table's own label after the whole unit: reviewed $L renames, nothing else."""

    SOURCE = 'src/table.c'

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name).resolve()
        (self.root / 'evidence').mkdir()
        self.scope = patch.object(binding_rename, 'ROOT', self.root)
        self.scope.start()
        self.addCleanup(self.scope.stop)
        self.addCleanup(self.tmp.cleanup)
        self.table = dict(kind='local', role='table', rva=0x112c, size=16, evidence=['evidence/review.json#tables.m.a'],
                          references=[dict(offset=87, type=6, addend=0)])
        self.case = dict(kind='local', role='code', rva=0x10ab, size=1, evidence=['evidence/review.json#cases.m.a'],
                         references=[dict(offset=220, type=6, addend=0)])
        self.binding_a = dict(symbols={'$L167': self.table, '$L104': self.case,
                                       '_callee': function_record(0x1000, [66])},
                              tables=[dict(offset=220, size=16, evidence=['evidence/review.json#tables.m.a'])])
        self.binding_b = dict(symbols={'$L167': copy.deepcopy(self.table)})  # another row, the same label number
        self.document = dict(schema=1, evidence_files={})
        self.path = 'evidence/parent.json'
        self.publish()

    def publish(self):
        self.document['functions'] = {'m.a': self.binding_a, 'm.b': self.binding_b}
        (self.root / self.path).write_text(json.dumps(self.document, indent=1) + '\n')
        self.state = dict(functions=[self.row('m.a'), self.row('m.b')])

    def row(self, rid):
        proof = dict(scope='s', bindings_path=self.path,
                     bindings_sha256=binding_rename.reference_digest(self.document['functions'][rid]))
        return dict(id=rid, status=RELOCATED, symbol='_' + rid, source=self.SOURCE, target=T, rva='0x1000', size=16,
                    flags=['/O2'], proof=proof)

    def derive(self, renames, rows=('m.a',), mutate=None):
        sha_ = sha((self.root / self.path).read_bytes())
        items = {rid: dict(bindings_path=self.path, bindings_sha256=sha_,
                           renames=renames[rid] if isinstance(renames, dict) and rid in renames else renames)
                 for rid in rows}
        review = dict(schema=1, profile=binding_rename.PROFILE, reason='Labels renumbered by a larger unit.', bindings=items)
        if mutate:
            mutate(review)
        return binding_rename.derive(self.state, review, binding_rename.parent_documents(review), 'evidence/r.json',
                                     'c' * 64, {self.SOURCE})

    def test_a_table_label_is_renamed_and_nothing_else_changes(self):
        plan = self.derive({'$L167': '$L223'})
        self.assertEqual(plan.rows, {'m.a': {'$L167': ('$L223', 0)}})
        self.assertEqual(plan.callee_names, {})  # labels are never definitions
        (path, document), = plan.manifests.values()
        derived = document['functions']['m.a']
        self.assertEqual(derived['symbols']['$L223'], self.table)  # kind, role, rva, size, evidence and uses
        self.assertEqual(derived['tables'], self.binding_a['tables'])  # tables name offsets, never labels
        self.assertEqual(sorted(derived['symbols']), ['$L104', '$L223', '_callee'])
        self.assertEqual(document['functions']['m.b'], self.binding_b)  # other rows keep their key
        self.assertEqual(json.loads((self.root / self.path).read_bytes()), self.document)  # parent is provenance

    def test_labels_are_scoped_to_their_row(self):
        plan = self.derive({'m.a': {'$L167': '$L223'}, 'm.b': {'$L167': '$L300'}}, rows=('m.a', 'm.b'))
        self.assertEqual((plan.rows['m.a'], plan.rows['m.b']), ({'$L167': ('$L223', 0)}, {'$L167': ('$L300', 0)}))

    def test_a_renumbering_may_shift_several_labels_in_any_order(self):
        for renames in ({'$L104': '$L105', '$L167': '$L104'}, {'$L167': '$L104', '$L104': '$L105'},
                        {'$L167': '$L104', '$L104': '$L167'}):
            with self.subTest(renames=renames):
                plan = self.derive(renames)
                symbols = plan.manifests[self.path][1]['functions']['m.a']['symbols']
                originals = {'$L104': self.case, '$L167': self.table}
                for old, (new, delta) in plan.rows['m.a'].items():
                    self.assertEqual((symbols[new], delta), (originals[old], 0))
                self.assertEqual(len(symbols), len(self.binding_a['symbols']))

    def test_refusals(self):
        self.binding_a['symbols']['$L170'] = dict(self.case, role='section', rva=0x1000, size=16)
        self.binding_a['symbols']['$L171'] = dict(self.case, rva=0x10c0, references=[dict(offset=228, type=6, addend=0)])
        self.publish()
        for label, renames in {
                'new name is not a label': {'$L167': '_table'}, 'unnumbered label': {'$L167': '$Lx'},
                'lowercase': {'$L167': '$l223'}, 'suffix': {'$L167': '$L223a'}, 'prefix': {'$L167': 'x$L223'},
                'same name': {'$L167': '$L167'}, 'old record absent': {'$L999': '$L223'},
                'section role': {'$L170': '$L223'}, 'a function takes a label name': {'_callee': '$L223'},
                'collision with another record': {'$L167': '$L104'}, 'collision with an untouched record': {'$L167': '$L171'},
                'non-injective': {'$L167': '$L223', '$L104': '$L223'}, 'to a local': {'$L167': '_callee'}}.items():
            with self.subTest(label), self.assertRaises(ValueError):
                self.derive(renames)

    def test_a_local_record_renamed_to_a_non_label_name_or_a_non_label_local_is_refused(self):
        self.binding_a['symbols']['_odd'] = dict(self.case, rva=0x10c0, references=[dict(offset=228, type=6, addend=0)])
        self.publish()
        with self.assertRaisesRegex(ValueError, 'compiler labels'):
            self.derive({'_odd': '$L223'})

    def test_a_label_named_by_another_record_or_field_is_refused(self):
        for mutate in (lambda: self.binding_a['tables'][0].update(evidence=['$L167']),
                       lambda: self.binding_a['tables'][0].update(label='$L167'),
                       lambda: self.binding_a['symbols']['_callee']['evidence'].append('$L167'),
                       lambda: self.binding_a.update(coff_identity=dict(symbol='$L167'))):
            self.setUp()
            mutate()
            self.publish()
            with self.assertRaisesRegex(ValueError, 'referenced by name'):
                self.derive({'$L167': '$L223'})

    def declare_table(self, symbol='$L167'):
        self.binding_a['tables'] = [dict(offset=220, size=16, symbol=symbol, profile='coff-local-byte-table-v1',
                                         sha256='b' * 64, evidence=['evidence/review.json#tables.m.a'])]
        self.publish()

    def test_a_table_declaration_follows_its_renamed_label(self):
        self.declare_table()
        plan = self.derive({'$L167': '$L223'})
        derived = plan.manifests[self.path][1]['functions']['m.a']
        self.assertEqual(derived['symbols']['$L223'], self.table)  # the record is copied verbatim
        expected = copy.deepcopy(self.binding_a['tables'])
        expected[0]['symbol'] = '$L223'
        self.assertEqual(derived['tables'], expected)  # only the declared name moves
        self.assertEqual({k: v for k, v in derived.items() if k not in ('symbols', 'tables')},
                         {k: v for k, v in self.binding_a.items() if k not in ('symbols', 'tables')})

    def test_a_shifted_renumbering_renames_declarations_simultaneously(self):
        self.declare_table('$L167')
        plan = self.derive({'$L104': '$L105', '$L167': '$L104'})
        derived = plan.manifests[self.path][1]['functions']['m.a']
        self.assertEqual(derived['tables'][0]['symbol'], '$L104')
        self.assertEqual(derived['symbols']['$L104'], self.table)
        self.assertEqual(derived['symbols']['$L105'], self.case)

    def test_a_declaration_rename_may_not_collide(self):
        self.declare_table('$L167')
        self.binding_a['tables'].append(dict(offset=240, size=4, symbol='$L223', evidence=['x']))
        self.publish()
        with self.assertRaises(ValueError):
            self.derive({'$L167': '$L223'})

    def test_byte_table_receipts_accept_exactly_the_renamed_label(self):
        receipt = dict(symbol='$L167', offset=220, size=16, rva=0x112c, sha256='b' * 64, unique=True)
        use = dict(offset=87, type=6, symbol='$L167', addend=0, kind='local', destination_rva=0x112c,
                   target_destination_rva=0x112c, evidence=['e'], one_past=False)
        before = dict(exact=True, references=[use], byte_table_checks=[receipt])
        changes = {'$L167': ('$L223', 0)}
        after = dict(before, references=[dict(use, symbol='$L223')], byte_table_checks=[dict(receipt, symbol='$L223')])
        expected = binding_rename.expected_findings(before, changes)
        self.assertEqual(expected, binding_rename.observed_findings(after, changes))
        for wrong in (dict(after, byte_table_checks=[receipt]),  # receipt not renamed
                      dict(after, byte_table_checks=[dict(receipt, symbol='$L224')]),
                      dict(after, byte_table_checks=[dict(receipt, symbol='$L223', sha256='c' * 64)])):
            self.assertNotEqual(expected, binding_rename.observed_findings(wrong, changes))

    def test_local_renames_need_an_accepted_unchanged_binding(self):
        with self.assertRaisesRegex(ValueError, 'differs from its proof'):
            self.state['functions'][0]['proof']['bindings_sha256'] = '0' * 64
            self.derive({'$L167': '$L223'})

    def test_repin_findings_accept_exactly_the_renamed_local_symbol(self):
        use = dict(offset=87, type=6, symbol='$L167', addend=0, kind='local', destination_rva=0x112c,
                   target_destination_rva=0x112c, evidence=['evidence/review.json#tables.m.a'], one_past=False)
        other = dict(use, offset=66, symbol='$L104', destination_rva=0x10ab, target_destination_rva=0x10ab)
        before = dict(exact=True, scope='s', references=[use, other])
        changes = {'$L167': ('$L223', 0)}
        after = dict(before, references=[dict(use, symbol='$L223'), other])
        expected = binding_rename.expected_findings(before, changes)
        self.assertEqual(expected, binding_rename.observed_findings(after, changes))
        for wrong in (dict(before, references=[use, other]),  # not renamed
                      dict(after, references=[dict(use, symbol='$L224'), other]),  # another name
                      dict(after, references=[dict(use, symbol='$L223', addend=4), other]),
                      dict(after, references=[dict(use, symbol='$L223', destination_rva=0x1130), other]),
                      dict(after, references=[dict(use, symbol='$L223'), dict(other, symbol='$L105')])):  # unreviewed
            with self.subTest(wrong=wrong['references']):
                self.assertNotEqual(expected, binding_rename.observed_findings(wrong, changes))

    def test_the_gate_needs_the_descendant_key_and_still_checks_role_and_rva(self):
        import masked
        from test_matching import make_pe
        from test_relocated import binding, coff_file
        code = bytes.fromhex('ff248500000000c300000000')
        native = bytes.fromhex('ff2485') + struct.pack('<I', 0x401008) + b'\xc3' + struct.pack('<I', 0x401007)
        parent = dict(symbols={'$L167': binding('local', 0x1008, 4, (3, masked.DIR32, 0), role='table'),
                               '$L168': binding('local', 0x1007, 1, (8, masked.DIR32, 0), role='code')},
                      tables=[dict(offset=8, size=4, evidence=['independent switch table bounds'])])
        self.binding_a = parent
        self.publish()
        plan = self.derive({'$L167': '$L223', '$L168': '$L224'})
        descendant = plan.manifests[self.path][1]['functions']['m.a']

        def compare(bindings, table_value=8, case_value=7):
            obj, target = self.root / 'f.obj', self.root / 'f.dll'
            coff_file(obj, code, [(3, 1, masked.DIR32), (8, 2, masked.DIR32)],
                      [('_sample', 0, 1, 0x20), ('$L223', table_value, 1, 0), ('$L224', case_value, 1, 0)])
            make_pe(target)
            image = bytearray(target.read_bytes())
            image[0x400:0x400 + len(native)] = native
            target.write_bytes(image)
            return masked.relocated_compare(obj, '_sample', target, 0x1000, len(native), bindings)

        with self.assertRaisesRegex(ValueError, 'Binding inventory differs'):
            compare(parent)  # the pre-rename key no longer names the compiled label
        self.assertTrue(compare(descendant)['relocated_equal'])
        # The label moving (rva) or the table becoming a case label (role) is still refused by the gate.
        with self.assertRaises(ValueError):
            compare(descendant, table_value=9)
        wrong_role = copy.deepcopy(descendant)
        wrong_role['symbols']['$L223']['role'] = 'code'
        with self.assertRaises(ValueError):
            compare(wrong_role)


class LabelRefreshTests(unittest.TestCase):
    """unit_refresh admits VC5 label renumbering in place; accepted users still need their reviewed renames."""

    def objects(self, root, after_names):
        code = bytes.fromhex('ff248500000000c3') + bytes([0x90]) * 8
        before = COFF(write_coff(root / 'b.obj', [('_user', code, [(3, '$L10', 6)]), ('_other', code, [(3, '$L12', 6)])],
                                 [], labels=[('$L10', 8, 1), ('$L12', 8, 2)]))
        renamed = dict(zip(('$L10', '$L12'), after_names))
        after = COFF(write_coff(root / 'a.obj', [('_user', code, [(3, renamed['$L10'], 6)]),
                                                 ('_other', code, [(3, renamed['$L12'], 6)])],
                                [], labels=[(renamed['$L10'], 8, 1), (renamed['$L12'], 8, 2)]))
        return before, after

    def declaration(self, before, after, pairs):
        record = lambda coff, name: next(r for r in unit_refresh.symbol_records(coff) if r['name'] == name)
        return dict(profile='reviewed-unit-delta-v2', removed=[], added=[], rebases=[], renames=[],
                    symbol_renames=[dict(before=record(before, a), after=record(after, b)) for a, b in pairs],
                    review='evidence/r.json', review_sha256='0' * 64)

    def test_shifted_labels_are_renamed_in_place(self):
        with tempfile.TemporaryDirectory() as directory:
            before, after = self.objects(Path(directory), ('$L8', '$L10'))  # $L10 -> $L8 while $L12 -> $L10
            declaration = self.declaration(before, after, [('$L10', '$L8'), ('$L12', '$L10')])
            result = unit_refresh.check_delta(before, after, declaration, {'_user': '_user'},
                                              allow_protected_renames=True)
            self.assertEqual(result['symbol_renames'], {'$L10': '$L8', '$L12': '$L10'})
            self.assertEqual([r['function'] for r in result['protected_renamed_references']], ['_user'])
            with self.assertRaises(ValueError):  # an accepted user needs its reviewed binding rename
                unit_refresh.check_delta(before, after, declaration, {'_user': '_user'})
            with self.assertRaises(ValueError):  # an undeclared renumbering stays an undeclared delta
                unit_refresh.check_delta(before, after, self.declaration(before, after, [('$L10', '$L8')]), set())

    def test_labels_rename_only_between_label_names(self):
        with tempfile.TemporaryDirectory() as directory:
            before, after = self.objects(Path(directory), ('_x', '$L12'))
            with self.assertRaises(ValueError):
                unit_refresh.check_delta(before, after, self.declaration(before, after, [('$L10', '_x')]), set())


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


class OwnerFixture:
    """Four accepted RELOCATED rows over two binding documents; three use the owner src/pipe.c."""

    PIPE_BEFORE = b'ENG1Pipe g_pipe = {0};\n'
    PIPE_AFTER = b'ENG1Pipe g_pipe;\n'
    NATIVE = {('m.p1', 1): 0x6010, ('m.p2', 1): 0x6010, ('m.p3', 1): 0x6010, ('m.p4', 1): 0x1000}

    def __init__(self, root):
        self.root = root
        for folder in ('src', 'work/cand', 'evidence/repin_snapshots', 'toolchains', 'objects'):
            (root / folder).mkdir(parents=True, exist_ok=True)
        (root / 'toolchains/manifest.json').write_text('{}')
        self.write('src/pipe.c', self.PIPE_BEFORE)
        self.write('work/cand/pipe.c', self.PIPE_AFTER)
        self.objects = {}
        for name in ('p1', 'p2', 'p3', 'p4'):
            source = ('int %s(void) { return g_pipe.w[4]; }\n' % name).encode()
            self.write('src/%s.c' % name, source)
            if name == 'p4':
                code, relocation = b'\xe8\0\0\0\0\xc3' + b'\x90' * 10, (1, '_helper', 20)
                externals = [('_helper', 0x20)]
            else:
                code, relocation = b'\xa1\x10\0\0\0\xc3' + b'\x90' * 10, (1, '_pipe', 6)
                externals = [('_pipe', 0)]
            self.objects[sha(source)] = write_coff(root / ('objects/%s.obj' % name), [('_' + name, code, [relocation])],
                                                   externals)
        self.doc_a = dict(schema=1, evidence_files={'src/pipe.c': sha(self.PIPE_BEFORE), 'src/p4.c': sha(self.source('p4'))},
                          functions={'m.p1': dict(symbols={'_pipe': self.owner()}),
                                     'm.p2': dict(symbols={'_pipe': self.owner()}),
                                     'm.p4': dict(symbols={'_helper': function_record(0x1000, [1])})})
        self.doc_b = dict(schema=1, evidence_files={'src/pipe.c': sha(self.PIPE_BEFORE)},
                          functions={'m.p3': dict(symbols={'_pipe': self.owner()})})
        self.write_json('evidence/pipe_a.json', self.doc_a)
        self.write_json('evidence/pipe_b.json', self.doc_b)
        self.state = dict(schema_version=1, functions=[
            self.row('m.p1', 'p1', 'evidence/pipe_a.json', self.doc_a),
            self.row('m.p2', 'p2', 'evidence/pipe_a.json', self.doc_a),
            self.row('m.p3', 'p3', 'evidence/pipe_b.json', self.doc_b),
            self.row('m.p4', 'p4', 'evidence/pipe_a.json', self.doc_a)])

    @staticmethod
    def owner():
        record = owner_record(0x6000, 0x40, [(1, 0x10)], '_pipe')
        record['owner']['source'] = 'src/pipe.c'
        return record

    def source(self, name):
        return (self.root / ('src/%s.c' % name)).read_bytes()

    def write(self, name, data):
        (self.root / name).write_bytes(data)

    def write_json(self, name, document):
        (self.root / name).write_text(json.dumps(document, indent=1) + '\n')

    def row(self, rid, name, bindings, document):
        proof = dict(scope='s', source_sha256=sha(self.source(name)), toolchain='fixture', bindings_path=bindings,
                     **binding_rename.proof_fields(document, rid))
        return dict(id=rid, status=RELOCATED, symbol='_' + name, source='src/%s.c' % name, target=T, rva='0x2000', size=16,
                    flags=['/O2'], proof=proof)

    def verify(self, rows, original_root=None, relocation_candidates=None, **_):
        """Fake comparator: record names, addends, destinations and the loader-zero owner pin (never the compiler)."""
        results = []
        for row in rows:
            source = (self.root / row['source']).read_bytes()
            if sha(source) != row['proof']['source_sha256']:
                raise ValueError('Canonical source differs from accepted proof: ' + row['id'])
            obj = self.objects[sha(source)]
            section = COFF(obj).function(row['symbol'])
            binding, evidence, _ = match.reference_inputs(row['proof']['bindings_path'], row)
            if (binding_rename.reference_digest(binding) != row['proof']['bindings_sha256'] or
                    binding_rename.reference_digest(evidence) != row['proof']['reference_evidence_sha256']):
                raise ValueError('Accepted reference bindings changed: ' + row['id'])
            for name, record in binding['symbols'].items():
                if 'owner' in record:  # match.verify_rows: the owner context is compiled from the current file
                    owner = record['owner']['source']
                    if evidence['files'].get(owner) != sha((self.root / owner).read_bytes()):
                        raise ValueError('Loader-zero owner source/include context must be independently pinned: ' + name)
            exact = set(binding['symbols']) == {r['symbol'] for r in section['relocations']}
            for rel in section['relocations']:
                record = binding['symbols'].get(rel['symbol'], {})
                addend = struct.unpack_from('<i', section['data'], rel['offset'])[0]
                exact &= record.get('rva', -1) + addend == self.NATIVE[(row['id'], rel['offset'])]
                exact &= 0 <= addend < record.get('size', 0)
            results.append(dict(id=row['id'], exact=exact, relocated_equal=exact, scope='s',
                                compiled_sha256=sha(section['data']), target_sha256='t', object=str(obj),
                                object_sha256=sha(obj.read_bytes()), compile_inputs={}, command=['cl'],
                                source_sha256=sha(source), toolchain='fixture',
                                bindings_sha256=binding_rename.reference_digest(binding),
                                bindings_path=row['proof']['bindings_path'],
                                reference_evidence_sha256=binding_rename.reference_digest(evidence)))
        return results, dict(compiler_calls=0, rows=len(rows))

    def review(self, **changes):
        sha_a = sha((self.root / 'evidence/pipe_a.json').read_bytes())
        sha_b = sha((self.root / 'evidence/pipe_b.json').read_bytes())
        spec = lambda path, digest: dict(bindings_path=path, bindings_sha256=digest)
        review = dict(schema=1, profile=binding_rename.PROFILE, reason='The pipeline owner becomes uninitialized COMMON.',
                      owner_revisions={'src/pipe.c': dict(
                          before=sha(self.PIPE_BEFORE), after=sha(self.PIPE_AFTER),
                          rows={'m.p1': spec('evidence/pipe_a.json', sha_a), 'm.p2': spec('evidence/pipe_a.json', sha_a),
                                'm.p3': spec('evidence/pipe_b.json', sha_b)})})
        review.update(changes)
        self.write_json('evidence/pipe_review.json', review)
        return dict(path='evidence/pipe_review.json', sha256=sha((self.root / 'evidence/pipe_review.json').read_bytes()))

    def plan(self, review):
        state = (self.root / 'recovery.json').read_bytes()
        return dict(state_sha256=sha(state), promotions=[], binding_review=review, sources={
            'src/pipe.c': dict(candidate='work/cand/pipe.c', before_sha256=sha(self.PIPE_BEFORE),
                               candidate_sha256=sha(self.PIPE_AFTER))})


class OwnerRevisionDeriveTests(unittest.TestCase):
    HASHES = {'src/pipe.c': (sha(OwnerFixture.PIPE_BEFORE), sha(OwnerFixture.PIPE_AFTER))}

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name).resolve()
        self.fx = OwnerFixture(self.root)
        self.scope = patch.object(binding_rename, 'ROOT', self.root)
        self.scope.start()

    def tearDown(self):
        self.scope.stop()
        self.tmp.cleanup()

    def derive(self, mutate=None, changed=('src/pipe.c',), hashes='default'):
        item = self.fx.review()
        review = json.loads((self.root / item['path']).read_bytes())
        if mutate:
            mutate(review)
        documents = binding_rename.parent_documents(review)
        return binding_rename.derive(self.fx.state, review, documents, item['path'], 'c' * 64, set(changed),
                                     self.HASHES if hashes == 'default' else hashes)

    def refuses(self, pattern, mutate=None, **kw):
        with self.assertRaisesRegex(ValueError, pattern):
            self.derive(mutate, **kw)

    def reseal(self, name, document, row_id):
        """Rewrite one accepted manifest and keep its accepted row's proof consistent."""
        self.fx.write_json(name, document)
        row = next(r for r in self.fx.state['functions'] if r['id'] == row_id)
        row['proof'].update(binding_rename.proof_fields(document, row_id))
        return sha((self.root / name).read_bytes())

    def test_pins_move_and_nothing_else(self):
        plan = self.derive()
        before, after = self.HASHES['src/pipe.c']
        self.assertEqual(plan.rows, {'m.p1': {}, 'm.p2': {}, 'm.p3': {}})
        self.assertEqual(plan.definitions, {})
        self.assertEqual(plan.owner_revisions(), {'src/pipe.c': dict(before=before, after=after, rows=['m.p1', 'm.p2', 'm.p3'])})
        for path, parent in (('evidence/pipe_a.json', self.fx.doc_a), ('evidence/pipe_b.json', self.fx.doc_b)):
            new_path, document = plan.manifests[path]
            self.assertTrue(new_path.startswith('evidence/rename_'))
            self.assertEqual(document['evidence_files']['src/pipe.c'], after)
            self.assertEqual(document['functions'], parent['functions'])  # every binding verbatim, m.p4 included
            self.assertEqual(document['evidence_files'][path], sha((self.root / path).read_bytes()))
            self.assertEqual(document['evidence_files']['evidence/pipe_review.json'], 'c' * 64)  # derive's review_sha
            rest = {k: v for k, v in document['evidence_files'].items()
                    if k not in ('src/pipe.c', path, 'evidence/pipe_review.json')}
            self.assertEqual(rest, {k: v for k, v in parent['evidence_files'].items() if k != 'src/pipe.c'})
            self.assertEqual(json.loads((self.root / path).read_bytes()), parent)  # the parent stays provenance

    def test_the_derivation_is_the_parent_with_the_pin_undone(self):
        plan = self.derive()
        path = 'evidence/pipe_a.json'
        restored = copy.deepcopy(plan.manifests[path][1])
        restored['evidence_files']['src/pipe.c'] = self.HASHES['src/pipe.c'][0]
        for name in (path, 'evidence/pipe_review.json'):
            del restored['evidence_files'][name]
        del restored['binding_rename_review']
        self.assertEqual(restored, self.fx.doc_a)

    def test_the_undo_check_refuses_every_other_difference(self):
        plan = self.derive()
        path = 'evidence/pipe_a.json'
        documents = binding_rename.parent_documents(dict(bindings={'x': dict(bindings_path=path)}))
        pins = plan.owner_pins[path]
        check = lambda: binding_rename._check_owner_undo(plan, documents, path, pins, 'evidence/pipe_review.json')
        check()
        document = plan.manifests[path][1]
        tampers = {
            'a record': lambda: document['functions']['m.p1']['symbols']['_pipe'].update(size=0x80),
            'a reference': lambda: document['functions']['m.p4']['symbols']['_helper']['references'].append(
                dict(offset=9, type=20, addend=0)),
            'another pin': lambda: document['evidence_files'].update({'src/p4.c': '0' * 64}),
            'the pin is not revised': lambda: document['evidence_files'].update({'src/pipe.c': pins['src/pipe.c'][0]}),
            'identity': lambda: document.update(schema=2),
            'a function is dropped': lambda: document['functions'].pop('m.p4'),
        }
        for label, tamper in tampers.items():
            with self.subTest(label):
                saved = copy.deepcopy(document)
                tamper()
                with self.assertRaises(ValueError):
                    check()
                document.clear()
                document.update(saved)
        check()

    def test_an_owner_revision_combines_with_a_reviewed_rename_of_the_same_row(self):
        self.fx.doc_a['functions']['m.p1']['symbols']['_helper'] = function_record(0x1000, [9])
        self.fx.write_json('evidence/pipe_a.json', self.fx.doc_a)
        for row_id in ('m.p1', 'm.p2', 'm.p4'):  # one document: every row's proof digest follows it
            row = next(r for r in self.fx.state['functions'] if r['id'] == row_id)
            row['proof'].update(binding_rename.proof_fields(self.fx.doc_a, row_id))
        digest = sha((self.root / 'evidence/pipe_a.json').read_bytes())

        def mutate(review):
            for spec in review['owner_revisions']['src/pipe.c']['rows'].values():
                if spec['bindings_path'] == 'evidence/pipe_a.json':
                    spec['bindings_sha256'] = digest
            review['bindings'] = {'m.p1': dict(bindings_path='evidence/pipe_a.json', bindings_sha256=digest,
                                               renames={'_helper': '_helper2'})}
        plan = self.derive(mutate, changed=('src/pipe.c', 'src/p1.c'))
        self.assertEqual(plan.rows['m.p1'], {'_helper': ('_helper2', 0)})
        self.assertEqual(plan.rows['m.p2'], {})
        document = plan.manifests['evidence/pipe_a.json'][1]
        self.assertEqual(document['evidence_files']['src/pipe.c'], self.HASHES['src/pipe.c'][1])
        self.assertEqual(document['functions']['m.p1']['symbols']['_helper2'], function_record(0x1000, [9]))
        self.assertNotIn('_helper', document['functions']['m.p1']['symbols'])
        self.assertEqual(document['functions']['m.p2'], self.fx.doc_a['functions']['m.p2'])

    def test_refusals(self):
        revision = lambda r: r['owner_revisions']['src/pipe.c']
        rows = lambda r: revision(r)['rows']
        move = lambda r, name: r['owner_revisions'].update({name: r['owner_revisions'].pop('src/pipe.c')})
        cases = [
            ('source outside src', 'canonical src', lambda r: move(r, 'recovery.json')),
            ('a header', 'canonical src', lambda r: move(r, 'src/pipe.h')),
            ('a path that is not spelled canonically', 'canonical src', lambda r: move(r, 'src/../src/pipe.c')),
            ('a missing source', 'canonical src', lambda r: move(r, 'src/gone.c')),
            ('before is not the pinned hash', 'disagrees', lambda r: revision(r).update(before=sha(b'other'))),
            ('after differs from the plan', 'disagrees', lambda r: revision(r).update(after=sha(b'other'))),
            ('before equals after', 'Invalid owner revision', lambda r: revision(r).update(after=revision(r)['before'])),
            ('not a hash', 'Invalid owner revision', lambda r: revision(r).update(after='ABC')),
            ('no rows', 'Invalid owner revision', lambda r: revision(r).update(rows={})),
            ('extra field', 'Invalid owner revision', lambda r: revision(r).update(note='x')),
            ('extra row field', 'Invalid owner revision row', lambda r: rows(r)['m.p1'].update(renames={})),
            ('unknown review key', 'Invalid binding rename review', lambda r: r.update(non_owner_pins={})),
            ('stale parent manifest', 'Reviewed parent manifest changed', lambda r: rows(r)['m.p1'].update(
                bindings_sha256='0' * 64)),
            ('wrong manifest for a row', 'different current binding', lambda r: rows(r)['m.p3'].update(
                bindings_path='evidence/pipe_a.json')),
            ('a row that is not in the reviewed set', 'not reviewed', lambda r: rows(r).pop('m.p3')),
            ('a row that does not use the owner', 'does not use', lambda r: rows(r).update(
                {'m.p4': dict(bindings_path='evidence/pipe_a.json', bindings_sha256=rows(r)['m.p1']['bindings_sha256'])})),
            ('an unknown row', 'RELOCATED', lambda r: rows(r).update({'m.nope': rows(r)['m.p1']})),
        ]
        changed = ('src/pipe.c', 'recovery.json', 'src/pipe.h', 'src/../src/pipe.c', 'src/gone.c')
        for label, pattern, mutate in cases:
            with self.subTest(label):
                self.refuses(pattern, mutate, changed=changed)
        with self.subTest('before is not the pinned hash, even when the plan agrees'):
            hashes = {'src/pipe.c': (sha(b'other'), self.HASHES['src/pipe.c'][1])}
            self.refuses('before-hash', lambda r: revision(r).update(before=sha(b'other')), hashes=hashes)
        with self.subTest('not a changed source'):
            self.refuses('changed source', changed=())
        with self.subTest('the plan does not declare the hashes'):
            self.refuses('disagrees', hashes=None)
        with self.subTest('a plan that declares other hashes'):
            self.refuses('disagrees', hashes={'src/pipe.c': (sha(b'a'), sha(b'b'))})

    def test_refusals_from_the_ledger_and_the_bindings(self):
        functions = self.fx.state['functions']
        with self.subTest('the revised source is an accepted caller unit'):
            functions[3]['source'] = 'src/pipe.c'
            self.refuses('accepted caller')
            functions[3]['source'] = 'src/p4.c'
        with self.subTest('a row that is not accepted RELOCATED'):
            functions[0]['status'] = 'FUNCTION_MATCH'
            self.refuses('RELOCATED')
            functions[0]['status'] = RELOCATED
        with self.subTest('a row with prior raw proof'):
            functions[0]['prior_raw_proof'] = {'scope': 'raw'}
            self.refuses('RELOCATED')
            del functions[0]['prior_raw_proof']
        with self.subTest('the accepted binding differs from its proof'):
            saved = functions[0]['proof']['bindings_sha256']
            functions[0]['proof']['bindings_sha256'] = '0' * 64
            self.refuses('differs from its proof')
            functions[0]['proof']['bindings_sha256'] = saved
        self.derive()

    def test_the_document_must_pin_the_before_hash(self):
        self.fx.doc_b['evidence_files']['src/pipe.c'] = sha(b'an older owner')
        digest = self.reseal('evidence/pipe_b.json', self.fx.doc_b, 'm.p3')
        self.refuses('before-hash', lambda r: r['owner_revisions']['src/pipe.c']['rows']['m.p3'].update(bindings_sha256=digest))

    def test_only_loader_zero_and_file_data_owners_are_revisable(self):
        record = self.fx.doc_b['functions']['m.p3']['symbols']['_pipe']
        record['kind'] = 'data'  # an owner-shaped record that is neither a bss nor a file-data owner
        digest = self.reseal('evidence/pipe_b.json', self.fx.doc_b, 'm.p3')
        mutate = lambda r: r['owner_revisions']['src/pipe.c']['rows']['m.p3'].update(bindings_sha256=digest)
        self.refuses('only as a loader-zero or file-data owner', mutate)
        record['data_profile'] = 'file-data-owner-v1'  # a file-data owner is revisable too
        digest = self.reseal('evidence/pipe_b.json', self.fx.doc_b, 'm.p3')
        self.assertEqual(sorted(self.derive(mutate).owner_rows), ['m.p1', 'm.p2', 'm.p3'])

    def test_a_review_with_nothing_to_do_is_refused(self):
        self.refuses('must declare', lambda r: r.pop('owner_revisions'))


class OwnerRevisionApplyTests(unittest.TestCase):
    """repin.apply() with a reviewed owner revision: the pins move, the fresh gate re-verifies every accepted row."""

    def setUp(self):
        base = Path(__file__).resolve().parents[1] / 'work'
        base.mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=base)
        self.root = Path(self.tmp.name).resolve()
        self.fx = OwnerFixture(self.root)
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

    def run_apply(self, plan, after_gate=None):
        state = json.loads((self.root / 'recovery.json').read_bytes())
        results, _ = self.fx.verify(match.accepted_rows(state['functions']))
        before = self.root / 'work/before.json'
        before.write_text(json.dumps(dict(results=results, state_sha256=sha((self.root / 'recovery.json').read_bytes()),
                                          historical_evidence={})))
        plan_path = self.root / 'work/plan.json'
        plan_path.write_text(json.dumps(plan))
        with patch.object(match, 'verify_rows', after_gate or self.fx.verify):
            return repin.apply(plan_path, before, self.root, self.root / 'work/repin')

    def test_owner_revision_reissues_the_users_and_reverifies_everything(self):
        receipt = self.run_apply(self.fx.plan(self.fx.review()))
        rows = {r['id']: r for r in json.loads((self.root / 'recovery.json').read_bytes())['functions']}
        self.assertEqual((self.root / 'src/pipe.c').read_bytes(), OwnerFixture.PIPE_AFTER)
        for row_id, parent in (('m.p1', self.fx.doc_a), ('m.p2', self.fx.doc_a), ('m.p3', self.fx.doc_b)):
            proof = rows[row_id]['proof']
            self.assertTrue(proof['bindings_path'].startswith('evidence/rename_'))
            manifest = json.loads((self.root / proof['bindings_path']).read_bytes())
            self.assertEqual(manifest['evidence_files']['src/pipe.c'], sha(OwnerFixture.PIPE_AFTER))
            self.assertEqual(manifest['functions'][row_id], parent['functions'][row_id])  # the record is verbatim
            self.assertNotIn('src/pipe.c', proof.get('reference_snapshots', {}))  # the live file satisfies the pin
            history = rows[row_id]['reference_rename_history'][0]
            self.assertTrue(history['proof']['bindings_path'].startswith('evidence/pipe_'))
            self.assertEqual(proof['bindings_sha256'], history['proof']['bindings_sha256'])  # binding digest unchanged
            self.assertNotEqual(proof['reference_evidence_sha256'], history['proof']['reference_evidence_sha256'])
            fresh = next(r for r in receipt['results'] if r['id'] == row_id)
            self.assertEqual({k: fresh[k] for k in binding_rename.PROVENANCE},
                             {k: proof[k] for k in binding_rename.PROVENANCE})
        # The unreviewed non-user keeps its parent manifest and reads the preserved old owner pin.
        p4 = rows['m.p4']['proof']
        self.assertEqual(p4['bindings_path'], 'evidence/pipe_a.json')
        self.assertEqual(p4['reference_snapshots']['src/pipe.c'],
                         'evidence/repin_snapshots/%s.txt' % sha(OwnerFixture.PIPE_BEFORE))
        self.assertNotIn('reference_rename_history', rows['m.p4'])
        self.assertEqual(receipt['binding_renames']['owner_revisions']['src/pipe.c']['rows'], ['m.p1', 'm.p2', 'm.p3'])
        self.assertEqual(receipt['binding_renames']['rows']['m.p1'], {})
        self.assertEqual(len(receipt['preserved']), 4)

    def test_a_user_missing_from_the_review_fails_before_anything_is_written(self):
        review = self.fx.review()
        document = json.loads((self.root / review['path']).read_bytes())
        del document['owner_revisions']['src/pipe.c']['rows']['m.p3']
        self.fx.write_json('evidence/pipe_review.json', document)
        review = dict(path=review['path'], sha256=sha((self.root / review['path']).read_bytes()))
        before = (self.root / 'recovery.json').read_bytes()
        with self.assertRaisesRegex(ValueError, 'not reviewed'):
            self.run_apply(self.fx.plan(review))
        self.assertEqual((self.root / 'recovery.json').read_bytes(), before)
        self.assertEqual((self.root / 'src/pipe.c').read_bytes(), OwnerFixture.PIPE_BEFORE)
        self.assertEqual(list((self.root / 'evidence').glob('rename_*.json')), [])

    def test_the_gate_still_fails_closed_without_the_review(self):
        # No review: the old pin meets the new owner source, exactly the refusal the gate exists for.
        plan = self.fx.plan(None)
        del plan['binding_review']
        before = (self.root / 'recovery.json').read_bytes()
        with self.assertRaisesRegex(ValueError, 'independently pinned'):
            self.run_apply(plan)
        self.assertEqual((self.root / 'recovery.json').read_bytes(), before)
        self.assertEqual((self.root / 'src/pipe.c').read_bytes(), OwnerFixture.PIPE_BEFORE)

    def test_a_comparator_that_reads_the_parent_manifest_fails_closed(self):
        real = self.fx.verify

        def parent_reader(rows, **kw):
            results, stats = real(rows, **kw)
            for result in results:
                if result['id'] == 'm.p1':
                    result['bindings_path'] = 'evidence/pipe_a.json'
            return results, stats
        before = (self.root / 'recovery.json').read_bytes()
        with self.assertRaisesRegex(ValueError, 'all verified findings m.p1'):
            self.run_apply(self.fx.plan(self.fx.review()), after_gate=parent_reader)
        self.assertEqual((self.root / 'recovery.json').read_bytes(), before)
        self.assertEqual((self.root / 'src/pipe.c').read_bytes(), OwnerFixture.PIPE_BEFORE)


if __name__ == '__main__':
    unittest.main()
