"""Regression tests from rvCON round 2 (work/review_consistency/REVIEW.md); each asserts the corrected outcome."""
import hashlib
import json
import unittest
from types import SimpleNamespace as NS
from unittest.mock import patch

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_consistency_review as fixture  # noqa: E402
from test_consistency_review import addr, ledger_row, pointer, rel, section, sym, unit  # noqa: E402
from test_consistency import M, c, ref  # noqa: E402


IMPORT = '__imp__Sleep@4'
LITERALS = b'ABCDEFGH'


# Reuse fixture methods without inheriting and rerunning the Round 1 tests.
class Round2(unittest.TestCase):
    setUp = fixture.Reproductions.setUp
    row = fixture.Reproductions.row
    define = fixture.Reproductions.define
    collect = fixture.Reproductions.collect
    prepare_code = fixture.Reproductions.prepare_code
    gather = fixture.Reproductions.gather
    grade = fixture.Reproductions.grade

    def table(self, target, target_rva, *, addend=0, function=False, name='_table', base=0x3010):
        self.define(name, base, LITERALS + addr(self.image.base + target_rva + addend),
                    LITERALS + addr(addend), [pointer(target, 8, function=function)])

    def import_code(self, rid='importer', opcode=b'\xff\x15', *, slot=0x3080, refused=None):
        native = opcode + addr(self.image.base + slot) + b'\xc3'
        raw = opcode + addr(0) + b'\xc3'
        name = '_' + rid
        row = ledger_row(rid, name, 0x1000, len(native))
        obj = unit([sym(name, 1, function=True), sym(IMPORT, function=True)],
                   raw, [rel(len(opcode), IMPORT)])
        return self.collect(row, obj, native, [len(opcode)], refused=refused)

    def test_using_import_removes_its_interval_reservation(self):
        self.define('_ordinary', 0x307c, LITERALS)
        self.row('data-user', ref('_ordinary', 0x307c))
        self.grade('data-user', None)  # 0x307c..0x3084 overlaps the IAT slot.
        found = self.import_code()
        self.assertFalse(found['refs'][0]['function'])
        self.grade('importer', 'A')
        self.grade('data-user', None)

    def test_real_symbol_can_shadow_synthetic_iat_key(self):
        self.define('__iat__Sleep', 0x3080, LITERALS)
        self.row('data-user', ref('__iat__Sleep', 0x3080))
        self.grade('data-user', None)

    def test_ordinal_iat_slot_is_never_reserved(self):
        native = NS(OPTIONAL_HEADER=NS(DATA_DIRECTORY=[NS(Size=1)] * 6),
                    DIRECTORY_ENTRY_IMPORT=[NS(dll=b'COMCTL32.dll', imports=[
                        NS(address=self.image.base + 0x3080, name=None, ordinal=17)])])
        pe = NS(image_base=self.image.base, pe=native, read_rva=self.image.read,
                sections=[dict(virtual_address=0x3000, virtual_size=0x100,
                               raw_size=0x100, characteristics=0)])
        with patch.object(c, 'PE', return_value=pe):
            self.ev.images[M] = image = c.Image('synthetic.dll')
        self.assertEqual(image.imports, {0x3080: 'ordinal:17'})
        self.define('_ordinary', 0x3080, LITERALS)
        self.row('data-user', ref('_ordinary', 0x3080))
        self.grade('data-user', None)

    def test_import_store_counts_as_a_direct_read(self):
        self.import_code(opcode=b'\xa3', refused='direct read')  # mov [Sleep's slot], eax

    def test_payload_alone_treats_an_import_as_ordinary_data(self):
        # The only IAT entry named Sleep is 0x3080. The table points to a BSS cell.
        self.table(IMPORT, 0x3200)
        self.row('user', ref('_table', 0x3010))
        self.grade('user', None)

    def test_payload_import_addend_can_point_to_adjacent_slot(self):
        self.image.imports[0x3084] = 'Beep'
        self.import_code()
        self.table(IMPORT, 0x3080, addend=4)
        self.row('table-user', ref('_table', 0x3010), start=0x1100)
        self.grade('table-user', None)

    def test_payload_can_reclassify_function_as_data_without_conflict(self):
        self.ev.functions[M, '_callee'] = [(0x1200, 'callee', True)]
        self.row('caller', ref('_callee', 0x1200, use='branch', function=True))
        self.table('_callee', 0x1200)  # In this TU the symbol is declared as data.
        self.row('table-user', ref('_table', 0x3010), start=0x1100)
        self.grade('caller', None)
        self.grade('table-user', None)

    def test_payload_can_reclassify_data_anchor_as_function(self):
        self.ev.anchors[M, '_callee'] = (0x1200, 4, 'data')
        self.ev.functions[M, '_callee'] = [(0x1200, 'callee', True)]
        self.table('_callee', 0x1200, function=True)
        self.row('table-user', ref('_table', 0x3010))
        self.grade('table-user', None)

    def test_payload_only_function_can_override_accepted_base(self):
        self.ev.anchors[M, '_callee'] = (0x1300, 0, 'function')
        self.ev.functions[M, '_callee'] = [(0x1200, 'callee', True)]
        self.table('_callee', 0x1200, function=True)
        self.row('table-user', ref('_table', 0x3010))
        self.grade('table-user', None)

    def test_payload_only_function_cannot_use_its_matching_anchor(self):
        self.ev.anchors[M, '_callee'] = (0x1200, 0, 'function')
        self.table('_callee', 0x1200, function=True)
        self.row('table-user', ref('_table', 0x3010))
        result = self.grade('table-user', 'A')
        self.assertNotIn((M, '_callee'), result['problems'])

    def test_wrong_pointer_to_anchored_data_disqualifies_its_correct_user(self):
        self.ev.anchors[M, '_g'] = (0x3200, 4)
        self.table('_g', 0x3220)
        self.row('bad-table', ref('_table', 0x3010))
        self.row('correct-user', ref('_g', 0x3200), start=0x1100)
        result = self.grade('bad-table', None)
        self.assertEqual(result['grades'].get('correct-user'), 'A')   # the wrong table carries the failure
        self.assertNotIn((M, '_g'), result['problems'])

    def test_accepted_payload_address_error_is_missing_from_check_inputs(self):
        self.ev.anchors[M, '_g'] = (0x3200, 4)
        self.table('_g', 0x3220)
        self.row('accepted-bad-table', ref('_table', 0x3010), accepted=True)
        self.table('_g', 0x3200, name='_other', base=0x3030)
        self.row('good-table', ref('_other', 0x3030), start=0x1100)
        result = self.grade('good-table', 'A')
        self.assertEqual(result['accepted_failing'], ['accepted-bad-table'])
        self.assertEqual(result['accepted_disagreement'], ['accepted-bad-table'])
        self.assertIn('X.DLL | _table | payload pointer: disagrees with the map', result['conflicts'])

    def test_valid_cycle_and_backward_failure_propagation(self):
        self.table('_right', 0x3030, name='_left')
        self.table('_left', 0x3010, name='_right', base=0x3030)
        self.row('root', ref('_left', 0x3010))
        self.grade('root', 'A')
        self.image.data[0x3030] ^= 1
        result = self.grade('root', None)
        self.assertEqual(result['problems'][M, '_right'], 'payload differs')
        self.assertTrue(result['problems'][M, '_left'].startswith('payload pointer'))

    def test_current_reference_inflates_extent_and_manufactures_accepted_collision(self):
        self.ev.anchors[M, '_first'] = (0x3200, 4)
        self.ev.anchors[M, '_second'] = (0x3204, 4)
        self.row('store', ref('_first', 0x3200, width=8))
        result = self.grade('store', None)
        self.assertEqual(result['collisions'], [])

    def test_bad_base_inflates_accepted_extent_for_correct_users(self):
        self.ev.anchors[M, '_first'] = (0x3200, 4)
        self.ev.anchors[M, '_second'] = (0x3204, 4)
        self.row('correct', ref('_second', 0x3204), start=0x1100)
        self.grade('correct', 'A')
        self.row('wrong-base', ref('_first', 0x3250, width=8))
        self.grade('wrong-base', None)
        self.grade('correct', 'A')

    def test_equal_extent_accepted_aliases_bypass_collision_report(self):
        for name in ('_first', '_second'):
            self.ev.anchors[M, name] = (0x3200, 4, 'data', '_owner')
            self.row(name, ref(name, 0x3200), start=0x1000 if name == '_first' else 0x1100)
        result = self.grade('_first', 'A')
        self.assertEqual(result['grades']['_second'], 'A')
        self.assertEqual(result['collisions'], [])

    def test_accepted_weak_row_still_votes_at_fixed_point(self):
        self.row('accepted', ref('_g', 0x3200), ref('_unwitnessed', 0x3250), accepted=True)
        self.row('pending', ref('_g', 0x3200), start=0x1100)
        self.grade('pending', 'B')

    def test_accepted_colliding_row_still_votes_at_fixed_point(self):
        self.ev.anchors[M, '_first'] = (0x3240, 8)
        self.ev.anchors[M, '_second'] = (0x3244, 4)
        self.row('accepted', ref('_g', 0x3200), ref('_first', 0x3240), accepted=True)
        self.row('pending', ref('_g', 0x3200), start=0x1100)
        result = self.grade('pending', 'B')
        self.assertEqual(len(result['collisions']), 1)

    def test_two_ledger_slices_count_same_operand_as_two_witnesses(self):
        for rid, start, prefix in [('full', 0x1000, b'\x90'), ('suffix', 0x1001, b'')]:
            raw = prefix + b'\xa1' + addr(0) + b'\xc3'
            native = prefix + b'\xa1' + addr(self.image.base + 0x3200) + b'\xc3'
            record = ledger_row(rid, '_' + rid, start, len(raw))
            obj = unit([sym('_' + rid, 1, function=True), sym('_g')], raw, [rel(len(prefix) + 1, '_g')])
            self.collect(record, obj, native, [len(prefix) + 1])
        # The one native absolute field is at 0x1002 in both rows.
        self.assertEqual(self.image.fixups, {0x1002})
        self.grade('full', 'B')
        self.grade('suffix', 'B')

    def test_own_rel32_masks_a_non_branch_immediate(self):
        raw = b'\xb8' + addr(0) + b'\xc3'
        native = b'\xb8' + addr(0xfffffffd) + b'\xc3'
        row = ledger_row('user', '_user', 0x1000, len(raw))
        obj = unit([sym('_user', 1, function=True), sym('$local', 1, 2, storage=c.STATIC)],
                   raw, [rel(1, '$local', c.REL32)])
        self.collect(row, obj, native, refused='not a branch')

    def test_own_self_call_disagrees_with_accepted_entry(self):
        raw = b'\xe8' + addr(0) + b'\xc3'
        native = b'\xe8' + addr(0xfffffffb) + b'\xc3'
        row = ledger_row('user', '_user', 0x1000, len(raw))
        obj = unit([sym('_user', 1, function=True)], raw, [rel(1, '_user', c.REL32)])
        self.prepare_code(row, native, [])
        self.gather([row], {('a.c', ()): obj})
        self.ev.anchors[M, '_user'] = (0x1200, 0, 'function')
        self.grade('user', None)

    def test_fnsave_store_width_still_undercounted(self):
        opcode = b'\xdd\x35'  # 32-bit FNSAVE saves 108 bytes.
        raw = opcode + addr(0) + b'\xc3'
        native = opcode + addr(self.image.base + 0x3200) + b'\xc3'
        row = ledger_row('store', '_store', 0x1000, len(raw))
        obj = unit([sym('_store', 1, function=True), sym('_state')], raw, [rel(2, '_state')])
        self.collect(row, obj, native, [2], refused='operand form')

    def test_decimal_hex_and_nested_dup_forms_are_counted(self):
        listing = '\n'.join(['_DATA SEGMENT', '_a DD 2 DUP(00H)',
                             '_b DB 02H DUP(01H, 3 DUP(02H))', '_DATA ENDS'])
        self.assertEqual(c.listing_sizes(listing), {'_a': 8, '_b': 8})

    def test_org_prefix_can_shrink_a_definition(self):
        listing = '_DATA SEGMENT\n_g DD 0\n\tORG $+0AH\n_DATA ENDS'
        sizes = c.listing_sizes(listing)
        self.assertEqual(sizes['_g'], 14)
        obj = unit([sym('_f', 1, function=True), sym('_g', 2)], b'\xc3',
                   data_sections=[section(bytes(14))], sizes=sizes)
        definition = c.define(M, 'a.c', (), obj, obj.unique('_g'))
        self.assertEqual(definition['payload'], bytes(14))
        self.image.data[0x3018] = 9
        self.ev.definitions[M, '_g'].append(definition)
        self.row('user', ref('_g', 0x3010))
        self.grade('user', None)
        self.assertIsNone(c.listing_sizes('_DATA SEGMENT\n_g DD 0\n\tORG $+X\n_DATA ENDS')['_g'])
        self.assertIsNone(c.listing_sizes('_DATA SEGMENT\n_g DD 0\n\tWEIRD 3\n_DATA ENDS')['_g'])

    def test_duplicate_definition_is_overwritten_by_good_payload(self):
        first = b'bad-byte'
        second = LITERALS
        obj = unit([sym('_f', 1, function=True), sym('_g', 2), sym('_g', 3)], b'\xc3',
                   data_sections=[section(first), section(second)], sizes={'_g': 8})
        self.ev.definitions[M, '_g'].extend(c.unit_definitions(M, 'a.c', (), obj)[M, '_g'])
        self.assertEqual([d['refs'] for d in self.ev.definitions[M, '_g']], [None])
        self.image.data[0x3010:0x3018] = second
        self.row('user', ref('_g', 0x3010))
        self.grade('user', None)

    def test_static_function_binding_can_be_reused_as_global_anchor(self):
        # A hash-pinned binding from a.c's caller names its separate static _f.
        binding = {'symbols': {'_f': {'rva': 0x1200, 'size': 8, 'kind': 'function'}}}
        digest = hashlib.sha256(json.dumps(binding, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        pinned = ledger_row('accepted', '_a', 0x1100, 8, source='a.c')
        pinned['status'] = 'FUNCTION_MATCH_RELOCATED'
        pinned['proof'] = {'bindings_path': 'binding.json', 'bindings_sha256': digest}
        with patch.object(c.Path, 'read_bytes',
                          return_value=json.dumps(dict(functions={'accepted': binding})).encode()):
            self.ev.anchors, self.ev.anchor_conflicts, _ = c.accepted_anchors(
                [pinned], lambda row, name: (M, 'a.c', (), name))   # _f is static in a.c
        self.ev.functions[M, 'a.c', (), '_f'] = [(0x1200, 'private-f', True)]
        self.ev.starts[M, 0x1200] = 'private-f'
        self.row('global-caller', ref('_f', 0x1200, use='branch', function=True))
        self.grade('global-caller', None)

    def test_segment_relative_import_is_classified_as_direct(self):
        self.import_code(opcode=b'\x64\xff\x15', refused='operand form')  # call dword ptr fs:[disp32]


if __name__ == '__main__':
    unittest.main()

