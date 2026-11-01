"""loader-zero-indexed-v1 address form (lea reg32, [index*scale + disp32]); synthetic PE/COFF only."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import fixed_indexed
import loader_zero_indexed as lz
import loader_zero_lea as lea
import masked
import test_zero_storage as zero_tests


def decode(raw, rva=0x1000):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return next(decoder.disasm(raw, rva), None)  # None: not decodable, which the verifier refuses

PLACEHOLDER = dict(profile=lz.ACCESS_PROFILE, access='address')
LEA = '8d048500000000c3'          # lea eax, [eax*4 + owner]; ret


def lea_receipt(raw, off, addend, extent, rva=0x1000):
    return lea.verify_lea_access(decode(bytes.fromhex(raw), rva), off, addend, extent, rva)


class LoaderZeroLeaTests(unittest.TestCase):
    def setUp(self):
        self.z = zero_tests.ZeroStorageTests()
        self.z.setUp()
        self.addCleanup(self.z.doCleanups)

    def prepare(self, raw=LEA, off=3, addend=0, native=None, extent=8, survey=None, reloc=None):
        raw = bytearray.fromhex(raw)
        struct.pack_into('<i', raw, off, addend)
        zero_tests.fixture_caller(self.z.caller, bytes(raw), off)
        zero_tests.fixture_owner(self.z.owner, extent)
        image = bytearray.fromhex(native) if native else bytearray(raw)
        struct.pack_into('<I', image, off, 0x402200 + addend)

        def apply(data):
            data[0x400:0x400 + len(image)] = image
            struct.pack_into('<IIHH', data, 0x800, 0x1000, 12, 0x3000 | (off if reloc is None else reloc), 0x3101)
        self.z.edit_pe(apply)
        self.size = len(raw)
        record = self.z.record
        record.update(size=extent, access_profile=lz.ACCESS_PROFILE,
                      references=[dict(offset=off, type=masked.DIR32, addend=addend)])
        if survey is None:
            survey = [lea.verify_lea_access(decode(bytes(raw)), off, addend, extent, 0x1000)]
        record['loader_zero_indexed'] = dict(extent_claim='minimum', accesses=survey)

    def compare(self, comparator=fixed.relocated_compare):
        return comparator(self.z.caller, '_sample', self.z.target, 0x1000, self.size,
                          dict(symbols={'_cell': self.z.record}),
                          storage_owners={'_cell': self.z.owner})

    def test_lea_proves_owner_plus_addend_and_is_marked_as_address(self):
        for raw, off, addend in [('8d048500000000c3', 3, 0), ('8d04c500000000c3', 3, 4),
                                 ('8d0c8d00000000c3', 3, 7), ('8d14d500000000c3', 3, 1),
                                 ('8d3c0500000000c3', 3, 2), ('8d1ccd00000000c3', 3, 0)]:
            with self.subTest(raw=raw, addend=addend):
                self.prepare(raw, off, addend)
                result = self.compare()
                self.assertTrue(result['relocated_equal'])
                self.assertEqual(result['references'][0]['destination_rva'], 0x2200 + addend)
                receipt = self.z.record['loader_zero_indexed']['accesses'][0]
                self.assertEqual(receipt['access'], 'address')
                self.assertNotIn('width', receipt)
                self.assertEqual(receipt['profile'], lz.ACCESS_PROFILE)

    def test_dereference_receipts_are_unchanged_and_carry_no_access_field(self):
        raw = bytes.fromhex('8b048500224000c3')
        ins = decode(raw)
        direct = lz.verify_record_access(ins, 3, 0, 8, 0x1000, {}, raw, 0x402200)
        routed = lea.verify_record_access(ins, 3, 0, 8, 0x1000, {}, raw, 0x402200)
        self.assertEqual(direct, routed)
        self.assertNotIn('access', routed)
        self.assertEqual(routed['width'], 4)

    def test_mixed_dereference_and_lea_survey(self):
        # The shared-twin shape: an accepted indexed dereference followed by the LEA form.
        code = bytearray.fromhex('8b048500000000' + '8d048500000000' + 'c3')
        struct.pack_into('<i', code, 3, 0)
        struct.pack_into('<i', code, 10, 4)
        zero_tests.coff_file(self.z.caller, bytes(code), [(3, 1, masked.DIR32), (10, 1, masked.DIR32)],
                             [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])
        zero_tests.fixture_owner(self.z.owner, 8)
        image = bytearray(code)
        struct.pack_into('<I', image, 3, 0x402200)
        struct.pack_into('<I', image, 10, 0x402204)

        def apply(data):
            data[0x400:0x400 + len(image)] = image
            struct.pack_into('<IIHHHH', data, 0x800, 0x1000, 16, 0x3003, 0x300a, 0x3101, 0)
        self.z.edit_pe(apply)
        self.size = len(code)
        deref = dict(fixed_indexed.verify_indexed_access(decode(bytes(code[:7])), 3, 0, 8, 0x1000),
                     profile=lz.ACCESS_PROFILE)
        address = lea.verify_lea_access(decode(bytes(code[7:14]), 0x1007), 10, 4, 8, 0x1000)
        self.z.record.update(size=8, access_profile=lz.ACCESS_PROFILE,
                             references=[dict(offset=3, type=masked.DIR32, addend=0),
                                         dict(offset=10, type=masked.DIR32, addend=4)],
                             loader_zero_indexed=dict(extent_claim='minimum', accesses=[deref, address]))
        self.assertTrue(self.compare()['relocated_equal'])
        self.assertNotIn('access', deref)
        self.assertEqual(address['access'], 'address')
        self.z.record['loader_zero_indexed']['accesses'] = [deref, dict(address, addend=0)]
        with self.assertRaises(ValueError):
            self.compare()

    def test_existing_dereference_module_still_refuses_lea(self):
        raw = bytes.fromhex('8d048500224000')
        with self.assertRaisesRegex(ValueError, 'scalar ModRM/SIB disp32 dereference'):
            lz.verify_record_access(decode(raw), 3, 0, 8, 0x1000, {}, raw, 0x402200)
        with self.assertRaisesRegex(ValueError, 'scalar ModRM/SIB disp32 dereference'):
            fixed_indexed.verify_record_access(decode(raw), 3, 0, 8, 0x1000,
                                               {'access_profile': fixed_indexed.ACCESS_PROFILE}, raw, 0x402200)

    def test_opt_in_is_required(self):
        self.prepare()
        record = self.z.record
        self.z.record = {k: v for k, v in record.items() if k not in ('access_profile', 'loader_zero_indexed')}
        for comparator in (fixed.relocated_compare, masked.relocated_compare):
            with self.subTest(comparator=comparator.__module__):
                with self.assertRaisesRegex(ValueError, 'scalar owner'):
                    self.compare(comparator)
        self.z.record = dict(record, loader_zero_indexed=dict(record['loader_zero_indexed'], accesses=[]))
        with self.assertRaises(ValueError):
            self.compare()

    def test_base_plus_index_base_only_and_missing_index_reject(self):
        for raw in ['8d848500224000', '8d8c0500224000', '8d800022400000', '8d042500224000', '8d002200400000']:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                lea.lea_encoding(decode(bytes.fromhex(raw)))
        self.prepare('8d848500000000c3', survey=[PLACEHOLDER])
        with self.assertRaisesRegex(ValueError, 'unprefixed lea'):
            self.compare()
        self.prepare('8d042500000000c3', survey=[PLACEHOLDER])
        with self.assertRaisesRegex(ValueError, 'unprefixed lea'):
            self.compare()

    def test_prefixes_segment_and_address_size_reject(self):
        for raw in ['648d048500224000', '658d048500224000', '2e8d048500224000', '678d048500224000',
                    '668d048500224000', 'f08d048500224000', 'f38d048500224000', '678d84850022400000']:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                lea.lea_encoding(decode(bytes.fromhex(raw)))
        self.prepare('648d048500000000c3', off=4, survey=[PLACEHOLDER])
        with self.assertRaisesRegex(ValueError, 'unprefixed lea'):
            self.compare()
        self.prepare('678d048500000000c3', off=4, survey=[PLACEHOLDER])
        with self.assertRaises(ValueError):
            self.compare()

    def test_other_opcodes_and_non_dword_destinations_reject(self):
        for raw in ['8b048500224000', 'ff148500224000', 'ff248500224000', 'c7048500224000ffffffff',
                    '0f10048500224000', 'dd048500224000', 'ff348500224000', '8f048500224000',
                    '0fb7048500224000', '8a048500224000', 'c4048500224000']:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                lea.lea_encoding(decode(bytes.fromhex(raw)))
        # 8D /r with a register ModRM is invalid; mod01/mod10 forms are other shapes.
        for raw in ['8d44850000', '8d8485224000', '8dc0']:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                lea.lea_encoding(decode(bytes.fromhex(raw)))
        with self.assertRaises(ValueError):
            lea.lea_encoding(None)

    def test_addend_outside_the_minimum_extent_rejects(self):
        for addend, extent in [(8, 8), (9, 8), (-1, 8), (4, 4)]:
            with self.subTest(addend=addend), self.assertRaises(ValueError):
                lea_receipt('8d048500224000', 3, addend, extent)
        self.assertEqual(lea_receipt('8d048500224000', 3, 7, 8)['addend'], 7)
        self.prepare(addend=4, extent=8)
        zero_tests.fixture_owner(self.z.owner, 4)
        self.z.record['size'] = 4
        with self.assertRaisesRegex(ValueError, 'exceeds independently bound object'):
            self.compare()
        with self.assertRaises(ValueError):
            lea_receipt('8d048500224000', 3, 1.5, 8)

    def test_wrong_relocation_field_rejects(self):
        for reloc in (4, 2):
            with self.subTest(reloc=reloc):
                self.prepare(reloc=reloc)
                with self.assertRaisesRegex(ValueError, 'HIGHLOW'):
                    self.compare()
        self.prepare()
        self.z.edit_pe(lambda d: struct.pack_into('<H', d, 0x808, 0))
        with self.assertRaisesRegex(ValueError, 'HIGHLOW'):
            self.compare()
        # The reviewed field must be the disp32 of the LEA.
        with self.assertRaises(ValueError):
            lea_receipt('8d048500224000', 2, 0, 8)
        with self.assertRaises(ValueError):
            lea_receipt('8d048500224000', 4, 0, 8)

    def test_dereference_only_record_cannot_claim_lea_and_vice_versa(self):
        deref = dict(fixed_indexed.verify_indexed_access(
            decode(bytes.fromhex('8b04850000000000')), 3, 0, 8, 0x1000), profile=lz.ACCESS_PROFILE)
        # A LEA caller whose survey only lists a dereference.
        self.prepare(survey=[deref])
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()
        # A dereference caller whose survey claims address formation.
        claim = lea_receipt('8d048500224000', 3, 0, 8)
        self.prepare('8b048500000000c3', survey=[claim])
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()
        # An address claim cannot be upgraded to or from an access width.
        for key, value in [('width', 4), ('access', 'load'), ('addend', 4), ('offset', 2)]:
            with self.subTest(key=key):
                self.prepare()
                self.z.record['loader_zero_indexed']['accesses'][0][key] = value
                with self.assertRaisesRegex(ValueError, 'survey differs'):
                    self.compare()
        self.prepare()
        del self.z.record['loader_zero_indexed']['accesses'][0]['access']
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()
        self.prepare()
        self.z.record['loader_zero_indexed']['accesses'][0]['encoding']['scale'] = 2
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            self.compare()

    def test_native_instruction_or_displacement_mismatch_rejects(self):
        for native in ['8d048d00000000c3', '8d0c8500000000c3', '8d04c500000000c3', '8b048500000000c3']:
            with self.subTest(native=native):
                self.prepare(native=native)
                with self.assertRaisesRegex(ValueError, 'native decoding differs'):
                    self.compare()
        self.prepare()
        self.z.edit_pe(lambda d: struct.pack_into('<I', d, 0x403, 0x402204))
        with self.assertRaisesRegex(ValueError, 'displacement differs'):
            self.compare()

    def test_record_validation_is_unchanged(self):
        self.prepare()
        good = self.z.record
        lz.validate_record(good)
        for bad in [dict(good, loader_zero_indexed=dict(good['loader_zero_indexed'], extent_claim='complete')),
                    dict(good, anchor_profile='cmp-test-absolute-v1'), dict(good, kind='data')]:
            with self.assertRaises(ValueError):
                lz.validate_record(bad)
        self.assertIs(fixed.access_form(good), lea.verify_record_access)


if __name__ == '__main__':
    unittest.main()
