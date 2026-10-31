"""file-data-owner-v1 controls over synthetic, unexecuted PE/COFF files."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest

# Works from tests/ (proposed canonical path) and from the lane directory.
ROOT = next(p for p in Path(__file__).resolve().parents if (p / 'scripts' / 'masked.py').is_file())
sys.path.insert(0, str(ROOT / 'scripts'))
import fixed_disp32
import file_data_owner as fdo
import masked
from test_matching import make_pe, workspace_temporary_directory

BASE = 0x400000
DATA_RVA, ANCHOR_RVA, CALLER_RVA, WINDOW_RVA = 0x2100, 0x1100, 0x1000, 0x1180
PAYLOAD = bytes(range(1, 9))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def coff(path, sections, symbols):
    """sections: [(name, data, flags, [(offset, symbol_index, type)])]; symbols: [(name, value, section, type, storage)]."""
    offset = 20 + 40 * len(sections)
    headers, body = bytearray(), bytearray()
    for name, data, flags, uses in sections:
        rels = b''.join(struct.pack('<IIH', off, sym, typ) for off, sym, typ in uses)
        headers += struct.pack('<8sIIIIIIHHI', name, 0, 0, len(data), offset if data else 0,
                               offset + len(data) if rels else 0, 0, len(uses), 0, flags)
        body += data + rels
        offset += len(data) + len(rels)
    entries = b''.join(struct.pack('<8sIhHBB', name.encode(), value, section, typ, storage, 0)
                       for name, value, section, typ, storage in symbols)
    path.write_bytes(struct.pack('<HHIIIHH', 0x14c, len(sections), 0, offset, len(symbols), 0, 0) +
                     headers + body + entries + struct.pack('<I', 4))


def owner_file(path, payload=PAYLOAD, *, flags=0xc0400040, relocations=(), symbols=None, code=None,
               drectve=b'-defaultlib:LIBC -defaultlib:OLDNAMES '):
    sections = [(b'.drectve', drectve, 0x100a00, [])] if drectve is not None else []
    sections.append((b'.data', payload, flags, list(relocations)))
    if code is not None:
        sections.append((b'.text', code, 0x60500020, []))
    number = len(sections) - (1 if code is not None else 0)
    coff(path, sections, symbols or [('_table', 0, number, 0, 2)])


def caller_file(path, value=0, storage=2, code=None, offset=1):
    code = bytes.fromhex('a100000000c3') if code is None else code
    coff(path, [(b'.text', code, 0x60501020, [(offset, 1, masked.DIR32)])],
         [('_sample', 0, 1, 0x20, 2), ('_table', value, 0, 0, storage)])


def image(path, *, data=PAYLOAD, anchor=None, fixups=None, window=None, relocations=True, directory=None,
          caller=None):
    make_pe(path, base_relocation=True)
    raw = bytearray(path.read_bytes())
    caller = b'\xa1' + struct.pack('<I', BASE + DATA_RVA) + b'\xc3' if caller is None else caller
    raw[0x400:0x400 + len(caller)] = caller
    anchor = b'\x68' + struct.pack('<I', BASE + DATA_RVA) if anchor is None else anchor
    raw[0x500:0x500 + len(anchor)] = anchor
    if window is not None:
        raw[0x580:0x580 + len(window)] = window
    raw[0x700:0x700 + len(data)] = data
    cells = sorted([CALLER_RVA + 1, ANCHOR_RVA + 1] if fixups is None else fixups)
    blocks = bytearray()
    for page in sorted({c & ~0xfff for c in cells}):
        entries = [0x3000 | (c - page) for c in cells if c & ~0xfff == page]
        if len(entries) % 2:
            entries.append(0)
        blocks += struct.pack('<II', page, 8 + 2 * len(entries)) + b''.join(struct.pack('<H', e) for e in entries)
    raw[0x800:0x800 + len(blocks)] = blocks
    optional = 0x98
    struct.pack_into('<II', raw, optional + 96 + 5 * 8, 0x3000 if relocations else 0, len(blocks) if relocations else 0)
    if directory is not None:
        struct.pack_into('<II', raw, optional + 96 + directory[0] * 8, directory[1], directory[2])
    path.write_bytes(raw)
    return anchor


def record(anchor=b'\x68' + struct.pack('<I', BASE + DATA_RVA), size=8, data=PAYLOAD, operand=0, **extra):
    rec = dict(kind='data', data_profile=fdo.PROFILE, rva=DATA_RVA, size=size, sha256=sha(data[:size]),
               owner=dict(source='src/eng1/table_owner.c', flags=['/O2'], symbol='_table'),
               file_data=dict(extent_claim='minimum',
                              anchor=dict(rva=ANCHOR_RVA, size=len(anchor), operand=operand, sha256=sha(anchor))),
               evidence=['synthetic independent review'], references=[dict(offset=1, type=6, addend=0)])
    rec.update(extra)
    return rec


class FileDataOwnerTests(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('file-data-owner-')
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.target, self.caller, self.owner = [self.root / n for n in ('original.dll', 'caller.obj', 'owner.obj')]
        image(self.target)
        caller_file(self.caller)
        owner_file(self.owner)
        self.record = record()

    def compare(self, comparator=None, rec=None, owner='default'):
        comparator = comparator or fixed_disp32.relocated_compare
        owners = {'_table': self.owner if owner == 'default' else owner}
        return comparator(self.caller, '_sample', self.target, CALLER_RVA, 6,
                          dict(symbols={'_table': rec or self.record}), storage_owners=owners)

    # positive paths
    def test_minimum_extent_positive(self):
        result = self.compare()
        self.assertTrue(result['relocated_equal'], result['errors'])
        check = result['data_checks'][0]
        self.assertEqual((check['profile'], check['extent_claim'], check['anchor']['form']),
                         (fdo.PROFILE, 'minimum', 'immediate'))

    def test_direct_scalar_anchors(self):
        for raw, operand in ((b'\xa1' + struct.pack('<I', BASE + DATA_RVA), 1),              # mov eax, [X]
                             (b'\x0f\xbf\x05' + struct.pack('<I', BASE + DATA_RVA), 1),      # movsx eax, word [X]
                             (b'\x8d\x05' + struct.pack('<I', BASE + DATA_RVA), 1),          # lea eax, [X]
                             (b'\xc7\x40\x64' + struct.pack('<I', BASE + DATA_RVA), 1)):     # mov [eax+64], X
            field = ANCHOR_RVA + len(raw) - 4
            image(self.target, anchor=raw, fixups=[CALLER_RVA + 1, field])
            self.assertTrue(self.compare(rec=record(anchor=raw, operand=operand))['relocated_equal'], raw.hex())

    # legacy comparators refuse the profile
    def test_legacy_comparators_refuse(self):
        # Pinned masked.py and local_byte_tables.py never dispatch the profile. (The unpatched pre-integration
        # fixed_disp32.py comparator refuses it identically; see the lane receipts.)
        import local_byte_tables
        for comparator in (masked.relocated_compare, local_byte_tables.relocated_compare):
            with self.assertRaisesRegex(ValueError, 'Unsupported explicit data profile'):
                self.compare(comparator)

    # anchors
    def test_wrong_anchor_address(self):
        raw = b'\x68' + struct.pack('<I', BASE + DATA_RVA + 4)
        image(self.target, anchor=raw)
        with self.assertRaisesRegex(ValueError, 'identifies another address'):
            self.compare(rec=record(anchor=raw))

    def test_anchor_hash_and_shape(self):
        rec = copy.deepcopy(self.record)
        rec['file_data']['anchor']['sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'hash changed'):
            self.compare(rec=rec)
        rec = copy.deepcopy(self.record)
        rec['file_data']['anchor']['size'] = 6
        with self.assertRaises(ValueError):
            self.compare(rec=rec)

    def test_anchor_inside_compared_caller(self):
        rec = copy.deepcopy(self.record)
        rec['file_data']['anchor'].update(rva=CALLER_RVA, size=5, operand=1,
                                          sha256=sha(b'\xa1' + struct.pack('<I', BASE + DATA_RVA)))
        with self.assertRaisesRegex(ValueError, 'independent of the compared caller'):
            self.compare(rec=rec)

    def test_host_promotion_does_not_invalidate_anchor(self):
        row = dict(id='eng1.caller', target='t.dll', rva=hex(CALLER_RVA), size=6, status='CODEGEN_SIMILAR')
        host = dict(id='eng1.host', target='t.dll', rva=hex(ANCHOR_RVA), size=16)
        for status in ('CODEGEN_SIMILAR', 'RECONSTRUCTED', 'FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED'):
            with self.subTest(status=status):
                fdo.review_inputs(self.record, '_table', row, [row, dict(host, status=status)], set())
                with self.assertRaisesRegex(ValueError, 'compared or selected caller: eng1.host'):
                    fdo.review_inputs(self.record, '_table', row, [row, dict(host, status=status)], {'eng1.host'})
        fdo.review_inputs(self.record, '_table', row, [row, dict(host, target='other.dll')], {'eng1.host'})

    def test_anchor_host_binding_names(self):
        check = self.compare()['data_checks'][0]
        self.assertEqual(check['anchor']['field'], ANCHOR_RVA + 1)
        row = dict(id='eng1.caller', target='t.dll', rva=hex(CALLER_RVA), size=6)
        host = dict(id='eng1.host', target='t.dll', rva=hex(ANCHOR_RVA), size=16)

        def bound(name, rva=DATA_RVA, size=8, **use):
            use = dict(dict(offset=1, type=6, addend=0), **use)
            return {'eng1.host': ({'symbols': {name: dict(kind='data', rva=rva, size=size, references=[use])}},
                                  {}, 'evidence/host.json')}
        ledger = [row, host]
        fdo.review_host_names([check], row, ledger, {})                      # no host binding in the run
        fdo.review_host_names([check], row, ledger, bound('_table'))         # same name, address and field
        fdo.review_host_names([check], row, [row, dict(host, target='other.dll')], bound('_alias'))
        fdo.review_host_names([check], row, [row, dict(host, rva=hex(ANCHOR_RVA + 8))], bound('_alias'))
        for bindings, message in ((bound('_alias'), 'names the cell differently'),
                                  (bound('_big', rva=DATA_RVA - 4, size=16, offset=9), 'names the cell differently'),
                                  (bound('_prev', rva=DATA_RVA - 8, addend=8, one_past=True), 'binds the anchor field'),
                                  (bound('_table', addend=4), 'binds the anchor field'),
                                  (bound('_table', rva=DATA_RVA + 16, offset=9), 'binds this symbol elsewhere')):
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                fdo.review_host_names([check], row, ledger, bindings)

    def test_review_rejects_compared_caller_even_if_not_selected(self):
        row = dict(id='eng1.caller', target='t.dll', rva=hex(ANCHOR_RVA), size=16)
        with self.assertRaisesRegex(ValueError, 'compared or selected caller: eng1.caller'):
            fdo.review_inputs(self.record, '_table', row, [row], set())

    def test_anchor_needs_one_highlow(self):
        image(self.target, fixups=[CALLER_RVA + 1])
        with self.assertRaisesRegex(ValueError, 'exactly one original HIGHLOW'):
            self.compare()

    def test_indexed_and_immediate_compare_anchors_reject(self):
        for raw, operand in ((b'\x8b\x04\x8d' + struct.pack('<I', BASE + DATA_RVA), 1),     # mov eax, [ecx*4+X]
                             (b'\x3d' + struct.pack('<I', BASE + DATA_RVA), 1),             # cmp eax, X (immediate)
                             (b'\x8b\x80' + struct.pack('<I', BASE + DATA_RVA), 1)):        # mov eax, [eax+X]
            image(self.target, anchor=raw, fixups=[CALLER_RVA + 1, ANCHOR_RVA + len(raw) - 4])
            with self.assertRaisesRegex(ValueError, 'absolute imm32 or direct disp32', msg=raw.hex()):
                self.compare(rec=record(anchor=raw, operand=operand))

    def test_stripped_or_directory_overlap_rejects(self):
        image(self.target, relocations=False)
        with self.assertRaisesRegex(ValueError, 'requires original base relocations'):
            self.compare()
        image(self.target, directory=(12, DATA_RVA, 8))
        with self.assertRaisesRegex(ValueError, 'overlaps a PE data directory'):
            self.compare()

    # payload and owner
    def test_payload_mismatch_and_native_pin(self):
        owner_file(self.owner, PAYLOAD[:-1] + b'\xff')
        result = self.compare()
        self.assertFalse(result['relocated_equal'])
        self.assertIn('Compiled owner payload differs: _table', result['errors'])
        owner_file(self.owner)
        result = self.compare(rec=dict(self.record, sha256='0' * 64))
        self.assertIn('Data payload identity differs: _table', result['errors'])

    def test_unaccounted_native_fixup_in_payload(self):
        image(self.target, fixups=[CALLER_RVA + 1, ANCHOR_RVA + 1, DATA_RVA + 4])
        with self.assertRaisesRegex(ValueError, 'unbound original loader fixup'):
            self.compare()

    def test_unbound_payload_relocation(self):
        owner_file(self.owner, relocations=[(4, 1, 6)],
                   symbols=[('_table', 0, 2, 0, 2), ('_callee', 0, 0, 0x20, 2)])
        with self.assertRaisesRegex(ValueError, 'explicit payload binding inventory'):
            self.compare()

    def test_bound_payload_relocation(self):
        pointer = PAYLOAD[:4] + struct.pack('<I', BASE + ANCHOR_RVA) + b''
        image(self.target, data=pointer, fixups=[CALLER_RVA + 1, ANCHOR_RVA + 1, DATA_RVA + 4])
        owner_file(self.owner, PAYLOAD[:4] + b'\0\0\0\0', relocations=[(4, 1, 6)],
                   symbols=[('_table', 0, 2, 0, 2), ('_callee', 0, 0, 0x20, 2)])
        child = dict(kind='function', rva=ANCHOR_RVA, size=5, evidence=['synthetic callee identity'],
                     references=[dict(offset=4, type=6, addend=0)])
        rec = dict(record(data=pointer), payload_bindings=dict(symbols={'_callee': child}))
        result = self.compare(rec=rec)
        self.assertTrue(result['relocated_equal'], result['errors'])
        self.assertEqual(result['data_checks'][0]['owner_relocations'], 1)
        child_data = dict(child, kind='data', sha256=sha(b'\x68\0\x21\x40\0'))
        with self.assertRaisesRegex(ValueError, 'children must be defined by the owner'):
            self.compare(rec=dict(rec, payload_bindings=dict(symbols={'_callee': child_data})))

    def test_payload_bindings_without_relocations_reject(self):
        with self.assertRaisesRegex(ValueError, 'require a relocation-bearing owner payload'):
            self.compare(rec=dict(self.record, payload_bindings=dict(symbols={})))

    def test_owner_allocation_extent(self):
        with self.assertRaisesRegex(ValueError, 'allocation differs from reviewed extent'):
            self.compare(rec=record(size=4))
        # A following object can hide inter-object alignment padding inside the allocation
        # (VC5: short[3] then long -> allocation 8 for a 6-byte object), so it is refused.
        owner_file(self.owner, PAYLOAD + b'\0' * 8,
                   symbols=[('_table', 0, 2, 0, 2), ('_next', 8, 2, 0, 2)])
        with self.assertRaisesRegex(ValueError, 'must end its section'):
            self.compare()
        # The last object of its section is exact: VC5 emits no trailing section padding.
        owner_file(self.owner, b'\5\0\0\0' + PAYLOAD,
                   symbols=[('_lead', 0, 2, 0, 2), ('_table', 4, 2, 0, 2)])
        self.assertTrue(self.compare()['relocated_equal'])

    def test_owner_shape_rejects(self):
        cases = [(dict(code=b'\xc3'), 'executable code'), (dict(flags=0xc0400080), 'initialized non-COMDAT'),
                 (dict(flags=0xc0401040), 'initialized non-COMDAT'),
                 (dict(symbols=[('_table', 0, 2, 0x20, 2)]), 'strong defined data'),
                 (dict(symbols=[('_table', 0, 2, 0, 3)]), 'strong defined data'),
                 (dict(drectve=b'/include:_evil '), 'linker directives'),
                 (dict(symbols=[('_table', 0, 2, 0, 2), ('_alias', 0, 0, 0, 105)]), 'weak aliases')]
        for case, message in cases:
            owner_file(self.owner, **case)
            with self.assertRaisesRegex(ValueError, message, msg=str(case)):
                self.compare()
        with self.assertRaisesRegex(ValueError, 'not freshly compiled'):
            self.compare(owner=None)

    def test_direct_caller_access_width_bound(self):
        for addend, ok in ((4, True), (6, False)):
            caller_file(self.caller, code=b'\xa1' + struct.pack('<I', addend) + b'\xc3')
            image(self.target, caller=b'\xa1' + struct.pack('<I', BASE + DATA_RVA + addend) + b'\xc3')
            rec = record(references=[dict(offset=1, type=6, addend=addend)])
            if ok:
                self.assertEqual(self.compare(rec=rec)['data_checks'][0]['direct_uses'],
                                 [dict(offset=1, addend=4, width=4)])
            else:
                with self.assertRaisesRegex(ValueError, 'direct access exceeds the claimed extent'):
                    self.compare(rec=rec)
        # An indexed use bounds only its displacement; no runtime index range is claimed.
        caller_file(self.caller, code=bytes.fromhex('8b048d00000000c3'), offset=3)
        image(self.target, caller=b'\x8b\x04\x8d' + struct.pack('<I', BASE + DATA_RVA) + b'\xc3',
              fixups=[CALLER_RVA + 3, ANCHOR_RVA + 1])
        result = fixed_disp32.relocated_compare(self.caller, '_sample', self.target, CALLER_RVA, 8,
                                                dict(symbols={'_table': record(references=[dict(offset=3, type=6, addend=0)])}),
                                                storage_owners={'_table': self.owner})
        self.assertTrue(result['relocated_equal'], result['errors'])
        self.assertEqual(result['data_checks'][0]['direct_uses'], [])

    def test_caller_symbol_must_be_undefined_external(self):
        caller_file(self.caller, value=8)
        with self.assertRaisesRegex(ValueError, 'non-COMMON'):
            self.compare()

    # schema and extent claims
    def test_schema_rejects(self):
        mutations = [(lambda r: r.update(anchor_profile='cmp-test-absolute-v1'), 'anchor profile'),
                     (lambda r: r.update(access_profile='loader-zero-indexed-v1'), 'loader-zero anchor'),
                     (lambda r: r.update(identity=r['file_data']['anchor']), 'unsupported fields'),
                     (lambda r: r['owner'].update(symbol='_other'), 'aliases are unsupported'),
                     (lambda r: r['file_data'].update(extent_claim='maximum'), 'minimum extent claim only'),
                     (lambda r: r['file_data'].pop('extent_claim'), 'minimum extent claim only'),
                     (lambda r: r['file_data'].update(extent_claim='complete'), 'minimum extent claim only'),
                     (lambda r: r['file_data'].update(capacity=64), 'survey has missing or unsupported'),
                     (lambda r: r.update(sha256='not-a-digest'), 'pinned native payload sha256'),
                     (lambda r: r['references'][0].update(one_past=True), 'asserts an end boundary')]
        for mutate, message in mutations:
            rec = copy.deepcopy(self.record)
            mutate(rec)
            with self.assertRaisesRegex(ValueError, message):
                self.compare(rec=rec)

    def test_owner_profile(self):
        root = self.root
        (root / 'src').mkdir()
        self.assertEqual(fdo.owner_profile(self.record, '_table', root)[1], ('/O2',))
        for owner in (dict(source='work/table_owner.c'), dict(source='src/table_owner.cpp'),
                      dict(flags=['/O2', '/Tp'])):
            rec = copy.deepcopy(self.record)
            rec['owner'].update(owner)
            with self.assertRaises(ValueError):
                fdo.owner_profile(rec, '_table', root)

    def test_complete_and_end_bound_are_unsupported(self):
        rec = copy.deepcopy(self.record)
        rec['file_data'].update(extent_claim='complete', end_bound=dict(
            profile='one-past-walk-v1', window=dict(rva=WINDOW_RVA, size=5, sha256='0' * 64),
            start=WINDOW_RVA, bound=WINDOW_RVA + 4))
        with self.assertRaisesRegex(ValueError, 'minimum extent claim only'):
            self.compare(rec=rec)
        rec['file_data']['extent_claim'] = 'minimum'
        with self.assertRaisesRegex(ValueError, 'survey has missing or unsupported fields'):
            self.compare(rec=rec)


if __name__ == '__main__':
    unittest.main()
