"""Focused DLL loader-zero indexed controls; originals are read only and never executed."""
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import loader_zero_indexed as lz
import masked
from pe import PE
from zero_storage import verify_native
import test_zero_storage as zero_tests
from test_relocated import coff_file


def decode(raw, rva=0x1000):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return next(decoder.disasm(raw, rva))


class LoaderZeroIndexedTests(unittest.TestCase):
    def setUp(self):
        self.z = zero_tests.ZeroStorageTests()
        self.z.setUp()
        self.addCleanup(self.z.doCleanups)

    def prepare(self, raw='8b048500000000c3', off=3, addend=0, native=None, extent=4):
        raw = bytearray.fromhex(raw)
        struct.pack_into('<i', raw, off, addend)
        zero_tests.fixture_caller(self.z.caller, bytes(raw), off)
        zero_tests.fixture_owner(self.z.owner, extent)
        image = bytearray.fromhex(native) if native else bytearray(raw)
        struct.pack_into('<I', image, off, 0x402200 + addend)

        def apply(data):
            data[0x400:0x400 + len(image)] = image
            struct.pack_into('<IIHH', data, 0x800, 0x1000, 12, 0x3000 | off, 0x3101)
        self.z.edit_pe(apply)
        self.size = len(raw)
        record = self.z.record
        record.update(size=extent, access_profile=lz.ACCESS_PROFILE,
                      references=[dict(offset=off, type=masked.DIR32, addend=addend)])
        receipt = dict(fixed_indexed_receipt(decode(bytes(raw)), off, addend, extent), profile=lz.ACCESS_PROFILE)
        record['loader_zero_indexed'] = dict(extent_claim='minimum', accesses=[receipt])

    def compare(self, comparator=fixed.relocated_compare):
        return comparator(self.z.caller, '_sample', self.z.target, 0x1000, self.size,
                          dict(symbols={'_cell': self.z.record}),
                          storage_owners={'_cell': self.z.owner})

    def test_sib_and_modrm_indexed_forms_prove_owner_plus_addend(self):
        for raw, off, addend in [('8b048500000000c3', 3, 0), ('8b04c500000000c3', 3, 4),
                                 ('8b8000000000c3', 2, 0), ('668b8800000000c3', 3, 2),
                                 ('8a04cd00000000c3', 3, 7)]:
            with self.subTest(raw=raw, addend=addend):
                self.prepare(raw, off, addend, extent=8)
                result = self.compare()
                self.assertTrue(result['relocated_equal'])
                self.assertEqual(result['references'][0]['destination_rva'], 0x2200 + addend)

    def test_shared_and_legacy_comparators_require_opt_in(self):
        self.prepare()
        record = self.z.record
        legacy = {k: v for k, v in record.items() if k not in ('access_profile', 'loader_zero_indexed')}
        self.z.record = legacy
        for comparator in (fixed.relocated_compare, masked.relocated_compare):
            with self.subTest(comparator=comparator.__module__):
                with self.assertRaisesRegex(ValueError, 'scalar owner'):
                    self.compare(comparator)

    def test_record_validation_requires_minimum_claim_and_unchanged_anchor(self):
        self.prepare()
        good = self.z.record
        lz.validate_record(good)
        bad = [dict(good, access_profile='fixed-i386-indexed-v1'), dict(good, kind='data'),
               dict(good, anchor_profile='cmp-test-absolute-v1'), dict(good, fixed_disp32={}),
               {k: v for k, v in good.items() if k != 'loader_zero_indexed'}]
        for claim in ('complete', 'prefix', None):
            bad.append(dict(good, loader_zero_indexed=dict(good['loader_zero_indexed'], extent_claim=claim)))
        bad.append(dict(good, loader_zero_indexed=dict(good['loader_zero_indexed'], capacity=160)))
        bad.append(dict(good, loader_zero_indexed=dict(extent_claim='minimum',
                                                       accesses=[dict(offset=3, addend=0, width=4, form='direct')])))
        for record in bad:
            with self.subTest(record=sorted(record)), self.assertRaises(ValueError):
                lz.validate_record(record)
        with self.assertRaisesRegex(ValueError, 'unchanged loader-zero anchor'):
            fixed.access_form(dict(good, anchor_profile=fixed.PROFILE))

    def test_changed_survey_receipt_rejects(self):
        for key, value in [('addend', 4), ('width', 2), ('offset', 2)]:
            with self.subTest(key=key):
                self.prepare()
                self.z.record['loader_zero_indexed']['accesses'][0][key] = value
                with self.assertRaisesRegex(ValueError, 'survey differs'):
                    self.compare()
        self.prepare()
        self.z.record['loader_zero_indexed']['accesses'][0]['profile'] = 'fixed-i386-indexed-v1'
        with self.assertRaisesRegex(ValueError, 'indexed access survey'):
            self.compare()
        self.prepare()
        self.z.record['loader_zero_indexed']['accesses'][0]['encoding']['index'] = 'ecx'
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()

    def test_wrong_anchor_cell_fixup_or_independence_rejects(self):
        self.prepare()
        self.z.edit_pe(lambda d: struct.pack_into('<H', d, 0x80a, 0))
        with self.assertRaisesRegex(ValueError, 'HIGHLOW operand fixup'):
            self.compare()
        self.prepare()
        anchor = dict(self.z.record['identity'])
        self.z.record['identity'].update(rva=0x1000, size=7, operand=1,
                                         sha256=hashlib.sha256(PE(self.z.target).read_rva(0x1000, 7)).hexdigest())
        with self.assertRaisesRegex(ValueError, 'independent'):
            self.compare()
        self.prepare()
        self.z.record['identity'] = anchor
        self.z.edit_pe(lambda d: struct.pack_into('<I', d, 0x501, 0x402204))
        anchor['sha256'] = hashlib.sha256(PE(self.z.target).read_rva(0x1100, 5)).hexdigest()
        with self.assertRaisesRegex(ValueError, 'another storage cell'):
            self.compare()

    def test_addend_outside_minimum_extent_rejects(self):
        self.prepare(addend=4, extent=8)
        zero_tests.fixture_owner(self.z.owner, 4)
        self.z.record['size'] = 4
        with self.assertRaisesRegex(ValueError, 'exceeds independently bound object'):
            self.compare()
        with self.assertRaises(ValueError):
            fixed_indexed_receipt(decode(bytes.fromhex('8b04850000000000')), 3, 4, 4)

    def test_owner_extent_must_equal_the_exact_compiled_owner(self):
        self.prepare(extent=8)
        zero_tests.fixture_owner(self.z.owner, 16)
        with self.assertRaisesRegex(ValueError, 'exact-size COMMON'):
            self.compare()
        self.prepare(extent=8)
        self.z.record['size'] = 160
        with self.assertRaises(ValueError):
            self.compare()

    def test_native_instruction_or_displacement_mismatch_rejects(self):
        for native in ['8b048d00000000c3', '8b0c8500000000c3', '8b04c500000000c3']:
            with self.subTest(native=native):
                self.prepare(native=native)
                with self.assertRaisesRegex(ValueError, 'native decoding differs'):
                    self.compare()
        self.prepare()
        self.z.edit_pe(lambda d: struct.pack_into('<I', d, 0x403, 0x402204))
        with self.assertRaisesRegex(ValueError, 'displacement differs'):
            self.compare()

    def test_direct_and_immediate_uses_keep_legacy_checks(self):
        record = dict(access_profile=lz.ACCESS_PROFILE)
        ins = decode(bytes.fromhex('a100224000'))
        self.assertEqual(lz.verify_record_access(ins, 1, 0, 4, 0x1000, record, b'', 0x402200),
                         dict(offset=1, addend=0, width=4, form='direct'))
        with self.assertRaisesRegex(ValueError, 'scalar owner'):
            lz.verify_record_access(ins, 1, 2, 4, 0x1000, record, b'', 0x402200)
        ins = decode(bytes.fromhex('6800224000'))
        self.assertEqual(lz.verify_record_access(ins, 1, 0, 4, 0x1000, record, b'', 0x402200),
                         dict(offset=1, addend=0, form='immediate'))
        with self.assertRaises(ValueError):
            lz.verify_record_access(ins, 2, 0, 4, 0x1000, record, b'', 0x402200)

    def test_lea_transfer_segment_and_wide_forms_reject(self):
        for raw in ['8d048500224000', 'ff148500224000', 'ff248500224000',
                    '648b048500224000', 'dd048500224000', '0f10048500224000']:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                lz.verify_record_access(decode(bytes.fromhex(raw)), 3 + (raw[:2] in ('64', '0f')), 0, 8,
                                        0x1000, {}, bytes.fromhex(raw), 0x402200)


def fixed_indexed_receipt(ins, off, addend, extent):
    from fixed_indexed import verify_indexed_access
    return verify_indexed_access(ins, off, addend, extent, 0x1000)


@unittest.skipUnless(os.environ.get('HERCULES_TEST_ORIGINAL_ROOT'), 'set original root for native controls')
class OriginalLoaderZeroControls(unittest.TestCase):
    """The ENG1 element-0 anchor identifies 0x6fee8 only; neighbours and interiors are refused."""

    def test_eng1_kind_info_anchor_and_negative_controls(self):
        root = Path(__file__).resolve().parents[1]
        rows = json.loads((root / 'recovery.json').read_bytes())['functions']
        row = next(r for r in rows if r['id'] == 'eng1.08450')
        pe = PE(Path(os.environ['HERCULES_TEST_ORIGINAL_ROOT']) / row['target'])
        self.assertEqual(hashlib.sha256(pe.data).hexdigest(), row['target_sha256'])
        identity = dict(rva=0x41855, size=5, operand=1,
                        sha256='6185a93c2947efefec0cc1c8533dafa3e82aef18be8a5597e31c45c8634b27e4')
        rva, size = int(row['rva'], 0), row['size']
        verify_native(pe, 0x6fee8, 8, identity, rva, size)
        for cell in (0x6fee4, 0x6feec, 0x6fef0):
            with self.subTest(cell=cell), self.assertRaisesRegex(ValueError, 'another storage cell'):
                verify_native(pe, cell, 8, identity, rva, size)
        with self.assertRaisesRegex(ValueError, 'independent'):
            verify_native(pe, 0x6fee8, 8, identity, 0x41850, 0x10)
        raw = pe.read_rva(rva, size)
        hits = [i for i in range(size - 3) if struct.unpack_from('<I', raw, i)[0] == pe.image_base + 0x6fee8]
        self.assertTrue(hits)
        for at in hits:
            ins = next(i for i in _decode_all(raw, rva) if i.address - rva <= at < i.address - rva + i.size)
            receipt = lz.verify_record_access(ins, at, 0, 8, rva, {}, raw, pe.image_base + 0x6fee8)
            self.assertEqual(receipt['profile'], lz.ACCESS_PROFILE)
            with self.assertRaises(ValueError):
                lz.verify_record_access(ins, at, 8, 8, rva, {}, raw, pe.image_base + 0x6fee8)
            with self.assertRaisesRegex(ValueError, 'displacement differs'):
                lz.verify_record_access(ins, at, 0, 8, rva, {}, raw, pe.image_base + 0x6fef0)


def _decode_all(raw, rva):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return list(decoder.disasm(raw, rva))


if __name__ == '__main__':
    unittest.main()
