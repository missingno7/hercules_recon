"""reviewed-unit-delta-v3: reviewed added and changed function contributions (no compiler, no originals)."""
import copy
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import unit_refresh as refresh
import repin

TEXT, DEBUG_F, DATA = 0x60501020, 0x42101048, 0xC0300040


def definition(length, relocations, number=0, selection=1):
    return struct.pack('<IHHIHB3x', length, relocations, 0, 0, number, selection)


class Unit:
    """COFF-like unit: .data, then per function a /Gy .text COMDAT and an associative .debug$F."""

    def __init__(self, functions, externals=(), data=b'\0' * 4):
        self.sections, self.symbols = [], {}

        def symbol(**record):
            self.symbols[len(self.symbols) * 3] = record  # sparse indices, as with auxiliary records

        self.sections.append(dict(name='.data', data=data, size=len(data), relocations=[], flags=DATA))
        symbol(name='.data', value=0, section=1, type=0, storage=3, aux=[definition(len(data), 0, 0, 0)])
        for name, code, calls, fpo in functions:
            relocations = [dict(offset=offset, symbol=target, type=20) for offset, target in calls]
            self.sections.append(dict(name='.text', data=code, size=len(code), relocations=relocations, flags=TEXT))
            number = len(self.sections)
            symbol(name='.text', value=0, section=number, type=0, storage=3, aux=[definition(len(code), len(calls))])
            symbol(name=name, value=0, section=number, type=0x20, storage=2, aux=[])
            if fpo is not None:
                self.sections.append(dict(name='.debug$F', data=fpo, size=len(fpo), flags=DEBUG_F,
                                          relocations=[dict(offset=0, symbol=name, type=7)]))
                symbol(name='.debug$F', value=0, section=len(self.sections), type=0, storage=3,
                       aux=[definition(len(fpo), 1, number, 5)])
        for name in externals:
            symbol(name=name, value=0, section=0, type=0x20, storage=2, aux=[])

    def function(self, name):
        hits = [s for s in self.symbols.values() if s['name'] == name and s['section'] > 0 and s['type'] & 0x20]
        if len(hits) != 1:
            raise ValueError('Expected one defined symbol ' + name)
        return self.sections[hits[0]['section'] - 1]


ACCEPTED = ('_accepted', b'\xe8\0\0\0\0\xc3' + b'\x90' * 10, [(1, '_ext')], b'F' * 16)
PENDING = ('_pending', b'\x33\xc0\xe8\0\0\0\0\xc3' + b'\x90' * 8, [(3, '_ext')], b'P' * 16)
PENDING2 = ('_pending', b'\x31\xc0\xe8\0\0\0\0\xc3' + b'\x90' * 8, [(3, '_ext')], b'Q' * 16)
ADDED = ('_added', b'\xe8\0\0\0\0\xc3' + b'\x90' * 10, [(1, '_new_ext')], b'A' * 16)


def v3(**changes):
    declaration = dict(profile='reviewed-unit-delta-v3', removed=[], added=[], rebases=[], renames=[],
                       symbol_renames=[], added_functions=[], changed_functions=[], contributions={},
                       review='evidence/review.json', review_sha256='a' * 64)
    declaration.update(changes)
    return declaration


def pin(before, after, name):
    return dict(before_sha256=refresh.group_digest(before, refresh.contribution_group(before, name)) if before else None,
                after_sha256=refresh.group_digest(after, refresh.contribution_group(after, name)))


def undef(unit, name):
    return next(r for r in refresh.symbol_records(unit) if r['name'] == name)


class ChangedFunctionTests(unittest.TestCase):
    def setUp(self):
        self.before = Unit([ACCEPTED, PENDING], ['_ext'])
        self.after = Unit([ACCEPTED, PENDING2], ['_ext'])

    def declared(self):
        return v3(changed_functions=['_pending'], contributions={'_pending': pin(self.before, self.after, '_pending')})

    def test_changed_group_with_associative_section_passes(self):
        result = refresh.check_delta(self.before, self.after, self.declared(), {'_accepted'})
        self.assertEqual(result['changed_functions'], ['_pending'])
        self.assertEqual(refresh.contribution_group(self.after, '_pending'), [4, 5])

    def test_undeclared_or_mispinned_change_refuses(self):
        for profile in ('reviewed-unit-delta-v1', 'reviewed-unit-delta-v2'):
            declaration = {k: v for k, v in v3().items() if k in refresh.DELTA_KEYS[profile]}
            declaration['profile'] = profile
            with self.assertRaisesRegex(ValueError, 'Undeclared section'):
                refresh.check_delta(self.before, self.after, declaration, {'_accepted'})
        with self.assertRaisesRegex(ValueError, 'Undeclared section'):
            refresh.check_delta(self.before, self.after, v3(), {'_accepted'})
        for side in ('before_sha256', 'after_sha256'):
            declaration = self.declared()
            declaration['contributions']['_pending'][side] = '0' * 64
            with self.assertRaisesRegex(ValueError, 'contribution differs'):
                refresh.check_delta(self.before, self.after, declaration, {'_accepted'})
        unpinned = self.declared()
        unpinned['contributions'] = {}
        with self.assertRaisesRegex(ValueError, 'pinned contribution'):
            refresh.check_delta(self.before, self.after, unpinned, {'_accepted'})

    def test_accepted_functions_cannot_be_declared_or_change(self):
        declaration = v3(changed_functions=['_accepted'], contributions={'_accepted': pin(self.before, self.before, '_accepted')})
        with self.assertRaisesRegex(ValueError, 'accepted contribution'):
            refresh.check_delta(self.before, self.before, declaration, {'_accepted'})
        after = Unit([(ACCEPTED[0], b'\xe8\0\0\0\0\xcc' + b'\x90' * 10, ACCEPTED[2], ACCEPTED[3]), PENDING2], ['_ext'])
        declaration = v3(changed_functions=['_pending'], contributions={'_pending': pin(self.before, after, '_pending')})
        with self.assertRaisesRegex(ValueError, 'Accepted contribution changed'):
            refresh.check_delta(self.before, after, declaration, {'_accepted'})

    def test_other_sections_symbols_and_associations_stay_exact(self):
        mutations = [
            lambda u: u.sections[0].update(data=b'\1\0\0\0'),                       # undeclared data change
            lambda u: u.sections[3].update(name='.text$x'),                        # changed group renamed
            lambda u: u.sections[4].update(flags=DATA),                            # associative flags
            lambda u: u.symbols[max(u.symbols)].update(name='_other'),             # an external renamed
        ]
        for mutate in mutations:
            after = Unit([ACCEPTED, PENDING2], ['_ext'])
            mutate(after)
            with self.assertRaisesRegex(ValueError, 'Undeclared'):
                refresh.check_delta(self.before, after, v3(changed_functions=['_pending'],
                                    contributions={'_pending': pin(self.before, after, '_pending')}), {'_accepted'})
        retargeted = Unit([ACCEPTED, PENDING2], ['_ext'])
        fpo = next(i for i, s in retargeted.symbols.items() if s['name'] == '.debug$F' and s['section'] == 5)
        retargeted.symbols[fpo]['aux'] = [definition(16, 1, 2, 5)]  # associate with the accepted section
        with self.assertRaisesRegex(ValueError, 'section positions'):
            refresh.check_delta(self.before, retargeted, v3(changed_functions=['_pending'], contributions={
                '_pending': pin(self.before, retargeted, '_pending')}), {'_accepted'})


class AddedFunctionTests(unittest.TestCase):
    def setUp(self):
        self.before = Unit([ACCEPTED, PENDING], ['_ext'])
        # The new definition is inserted between existing ones: later section numbers shift by two.
        self.after = Unit([ACCEPTED, ADDED, PENDING], ['_ext', '_new_ext'])

    def declared(self):
        return v3(added_functions=['_added'], contributions={'_added': pin(None, self.after, '_added')},
                  added=[undef(self.after, '_new_ext')])

    def test_inserted_definition_with_new_external_passes(self):
        result = refresh.check_delta(self.before, self.after, self.declared(), {'_accepted'})
        self.assertEqual(result['added_functions'], ['_added'])
        self.assertEqual(refresh.declared_added_functions(self.declared()), ['_added'])

    def test_undeclared_or_mispinned_addition_refuses(self):
        with self.assertRaisesRegex(ValueError, 'section inventory'):
            refresh.check_delta(self.before, self.after, v3(added=[undef(self.after, '_new_ext')]), {'_accepted'})
        declaration = self.declared()
        declaration['contributions']['_added']['after_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'after contribution differs'):
            refresh.check_delta(self.before, self.after, declaration, {'_accepted'})
        declaration = self.declared()
        declaration['added'] = []
        with self.assertRaisesRegex(ValueError, 'symbol/auxiliary'):
            refresh.check_delta(self.before, self.after, declaration, {'_accepted'})

    def test_existing_definition_cannot_be_declared_added(self):
        declaration = v3(added_functions=['_pending'], contributions={'_pending': pin(None, self.before, '_pending')})
        with self.assertRaisesRegex(ValueError, 'must not exist before'):
            refresh.check_delta(self.before, self.before, declaration, {'_accepted'})

    def test_extra_sections_or_changes_beside_an_addition_refuse(self):
        extra = Unit([ACCEPTED, ADDED, PENDING], ['_ext', '_new_ext'])
        extra.sections.append(dict(name='.rdata', data=b'x', size=1, relocations=[], flags=DATA))
        with self.assertRaisesRegex(ValueError, 'section inventory'):
            refresh.check_delta(self.before, extra, self.declared(), {'_accepted'})
        changed = Unit([ACCEPTED, ADDED, PENDING2], ['_ext', '_new_ext'])
        declaration = v3(added_functions=['_added'], contributions={'_added': pin(None, changed, '_added')},
                         added=[undef(changed, '_new_ext')])
        with self.assertRaisesRegex(ValueError, 'Undeclared section'):
            refresh.check_delta(self.before, changed, declaration, {'_accepted'})

    def test_addition_and_change_together(self):
        after = Unit([ACCEPTED, ADDED, PENDING2], ['_ext', '_new_ext'])
        declaration = v3(added_functions=['_added'], changed_functions=['_pending'],
                         contributions={'_added': pin(None, after, '_added'), '_pending': pin(self.before, after, '_pending')},
                         added=[undef(after, '_new_ext')])
        result = refresh.check_delta(self.before, after, declaration, {'_accepted'})
        self.assertEqual((result['added_functions'], result['changed_functions']), (['_added'], ['_pending']))

    def test_rename_still_applies_with_additions(self):
        before = Unit([ACCEPTED, PENDING], ['_ext', '_old'])
        before.sections[3]['relocations'].append(dict(offset=8, symbol='_old', type=20))
        after = Unit([ACCEPTED, ADDED, PENDING], ['_ext', '_new', '_new_ext'])
        after.sections[5]['relocations'].append(dict(offset=8, symbol='_new', type=20))
        declaration = v3(added_functions=['_added'], contributions={'_added': pin(None, after, '_added')},
                         added=[undef(after, '_new_ext')],
                         symbol_renames=[dict(before=undef(before, '_old'), after=undef(after, '_new'))])
        result = refresh.check_delta(before, after, declaration, {'_accepted'})
        self.assertEqual(result['symbol_renames'], {'_old': '_new'})


class RepinConsistencyTests(unittest.TestCase):
    def test_plan_and_delta_must_declare_the_same_added_functions(self):
        declaration = v3(added_functions=['_added'])
        plan = dict(sources={'src/a.c': dict(added_functions=['_added'])})
        repin.check_refresh_additions(plan, {'src/a.c': declaration})
        for sources in ({'src/a.c': {}}, {'src/a.c': dict(added_functions=['_added', '_other'])}):
            with self.assertRaisesRegex(ValueError, 'disagree about added functions'):
                repin.check_refresh_additions(dict(sources=sources), {'src/a.c': declaration})
        v2 = {k: v for k, v in declaration.items() if k in refresh.DELTA_KEYS['reviewed-unit-delta-v2']}
        v2['profile'] = 'reviewed-unit-delta-v2'
        with self.assertRaisesRegex(ValueError, 'disagree about added functions'):
            repin.check_refresh_additions(plan, {'src/a.c': v2})
        repin.check_refresh_additions(dict(sources={'src/a.c': {}}), {'src/a.c': v2})


if __name__ == '__main__':
    unittest.main()
