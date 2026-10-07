"""Tests for experimental diagnostics; none alters the strict acceptance gate."""
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from env import ROOT
from match import digest
from macro_compare import structure, tokens_and_calls, restore_wave


class MacroMetricsTests(unittest.TestCase):
    def test_cfg_ignores_layout_but_preserves_predicate(self):
        original = bytes.fromhex('85c0740240c348c3')
        shifted = b'\x90'+original
        self.assertEqual(structure(original,0x1000),structure(shifted,0x2000))
        different = bytes.fromhex('85c0750240c348c3')
        self.assertNotEqual(structure(original,0x1000),structure(different,0x1000))

    def test_call_target_identity_is_not_masked(self):
        first = bytes.fromhex('e8fb2f0000c3')  # CALL 0x4000 from 0x1000
        shifted = bytes.fromhex('e8fb1f0000c3') # CALL 0x4000 from 0x2000
        wrong = bytes.fromhex('e8fc1f0000c3')
        self.assertEqual(tokens_and_calls(first,0x1000)[1],tokens_and_calls(shifted,0x2000)[1])
        self.assertNotEqual(tokens_and_calls(first,0x1000)[1],tokens_and_calls(wrong,0x2000)[1])

    def test_every_wave_is_recoverable(self):
        spec = json.loads((ROOT/'evidence/macro_actor.json').read_text())
        source = (ROOT/spec['source']).read_text()
        for wave in spec['waves']:
            restored = restore_wave(spec,wave['wave'],source)
            self.assertEqual(digest(restored.encode()),wave['source_sha256'])

    def test_wave_recovery_rejects_changed_source(self):
        spec = json.loads((ROOT/'evidence/macro_actor.json').read_text())
        with self.assertRaises(ValueError):
            restore_wave(spec,0,(ROOT/spec['source']).read_text()+'\n')


if __name__ == '__main__':
    unittest.main()
