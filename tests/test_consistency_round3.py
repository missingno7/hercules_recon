"""Regression tests for Round 3 of the rvCON review."""
import hashlib
import json
import unittest
from unittest.mock import patch

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_consistency_review as fixture  # noqa: E402
from test_consistency_review import addr, ledger_row, pointer, rel, section, sym, unit
from test_consistency import M, c, ref


LITERALS = b'ABCDEFGH'
IMPORT = '__imp__Sleep@4'


class Round3(unittest.TestCase):
    setUp = fixture.Reproductions.setUp
    row = fixture.Reproductions.row
    define = fixture.Reproductions.define
    prepare_code = fixture.Reproductions.prepare_code
    collect = fixture.Reproductions.collect
    grade = fixture.Reproductions.grade

    def table(self, name, base, target, target_rva):
        self.define(name, base, LITERALS + addr(self.image.base + target_rva),
                    LITERALS + addr(0), [pointer(target, 8)])

    def test_unused_placed_pin_checks_payload(self):
        self.ev.anchors[M, '_table'] = (0x3010, 12)
        self.table('_table', 0x3010, '_actual', 0x3200)
        self.row('claim-a', ref('_claim', 0x3200))
        self.row('claim-b', ref('_claim', 0x3200), start=0x1100)
        result = self.grade('claim-a', None)
        self.assertEqual(result['grades'], {})
        self.assertEqual(result['problems'][M, '_actual'], 'shares storage with another name')

    def test_unresolved_pin_keeps_its_physical_reservation(self):
        binding = {'symbols': {'_pinned': {'rva': 0x3200, 'size': 8, 'kind': 'bss'}}}
        canonical = json.dumps(binding, sort_keys=True, separators=(',', ':')).encode()
        row = dict(id='accepted', target=M, status='FUNCTION_MATCH_RELOCATED', source='a.c', flags=[],
                   proof=dict(bindings_path='accepted.json',
                              bindings_sha256=hashlib.sha256(canonical).hexdigest()))
        data = json.dumps(dict(functions={'accepted': binding})).encode()
        with patch.object(c.Path, 'read_bytes', return_value=data):
            self.ev.anchors, self.ev.anchor_conflicts, unresolved, self.ev.opaque_pins = c.accepted_anchors(
                [row], lambda bound, name: None)
        self.assertEqual(unresolved, [('accepted', '_pinned')])
        self.assertEqual(self.ev.anchors, {})
        self.assertEqual([(base, size) for _, base, size in self.ev.opaque_pins], [(0x3200, 8)])
        self.row('claim-a', ref('_claim', 0x3204))
        self.row('claim-b', ref('_claim', 0x3204), start=0x1100)
        result = self.grade('claim-a', None)
        self.assertEqual(result['grades'], {})
        self.assertEqual(result['problems'][M, '_claim'], 'shares storage with another name')

    def test_private_same_spelling_pins_have_distinct_owners(self):
        left = (M, 'a.c', (), '_same')
        right = (M, 'b.c', (), '_same')
        self.ev.anchors[left] = (0x3200, 8)
        self.ev.anchors[right] = (0x3204, 4)
        self.row('left', ref('_same', 0x3200, key=left))
        self.row('right', ref('_same', 0x3204, key=right), start=0x1100)
        result = self.grade('left', 'B')
        self.assertEqual(result['grades']['right'], 'B')
        self.assertEqual(len(result['collisions']), 1)

    def test_alias_owner_cannot_exempt_unequal_pin_intervals(self):
        self.ev.anchors[M, '_owner'] = (0x3200, 8)
        self.ev.anchors[M, '_alias'] = (0x3204, 4, 'data', (M, '_owner'))
        self.row('owner', ref('_owner', 0x3200))
        self.row('alias', ref('_alias', 0x3204), start=0x1100)
        result = self.grade('alias', 'B')
        self.assertEqual(result['grades']['owner'], 'B')
        self.assertEqual(len(result['collisions']), 1)

    def test_native_iat_displaces_ordinary_pin_without_collision_cap(self):
        self.ev.anchors[M, '_data'] = (0x3080, 4)
        self.row('import-user', ref(IMPORT, 0x3080, width=1))
        self.row('data-user', ref('_data', 0x3080), start=0x1100)
        result = self.grade('data-user', None)
        self.assertEqual(result['grades']['import-user'], 'A')
        self.assertEqual(result['collisions'], [])
        self.assertEqual(result['conflicts'], ['X.DLL | _data | shares storage with an IAT slot'])

    def test_own_dir32_refuses_unsupported_operand_forms(self):
        for rid, start, opcode in [('segment', 0x1000, b'\x64\xa1'),
                                   ('state-save', 0x1100, b'\xdd\x35')]:
            name = '_' + rid
            raw = opcode + addr(0) + b'\xc3'
            native = opcode + addr(self.image.base + start) + b'\xc3'
            row = ledger_row(rid, name, start, len(raw))
            obj = unit([sym(name, 1, function=True)], raw, [rel(len(opcode), name)])
            self.collect(row, obj, native, [len(opcode)], refused='supported operand form')

    def test_unclassified_own_dir32_cannot_mask_instruction_bytes(self):
        # DIR32 begins on an opcode, so it is neither an operand nor a verified table cell.
        self.image.relocated = False
        start = 0x1000
        raw = addr(0) + b'\xc3'
        native = addr(self.image.base + start) + b'\xc3'
        row = ledger_row('user', '_user', start, len(raw))
        obj = unit([sym('_user', 1, function=True)], raw, [rel(0, '_user')])
        self.collect(row, obj, native, refused='supported operand form')

    def test_supported_named_own_dir32_preserves_operand(self):
        raw = b'\xa1' + addr(0) + b'\xc3'
        native = b'\xa1' + addr(self.image.base + 0x1000) + b'\xc3'
        row = ledger_row('user', '_user', 0x1000, len(raw))
        obj = unit([sym('_user', 1, function=True)], raw, [rel(1, '_user')])
        found = self.collect(row, obj, native, [1])
        self.assertEqual((found['refs'][0]['use'], found['refs'][0]['width']), ('direct', 1))

    def test_witness_marginals_are_insufficient_without_an_eligible_pair(self):
        self.image.relocated = False
        g, h = (addr(self.image.base + rva) for rva in (0x3200, 0x3300))
        mov = b'\xb8'
        gh, hg = (mov + left + mov + right for left, right in ((g, h), (h, g)))
        cases = [
            ('left', 0x1000, gh, [(1, '_g'), (6, '_h')]),
            ('near', 0x1100, gh, [(1, '_g')]),
            ('suffix', 0x1105, hg, [(1, '_h'), (6, '_g')]),
        ]
        self.ev.anchors[M, '_h'] = (0x3300, 4)
        for rid, start, native, fields in cases:
            name = '_' + rid
            row = ledger_row(rid, name, start, len(native))
            raw = bytearray(native)
            for offset, _ in fields:
                raw[offset:offset + 4] = addr(0)
            obj = unit([sym(name, 1, function=True), sym('_g'), sym('_h')],
                       bytes(raw), [rel(offset, name) for offset, name in fields])
            self.collect(row, obj, native)
        a, b, c_ = (self.ev.rows[r] for r in ('left', 'near', 'suffix'))
        self.assertEqual(a['native'], b['native'])
        self.assertEqual(a['body'], c_['body'])
        self.assertGreater(b['start'] + b['size'], c_['start'])
        result = self.grade('left', 'B')
        self.assertEqual(result['grades'], dict.fromkeys(('left', 'near', 'suffix'), 'B'))

    def test_ineligible_bridge_cannot_merge_disjoint_witnesses(self):
        self.row('left', ref('_g', 0x3200), start=0x1000)
        self.row('right', ref('_g', 0x3200), start=0x1018)
        self.grade('left', 'A')
        self.row('bridge', ref('_wrong', 0x5000), start=0x100c)
        result = self.grade('left', 'A')
        self.assertEqual(result['grades']['right'], 'A')
        self.assertIn('bridge', result['reasons'])

    def test_listing_trivia_preserves_real_continuation(self):
        for separator in ('', '; comments emit no bytes and define no symbol'):
            with self.subTest(separator=separator):
                listing = f'_DATA SEGMENT\n_g DD 0\n{separator}\n\tDD 0\n_DATA ENDS'
                sizes = c.listing_sizes(listing)
                self.assertEqual(sizes['_g'], 8)
                obj = unit([sym('_f', 1, function=True), sym('_g', 2)], b'\xc3',
                           data_sections=[section(bytes(8))], sizes=sizes)
                self.ev.definitions[M, '_g'] = [c.define(M, 'a.c', (), obj, obj.unique('_g'))]
                self.image.data[0x3014] = 9
                self.row('user', ref('_g', 0x3010))
                result = self.grade('user', None)
                self.assertEqual(result['problems'][M, '_g'], 'payload differs')

    def test_overlapping_payload_fields_fail_before_pointer_resolution(self):
        first, second = addr(self.image.base + 0x3200), addr(self.image.base + 0x3310)
        self.assertEqual(first[-1:], second[:1])
        data = LITERALS + first + second[1:]
        obj = unit([sym('_f', 1, function=True), sym('_table', 2), sym('_g'), sym('_h')], b'\xc3',
                   data_sections=[section(LITERALS + bytes(7), [rel(8, '_g'), rel(11, '_h')])],
                   sizes={'_table': len(data)})
        self.ev.definitions[M, '_table'] = [c.define(M, 'a.c', (), obj, obj.unique('_table'))]
        self.ev.anchors[M, '_g'] = (0x3200, 4)
        self.ev.anchors[M, '_h'] = (0x3310, 4)
        self.image.data[0x3010:0x3010 + len(data)] = data
        self.image.fixups = {0x3018, 0x301b}
        shifted = self.image.base + 0x01000000
        self.assertNotEqual(addr(shifted + 0x3200)[-1:], addr(shifted + 0x3310)[:1])
        self.row('user', ref('_table', 0x3010))
        self.assertIsNone(self.ev.definitions[M, '_table'][0]['refs'])
        for relocated in (True, False):
            with self.subTest(relocated=relocated):
                self.image.relocated = relocated
                if not relocated:
                    self.image.fixups.clear()
                result = self.grade('user', None)
                self.assertEqual(result['problems'][M, '_table'], 'unsupported definition')

    def test_duplicate_and_overlapping_code_fields_fail_before_masking(self):
        self.image.relocated = False
        raw = b'\xa1' + addr(0) + b'\xc3'
        native = b'\xa1' + addr(self.image.base + 0x3200) + b'\xc3'
        row = ledger_row('user', '_user', 0x1000, len(raw))
        for offset in (1, 2):
            with self.subTest(offset=offset):
                obj = unit([sym('_user', 1, function=True), sym('_g'), sym('_h')], raw,
                           [rel(1, '_g'), rel(offset, '_h')])
                self.collect(row, obj, native, refused='overlapping or out-of-bounds code relocation fields')

    def test_pending_placements_are_order_independent_and_retracted_on_conflict(self):
        self.table('_first', 0x3010, '_second', 0x3034)
        self.table('_second', 0x3030, '_data', 0x3200)
        self.row('first', ref('_first', 0x3010))
        self.row('second', ref('_second', 0x3030), start=0x1100)
        self.row('good-a', ref('_data', 0x3220), start=0x1200)
        self.row('good-b', ref('_data', 0x3220), start=0x1300)
        result = self.grade('first', None)
        self.assertEqual(result['problems'][M, '_second'], 'several addresses')
        self.assertEqual(result['grades'], {'good-a': 'A', 'good-b': 'A'})
        self.ev.rows = dict(reversed(list(self.ev.rows.items())))
        self.assertEqual(c.evaluate(self.ev), result)

    def test_failed_sibling_definition_blocks_its_entire_sources_equations(self):
        self.table('_bad', 0x3010, '_g', 0x3220)
        self.ev.definitions[M, '_bad'].append(dict(size=12, payload=b'wrong bytes!', refs=[]))
        self.row('good-a', ref('_g', 0x3200), start=0x1100)
        self.row('good-b', ref('_g', 0x3200), start=0x1200)
        self.grade('good-a', 'A')
        self.row('bad', ref('_bad', 0x3010))
        result = self.grade('good-a', 'A')
        self.assertEqual(result['grades'], {'good-a': 'A', 'good-b': 'A'})
        self.assertNotIn('X.DLL | _g | several addresses', result['conflicts'])
        self.assertIn('X.DLL | _bad | payload differs', result['defects'])

    def test_nested_accepted_pointer_contradiction_retains_address_cause(self):
        self.ev.anchors[M, '_pinned'] = (0x3200, 4)
        self.table('_inner', 0x3030, '_pinned', 0x3220)
        self.table('_outer', 0x3010, '_inner', 0x3030)
        self.row('accepted', ref('_outer', 0x3010), accepted=True)
        result = c.evaluate(self.ev)
        self.assertEqual(result['accepted_failing'], ['accepted'])
        self.assertEqual(result['accepted_disagreement'], ['accepted'])
        self.assertIn('X.DLL | _inner | payload pointer: disagrees with the map', result['conflicts'])
        self.assertEqual(result['problems'][M, '_outer'],
                         'payload pointer: payload pointer: disagrees with the map')

    def test_payload_failure_does_not_hide_accepted_address_disagreement(self):
        self.ev.anchors[M, '_pinned'] = (0x3010, 8)
        self.ev.definitions[M, '_pinned'] = [dict(size=8, payload=LITERALS, refs=[])]
        self.row('accepted', ref('_pinned', 0x3020), accepted=True)
        result = c.evaluate(self.ev)
        self.assertEqual(result['accepted_failing'], ['accepted'])
        self.assertEqual(result['accepted_disagreement'], ['accepted'])
        self.assertEqual(result['conflicts'], ['X.DLL | _pinned | several addresses'])
        self.assertEqual(result['defects'], ['X.DLL | _pinned | payload differs'])

    def test_per_use_overlap_is_visible_to_check_inputs(self):
        self.ev.anchors[M, '_first'] = (0x3200, 4)
        self.ev.anchors[M, '_second'] = (0x3204, 4)
        self.row('store', ref('_first', 0x3200, width=8))
        result = self.grade('store', None)
        self.assertIn('overlaps another object', result['reasons']['store'][0])
        self.assertEqual(result['collisions'] + result['defects'], [])
        self.assertEqual(result['conflicts'], [
            'X.DLL | _first | row store reference 1 | access beyond the accepted extent overlaps another object'])


class SwitchTables(unittest.TestCase):
    """Integrator rule after fixCON: a local-label DIR32 cell outside any operand is a switch table entry."""
    setUp = fixture.Reproductions.setUp
    collect = fixture.Reproductions.collect
    prepare_code = fixture.Reproductions.prepare_code

    def case(self, relocated, refused=None):
        self.image.relocated = relocated
        start = 0x1000
        raw = b'\xc3\x90\x90\x90' + addr(0)
        native = b'\xc3\x90\x90\x90' + addr(self.image.base + start)
        row = ledger_row('switch', '_switch', start, len(raw))
        obj = unit([sym('_switch', 1, function=True), sym('$L1', 1, 0, storage=c.STATIC)], raw, [rel(4, '$L1')])
        return self.collect(row, obj, native, [4] if relocated else [], refused=refused)

    def test_table_cell_is_collected_with_base_relocations(self):
        self.assertEqual(self.case(True)['refs'], [])

    def test_table_cell_is_refused_without_base_relocations(self):
        self.case(False, refused='supported operand form')


if __name__ == '__main__':
    unittest.main()

