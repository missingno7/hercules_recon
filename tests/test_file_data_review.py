"""Regressions from the adversarial file-data-owner-v1 review (2026-10-31); synthetic files only.

Payload children share the caller's identities (F1), padding never counts as owner storage (F2),
the narrative stays out of the pinned docs/matching.md, and reanchor.py changes only anchor fields.
"""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest

ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'scripts' / 'masked.py').is_file())
sys.path.insert(0, str(ROOT / 'scripts'))
import fixed_disp32
import masked
from reanchor import derive
from test_matching import workspace_temporary_directory
import test_file_data_owner as T
import test_reanchor as R

BASE, DATA, ANCHOR, CALLER = T.BASE, T.DATA_RVA, T.ANCHOR_RVA, T.CALLER_RVA


def sha(data):
    return hashlib.sha256(data).hexdigest()


class FileDataReviewComparator(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('file-data-review-')
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.target, self.caller, self.owner = [self.root / n for n in ('original.dll', 'caller.obj', 'owner.obj')]

    def compare(self, symbols, size):
        return fixed_disp32.relocated_compare(self.caller, '_sample', self.target, CALLER, size,
                                              dict(symbols=symbols), storage_owners={'_table': self.owner})

    def assertAccepted(self, symbols, size):
        result = self.compare(symbols, size)
        self.assertTrue(result['relocated_equal'], result['errors'])

    # F1: payload children share the caller's identities --------------------------------------
    def callee_case(self, child_rva):
        """Caller: mov eax,[_table]; call _callee (record 0x1100). Owner table: {.., &_callee}."""
        code = b'\xa1' + bytes(4) + b'\xe8' + bytes(4) + b'\xc3'
        T.coff(self.caller, [(b'.text', code, 0x60501020, [(1, 1, masked.DIR32), (6, 2, masked.REL32)])],
               [('_sample', 0, 1, 0x20, 2), ('_table', 0, 0, 0, 2), ('_callee', 0, 0, 0x20, 2)])
        native_caller = (b'\xa1' + struct.pack('<I', BASE + DATA) + b'\xe8' +
                         struct.pack('<i', 0x1100 - (CALLER + 10)) + b'\xc3')
        payload = T.PAYLOAD[:4] + struct.pack('<I', BASE + child_rva)
        T.image(self.target, data=payload, caller=native_caller, fixups=[CALLER + 1, ANCHOR + 1, DATA + 4])
        T.owner_file(self.owner, T.PAYLOAD[:4] + bytes(4), relocations=[(4, 1, 6)],
                     symbols=[('_table', 0, 2, 0, 2), ('_callee', 0, 0, 0x20, 2)])
        child = dict(kind='function', rva=child_rva, size=5, evidence=['payload child identity'],
                     references=[dict(offset=4, type=6, addend=0)])
        callee = dict(kind='function', rva=0x1100, size=5, evidence=['caller callee identity'],
                      references=[dict(offset=6, type=masked.REL32, addend=0)])
        rec = dict(T.record(data=payload), payload_bindings=dict(symbols={'_callee': child}))
        return {'_table': rec, '_callee': callee}, len(code)

    def test_payload_child_cannot_contradict_caller_record(self):
        # _callee at 0x1100 (caller REL32) and 0x1180 (table pointer) in one binding.
        with self.assertRaisesRegex(ValueError, 'Conflicting independent payload identities'):
            self.compare(*self.callee_case(0x1180))

    def test_payload_child_agrees_with_caller_record(self):
        self.assertAccepted(*self.callee_case(0x1100))

    def self_case(self, child_rva, child_size):
        """Owner table holds &_sample, the compared function (0x1000, 6 bytes)."""
        T.caller_file(self.caller)
        payload = T.PAYLOAD[:4] + struct.pack('<I', BASE + child_rva)
        T.image(self.target, data=payload, fixups=[CALLER + 1, ANCHOR + 1, DATA + 4])
        T.owner_file(self.owner, T.PAYLOAD[:4] + bytes(4), relocations=[(4, 1, 6)],
                     symbols=[('_table', 0, 2, 0, 2), ('_sample', 0, 0, 0x20, 2)])
        child = dict(kind='function', rva=child_rva, size=child_size, evidence=['payload child identity'],
                     references=[dict(offset=4, type=6, addend=0)])
        return {'_table': dict(T.record(data=payload), payload_bindings=dict(symbols={'_sample': child}))}, 6

    def test_payload_child_cannot_rebind_compared_function(self):
        with self.assertRaisesRegex(ValueError, 'Conflicting independent payload identities'):
            self.compare(*self.self_case(0x1180, 5))

    def test_payload_child_may_point_to_compared_function(self):
        self.assertAccepted(*self.self_case(CALLER, 6))

    # F2: alignment padding is not owner storage ----------------------------------------------
    def test_alignment_padding_is_not_object_storage(self):
        # VC5 measured: short tbl[3] then long -> _tbl at 0, next at 8 (allocation 8, object 6).
        owner_payload = bytes.fromhex('010002000300') + bytes(2) + struct.pack('<I', 5)
        T.owner_file(self.owner, owner_payload, symbols=[('_table', 0, 2, 0, 2), ('_next', 8, 2, 0, 2)])
        native = owner_payload[:8]
        code = b'\x66\xa1' + struct.pack('<I', 6) + b'\xc3'                       # mov ax,[_table+6]
        T.coff(self.caller, [(b'.text', code, 0x60501020, [(2, 1, masked.DIR32)])],
               [('_sample', 0, 1, 0x20, 2), ('_table', 0, 0, 0, 2)])
        T.image(self.target, data=native + struct.pack('<I', 5),
                caller=b'\x66\xa1' + struct.pack('<I', BASE + DATA + 6) + b'\xc3', fixups=[CALLER + 2, ANCHOR + 1])
        rec = T.record(size=8, data=native, references=[dict(offset=2, type=6, addend=6)])
        with self.assertRaisesRegex(ValueError, 'must end its section'):
            self.compare({'_table': rec}, len(code))

    def test_last_object_owner_has_exact_storage(self):
        # VC5 measured: long then short tbl[3] -> _tbl at 4, section size 10 (no trailing padding).
        T.owner_file(self.owner, struct.pack('<I', 5) + bytes.fromhex('010002000300'),
                     symbols=[('_lead', 0, 2, 0, 2), ('_table', 4, 2, 0, 2)])
        T.caller_file(self.caller)
        T.image(self.target, data=bytes.fromhex('010002000300'))
        self.assertAccepted({'_table': T.record(size=6, data=bytes.fromhex('010002000300'))}, 6)


class FileDataReviewEvidence(unittest.TestCase):
    def test_pinned_matching_doc_is_unchanged(self):
        """Accepted proofs pin docs/matching.md; docs/ cannot be snapshotted, so it must never change."""
        current = sha((ROOT / 'docs' / 'matching.md').read_bytes())
        state = json.loads((ROOT / 'recovery.json').read_bytes())
        cache, broken = {}, []
        for row in state['functions']:
            if row['status'] != 'FUNCTION_MATCH_RELOCATED':
                continue
            path = row['proof']['bindings_path']
            if path not in cache:
                cache[path] = json.loads((ROOT / path).read_bytes())
            pinned = cache[path]['evidence_files'].get('docs/matching.md')
            if pinned and pinned != current:
                broken.append(row['id'])
        self.assertEqual(broken, [])
        from reviewed_evidence import snapshot_path
        digest = '0' * 64
        with self.assertRaisesRegex(ValueError, 'Only repository source/review provenance'):
            snapshot_path(ROOT, 'docs/matching.md', digest,
                          {'docs/matching.md': 'evidence/repin_snapshots/' + digest + '.txt'})


class FileDataReviewReanchor(unittest.TestCase):
    fixture = R.FileDataReanchorTests.fixture
    repin = R.FileDataReanchorTests.repin

    def test_reanchor_changes_only_reviewed_anchor_fields(self):
        state, review, docs = self.fixture(peer=True)
        doc = docs['evidence/old.json'][1]
        for fid in ('test', 'peer'):
            rec = doc['functions'][fid]['symbols']['_cell']
            rec['payload_bindings'] = dict(symbols={'_cb': dict(kind='function', rva=0x1180, size=5, evidence=['e'],
                                                                references=[dict(offset=4, type=6, addend=0)])})
            other = copy.deepcopy(rec)
            other['owner']['symbol'] = '_other'
            other['file_data']['anchor'] = dict(rva=7, size=5, operand=0, sha256='c')
            doc['functions'][fid]['symbols']['_other'] = other
        self.repin(state, review, docs)
        before = copy.deepcopy(docs)
        _, manifests, touched, _ = derive(state, review, docs, 'evidence/review.json', 'review-sha')
        self.assertEqual(docs, before)
        new = manifests['evidence/old.json'][1]
        allowed = {('evidence_files', 'evidence/old.json'), ('evidence_files', 'evidence/review.json'),
                   ('anchor_refresh_review',)}
        allowed |= {('functions', fid, 'symbols', name, 'file_data', 'anchor') for fid, name in touched}

        def diff(old, new, path=()):
            if isinstance(old, dict) and isinstance(new, dict):
                for key in sorted(set(old) | set(new)):
                    yield from diff(old.get(key, KeyError), new.get(key, KeyError), path + (key,))
            elif old != new:
                yield path
        changed = list(diff(doc, new))
        self.assertTrue(changed)
        for path in changed:
            self.assertTrue(any(path[:len(a)] == a for a in allowed), path)
        self.assertEqual(sorted(touched), [('peer', '_cell'), ('test', '_cell')])


if __name__ == '__main__':
    unittest.main()
