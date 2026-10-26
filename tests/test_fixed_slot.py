"""Real VC5 wrapper controls plus adversarial, unexecuted proof mutations."""
import copy
import json
import os
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import fixed_slot
import interface_slots
import masked
import match
from coff import COFF
from test_matching import workspace_temporary_directory


class FixedSlotTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.review = json.loads((match.ROOT / fixed_slot.REVIEW).read_bytes())
        if not all((match.ROOT / p).is_file() for p,_ in interface_slots.IMAGES.values()):
            raise unittest.SkipTest('Pinned original PC files are unavailable')
        os.environ.setdefault('HERCULES_COMPILE_METRICS_DIR', 'work/compiler_events')
        interface_slots.generate_bindings(cls.review)
        cls.rows = {r['id']:r for r in json.loads((match.ROOT / 'recovery.json').read_bytes())['functions']}
        cls.documents = {name:json.loads((match.ROOT / 'work/iface_bindings' / (name + '.json')).read_bytes()) for name in ('eng1', 'eng3', 'title')}
        cls.ids = ['title.0c280', 'eng1.dispatch_engine_2c270', 'eng3.slot_020']
        cls.objects = {}
        for id in cls.ids:
            row = cls.rows[id]; key = (row['source'], tuple(row['flags']))
            if key not in cls.objects:
                cls.objects[key] = match.compile_source(match.ROOT / key[0], list(key[1]))[0]
        # Each module's accepted root record names its own owner source/profile.
        cls.owners = {}
        for name, document in cls.documents.items():
            owner = next(iter(document['functions'].values()))['symbols'][fixed_slot.ROOT_SYMBOL]['owner']
            cls.owners[name] = match.compile_source(match.ROOT / owner['source'], list(owner['flags']))[0]

    def setUp(self):
        self.directory = workspace_temporary_directory('fixed-slot-'); self.addCleanup(self.directory.cleanup)
        self.id = 'title.0c280'; self.binding = self.get_binding(self.id)
        self.obj = None; self.target = None

    def get_binding(self, id):
        return copy.deepcopy(self.documents[id.split('.')[0]]['functions'][id])

    def rejected_binding(self, id):
        self.id = id; row = self.review['wrappers'][id]
        binding = self.get_binding('title.0c280'); proof = binding['indirect_calls'][0]
        proof.update({key:row[key] for key in ('offset','slot','transfer','pushed_dwords','cleanup_bytes')})
        proof['evidence'] = [fixed_slot.REVIEW + '#slots.' + hex(row['slot'])]
        binding['symbols'][fixed_slot.ROOT_SYMBOL]['references'] = [dict(offset=row['offset'] + 2, type=6, addend=row['slot'])]
        self.binding = binding

    def object(self):
        row = self.rows[self.id]
        return self.obj or self.objects[(row['source'], tuple(row['flags']))]

    def compare(self, owners=True):
        row = self.rows[self.id]
        return masked.relocated_compare(self.object(), row['symbol'], self.target or match.ROOT / row['target'],
                int(row['rva'],0), row['size'], self.binding,
                storage_owners={fixed_slot.ROOT_SYMBOL:self.owners[self.id.split('.')[0]]} if owners else {})

    def change_code(self, offset, raw):
        original = self.object(); coff = COFF(original); section = coff.function(self.rows[self.id]['symbol'])
        number = coff.sections.index(section)
        data = bytearray(original.read_bytes()); pointer = struct.unpack_from('<I',data,20 + 40 * number + 20)[0]
        data[pointer + offset:pointer + offset + len(raw)] = raw
        self.obj = Path(self.directory.name) / 'changed.obj'; self.obj.write_bytes(data)

    def test_real_call_tail_and_five_argument_wrappers(self):
        for id in self.ids:
            with self.subTest(id=id):
                self.id = id; self.binding = self.get_binding(id)
                result = self.compare(); self.assertTrue(result['relocated_equal'])
                self.assertEqual(result['indirect_call_checks'][0]['profile'], 'fixed-slot-v1')

    def test_wrong_slot_and_relocated_addend_reject(self):
        proof = self.binding['indirect_calls'][0]; proof['slot'] += 4
        proof['evidence'] = [fixed_slot.REVIEW + '#slots.' + hex(proof['slot'])]
        with self.assertRaisesRegex(ValueError, 'root/addend'): self.compare()
        self.binding['symbols'][fixed_slot.ROOT_SYMBOL]['references'][0]['addend'] += 4
        self.change_code(proof['offset'] + 2, struct.pack('<I',proof['slot']))
        with self.assertRaisesRegex(ValueError, 'root/addend'): self.compare()

    def test_real_conflict_and_unowned_provider_reject(self):
        for slot in (0x4c, 0x208):
            id = next(id for id,row in self.review['wrappers'].items() if row['module'] == 'TITLE.DLL' and row['slot'] == slot)
            with self.subTest(slot=slot):
                self.rejected_binding(id)
                with self.assertRaisesRegex(ValueError, 'provider ABI'): self.compare()

    def test_wrong_root_symbol_base_and_transfer_reject(self):
        baseline = copy.deepcopy(self.binding)
        for mutate in [lambda b:b['indirect_calls'][0].update(root='_other'),
                       lambda b:b['symbols'][fixed_slot.ROOT_SYMBOL].update(rva=0x2bb44),
                       lambda b:b['indirect_calls'][0].update(transfer='jmp')]:
            self.binding = copy.deepcopy(baseline); mutate(self.binding)
            with self.assertRaises(ValueError): self.compare()

    def test_wrong_push_count_cleanup_and_ret_reject(self):
        baseline = copy.deepcopy(self.binding)
        for key in ('pushed_dwords', 'cleanup_bytes'):
            self.binding = copy.deepcopy(baseline); self.binding['indirect_calls'][0][key] += 1
            with self.assertRaisesRegex(ValueError, 'wrapper'): self.compare()
        self.binding = baseline
        code = COFF(self.object()).function(self.rows[self.id]['symbol'])['data']
        self.change_code(code.index(b'\xc3'), b'\xcb')
        with self.assertRaises(ValueError): self.compare()

    def test_review_hash_and_original_input_hash_reject(self):
        self.binding['indirect_calls'][0]['review_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'review hash mismatch'): self.compare()
        self.binding = self.get_binding(self.id)
        row = self.rows[self.id]; original = match.ROOT / row['target']
        self.target = Path(self.directory.name) / original.name
        raw = bytearray(original.read_bytes()); raw[-1] ^= 1; self.target.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'original input hash mismatch'): self.compare()

    def test_review_file_itself_is_pinned(self):
        root = Path(self.directory.name)
        path = root / fixed_slot.REVIEW; path.parent.mkdir(parents=True)
        path.write_bytes((match.ROOT / fixed_slot.REVIEW).read_bytes() + b'\n')
        with patch.object(fixed_slot, 'ROOT', root):
            with self.assertRaisesRegex(ValueError, 'review hash mismatch'): self.compare()

    def test_register_indirect_and_unreviewed_transfers_reject(self):
        off = self.binding['indirect_calls'][0]['offset']
        self.change_code(off, b'\xff\xd0\x90\x90\x90\x90')
        with self.assertRaises(ValueError): self.compare()
        self.obj = None; del self.binding['indirect_calls']
        with self.assertRaisesRegex(ValueError, 'Unsupported indirect'): self.compare()

    def test_owner_record_and_fresh_owner_are_mandatory(self):
        with self.assertRaisesRegex(ValueError, 'compiled owner'): self.compare(owners=False)
        baseline = copy.deepcopy(self.binding)
        for mutate in [lambda b:b['symbols'].clear(),
                       lambda b:b['symbols'][fixed_slot.ROOT_SYMBOL].update(owner_storage='zero-section-v1'),
                       lambda b:b['symbols'][fixed_slot.ROOT_SYMBOL].update(owner_storage='unknown'),
                       lambda b:b['symbols'][fixed_slot.ROOT_SYMBOL].update(size=996),
                       lambda b:b['symbols'][fixed_slot.ROOT_SYMBOL].update(kind='data')]:
            self.binding = copy.deepcopy(baseline); mutate(self.binding)
            with self.assertRaises(ValueError): self.compare()

    def test_strict_schema_alignment_duplicate_and_unused_proofs_reject(self):
        baseline = copy.deepcopy(self.binding)
        for changes in [dict(slot=1),dict(slot=1000),dict(offset=True),dict(extra=True),dict(profile='unknown'),dict(evidence=[])]:
            self.binding = copy.deepcopy(baseline); self.binding['indirect_calls'][0].update(changes)
            with self.assertRaises(ValueError): self.compare()
        self.binding = baseline; self.binding['indirect_calls'] *= 2
        with self.assertRaisesRegex(ValueError, 'duplicate'): self.compare()

    def test_argument_reordering_and_additional_transfer_cannot_hide(self):
        # Swap the two real PUSH registers, leaving every other field intact.
        code = COFF(self.object()).function(self.rows[self.id]['symbol'])['data']
        shape = fixed_slot.wrapper_shape(code,0)
        self.change_code(shape['offset'] - 2, code[shape['offset'] - 1:shape['offset']] + code[shape['offset'] - 2:shape['offset'] - 1])
        with self.assertRaisesRegex(ValueError, 'forwarding'): self.compare()
        self.obj = None
        ret = code.index(b'\xc3')
        self.change_code(ret, b'\xff\xd0')
        with self.assertRaises(ValueError): self.compare()

    def test_all_generated_bindings_preserve_accepted_owner_fields(self):
        docs = [json.loads((match.ROOT / p).read_bytes()) for p in
                ('evidence/interface_transfer_bindings_2026_10_25.json','evidence/title_interface_bindings_2026_10_25.json')]
        count = 0
        for document in self.documents.values():
            for binding in document['functions'].values():
                owner = binding['symbols'][fixed_slot.ROOT_SYMBOL]
                expected = next(f['symbols'][fixed_slot.ROOT_SYMBOL] for d in docs for f in d['functions'].values()
                                if f.get('symbols',{}).get(fixed_slot.ROOT_SYMBOL,{}).get('rva') == owner['rva'])
                except_fields = {'references'}
                self.assertEqual({k:v for k,v in owner.items() if k not in except_fields},
                                 {k:v for k,v in expected.items() if k not in except_fields})
                count += 1
        self.assertEqual(count, self.review['counts']['agree'] - 2)


if __name__ == '__main__': unittest.main()
