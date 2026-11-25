"""Regression tests from the rvCON review (work/review_consistency/REVIEW.md); each asserts the corrected outcome."""
import hashlib
import json
import struct
import unittest
from types import SimpleNamespace
from unittest.mock import patch

import sys
from pathlib import Path

import capstone

sys.path.insert(0, str(Path(__file__).resolve().parent))
from test_consistency import FakeImage, M, c, ref  # noqa: E402


def addr(value):
    return struct.pack('<I', value)


def sym(name, section=0, value=0, storage=c.EXTERNAL, function=False):
    return dict(name=name, section=section, value=value, storage=storage,
                type=0x20 if function else 0, aux=[])


def rel(offset, name, kind=c.DIR32):
    return dict(offset=offset, symbol=name, type=kind)


def section(data, relocations=(), flags=0x40, name='.data'):
    return dict(name=name, flags=flags, data=data, size=len(data), relocations=list(relocations))


def unit(symbols, instructions, relocations=(), data_sections=(), sizes=None):
    coff = c.COFF.__new__(c.COFF)
    coff.symbols = dict(enumerate(symbols))
    coff.sections = [section(instructions, relocations, flags=0x20001020, name='.text'),
                     *data_sections]
    return c.Unit(coff, sizes)


def pointer(name, offset=4, key=None, function=False):
    return dict(offset=offset, name=name, key=key or (M, name), function=function,
                defined=False, section=0, value=0, common=0)


def ledger_row(rid, symbol, start, size, source='a.c', flags=()):
    return dict(id=rid, target=M, rva=hex(start), size=size, symbol=symbol,
                source=source, flags=list(flags), status='CODEGEN_SIMILAR')


class Reproductions(unittest.TestCase):
    def setUp(self):
        self.image = FakeImage()
        self.ev = c.Evidence({M: self.image})
        self.decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.decoder.detail = True

    def row(self, rid, *refs, body=None, start=0x1000, accepted=False):
        self.ev.rows[rid] = dict(refs=list(refs), body=body or rid, start=start,
                                 size=16, module=M, accepted=accepted)

    def define(self, name, base, native, payload=None, refs=()):
        self.image.data[base:base + len(native)] = native
        self.ev.definitions[M, name].append(
            dict(size=len(native), payload=native if payload is None else payload, refs=list(refs)))
        self.image.fixups.update(base + r['offset'] for r in refs)

    def grade(self, rid, actual):
        result = c.evaluate(self.ev)
        self.assertEqual(result['grades'].get(rid), actual, result)
        return result

    def prepare_code(self, row, native, fixups):
        start = int(row['rva'], 16)
        self.assertEqual(row['size'], len(native))
        self.image.data[start:start + len(native)] = native
        self.image.fixups.update(start + f for f in fixups)
        entries = [SimpleNamespace(rva=f, type=3) for f in self.image.fixups]
        optional = SimpleNamespace(
            DATA_DIRECTORY=[SimpleNamespace(Size=int(self.image.relocated))] * 6)
        self.image.pe = SimpleNamespace(
            read_rva=self.image.read, image_base=self.image.base,
            pe=SimpleNamespace(OPTIONAL_HEADER=optional,
                               DIRECTORY_ENTRY_BASERELOC=[SimpleNamespace(entries=entries)]))

    def collect(self, row, obj, native, fixups=(), refused=None):
        self.prepare_code(row, native, fixups)
        found = c.collect_row(row, obj, self.image, self.decoder)
        if refused:
            self.assertIn(refused, found.get('reason', ''), found)
            return found
        self.assertIn('refs', found, found)
        self.ev.rows[row['id']] = dict(found, accepted=False)
        return found

    def gather(self, rows, objects, owners=()):
        ledger = json.dumps(dict(functions=rows)).encode('utf-8')
        with (patch.object(c.Path, 'read_bytes', return_value=ledger),
              patch.object(c, 'Image', return_value=self.image),
              patch.object(c, 'data_units', return_value=owners),
              patch.object(c, 'compile_unit',
                           side_effect=lambda source, flags: objects[source, tuple(flags)])):
            self.ev = c.gather(jobs=1)

    def test_payload_does_not_propagate_grandchild_failure(self):
        prefix = bytes.fromhex('11223344')
        self.define('_outer', 0x3010, prefix + addr(self.image.base + 0x3020),
                    prefix + addr(0), [pointer('_inner')])
        self.define('_inner', 0x3020, prefix + addr(self.image.base + 0x5000),
                    prefix + addr(0), [pointer('_outside')])
        self.row('user', ref('_outer', 0x3010, use='value'))
        result = self.grade('user', None)
        self.assertEqual(result['problems'][M, '_outside'], 'not inside one data zone')
        self.assertTrue(result['problems'][M, '_outer'].startswith('payload pointer'))

    def test_payload_does_not_propagate_target_overlap(self):
        self.define('_outer', 0x3010, b'ABCD' + addr(self.image.base + 0x3204),
                    b'ABCD' + addr(0), [pointer('_interior')])
        self.row('user', ref('_outer', 0x3010, use='value'))
        self.row('owner', ref('_wide', 0x3200, width=8), start=0x1100)
        result = self.grade('user', None)
        self.assertEqual(result['problems'][M, '_interior'], 'shares storage with another name')

    def test_gather_drops_private_pointer_target_definition(self):
        raw = b'\xb8' + addr(0) + b'\xc3'
        rid = ledger_row('user', '_user', 0x1000, len(raw))
        obj = unit([sym('_user', 1, function=True), sym('_outer', 2),
                    sym('_private', 3, storage=c.STATIC)], raw, [rel(1, '_outer')],
                   [section(b'ABCD' + addr(0), [rel(4, '_private')]), section(b'nope')],
                   {'_outer': 8, '_private': 4})
        self.prepare_code(rid, b'\xb8' + addr(self.image.base + 0x3010) + b'\xc3', [1])
        self.image.data[0x3010:0x3018] = b'ABCD' + addr(self.image.base + 0x3200)
        self.image.fixups.add(0x3014)
        # _private's actual object is initialized to "nope", but native 0x3200 is zero-fill.
        self.gather([rid], {('a.c', ()): obj})
        self.assertIn((M, 'a.c', (), '_private'), self.ev.definitions)
        self.grade('user', None)

    def test_gather_drops_definitions_from_a_refused_profile(self):
        raw = b'\xb8' + addr(0) + b'\xc3'
        user = ledger_row('user', '_user', 0x1000, len(raw))
        refused = ledger_row('owner', '_owner', 0x1100, 1, source='owner.c')
        user_obj = unit([sym('_user', 1, function=True), sym('_g')], raw, [rel(1, '_g')])
        owner_obj = unit([sym('_owner', 1, function=True), sym('_g', 2)], b'\x90',
                         data_sections=[section(b'bad!')], sizes={'_g': 4})
        self.prepare_code(user, b'\xb8' + addr(self.image.base + 0x3200) + b'\xc3', [1])
        self.prepare_code(refused, b'\xc3', [])
        self.gather([user, refused], {('a.c', ()): user_obj, ('owner.c', ()): owner_obj})
        self.assertEqual(self.ev.refused['owner'], 'not instruction-equal')
        self.assertIn((M, '_g'), self.ev.definitions)
        self.grade('user', None)

    def test_unreferenced_accepted_extent_is_not_reserved(self):
        self.ev.anchors[M, '_real'] = (0x3200, 8)
        self.row('a', ref('_interior', 0x3204), start=0x1000)
        self.row('b', ref('_interior', 0x3204), start=0x1100)
        self.grade('a', None)
        self.grade('b', None)

    def test_two_accepted_extents_can_overlap_and_both_grade_a(self):
        self.ev.anchors.update({(M, '_outer'): (0x3200, 8), (M, '_inner'): (0x3204, 4)})
        self.row('a', ref('_outer', 0x3200), start=0x1000)
        self.row('b', ref('_inner', 0x3204), start=0x1100)
        self.grade('a', 'B')
        result = self.grade('b', 'B')
        self.assertEqual(len(result['collisions']), 1)

    def test_conflicting_accepted_anchors_are_forgotten(self):
        entries = [{'symbols': {'_g': {'rva': rva, 'kind': 'bss', 'size': 4}}}
                   for rva in (0x3200, 0x3210)]
        rows = [
            dict(id=str(i), target=M, status='FUNCTION_MATCH_RELOCATED',
                 proof=dict(bindings_path='binding.json',
                            bindings_sha256=hashlib.sha256(json.dumps(
                                entry, sort_keys=True, separators=(',', ':')).encode()).hexdigest()))
            for i, entry in enumerate(entries)
        ]
        with patch.object(c.Path, 'read_bytes', return_value=json.dumps(
                         dict(functions=dict(zip(('0', '1'), entries)))).encode()):
            self.ev.anchors, self.ev.anchor_conflicts, _ = c.accepted_anchors(rows, lambda row, name: (M, name))
        self.assertEqual(self.ev.anchors, {})
        self.assertEqual(self.ev.anchor_conflicts, {(M, '_g')})
        self.row('a', ref('_g', 0x3220), start=0x1000)
        self.row('b', ref('_g', 0x3220), start=0x1100)
        self.grade('a', None)

    def test_function_pointer_and_branch_addends_bypass_entry_check(self):
        self.ev.functions[M, '_f'] = [(0x1100, 'target', True)]
        for rid, start, opcode, kind in [('call', 0x1000, b'\xe8', c.REL32),
                                          ('address', 0x1020, b'\xb8', c.DIR32)]:
            raw = opcode + addr(1) + b'\xc3'
            row = ledger_row(rid, '_caller', start, len(raw))
            obj = unit([sym('_caller', 1, function=True), sym('_f', function=True)],
                       raw, [rel(1, '_f', kind)])
            field = 0x1101 - (start + 5) if kind == c.REL32 else self.image.base + 0x1101
            self.collect(row, obj, opcode + addr(field) + b'\xc3', [1] if kind == c.DIR32 else [],
                         refused='with an addend')

    def test_branch_to_data_symbol_never_checks_a_callee(self):
        start = 0x1000
        raw = b'\xe8' + addr(0) + b'\xc3'
        row = ledger_row('user', '_caller', start, len(raw))
        obj = unit([sym('_caller', 1, function=True), sym('_data')], raw,
                   [rel(1, '_data', c.REL32)])
        self.collect(row, obj, b'\xe8' + addr(0x3200 - start - 5) + b'\xc3', refused='non-function symbol')

    def test_payload_function_pointer_addend_is_ignored(self):
        self.ev.functions[M, '_f'] = [(0x1100, 'target', True)]
        self.define('_outer', 0x3010, b'ABCD' + addr(self.image.base + 0x1101),
                    b'ABCD' + addr(1), [pointer('_f', function=True)])
        self.row('user', ref('_outer', 0x3010))
        self.grade('user', None)

    def test_import_addend_accepts_call_through_adjacent_slot(self):
        self.image.imports[0x3084] = 'Beep'
        name = '__imp__Sleep@4'
        raw = b'\xff\x15' + addr(4) + b'\xc3'
        row = ledger_row('user', '_caller', 0x1000, len(raw))
        obj = unit([sym('_caller', 1, function=True), sym(name)], raw, [rel(2, name)])
        self.collect(row, obj, b'\xff\x15' + addr(self.image.base + 0x3084) + b'\xc3', [2],
                     refused='with an addend')

    def test_static_function_reference_uses_another_units_same_named_row(self):
        # a.c defines a private _f, but only b.c's distinct private _f is recorded at 0x1100.
        target = ledger_row('wrong-target', '_f', 0x1100, 1, source='b.c')
        caller = ledger_row('user', '_caller', 0x1000, 6)
        generated = b'\xb8' + addr(0) + b'\xc3'
        obj_a = unit([sym('_caller', 1, function=True), sym('_f', 2, storage=c.STATIC, function=True)],
                     generated, [rel(1, '_f')], [section(b'\x90\xc3', flags=0x20001020, name='.text')])
        obj_b = unit([sym('_f', 1, storage=c.STATIC, function=True)], b'\xc3')
        self.prepare_code(target, b'\xc3', [])
        self.prepare_code(caller, b'\xb8' + addr(self.image.base + 0x1100) + b'\xc3', [1])
        self.gather([target, caller], {('a.c', ()): obj_a, ('b.c', ()): obj_b})
        self.assertEqual(self.ev.rows['user']['refs'][0]['key'], (M, 'a.c', (), '_f'))
        self.grade('user', None)

    def test_data_anchor_can_prove_a_function_at_a_data_rva(self):
        self.ev.anchors[M, '_f'] = (0x3200, 4)  # nonzero extent from a data/bss binding
        self.row('user', ref('_f', 0x3200, use='value', function=True))
        self.grade('user', None)

    def test_fstp_store_is_counted_as_one_byte(self):
        raw = b'\xdd\x1d' + addr(0) + b'\xc3'
        row = ledger_row('store', '_store', 0x1000, len(raw))
        obj = unit([sym('_store', 1, function=True), sym('_double')], raw, [rel(2, '_double')])
        found = self.collect(row, obj, b'\xdd\x1d' + addr(self.image.base + 0x3200) + b'\xc3', [2])
        self.assertEqual(found['refs'][0]['width'], 8)
        self.row('neighbor', ref('_neighbor', 0x3204), start=0x1100)
        self.grade('store', None)
        self.grade('neighbor', None)

    def test_accepted_row_with_bad_reference_endorses_a(self):
        self.ev.anchors[M, '_h'] = (0x3300, 4)
        self.row('accepted', ref('_g', 0x3200), ref('_h', 0x3310),
                 accepted=True, start=0x1000)
        self.row('user', ref('_g', 0x3200), start=0x1100)
        result = self.grade('user', 'B')
        self.assertEqual(result['accepted_disagreement'], ['accepted'])

    def test_one_native_body_under_two_profiles_votes_twice(self):
        # In a stripped image one profile emits a relocation for PUSH, one a literal.
        # Same exact native bytes and extent. Only the masked hashes differ.
        self.image.relocated = False
        start = 0x1000
        native = b'\x68' + addr(self.image.base + 0x3300) + b'\xa1' + addr(self.image.base + 0x3200) + b'\xc3'
        first = ledger_row('a', '_twin', start, len(native), flags=('/O1',))
        second = ledger_row('b', '_twin', start, len(native), flags=('/O2',))
        symbols = [sym('_twin', 1, function=True), sym('_g'), sym('_h')]
        a = unit(symbols, native[:6] + addr(0) + native[10:], [rel(6, '_g')])
        b = unit(symbols, native[:1] + addr(0) + native[5:6] + addr(0) + native[10:],
                 [rel(1, '_h'), rel(6, '_g')])
        self.ev.anchors[M, '_h'] = (0x3300, 4)
        self.assertNotEqual(self.collect(first, a, native)['body'],
                            self.collect(second, b, native)['body'])
        self.grade('a', 'B')
        self.grade('b', 'B')

    def test_own_section_self_calls_escape_symbol_map(self):
        raw = b'\xe8' + addr(0) + b'\xc3'
        obj = unit([sym('_f', 1, function=True)], raw, [rel(1, '_f', c.REL32)])
        native = b'\xe8' + struct.pack('<i', -5) + b'\xc3'
        a = ledger_row('a', '_f', 0x1000, len(raw))
        b = ledger_row('b', '_f', 0x1100, len(raw))
        self.prepare_code(a, native, [])
        self.prepare_code(b, native, [])
        self.gather([a, b], {('a.c', ()): obj})
        self.assertEqual([r['base'] for r in self.ev.rows['a']['refs']], [0x1000])   # own entry recorded
        self.assertEqual([r['base'] for r in self.ev.rows['b']['refs']], [0x1100])
        self.grade('a', None)
        self.grade('b', None)

    def test_own_section_addend_can_leave_the_contribution(self):
        raw = b'\xe8' + addr(0x100) + b'\xc3'
        obj = unit([sym('_f', 1, function=True)], raw, [rel(1, '_f', c.REL32)])
        native = b'\xe8' + addr(0x100 - 5) + b'\xc3'
        row = ledger_row('a', '_f', 0x1000, len(raw))
        self.collect(row, obj, native, refused='leaves the contribution')

    def test_pointer_addends_alone_are_counted_as_initialized_content(self):
        self.ev.definitions[M, '_g'] = [dict(size=0x220, payload=None, refs=[])]
        data_rva = 0x3200
        own = addr(0x0102) + addr(0x0203)
        native = addr(self.image.base + data_rva + 0x0102) + addr(self.image.base + data_rva + 0x0203)
        self.define('_pointers', 0x3010, native, own, [pointer('_g', 0), pointer('_g', 4)])
        self.row('user', ref('_pointers', 0x3010, use='value'))
        self.grade('user', 'B')

    def test_listing_underparse_checks_only_prefix_of_definition(self):
        listing = '_DATA SEGMENT\n_g DD 2 DUP(00H)\n_DATA ENDS'
        sizes = c.listing_sizes(listing)
        self.assertEqual(sizes['_g'], 8)
        obj = unit([sym('_f', 1, function=True), sym('_g', 2)], b'\xc3',
                   data_sections=[section(bytes(8))], sizes=sizes)
        definition = c.define(M, 'a.c', (), obj, obj.unique('_g'))
        self.assertEqual(definition['payload'], bytes(8))
        self.image.data[0x3014] = 9
        self.ev.definitions[M, '_g'].append(definition)
        self.row('user', ref('_g', 0x3010, use='value'))
        self.grade('user', None)

    def test_function_classification_hides_same_key_data_definition(self):
        self.ev.functions[M, '_same'] = [(0x1100, 'callee', True)]
        self.ev.definitions[M, '_same'].append(dict(size=4, payload=b'data', refs=[]))
        self.row('function-use', ref('_same', 0x1100, use='branch', function=True))
        self.row('data-use', ref('_same', 0x1100), start=0x1200)
        self.grade('data-use', None)
        self.ev.rows = dict(reversed(list(self.ev.rows.items())))
        self.grade('data-use', None)

    def test_unreferenced_iat_can_be_claimed_by_ordinary_data(self):
        self.ev.definitions[M, '_ordinary'].append(dict(size=4, payload=bytes(4), refs=[]))
        self.row('user', ref('_ordinary', 0x3080))
        self.grade('user', None)  # 0x3080 is already the Sleep IAT slot.

    def test_zero_definition_accepts_fixups_that_touch_it(self):
        for base, fixup in [(0x3200, 0x3200), (0x3020, 0x301f)]:
            with self.subTest(base=hex(base), fixup=hex(fixup)):
                self.image.fixups = {fixup}
                self.ev.definitions[M, '_g'] = [dict(size=4, payload=None, refs=[])]
                self.row('user', ref('_g', base))
                self.grade('user', None)

    def test_unproven_pointer_leaf_is_not_part_of_grade_a(self):
        self.define('_outer', 0x3010, b'ABCD' + addr(self.image.base + 0x3200),
                    b'ABCD' + addr(0), [pointer('_one_witness_or_none')])
        self.row('user', ref('_outer', 0x3010))
        self.grade('user', 'B')

    def test_one_module_can_grade_despite_an_inconsistent_map(self):
        # Documented scope (docs/consistency_tier.md): the map covers the surviving rows; conflicts are listed.
        self.ev.anchors[M, '_good'] = (0x3400, 4)
        self.row('a', ref('_g', 0x3200), start=0x1000)
        self.row('b', ref('_g', 0x3210), start=0x1100)
        self.row('good', ref('_good', 0x3400), start=0x1200)
        result = self.grade('good', 'A')
        self.assertIn('X.DLL | _g | several addresses', result['conflicts'])

    def test_exe_literal_and_address_are_observationally_identical(self):
        # Documented limit (G3): without base relocations the absolute-field inventory is the candidate's.
        self.image.relocated = False
        for rid, start, op, tail in [('a', 0x1000, b'\xb8', b'\xc3'),
                                     ('b', 0x1100, b'\x68', b'\x58\xc3')]:
            raw = op + addr(0) + tail
            obj = unit([sym('_' + rid, 1, function=True), sym('_g')], raw, [rel(1, '_g')])
            row = ledger_row(rid, '_' + rid, start, len(raw))
            # The native files could be compiled from integer literals.
            self.collect(row, obj, op + addr(self.image.base + 0x3200) + tail)
        self.grade('a', 'A')
        self.grade('b', 'A')


if __name__ == '__main__':
    unittest.main()

