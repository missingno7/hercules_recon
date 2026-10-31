"""Byte-table checks in the opt-in hub; synthetic COFF/PE files, never execution."""
import copy
import hashlib
import inspect
import struct
import textwrap
import unittest
from unittest.mock import patch

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_disp32 as fixed
import local_byte_tables as local
import masked
import match
from pe import PE
from coff import COFF
import test_local_byte_tables as original
from test_relocated import binding, coff_file
from test_zero_storage import fixture_owner
from test_opaque_call import record, REG_EAX


class HubByteTableTests(original.LocalByteTableTests):
    # Run every existing table negative and end-to-end control against the hub.
    def compare(self, comparator=fixed.relocated_compare):
        if getattr(self, 'fixups', None) is None:
            return super().compare(comparator)
        def relocated(*args):
            image = bytearray(self.target.read_bytes())
            entries = list(self.fixups)
            entries += [0] * (len(entries) % 2)
            struct.pack_into('<II', image, 0x98 + 96 + 5 * 8, 0x3000, 8 + 2 * len(entries))
            struct.pack_into('<II' + 'H' * len(entries), image, 0x800, 0x1000, 8 + 2 * len(entries), *entries)
            self.target.write_bytes(image)
            return comparator(*args)
        return super().compare(relocated)

    def test_binding_extent_and_review_inventory_cannot_trim(self):
        self.bindings['symbols']['$L1']['size'] = 1
        with self.assertRaisesRegex(ValueError, 'complete allocation'):
            self.compare()
        self.bindings['symbols']['$L1']['size'] = 8
        self.table['profile'] = 'unknown-profile'
        # No byte-table opt-in remains: preserve the old hub jump-table checks.
        with self.assertRaisesRegex(ValueError, 'Reviewed relocation/table entry inventory'):
            self.compare()

    def configure(self, code, field):
        payload = bytes.fromhex('020109077ffa00cc')
        raw = bytes.fromhex(code)
        off = len(raw)
        self.code = raw + payload
        self.native = bytearray(self.code)
        struct.pack_into('<I', self.native, field, 0x401000 + off)
        self.table.update(offset=off, size=8, sha256=hashlib.sha256(payload).hexdigest())
        self.symbols = [('_sample', 0, 1, 32), ('$L1', off, 1, 0)]
        self.relocations = [(field, 1, masked.DIR32)]
        self.bindings['symbols'] = {'$L1': binding('local', 0x1000 + off, 8,
                                                  (field, masked.DIR32, 0), role='table')}

    def test_helpers_are_the_reviewed_implementations(self):
        self.assertIs(fixed.verify_byte_table, local.verify_table)
        self.assertIs(fixed.verify_byte_table_accesses, local.verify_accesses)
        self.assertEqual(fixed.BYTE_TABLE_PROFILE, local.PROFILE)

    def test_declaration_schema_types_and_evidence_reject(self):
        good = copy.deepcopy(self.table)
        bad = [dict(good, extra=1), {k:v for k,v in good.items() if k != 'symbol'},
               dict(good, offset=True), dict(good, offset=-1), dict(good, size=True),
               dict(good, size=0), dict(good, size=65537), dict(good, evidence=[])]
        for table in bad:
            with self.subTest(table=table):
                self.bindings['tables'] = [table]
                with self.assertRaises(ValueError):
                    self.compare()

    def test_unique_plain_label_and_complete_section_reject(self):
        self.table['symbol'] = '$Lnone'
        with self.assertRaisesRegex(ValueError, 'unique COFF label'):
            self.compare()
        self.table['symbol'] = '$L1'
        self.symbols.append(('$L1', 10, 1, 0))
        with self.assertRaisesRegex(ValueError, 'ambiguous duplicate COFF symbol'):
            self.compare()
        self.symbols.pop()
        self.symbols[1] = ('$L1', 9, 1, 16)
        with self.assertRaisesRegex(ValueError, 'plain compiler-local'):
            self.compare()
        self.symbols[1] = ('$L1', 9, 1, 0)
        # Exact hook input mutations cover malformed/incomplete COFF metadata
        # that the parser normally refuses before the table checker is reached.
        verify = local.verify_table
        for key, value, message in [('aux', [b'x'], 'plain compiler-local'),
                                    ('section', 2, 'plain compiler-local'),
                                    ('value', 10, 'plain compiler-local')]:
            def altered(coff, section, number, pe, rva, table, records):
                next(s for s in coff.symbols.values() if s['name'] == '$L1')[key] = value
                return verify(coff, section, number, pe, rva, table, records)
            with self.subTest(key=key), patch.object(fixed, 'verify_byte_table', altered):
                with self.assertRaisesRegex(ValueError, message):
                    self.compare()
        def incomplete(coff, section, *args):
            section['data'] = section['data'][:-1]
            return verify(coff, section, *args)
        with patch.object(fixed, 'verify_byte_table', incomplete):
            with self.assertRaisesRegex(ValueError, 'Incomplete compiler section'):
                self.compare()

    def test_binding_kind_role_rva_and_uses_reject(self):
        good = copy.deepcopy(self.bindings['symbols']['$L1'])
        for change in [dict(kind='data'), dict(role='code'), dict(rva=0x100a),
                       dict(size=7), dict(references=[])]:
            self.bindings['symbols']['$L1'] = dict(good, **change)
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.compare()

    def test_all_byte_load_shapes_and_fields(self):
        # One base OR one index, byte-sized source, register destination.
        for code, field in [('0fb6040500000000c3', 4), ('0fbe040500000000c3', 4),
                            ('8a040500000000c3', 3), ('0fb68000000000c3', 3)]:
            self.configure(code, field)
            self.assertTrue(self.compare()['relocated_equal'])
        for code, field in [('8d040500000000c3', 3), ('88040500000000c3', 3),
                            ('8b040500000000c3', 3), ('0fb600c3', 0),
                            ('0fb60500000000c3', 3), ('0fb6040000000000c3', 4),
                            ('0fb6044500000000c3', 4), ('640fb6040500000000c3', 5)]:
            self.configure(code, field)
            with self.subTest(code=code), self.assertRaises(ValueError):
                self.compare()
        self.configure('0fb6040500000000c3', 4)
        for key, value in [('offset', 3), ('type', masked.REL32), ('addend', 1)]:
            use = self.bindings['symbols']['$L1']['references'][0]
            old = use[key]
            use[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.compare()
            use[key] = old

    def test_mixed_unknown_explicit_table_refuses(self):
        self.bindings['tables'].append(dict(offset=0, size=4, profile='unknown', evidence=['review']))
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit table'):
            self.compare()

    def test_opaque_combination_and_missing_or_wrong_call_reject(self):
        self.configure('0fb6040500000000ffd0c3', 4)
        self.fixups = [0x3004]
        self.bindings['indirect_calls'] = [record(8, copy.deepcopy(REG_EAX))]
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual(len(result['byte_table_checks']), 1)
        self.assertEqual(result['indirect_call_checks'][0]['target'], 'unidentified')
        self.bindings['indirect_calls'][0]['operand']['register'] = 'ecx'
        with self.assertRaisesRegex(ValueError, 'operand differs'):
            self.compare()
        self.bindings['indirect_calls'] = []
        with self.assertRaisesRegex(ValueError, 'Register indirect'):
            self.compare()

    def test_indexed_combination_and_other_form_still_checks(self):
        self.configure('0fb60405000000008b0c8500000000c3', 4)
        self.relocations.append((11, 2, masked.DIR32))
        self.symbols.append(('_cell', 0, 0, 0))
        native = bytearray(self.native)
        struct.pack_into('<I', native, 11, 0x402200)
        self.native = native
        owner = self.obj.with_name('owner.obj')
        fixture_owner(owner, 8)
        from test_loader_zero_indexed import decode, fixed_indexed_receipt
        anchor = bytes.fromhex('6800224000')
        rec = binding('bss', 0x2200, 8, (11, masked.DIR32, 0),
                      owner=dict(source='src/owner.c', flags=[], symbol='_cell'),
                      identity=dict(rva=0x1100, size=5, operand=0, sha256=hashlib.sha256(anchor).hexdigest()),
                      access_profile='loader-zero-indexed-v1')
        ins = decode(bytes.fromhex('8b0c8500000000'), 0x1008)
        survey = dict(fixed_indexed_receipt(ins, 11, 0, 8), profile='loader-zero-indexed-v1')
        rec['loader_zero_indexed'] = dict(extent_claim='minimum', accesses=[survey])
        self.bindings['symbols']['_cell'] = rec
        def compare(*args):
            image = bytearray(self.target.read_bytes())
            struct.pack_into('<I', image, 0x98 + 224 + 40 + 8, 0x400)
            image[0x500:0x505] = anchor
            struct.pack_into('<II', image, 0x98 + 96 + 5 * 8, 0x3000, 16)
            struct.pack_into('<IIHHHH', image, 0x800, 0x1000, 16, 0x3004, 0x300b, 0x3101, 0)
            self.target.write_bytes(image)
            return fixed.relocated_compare(*args, storage_owners={'_cell': owner})
        self.assertTrue(super().compare(compare)['relocated_equal'])
        rec['loader_zero_indexed']['accesses'][0]['width'] = 2
        with self.assertRaisesRegex(ValueError, 'survey differs'):
            super().compare(compare)

    def test_combined_routing_for_each_existing_hub_trigger(self):
        triggers = [dict(anchor_profile=p) for p in ('fixed-i386-disp32-v1', 'fixed-i386-moffs32-v1', 'fixed-i386-regimm-v1')]
        triggers += [dict(access_profile='loader-zero-indexed-v1'), dict(alias_profile='compile-renaming-byte-v1'),
                     dict(opaque=True)]
        for trigger in triggers:
            # Stop at the comparator boundary after the driver's real loading,
            # evidence freezing and fixture compile; retain the valid table row.
            with self.subTest(trigger=trigger):
                original_binding = copy.deepcopy(self.bindings)
                if trigger.get('opaque'):
                    self.bindings['indirect_calls'] = [record(0, REG_EAX)]
                else:
                    self.bindings['symbols']['$L1'].update(trigger)
                # Pre-driver profile checks are independently covered by the
                # respective form suites; isolate routing from those requirements.
                with patch.object(fixed, 'access_form'), \
                        patch.object(fixed, 'relocated_compare', side_effect=StopIteration) as hub, \
                        patch.object(local, 'relocated_compare', side_effect=AssertionError('local route')):
                    with self.assertRaises(StopIteration):
                        self.normal_gate()
                    hub.assert_called_once()
                self.bindings = original_binding

    def test_routing_matrix_including_file_owner_and_nonopt_defaults(self):
        # Execute the actual driver block: importing a comparator is the full
        # observable effect. This avoids synthesizing unrelated owner proofs.
        source = inspect.getsource(match.verify_rows)
        start = source.index("                tables = binding.get('tables', [])")
        end = source.index('                owners = ', start)
        routing = compile(textwrap.dedent(source[start:end]), '<match routing>', 'exec')
        triggers = [dict(anchor_profile=p) for p in ('fixed-i386-disp32-v1', 'fixed-i386-moffs32-v1', 'fixed-i386-regimm-v1')]
        triggers += [dict(access_profile='loader-zero-indexed-v1'), dict(alias_profile='compile-renaming-byte-v1'),
                     dict(data_profile='file-data-owner-v1'), dict(opaque=True), {}]
        for table_inventory in ([], [copy.deepcopy(self.table)], [{'profile': 'other-profile'}], None, [None]):
            byte_opt = table_inventory == [self.table]
            for trigger in triggers:
                binding = dict(symbols={'_x': trigger}, tables=table_inventory)
                if trigger.get('opaque'):
                    binding['indirect_calls'] = [record(0, REG_EAX)]
                scope = dict(binding=binding, relocated_compare=masked.relocated_compare)
                exec(routing, scope)
                expected = fixed.relocated_compare if trigger else (local.relocated_compare if byte_opt else masked.relocated_compare)
                with self.subTest(tables=table_inventory, trigger=trigger):
                    self.assertIs(scope['relocated_compare'], expected)
        for table_inventory in ([], [self.table]):
            scope = dict(binding=dict(symbols={'_x': dict(anchor_profile='loader-zero-paired-v1')},
                                      tables=table_inventory), relocated_compare=masked.relocated_compare)
            exec(routing, scope)
            self.assertIs(scope['relocated_compare'], local.relocated_compare if table_inventory else masked.relocated_compare)


if __name__ == '__main__':
    unittest.main()
