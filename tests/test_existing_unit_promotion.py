"""Status-only mixed-unit acceptance must protect all prior evidence."""
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import promote
import similar
from test_matching import make_coff, make_pe, CODE, workspace_temporary_directory


class ExistingUnitPromotionTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('unit-promotion-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'src').mkdir()
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
