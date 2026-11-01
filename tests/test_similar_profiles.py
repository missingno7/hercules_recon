"""In-place masked staging on a shared source: one plan owns one module, profiles never cross-contaminate."""
import copy
import hashlib
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest
from unittest.mock import patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / 'scripts'))
import similar

REAL_CONNECT = sqlite3.connect
SOURCE = 'src/shared/reg.c'
E1_FLAGS = ['/O2']
E3_FLAGS = ['/O2', '/DHERCULES_ENGINE3']


class Obj:
    def __init__(self, source, flags):
        self.source, self.flags = Path(source), tuple(flags)


class SharedSourceProfileTests(unittest.TestCase):
    def setUp(self):
        parent = (PROJECT_ROOT / 'work/test-fixtures').resolve()
        parent.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix='masked-profile-', dir=parent)
        self.root = Path(temporary.name).resolve()
        self.addCleanup(temporary.cleanup)
        self.addCleanup(patch.stopall)
        (self.root / 'src/shared').mkdir(parents=True)
        (self.root / SOURCE).write_bytes(b'/* shared registry */\n')
        (self.root / 'src/other.c').write_bytes(b'/* other */\n')
        self.targets = {}
        for module in ('ENG1.DLL', 'ENG3.DLL'):
            (self.root / module).write_bytes(b'synthetic oracle ' + module.encode())
            self.targets[module] = hashlib.sha256((self.root / module).read_bytes()).hexdigest()

        def memory_db(path):
            db = REAL_CONNECT(':memory:')
            db.execute('create table modules(name, path)')
            db.execute('create table refs(module, target_rva, source_rva, kind)')
            db.executemany('insert into modules values(?, ?)', [(m, m) for m in self.targets])
            return db
        self.defined = {'_a', '_b', '_c', '_d'}
        self.identity = lambda obj, symbol: 'id' + symbol
        self.compiled, self.compared = [], []
        self.state_path = self.root / 'recovery.json'
        self.state_path.write_bytes((json.dumps(dict(functions=[]), indent=2) + '\n').encode())
        for name, value in (('ROOT', self.root), ('STATE', self.state_path), ('DB', self.root / 'arch.sqlite')):
            patch.object(similar, name, value).start()
        patch.object(similar.sqlite3, 'connect', side_effect=memory_db).start()
        patch.object(similar, 'compile_source', side_effect=self.compile).start()
        patch.object(similar, 'defined_functions', side_effect=lambda obj: set(self.defined)).start()
        patch.object(similar, 'interior_entries', return_value=0).start()
        patch.object(similar, 'masked_compare', side_effect=self.compare).start()
        patch.object(similar, 'masked_identity', side_effect=lambda obj, symbol: self.identity(obj, symbol)).start()
        patch.object(similar, 'verify_toolchain', return_value='tc').start()

    def compile(self, source, flags):
        self.compiled.append((Path(source).relative_to(self.root).as_posix(), list(flags)))
        return Obj(source, flags), []

    def compare(self, obj, symbol, target, rva, size):
        self.compared.append((symbol, Path(target).name, obj.flags))
        return dict(masked_equal=True, symbols={})

    def plan(self, module, symbols, flags, base, **extra):
        plan = dict(source=SOURCE, in_place=True, module=module, flags=flags,
                    functions=[dict(rva=hex(base + 16 * i), size=16, symbol=s, evidence=f'{module} {s}')
                               for i, s in enumerate(symbols)])
        plan.update(extra)
        return plan

    def e1_plan(self, **extra):
        return self.plan('ENG1.DLL', ['_a', '_b'], E1_FLAGS, 0x1000, **extra)

    def e3_plan(self, flags=E3_FLAGS, **extra):
        return self.plan('ENG3.DLL', ['_c', '_d'], flags, 0x2000, **extra)

    def stage(self, plan):
        path = self.root / 'plan.json'
        path.write_bytes(json.dumps(plan).encode())
        similar.stage(path)
        return self.rows()

    def rows(self):
        return {r['id']: r for r in json.loads(self.state_path.read_text())['functions']}

    def ids(self):
        return [r['id'] for r in json.loads(self.state_path.read_text())['functions']]

    def assert_verify_agrees(self):
        similar.verify()

    def test_stage_other_module_keeps_flags_and_proofs(self):
        eng3 = self.stage(self.e3_plan())
        self.assertEqual(eng3['eng3.c']['flags'], E3_FLAGS)
        before = copy.deepcopy(eng3)
        rows = self.stage(self.e1_plan())
        for row_id in ('eng3.c', 'eng3.d'):
            self.assertEqual(rows[row_id], before[row_id])
            self.assertEqual(rows[row_id]['flags'], E3_FLAGS)
        self.assertEqual({rows['eng1.a']['symbol'], rows['eng1.b']['symbol']}, {'_a', '_b'})
        self.assertEqual(rows['eng1.a']['flags'], E1_FLAGS)
        # ENG3 rows were not recompared by the ENG1 stage; each profile pins what it leaves co-defined.
        self.assertEqual(self.compared[-2:], [('_a', 'ENG1.DLL', tuple(E1_FLAGS)), ('_b', 'ENG1.DLL', tuple(E1_FLAGS))])
        self.assertEqual(rows['eng1.a']['proof']['co_defined_functions'], ['_c', '_d'])
        self.assertEqual(rows['eng3.c']['proof']['co_defined_functions'], ['_a', '_b'])
        self.assertEqual(len(self.ids()), len(set(self.ids())))
        self.assert_verify_agrees()

    def test_restaging_an_id_replaces_it(self):
        self.stage(self.e3_plan())
        self.stage(self.e1_plan())
        rows = self.stage(self.e3_plan(flags=E3_FLAGS))
        self.assertEqual(sorted(self.ids()), ['eng1.a', 'eng1.b', 'eng3.c', 'eng3.d'])
        self.assertEqual(rows['eng3.c']['flags'], E3_FLAGS)
        self.assert_verify_agrees()

    def test_restaging_replaces_a_row_recorded_under_other_flags(self):
        # ENG3 rows once staged without the define, next to ENG1's rows of the same flags.
        self.stage(self.e1_plan())
        self.stage(self.e3_plan(flags=E1_FLAGS))
        self.assertEqual(self.rows()['eng3.c']['flags'], E1_FLAGS)
        self.stage(self.e1_plan())
        self.assertEqual(self.rows()['eng3.c']['flags'], E1_FLAGS)
        self.assert_verify_agrees()
        rows = self.stage(self.e3_plan())
        self.assertEqual(sorted(self.ids()), ['eng1.a', 'eng1.b', 'eng3.c', 'eng3.d'])
        self.assertEqual(rows['eng3.c']['flags'], E3_FLAGS)
        self.assertEqual(rows['eng3.d']['flags'], E3_FLAGS)
        # The /O2 profile no longer owns _c/_d, so it now pins them as co-defined.
        self.assertEqual(rows['eng1.a']['proof']['co_defined_functions'], ['_c', '_d'])
        self.assert_verify_agrees()

    def test_same_flags_group_pins_agree_without_rerecording_other_module(self):
        e1 = self.stage(self.e1_plan())
        eng1_before = copy.deepcopy({k: v for k, v in e1.items()})
        rows = self.stage(self.plan('ENG3.DLL', ['_c'], E1_FLAGS, 0x2000))
        # ENG1's rows only follow the shared group pins; nothing else about them is rewritten.
        for row_id in ('eng1.a', 'eng1.b'):
            changed = {k for k in rows[row_id] if rows[row_id][k] != eng1_before[row_id][k]}
            self.assertEqual(changed, {'proof'})
            self.assertEqual({k for k in rows[row_id]['proof'] if rows[row_id]['proof'][k] != eng1_before[row_id]['proof'].get(k)},
                             {'co_defined_functions'})
            self.assertEqual(rows[row_id]['proof']['co_defined_functions'], ['_d'])
        self.assertEqual(rows['eng3.c']['proof']['co_defined_functions'], ['_d'])
        self.assert_verify_agrees()

    def test_other_module_row_changed_by_the_group_refuses(self):
        self.stage(self.e1_plan())
        self.identity = lambda obj, symbol: 'changed' + symbol if symbol == '_a' else 'id' + symbol
        before = self.state_path.read_bytes()
        with self.assertRaisesRegex(ValueError, 'another module'):
            self.stage(self.plan('ENG3.DLL', ['_c'], E1_FLAGS, 0x2000))
        self.assertEqual(self.state_path.read_bytes(), before)

    def test_duplicate_ids_are_refused(self):
        self.stage(self.e3_plan())
        before = self.state_path.read_bytes()
        compiled = len(self.compiled)
        cases = []
        taken = self.e1_plan()
        taken['functions'][0]['id'] = 'eng3.c'
        cases.append(taken)
        twice = self.e1_plan()
        twice['functions'][1]['id'] = 'eng1.a'
        cases.append(twice)
        for plan in cases:
            with self.subTest(plan=len(cases)), self.assertRaisesRegex(ValueError, 'duplicate row ids'):
                self.stage(plan)
            self.assertEqual(self.state_path.read_bytes(), before)
        self.assertEqual(len(self.compiled), compiled)  # refused before any compilation

    def test_duplicate_state_result_is_refused_at_record_time(self):
        rows = [dict(id='x'), dict(id='y'), dict(id='x')]
        with self.assertRaisesRegex(ValueError, 'duplicate row ids: x'):
            similar._require_unique_ids(rows)

    def test_restaged_id_of_other_flags_keeps_its_rva(self):
        self.stage(self.e3_plan(flags=E1_FLAGS))
        moved = self.e3_plan()
        moved['functions'][0].update(id='eng3.c', rva='0x3000')
        before = self.state_path.read_bytes()
        with self.assertRaisesRegex(ValueError, 'keeps its RVA'):
            self.stage(moved)
        self.assertEqual(self.state_path.read_bytes(), before)

    def test_planned_rva_tracked_by_another_profile_is_refused(self):
        self.stage(self.e3_plan(flags=E1_FLAGS))
        clash = self.plan('ENG3.DLL', ['_c'], E3_FLAGS, 0x2010)
        before = self.state_path.read_bytes()
        with self.assertRaisesRegex(ValueError, 'already tracked as eng3.d'):
            self.stage(clash)
        self.assertEqual(self.state_path.read_bytes(), before)

    def test_strict_row_of_other_flags_is_not_replaced(self):
        state = json.loads(self.state_path.read_text())
        state['functions'].append(dict(id='eng3.c', status='FUNCTION_MATCH_RELOCATED', target='ENG3.DLL',
                                       target_sha256=self.targets['ENG3.DLL'], rva='0x2000', size=16, source=SOURCE,
                                       symbol='_c', flags=E1_FLAGS, evidence='e', proof={}))
        self.state_path.write_bytes((json.dumps(state, indent=2) + '\n').encode())
        before = self.state_path.read_bytes()
        with self.assertRaisesRegex(ValueError, 'strict row cannot be staged'):
            self.stage(self.e3_plan())
        self.assertEqual(self.state_path.read_bytes(), before)

    def test_untouched_bytes_and_line_endings(self):
        self.stage(self.e3_plan())
        self.stage(self.e1_plan())
        self.assertNotIn(b'\r', self.state_path.read_bytes())
        self.assertNotIn(b'\r', (self.root / SOURCE).read_bytes())


if __name__ == '__main__':
    unittest.main()
