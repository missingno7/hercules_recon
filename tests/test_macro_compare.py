"""Tests for experimental diagnostics; none alters the strict acceptance gate."""
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from env import ROOT
from match import digest
from macro_compare import structure, tokens_and_calls, restore_wave, audit_symbol_map, decode, encoded_absolute_references
import struct


class MacroMetricsTests(unittest.TestCase):
    def test_fixed_image_references_include_absolute_memory_and_pointer_loads(self):
        sections=[dict(virtual_address=0x7a0000,virtual_size=0x10000,raw_size=0)]
        # Absolute memory, indexed absolute memory, address load and address push.
        code=bytes.fromhex('a1c029ba008b0485d029ba00b8e029ba0068e429ba00c3')
        refs=encoded_absolute_references(decode(code,0x404450),0x400000,sections)
        self.assertEqual(refs,{0x7a29c0,0x7a29d0,0x7a29e0,0x7a29e4})

    def test_fixed_image_audit_rejects_relative_offsets_scalar_arithmetic_and_unmapped_values(self):
        sections=[dict(virtual_address=0x7a0000,virtual_size=0x10000,raw_size=0)]
        code=bytes.fromhex('8b80c029ba003dc029ba0005c029ba00a10000007fc3')
        self.assertEqual(encoded_absolute_references(decode(code,0x404450),0x400000,sections),set())

    def test_member_symbol_requires_all_observed_member_addresses(self):
        class Object:
            def function(self,name):
                return dict(data=struct.pack('<II',12,16),relocations=[
                    dict(offset=0,type=6,symbol='_records'),dict(offset=4,type=6,symbol='_records')])
        spec=dict(functions=[dict(symbol='_fn',rva='0x1000')],symbol_rvas={'_records':'0x2000'})
        audit_symbol_map(spec,Object(),set(),{0x200c,0x2010})
        with self.assertRaises(ValueError):audit_symbol_map(spec,Object(),set(),{0x200c})
        wrong=dict(spec,symbol_rvas={'_records':'0x2001'})
        with self.assertRaises(ValueError):audit_symbol_map(wrong,Object(),set(),{0x200c,0x2010})

    def test_unreferenced_symbol_base_is_not_member_evidence(self):
        class Object:
            def function(self,name):return dict(data=b'',relocations=[])
        spec=dict(functions=[dict(symbol='_fn',rva='0x1000')],symbol_rvas={'_unknown':'0x2000'})
        with self.assertRaises(ValueError):audit_symbol_map(spec,Object(),set(),{0x200c})

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
