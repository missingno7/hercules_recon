"""Keep diagnostic leads and legacy layout work outside recovery authority."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import function_map
import title_link
import unit_bounds


class FixturePE:
    image_base = 0x400000
    sections = [dict(virtual_address=0x1000, virtual_size=64, raw_size=64, characteristics=0x20000000),
                dict(virtual_address=0x2000, virtual_size=4, raw_size=4, characteristics=0)]

    def __init__(self, path):
        self.data = path.read_bytes()

    def read_rva(self, rva, size):
        offset = rva - 0x1000 if rva < 0x2000 else 64 + rva - 0x2000
        return self.data[offset:offset + size]

    def metadata(self):
        return dict(exports=[])


class DiagnosticTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.local, self.original = root / 'local', root / 'original'
        for base in (self.local, self.original):
            (base / 'work').mkdir(parents=True)
        (self.original / 'oracle').mkdir()
        (self.original / 'work/library_map').mkdir()
        self.target = self.original / 'oracle/TEST.DLL'
        self.target.write_bytes(b'\xc3' * 64 + (0x401003).to_bytes(4, 'little'))
        (self.original / 'work/library_map/library_map.json').write_text(
            json.dumps(dict(contributions={'TEST.DLL': []})))
        self.state = dict(functions=[dict(target='oracle/TEST.DLL', rva='0x1010',
                                         size=16, status='FUNCTION_MATCH')])
        (self.local / 'recovery.json').write_text(json.dumps(self.state))
        self.database = self.original / 'work/archaeology.sqlite'
        with sqlite3.connect(self.database) as db:
            db.execute('create table modules(name, path)')
            db.execute('insert into modules values (?, ?)', ('TEST.DLL', 'oracle/TEST.DLL'))
            db.execute('create table refs(module, target_rva, source_rva, kind)')
            db.executemany('insert into refs values (?, ?, ?, ?)', [
                ('TEST.DLL', 0x1003, 0x1000, 'linear_call'),
                ('TEST.DLL', 0x1013, 0x1000, 'linear_call'),
                ('TEST.DLL', 0x1003, 0x2000, 'pe_highlow_pointer'),
                ('TEST.DLL', 0x2000, 0x1000, 'linear_memory'),
                ('TEST.DLL', 0x2000, 0x1010, 'linear_memory')])
        db.close()
        self.addCleanup(patch.stopall)
        patch.object(function_map, 'ROOT', self.local).start()
        patch.object(function_map, 'PE', FixturePE).start()

    def mapping(self, **kwargs):
        return function_map.build('TEST.DLL', original_root=self.original, **kwargs)

    def evidence(self, entry=0x1003):
        self.target.write_bytes(b'\xc3' * 64 + (0x400000 + entry).to_bytes(4, 'little'))
        data = self.target.read_bytes()
        record = dict(module='TEST.DLL', target_sha256=hashlib.sha256(data).hexdigest(),
                      pointer_tables=[dict(rva='0x2000', size=4, sha256=hashlib.sha256(data[64:]).hexdigest())],
                      entries=[dict(rva=hex(entry), size=1, sha256=hashlib.sha256(b'\xc3').hexdigest(), source_cells=['0x2000'])])
        path = self.local / 'entry_evidence.json'
        path.write_text(json.dumps(record))
        return path

    def test_default_inventory_keeps_existing_aligned_policy(self):
        result = self.mapping()
        self.assertEqual([r['rva'] for r in result['functions']], ['0x1000', '0x1010', '0x1020'])
        self.assertFalse(result['include_unaligned'])
        self.assertIn('not a complete function census', result['scope'])

    def test_opt_in_unaligned_leads_never_split_recorded_extent(self):
        result = self.mapping(include_unaligned=True)
        self.assertEqual([r['rva'] for r in result['functions']], ['0x1000', '0x1003', '0x1010', '0x1020'])
        self.assertEqual(json.loads((self.local / 'recovery.json').read_text()), self.state)

    def test_reviewed_entries_bind_to_original_root_not_local_checkout(self):
        result = self.mapping(entry_evidence=self.evidence())
        self.assertIn('0x1003', [r['rva'] for r in result['functions']])
        self.assertIn('0x1004', [r['rva'] for r in result['functions']])
        self.assertFalse((self.local / 'oracle').exists())

    def test_reviewed_entry_cannot_override_recorded_extent(self):
        with self.assertRaisesRegex(AssertionError, 'conflicts with recorded extent'):
            self.mapping(entry_evidence=self.evidence(0x1013))

    def test_storage_clusters_expose_assumptions_without_language_inference(self):
        mapping = self.local / 'map.json'
        mapping.write_text(json.dumps(self.mapping()))
        result = unit_bounds.build('TEST.DLL', mapping, 0x2000, 0x2004, set(), self.original)
        self.assertIn('not TU or source-language proof', result['scope'])
        self.assertEqual(result['intervals'][0]['functions'], 2)
        self.assertEqual(len(result['assumptions']), 2)
        self.assertEqual(unit_bounds.build('TEST.DLL', mapping, 0x2000, 0x2004, {0x2000}, self.original)['intervals'], [])

    def test_invalid_storage_range_is_rejected(self):
        for low, high in [(4, 4), (5, 4), (-1, 4), (0, 0x100000001)]:
            with self.subTest(low=low, high=high), self.assertRaises(ValueError):
                unit_bounds.build('TEST.DLL', self.local / 'absent.json', low, high, set())

    def test_missing_reference_database_is_not_created(self):
        mapping = self.local / 'map.json'
        mapping.write_text(json.dumps(self.mapping()))
        with self.assertRaises(sqlite3.OperationalError):
            unit_bounds.build('TEST.DLL', mapping, 0x2000, 0x2004, set(), self.local)
        self.assertFalse((self.local / 'work/archaeology.sqlite').exists())

    def test_cli_requires_explicit_storage_hypothesis(self):
        with patch.object(sys, 'argv', ['unit_bounds.py']), contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as result:
                unit_bounds.main()
        self.assertEqual(result.exception.code, 2)

    def test_natural_link_never_uses_forced_layout(self):
        with patch.object(title_link, 'ROOT', self.local), \
             patch.object(title_link, 'sources', return_value={}), \
             patch.object(title_link, 'verify_toolchain', return_value='fixture'), \
             patch.object(title_link.subprocess, 'run', return_value=types.SimpleNamespace(
                 returncode=1, stdout='unresolved external symbol _g_missing', stderr='')) as link:
            result = title_link.build('TITLE.DLL')
            self.assertEqual(result['unresolved_count'], 1)
            self.assertNotIn('layout', result)
        command = link.call_args.args[0]
        self.assertFalse(any(argument.upper().startswith('/FORCE') for argument in command))

    def test_legacy_forced_layout_option_is_rejected(self):
        with patch.object(sys, 'argv', ['title_link.py', 'TITLE.DLL', '--legacy-forced-layout']), \
             contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as result:
                title_link.main()
        self.assertEqual(result.exception.code, 2)


if __name__ == '__main__':
    unittest.main()
