"""Adopted combreview controls: native synthetic COFF/PE files, no original execution.

The old routing snapshot is a regression expectation, never binding evidence.
"""
import copy
import hashlib
import inspect
import struct
import sys
import textwrap
import types
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import local_byte_tables as local
import masked
import match
from test_combined_byte_tables import HubByteTableTests
from test_opaque_call import record, REG_EAX
from test_relocated import binding
from test_zero_storage import fixture_owner

LEGACY_ROUTING = "                from masked import relocated_compare\n                binding, evidence, path = references[row['id']]\n                if any(record.get('anchor_profile') in ('fixed-i386-disp32-v1', 'fixed-i386-moffs32-v1', 'fixed-i386-regimm-v1')\n                       for record in binding.get('symbols', {}).values()):\n                    from fixed_disp32 import relocated_compare\n                tables = binding.get('tables', [])\n                if isinstance(tables, list) and any(isinstance(table, dict) and\n                        table.get('profile') == 'coff-local-byte-table-v1' for table in tables):\n                    from local_byte_tables import relocated_compare\n                if any('access_profile' in record or 'alias_profile' in record\n                       for record in binding.get('symbols', {}).values()):\n                    if any(isinstance(table, dict) and table.get('profile') == 'coff-local-byte-table-v1'\n                           for table in tables):\n                        raise ValueError('Indexed access and local byte table profiles cannot be combined')\n                    from fixed_disp32 import relocated_compare\n                if any(isinstance(call, dict) and call.get('profile') == 'opaque-indirect-call-v1'\n                       for call in binding.get('indirect_calls', [])):\n                    if any(isinstance(table, dict) and table.get('profile') == 'coff-local-byte-table-v1'\n                           for table in tables):\n                        raise ValueError('Opaque indirect calls and local byte table profiles cannot be combined')\n                    from fixed_disp32 import relocated_compare\n                if any(isinstance(record, dict) and record.get('data_profile') == 'file-data-owner-v1'\n                       for record in binding.get('symbols', {}).values()):\n                    if any(isinstance(table, dict) and table.get('profile') == 'coff-local-byte-table-v1'\n                           for table in tables):\n                        raise ValueError('File data owners and local byte table profiles cannot be combined')\n                    from fixed_disp32 import relocated_compare\n"

def route(which, bindings):
    source = inspect.getsource(match.verify_rows)
    start = source.index("            if row['id'] in references:\n")
    start = source.index('\n', start) + 1
    end = source.index('                owners = ', start)
    code = source[start:end] if which == 'patched' else LEGACY_ROUTING
    saved = {n: sys.modules.get(n) for n in ('masked', 'fixed_disp32', 'local_byte_tables')}
    try:
        for name in saved:
            stub = types.ModuleType(name)
            stub.relocated_compare = name
            sys.modules[name] = stub
        scope = dict(references={'x': (bindings, None, None)}, row={'id': 'x'})
        try:
            exec(compile(textwrap.dedent(code), '<comparator routing>', 'exec'), scope)
        except ValueError as exc:
            return 'refused: ' + str(exc)
        return scope['relocated_compare']
    finally:
        for name, module in saved.items():
            sys.modules[name] = module


class CombinedNegatives(HubByteTableTests):
    """Every byte-table rule must still refuse while an opaque call keeps the row in the hub."""

    CODE = '0fb6040500000000ffd0c3'  # movzx eax,byte [eax+$L1] ; call eax ; ret ; $L1: 8 bytes

    def setUp(self):
        super().setUp()
        self.configure(self.CODE, 4)
        self.fixups = [0x3004]
        self.bindings['indirect_calls'] = [record(8, copy.deepcopy(REG_EAX))]
        self.extra_image = []

    def compare(self, comparator=fixed.relocated_compare):
        extra = self.extra_image

        def wrapped(*args, **kwargs):
            if extra:
                image = bytearray(args[2].read_bytes())
                for at, data in extra:
                    image[at:at + len(data)] = data
                args[2].write_bytes(image)
            return comparator(*args, **kwargs)
        return super().compare(wrapped)

    def test_00_positive_combined_row_and_routing(self):
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual(route('patched', self.bindings), 'fixed_disp32')
        self.assertTrue(route('head', self.bindings).startswith('refused: Opaque indirect calls'))
        with self.assertRaises(ValueError):
            self.compare(local.relocated_compare)       # dedicated comparator: no opaque support
        with self.assertRaises(ValueError):
            self.compare(masked.relocated_compare)  # pinned comparator: no byte-table support

    def refuses(self, pattern):
        with self.assertRaisesRegex(ValueError, pattern):
            self.compare()

    def test_relocation_inside_table(self):
        self.relocations.append((11, 1, masked.DIR32))
        self.bindings['symbols']['$L1']['references'].append(dict(offset=11, type=masked.DIR32, addend=0))
        self.refuses('cannot contain relocations')

    def test_ambiguous_native_payload(self):
        self.extra_image = [(0x500, bytes.fromhex('020109077ffa00cc'))]
        self.refuses('ambiguous native placement')

    def test_native_fixup_inside_table(self):
        self.fixups = [0x3004, 0x300c]
        self.refuses('overlaps relocation')

    def test_native_fixup_three_bytes_before_table(self):
        self.fixups = [0x3004, 0x3008]   # field 0x1008..0x100b overlaps the table start 0x100b
        self.refuses('overlaps relocation|HIGHLOW')

    def test_trimmed_or_overlong_extent(self):
        for size in (7, 9, 4):
            self.table['size'] = size
            self.table['sha256'] = hashlib.sha256(self.code[11:11 + size]).hexdigest()
            self.bindings['symbols']['$L1']['size'] = size
            with self.subTest(size=size), self.assertRaises(ValueError):
                self.compare()

    def test_padding_split_into_second_table(self):
        # Two byte tables cannot share one allocation (second label absent / overlap).
        second = dict(self.table, offset=15, size=4, symbol='$L1',
                      sha256=hashlib.sha256(self.code[15:]).hexdigest())
        self.bindings['tables'].append(second)
        self.refuses('Overlapping|complete|Byte table')

    def test_escaping_or_nonload_uses(self):
        for code, field in [('8d0405' + '00000000' + 'ffd0c3', 3),   # lea eax,[eax+$L1]
                            ('68' + '00000000' + '90' * 3 + 'ffd0c3', 1),  # push offset $L1
                            ('ff2485' + '00000000' + '90' + 'ffd0c3', 3),  # jmp [eax*4+$L1]
                            ('880405' + '00000000' + 'ffd0c3', 3)]:        # mov [eax+$L1],al
            self.configure(code, field)
            self.fixups = [0x3000 + field]
            self.bindings['indirect_calls'] = [record(len(bytes.fromhex(code)) - 3, copy.deepcopy(REG_EAX))]
            with self.subTest(code=code), self.assertRaisesRegex(ValueError, 'direct indexed byte load'):
                self.compare()

    def test_opaque_record_inside_table_bytes(self):
        self.bindings['indirect_calls'].append(record(12, copy.deepcopy(REG_EAX)))
        self.refuses('instruction boundary')

    def test_hidden_second_register_call_still_refused(self):
        self.configure('0fb6040500000000ffd0ffd1c3', 4)
        self.bindings['indirect_calls'] = [record(8, copy.deepcopy(REG_EAX))]
        self.refuses('Register indirect')

    def test_section_symbol_addend_into_table(self):
        # Second load reaches the table through the section symbol, bypassing $L1's use checks.
        self.configure('0fb60405000000000fb68800000000ffd0c3', 4)
        self.code = bytearray(self.code)
        struct.pack_into('<i', self.code, 11, 18)               # .text+18 == $L1
        self.code = bytes(self.code)
        native = bytearray(self.native)
        struct.pack_into('<I', native, 11, 0x401000 + 18)
        self.native = native
        self.symbols.append(('.text', 0, 1, 0))
        self.relocations.append((11, 2, masked.DIR32))
        self.bindings['symbols']['.text'] = binding('local', 0x1000, len(self.code), (11, masked.DIR32, 18),
                                                    role='section')
        self.bindings['indirect_calls'] = [record(15, copy.deepcopy(REG_EAX))]
        self.fixups = [0x3004, 0x300b]
        self.refuses('Section-symbol addend|Missing or ambiguous|Byte table')

    def test_unknown_explicit_profile_next_to_byte_table(self):
        self.bindings['tables'].append(dict(offset=0, size=4, profile='other', evidence=['x']))
        self.refuses('Unsupported explicit table profile')

    def test_native_payload_mutation(self):
        self.native[12] ^= 0x40
        self.refuses('differs or has wrong native placement')

    def test_byte_table_record_cannot_carry_other_profiles(self):
        for extra in (dict(access_profile='loader-zero-indexed-v1'), dict(alias_profile='compile-renaming-byte-v1'),
                      dict(anchor_profile='loader-zero-paired-v1'), dict(data_profile='file-data-owner-v1')):
            saved = copy.deepcopy(self.bindings)
            self.bindings['symbols']['$L1'].update(extra)
            with self.subTest(extra=extra), self.assertRaises(ValueError):
                self.compare()
            self.bindings = saved


class PairedRoutingScope(HubByteTableTests):
    """A byte table does not independently enable paired-anchor routing."""

    PAIR = bytes.fromhex('bf042240008b47fc')  # mov edi,0x402204 ; mov eax,[edi-4]

    def build(self, with_table=True):
        code = '0fb6040500000000' if with_table else ''
        code += '8b0d00000000c3'                    # mov ecx,[_cell] ; ret
        cell = 10 if with_table else 2
        if with_table:
            self.configure(code, 4)
        else:
            self.code = bytes.fromhex(code)
            self.native = bytearray(self.code)
            self.symbols = [('_sample', 0, 1, 32), ('$L1', 0, 0, 0)]
            self.relocations = []
            self.bindings = dict(symbols={}, tables=[])
        self.symbols.append(('_cell', 0, 0, 0))
        self.relocations.append((cell, 2, masked.DIR32))
        struct.pack_into('<I', self.native, cell, 0x402200)
        self.owner = self.obj.with_name('owner.obj')
        fixture_owner(self.owner, 8)
        self.bindings['symbols']['_cell'] = binding(
            'bss', 0x2200, 8, (cell, masked.DIR32, 0), owner=dict(source='src/owner.c', flags=[], symbol='_cell'),
            identity=dict(rva=0x1100, size=8, addend=4, sha256=hashlib.sha256(self.PAIR).hexdigest()),
            anchor_profile='loader-zero-paired-v1')
        self.fixups = ([0x3004] if with_table else []) + [0x3000 + cell, 0x3101]

    def run_with(self, comparator):
        pair = self.PAIR

        def wrapped(*args, **kwargs):
            image = bytearray(args[2].read_bytes())
            struct.pack_into('<I', image, 0x98 + 224 + 40 + 8, 0x400)   # .data zero tail to 0x2400
            image[0x500:0x508] = pair
            args[2].write_bytes(image)
            return comparator(*args, storage_owners={'_cell': self.owner})
        return super(PairedRoutingScope, self).compare(wrapped)

    def test_paired_without_access_profile_keeps_local_route(self):
        self.build(with_table=True)
        self.assertEqual(route('patched', self.bindings), 'local_byte_tables')
        self.assertEqual(route('head', self.bindings), 'local_byte_tables')
        self.assertTrue(self.run_with(fixed.relocated_compare)['relocated_equal'])        # direct hub capability only
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit loader-zero anchor'):
            self.run_with(local.relocated_compare)                                       # HEAD route refuses

    def test_same_paired_record_without_table_is_refused_by_routing(self):
        self.build(with_table=False)
        self.assertEqual(route('patched', self.bindings), 'masked')
        self.assertEqual(route('head', self.bindings), 'masked')
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit loader-zero anchor'):
            self.run_with(masked.relocated_compare)
        # The hub itself accepts this row; only the routing trigger set differs.
        self.assertTrue(self.run_with(fixed.relocated_compare)['relocated_equal'])


class PreexistingTableRecordSpan(HubByteTableTests):
    """Regression fixed in both extensible comparators: a jump-table record sized across an adjacent byte table.

    The old role 'table' check only required positions <= table_bytes, so
    the jump table's own $L2 record may extend over the byte table $L1. A use of $L2+4 then
    reaches the byte table without verify_accesses' direct-indexed-load rule, which is keyed
    only by the byte table's own symbol. Here `lea eax,[$L2+4]` materializes the table address.
    """

    PAYLOAD = bytes.fromhex('020109077ffa00cc')

    def build(self, oversized=True):
        code = bytearray.fromhex('8d0500000000'        # 0  lea eax,[$L2+4]   (field 2)
                                 '0fb68000000000'      # 6  movzx eax,byte [eax+$L1] (field 9)
                                 'ff248500000000'      # 13 jmp [eax*4+$L2]   (field 16)
                                 'c3'                  # 20 $L3: ret
                                 '00000000') + self.PAYLOAD  # 21 $L2 jump table, 25 $L1 bytes
        struct.pack_into('<i', code, 2, 4)
        self.code = bytes(code)
        native = bytearray(code)
        for field, value in ((2, 0x401019), (9, 0x401019), (16, 0x401015), (21, 0x401014)):
            struct.pack_into('<I', native, field, value)
        self.native = native
        self.symbols = [('_sample', 0, 1, 32), ('$L1', 25, 1, 0), ('$L2', 21, 1, 0), ('$L3', 20, 1, 0)]
        self.relocations = [(2, 2, masked.DIR32), (9, 1, masked.DIR32), (16, 2, masked.DIR32), (21, 3, masked.DIR32)]
        self.table = dict(profile=local.PROFILE, symbol='$L1', offset=25, size=8,
                          sha256=hashlib.sha256(self.PAYLOAD).hexdigest(), evidence=['synthetic review'])
        self.bindings = dict(symbols={
            '$L1': binding('local', 0x1019, 8, (9, masked.DIR32, 0), role='table'),
            '$L2': binding('local', 0x1015, 12 if oversized else 4, (2, masked.DIR32, 4), (16, masked.DIR32, 0),
                           role='table'),
            '$L3': binding('local', 0x1014, 1, (21, masked.DIR32, 0), role='code')},
            tables=[dict(offset=21, size=4, evidence=['synthetic jump table']), self.table])
        self.fixups = [0x3002, 0x3009, 0x3010, 0x3015]

    def test_lea_of_byte_table_through_oversized_jump_record(self):
        self.build(oversized=True)
        for comparator in (local.relocated_compare, fixed.relocated_compare):
            with self.subTest(comparator=comparator.__module__):
                with self.assertRaisesRegex(ValueError, 'equal exactly one declared table'):
                    self.compare(comparator)
        # Control: with $L2 sized to its own 4-byte jump table the same use is refused.
        self.build(oversized=False)
        with self.assertRaisesRegex(ValueError, 'Interior offset exceeds'):
            self.compare(local.relocated_compare)


    def test_pinned_masked_gap_and_hardened_jump_only_refusal(self):
        self.build(oversized=True)
        code = bytearray(self.code)
        native = bytearray(self.native)
        for offset in (25, 29):
            struct.pack_into('<I', code, offset, 0)
            struct.pack_into('<I', native, offset, 0x401014)
            self.relocations.append((offset, 3, masked.DIR32))
            self.bindings['symbols']['$L3']['references'].append(dict(offset=offset, type=masked.DIR32, addend=0))
        self.code, self.native = bytes(code), native
        self.bindings['tables'][1] = dict(offset=25, size=8, evidence=['synthetic jump table'])
        self.fixups += [0x3019, 0x301d]
        self.assertTrue(self.compare(masked.relocated_compare)['relocated_equal'])
        self.assertEqual(route('head', self.bindings), 'masked')
        self.assertEqual(route('patched', self.bindings), 'local_byte_tables')
        for comparator in (local.relocated_compare, fixed.relocated_compare):
            with self.subTest(comparator=comparator.__module__), self.assertRaisesRegex(
                    ValueError, 'equal exactly one declared table'):
                self.compare(comparator)


class FixedAnchorExeCombination(unittest.TestCase):
    """Relocation-stripped EXE: fixed-i386-disp32-v1 anchor plus a byte table, end to end."""

    PAYLOAD = bytes.fromhex('020109077ffa00cc')

    def setUp(self):
        from test_matching import make_pe, workspace_temporary_directory
        from test_relocated import coff_file
        directory = workspace_temporary_directory('combreview-exe-')
        self.addCleanup(directory.cleanup)
        root = Path(directory.name)
        self.target, self.caller, self.owner = [root / x for x in ('original.exe', 'caller.obj', 'owner.obj')]
        make_pe(self.target)
        code = bytes.fromhex('a100000000' '0fb68800000000' 'c3') + self.PAYLOAD   # $L1 at 13
        native = bytearray(code)
        struct.pack_into('<I', native, 1, 0x402200)
        struct.pack_into('<I', native, 8, 0x40100d)
        self.size = len(code)
        data = bytearray(self.target.read_bytes())
        struct.pack_into('<H', data, 0x96, 0x103)                     # relocations stripped
        struct.pack_into('<I', data, 0x98 + 224 + 40 + 8, 0x400)      # .data zero tail
        data[0x400:0x400 + len(native)] = native
        data[0x500:0x507] = bytes.fromhex('8b0d00224000c3')          # independent anchor function
        self.target.write_bytes(data)
        coff_file(self.caller, code, [(1, 2, masked.DIR32), (8, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('$L1', 13, 1, 0), ('_cell', 0, 0, 0)])
        obj = bytearray(self.caller.read_bytes())
        obj[struct.unpack_from('<I', obj, 8)[0] + 18 + 16] = 6        # $L1 is a plain label
        self.caller.write_bytes(obj)
        fixture_owner(self.owner)
        self.table = dict(profile=local.PROFILE, symbol='$L1', offset=13, size=8,
                          sha256=hashlib.sha256(self.PAYLOAD).hexdigest(), evidence=['synthetic review'])
        self.refresh_profile()

    def refresh_profile(self):
        from pe import PE
        data = self.target.read_bytes()
        profile = dict(image=fixed.image_facts(PE(self.target)),
                       encoding=dict(disp_offset=2, disp_size=4, address_size=4, modrm=13, mnemonic='mov', width=4),
                       boundary=dict(rva=0x1100, size=7, anchor_offset=0,
                                     sha256=hashlib.sha256(data[0x500:0x507]).hexdigest(), review='evidence/native.json'),
                       excluded=[], excluded_sha256=fixed.fingerprint([]), accesses=[dict(offset=1, addend=0, width=4)])
        identity = dict(rva=0x1100, size=6, operand=1, sha256=hashlib.sha256(data[0x500:0x506]).hexdigest())
        self.bindings = dict(symbols={
            '_cell': binding('bss', 0x2200, 4, (1, masked.DIR32, 0), identity=identity,
                             owner=dict(source='src/owner.c', flags=[], symbol='_cell'),
                             anchor_profile=fixed.PROFILE, fixed_disp32=profile),
            '$L1': binding('local', 0x100d, 8, (8, masked.DIR32, 0), role='table')}, tables=[self.table])

    def compare(self, comparator=fixed.relocated_compare):
        return comparator(self.caller, '_sample', self.target, 0x1000, self.size, copy.deepcopy(self.bindings),
                          storage_owners={'_cell': self.owner})

    def test_positive_and_both_old_routes_refuse(self):
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual(result['byte_table_checks'][0]['rva'], 0x100d)
        self.assertEqual(route('patched', self.bindings), 'fixed_disp32')
        self.assertEqual(route('head', self.bindings), 'local_byte_tables')
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit loader-zero anchor'):
            self.compare(local.relocated_compare)
        with self.assertRaises(ValueError):
            self.compare(masked.relocated_compare)

    def test_ambiguous_payload_in_exe_data(self):
        data = bytearray(self.target.read_bytes())
        data[0x600:0x608] = self.PAYLOAD
        self.target.write_bytes(data)
        self.refresh_profile()
        with self.assertRaisesRegex(ValueError, 'ambiguous native placement'):
            self.compare()

    def test_byte_table_use_must_stay_a_byte_load(self):
        self.bindings['symbols']['$L1']['references'][0]['addend'] = 1
        with self.assertRaises(ValueError):
            self.compare()


def _own_tests_only(cls):
    # Inherited lane/local tests assume other layouts; run only the controls defined here.
    for name in dir(cls):
        if name.startswith('test') and name not in cls.__dict__:
            setattr(cls, name, None)


_own_tests_only(CombinedNegatives)
_own_tests_only(PairedRoutingScope)
_own_tests_only(PreexistingTableRecordSpan)
del HubByteTableTests  # do not rerun the lane's inherited suite here

if __name__ == '__main__':
    unittest.main()
