"""Status-only mixed-unit acceptance must protect all prior evidence."""
import copy
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import promote
import similar
import masked
from test_matching import make_coff, make_pe, CODE, workspace_temporary_directory
from test_relocated import binding as reference_binding


def make_mixed_unit_coff(path):
    """Three independently bounded functions, one with a reviewed external call."""
    flags = 0x60501020
    code = b'\xe8\0\0\0\0\xc3'
    contributions = [
        (b'.text$A', CODE, []),
        (b'.text$B', code, [(1, 3, masked.REL32)]),
        (b'.text$C', CODE, []),
    ]
    offset = 20 + 40 * len(contributions)
    headers, payload = bytearray(), bytearray()
    for name, contents, uses in contributions:
        relocs = b''.join(struct.pack('<IIH', at, symbol, kind) for at, symbol, kind in uses)
        headers += struct.pack('<8sIIIIIIHHI', name, 0, 0, len(contents), offset,
                               offset + len(contents) if relocs else 0, 0, len(uses), 0, flags)
        payload += contents + relocs
        offset += len(contents) + len(relocs)
    symbols = b''.join(struct.pack('<8sIhHBB', name.encode(), 0, section, 0x20, 2, 0)
                       for name, section in (('_sample', 1), ('_reloc', 2),
                                             ('_extra', 3), ('_callee', 0)))
    path.write_bytes(struct.pack('<HHIIIHH', 0x14c, len(contributions), 0, offset,
                                 4, 0, 0) + headers + payload + symbols + struct.pack('<I', 4))


class ExistingUnitPromotionTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('unit-promotion-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'src').mkdir()
        (self.root / 'evidence').mkdir()
        (self.root / 'toolchains').mkdir()
        (self.root / 'toolchains/manifest.json').write_bytes(b'fixture manifest')
        self.source = self.root / 'src/unit.c'
        self.source.write_bytes(b'#include "unit.h"\nint sample(void) { return 0; }\n')
        self.header = self.root / 'src/unit.h'
        self.header.write_bytes(b'/* genuine header */\n')
        self.obj = self.root / 'unit.obj'
        make_coff(self.obj, extra_function=True, extra_data=True)
        self.target = self.root / 'TEST.DLL'
        make_pe(self.target)
        data = bytearray(self.target.read_bytes())
        data[0x410:0x420] = CODE
        self.target.write_bytes(data)
        self.rows = [dict(id=name, source='src/unit.c', symbol=symbol,
                         target='TEST.DLL', target_sha256=match.digest(data), rva=hex(rva), size=16,
                         flags=['/O2'], status='CODEGEN_SIMILAR', link_symbols={},
                         proof=dict(scope=similar.SCOPE, source_sha256=match.digest(self.source.read_bytes()),
                                    masked_sha256=match.digest(CODE), toolchain='fixture'))
                     for name, symbol, rva in [('sample', '_sample', 0x1000), ('peer', '_extra', 0x1010)]]
        self.state = self.root / 'recovery.json'
        self.state.write_bytes(json.dumps({'functions': self.rows}).encode())
        self.journal = self.root / 'work/promotion-journal.json'
        self.addCleanup(patch.stopall)
        for module in (match, promote):
            patch.object(module, 'ROOT', self.root).start()
        patch.object(promote, 'STATE', self.state).start()
        patch.object(promote, 'JOURNAL', self.journal).start()
        patch.object(match, 'verify_toolchain', Mock(return_value='fixture')).start()
        self.compiler = patch.object(match, 'compile_source', return_value=(self.obj, ['synthetic'])).start()

    def multi_target_rows(self, co_defined=('_extra',)):
        """Represent one source symbol mapped to two target modules."""
        other = self.root / 'OTHER.DLL'
        other.write_bytes(self.target.read_bytes())
        rows = []
        for row_id, target in (('module-a', self.target), ('module-b', other)):
            row = copy.deepcopy(self.rows[0])
            row.update(id=row_id, target=target.name, target_sha256=match.digest(target.read_bytes()),
                       symbol='_sample', rva='0x1000', size=16, link_symbols={})
            row['proof']['co_defined_functions'] = list(co_defined)
            rows.append(row)
        self.state.write_text(json.dumps({'functions': rows}))
        return rows

    def prepare_same_unit_relocated_peer(self):
        obj_code = self.root / 'unit.obj'
        make_mixed_unit_coff(obj_code)
        image = bytearray(self.target.read_bytes())
        call = b'\xe8' + struct.pack('<i', 0x1180 - (0x1100 + 5)) + b'\xc3'
        image[0x500:0x500 + len(call)] = call
        self.target.write_bytes(image)
        source_sha = match.digest(self.source.read_bytes())
        target_sha = match.digest(image)
        self.compiler.return_value = (obj_code, ['synthetic'])

        note = self.root / 'evidence/reference-map.txt'
        note.write_bytes(b'independent synthetic symbol identity\n')
        bindings = self.root / 'evidence/reference-bindings.json'
        document = {
            'schema': 1,
            'evidence_files': {'evidence/reference-map.txt': match.digest(note.read_bytes())},
            'functions': {'relocated-peer': {'symbols': {
                '_callee': reference_binding('function', 0x1180, 1,
                                             (1, masked.REL32, 0)),
            }}},
        }
        bindings.write_text(json.dumps(document, indent=2))

        raw = copy.deepcopy(self.rows[0])
        raw.update(id='raw-peer', symbol='_sample', target=self.target.name,
                   target_sha256=target_sha, rva='0x1000', size=len(CODE), link_symbols={})
        raw['proof']['source_sha256'] = source_sha
        raw['proof']['co_defined_functions'] = ['_extra']
        relocated = copy.deepcopy(raw)
        relocated.update(id='relocated-peer', symbol='_reloc', rva='0x1100', size=len(call))
        relocated['proof']['masked_sha256'] = similar.masked_identity(obj_code, '_reloc')
        self.state.write_text(json.dumps({'functions': [raw, relocated]}))
        promote.promote_relocated(['relocated-peer'], bindings)
        return note, bindings, obj_code

    def test_promotes_only_requested_raw_row_without_moving_source_or_accepting_data(self):
        source_before = self.source.read_bytes()
        receipt = promote.promote_existing_units(['sample'])
        after = json.loads(self.state.read_bytes())['functions']
        self.assertEqual(after[0]['status'], 'FUNCTION_MATCH')
        self.assertEqual(after[1], self.rows[1])
        self.assertEqual(self.source.read_bytes(), source_before)
        self.assertIn('unit_inventory_sha256', after[0]['proof'])
        self.assertIn('src/unit.h', after[0]['proof']['context_sha256'])
        self.assertEqual(len(receipt['units']['src/unit.c']['sections']), 3)
        self.assertFalse(self.journal.exists())
        self.assertTrue(match.verify_rows([after[0]])[0][0]['exact'])

    def test_multi_target_symbol_and_pinned_helper_have_one_exact_inventory(self):
        rows = self.multi_target_rows()

        receipt = promote.promote_existing_units(['module-a', 'module-b'])

        after = json.loads(self.state.read_text())['functions']
        self.assertEqual([row['status'] for row in after], ['FUNCTION_MATCH', 'FUNCTION_MATCH'])
        self.assertEqual(receipt['units']['src/unit.c']['functions'], ['_sample', '_extra'])
        self.assertEqual(len(receipt['results']), 2)
        self.assertEqual({row['target'] for row in rows}, {'TEST.DLL', 'OTHER.DLL'})

    def test_missing_pinned_function_and_unrecorded_helper_are_rejected(self):
        self.multi_target_rows(('_missing',))
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'function inventory'):
            promote.promote_existing_units(['module-a'])

        self.assertEqual(self.state.read_bytes(), before)

    def test_inconsistent_co_defined_function_declarations_are_rejected(self):
        rows = self.multi_target_rows()
        rows[1]['proof']['co_defined_functions'] = []
        self.state.write_text(json.dumps({'functions': rows}))
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'disagree about pinned co-defined'):
            promote.promote_existing_units(['module-a'])

        self.assertEqual(self.state.read_bytes(), before)

    def test_duplicate_emitted_function_symbols_are_rejected(self):
        self.multi_target_rows()
        obj = bytearray(self.obj.read_bytes())
        symbol_table = struct.unpack_from('<I', obj, 8)[0]
        obj[symbol_table + 18:symbol_table + 26] = b'_sample\0'
        self.obj.write_bytes(obj)
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'defined symbol|function inventory'):
            promote.promote_existing_units(['module-a'])

        self.assertEqual(self.state.read_bytes(), before)

    def test_co_defined_helper_has_no_independent_acceptance_row(self):
        self.multi_target_rows()
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'unique, reviewed masked rows'):
            promote.promote_existing_units(['helper'])

        self.assertEqual(self.state.read_bytes(), before)

    def test_reconstructed_peer_remains_unsupported(self):
        rows = self.multi_target_rows()
        rows[1]['status'] = 'RECONSTRUCTED'
        self.state.write_text(json.dumps({'functions': rows}))
        before = self.state.read_bytes()

        with self.assertRaisesRegex(ValueError, 'supported prior proof'):
            promote.promote_existing_units(['module-a'])

        self.assertEqual(self.state.read_bytes(), before)

    def test_same_unit_relocated_peer_keeps_its_full_proof_and_references(self):
        _note, _bindings, _obj = self.prepare_same_unit_relocated_peer()
        before = json.loads(self.state.read_text())['functions']
        relocated_before = copy.deepcopy(next(r for r in before if r['id'] == 'relocated-peer'))

        receipt = promote.promote_existing_units(['raw-peer'])

        after = json.loads(self.state.read_text())['functions']
        relocated_after = next(r for r in after if r['id'] == 'relocated-peer')
        self.assertEqual(relocated_after, relocated_before)
        self.assertEqual(next(r for r in after if r['id'] == 'raw-peer')['status'], 'FUNCTION_MATCH')
        self.assertEqual({r['id'] for r in receipt['results']}, {'raw-peer', 'relocated-peer'})

    def test_same_unit_relocated_peer_reference_regression_blocks_raw_promotion(self):
        note, _bindings, _obj = self.prepare_same_unit_relocated_peer()
        before = self.state.read_bytes()
        note.write_bytes(b'changed independent evidence\n')

        with self.assertRaisesRegex(ValueError, 'Independent reference evidence changed'):
            promote.promote_existing_units(['raw-peer'])

        self.assertEqual(self.state.read_bytes(), before)

    def test_masked_peer_regression_blocks_state_write(self):
        before = self.state.read_bytes()
        with patch('masked.masked_compare', return_value=dict(masked_equal=False)):
            with self.assertRaisesRegex(ValueError, 'masked peer regressed'):
                promote.promote_existing_units(['sample'])
        self.assertEqual(self.state.read_bytes(), before)

    def test_incomplete_function_inventory_blocks_state_write(self):
        self.state.write_text(json.dumps({'functions': self.rows[:1]}))
        before = self.state.read_bytes()
        with self.assertRaisesRegex(ValueError, 'function inventory'):
            promote.promote_existing_units(['sample'])
        self.assertEqual(self.state.read_bytes(), before)

    def test_missing_mapped_emitted_function_blocks_state_write(self):
        make_coff(self.obj)
        before = self.state.read_bytes()
        with self.assertRaisesRegex(ValueError, 'function inventory'):
            promote.promote_existing_units(['sample'])
        self.assertEqual(self.state.read_bytes(), before)

    def test_new_raw_mismatch_blocks_state_write(self):
        data = bytearray(self.target.read_bytes()); data[0x400] ^= 1
        self.target.write_bytes(data)
        for row in self.rows:
            row['target_sha256'] = match.digest(data)
        self.state.write_text(json.dumps({'functions': self.rows}))
        before = self.state.read_bytes()
        with self.assertRaisesRegex(ValueError, 'raw comparison'):
            promote.promote_existing_units(['sample'])
        self.assertEqual(self.state.read_bytes(), before)

    def test_header_and_noncode_changes_rejected_after_acceptance(self):
        promote.promote_existing_units(['sample'])
        row = json.loads(self.state.read_bytes())['functions'][0]
        header = self.header.read_bytes()
        self.header.write_bytes(header + b'/* changed */')
        with self.assertRaisesRegex(ValueError, 'context changed'):
            match.verify_rows([row])
        self.header.write_bytes(header)
        self.obj.write_bytes(self.obj.read_bytes().replace(b'data', b'DATA'))
        with self.assertRaisesRegex(ValueError, 'unit inventory changed'):
            match.verify_rows([row])

    def test_verify_only_does_not_publish(self):
        before = self.state.read_bytes()
        promote.promote_existing_units(['sample'], verify_only=True)
        self.assertEqual(self.state.read_bytes(), before)

    def test_source_hash_mismatch_is_not_reaccepted(self):
        self.source.write_bytes(self.source.read_bytes() + b'/* edited */')
        before = self.state.read_bytes()
        with self.assertRaisesRegex(ValueError, 'differs from recorded proof'):
            promote.promote_existing_units(['sample'])
        self.assertEqual(self.state.read_bytes(), before)
        self.compiler.assert_not_called()


if __name__ == '__main__':
    unittest.main()
