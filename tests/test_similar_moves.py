"""In-place masked staging: rename by id, explicit single-row moves, per-module strict symbols."""
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
DEFINED = {'src/a.c': {'_shared_x', '_stay', '_x2'}, 'src/b.c': {'_canon', '_other', '_b3'}}


class MaskedMoveTests(unittest.TestCase):
    def setUp(self):
        parent = (PROJECT_ROOT / 'work/test-fixtures').resolve()
        parent.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix='masked-move-', dir=parent)
        self.root = Path(temporary.name).resolve()
        self.addCleanup(temporary.cleanup)
        self.addCleanup(patch.stopall)
        (self.root / 'src').mkdir()
        for name in DEFINED:
            (self.root / name).write_bytes(('/* %s */\n' % name).encode())
        targets = {}
        for module in ('T1.DLL', 'T2.DLL'):
            (self.root / module).write_bytes(b'synthetic oracle ' + module.encode())
            targets[module] = hashlib.sha256((self.root / module).read_bytes()).hexdigest()

        def memory_db(path):
            db = REAL_CONNECT(':memory:')
            db.execute('create table modules(name, path)')
            db.execute('create table refs(module, target_rva, source_rva, kind)')
            db.executemany('insert into modules values(?, ?)', [(m, m) for m in targets])
            return db
        source_sha = {name: hashlib.sha256((self.root / name).read_bytes()).hexdigest() for name in DEFINED}

        def row(row_id, module, status, source, symbol, rva, **proof):
            r = dict(id=row_id, status=status, target=module, target_sha256=targets[module], rva=rva, size=16,
                     source=source, symbol=symbol, flags=['/O2'], evidence='e')
            if status == 'CODEGEN_SIMILAR':
                r['link_symbols'] = {}
                r['proof'] = dict(scope=similar.SCOPE, source_sha256=source_sha[source],
                                  masked_sha256='id' + symbol, toolchain='tc', **proof)
            else:
                r['proof'] = dict(source_sha256=source_sha[source])
            return r
        self.state = dict(functions=[
            row('m2.moved', 'T2.DLL', 'CODEGEN_SIMILAR', 'src/a.c', '_shared_x', '0x1000', co_defined_functions=['_x2']),
            row('m1.stay', 'T1.DLL', 'CODEGEN_SIMILAR', 'src/a.c', '_stay', '0x2000', co_defined_functions=['_x2']),
            row('t1.strict', 'T1.DLL', 'FUNCTION_MATCH_RELOCATED', 'src/a.c', '_shared_x', '0x3000'),
            row('t1.canon', 'T1.DLL', 'FUNCTION_MATCH_RELOCATED', 'src/b.c', '_canon', '0x4000'),
            row('m2.other', 'T2.DLL', 'CODEGEN_SIMILAR', 'src/b.c', '_other', '0x5000', co_defined_functions=['_b3', '_canon']),
        ])
        self.state_path = self.root / 'recovery.json'
        self.state_path.write_bytes((json.dumps(self.state, indent=2) + '\n').encode())
        self.defined = {k: set(v) for k, v in DEFINED.items()}
        for name, value in (('ROOT', self.root), ('STATE', self.state_path), ('DB', self.root / 'arch.sqlite')):
            patch.object(similar, name, value).start()
        patch.object(similar.sqlite3, 'connect', side_effect=memory_db).start()
        patch.object(similar, 'compile_source', side_effect=lambda source, flags: (Path(source), [])).start()
        patch.object(similar, 'defined_functions',
                     side_effect=lambda obj: set(self.defined[Path(obj).relative_to(self.root).as_posix()])).start()
        patch.object(similar, 'interior_entries', return_value=0).start()
        patch.object(similar, 'masked_compare', return_value=dict(masked_equal=True, symbols={})).start()
        patch.object(similar, 'masked_identity', side_effect=lambda obj, symbol: 'id' + symbol).start()
        patch.object(similar, 'verify_toolchain', return_value='tc').start()

    def stage(self, plan):
        path = self.root / 'plan.json'
        path.write_bytes(json.dumps(plan).encode())
        similar.stage(path)
        return {r['id']: r for r in json.loads(self.state_path.read_text())['functions']}

    def move_plan(self, **extra):
        plan = dict(source='src/b.c', in_place=True, module='T2.DLL', flags=['/O2'], moves=['m2.moved'],
                    functions=[dict(id='m2.moved', rva='0x1000', size=16, symbol='_canon', evidence='moved')])
        plan.update(extra)
        return plan

    def test_move_resources_one_row_and_repins_the_old_group(self):
        rows = self.stage(self.move_plan())
        self.assertEqual((rows['m2.moved']['source'], rows['m2.moved']['symbol']), ('src/b.c', '_canon'))
        self.assertEqual(rows['m2.moved']['proof']['co_defined_functions'], ['_b3'])
        self.assertEqual(rows['m2.other']['proof']['co_defined_functions'], ['_b3'])
        # The moved symbol is now pinned co-defined for the rows left in the old provider group.
        self.assertEqual(rows['m1.stay']['proof']['co_defined_functions'], ['_shared_x', '_x2'])
        self.assertEqual(rows['t1.strict'], {r['id']: r for r in self.state['functions']}['t1.strict'])
        self.assertEqual(len(rows), 5)

    def test_rename_restages_by_id(self):
        self.defined['src/b.c'] = {'_canon', '_renamed', '_b3'}
        plan = dict(source='src/b.c', in_place=True, module='T2.DLL', flags=['/O2'],
                    functions=[dict(id='m2.other', rva='0x5000', size=16, symbol='_renamed', evidence='renamed')])
        rows = self.stage(plan)
        self.assertEqual(rows['m2.other']['symbol'], '_renamed')
        self.assertEqual(sum(1 for r in json.loads(self.state_path.read_text())['functions'] if r['id'] == 'm2.other'), 1)

    def test_restage_keeps_rva(self):
        plan = dict(source='src/b.c', in_place=True, module='T2.DLL', flags=['/O2'],
                    functions=[dict(id='m2.other', rva='0x6000', size=16, symbol='_other', evidence='x')])
        with self.assertRaisesRegex(ValueError, 'keeps its RVA'):
            self.stage(plan)

    def test_strict_symbol_blocks_only_its_own_module(self):
        plan = dict(source='src/b.c', in_place=True, module='T1.DLL', flags=['/O2'],
                    functions=[dict(id='m1.new', rva='0x7000', size=16, symbol='_canon', evidence='x')])
        with self.assertRaisesRegex(ValueError, 'strict row cannot be staged'):
            self.stage(plan)

    def test_move_refusals(self):
        before = self.state_path.read_bytes()
        cases = [
            (self.move_plan(in_place=False, candidate='src/b.c'), 'need an in-place provider|take'),
            (self.move_plan(moves=['t1.strict']), 'A move names'),
            (self.move_plan(moves=['m1.stay']), 'A move names'),
            (self.move_plan(moves=['m2.moved', 'm2.moved']), 'distinct row ids'),
            (self.move_plan(functions=[dict(id='m2.moved', rva='0x1010', size=16, symbol='_canon', evidence='x')]),
             'A move names'),
            (dict(self.move_plan(), moves=[]), 'already tracked as m2.moved'),
        ]
        for plan, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                self.stage(plan)
            self.assertEqual(self.state_path.read_bytes(), before)

    def test_changed_row_left_behind_refuses_the_move(self):
        patch.object(similar, 'masked_identity', side_effect=lambda obj, symbol: 'changed').start()
        before = self.state_path.read_bytes()
        with self.assertRaisesRegex(ValueError, 'left in the old provider changed'):
            self.stage(self.move_plan())
        self.assertEqual(self.state_path.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
