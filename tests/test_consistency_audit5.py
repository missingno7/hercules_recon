"""Regressions from the independent audit of d89dc97."""
import hashlib
import json
import unittest
from pathlib import Path
from struct import pack
from unittest.mock import patch

import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_consistency_cv4 as cv
import test_consistency_review as fixtures
import test_consistency_round5 as round5
from test_consistency import M, c, ref
from test_consistency_review import addr, ledger_row, rel, sym, unit


class Audit5(unittest.TestCase):
    setUp = fixtures.Reproductions.setUp
    row = fixtures.Reproductions.row
    grade = fixtures.Reproductions.grade
    collect = fixtures.Reproductions.collect
    prepare_code = fixtures.Reproductions.prepare_code
    gather = fixtures.Reproductions.gather
    compile_fixture = round5.Round5.compile_fixture

    def anchored_records(self, *symbols, unresolved=()):
        rows, bindings = [], {}
        for i, records in enumerate(symbols):
            rid = f'accepted-{i}'
            binding = dict(symbols=records)
            bindings[rid] = binding
            canonical = json.dumps(binding, sort_keys=True, separators=(',', ':')).encode()
            row = ledger_row(rid, '_f', 0x1000, 1)
            rows.append(dict(row, status='FUNCTION_MATCH_RELOCATED',
                             proof=dict(bindings_path='pins.json',
                                        bindings_sha256=hashlib.sha256(canonical).hexdigest())))
        with patch.object(c.Path, 'read_bytes', return_value=json.dumps(dict(functions=bindings)).encode()):
            self.ev.anchors, self.ev.anchor_conflicts, _, self.ev.opaque_pins = c.accepted_anchors(
                rows, lambda row, name: None if name in unresolved else (M, name),
                lambda module, owner: (module, owner['symbol']))

    def definition_grade(self, compiled, name, native, expected):
        key, base = (M, name), 0x3010
        self.ev.definitions.update(c.unit_definitions(M, 'a.c', (), compiled))
        self.image.data[base:base + len(native)] = native
        self.row('user', ref(name, base, use='value'))
        return self.ev.definitions[key][0], self.grade('user', expected)

    def test_switch_case_segment_override_cannot_supply_a_witness(self):
        self.image.base = 0x6a000000
        start, g = 0x105f, 0x3200
        jump, case = b'\xff\x24\x85', b'\x64\xa1'  # indexed JMP, then MOV EAX, FS:[disp32]
        raw = jump + addr(0) + b'\xc3' + addr(0) + case + addr(0) + b'\xc3'
        native = (jump + addr(self.image.base + start + 8) + b'\xc3'
                  + addr(self.image.base + start + 12) + case + addr(self.image.base + g) + b'\xc3')
        obj = unit([sym('_switch', 1, function=True), sym('$Table', 1, 8, storage=c.LABEL),
                    sym('$Case', 1, 12, storage=c.LABEL), sym('_g')],
                   raw, [rel(3, '$Table'), rel(8, '$Case'), rel(14, '_g')])
        cells = c.switch_cells(self.decoder, native, start, {3: 8, 8: 12})
        self.assertEqual(cells, {8})
        self.assertIsNone(c.accesses(self.decoder, native, start, 14, cells))
        switch = dict(ledger_row('switch', '_switch', start, len(raw)), status='FUNCTION_MATCH')
        self.prepare_code(switch, native, [3, 8, 14])
        plain = b'\xa1' + addr(self.image.base + g) + b'\xc3'
        caller = ledger_row('plain', '_plain', 0x1100, len(plain), source='b.c')
        self.prepare_code(caller, plain, [1])
        plain_unit = unit([sym('_plain', 1, function=True), sym('_g')],
                          b'\xa1' + addr(0) + b'\xc3', [rel(1, '_g')])
        self.gather([switch, caller], {('a.c', ()): obj, ('b.c', ()): plain_unit})
        self.assertEqual(self.ev.refused, {'switch': 'operand form at 0xe'})
        self.grade('plain', 'B')

    def test_recordless_listing_refuses_open_object_at_eof(self):
        ordinary = cv.coff(b'ABCDEFGHold!', sym('_g', 1))
        sidecar = cv.add_debug(cv.coff(bytes(12), sym('_g', 1)))
        listing = "_DATA SEGMENT\n_earlier DB 'abcd'\n_g DB 'ABCDEFGH'\n"
        self.assertEqual(c.listing_sizes(listing, exact=True), {'_earlier': 4, '_g': None})
        compiled = self.compile_fixture(ordinary, sidecar, listing)
        found, _ = self.definition_grade(compiled, '_g', b'ABCDEFGHnew!', None)
        self.assertIsNone(found['refs'])

    def test_codeview_refuses_relocation_overlapping_data_record_location(self):
        ordinary = cv.coff(b'ABCDEFGHold!', sym('_g', 1))
        sidecar = cv.add_debug(cv.coff(bytes(12), sym('_g', 1)), cv.array(0x20, 8))
        cv.bind(sidecar, '_g', 0x1000)
        fields = sidecar.sections[1]['relocations']
        fields.append(rel(fields[0]['offset'] + 1, '_g'))
        listing = "_DATA SEGMENT\n_g DB 'ABCDEFGH'\n    ORG $+4\n_DATA ENDS"
        compiled = self.compile_fixture(ordinary, sidecar, listing)
        found, _ = self.definition_grade(compiled, '_g', b'ABCDEFGHnew!', None)
        self.assertIsNone(found['refs'])

    def test_codeview_refuses_missing_sidecar_symbol_section(self):
        ordinary = cv.coff(b'ABCDEFGHold!', sym('_g', 1))
        sidecar = cv.add_debug(cv.coff(bytes(12), sym('_g', 99)), cv.array(0x20, 8))
        cv.bind(sidecar, '_g', 0x1000)
        compiled = self.compile_fixture(ordinary, sidecar, '')
        found, _ = self.definition_grade(compiled, '_g', b'ABCDEFGHnew!', None)
        self.assertIsNone(found['refs'])

    def test_c11_member_pointer_remains_unsupported(self):
        name = '?sd@@3PQSimple@@HQ1@'
        structure = cv.record(0x1005, pack('<HHIIIH', 0, 0, 0, 0, 0, 4) + cv.name(b'Simple'))
        member = cv.record(0x1002, bytes.fromhex('740000004a000000001000000000f2f1'))
        ordinary = cv.coff(bytes(4), sym(name, 1))
        sidecar = cv.add_debug(cv.coff(bytes(4), sym(name, 1)), structure + member)
        cv.bind(sidecar, name, 0x1001)
        listing = f'_DATA SEGMENT\n{name} DD 0\n_DATA ENDS'
        compiled = self.compile_fixture(ordinary, sidecar, listing)
        found, _ = self.definition_grade(compiled, name, bytes(4), None)
        self.assertIsNone(found['refs'])
        self.assertIn('C++ pointers to members are unsupported', self.doc())

    def test_conflicting_sizes_on_one_key_are_reported_and_reserved(self):
        g = 0x3040
        self.anchored_records({'_g': dict(rva=g, size=4, kind='data')},
                              {'_g': dict(rva=g, size=8, kind='data')})
        conflict = self.ev.anchor_conflicts[M, '_g']
        self.assertEqual([(r['row'], r['rva'], r['size']) for r in conflict['records']],
                         [('accepted-0', g, 4), ('accepted-1', g, 8)])
        self.assertEqual({(base, size) for _, base, size in self.ev.opaque_pins}, {(g, 4), (g, 8)})
        self.assertNotIn((M, '_g'), self.ev.anchors)
        self.row('user', ref('_g', g))
        self.row('accepted-user', ref('_g', g), start=0x1100, accepted=True)
        result = self.grade('user', None)
        self.assertIn('X.DLL | _g | accepted binding sizes disagree (4, 8 bytes)', result['conflicts'])
        self.assertEqual(result['accepted_failing'], ['accepted-user'])
        self.assertEqual(result['accepted_disagreement'], [])

    def test_conflicting_opaque_alias_fails_its_verified_owner(self):
        owner = dict(source='owner.c', flags=[], symbol='_owner')
        def alias(rva):
            return dict(rva=rva, size=4, kind='bss', source_alias={'kind': 'compile-renaming'}, owner=owner)

        self.anchored_records({'_owner': dict(rva=0x3200, size=4, kind='bss')},
                              {'_alias': alias(0x3220)},
                              {'_alias': alias(0x3240)})
        self.assertEqual(set(self.ev.anchor_conflicts), {(M, '_alias')})
        self.assertEqual(self.ev.anchor_conflicts[M, '_alias']['alias_of'], (M, '_owner'))
        self.assertEqual({(rva, size) for _, rva, size in self.ev.opaque_pins},
                         {(0x3220, 4), (0x3240, 4)})
        self.row('owner', ref('_owner', 0x3200))
        self.row('claim', ref('_other', 0x3220), start=0x1100)
        result = self.grade('owner', None)
        self.assertEqual(result['grades'], {})
        self.assertIn('X.DLL | _owner | accepted bindings disagree', result['conflicts'])

    def test_verified_data_alias_preserves_a_recorded_function_kind(self):
        owner = dict(source='owner.c', flags=[], symbol='_owner')
        self.anchored_records({'_alias': dict(rva=0x3200, size=4, kind='bss',
                                             source_alias={'kind': 'compile-renaming'}, owner=owner)})
        self.ev.functions[M, '_alias'] = [(0x1100, 'recorded-function', True)]
        self.row('user', ref('_alias', 0x3200))
        self.assertIn('X.DLL | _owner | kind conflict', self.grade('user', None)['conflicts'])

    def test_data_pins_with_compiler_label_spelling_are_reserved(self):
        self.anchored_records({'$SG1': dict(rva=0x3040, size=8, kind='data'),
                               '.pool': dict(rva=0x3050, size=8, kind='bss')},
                              unresolved={'$SG1', '.pool'})
        self.assertEqual({(base, size) for _, base, size in self.ev.opaque_pins},
                         {(0x3040, 8), (0x3050, 8)})
        for i, (name, offset) in enumerate((('_other', 0x3044), ('_next', 0x3054))):
            self.ev.definitions[M, name] = [dict(size=4, payload=bytes(4), refs=[])]
            self.row('a' + name, ref(name, offset), start=0x1000 + i * 0x200)
            self.row('b' + name, ref(name, offset), start=0x1100 + i * 0x200)
        self.assertEqual(self.grade('a_other', None)['grades'], {})

    def test_ordinal_iat_is_reserved_but_its_import_has_no_binding_path(self):
        name, slot = '__imp__ImportedByOrdinal@0', 0x3080
        self.image.imports = {slot: 'ordinal:17'}
        self.anchored_records({name: dict(rva=slot, size=4, kind='import', ordinal=17, dll='example.dll')})
        raw = b'\xff\x15' + addr(0) + b'\xc3'
        native = b'\xff\x15' + addr(self.image.base + slot) + b'\xc3'
        obj = unit([sym('_f', 1, function=True), sym(name)], raw, [rel(2, name)])
        self.collect(ledger_row('import', '_f', 0x1000, len(raw)), obj, native, [2])
        out = self.grade('import', None)
        self.assertEqual(out['problems'][M, name], 'import slot')
        self.assertFalse(self.ev.anchors)
        self.assertIn('Ordinal IAT slots are reserved. Import resolution is name-based.', self.doc())
        self.ev.definitions[M, '_ordinary'] = [dict(size=4, payload=bytes(4), refs=[])]
        self.row('competing-data', ref('_ordinary', slot), start=0x1100)
        self.assertIn('X.DLL | _ordinary | shares storage with another name',
                      self.grade('competing-data', None)['conflicts'])

    def test_local_rel32_label_uses_documented_exception(self):
        start = 0x1000
        native = b'\xe9' + addr(0) + b'\xc3'
        obj = unit([sym('_f', 1, function=True), sym('$local', 1, 5, storage=c.LABEL)],
                   native, [rel(1, '$local', c.REL32)])
        self.collect(ledger_row('user', '_f', start, len(native)), obj, native)
        self.grade('user', 'A')
        self.assertIn('An own-section REL32 may name a local label', self.doc())

    def test_extra_native_fixup_preserves_a_corroborated_address_contradiction(self):
        start = 0x1000
        fields = [addr(self.image.base + target) for target in (0x3210, 0x3220, 0x3230)]
        native = b''.join(b'\xa1' + field for field in fields) + b'\xc3'
        raw = b'\xa1' + addr(0) + b'\xa1' + addr(0) + b'\xa1' + fields[2] + b'\xc3'
        row = dict(ledger_row('accepted', '_f', start, len(native)), status='FUNCTION_MATCH')
        obj = unit([sym('_f', 1, function=True), sym('_g')], raw, [rel(1, '_g'), rel(6, '_g')])
        self.prepare_code(row, native, [1, 6, 11])
        result = c._compare_section(obj.coff, obj.coff.sections[0], 1, self.image.pe, start, len(native))
        self.assertTrue(any('previous' in failure for failure in result['conflicts']))
        self.gather([row], {('a.c', ()): obj})
        self.assertEqual(self.ev.refused, {'accepted': 'not instruction-equal'})
        out = self.grade('accepted', None)
        self.assertEqual(out['accepted_disagreement'], ['accepted'])
        self.assertEqual(out['conflicts'], ['X.DLL | _g | row accepted | incompatible relocation addresses'])
        self.assertEqual(self.ev.collection_conflicts['accepted'][0]['offset'], 6)

        # With one proposed operand lacking HIGHLOW, only one equation survives.
        self.image.fixups.clear()
        self.prepare_code(row, native, [6, 11])
        self.gather([row], {('a.c', ()): obj})
        self.assertEqual(self.ev.refused, {'accepted': 'not instruction-equal'})
        self.assertEqual(self.grade('accepted', None)['accepted_disagreement'], [])

        # Matching starts alone also do not establish a four-byte HIGHLOW.
        self.image.fixups.clear()
        self.prepare_code(row, native, [1, 6])
        self.image.highlow.remove(start + 1)
        for entry in self.image.pe.pe.DIRECTORY_ENTRY_BASERELOC[0].entries:
            if entry.rva == start + 1:
                entry.type = 1
        self.gather([row], {('a.c', ()): obj})
        self.assertEqual(self.ev.refused, {'accepted': 'not instruction-equal'})
        self.assertEqual(self.grade('accepted', None)['accepted_disagreement'], [])

    def test_missing_native_digest_cannot_supply_a_witness(self):
        self.row('a', ref('_g', 0x3200), start=0x1000)
        self.row('b', ref('_g', 0x3200), start=0x1100)
        del self.ev.rows['a']['native']
        self.assertEqual(self.grade('a', 'B')['grades'], {'a': 'B', 'b': 'B'})

    @staticmethod
    def doc():
        copy = Path(__file__).with_name('consistency_tier.md')
        return (copy if copy.is_file() else c.ROOT / 'docs/consistency_tier.md').read_text(encoding='utf-8')

    def test_measurement_receipt_links_resolve_from_canonical_docs(self):
        root = c.ROOT
        content = self.doc()
        receipts = {
            'the CodeView-only comparison': 'cv4/',
            'the Round 4 comparison': 'cv4/round4/',
            'the strict collector receipt': '',
            'the Round 5 comparison': 'r5/',
        }
        for title, archive in receipts.items():
            path = '../work/velocity_q/fixCON/' + archive + 'COMPARISON.md'
            with self.subTest(title=title):
                self.assertIn(f'[{title}]({path})', content)
                self.assertTrue((root / 'docs' / path).is_file())


if __name__ == '__main__':
    unittest.main()
