"""Bounded preferred-base verifier controls; synthetic images are never run."""
import copy
import hashlib
import inspect
import json
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import masked
from pe import PE
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file
from test_zero_storage import fixture_owner


class FixedDisp32Tests(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('fixed-disp32-')
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.target, self.caller, self.owner = [self.root / x for x in ('original.exe', 'caller.obj', 'owner.obj')]
        make_pe(self.target)
        data = bytearray(self.target.read_bytes())
        struct.pack_into('<H', data, 0x96, 0x103)
        struct.pack_into('<I', data, 0x98 + 224 + 40 + 8, 0x400)
        data[0x400:0x406] = bytes.fromhex('a100224000c3')
        data[0x500:0x507] = bytes.fromhex('8b0d00224000c3')
        self.target.write_bytes(data)
        self.identity = dict(rva=0x1100, size=6, operand=1, sha256=hashlib.sha256(data[0x500:0x506]).hexdigest())
        self.profile = dict(image=fixed.image_facts(PE(self.target)),
            encoding=dict(disp_offset=2, disp_size=4, address_size=4, modrm=13, mnemonic='mov', width=4),
            boundary=dict(rva=0x1100, size=7, anchor_offset=0,
                          sha256=hashlib.sha256(data[0x500:0x507]).hexdigest(), review='evidence/native.json'),
            excluded=[], excluded_sha256=fixed.fingerprint([]), accesses=[dict(offset=1, addend=0, width=4)])
        fixture_owner(self.owner)
        coff_file(self.caller, bytes.fromhex('a100000000c3'), [(1, 1, 6)],
                  [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])
        self.record = binding('bss', 0x2200, 4, (1, 6, 0), identity=self.identity,
            owner=dict(source='src/owner.c', flags=[], symbol='_cell'),
            anchor_profile=fixed.PROFILE, fixed_disp32=self.profile)

    def compare(self, comparator=fixed.relocated_compare):
        return comparator(self.caller, '_sample', self.target, 0x1000, 6,
                          dict(symbols={'_cell': self.record}), storage_owners={'_cell': self.owner})

    def native(self):
        return fixed.verify_native(PE(self.target), self.record['rva'], self.record['size'],
                                   self.identity, 0x1000, 6, self.profile)

    def replace_anchor(self, raw, operand=1):
        data = bytearray(self.target.read_bytes())
        data[0x500:0x500 + len(raw) + 1] = raw + b'\xc3'
        self.target.write_bytes(data)
        self.identity.update(size=len(raw), operand=operand, sha256=hashlib.sha256(raw).hexdigest())
        self.profile['image'] = fixed.image_facts(PE(self.target))
        self.profile['boundary'].update(size=len(raw)+1, sha256=hashlib.sha256(raw+b'\xc3').hexdigest())
        self.profile['encoding'] = fixed.encoding(fixed._instruction(PE(self.target), self.identity), operand)

    def test_positive_and_preferred_base_scope(self):
        self.assertTrue(self.compare()['relocated_equal'])
        self.assertTrue(self.native()['preferred_base_only'])
        self.assertFalse(self.native()['global_isolation_proved'])

    def test_legacy_path_does_not_admit_profile(self):
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit'):
            self.compare(masked.relocated_compare)
        del self.record['anchor_profile']
        with self.assertRaisesRegex(ValueError, 'HIGHLOW'):
            self.compare(masked.relocated_compare)

    def test_anchor_inside_pending_rejects(self):
        self.profile['excluded'] = [dict(id='pending', rva=0x1100, size=16)]
        self.profile['excluded_sha256'] = fixed.fingerprint(self.profile['excluded'])
        with self.assertRaisesRegex(ValueError, 'pending/selected'):
            self.native()

    def test_caller_overlap_rejects(self):
        with self.assertRaisesRegex(ValueError, 'pending/selected'):
            fixed.verify_native(PE(self.target), 0x2200, 4, self.identity, 0x10ff, 2, self.profile)

    def test_whole_containing_function_must_be_independent(self):
        self.profile['excluded'] = [dict(id='other', rva=0x1106, size=1)]
        self.profile['excluded_sha256'] = fixed.fingerprint(self.profile['excluded'])
        with self.assertRaisesRegex(ValueError, 'containing native function'):
            self.native()

    def test_stale_image_and_context_reject(self):
        self.profile['image']['sha256'] = '0'*64
        with self.assertRaisesRegex(ValueError, 'fingerprint'):
            self.native()
        self.profile['image'] = fixed.image_facts(PE(self.target))
        self.profile['boundary']['sha256'] = '0'*64
        with self.assertRaisesRegex(ValueError, 'context hash'):
            self.native()

    def test_pending_snapshot_omission_and_staleness(self):
        with self.assertRaisesRegex(ValueError, 'omits'):
            fixed.validate_exclusions(self.profile, [dict(id='p', rva=0x1100, size=16)])
        self.profile['excluded_sha256'] = '0'*64
        with self.assertRaisesRegex(ValueError, 'snapshot'):
            fixed.validate_exclusions(self.profile, [])

    def _review(self, ledger):
        record = dict(anchor_profile='fixed-i386-disp32-v1', fixed_disp32=self.profile)
        caller = dict(id='m.caller', target='original.exe', rva='0x1000', size=6)
        return fixed.review_inputs(record, ledger, caller, {}, self.root)

    def test_live_pending_row_overlapping_the_anchor_function_rejects(self):
        ledger = [dict(id='m.pending', target='original.exe', status='CODEGEN_SIMILAR', rva='0x1100', size=16)]
        with self.assertRaisesRegex(ValueError, 'current pending row: m.pending'):
            self._review(ledger)

    def test_newly_staged_pending_row_elsewhere_does_not_invalidate_the_proof(self):
        ledger = [dict(id='m.new', target='original.exe', status='CODEGEN_SIMILAR', rva='0x3000', size=16),
                  dict(id='m.caller', target='original.exe', status='CODEGEN_SIMILAR', rva='0x1000', size=6)]
        try:
            self._review(ledger)
        except ValueError as error:  # later checks may refuse this synthetic record, never the pending rule
            self.assertNotIn('pending', str(error))

    def test_reviewed_encoding_fields_are_pinned(self):
        self.profile['encoding']['disp_offset'] += 1
        with self.assertRaisesRegex(ValueError, 'encoding receipt'):
            self.native()

    def test_sib_index_segment_address_and_rep_reject(self):
        for raw in ('8b0c2500224000', '8b0c8500224000', '648b0d00224000',
                    '678b0e0022', 'f38b0d00224000'):
            with self.subTest(raw=raw):
                self.replace_anchor(bytes.fromhex(raw))
                with self.assertRaisesRegex(ValueError, 'non-SIB'):
                    self.native()

    def test_lea_push_immediate_moffs_reject(self):
        for raw, operand in (('8d0d00224000', 1), ('6800224000', 0), ('a100224000', 1)):
            with self.subTest(raw=raw):
                self.replace_anchor(bytes.fromhex(raw), operand)
                with self.assertRaises(ValueError):
                    self.native()

    def test_movsx_movzx_scalar_widths(self):
        for raw, width in (('0fbf0d00224000', 2), ('0fb60d00224000', 1), ('668b0d00224000', 2)):
            self.replace_anchor(bytes.fromhex(raw))
            self.assertEqual(self.native()['width'], width)

    def test_interior_anchor_and_wrong_owner_size_reject(self):
        self.record['rva'] += 1
        with self.assertRaisesRegex(ValueError, 'interior'):
            self.native()
        self.record['rva'] -= 1
        fixture_owner(self.owner, size=8)
        with self.assertRaisesRegex(ValueError, 'exact-size COMMON'):
            self.compare()

    def test_access_survey_and_capacity_reject(self):
        self.profile['accesses'] = []
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()
        self.profile['accesses'] = [dict(offset=1, addend=0, width=4)]
        self.record['size'] = 2
        fixture_owner(self.owner, size=2)
        with self.assertRaises(ValueError):
            self.compare()

    def test_escaping_and_indexed_caller_accesses_reject(self):
        import capstone
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32); decoder.detail = True
        for raw, off in (('6800224000', 1), ('8d0500224000', 2), ('be00224000', 1), ('8b048500224000', 3)):
            ins = next(decoder.disasm(bytes.fromhex(raw), 0x1000))
            with self.assertRaisesRegex(ValueError, 'escapes'):
                fixed.verify_access(ins, off, 0, 4, 0x1000)
        ins = next(decoder.disasm(bytes.fromhex('a100224000'), 0x1000))
        with self.assertRaisesRegex(ValueError, 'exceeds'):
            fixed.verify_access(ins, 1, 1, 4, 0x1000)

    def test_review_must_be_independently_pinned(self):
        row = dict(id='x', target='original.exe', target_sha256='x')
        with self.assertRaisesRegex(ValueError, 'independently pinned'):
            fixed.review_inputs(self.record, [], row, {}, self.root)
        path = self.root / 'evidence/native.json'; path.parent.mkdir()
        native = {k:v for k,v in self.profile['boundary'].items() if k != 'review'}
        path.write_text(json.dumps(dict(target_sha256='x', native_functions=[native])))
        fixed.review_inputs(self.record, [], row, {path: 'hash'}, self.root)
        path.write_text('{}')
        with self.assertRaisesRegex(ValueError, 'lacks a native review'):
            fixed.review_inputs(self.record, [], row, {path: 'hash'}, self.root)

    def test_pinned_comparator_preservation(self):
        original = inspect.getsource(masked.relocated_compare)
        expected = original.replace("record['anchor_profile'] != 'cmp-test-absolute-v1'",
                                    "record['anchor_profile'] not in ('cmp-test-absolute-v1', PROFILE)")
        expected = expected.replace("if record.get('anchor_profile') == 'cmp-test-absolute-v1':",
            "if record.get('anchor_profile') == PROFILE:\n                verify_native_fixed(pe, dest, extent, record.get('identity'), rva, size, record['fixed_disp32'])\n            elif record.get('anchor_profile') == 'cmp-test-absolute-v1':")
        expected = expected.replace('    fields, actual_offsets = set(), set()',
            "    access_surveys = {name: [] for name, rec in records.items() if rec.get('anchor_profile') == PROFILE}\n    fields, actual_offsets = set(), set()")
        expected = expected.replace("        if kind == 'bss':\n            if typ != DIR32",
            "        if kind == 'bss':\n            if record.get('anchor_profile') == PROFILE:\n                access_surveys[name].append(verify_access(ins, off, addend, record['size'], rva))\n            if typ != DIR32")
        expected = expected.replace('    if set(reviewed) != actual_offsets',
            "    for name, accesses in access_surveys.items():\n        if accesses != records[name]['fixed_disp32'].get('accesses'):\n            raise ValueError('Fixed disp32 reviewed access survey differs from complete COFF uses')\n    if set(reviewed) != actual_offsets")
        self.assertEqual(inspect.getsource(fixed.relocated_compare), expected)


if __name__ == '__main__':
    unittest.main()
