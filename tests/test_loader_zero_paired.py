"""Controls for loader-zero-paired-v1 over synthetic PE/COFF; originals are read only and never executed."""
import hashlib
import os
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import fixed_disp32 as fixed
import loader_zero_indexed as lz
import loader_zero_paired as paired
import masked
from pe import PE
import test_zero_storage as zero_tests


GOOD = 'bf042240008b47fc'          # mov edi,0x402204 ; mov eax,[edi-4]


class PairedNative(unittest.TestCase):
    """verify_native over one synthetic PE: owner 0x2200 in the .data zero tail."""

    def setUp(self):
        self.z = zero_tests.ZeroStorageTests()
        self.z.setUp()
        self.addCleanup(self.z.doCleanups)

    def place(self, raw=GOOD, addend=4, fixups=(0x3001, 0x3101), at=0x500):
        raw = bytes.fromhex(raw)

        def apply(data):
            data[0x500:0x520] = bytes(0x20)
            data[at:at + len(raw)] = raw
            struct.pack_into('<II', data, 0x800, 0x1000, 8 + 2 * len(fixups))
            for n, entry in enumerate(fixups):
                struct.pack_into('<H', data, 0x808 + 2 * n, entry)
            struct.pack_into('<I', data, 0x98 + 96 + 5 * 8 + 4, 8 + 2 * len(fixups))
        self.z.edit_pe(apply)
        return dict(rva=0x1000 + at - 0x400, size=len(raw), addend=addend, sha256=hashlib.sha256(raw).hexdigest())

    def verify(self, identity, rva=0x2200, extent=8, caller=(0x1000, 6)):
        return paired.verify_native(PE(self.z.target), rva, extent, identity, *caller)

    def rejects(self, pattern, identity, **kw):
        with self.assertRaisesRegex(ValueError, pattern):
            self.verify(identity, **kw)

    def test_positive_forms(self):
        for raw, k, width, extent in [(GOOD, 4, 4, 8), ('bf042240008b7ffc', 4, 4, 8),
                                      ('bf04224000668b47fc', 4, 2, 6), ('bf042240008a47fc', 4, 1, 5),
                                      ('bf042240008b4427fc', 4, 4, 8), ('be012240008a46ff', 1, 1, 2),
                                      ('b9082240008b81f8ffffff', 8, 4, 12)]:
            with self.subTest(raw=raw):
                receipt = self.verify(self.place(raw, k), extent=extent)
                self.assertEqual((receipt['addend'], receipt['load_width'], receipt['identity_rva']), (k, width, 0x1100))

    def test_wrong_anchor_address_or_owner_cell(self):
        self.rejects('another storage cell', self.place('bf082240008b47fc'))     # imm points at owner+8
        self.rejects('another storage cell', self.place(), rva=0x2204, extent=8)
        self.rejects('another storage cell', self.place(), rva=0x21fc + 0x8, extent=4)

    def test_wrong_k(self):
        self.rejects('another storage cell', self.place(addend=8))                # identity k vs imm
        self.rejects('dereference exactly', self.place('bf042240008b47f8'))       # disp -8 with k 4
        self.rejects('dereference exactly', self.place('bf042240008b4704'))       # disp +4
        self.rejects('Invalid paired', self.place('bf002240008b4700', addend=0))  # k=0 is the legacy form

    def test_intervening_instruction_or_write(self):
        for raw in ('bf04224000908b47fc', 'bf0422400083c7088b47fc', 'bf0422400047' + '8b47fc'):
            with self.subTest(raw=raw):
                self.rejects('exactly two', self.place(raw))
        self.rejects('exactly two', self.place(GOOD + 'c3'))

    def test_control_transfer_cannot_form_either_half(self):
        # A pair must be a fall-through register move then a plain load; no jump, call or
        # transfer through [reg-k] is admitted.
        for raw, pattern in [('e9000000008b47fc', 'mov r32, imm32'), ('bf04224000ff67fc', 'scalar register load'),
                             ('bf04224000ff57fc', 'scalar register load')]:
            with self.subTest(raw=raw):
                self.rejects(pattern, self.place(raw))

    def test_external_edge_into_second_instruction_is_not_a_proof_input(self):
        # Documented policy: a branch landing at the second instruction's start (the ENG3 loop
        # back edge at 0x1c275 -> 0x1c1ba) neither helps nor hurts; only the pinned fall-through
        # from the first instruction is used. A branch elsewhere in .text cannot alter the hash.
        identity = self.place()
        self.z.edit_pe(lambda d: d.__setitem__(slice(0x510, 0x512), bytes([0x7c, 0x105 - 0x112 & 0xff])))
        self.assertEqual(self.verify(identity)['addend'], 4)
        # A transfer that lands inside the pinned span changes the pinned bytes, so it cannot be
        # smuggled in without failing the hash.
        identity['sha256'] = '0' * 64
        self.rejects('hash changed', identity)

    def test_missing_extra_or_wrong_fixup(self):
        self.rejects('exactly one original HIGHLOW', self.place(fixups=(0x3001,)))
        self.rejects('exactly one original HIGHLOW', self.place(fixups=(0x3001, 0x3101, 0x3105)))
        self.rejects('exactly one original HIGHLOW', self.place(fixups=(0x3001, 0x3102)))
        self.rejects('exactly one original HIGHLOW', self.place(fixups=(0x3001, 0x2101)))
        self.rejects('exactly one original HIGHLOW', self.place(fixups=(0x3001, 0x3100)))

    def test_anchor_inside_caller(self):
        for caller in ((0x1100, 8), (0x1104, 1), (0x10f0, 0x11), (0x1107, 4)):
            with self.subTest(caller=caller):
                self.rejects('independent of the compared caller', self.place(), caller=caller)
        self.assertEqual(self.verify(self.place(), caller=(0x1108, 4))['addend'], 4)

    def test_wrong_register_or_operand(self):
        for raw in ('bf042240008b46fc',            # loads through esi, not edi
                    'bf04224000648b47fc',          # fs: override
                    'bf042240008b443ffc',          # [edi+edi-4] indexed
                    'bf042240008d47fc',            # lea, not a load
                    'bf042240000fb647fc',          # movzx
                    'bf04224000894ffc',            # store
                    'bf04224000a100224000',        # absolute load, not via the register
                    'bf04224000f38b47fc'):         # rep prefix
            with self.subTest(raw=raw):
                self.rejects('scalar register load|dereference exactly|exactly two', self.place(raw))
        for raw in ('bc042240008b44fcfc', '66bf04228b47fc', 'c7c7042240008b47fc', '8d3d042240008b47fc'):
            with self.subTest(raw=raw):
                self.rejects('mov r32, imm32|exactly two', self.place(raw))

    def test_width_must_fit_the_minimum_extent(self):
        identity = self.place()
        for extent in (4, 5, 6, 7):
            with self.subTest(extent=extent):
                self.rejects('minimum owner extent', identity, extent=extent)
        self.rejects('minimum owner extent', self.place('bf04224000668b47fc'), extent=5)
        self.assertEqual(self.verify(self.place('bf04224000668b47fc'), extent=6)['load_width'], 2)

    def test_identity_record_shape_and_hash(self):
        good = self.place()
        for bad in (dict(good, operand=1), {k: v for k, v in good.items() if k != 'addend'},
                    dict(good, sha256='0' * 64), dict(good, size=5), dict(good, size=9),
                    dict(good, rva=0x1101), dict(good, addend='4'), dict(good, addend=-4)):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.verify(bad)

    def test_storage_gate_unchanged(self):
        identity = self.place()
        self.rejects('zero tail', identity, rva=0x21f8)        # file-backed .data payload
        self.rejects('storage extent', identity, rva=0x2200, extent=0)


class PairedPipeline(unittest.TestCase):
    """Shadow fixed_disp32 + loader_zero_indexed: indexed accesses with a paired anchor."""

    def setUp(self):
        self.z = zero_tests.ZeroStorageTests()
        self.z.setUp()
        self.addCleanup(self.z.doCleanups)

    def prepare(self, raw='8b04c500000000c3', off=3, addend=0, extent=8, anchor=GOOD):
        code = bytearray.fromhex(raw)
        struct.pack_into('<i', code, off, addend)
        zero_tests.fixture_caller(self.z.caller, bytes(code), off)
        zero_tests.fixture_owner(self.z.owner, extent)
        image = bytearray(code)
        struct.pack_into('<I', image, off, 0x402200 + addend)
        pair = bytes.fromhex(anchor)

        def apply(data):
            data[0x400:0x400 + len(image)] = image
            data[0x500:0x510] = bytes(0x10)
            data[0x500:0x500 + len(pair)] = pair
            struct.pack_into('<IIHH', data, 0x800, 0x1000, 12, 0x3000 | off, 0x3101)
        self.z.edit_pe(apply)
        self.size = len(code)
        from fixed_indexed import verify_indexed_access
        import capstone
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32); md.detail = True
        ins = next(md.disasm(bytes(code), 0x1000))
        record = self.z.record
        record.update(rva=0x2200, size=extent, access_profile=lz.ACCESS_PROFILE, anchor_profile=paired.ANCHOR_PROFILE,
                      identity=dict(rva=0x1100, size=len(pair), addend=4, sha256=hashlib.sha256(pair).hexdigest()),
                      references=[dict(offset=off, type=masked.DIR32, addend=addend)],
                      loader_zero_indexed=dict(extent_claim='minimum', accesses=[
                          dict(verify_indexed_access(ins, off, addend, extent, 0x1000), profile=lz.ACCESS_PROFILE)]))

    def compare(self, comparator=None):
        return (comparator or fixed.relocated_compare)(
            self.z.caller, '_sample', self.z.target, 0x1000, self.size,
            dict(symbols={'_cell': self.z.record}), storage_owners={'_cell': self.z.owner})

    def test_paired_anchor_admits_indexed_owner_plus_addend(self):
        for raw, off, addend in [('8b04c500000000c3', 3, 0), ('8b04c500000000c3', 3, 4), ('8b8000000000c3', 2, 0)]:
            with self.subTest(raw=raw, addend=addend):
                self.prepare(raw, off, addend)
                result = self.compare()
                self.assertTrue(result['relocated_equal'])
                self.assertEqual(result['references'][0]['destination_rva'], 0x2200 + addend)

    def test_pipeline_negative_controls(self):
        self.prepare()
        self.z.record['rva'] = 0x2208
        with self.assertRaises(ValueError):
            self.compare()
        self.prepare(anchor='bf082240008b47fc')
        with self.assertRaisesRegex(ValueError, 'another storage cell'):
            self.compare()
        self.prepare(extent=4)
        with self.assertRaisesRegex(ValueError, 'minimum owner extent'):
            self.compare()
        self.prepare()
        zero_tests.fixture_owner(self.z.owner, 16)
        with self.assertRaisesRegex(ValueError, 'exact-size COMMON'):
            self.compare()
        self.prepare()
        self.z.edit_pe(lambda d: struct.pack_into('<H', d, 0x80a, 0))
        with self.assertRaisesRegex(ValueError, 'HIGHLOW'):
            self.compare()
        self.prepare()
        self.z.record['identity'].update(rva=0x1000, size=8,
                                         sha256=hashlib.sha256(PE(self.z.target).read_rva(0x1000, 8)).hexdigest())
        with self.assertRaisesRegex(ValueError, 'independent'):
            self.compare()
        self.prepare()
        self.z.edit_pe(lambda d: struct.pack_into('<I', d, 0x403, 0x402204))
        with self.assertRaisesRegex(ValueError, 'displacement differs'):
            self.compare()

    def test_profiles_stay_closed(self):
        self.prepare()
        good = self.z.record
        lz.validate_record(good)
        for bad in (dict(good, anchor_profile='cmp-test-absolute-v1'), dict(good, anchor_profile=fixed.PROFILE),
                    dict(good, anchor_profile='loader-zero-paired-v2')):
            with self.subTest(anchor=bad['anchor_profile']), self.assertRaises(ValueError):
                lz.validate_record(bad)
        # Legacy identity verifier and legacy comparator never accept the paired identity shape.
        legacy = {k: v for k, v in good.items() if k != 'anchor_profile'}
        self.z.record = legacy
        with self.assertRaisesRegex(ValueError, 'one reviewed native instruction anchor'):
            self.compare()
        self.z.record = good
        with self.assertRaises(ValueError):
            self.compare(masked.relocated_compare)


@unittest.skipUnless(os.environ.get('HERCULES_TEST_ORIGINAL_ROOT'), 'set original root for native controls')
class OriginalEng3PairControls(unittest.TestCase):
    """ENG3 0x1c1b5/0x1c1ba identifies 0x3fbd0 only; neighbours, other k and the caller overlap refuse."""

    def test_eng3_kind_info_pair(self):
        import json
        rows = json.loads((ROOT / 'recovery.json').read_bytes())['functions']
        row = next(r for r in rows if r['id'] == 'eng3.023a0')
        pe = PE(Path(os.environ['HERCULES_TEST_ORIGINAL_ROOT']) / row['target'])
        self.assertEqual(hashlib.sha256(pe.data).hexdigest(), row['target_sha256'])
        identity = dict(rva=0x1c1b5, size=8, addend=4,
                        sha256='a5697269686124f4b975ebe571a3a49629792bd53e87a20768912d4a2a6955c4')
        caller = (int(row['rva'], 0), row['size'])
        self.assertEqual(paired.verify_native(pe, 0x3fbd0, 8, identity, *caller)['load_width'], 4)
        for cell in (0x3fbcc, 0x3fbd4, 0x3fbd8):
            with self.subTest(cell=cell), self.assertRaises(ValueError):
                paired.verify_native(pe, cell, 8, identity, *caller)
        for extent in (4, 7):
            with self.assertRaisesRegex(ValueError, 'minimum owner extent'):
                paired.verify_native(pe, 0x3fbd0, extent, identity, *caller)
        with self.assertRaises(ValueError):
            paired.verify_native(pe, 0x3fbd0, 8, dict(identity, addend=0), *caller)
        with self.assertRaisesRegex(ValueError, 'independent'):
            paired.verify_native(pe, 0x3fbd0, 8, identity, 0x1c010, 0x273)


if __name__ == '__main__':
    unittest.main()
