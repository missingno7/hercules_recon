"""Adversarial relocation proof checks using only synthetic, unexecuted files."""
import copy
import hashlib
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import masked
from test_matching import make_pe, workspace_temporary_directory


def coff_file(path, code, relocations, symbols, data=None):
    """Write actual i386 COFF rather than replacing the parser in the tests."""
    sections = [(b'.text', code, 0x60501020, relocations)]
    if data is not None:
        sections.append((b'.data', data, 0xc0300040, []))
    offset = 20 + 40 * len(sections)
    headers, payload = bytearray(), bytearray()
    for name, contents, flags, uses in sections:
        rels = b''.join(struct.pack('<IIH', off, sym, typ) for off, sym, typ in uses)
        headers += struct.pack('<8sIIIIIIHHI', name, 0, 0, len(contents), offset,
                               offset + len(contents) if rels else 0, 0, len(uses), 0, flags)
        payload += contents + rels
        offset += len(contents) + len(rels)
    entries = b''.join(struct.pack('<8sIhHBB', name.encode(), value, section, typ, 2, 0)
                       for name, value, section, typ in symbols)
    path.write_bytes(struct.pack('<HHIIIHH', 0x14c, len(sections), 0, offset, len(symbols), 0, 0) +
                     headers + payload + entries + struct.pack('<I', 4))


def binding(kind, rva, size, *uses, **extra):
    return dict(kind=kind, rva=rva, size=size, evidence=['reviewed independent synthetic identity'],
                references=[dict(offset=off, type=typ, addend=addend) for off, typ, addend in uses], **extra)


class RelocatedTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('relocated-')
        self.addCleanup(self.directory.cleanup)
        self.obj = Path(self.directory.name) / 'fixture.obj'
        self.target = Path(self.directory.name) / 'fixture.dll'
        make_pe(self.target)
        self.code = bytes.fromhex('e800000000c3')
        self.native = b'\xe8' + struct.pack('<i', 0x1100 - 0x1005) + b'\xc3'
        self.relocations = [(1, 1, masked.REL32)]
        self.symbols = [('_sample', 0, 1, 0x20), ('_callee', 0, 0, 0x20)]
        self.bindings = dict(symbols={'_callee': binding('function', 0x1100, 1, (1, masked.REL32, 0))})
        self.data = None

    def compare(self, size=None):
        coff_file(self.obj, self.code, self.relocations, self.symbols, self.data)
        image = bytearray(self.target.read_bytes())
        image[0x400:0x400 + len(self.native)] = self.native
        self.target.write_bytes(image)
        return masked.relocated_compare(self.obj, '_sample', self.target, 0x1000,
                                         len(self.native) if size is None else size, self.bindings)

    def test_direct_call_and_proof_digest(self):
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual(result['references'][0]['destination_rva'], 0x1100)
        previous = result['bindings_sha256']
        self.bindings['symbols']['_callee']['evidence'].append('another review')
        self.assertNotEqual(previous, self.compare()['bindings_sha256'])

    def test_wrong_callee_rejected_even_when_masked_equal(self):
        self.bindings['symbols']['_callee']['rva'] = 0x1101
        self.assertFalse(self.compare()['relocated_equal'])
        self.assertTrue(masked.masked_compare(self.obj, '_sample', self.target, 0x1000, 6)['masked_equal'])

    def test_swapped_call_identities_rejected(self):
        self.code += bytes.fromhex('e800000000c3')
        self.native += b'\xe8' + struct.pack('<i', 0x1110 - 0x100b) + b'\xc3'
        self.symbols.append(('_other', 0, 0, 0x20))
        self.relocations.append((7, 2, masked.REL32))
        self.bindings['symbols']['_other'] = binding('function', 0x1100, 1, (7, masked.REL32, 0))
        self.bindings['symbols']['_callee']['rva'] = 0x1110
        self.assertFalse(self.compare()['relocated_equal'])

    def test_recursion_must_bind_own_complete_extent(self):
        self.symbols = self.symbols[:1]
        self.relocations = [(1, 0, masked.REL32)]
        self.native = b'\xe8' + struct.pack('<i', -5) + b'\xc3'
        self.bindings['symbols'] = {'_sample': binding('function', 0x1000, 6, (1, masked.REL32, 0))}
        self.assertTrue(self.compare()['relocated_equal'])
        self.bindings['symbols']['_sample']['size'] = 5
        with self.assertRaisesRegex(ValueError, 'Recursive'):
            self.compare()

    def test_changed_nonrelocation_byte_is_not_matching(self):
        self.native = self.native[:-1] + b'\xcc'
        self.assertFalse(self.compare()['relocated_equal'])

    def test_complete_extent_cannot_trim_compiler_padding(self):
        self.code += b'\xcc'
        with self.assertRaisesRegex(ValueError, 'Complete compiler-owned'):
            self.compare()

    def test_stale_or_changed_compiler_operand_requires_review(self):
        self.code = b'\xe8\x01\0\0\0\xc3'
        with self.assertRaisesRegex(ValueError, 'reviewed symbol/type/addend'):
            self.compare()
        self.bindings['symbols']['_callee']['references'][0]['addend'] = 1
        with self.assertRaisesRegex(ValueError, 'interior addends'):
            self.compare()

    def test_missing_extra_evidence_and_uses_rejected(self):
        original = copy.deepcopy(self.bindings)
        for transform in (
            lambda b: b['symbols'].clear(),
            lambda b: b['symbols']['_callee'].update(evidence=[]),
            lambda b: b['symbols']['_callee']['references'].append(dict(offset=2, type=20, addend=0)),
            lambda b: b.update(implied_symbols={'_callee': '0x1100'}),
        ):
            self.bindings = copy.deepcopy(original)
            transform(self.bindings)
            with self.assertRaises(ValueError):
                self.compare()

    def test_duplicate_symbols_unsupported_types_and_overlaps_rejected(self):
        self.symbols.append(('_callee', 0, 0, 0x20))
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.compare()
        self.symbols.pop()
        self.relocations = [(1, 1, masked.DIR32NB)]
        with self.assertRaisesRegex(ValueError, 'Unsupported'):
            self.compare()
        self.relocations = [(1, 1, masked.REL32), (2, 1, masked.REL32)]
        with self.assertRaisesRegex(ValueError, 'overlapping'):
            self.compare()
        self.relocations = [(4, 1, masked.REL32)]
        with self.assertRaisesRegex(ValueError, 'out-of-bounds'):
            self.compare()

    def test_function_binding_cannot_relabel_data_symbol(self):
        self.symbols[1] = ('_callee', 0, 0, 0)
        with self.assertRaisesRegex(ValueError, 'symbol type'):
            self.compare()

    def test_nonlocal_alias_into_caller_is_not_silently_accepted(self):
        self.bindings['symbols']['_callee']['rva'] = 0x1001
        with self.assertRaisesRegex(ValueError, 'alias proof unsupported'):
            self.compare()

    def test_local_relative_branch_must_land_on_an_instruction(self):
        self.relocations = []
        self.bindings = {}
        self.code = self.native = bytes.fromhex('eb0090c3')
        self.assertTrue(self.compare()['relocated_equal'])
        self.code = self.native = bytes.fromhex('ebff90c3')
        with self.assertRaisesRegex(ValueError, 'non-instruction'):
            self.compare()

    def data_fixture(self):
        self.code = b'\xa1' + struct.pack('<i', 4) + b'\xc3'
        self.native = b'\xa1' + struct.pack('<I', 0x402004) + b'\xc3'
        self.data = b'ABCDwxyz'
        image = bytearray(self.target.read_bytes())
        image[0x600:0x608] = self.data
        image[0x610:0x618] = self.data
        self.target.write_bytes(image)
        self.symbols[1] = ('_object', 0, 2, 0)
        self.relocations = [(1, 1, masked.DIR32)]
        self.bindings['symbols'] = {'_object': binding('data', 0x2000, 8, (1, masked.DIR32, 4),
                                                      sha256=hashlib.sha256(self.data).hexdigest())}

    def test_data_payload_and_interior_offset_verified(self):
        self.data_fixture()
        self.assertTrue(self.compare()['relocated_equal'])
        self.native = b'\xa1' + struct.pack('<I', 0x402005) + b'\xc3'
        self.assertFalse(self.compare()['relocated_equal'])

    def test_identical_payload_does_not_authorize_wrong_storage_identity(self):
        self.data_fixture()
        self.bindings['symbols']['_object']['rva'] = 0x2010
        self.assertFalse(self.compare()['relocated_equal'])

    def test_data_payload_hash_and_compiled_definition_both_required(self):
        self.data_fixture()
        self.bindings['symbols']['_object']['sha256'] = '0' * 64
        self.assertFalse(self.compare()['relocated_equal'])
        self.bindings['symbols']['_object']['sha256'] = hashlib.sha256(self.data).hexdigest()
        self.data = b'ABCDWRNG'
        self.assertFalse(self.compare()['relocated_equal'])

    def test_data_offset_cannot_cross_proven_object(self):
        self.data_fixture()
        self.code = b'\xa1' + struct.pack('<i', 8) + b'\xc3'
        self.bindings['symbols']['_object']['references'][0]['addend'] = 8
        with self.assertRaisesRegex(ValueError, 'Interior offset'):
            self.compare()

    def one_past_fixture(self):
        self.data_fixture()
        self.code = b'\xb8' + struct.pack('<i', 8) + b'\xc3'
        self.native = b'\xb8' + struct.pack('<I', 0x402008) + b'\xc3'
        self.bindings['symbols']['_object']['references'][0].update(addend=8, one_past=True)

    def test_explicit_reviewed_one_past_immediate_is_supported(self):
        self.one_past_fixture()
        self.assertTrue(self.compare()['relocated_equal'])
        self.bindings['symbols']['_object']['references'][0].pop('one_past')
        with self.assertRaisesRegex(ValueError, 'Interior offset'):
            self.compare()

    def test_one_past_cannot_authorize_memory_dereference(self):
        self.one_past_fixture()
        self.code = b'\xa1' + self.code[1:]
        self.native = b'\xa1' + self.native[1:]
        with self.assertRaisesRegex(ValueError, 'immediate operand, not memory'):
            self.compare()

    def test_one_past_flag_requires_exact_bound_boolean_and_actual_use(self):
        self.one_past_fixture()
        original = copy.deepcopy(self.bindings)
        for change in (dict(addend=7), dict(addend=9), dict(one_past=1), dict(offset=2)):
            self.bindings = copy.deepcopy(original)
            self.bindings['symbols']['_object']['references'][0].update(change)
            with self.assertRaises(ValueError):
                self.compare()

    def test_inconsistent_defined_section_addresses_rejected(self):
        self.data_fixture()
        self.code += b'\xa1\0\0\0\0\xc3'
        self.native += b'\xa1' + struct.pack('<I', 0x402014) + b'\xc3'
        self.symbols.append(('_member', 4, 2, 0))
        self.relocations.append((7, 2, masked.DIR32))
        self.bindings['symbols']['_member'] = binding('data', 0x2014, 4, (7, masked.DIR32, 0),
                                                     sha256=hashlib.sha256(b'wxyz').hexdigest())
        with self.assertRaisesRegex(ValueError, 'Inconsistent'):
            self.compare()

    def test_import_identity_comes_from_actual_pe_iat(self):
        image = bytearray(self.target.read_bytes())
        struct.pack_into('<II', image, 0x98 + 96 + 8, 0x2000, 40)
        struct.pack_into('<IIIII', image, 0x600, 0x2040, 0, 0, 0x2080, 0x2060)
        struct.pack_into('<II', image, 0x640, 0x2090, 0)
        struct.pack_into('<II', image, 0x660, 0x2090, 0)
        image[0x680:0x68d] = b'KERNEL32.dll\0'
        image[0x690:0x69a] = b'\0\0GetTick\0'
        self.target.write_bytes(image)
        self.code = bytes.fromhex('ff1500000000c3')
        self.native = bytes.fromhex('ff15') + struct.pack('<I', 0x402060) + b'\xc3'
        self.symbols[1] = ('_import', 0, 0, 0)
        self.relocations = [(2, 1, masked.DIR32)]
        self.bindings['symbols'] = {'_import': binding('import', 0x2060, 4, (2, masked.DIR32, 0),
                                                      dll='kernel32.DLL', name='GetTick')}
        self.assertTrue(self.compare()['relocated_equal'])
        self.bindings['symbols']['_import']['name'] = 'WrongApi'
        with self.assertRaisesRegex(ValueError, 'PE IAT'):
            self.compare()

    def table_fixture(self):
        self.code = bytes.fromhex('ff248500000000c300000000')
        self.native = bytes.fromhex('ff2485') + struct.pack('<I', 0x401008) + b'\xc3' + struct.pack('<I', 0x401007)
        self.symbols = [('_sample', 0, 1, 0x20), ('_table', 8, 1, 0), ('_case', 7, 1, 0)]
        self.relocations = [(3, 1, masked.DIR32), (8, 2, masked.DIR32)]
        self.bindings = dict(symbols={
            '_table': binding('local', 0x1008, 4, (3, masked.DIR32, 0), role='table'),
            '_case': binding('local', 0x1007, 1, (8, masked.DIR32, 0), role='code')},
            tables=[dict(offset=8, size=4, evidence=['independent switch table bounds'])])

    def test_own_section_jump_table_and_case_boundaries(self):
        self.table_fixture()
        self.assertTrue(self.compare()['relocated_equal'])
        self.native = self.native[:8] + struct.pack('<I', 0x401006)
        self.assertFalse(self.compare()['relocated_equal'])

    def test_section_symbol_addends_for_jump_table_and_case(self):
        self.table_fixture()
        self.code = bytes.fromhex('ff248508000000c307000000')
        self.symbols = [('_sample', 0, 1, 0x20), ('.text', 0, 1, 0)]
        self.relocations = [(3, 1, masked.DIR32), (8, 1, masked.DIR32)]
        self.bindings['symbols'] = {'.text': binding('local', 0x1000, 12,
            (3, masked.DIR32, 8), (8, masked.DIR32, 7), role='section')}
        self.assertTrue(self.compare()['relocated_equal'])
        self.code = self.code[:8] + struct.pack('<i', 6)
        self.bindings['symbols']['.text']['references'][1]['addend'] = 6
        with self.assertRaisesRegex(ValueError, 'instruction or table'):
            self.compare()

    def test_tables_reject_noninstruction_cases_missing_entries_and_bad_bounds(self):
        self.table_fixture()
        original = copy.deepcopy(self.bindings)
        self.symbols[2] = ('_case', 6, 1, 0)
        self.bindings['symbols']['_case']['rva'] = 0x1006
        with self.assertRaisesRegex(ValueError, 'instruction'):
            self.compare()
        self.symbols[2] = ('_case', 7, 1, 0)
        self.bindings = original
        self.bindings['tables'][0]['size'] = 8
        with self.assertRaisesRegex(ValueError, 'bounds'):
            self.compare()
        self.bindings['tables'][0]['size'] = 4
        self.relocations.pop()
        self.bindings['symbols'].pop('_case')
        with self.assertRaisesRegex(ValueError, 'inventory'):
            self.compare()

    def test_unbound_external_branch_and_register_indirect_fail_closed(self):
        self.relocations = []
        self.bindings = {}
        self.code = self.native = bytes.fromhex('e9fb000000c3')
        with self.assertRaisesRegex(ValueError, 'Unbound external'):
            self.compare()
        self.code = self.native = bytes.fromhex('ffd0c3')
        with self.assertRaisesRegex(ValueError, 'Register indirect'):
            self.compare()

    def test_highlow_base_relocation_inventory_not_silently_ignored(self):
        self.data_fixture()
        image = bytearray(self.target.read_bytes())
        struct.pack_into('<II', image, 0x98 + 96 + 5 * 8, 0x3000, 12)
        struct.pack_into('<IIHH', image, 0x800, 0x1000, 12, 0x3001, 0)
        self.target.write_bytes(image)
        self.assertTrue(self.compare()['relocated_equal'])
        image = bytearray(self.target.read_bytes())
        struct.pack_into('<H', image, 0x808, 0x3002)
        self.target.write_bytes(image)
        with self.assertRaisesRegex(ValueError, 'HIGHLOW'):
            self.compare()


if __name__ == '__main__':
    unittest.main()
