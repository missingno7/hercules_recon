"""Opaque indirect CALL and tail JMP controls; synthetic files only, originals are read and never executed."""
import hashlib
import inspect
import json
import os
from pathlib import Path
import struct
import sys
import textwrap
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import local_byte_tables as local
import masked
import match
import opaque_call as oc
from pe import PE
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file
from test_zero_storage import fixture_owner

REG_EAX = dict(form='register', register='eax')
OPTIONAL = 0x98  # make_pe optional header; data directories start at +96
ANCHOR = b'\x68' + struct.pack('<I', 0x402200)  # push offset _cell, at RVA 0x1100
CALL_SCOPE = '; %d opaque indirect CALL(s): targets unidentified, ABI unclaimed'
JMP_SCOPE = '; %d opaque tail JMP(s): targets unidentified, ABI unclaimed'


def record(offset, operand, **extra):
    result = dict(profile=oc.PROFILE, offset=offset, operand=operand, target='unidentified',
                  abi='unclaimed', evidence=['evidence/synthetic_review.json#call_sites.sample'])
    result.update(extra)
    return result


def jump_record(offset, operand, **extra):
    return record(offset, operand, profile=oc.JUMP_PROFILE, **extra)


def cell_operand(addend=0, symbol='_cell'):
    return dict(form='cell', symbol=symbol, addend=addend)


def memory(base, disp=0, index=None, scale=1):
    return dict(form='memory', base=base, index=index, scale=scale, disp=disp)


def decode(raw, address=0x1000):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return next(decoder.disasm(raw, address))


def put(image, rva, data):
    """make_pe .data: RVA 0x2000 at file 0x600, 0x200 bytes."""
    image[0x600 + rva - 0x2000:0x600 + rva - 0x2000 + len(data)] = data


def add_imports(image):
    """One import descriptor X.DLL with A (IAT slot 0x2160) and B (IAT slot 0x2164)."""
    put(image, 0x2100, struct.pack('<IIIII', 0x2140, 0, 0, 0x2180, 0x2160) + bytes(20))
    put(image, 0x2140, struct.pack('<III', 0x2190, 0x21a0, 0))
    put(image, 0x2160, struct.pack('<III', 0x2190, 0x21a0, 0))
    put(image, 0x2180, b'X.DLL\0')
    put(image, 0x2190, b'\0\0A\0')
    put(image, 0x21a0, b'\0\0B\0')
    struct.pack_into('<II', image, OPTIONAL + 96 + 8 * 1, 0x2100, 40)
    struct.pack_into('<II', image, OPTIONAL + 96 + 8 * 12, 0x2160, 12)


class OpaqueCallTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('opaque-')
        self.addCleanup(self.directory.cleanup)
        self.obj = Path(self.directory.name) / 'fixture.obj'
        self.owner = Path(self.directory.name) / 'owner.obj'
        self.target = Path(self.directory.name) / 'fixture.dll'
        self.stripped, self.patch, self.owners = False, None, None
        self.use('ffd0c3', record(0, REG_EAX))

    def use(self, code, *records, native=None):
        self.code = bytes.fromhex(code)
        self.native = bytes.fromhex(native) if native else self.code
        self.relocations, self.symbols = [], [('_sample', 0, 1, 0x20)]
        self.fixups = [0x3ff0]  # one HIGHLOW far from the contribution: the image keeps an inventory
        self.bindings = dict(symbols={}, indirect_calls=list(records))

    def use_cell(self, code='ff15', addend=0, native_cell=None, kind='bss', operand=None, size=4,
                 profile=oc.PROFILE, tail='c3'):
        field = len(code) // 2
        self.use(code + struct.pack('<i', addend).hex() + tail,
                 record(0, operand or dict(form='cell', symbol='_cell', addend=addend), profile=profile))
        self.native = (bytes.fromhex(code) + struct.pack('<I', 0x400000 + (native_cell or 0x2200 + addend)) +
                       bytes.fromhex(tail))
        self.relocations = [(field, 1, masked.DIR32)]
        self.symbols.append(('_cell', 0, 0, 0x20 if kind == 'function' else 0))
        self.fixups = [0x3000 | field, 0x3101]
        fixture_owner(self.owner, size)
        self.owners = {'_cell': self.owner}
        if kind == 'bss':
            cell = binding('bss', 0x2200, size, (field, masked.DIR32, addend),
                           identity=dict(rva=0x1100, size=5, operand=0, sha256=hashlib.sha256(ANCHOR).hexdigest()),
                           owner=dict(source='src/owner.c', flags=[], symbol='_cell'))
        else:
            cell = binding('function', 0x1100, 16, (field, masked.DIR32, addend))
        self.bindings['symbols'] = {'_cell': cell}

    def use_jump_cell(self, code='ff25', **options):
        """The wrapper shape: one JMP through a cell, then the compiler's NOP padding to 16 bytes."""
        self.use_cell(code, profile=oc.JUMP_PROFILE, tail='90' * 10, **options)

    def compare(self, comparator=fixed.relocated_compare):
        coff_file(self.obj, self.code, self.relocations, self.symbols)
        make_pe(self.target, base_relocation=not self.stripped)
        image = bytearray(self.target.read_bytes())
        struct.pack_into('<I', image, OPTIONAL + 224 + 40 + 8, 0x400)  # .data virtual tail is loader-zero
        image[0x400:0x400 + len(self.native)] = self.native
        image[0x500:0x505] = ANCHOR
        if not self.stripped:
            entries = list(self.fixups) + [0] * (len(self.fixups) % 2)
            struct.pack_into('<II' + 'H' * len(entries), image, 0x800, 0x1000, 8 + 2 * len(entries), *entries)
        if self.patch:
            self.patch(image)
        self.target.write_bytes(image)
        kwargs = dict(storage_owners=self.owners) if self.owners else {}
        return comparator(self.obj, '_sample', self.target, 0x1000, len(self.native), self.bindings, **kwargs)

    def test_register_and_register_based_memory_calls_prove_instruction_identity(self):
        cases = [('ffd0c3', 0, REG_EAX), ('ffd7c3', 0, dict(form='register', register='edi')),
                 ('ff5010c3', 0, memory('eax', 0x10)), ('ff1424c3', 0, memory('esp')),
                 ('ff542408c3', 0, memory('esp', 8)), ('ff9100010000c3', 0, memory('ecx', 0x100)),
                 ('ff148ec3', 0, memory('esi', 0, 'ecx', 4)), ('ff55f8c3', 0, memory('ebp', -8)),
                 ('51ffd0c3', 1, REG_EAX)]
        for code, offset, operand in cases:
            with self.subTest(code=code):
                self.use(code, record(offset, operand))
                result = self.compare()
                self.assertTrue(result['relocated_equal'])
                proof, = result['indirect_call_checks']
                self.assertEqual((proof['profile'], proof['offset'], proof['operand'], proof['target'], proof['abi']),
                                 (oc.PROFILE, offset, operand, 'unidentified', 'unclaimed'))
                self.assertIn('1 opaque indirect CALL(s): targets unidentified, ABI unclaimed', result['scope'])

    def test_unrecorded_indirect_calls_stay_refused(self):
        for code, message in [('ffd0c3', 'Register indirect'), ('ff5010c3', 'Unsupported indirect')]:
            with self.subTest(code=code):
                self.use(code)
                with self.assertRaisesRegex(ValueError, message):
                    self.compare()

    def test_legacy_comparator_refuses_opaque_records(self):
        with self.assertRaises(ValueError):
            self.compare(masked.relocated_compare)

    def test_record_validation_is_closed(self):
        good = record(0, REG_EAX)
        oc.validate_record(good)
        bad = [dict(good, target='callee'), dict(good, abi='cdecl'), dict(good, evidence=[]),
               dict(good, evidence=['  ']), dict(good, evidence=['work/velocity_j/review.json']),
               dict(good, offset=-1), dict(good, offset=True),
               dict(good, profile='fixed-slot-v1'), dict(good, extra=1),
               {k: v for k, v in good.items() if k != 'abi'},
               dict(good, operand=dict(form='register', register='ax')),
               dict(good, operand=dict(REG_EAX, extra=1)),
               dict(good, operand=memory(None)), dict(good, operand=memory('eax', 0, 'esp', 4)),
               dict(good, operand=memory('eax', 0, 'ecx', 3)), dict(good, operand=memory('eax', 1.0)),
               dict(good, operand=memory('eax', 0, 'ecx', True)), dict(good, operand=memory('eax', 0, 'ecx', 4.0)),
               dict(good, operand=dict(form='cell', symbol='_cell', addend=-4)),
               dict(good, operand=dict(form='import', symbol='_cell')), dict(good, operand='eax')]
        for item in bad:
            with self.subTest(item=item), self.assertRaises(ValueError):
                oc.validate_record(item)

    def test_reviewed_operand_must_equal_the_compiled_decode(self):
        for code, operand in [('ffd0c3', dict(form='register', register='ecx')), ('ff5010c3', memory('eax', 0x14)),
                              ('ff148ec3', memory('ecx', 0, 'esi', 4)), ('ff5010c3', REG_EAX)]:
            with self.subTest(code=code, operand=operand):
                self.use(code, record(0, operand))
                with self.assertRaisesRegex(ValueError, 'operand differs'):
                    self.compare()

    def test_offset_must_be_one_compiled_boundary_once(self):
        self.use('51ffd0c3', record(2, REG_EAX))
        with self.assertRaisesRegex(ValueError, 'boundary'):
            self.compare()
        self.use('ffd0c3', record(0, REG_EAX), record(0, REG_EAX))
        with self.assertRaisesRegex(ValueError, 'duplicated'):
            self.compare()

    def test_jmp_far_prefixed_and_segment_forms_are_refused(self):
        for code, operand in [('ffe0c3', REG_EAX), ('ff20c3', memory('eax')), ('ff18c3', memory('eax')),
                              ('66ffd0c3', REG_EAX), ('64ff10c3', memory('eax')), ('2eff10c3', memory('eax')),
                              ('e800000000c3', REG_EAX)]:
            with self.subTest(code=code), self.assertRaises(ValueError):
                self.use(code, record(0, operand))
                self.compare()

    def test_native_instruction_must_be_identical(self):
        for code, native, operand in [('ffd0c3', 'ffd1c3', REG_EAX), ('ff5010c3', 'ff5014c3', memory('eax', 0x10)),
                                      ('ff5010c3', 'ff9010000000c3', memory('eax', 0x10))]:
            with self.subTest(native=native):
                self.use(code, record(0, operand), native=native)
                with self.assertRaises(ValueError):
                    self.compare()

    def test_relocation_stripped_image_is_refused(self):
        # Review finding D1: without a base-relocation inventory an unrelocated literal
        # (mov eax, 0x401100; call eax) cannot be told from an original address.
        for code in ('ffd0c3', 'b800114000ffd0c3'):
            with self.subTest(code=code):
                self.use(code, record(code.index('ffd0') // 2, REG_EAX))
                self.stripped = True
                with self.assertRaisesRegex(ValueError, 'base-relocated image'):
                    self.compare()

    def test_relocated_register_based_operand_is_refused(self):
        self.use_cell()
        self.code = bytes.fromhex('ff9000000000c3')
        self.native = b'\xff\x90' + struct.pack('<I', 0x402200) + b'\xc3'
        for operand in (memory('eax', 0), dict(form='cell', symbol='_cell', addend=0)):
            with self.subTest(operand=operand):
                self.bindings['indirect_calls'] = [record(0, operand)]
                with self.assertRaises(ValueError):  # loader-zero direct-use or opaque relocation check
                    self.compare()

    def test_cell_call_binds_one_proven_loader_zero_cell(self):
        for code in ('ff15', 'ff1425'):
            with self.subTest(code=code):
                self.use_cell(code)
                result = self.compare()
                self.assertTrue(result['relocated_equal'])
                self.assertEqual(result['indirect_call_checks'][0]['operand'], dict(form='cell', symbol='_cell', addend=0))
                self.assertEqual(result['references'][0]['destination_rva'], 0x2200)

    def test_cell_call_negative_controls(self):
        cases = [dict(native_cell=0x2204), dict(kind='function', native_cell=0x1100),
                 dict(operand=dict(form='cell', symbol='_other', addend=0)),
                 dict(operand=dict(form='cell', symbol='_cell', addend=4)), dict(addend=2)]
        for options in cases:
            with self.subTest(options=options), self.assertRaises(ValueError):
                self.use_cell(**options)
                self.compare()

    def test_cell_call_without_record_stays_refused(self):
        self.use_cell()
        self.bindings['indirect_calls'] = []
        with self.assertRaisesRegex(ValueError, 'Unsupported indirect'):
            self.compare()

    def test_import_slot_relabelled_as_data_is_refused(self):
        # Review finding D2: a payload-hash 'data' record over another import's IAT slot must not
        # authorise a call, neither through the cell nor after loading it into a register.
        for code, native, opaque in [('ff1500000000c3', b'\xff\x15' + struct.pack('<I', 0x402164) + b'\xc3',
                                      record(0, dict(form='cell', symbol='__imp__A', addend=0))),
                                     ('8b3500000000ffd6c3', b'\x8b\x35' + struct.pack('<I', 0x402164) + b'\xff\xd6\xc3',
                                      record(6, dict(form='register', register='esi')))]:
            with self.subTest(code=code):
                self.use(code, opaque)
                self.native = native
                self.relocations = [(2, 1, masked.DIR32)]
                self.symbols.append(('__imp__A', 0, 0, 0))
                self.fixups = [0x3002]
                self.patch = add_imports
                data = binding('data', 0x2164, 4, (2, masked.DIR32, 0), sha256=hashlib.sha256(
                    struct.pack('<I', 0x21a0)).hexdigest())
                self.bindings['symbols'] = {'__imp__A': data}
                with self.assertRaisesRegex(ValueError, 'import slots or PE directories'):
                    self.compare()

    def test_describe_refuses_direct_and_prefixed_forms(self):
        for raw in ('e800000000', '66ffd0', '64ff10', 'ffe0', 'ff18', 'ff2500000000'):
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                oc.describe(decode(bytes.fromhex(raw)))
        self.assertEqual(oc.describe(decode(bytes.fromhex('ff1578563412'))), dict(form='cell', disp=0x12345678))
        self.assertEqual(oc.describe(decode(bytes.fromhex('ff142578563412'))), dict(form='cell', disp=0x12345678))

    # --- opaque-tail-jump-v1: the JMP sibling, absolute cell operands only -------------------------------

    def test_call_records_keep_their_receipt_and_scope_text(self):
        # The tail-jump profile must not change what an existing CALL record proves or says.
        self.use_cell()
        result = self.compare()
        proof, = result['indirect_call_checks']
        self.assertEqual(list(proof), ['profile', 'offset', 'size', 'operand', 'target', 'abi', 'evidence'])
        self.assertEqual(proof, dict(profile=oc.PROFILE, offset=0, size=6, operand=cell_operand(),
                                     target='unidentified', abi='unclaimed',
                                     evidence=['evidence/synthetic_review.json#call_sites.sample']))
        self.assertEqual(result['scope'], 'complete compiler contribution and independently supplied reference '
                                          'bindings; build provenance is caller-owned' + CALL_SCOPE % 1)

    def test_tail_jump_binds_one_proven_loader_zero_cell(self):
        for code in ('ff25', 'ff2425'):
            with self.subTest(code=code):
                self.use_jump_cell(code)
                result = self.compare()
                self.assertTrue(result['relocated_equal'])
                proof, = result['indirect_call_checks']
                self.assertEqual(list(proof), ['profile', 'offset', 'size', 'operand', 'target', 'abi', 'evidence'])
                self.assertEqual((proof['profile'], proof['offset'], proof['size'], proof['operand'],
                                  proof['target'], proof['abi']),
                                 (oc.JUMP_PROFILE, 0, len(code) // 2 + 4, cell_operand(), 'unidentified', 'unclaimed'))
                self.assertEqual(result['references'][0]['destination_rva'], 0x2200)
                self.assertTrue(result['scope'].endswith(JMP_SCOPE % 1))
                self.assertNotIn('indirect CALL', result['scope'])

    def test_tail_jump_proves_the_interface_wrapper_shape(self):
        # The pending wrappers: jmp [_g_engine_interface+0x258] in a 1000-byte owner, NOP padded to 16 bytes.
        self.use_jump_cell(addend=0x258, size=1000)
        self.patch = lambda image: struct.pack_into('<I', image, OPTIONAL + 224 + 40 + 8, 0x600)
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual((result['compiled_size'], result['differing_nonreference_bytes']), (16, 0))
        proof, = result['indirect_call_checks']
        self.assertEqual((proof['profile'], proof['operand']), (oc.JUMP_PROFILE, cell_operand(0x258)))
        self.assertEqual(result['references'][0]['destination_rva'], 0x2200 + 0x258)

    def test_tail_jump_may_read_exactly_the_last_dword_of_its_cell(self):
        self.use_jump_cell(addend=4, size=8)
        self.assertTrue(self.compare()['relocated_equal'])

    def test_call_and_tail_jump_in_one_function_keep_their_own_receipts_and_counts(self):
        self.use('ff1500000000' + 'ff2504000000' + '90' * 10, record(0, cell_operand()), jump_record(6, cell_operand(4)))
        self.native = (b'\xff\x15' + struct.pack('<I', 0x402200) + b'\xff\x25' + struct.pack('<I', 0x402204) +
                       b'\x90' * 10)
        self.relocations = [(2, 1, masked.DIR32), (8, 1, masked.DIR32)]
        self.symbols.append(('_cell', 0, 0, 0))
        self.fixups = [0x3002, 0x3008, 0x3101, 0]
        self.patch = lambda image: struct.pack_into('<II', image, OPTIONAL + 96 + 5 * 8, 0x3000, 16)
        fixture_owner(self.owner, 8)
        self.owners = {'_cell': self.owner}
        self.bindings['symbols'] = {'_cell': binding(
            'bss', 0x2200, 8, (2, masked.DIR32, 0), (8, masked.DIR32, 4),
            identity=dict(rva=0x1100, size=5, operand=0, sha256=hashlib.sha256(ANCHOR).hexdigest()),
            owner=dict(source='src/owner.c', flags=[], symbol='_cell'))}
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual({p['offset']: p['profile'] for p in result['indirect_call_checks']},
                         {0: oc.PROFILE, 6: oc.JUMP_PROFILE})
        self.assertTrue(result['scope'].endswith(CALL_SCOPE % 1 + JMP_SCOPE % 1))

    def test_tail_jump_record_validation_is_closed(self):
        good = jump_record(0, cell_operand())
        oc.validate_record(good)
        oc.validate_record(jump_record(0, cell_operand(0x258)))
        bad = [dict(good, target='callee'), dict(good, abi='cdecl'), dict(good, evidence=[]),
               dict(good, evidence=['  ']), dict(good, evidence=['work/velocity_j/review.json']),
               dict(good, offset=-1), dict(good, offset=True),
               dict(good, profile='opaque-tail-jump-v2'), dict(good, profile='fixed-slot-v1'), dict(good, extra=1),
               {k: v for k, v in good.items() if k != 'abi'},
               dict(good, operand=cell_operand(-4)), dict(good, operand=cell_operand(True)),
               dict(good, operand=cell_operand(1.0)), dict(good, operand=cell_operand(symbol=7)),
               dict(good, operand=dict(cell_operand(), extra=1)),
               dict(good, operand=dict(form='import', symbol='_cell')), dict(good, operand='eax')]
        for item in bad:
            with self.subTest(item=item), self.assertRaises(ValueError):
                oc.validate_record(item)

    def test_tail_jump_register_and_register_based_memory_forms_are_refused(self):
        # A record cannot name them: jump tables and other dispatch never enter through this form ...
        for operand in (REG_EAX, dict(form='register', register='edi'), memory('eax'), memory('eax', 0x10),
                        memory('esp', 8), memory('esi', 0, 'ecx', 4), memory(None, 0, 'eax', 4)):
            with self.subTest(operand=operand), self.assertRaisesRegex(ValueError, 'only an absolute cell'):
                oc.validate_record(jump_record(0, operand))
        # ... and a compiled register or register-based JMP cannot hide behind a valid cell record.
        for code, message in [('ffe0c3', 'never a register'), ('ffe7c3', 'never a register'),
                              ('ff20c3', 'register-based'), ('ff6010c3', 'register-based'),
                              ('ff2424c3', 'register-based'), ('ff248500000000c3', 'register-based')]:
            with self.subTest(code=code):
                self.use(code, jump_record(0, cell_operand()))
                with self.assertRaisesRegex(ValueError, message):
                    self.compare()

    def test_unrecorded_tail_jumps_stay_refused(self):
        for code, message in [('ffe0c3', 'Register indirect'), ('ff20c3', 'Unsupported indirect')]:
            with self.subTest(code=code):
                self.use(code)
                with self.assertRaisesRegex(ValueError, message):
                    self.compare()
        # Even a JMP through a fully proven cell needs its own record.
        self.use_jump_cell()
        self.bindings['indirect_calls'] = []
        with self.assertRaisesRegex(ValueError, 'Unsupported indirect'):
            self.compare()

    def test_tail_jump_profile_must_match_the_decoded_mnemonic(self):
        # The JMP profile on a CALL through a cell ...
        self.use_cell(profile=oc.JUMP_PROFILE)
        with self.assertRaisesRegex(ValueError, 'FF /4 JMP'):
            self.compare()
        # ... on a CALL through a register, a PUSH or a direct CALL ...
        for code in ('ffd0c3', '51c3', 'e800000000c3'):
            with self.subTest(code=code):
                self.use(code, jump_record(0, cell_operand()))
                with self.assertRaisesRegex(ValueError, 'FF /4 JMP'):
                    self.compare()
        # ... and the CALL profile on a JMP through the very same proven cell.
        self.use_jump_cell()
        self.bindings['indirect_calls'] = [record(0, cell_operand())]
        with self.assertRaisesRegex(ValueError, 'FF /2 CALL'):
            self.compare()

    def test_tail_jump_cell_negative_controls(self):
        cases = [(dict(native_cell=0x2204), 'differs from its proven cell'),                  # wrong native displacement
                 (dict(kind='function', native_cell=0x1100), 'independently proven loader-zero'),  # non-bss record
                 (dict(operand=cell_operand(symbol='_other')), 'operand differs'),
                 (dict(operand=cell_operand(4)), 'operand differs'),
                 (dict(addend=2), 'exceeds its complete scalar owner')]                         # addend + 4 > size
        for options, message in cases:
            with self.subTest(options=options):
                self.use_jump_cell(**options)
                with self.assertRaisesRegex(ValueError, message):
                    self.compare()

    def test_tail_jump_native_instruction_must_be_the_same_cell_jump(self):
        cell_address = struct.pack('<I', 0x402200)
        cases = [(b'\xff\x15' + cell_address, 'FF /4 JMP'),     # native is a CALL through the cell
                 (b'\xff\xa0' + cell_address, 'register-based'),  # native is a register-based JMP of the same size
                 (b'\xff\xe0\x90\x90\x90\x90', 'boundary')]       # native is a shorter instruction
        for raw, message in cases:
            with self.subTest(native=raw.hex()):
                self.use_jump_cell()
                self.native = raw + b'\x90' * 10
                with self.assertRaisesRegex(ValueError, message):
                    self.compare()

    def test_tail_jump_cell_checks_hold_without_the_hub(self):
        # verify_calls enforces the cell rules itself: the hub's own scalar-owner check is not relied on.
        import capstone
        self.use_jump_cell(addend=4, size=8)
        self.assertTrue(self.compare()['relocated_equal'])  # writes the fixture image
        pe = PE(self.target)
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        instructions = list(decoder.disasm(self.code, 0x1000))
        expected = pe.read_rva(0x1000, len(self.native))
        good_records = {'_cell': dict(kind='bss', rva=0x2200, size=8)}
        good_reviewed = {2: ('_cell', oc.DIR32, 4, False)}

        def verify(records=good_records, reviewed=good_reviewed):
            return oc.verify_calls(pe, [jump_record(0, cell_operand(4))], records, reviewed, instructions, 0x1000,
                                   expected)

        proof = verify()[0]
        self.assertEqual((proof['profile'], proof['operand'], proof['size']), (oc.JUMP_PROFILE, cell_operand(4), 6))
        cases = [(dict(records={'_cell': dict(kind='bss', rva=0x2200, size=7)}), 'reads outside its proven cell'),
                 (dict(records={'_cell': dict(kind='bss', rva=0x2200, size=3)}), 'reads outside its proven cell'),
                 (dict(records={'_cell': dict(kind='data', rva=0x2200, size=8)}), 'loader-zero'),
                 (dict(records={'_cell': dict(kind='function', rva=0x1100, size=16)}), 'loader-zero'),
                 (dict(reviewed={2: ('_cell', oc.DIR32, 4, True)}), 'loader-zero'),
                 (dict(reviewed={2: ('_cell', masked.REL32, 4, False)}), 'loader-zero'),
                 (dict(reviewed={2: ('_cell', oc.DIR32, -4, False)}), 'reads outside its proven cell'),
                 (dict(reviewed={}), 'own disp32 relocation'),
                 (dict(reviewed={3: ('_cell', oc.DIR32, 4, False)}), 'own disp32 relocation'),
                 (dict(reviewed={2: ('_cell', oc.DIR32, 4, False), 4: ('_cell', oc.DIR32, 0, False)}),
                  'own disp32 relocation')]
        for options, message in cases:
            with self.subTest(options=options), self.assertRaisesRegex(ValueError, message):
                verify(**options)

    def test_tail_jump_through_a_relabelled_import_slot_is_refused(self):
        # Review finding D2, JMP form: a payload-hash 'data' record over another import's IAT slot
        # must not authorise a tail jump through the cell.
        self.use('ff2500000000' + '90' * 10, jump_record(0, cell_operand(symbol='__imp__A')))
        self.native = b'\xff\x25' + struct.pack('<I', 0x402164) + b'\x90' * 10
        self.relocations = [(2, 1, masked.DIR32)]
        self.symbols.append(('__imp__A', 0, 0, 0))
        self.fixups = [0x3002]
        self.patch = add_imports
        data = binding('data', 0x2164, 4, (2, masked.DIR32, 0), sha256=hashlib.sha256(
            struct.pack('<I', 0x21a0)).hexdigest())
        self.bindings['symbols'] = {'__imp__A': data}
        with self.assertRaisesRegex(ValueError, 'import slots or PE directories'):
            self.compare()

    def test_tail_jump_on_a_relocation_stripped_image_is_refused(self):
        self.use_jump_cell()
        self.stripped = True
        with self.assertRaisesRegex(ValueError, 'HIGHLOW'):  # the loader-zero anchor already needs the original's fixups
            self.compare()
        # The module's own rule stands independently: without an inventory, a literal looks like an address.
        pe = PE(self.target)
        self.assertFalse(pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size)
        with self.assertRaisesRegex(ValueError, 'base-relocated image'):
            oc.verify_calls(pe, [jump_record(0, cell_operand())], {}, {}, [], 0x1000, b'')

    def test_tail_jump_prefixed_and_segment_forms_are_refused(self):
        self.use_jump_cell()
        self.assertTrue(self.compare()['relocated_equal'])  # control: the same fixture without a prefix
        for code in ('66ff25', '64ff25', '2eff25', 'f2ff25', 'f3ff25'):
            with self.subTest(code=code), self.assertRaises(ValueError):
                self.use_jump_cell(code)
                self.compare()

    def test_describe_jump_admits_only_one_unprefixed_near_absolute_cell(self):
        for raw in ('e900000000', 'eb00', 'ffe0', 'ff20', 'ff6010', 'ff2424', 'ff248500000000', 'ff2d00000000',
                    '66ff2500000000', '64ff2500000000', '2eff2500000000', 'f2ff2500000000', 'ff1500000000', 'ffd0'):
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                oc.describe_jump(decode(bytes.fromhex(raw)))
        self.assertEqual(oc.describe_jump(decode(bytes.fromhex('ff2578563412'))), dict(form='cell', disp=0x12345678))
        self.assertEqual(oc.describe_jump(decode(bytes.fromhex('ff242578563412'))), dict(form='cell', disp=0x12345678))
        # The CALL profile keeps refusing every JMP, the admitted cell form included.
        for raw in ('ff2578563412', 'ff242578563412', 'ffe0', 'ff20'):
            with self.subTest(call_profile=raw), self.assertRaisesRegex(ValueError, 'FF /2 CALL'):
                oc.describe(decode(bytes.fromhex(raw)))

    def test_legacy_comparators_refuse_tail_jump_records(self):
        self.use_jump_cell()
        for comparator in (masked.relocated_compare, local.relocated_compare):
            with self.subTest(comparator=comparator.__module__), self.assertRaises(ValueError):
                self.compare(comparator)


class OpaqueDriverTests(unittest.TestCase):
    """The normal gate validates and routes both opaque profiles; only the comparator is stubbed."""

    def setUp(self):
        self.directory = workspace_temporary_directory('opaque-driver-')
        self.addCleanup(self.directory.cleanup)
        self.obj, self.target = [Path(self.directory.name) / n for n in ('fixture.obj', 'original.dll')]
        coff_file(self.obj, bytes.fromhex('ffd0c3'), [], [('_sample', 0, 1, 0x20)])
        make_pe(self.target, base_relocation=True)

    def gate(self, calls):
        # Real evidence freezing, source closure and pre-checks; no compiler process is launched.
        source = self.obj.with_suffix('.c')
        source.write_bytes(b'/* synthetic compile input */\n')
        relative = lambda p: p.resolve().relative_to(match.ROOT).as_posix()
        row = dict(id='fixture.opaque', status='CODEGEN_SIMILAR', symbol='_sample', source=relative(source),
                   target=relative(self.target), flags=['/O2'], rva='0x1000', size=3,
                   target_sha256=match.digest(self.target.read_bytes()),
                   proof=dict(source_sha256=match.digest(source.read_bytes())))
        document = self.obj.with_suffix('.json')
        document.write_text(json.dumps(dict(schema=1, evidence_files={relative(self.target): row['target_sha256']},
                                            functions={row['id']: dict(symbols={}, indirect_calls=calls)})))
        with patch.object(match, 'verify_toolchain', return_value='fixture'), \
                patch.object(match, 'compile_source', return_value=(self.obj, ['synthetic compiler'])):
            return match.verify_rows([row], relocation_candidates={row['id']: relative(document)})

    def test_either_profile_passes_the_pre_check_and_routes_to_the_hub(self):
        for calls in ([record(0, REG_EAX)], [jump_record(0, cell_operand())],
                      [record(0, REG_EAX), jump_record(3, cell_operand(4))]):
            with self.subTest(profiles=[call['profile'] for call in calls]):
                with patch.object(fixed, 'relocated_compare', side_effect=StopIteration) as hub, \
                        patch.object(masked, 'relocated_compare', side_effect=AssertionError('legacy route')), \
                        patch.object(local, 'relocated_compare', side_effect=AssertionError('local route')):
                    with self.assertRaises(StopIteration):
                        self.gate(calls)
                hub.assert_called_once()

    def test_pre_check_refuses_an_invalid_tail_jump_record_before_any_comparison(self):
        cases = [(jump_record(0, REG_EAX), 'only an absolute cell'), (jump_record(0, memory('eax')), 'only an absolute cell'),
                 (dict(jump_record(0, cell_operand()), abi='cdecl'), 'no callee identity or ABI'),
                 (dict(jump_record(0, cell_operand()), evidence=['work/review.json']), 'durable review evidence')]
        for bad, message in cases:
            with self.subTest(record=bad), patch.object(fixed, 'relocated_compare',
                                                         side_effect=AssertionError('reached the comparator')):
                with self.assertRaisesRegex(ValueError, message):
                    self.gate([bad])

    def test_an_unknown_profile_still_needs_a_rooted_callback_witness(self):
        with self.assertRaisesRegex(ValueError, 'independent native window identity'):
            self.gate([dict(jump_record(0, cell_operand()), profile='opaque-tail-jump-v2')])

    def test_comparator_routing_block_treats_either_profile_as_hub_use(self):
        source = inspect.getsource(match.verify_rows)
        start = source.index("                tables = binding.get('tables', [])")
        end = source.index('                owners = ', start)
        routing = compile(textwrap.dedent(source[start:end]), '<match routing>', 'exec')
        cases = [([record(0, REG_EAX)], fixed), ([jump_record(0, cell_operand())], fixed),
                 ([record(0, REG_EAX), jump_record(3, cell_operand(4))], fixed),
                 ([dict(jump_record(0, cell_operand()), profile='opaque-tail-jump-v2')], masked),
                 ([dict(record(0, REG_EAX), profile='opaque-indirect-call-v2')], masked),
                 ([None], masked), ([], masked)]
        for calls, module in cases:
            with self.subTest(calls=calls):
                scope = dict(binding=dict(symbols={}, indirect_calls=calls), relocated_compare=masked.relocated_compare)
                exec(routing, scope)
                self.assertIs(scope['relocated_compare'], module.relocated_compare)


@unittest.skipUnless(os.environ.get('HERCULES_TEST_ORIGINAL_ROOT'), 'set original root for native controls')
class OriginalOpaqueCallControls(unittest.TestCase):
    """Native pilot call sites decode to the reviewed forms; direct calls never qualify."""

    def test_eng3_pilot_and_interface_wrapper_call_sites(self):
        import capstone
        root = Path(__file__).resolve().parents[1]
        rows = {r['id']: r for r in json.loads((root / 'recovery.json').read_bytes())['functions']}
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        seen = {}
        for row_id in ('eng3.03290', 'eng3.11d80', 'eng3.25770', 'eng3.26eb0', 'eng1.dispatch_engine_2c620'):
            row = rows[row_id]
            pe = PE(Path(os.environ['HERCULES_TEST_ORIGINAL_ROOT']) / row['target'])
            self.assertEqual(hashlib.sha256(pe.data).hexdigest(), row['target_sha256'])
            self.assertTrue(pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size)
            rva = int(row['rva'], 0)
            forms = []
            for ins in decoder.disasm(pe.read_rva(rva, row['size']), rva):
                if ins.mnemonic != 'call':
                    continue
                if ins.bytes[0] == 0xe8:
                    with self.assertRaises(ValueError):
                        oc.describe(ins)
                    continue
                forms.append(oc.describe(ins)['form'])
            seen[row_id] = forms
        self.assertTrue(all(f == ['memory'] * len(f) and f for k, f in seen.items() if k.startswith('eng3.')))
        self.assertEqual(seen['eng1.dispatch_engine_2c620'], ['cell'])

    def test_pending_interface_wrappers_decode_to_the_admitted_tail_jump_form(self):
        import capstone
        import interface_slots
        root = Path(__file__).resolve().parents[1]
        rows = {r['id']: r for r in json.loads((root / 'recovery.json').read_bytes())['functions']}
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        decoder.detail = True
        for row_id, module in (('eng1.engine_host_update', 'ENG1.DLL'), ('eng3.1fec0', 'ENG3.DLL'),
                               ('title.0ca00', 'TITLE.DLL')):
            with self.subTest(row=row_id):
                row = rows[row_id]
                pe = PE(Path(os.environ['HERCULES_TEST_ORIGINAL_ROOT']) / row['target'])
                self.assertEqual(hashlib.sha256(pe.data).hexdigest(), row['target_sha256'])
                self.assertTrue(pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size)
                rva, raw = int(row['rva'], 0), pe.read_rva(int(row['rva'], 0), row['size'])
                ins = next(decoder.disasm(raw, rva))
                self.assertEqual((row['size'], ins.size, set(raw[ins.size:])), (16, 6, {0x90}))
                # jmp [_g_engine_interface+0x258]: one cell, no register, no prefix.
                self.assertEqual(oc.describe_jump(ins),
                                 dict(form='cell', disp=pe.image_base + interface_slots.ROOTS[module] + 0x258))
                with self.assertRaisesRegex(ValueError, 'FF /2 CALL'):  # the CALL profile never admits it
                    oc.describe(ins)

    def test_relocation_stripped_exe_is_out_of_scope(self):
        root = Path(__file__).resolve().parents[1]
        rows = json.loads((root / 'recovery.json').read_bytes())['functions']
        exe = next(r for r in rows if r['target'].upper().endswith('HERCULES.EXE'))
        pe = PE(Path(os.environ['HERCULES_TEST_ORIGINAL_ROOT']) / exe['target'])
        self.assertFalse(pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size)
        with self.assertRaisesRegex(ValueError, 'base-relocated image'):
            oc.verify_calls(pe, [], {}, {}, [], 0, b'')


if __name__ == '__main__':
    unittest.main()
