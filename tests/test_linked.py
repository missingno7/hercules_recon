"""LINKED_MATCH gate: plan/flag/source refusals, natural-link refusals, promotion and verification.

Synthetic fixtures only (tiny C sources written here; no game code). The oracle is an image linked
from the same fixture sources, so equality is constructed, then broken on purpose.
"""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
PROJECT_ROOT = next(p for p in [HERE, *HERE.parents] if (p / 'recovery.json').exists())
sys.path.insert(0, str(PROJECT_ROOT / 'scripts'))
if (HERE / 'linked.py').exists():  # scratch location; installed copy lives in scripts/
    sys.path.insert(0, str(HERE))
import linked  # noqa: E402
from match import DEFAULT_TOOLCHAIN, digest  # noqa: E402

HAVE_TOOLCHAIN = (DEFAULT_TOOLCHAIN / 'bin/link.exe').is_file()

A = '''int fix_add(int a, int b) { return a + b; }
int fix_twice(int a) { return fix_add(a, a); }
int fix_counter;
void fix_bump(void) { ++fix_counter; }
'''
B = '''int fix_twice(int a);
int fix_table[4] = {1, 2, 3, 4};
int fix_entry(int i) { return fix_twice(fix_table[i & 3]); }
'''


class Fixture:
    def __init__(self, case):
        parent = (PROJECT_ROOT / 'work/test-fixtures').resolve()
        parent.mkdir(parents=True, exist_ok=True)
        tmp = tempfile.TemporaryDirectory(prefix='linked-', dir=parent)
        case.addCleanup(tmp.cleanup)
        case.addCleanup(patch.stopall)
        self.root = Path(tmp.name).resolve()
        patch.object(linked, 'ROOT', self.root).start()
        patch.object(linked, 'STATE', self.root / 'recovery.json').start()
        (self.root / 'src/fix').mkdir(parents=True)
        (self.root / 'work').mkdir()
        self.write('a.c', A)
        self.write('b.c', B)
        self.state = dict(schema_version=1, functions=[])
        self.save_state()
        lib = DEFAULT_TOOLCHAIN / 'lib'
        self.plan = dict(module='FIX.DLL', target='oracle/FIX.DLL', target_sha256='0' * 64,
                         toolchain=linked.verify_toolchain(DEFAULT_TOOLCHAIN), output_name='Fix.dll',
                         objects=[dict(source='src/fix/a.c', flags=['/O2']),
                                  dict(source='src/fix/b.c', flags=['/O2'])],
                         libraries=[dict(name=n, sha256=digest((lib / n).read_bytes()))
                                    for n in ('libcmt.lib', 'kernel32.lib')],
                         link_flags=['/DLL', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF', '/OPT:NOICF',
                                     '/EXPORT:fix_entry'])
        self.save_plan()

    def write(self, name, text):
        (self.root / 'src/fix' / name).write_text(text, newline='\n')

    def save_state(self):
        (self.root / 'recovery.json').write_text(json.dumps(self.state, indent=2) + '\n')

    def save_plan(self):
        (self.root / 'config/fix').mkdir(parents=True, exist_ok=True)
        (self.root / 'config/fix/link_plan.json').write_text(json.dumps(self.plan, indent=1))

    def oracle_from_build(self):
        """Link once with no rows, then adopt that image as the synthetic oracle and record rows."""
        oracle = self.root / 'oracle/FIX.DLL'
        oracle.parent.mkdir(exist_ok=True)
        oracle.write_bytes(b'placeholder')
        self.plan['target_sha256'] = digest(oracle.read_bytes())
        self.save_plan()
        report = linked.build('FIX.DLL', quiet=True)
        image = self.root / report['image']
        oracle.write_bytes(image.read_bytes())
        self.plan['target_sha256'] = digest(oracle.read_bytes())
        self.save_plan()
        base, symbols = linked.read_map(self.root / 'work/linked/fix/link.map')
        funcs = sorted((s['address'] - base, name, s['object']) for name, s in symbols.items()
                       if s['object'].startswith(('00_', '01_')) and s['section'] == 1)
        ends = [a for a, _, _ in funcs[1:]] + [None]
        for (rva, name, obj), end in zip(funcs, ends):
            if end is None:
                continue  # last first-party function: extent to the library is not a function size
            src = 'src/fix/a.c' if obj.startswith('00_') else 'src/fix/b.c'
            self.state['functions'].append(dict(
                id='fix.' + name.lstrip('_'), status='CODEGEN_SIMILAR', target='oracle/FIX.DLL',
                target_sha256=self.plan['target_sha256'], rva=hex(rva), size=end - rva, source=src,
                symbol=name, flags=['/O2'],
                proof=dict(scope=linked.MASKED_SCOPE, source_sha256=digest((self.root / src).read_bytes()),
                           masked_sha256='x', toolchain='msvc5_rtm')))
        self.save_state()
        return {r['symbol']: r for r in self.state['functions']}


class PlanRefusalTests(unittest.TestCase):
    def setUp(self):
        if not HAVE_TOOLCHAIN:
            self.skipTest('pinned toolchain not installed')
        self.f = Fixture(self)

    def refused(self, pattern, **change):
        plan = dict(self.f.plan, **change)
        with self.assertRaisesRegex(linked.LinkRefused, pattern):
            linked.validate_plan(plan)

    def test_valid_plan(self):
        linked.validate_plan(self.f.plan)

    def test_force_and_placement_link_flags_refused(self):
        for flag in ('/FORCE:UNRESOLVED', '/FORCE', '/ORDER:@order.txt', '/MERGE:.data=.text', '/BASE:0x10000000',
                     '/SECTION:.text,rwe', '/ALIGN:16', '/ENTRY:fix_entry'):
            self.refused('Link flag not allowed', link_flags=self.f.plan['link_flags'] + [flag])

    def test_compiler_steering_flags_refused(self):
        objs = [dict(source='src/fix/a.c', flags=['/O2', '/d1Binl'])]
        self.refused('Compiler flag not allowed', objects=objs)
        objs = [dict(source='src/fix/a.c', flags=['/O2', '/Fa'])]
        self.refused('Compiler flag not allowed', objects=objs)

    def test_sources_outside_canonical_trees_refused(self):
        (self.f.root / 'work/stub.c').write_text('int stub;\n')
        self.refused('outside', objects=[dict(source='work/stub.c', flags=['/O2'])])
        self.refused('repository-relative', objects=[dict(source='src/../work/stub.c', flags=['/O2'])])
        self.refused('Duplicate', objects=[dict(source='src/fix/a.c'), dict(source='src/fix/a.c')])

    def test_unpinned_library_refused(self):
        self.refused('pinned toolchain library', libraries=[dict(name='libcmt.lib')])
        plan = dict(self.f.plan, libraries=[dict(name='libcmt.lib', sha256='1' * 64)])
        with self.assertRaisesRegex(linked.LinkRefused, 'Library identity changed'):
            linked.libraries(plan)


class SourceScanTests(unittest.TestCase):
    def test_placement_and_assembly_constructs_refused(self):
        for text in (b'#pragma code_seg(".text$a")\n', b'#pragma data_seg("X")\n', b'#pragma comment(linker, "/include:x")\n',
                     b'void f(void) { __asm nop }\n', b'int __declspec(allocate(".x")) v;\n',
                     b'__declspec(naked) void g(void) {}\n', b'#pragma optimize("", off)\n'):
            with self.assertRaises(linked.LinkRefused):
                linked.scan_source('x.c', text)

    def test_ordinary_pragmas_allowed(self):
        linked.scan_source('x.c', b'#pragma pack(push, 1)\n#pragma warning(disable: 4101)\n#pragma once\n')

    def test_map_parsing_drops_ambiguous_statics(self):
        root = Path(tempfile.mkdtemp(dir=PROJECT_ROOT / 'work'))
        self.addCleanup(shutil.rmtree, root)
        m = root / 'x.map'
        m.write_text(' Preferred load address is 10000000\n'
                     ' 0001:00000000       _fix_add                   10001000 f   00_a.obj\n'
                     ' 0003:00000010       _s_local                   10003010     00_a.obj\n'
                     ' 0003:00000020       _s_local                   10003020     01_b.obj\n')
        base, symbols = linked.read_map(m)
        self.assertEqual(base, 0x10000000)
        self.assertEqual(symbols['_fix_add']['address'], 0x10001000)
        self.assertNotIn('_s_local', symbols)


class LinkTests(unittest.TestCase):
    def setUp(self):
        if not HAVE_TOOLCHAIN:
            self.skipTest('pinned toolchain not installed')
        self.f = Fixture(self)

    def test_unresolved_refused(self):
        self.f.write('b.c', B.replace('fix_twice(fix_table', 'fix_missing(fix_table').replace(
            'int fix_twice(int a);', 'int fix_missing(int a);'))
        oracle = self.f.root / 'oracle/FIX.DLL'
        oracle.parent.mkdir()
        oracle.write_bytes(b'x')
        self.f.plan['target_sha256'] = digest(b'x')
        self.f.save_plan()
        with self.assertRaisesRegex(linked.LinkRefused, 'unresolved'):
            linked.build('FIX.DLL', quiet=True)

    def test_duplicate_definition_refused(self):
        self.f.write('b.c', B + 'int fix_add(int a, int b) { return a - b; }\n')
        oracle = self.f.root / 'oracle/FIX.DLL'
        oracle.parent.mkdir()
        oracle.write_bytes(b'x')
        self.f.plan['target_sha256'] = digest(b'x')
        self.f.save_plan()
        with self.assertRaisesRegex(linked.LinkRefused, 'duplicates'):
            linked.build('FIX.DLL', quiet=True)

    def test_source_changed_during_compile_refused(self):
        oracle = self.f.root / 'oracle/FIX.DLL'
        oracle.parent.mkdir()
        oracle.write_bytes(b'x')
        self.f.plan['target_sha256'] = digest(b'x')
        self.f.save_plan()
        real = linked.compile_source

        def mutate(source, flags):
            result = real(source, flags)
            Path(source).write_text(Path(source).read_text() + '/* edit */\n', newline='\n')
            return result
        with patch.object(linked, 'compile_source', side_effect=mutate), \
                self.assertRaisesRegex(linked.LinkRefused, 'changed during compilation'):
            linked.build('FIX.DLL', quiet=True)

    def test_candidates_promote_verify_and_regression(self):
        rows = self.f.oracle_from_build()
        self.assertGreaterEqual(len(rows), 3)
        found = linked.candidates('FIX.DLL')
        self.assertEqual({r['id'] for r in found}, {r['id'] for r in rows.values()})
        # A strict row is never a candidate and cannot be promoted.
        state = json.loads(self.f.root.joinpath('recovery.json').read_text())
        state['functions'][0]['status'] = 'FUNCTION_MATCH'
        strict_id = state['functions'][0]['id']
        self.f.root.joinpath('recovery.json').write_text(json.dumps(state, indent=2) + '\n')
        self.assertNotIn(strict_id, {r['id'] for r in linked.candidates('FIX.DLL')})
        with self.assertRaisesRegex(linked.LinkRefused, 'FUNCTION_MATCH'):
            linked.promote('FIX.DLL', [strict_id])
        promote_ids = [r['id'] for r in state['functions'][1:] if r['source'] == 'src/fix/a.c'][:1]
        linked.promote('FIX.DLL', promote_ids)
        state = json.loads(self.f.root.joinpath('recovery.json').read_text())
        promoted = next(r for r in state['functions'] if r['id'] == promote_ids[0])
        self.assertEqual(promoted['status'], linked.STATUS)
        for key in ('scope', 'link_plan_sha256', 'source_sha256', 'sources_sha256', 'image_sha256',
                    'image_normalized_sha256', 'function_bytes_sha256', 'toolchain', 'libraries'):
            self.assertIn(key, promoted['proof'])
        rva = int(promoted['rva'], 16)
        import pe as pe_module
        self.assertEqual(promoted['proof']['function_bytes_sha256'],
                         hashlib.sha256(pe_module.PE(self.f.root / 'oracle/FIX.DLL').read_rva(rva, promoted['size'])).hexdigest())
        # Masked siblings of the same source now pin the promoted function.
        for r in state['functions']:
            if r['status'] == 'CODEGEN_SIMILAR' and r['source'] == promoted['source']:
                self.assertIn(promoted['symbol'], r['proof']['co_defined_functions'])
        linked.verify('FIX.DLL')
        # Changing the promoted function's source breaks verification.
        self.f.write('a.c', A.replace('return a + b;', 'return a + b + 1;'))
        with self.assertRaises(SystemExit):
            linked.verify('FIX.DLL')

    def test_concurrent_state_edit_refused(self):
        rows = self.f.oracle_from_build()
        ident = next(iter(rows.values()))['id']
        real = linked.build

        def build_then_edit(*args, **kw):
            report = real(*args, **kw)
            self.f.root.joinpath('recovery.json').write_text(self.f.root.joinpath('recovery.json').read_text() + ' ')
            return report
        with patch.object(linked, 'build', side_effect=build_then_edit), \
                self.assertRaisesRegex(linked.LinkRefused, 'recovery.json changed'):
            linked.promote('FIX.DLL', [ident])
        self.assertNotIn(linked.STATUS, self.f.root.joinpath('recovery.json').read_text())


if __name__ == '__main__':
    unittest.main()
