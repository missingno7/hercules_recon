"""Adversarial preservation checks; no compiler or original execution."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import unit_refresh as refresh
from test_matching import workspace_temporary_directory


class Coff:
    def __init__(self):
        self.sections = [dict(data=b'\xc3', size=1, flags=0x60501020, relocations=[]),
                         dict(data=b'\xa1\0\0\0\0\xc3', size=6, flags=0x60501020,
                              relocations=[dict(offset=1, symbol='_old', type=6)])]
        self.symbols = {0: dict(name='_old', value=0, section=0, type=0, storage=2, aux=[])}

    def function(self, name):
        return self.sections[{'_accepted': 0, '_pending': 1}[name]]


class UnitRefreshTests(unittest.TestCase):
    def setUp(self):
        self.before = Coff()
        self.after = copy.deepcopy(self.before)
        self.declaration = dict(profile='reviewed-unit-delta-v1', removed=[], added=[], rebases=[],
                                review='evidence/review.json', review_sha256='a' * 64)

    def check(self):
        return refresh.check_delta(self.before, self.after, self.declaration, {'_accepted'})

    def rebase(self):
        self.after.sections[1]['data'] = b'\xa1\x14\0\0\0\xc3'
        self.after.sections[1]['relocations'][0]['symbol'] = '_new'
        self.after.symbols[0]['name'] = '_new'
        self.declaration['removed'] = refresh.symbol_records(self.before)
        self.declaration['added'] = refresh.symbol_records(self.after)
        self.declaration['rebases'] = [dict(function='_pending', offset=1,
            before_symbol='_old', after_symbol='_new', before_addend=0, after_addend=20)]

    def test_default_symbol_only_refuses_real_operand_change(self):
        self.rebase()
        self.declaration['rebases'] = []
        with self.assertRaisesRegex(ValueError, 'Undeclared section'):
            self.check()

    def test_explicit_pending_rebase_preserves_all_other_content(self):
        self.rebase()
        self.assertEqual(self.check()['rebases'], 1)
        self.assertEqual(self.after.sections[1]['data'][1], 20)  # checker is read-only

    def test_accepted_change_cannot_be_declared_away(self):
        self.rebase()
        with self.assertRaisesRegex(ValueError, 'Accepted contribution'):
            refresh.check_delta(self.before, self.after, self.declaration, {'_pending'})

    def test_extra_byte_fixup_order_flags_extent_or_symbol_aux_refuses(self):
        self.rebase()
        baseline = copy.deepcopy(self.after)
        for mutate in (lambda c: c.sections[1].update(data=b'\xa1\x14\0\0\0\xcc'),
                       lambda c: c.sections[1].update(flags=0),
                       lambda c: c.sections[1].update(size=7),
                       lambda c: c.sections[1]['relocations'].append(dict(offset=0, symbol='_new', type=6)),
                       lambda c: c.symbols.update({9: dict(name='extra', value=0, section=0,
                                                          type=0, storage=2, aux=[])})):
            self.after = copy.deepcopy(baseline)
            mutate(self.after)
            with self.assertRaises(ValueError):
                self.check()

    def test_wrong_use_addend_duplicate_rebase_and_defined_symbol_refuse(self):
        self.rebase()
        self.declaration['rebases'][0]['before_addend'] = 1
        with self.assertRaisesRegex(ValueError, 'relocation/addend'):
            self.check()
        self.declaration['rebases'][0]['before_addend'] = 0
        self.declaration['rebases'] *= 2
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.check()
        self.declaration['rebases'] = self.declaration['rebases'][:1]
        self.declaration['removed'][0]['section'] = 1
        with self.assertRaisesRegex(ValueError, 'UNDEF'):
            self.check()

    def test_symbol_only_removal_and_index_shifts(self):
        self.before.sections[1]['relocations'] = []
        self.after = copy.deepcopy(self.before)
        self.after.symbols = {}
        self.declaration['removed'] = refresh.symbol_records(self.before)
        self.assertEqual(self.check()['rebases'], 0)
        self.after.sections[0]['data'] = b'\xcc'
        with self.assertRaisesRegex(ValueError, 'Accepted contribution'):
            self.check()

    def test_history_is_retained_and_stale_before_object_refuses(self):
        with workspace_temporary_directory('inventory-refresh-') as directory:
            root = Path(directory).resolve()
            review = root / 'evidence/review.json'
            review.parent.mkdir()
            source = 'src/unit.c'
            declared = {k: v for k, v in self.declaration.items() if k not in ('review', 'review_sha256')}
            review.write_text(json.dumps(dict(units={source: dict(declaration=declared)})))
            self.declaration['review_sha256'] = hashlib.sha256(review.read_bytes()).hexdigest()
            before, after = root / 'before.obj', root / 'after.obj'
            before.write_bytes(b'immutable before fixture')
            after.write_bytes(b'fresh after fixture')
            old_results = {'row': dict(object=str(before), object_sha256=hashlib.sha256(before.read_bytes()).hexdigest())}
            row = dict(id='row', source=source, flags=['/O2'], symbol='_accepted', proof=dict(
                unit_inventory_sha256='old raw', unit_inventory_portable_sha256='old portable',
                unit_inventory_history=[dict(earlier='retained')]))
            with patch('env.ROOT', root), patch.object(refresh, 'COFF', return_value=self.before), \
                    patch('match.compile_source', return_value=(after, ['fixture compiler'])), \
                    patch('match.unit_inventory', return_value='new raw'), \
                    patch('match.portable_unit_inventory', return_value='new portable'):
                refresh.refresh([row], old_results, {source: self.declaration}, None)
                self.assertEqual(row['proof']['unit_inventory_sha256'], 'new raw')
                self.assertEqual(row['proof']['unit_inventory_portable_sha256'], 'new portable')
                history = row['proof']['unit_inventory_history']
                self.assertEqual(history[0], dict(earlier='retained'))
                self.assertEqual(history[-1]['unit_inventory_sha256'], 'old raw')
                self.assertEqual(history[-1]['unit_inventory_portable_sha256'], 'old portable')
                before.write_bytes(b'tampered')
                with self.assertRaisesRegex(ValueError, 'Before unit object changed'):
                    refresh.refresh([row], old_results, {source: self.declaration}, None)


if __name__ == '__main__':
    unittest.main()
