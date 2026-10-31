"""Opaque indirect CALL controls; synthetic files only, originals are read and never executed."""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import masked
import opaque_call as oc
from pe import PE
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file
from test_zero_storage import fixture_owner

REG_EAX = dict(form='register', register='eax')
OPTIONAL = 0x98  # make_pe optional header; data directories start at +96
ANCHOR = b'\x68' + struct.pack('<I', 0x402200)  # push offset _cell, at RVA 0x1100


def record(offset, operand, **extra):
    result = dict(profile=oc.PROFILE, offset=offset, operand=operand, target='unidentified',
                  abi='unclaimed', evidence=['evidence/synthetic_review.json#call_sites.sample'])
    result.update(extra)
    return result


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

    def use_cell(self, code='ff15', addend=0, native_cell=None, kind='bss', operand=None, size=4):
        field = len(code) // 2
        self.use(code + struct.pack('<i', addend).hex() + 'c3',
                 record(0, operand or dict(form='cell', symbol='_cell', addend=addend)))
        self.native = (bytes.fromhex(code) + struct.pack('<I', 0x400000 + (native_cell or 0x2200 + addend)) + b'\xc3')
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
