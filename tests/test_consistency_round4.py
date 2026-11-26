"""Round 4 regression tests."""
import hashlib
import json
from pathlib import Path
import unittest
from unittest.mock import patch

import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_consistency_review as fixture  # noqa: E402
from test_consistency_review import addr, ledger_row, pointer, rel, section, sym, unit
from test_consistency import M, c, ref


LITERALS = b'ABCDEFGH'


class Round4(unittest.TestCase):
    setUp = fixture.Reproductions.setUp
    row = fixture.Reproductions.row
    define = fixture.Reproductions.define
    prepare_code = fixture.Reproductions.prepare_code
    collect = fixture.Reproductions.collect
    grade = fixture.Reproductions.grade

    def table(self, name, base, *targets):
        native = LITERALS + b''.join(addr(self.image.base + rva) for _, rva in targets)
        own = LITERALS + bytes(4 * len(targets))
        refs = [pointer(name, 8 + 4 * i) for i, (name, _) in enumerate(targets)]
        self.define(name, base, native, own, refs)

    def own_label(self, rid, start, opcode, storage=c.STATIC, refused=None):
        tail = b'\xc3\xc3'
        target_offset = len(opcode) + 5
        raw = opcode + addr(0) + tail
        native = opcode + addr(self.image.base + start + target_offset) + tail
        row = ledger_row(rid, '_' + rid, start, len(raw))
        obj = unit([sym('_' + rid, 1, function=True),
                    sym('_label', 1, target_offset, storage=storage)],
                   raw, [rel(len(opcode), '_label')])
        return self.collect(row, obj, native, [len(opcode)], refused=refused), native

    def test_dll_own_label_refuses_unsupported_instruction_operands(self):
        for rid, start, opcode in [('segment', 0x1000, b'\x64\xa1'),
                                   ('save', 0x1100, b'\xdd\x35')]:
            with self.subTest(form=rid):
                _, native = self.own_label(rid, start, opcode, refused='supported operand form')
                self.assertIsNone(c.accesses(self.decoder, native, start, len(opcode)))


    def test_dll_unclassified_field_cannot_mask_executed_instruction_bytes(self):
        start = 0x1000
        raw = addr(0) + b'\xc3'
        native = addr(self.image.base + start + 4) + b'\xc3'
        row = ledger_row('user', '_user', start, len(raw))
        obj = unit([sym('_user', 1, function=True), sym('$L1', 1, 4, storage=c.STATIC)],
                   raw, [rel(0, '$L1')])
        self.assertEqual(next(self.decoder.disasm(native, start)).address, start)
        self.assertIsNone(c.accesses(self.decoder, native, start, 0))
        self.collect(row, obj, native, [0], refused='supported operand form')

    def test_own_section_labels_require_local_linkage(self):
        for rid, start, opcode in [('first', 0x1000, b'\xb8'), ('second', 0x1100, b'\x64\xa1')]:
            self.own_label(rid, start, opcode, storage=c.EXTERNAL, refused='without local linkage')

    def test_genuine_indexed_dispatch_and_fixup_backed_table(self):
        start = 0x1000
        jmp = b'\xff\x24\x85'  # jmp dword ptr [eax*4 + disp32]
        raw = jmp + addr(0) + b'\xc3' + addr(0)
        native = jmp + addr(self.image.base + start + 8) + b'\xc3' + addr(self.image.base + start + 7)
        row = ledger_row('switch', '_switch', start, len(raw))
        obj = unit([sym('_switch', 1, function=True), sym('$Table', 1, 8, storage=c.LABEL),
                    sym('$Case', 1, 7, storage=c.LABEL)],
                   raw, [rel(3, '$Table'), rel(8, '$Case')])
        self.assertEqual(c.accesses(self.decoder, native, start, 3), ('indexed', 0, False))
        self.collect(row, obj, native, [3, 8])
        self.grade('switch', 'A')
        self.image.fixups.remove(start + 8)
        self.collect(row, obj, native, [3], refused='not instruction-equal')

    def test_adjacent_tables_share_a_contiguous_fixup_run(self):
        start = 0x1000
        jump = b'\xff\x24\x85'
        raw = b'\x74\x07' + jump + addr(0) + jump + addr(0) + b'\xc3' + bytes(8)
        native = (b'\x74\x07' + jump + addr(self.image.base + start + 17)
                  + jump + addr(self.image.base + start + 21) + b'\xc3'
                  + addr(self.image.base + start + 16) * 2)
        row = ledger_row('adjacent', '_adjacent', start, len(raw))
        obj = unit([sym('_adjacent', 1, function=True), sym('$T1', 1, 17, storage=c.LABEL),
                    sym('$T2', 1, 21, storage=c.LABEL), sym('$Case', 1, 16, storage=c.LABEL)],
                   raw, [rel(5, '$T1'), rel(12, '$T2'), rel(17, '$Case'), rel(21, '$Case')])
        self.collect(row, obj, native, [5, 12, 17, 21])
        self.grade('adjacent', 'A')

    def test_incompatible_verified_alias_placements_are_refused(self):
        owner = (M, '_owner')
        self.ev.anchors[owner] = (0x3200, 4)
        self.ev.anchors[M, '_alias'] = (0x3240, 4, 'data', owner)
        self.row('owner-use', ref('_owner', 0x3200))
        self.row('alias-use', ref('_alias', 0x3240), start=0x1100)
        result = self.grade('alias-use', None)
        self.assertEqual(result['grades'], {})
        self.assertIn('X.DLL | _owner | accepted alias pins disagree', result['conflicts'])
        self.assertEqual(result['collisions'], [])

    def test_exact_verified_alias_inherits_failed_owner_definition(self):
        owner = (M, '_owner')
        self.ev.anchors[owner] = (0x3200, 8)
        self.ev.anchors[M, '_alias'] = (0x3200, 8, 'data', owner)
        self.ev.definitions[owner].append(dict(size=8, payload=LITERALS, refs=[]))
        self.row('owner-use', ref('_owner', 0x3200))
        self.row('alias-use', ref('_alias', 0x3200), start=0x1100)
        result = self.grade('alias-use', None)
        self.assertNotIn('owner-use', result['grades'])
        self.assertIn('X.DLL | _owner | initialized definition outside file-backed data', result['defects'])
        self.assertEqual(result['collisions'], [])

    def test_failed_pointer_source_cannot_release_trusted_storage(self):
        self.ev.anchors[M, '_root'] = (0x3010, 12)
        self.ev.anchors[M, '_stop'] = (0x3250, 4)
        self.table('_root', 0x3010, ('_next', 0x3030))
        self.table('_next', 0x3030, ('_actual', 0x3200))
        self.table('_suspect', 0x3050, ('_next', 0x3034), ('_stop', 0x3260))
        self.row('claim-a', ref('_claim', 0x3200))
        self.row('claim-b', ref('_claim', 0x3200), start=0x1100)
        result = self.grade('claim-a', None)
        self.assertIn((M, '_actual'), result['problems'])

        self.row('invalid-source', ref('_suspect', 0x3050), start=0x1200)
        result = self.grade('claim-a', None)
        self.assertNotIn('claim-b', result['grades'])
        self.assertIn((M, '_actual'), result['problems'])
        self.assertIn('payload pointer: disagrees with the map: _suspect',
                      result['reasons']['invalid-source'])

    def test_visibility_directive_preserves_an_unlabeled_continuation(self):
        listing = '_DATA SEGMENT\n_g DD 0\nPUBLIC _next\n\tDD 0\n_next DD 0\n_DATA ENDS'
        sizes = c.listing_sizes(listing, exact=True)
        self.assertEqual(sizes['_g'], 8)
        obj = unit([sym('_f', 1, function=True), sym('_g', 2), sym('_next', 2, 8)],
                   b'\xc3', data_sections=[section(bytes(12))], sizes=sizes)
        self.ev.definitions[M, '_g'] = [c.define(M, 'a.c', (), obj, obj.unique('_g'))]
        self.image.data[0x3014] = 9
        self.row('user', ref('_g', 0x3010))
        self.assertEqual(self.grade('user', None)['problems'][M, '_g'], 'payload differs')

    def test_collection_reports_same_symbol_address_contradiction_in_accepted_row(self):
        op = b'\xa1'
        native = op + addr(self.image.base + 0x3200) + op + addr(self.image.base + 0x3220) + b'\xc3'
        raw = op + addr(0) + op + addr(0) + b'\xc3'
        row = ledger_row('accepted', '_accepted', 0x1000, len(raw))
        row['status'] = 'FUNCTION_MATCH_RELOCATED'
        binding = {'symbols': {
            '_g': {'kind': 'bss', 'rva': 0x3200, 'size': 4},
            '_h': {'kind': 'bss', 'rva': 0x3220, 'size': 4},
        }}
        serialized = json.dumps(binding, sort_keys=True, separators=(',', ':')).encode()
        row['proof'] = {'bindings_path': 'accepted.json',
                        'bindings_sha256': hashlib.sha256(serialized).hexdigest()}
        obj = unit([sym('_accepted', 1, function=True), sym('_g'), sym('_h')],
                   raw, [rel(1, '_g'), rel(6, '_g')])
        self.prepare_code(row, native, [1, 6])
        inputs = {
            'recovery.json': {'functions': [row]},
            'accepted.json': {'functions': {'accepted': binding}},
        }
        with (patch.object(c.Path, 'read_bytes', lambda path: json.dumps(inputs[path.name]).encode()),
              patch.object(c, 'Image', return_value=self.image),
              patch.object(c, 'data_units', return_value=[]),
              patch.object(c, 'compile_unit', return_value=obj)):
            self.ev = c.gather(jobs=1)
        self.assertEqual(self.ev.refused, {'accepted': 'not instruction-equal'})
        result = c.evaluate(self.ev)
        self.assertEqual(result['accepted_disagreement'], ['accepted'])
        self.assertEqual(result['conflicts'], ['X.DLL | _g | row accepted | incompatible relocation addresses'])
        ledger = json.dumps(inputs['recovery.json']).encode()
        debt = json.dumps(dict(conflicts=result['conflicts'], defects=[], collisions=[])).encode()
        with (patch.object(c.Path, 'read_bytes', lambda p: ledger if p.name == 'recovery.json' else debt),
              patch.object(c, 'gather', return_value=self.ev),
              patch.object(c.sys, 'argv', ['consistency.py', 'check']),
              patch('builtins.print')):
            self.assertEqual(c.main(), 1)

    def test_ambiguous_table_does_not_release_possible_storage(self):
        self.ev.anchors[M, '_root'] = (0x3010, 12)
        self.table('_root', 0x3010, ('_next', 0x3030))
        self.table('_next', 0x3030, ('_actual', 0x3200))
        self.table('_suspect', 0x3050, ('_next', 0x3034))
        self.row('claim-a', ref('_claim', 0x3200))
        self.row('claim-b', ref('_claim', 0x3200), start=0x1100)
        self.row('suspect', ref('_suspect', 0x3050), start=0x1200)
        result = self.grade('claim-a', None)
        self.assertNotIn('claim-b', result['grades'])
        self.assertIn('X.DLL | _claim | shares storage with another name', result['conflicts'])

    def test_table_requires_case_boundary_and_unexecuted_cell(self):
        start = 0x1000
        jmp = b'\xff\x24\x85'
        for variant, case in [('middle of an instruction', 8), ('case jumps into the cell', 7)]:
            with self.subTest(variant=variant):
                suffix = b'\xb0\xc3\xc3' if case == 8 else b'\xeb\x01\xc3'
                raw = jmp + addr(0) + suffix + addr(0)
                native = jmp + addr(self.image.base + start + 10) + suffix + addr(self.image.base + start + case)
                row = ledger_row('switch', '_switch', start, len(raw))
                obj = unit([sym('_switch', 1, function=True), sym('$Table', 1, 10, storage=c.STATIC),
                            sym('$Case', 1, case, storage=c.STATIC)], raw, [rel(3, '$Table'), rel(10, '$Case')])
                self.collect(row, obj, native, [3, 10], refused='supported operand form')

    def test_conflicting_pins_still_reserve_both_opaque_intervals(self):
        rows, bindings = [], {}
        for name, rva in [('first', 0x3200), ('second', 0x3220)]:
            b = {'symbols': {'_key': {'rva': rva, 'size': 8, 'kind': 'bss'}}}
            canonical = json.dumps(b, sort_keys=True, separators=(',', ':')).encode()
            rows.append(dict(id=name, target=M, status='FUNCTION_MATCH_RELOCATED',
                             proof=dict(bindings_path='accepted.json',
                                        bindings_sha256=hashlib.sha256(canonical).hexdigest())))
            bindings[name] = b
        with patch.object(c.Path, 'read_bytes', return_value=json.dumps(dict(functions=bindings)).encode()):
            self.ev.anchors, self.ev.anchor_conflicts, unresolved, self.ev.opaque_pins = c.accepted_anchors(
                rows, lambda bound, name: (M, name))
        self.assertEqual(unresolved, [])
        self.assertEqual(set(self.ev.anchor_conflicts), {(M, '_key')})
        self.assertEqual({(base, size) for _, base, size in self.ev.opaque_pins}, {(0x3200, 8), (0x3220, 8)})
        self.row('claim-first', ref('_first', 0x3204))
        self.row('claim-second', ref('_second', 0x3224), start=0x1100)
        self.assertEqual(c.evaluate(self.ev)['grades'], {})

    def test_pairwise_witnesses_are_retested_after_each_round(self):
        self.row('one', ref('_g', 0x3200))
        self.row('two', ref('_g', 0x3200), ref('_h', 0x3230), start=0x1100, accepted=True)
        self.row('three', ref('_h', 0x3230), ref('_weak', 0x3250), start=0x1200)
        outcome = self.grade('one', 'B')
        self.assertIn('no qualifying witness pair: X.DLL | _g', outcome['grade_b_reasons']['one'])
        self.ev.anchors[M, '_weak'] = (0x3250, 4)
        self.grade('one', 'A')


if __name__ == '__main__':
    unittest.main()

