"""Focused indexed-access controls; originals are read only and never executed."""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import fixed_indexed as indexed
from pe import PE
import test_fixed_disp32 as fixed_tests
from test_relocated import coff_file


def decode(raw, rva=0x1000):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return next(decoder.disasm(raw, rva))


class IndexedTests(unittest.TestCase):
    def setUp(self):
        self.f = fixed_tests.FixedDisp32Tests()
        self.f.setUp()
        self.addCleanup(self.f.doCleanups)

    def prepare(self, raw='8b8000000000c3', off=2, addend=0):
        raw = bytearray.fromhex(raw)
        struct.pack_into('<i', raw, off, addend)
        coff_file(self.f.caller, bytes(raw), [(off, 1, 6)],
                  [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])
        native = bytearray(raw)
        struct.pack_into('<I', native, off, 0x402200 + addend)
        data = bytearray(self.f.target.read_bytes())
        data[0x400:0x400 + len(native)] = native
        self.f.target.write_bytes(data)
        self.f.profile['image'] = fixed.image_facts(PE(self.f.target))
        self.size = len(raw)
        self.f.record['references'] = [dict(offset=off, type=6, addend=addend)]
        self.f.record['access_profile'] = indexed.ACCESS_PROFILE
        receipt = indexed.verify_indexed_access(decode(raw), off, addend, 4, 0x1000)
        self.f.profile['accesses'] = [receipt]

    def compare(self, comparator=fixed.relocated_compare):
        return comparator(self.f.caller, '_sample', self.f.target, 0x1000, self.size,
                          dict(symbols={'_cell': self.f.record}),
                          storage_owners={'_cell': self.f.owner})

    def test_modrm_sib_scalar_forms_and_static_base_scope(self):
        for raw, off, addend in [('8b8000000000c3', 2, 0),
                                 ('8b048500000000c3', 3, 0),
                                 ('8b848800000000c3', 3, 0),
                                 ('8b842400000000c3', 3, 0),
                                 ('668b8000000000c3', 3, 0),
                                 ('8a8000000000c3', 2, 0),
                                 ('8b8000000000c3', 2, 3)]:
            with self.subTest(raw=raw, addend=addend):
                self.prepare(raw, off, addend)
                self.assertTrue(self.compare()['relocated_equal'])

    def test_shared_comparator_requires_opt_in(self):
        self.prepare()
        self.assertTrue(self.compare()['relocated_equal'])
        del self.f.record['access_profile']
        with self.assertRaisesRegex(ValueError, 'indexed'):
            self.compare()

    def test_opt_in_direct_access_keeps_legacy_bounds_and_receipt(self):
        ins = decode(bytes.fromhex('a100224000'))
        record = dict(access_profile=indexed.ACCESS_PROFILE)
        receipt = indexed.verify_record_access(ins, 1, 0, 4, 0x1000, record, b'', 0x402200)
        self.assertEqual(receipt, fixed.verify_access(ins, 1, 0, 4, 0x1000))
        with self.assertRaises(ValueError):
            indexed.verify_record_access(ins, 1, 1, 4, 0x1000, record, b'', 0x402200)

    def test_bad_profile_anchor_and_missing_survey(self):
        self.prepare()
        for key, value in [('access_profile', 'unknown'), ('anchor_profile', 'cmp-test-absolute-v1'),
                           ('kind', 'data')]:
            record = dict(self.f.record, **{key: value})
            with self.assertRaises(ValueError):
                indexed.validate_record(record)
        self.f.profile['accesses'] = []
        with self.assertRaisesRegex(ValueError, 'survey'):
            self.compare()

    def test_changed_encoding_receipt_rejects(self):
        self.prepare()
        self.f.profile['accesses'][0]['encoding']['base'] = 'ecx'
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()

    def test_changed_compiler_addend_rejects(self):
        self.prepare()
        self.f.record['references'][0]['addend'] = 1
        with self.assertRaisesRegex(ValueError, 'COFF relocation differs'):
            self.compare()

    def test_non_disp_relocation_rejects(self):
        self.prepare()
        raw = bytes.fromhex('c7800000000000000000c3')
        coff_file(self.f.caller, raw, [(6, 1, 6)],
                  [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])
        self.size = len(raw)
        self.f.record['references'] = [dict(offset=6, type=6, addend=0)]
        with self.assertRaisesRegex(ValueError, 'exactly the disp32'):
            self.compare()

    def test_outside_addends_and_wrong_field_reject(self):
        ins = decode(bytes.fromhex('8b8000000000'))
        for off, addend in [(1, 0), (3, 0), (2, -1), (2, 4), (2, 0x7fffffff)]:
            with self.subTest(off=off, addend=addend), self.assertRaises(ValueError):
                indexed.verify_indexed_access(ins, off, addend, 4, 0x1000)

    def test_nonindexed_escaping_prefixed_and_short_forms_reject(self):
        for raw in ['a100224000', '8b0500224000', '8b042500224000', '8b4004',
                    '648b8000224000', '678b800022', 'f38b8000224000',
                    '8d8000224000', 'ff9000224000', 'ffa000224000', 'b800224000']:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                indexed.verify_indexed_access(decode(bytes.fromhex(raw)), 2, 0, 4, 0x1000)



@unittest.skipUnless(os.environ.get('HERCULES_TEST_ORIGINAL_ROOT'), 'set original root for native controls')
class OriginalIndexedControls(unittest.TestCase):
    def test_original_two_rows_and_negative_controls(self):
        root = Path(__file__).resolve().parents[1]
        rows = json.loads((root / 'recovery.json').read_bytes())['functions']
        for id in ('exe.host_04b10', 'exe.cleanup_native_4cc0'):
            row = next(r for r in rows if r['id'] == id)
            pe = PE(Path(os.environ['HERCULES_TEST_ORIGINAL_ROOT']) / row['target'])
            self.assertEqual(hashlib.sha256(pe.data).hexdigest(), row['target_sha256'])
            rva = int(row['rva'], 0)
            raw = pe.read_rva(rva, row['size'])
            start, off, addend = (14, 16, 4) if id == 'exe.host_04b10' else (10, 12, 0)
            ins = decode(raw[start:], rva + start)
            record = dict(access_profile=indexed.ACCESS_PROFILE)
            receipt = indexed.verify_record_access(ins, off, addend, 72, rva, record, raw, 0xb9e858)
            self.assertEqual(receipt['encoding']['base'], 'eax' if start == 14 else 'ecx')
            with self.assertRaisesRegex(ValueError, 'exactly the disp32'):
                indexed.verify_indexed_access(ins, off + 1, addend, 72, rva)
            for outside in (-1, 72):
                with self.assertRaises(ValueError):
                    indexed.verify_indexed_access(ins, off, outside, 72, rva)
            changed = bytearray(raw)
            changed[start + ins.modrm_offset] ^= 1  # another base register
            with self.assertRaisesRegex(ValueError, 'decoding differs'):
                indexed.verify_record_access(ins, off, addend, 72, rva, record, changed, 0xb9e858)
            changed = bytearray(raw)
            changed[off] ^= 1
            with self.assertRaisesRegex(ValueError, 'displacement differs'):
                indexed.verify_record_access(ins, off, addend, 72, rva, record, changed, 0xb9e858)


if __name__ == '__main__':
    unittest.main()
