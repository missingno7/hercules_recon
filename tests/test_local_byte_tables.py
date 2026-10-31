"""Actual COFF/PE fixtures exercise the opt-in full comparator, never execution."""
import copy
import hashlib
import inspect
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import local_byte_tables as local
import local_table_verify
import masked
import match
import promote
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import coff_file, binding


class LocalByteTableTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('local-byte-table-')
        self.addCleanup(self.directory.cleanup)
        self.obj, self.target = [Path(self.directory.name) / n for n in ('fixture.obj', 'original.dll')]
        make_pe(self.target)
        self.code = bytes.fromhex('0fb6040500000000c3') + bytes.fromhex('020109077ffa00cc')
        self.native = bytearray(self.code)
        struct.pack_into('<I', self.native, 4, 0x401009)
        self.table = dict(profile=local.PROFILE, symbol='$L1', offset=9, size=8,
                          sha256=hashlib.sha256(self.code[9:]).hexdigest(), evidence=['synthetic review'])
        self.bindings = dict(symbols={'$L1': binding('local', 0x1009, 8, (4, 6, 0), role='table')},
                             tables=[self.table])
        self.duplicate = False
        self.relocations = [(4, 1, 6)]
        self.symbols = [('_sample', 0, 1, 32), ('$L1', 9, 1, 0)]
        self.label_storage = 6
        self.native_table_relocation = False

    def compare(self, comparator=local.relocated_compare):
        coff_file(self.obj, self.code, self.relocations, self.symbols)
        obj = bytearray(self.obj.read_bytes())
        symptr = struct.unpack_from('<I', obj, 8)[0]
        obj[symptr + 18 + 16] = self.label_storage
        self.obj.write_bytes(obj)
        pe = bytearray(self.target.read_bytes())
        pe[0x400:0x400 + len(self.native)] = self.native
        if self.duplicate:
            pe[0x500:0x508] = self.code[9:]
        if self.native_table_relocation:
            struct.pack_into('<II', pe, 0x98 + 96 + 5 * 8, 0x2000, 12)
            struct.pack_into('<IIHH', pe, 0x600, 0x1000, 12, 0x3009, 0)
        self.target.write_bytes(pe)
        return comparator(self.obj, '_sample', self.target, 0x1000, len(self.code), self.bindings)

    def test_complete_allocation_and_base_comparator_stays_closed(self):
        self.assertTrue(self.compare()['relocated_equal'])
        self.assertEqual(self.compare()['byte_table_checks'][0]['size'], 8)
        with self.assertRaises(ValueError):
            self.compare(masked.relocated_compare)

    def test_partial_overlong_bad_hash_and_native_mutation_refuse(self):
        for size in (1, 7, 9, 1000000000):
            self.table['size'] = size
            with self.assertRaises(ValueError):
                self.compare()
        self.table['size'] = 8
        self.table['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'hash differs'):
            self.compare()
        self.table['sha256'] = hashlib.sha256(self.code[9:]).hexdigest()
        self.native[-1] ^= 1
        with self.assertRaisesRegex(ValueError, 'differs or has wrong'):
            self.compare()

    def test_ambiguous_payload_cannot_bind_from_caller_operand(self):
        self.duplicate = True
        with self.assertRaisesRegex(ValueError, 'ambiguous native placement'):
            self.compare()

    def test_binding_extent_and_review_inventory_cannot_trim(self):
        self.bindings['symbols']['$L1']['size'] = 1
        with self.assertRaisesRegex(ValueError, 'complete allocation'):
            self.compare()
        self.bindings['symbols']['$L1']['size'] = 8
        self.table['profile'] = 'unknown-profile'
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit table'):
            self.compare()

    def test_lea_store_and_nonbyte_memory_uses_refuse(self):
        for raw in ('8d84050000000090c3', '8884050000000090c3', '8b84050000000090c3'):
            self.code = bytes.fromhex(raw) + self.code[9:]
            # These instructions' disp32 field begins at 3, so use-level check
            # rejects the declared field as well as the unsupported operation.
            with self.assertRaises(ValueError):
                self.compare()

    def test_duplicate_table_or_relocation_inside_table_refuses(self):
        self.bindings['tables'].append(copy.deepcopy(self.table))
        with self.assertRaisesRegex(ValueError, 'Overlapping'):
            self.compare()
        self.bindings['tables'].pop()
        self.relocations.append((9, 1, 6))
        with self.assertRaisesRegex(ValueError, 'cannot contain relocations'):
            self.compare()

    def test_native_relocation_and_label_aliases_refuse(self):
        self.native_table_relocation = True
        with self.assertRaisesRegex(ValueError, 'overlaps relocation'):
            self.compare()
        self.native_table_relocation = False
        self.symbols.append(('$Lother', 9, 1, 0))
        with self.assertRaisesRegex(ValueError, 'complete COFF allocation'):
            self.compare()

    def test_next_label_boundary_and_wrong_storage_refuse(self):
        self.symbols.append(('$Lnext', 13, 1, 0))
        with self.assertRaisesRegex(ValueError, 'complete COFF allocation'):
            self.compare()
        self.symbols.pop()
        self.label_storage = 2
        with self.assertRaisesRegex(ValueError, 'plain compiler-local'):
            self.compare()

    def normal_gate(self, publish=False):
        # Real COFF, PE, source closure and evidence freezing. Only the compiler
        # and toolchain identity are synthetic; no compiler process is launched.
        self.compare(lambda *args, **kwargs: None)
        source = self.obj.with_suffix('.c')
        source.write_bytes(b'/* synthetic compile input */\n')
        relative = lambda p: p.resolve().relative_to(match.ROOT).as_posix()
        row = dict(id='fixture.byte-table', status='CODEGEN_SIMILAR', symbol='_sample',
                   source=relative(source), target=relative(self.target), flags=['/O2'],
                   rva='0x1000', size=len(self.code), target_sha256=match.digest(self.target.read_bytes()),
                   proof=dict(source_sha256=match.digest(source.read_bytes())))
        document = self.obj.with_suffix('.json')
        document.write_text(json.dumps(dict(schema=1,
            evidence_files={relative(self.target): row['target_sha256']},
            functions={row['id']: self.bindings})))
        with patch.object(match, 'verify_toolchain', return_value='fixture'), \
                patch.object(match, 'compile_source', return_value=(self.obj, ['synthetic compiler'])):
            if publish:
                # The caller roots this entire transaction in its private
                # fixture directory; the real recovery ledger is never touched.
                state = match.ROOT / 'recovery.json'
                state.write_text(json.dumps(dict(functions=[row])))
                with patch.object(promote, 'ROOT', match.ROOT), patch.object(promote, 'STATE', state), \
                        patch.object(promote, 'JOURNAL', match.ROOT / 'work/promotion-journal.json'):
                    promote.promote_relocated([row['id']], document)
                    accepted = json.loads(state.read_text())['functions']
                    self.assertEqual(accepted[0]['status'], match.RELOCATED)
                    return match.verify_rows(accepted)
            return match.verify_rows([row], relocation_candidates={row['id']: relative(document)})

    def test_wrapper_uses_normal_driver(self):
        self.assertIs(local_table_verify.verify_rows, match.verify_rows)

    def test_normal_gate_dispatches_explicit_profile(self):
        with patch.object(local, 'relocated_compare', wraps=local.relocated_compare) as comparator, \
                patch.object(masked, 'relocated_compare', side_effect=AssertionError('legacy dispatch')):
            results, statistics = self.normal_gate()
        comparator.assert_called_once()
        self.assertTrue(results[0]['exact'])
        self.assertEqual(results[0]['byte_table_checks'][0]['size'], 8)
        self.assertEqual(statistics['rows'], 1)

    def test_relocated_promotion_and_accepted_reverification_use_normal_gate(self):
        root = Path(self.directory.name).resolve()
        (root / 'toolchains').mkdir()
        (root / 'toolchains/manifest.json').write_text('{}')
        (root / 'work').mkdir()
        with patch.object(match, 'ROOT', root):
            results, _ = self.normal_gate(publish=True)
        self.assertTrue(results[0]['exact'])
        self.assertEqual(results[0]['byte_table_checks'][0]['size'], 8)

    def test_normal_gate_routes_table_records_without_byte_profile(self):
        original = copy.deepcopy(self.table)
        for tables in ([], [{k: v for k, v in original.items() if k != 'profile'}],
                       [dict(original, profile='other-profile')], None, [None]):
            with self.subTest(tables=tables):
                self.bindings['tables'] = tables
                with patch.object(local, 'relocated_compare', side_effect=StopIteration) as legacy, \
                        patch.object(masked, 'relocated_compare', side_effect=AssertionError('unhardened table route')):
                    with self.assertRaises(StopIteration):
                        self.normal_gate()
                legacy.assert_called_once()

    def test_normal_gate_retains_anchor_dispatch_without_table_profile(self):
        self.bindings['tables'] = []
        self.bindings['symbols']['$L1']['anchor_profile'] = 'fixed-i386-moffs32-v1'
        with patch('fixed_disp32.relocated_compare', side_effect=StopIteration) as anchored, \
                patch.object(local, 'relocated_compare', side_effect=AssertionError('unexpected opt-in')):
            with self.assertRaises(StopIteration):
                self.normal_gate()
        anchored.assert_called_once()

    def test_comparator_is_verbatim_except_explicit_table_checks(self):
        expected = inspect.getsource(masked.relocated_compare)
        expected = expected.replace('    table_bytes, table_starts = set(), set()',
            '    table_bytes, table_starts = set(), set()\n    byte_tables = []')
        expected = expected.replace('        cells = set(range(off, off + length))\n',
            "        if table.get('profile') == PROFILE and (off + length > size or length > 65536):\n"
            "            raise ValueError('Invalid bounded byte table extent')\n        cells = set(range(off, off + length))\n")
        expected = expected.replace('        if length % 4 or off + length > size or table_bytes & cells:',
            "        if table.get('profile') == PROFILE:\n"
            "            byte_tables.append(verify_table(coff, section, number, pe, rva, table, records))\n"
            "            if table_bytes & cells:\n                raise ValueError('Overlapping byte table')\n"
            "            table_bytes |= cells\n            continue\n"
            "        if 'profile' in table:\n            raise ValueError('Unsupported explicit table profile')\n"
            '        if length % 4 or off + length > size or table_bytes & cells:')
        expected = expected.replace('    fields, actual_offsets = set(), set()',
            '    verify_accesses(byte_tables, records, operand_fields, rva)\n    fields, actual_offsets = set(), set()')
        expected = expected.replace('    if alias_checks:\n',
            "    if byte_tables:\n        result['byte_table_checks'] = byte_tables\n    if alias_checks:\n")
        self.assertEqual(expected.count("                if not positions <= table_bytes:\n                    raise ValueError('Local table binding exceeds declared jump table')\n"), 1)
        expected = expected.replace("                if not positions <= table_bytes:\n                    raise ValueError('Local table binding exceeds declared jump table')\n", "                if not positions <= table_bytes:\n                    raise ValueError('Local table binding exceeds declared jump table')\n                if sum(table['offset'] == dest - rva and table['size'] == extent\n                       for table in tables) != 1:\n                    raise ValueError('Local table binding must equal exactly one declared table')\n", 1)
        self.assertEqual(expected.count('    has offset, size and evidence and delimits an embedded DIR32 jump table.\n'), 1)
        expected = expected.replace('    has offset, size and evidence and delimits an embedded DIR32 jump table.\n', '    has offset, size and evidence and delimits an embedded DIR32 jump table;\n    coff-local-byte-table-v1 additionally declares a complete relocation-free\n    byte allocation with its symbol and SHA-256. Each local table record must\n    equal exactly one declared table, never a subrange or a union of tables.\n', 1)
        self.assertEqual(inspect.getsource(local.relocated_compare), expected)


if __name__ == '__main__':
    unittest.main()
