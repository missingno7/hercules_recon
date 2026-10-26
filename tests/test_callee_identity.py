"""Adversarial synthetic identity controls. No original code execution/compiles."""
import copy
import json
import struct
import sys
from pathlib import Path
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import callee_identity as ci
from pe import PE
from test_matching import CODE, make_pe, make_coff, workspace_temporary_directory


class CalleeIdentityTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('callee-identity-')
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / 'ENG1.DLL'
        make_pe(self.path)
        raw = bytearray(self.path.read_bytes())
        raw[0x440:0x450] = b'\xe8' + struct.pack('<i', 0x1000 - 0x1045) + b'\xc3' + b'\x90' * 10
        self.path.write_bytes(raw)
        self.pe = PE(self.path)
        h = ci.sha(self.path)
        self.rows = [dict(id='accepted', status='FUNCTION_MATCH', target='ENG1.DLL', target_sha256=h,
                          rva='0x1040', size=16),
                     dict(id='pending', status='RECONSTRUCTED', target='ENG1.DLL', target_sha256=h,
                          rva='0x1080', size=16)]
        self.entries = {0x1000:16}
        self.witnesses = ci.safe_witnesses(self.pe, self.rows)
        self.claims = {0x1000:{'_sample'}}

    def admit(self, **kwargs):
        arguments = dict(symbol='_sample', code=CODE, relocations=[], pe=self.pe, entries=self.entries,
                         witnesses=self.witnesses, claimed=self.claims)
        arguments.update(kwargs)
        return ci.admit(**arguments)

    def test_unique_complete_own_body_with_accepted_witness(self):
        r = self.admit()
        self.assertEqual((r['rva'], r['size'], r['witness']['row']), (0x1000,16,'accepted'))

    def test_wrong_entry_plus_16_and_neighbour(self):
        for entry in (0x1010, 0x1040):
            with self.subTest(entry=entry), self.assertRaisesRegex(ValueError, 'disagrees'):
                self.admit(expected=entry)

    def test_two_symbols_on_one_entry(self):
        with self.assertRaisesRegex(ValueError, 'two symbols'):
            self.admit(claimed={0x1000:{'_sample','_other'}})

    def test_mid_function_target(self):
        with self.assertRaisesRegex(ValueError, 'mid-function'):
            self.admit(entries={0x0ff0:32})

    def test_spurious_map_start_inside_recorded_function_rejects(self):
        with self.assertRaisesRegex(ValueError, 'mid-function/overlapping'):
            self.admit(native_spans=[(0x0ff0, 32)])

    def test_only_callers_are_pending(self):
        rows = copy.deepcopy(self.rows)
        rows[0]['status'] = 'RECONSTRUCTED'
        witnesses = ci.safe_witnesses(self.pe, rows)
        with self.assertRaisesRegex(ValueError, 'pending-only'):
            self.admit(witnesses=witnesses)

    def test_accepted_witness_overlapped_by_another_pending_row(self):
        rows = copy.deepcopy(self.rows)
        rows[1].update(rva='0x1044', size=1)
        with self.assertRaisesRegex(ValueError, 'pending-only'):
            self.admit(witnesses=ci.safe_witnesses(self.pe, rows))

    def test_near_body_with_changed_control_flow(self):
        altered = b'\x75\x00' + CODE[2:]
        with self.assertRaisesRegex(ValueError, 'absent or ambiguous'):
            self.admit(code=altered)

    def test_title_name_cannot_select_title_rva_in_eng1(self):
        # The requested RVA is deliberately irrelevant to the own-body search.
        with self.assertRaisesRegex(ValueError, 'disagrees'):
            self.admit(symbol='_title_01800', claimed={0x1000:{'_title_01800'}}, expected=0x1800)

    def test_abi_cleanup_mismatch(self):
        with self.assertRaisesRegex(ValueError, 'ABI|cleanup'):
            self.admit(symbol='_sample@4', claimed={0x1000:{'_sample@4'}})

    def test_duplicate_body_at_non_map_offset_rejects(self):
        raw = bytearray(self.path.read_bytes()); raw[0x481:0x491] = CODE
        self.path.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'ambiguous'):
            self.admit(pe=PE(self.path))

    def test_outgoing_unclosed_reference_rejects(self):
        with self.assertRaisesRegex(ValueError, 'outgoing relocations'):
            self.admit(relocations=[dict(offset=1,type=20,symbol='_unknown')])

    def test_tail_thunk_and_escaping_branch_reject(self):
        for code in (b'\xe9\x00\x00\x00\x00' + b'\x90' * 11,
                     b'\x75\x70' + CODE[2:]):
            with self.subTest(code=code), self.assertRaises(ValueError):
                ci.abi(code, 0x1000, '_sample')

    def test_scanner_emits_existing_identity_format_from_body_not_queue(self):
        root = Path(self.directory.name)
        (root / 'src').mkdir(); (root / 'maps').mkdir()
        (root / 'src/sample.c').write_text('int sample(void) { return 0; }')
        obj = root / 'sample.obj'; make_coff(obj)
        rows = copy.deepcopy(self.rows)
        rows.append(dict(id='callee', status='RECONSTRUCTED', target='ENG1.DLL',
                         target_sha256=ci.sha(self.path), rva='0x1000', size=16))
        for row in rows:
            row.update(source='src/sample.c', flags=['/O2'], symbol='_sample')
        (root / 'recovery.json').write_text(json.dumps(dict(functions=rows)))
        (root / 'maps/function_map_eng1.dll.json').write_text(json.dumps(
            dict(module='ENG1.DLL', functions=[dict(rva='0x1000',size=16)])))
        queue = dict(rows=[dict(id='pending', module='ENG1.DLL',
                     blockers=[dict(symbol='_sample',kind='function_unknown')],
                     refs=[dict(kind='function_unknown',implied_destination_rva=0x1800)])])
        qp = root / 'queue.json'; qp.write_text(json.dumps(queue))
        with patch.object(ci, 'ROOT', root), patch.object(ci.match, 'ROOT', root), \
             patch.object(ci.match, 'compile_source', return_value=(obj, ['fixture'])) as compiler:
            result = ci.scan(qp, root / 'maps', root / 'result.json', 1)
        record = result['function_identities']['ENG1.DLL:_sample']
        self.assertEqual(record['rva'], 0x1000)
        self.assertEqual(record['source'], 'src/sample.c')
        self.assertEqual(record['target'], 'ENG1.DLL')
        self.assertEqual(result['stats']['admitted_symbols'], 1)
        self.assertEqual(compiler.call_count, 1)
        self.assertEqual(len(result['compiled_inputs']), 1)

    def test_queue_caller_destination_does_not_affect_subjects(self):
        q = dict(rows=[dict(id='x',module='ENG1.DLL',blockers=[dict(symbol='_sample',kind='function_unknown')],
                           refs=[dict(implied_destination_rva=0xdeadbeef)])])
        self.assertEqual(ci.queue_symbols(q), {('ENG1.DLL','_sample'):['x']})


if __name__ == '__main__':
    unittest.main()
