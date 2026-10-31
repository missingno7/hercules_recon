"""reviewed-unit-delta-v2: identifier renames that change no generated code (no compiler, no originals)."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import unit_refresh as refresh

UNDEF_FUNCTION = dict(value=0, section=0, type=0x20, storage=2, aux=[])


class Coff:
    """Section 1: accepted '_accepted' calls `callee`; section 2: pending '_pending' calls `old` twice."""

    def __init__(self, old='_old', accepted='_accepted', callee='_callee'):
        self.sections = [
            dict(name='.text', data=b'\xe8\0\0\0\0\xc3', size=6, flags=0x60501020,
                 relocations=[dict(offset=1, symbol=callee, type=20)]),
            dict(name='.text', data=b'\xe8\0\0\0\0\xe8\0\0\0\0\xc3', size=11, flags=0x60501020,
                 relocations=[dict(offset=1, symbol=old, type=20), dict(offset=6, symbol=old, type=20)]),
        ]
        self.symbols = {0: dict(name=accepted, value=0, section=1, type=0x20, storage=2, aux=[]),
                        1: dict(name='_pending', value=0, section=2, type=0x20, storage=2, aux=[]),
                        2: dict(name=old, **UNDEF_FUNCTION)}
        if callee != old:
            self.symbols[3] = dict(name=callee, **UNDEF_FUNCTION)

    def function(self, name):
        hits = [s for s in self.symbols.values() if s['name'] == name and s['section'] > 0]
        if len(hits) != 1:
            raise ValueError('Expected one defined symbol ' + name)
        return self.sections[hits[0]['section'] - 1]


def v1(**changes):
    declaration = dict(profile='reviewed-unit-delta-v1', removed=[], added=[], rebases=[],
                       review='evidence/review.json', review_sha256='a' * 64)
    declaration.update(changes)
    return declaration


def v2(**changes):
    declaration = dict(profile='reviewed-unit-delta-v2', removed=[], added=[], rebases=[], renames=[],
                       symbol_renames=[], review='evidence/review.json', review_sha256='a' * 64)
    declaration.update(changes)
    return declaration


def record(coff, name):
    return next(r for r in refresh.symbol_records(coff) if r['name'] == name)


class InPlaceRenameTests(unittest.TestCase):
    def setUp(self):
        self.before, self.after = Coff(), Coff(old='_new')

    def declared(self):
        return v2(symbol_renames=[dict(before=record(self.before, '_old'), after=record(self.after, '_new'))])

    def test_in_place_undef_function_rename_passes(self):
        result = refresh.check_delta(self.before, self.after, self.declared(), {'_accepted'})
        self.assertEqual(result['symbol_renames'], {'_old': '_new'})
        self.assertEqual(result['protected_renamed_references'], [])
        self.assertEqual(self.after.sections[1]['relocations'][0]['symbol'], '_new')  # read-only

    def test_v1_and_undeclared_renames_refuse(self):
        for declaration in (v1(), v2()):
            with self.assertRaisesRegex(ValueError, 'Undeclared section'):
                refresh.check_delta(self.before, self.after, declaration, {'_accepted'})

    def test_profiles_keep_their_exact_key_sets(self):
        declaration = self.declared()
        declaration['profile'] = 'reviewed-unit-delta-v1'
        with self.assertRaisesRegex(ValueError, 'Invalid reviewed unit delta'):
            refresh.check_delta(self.before, self.after, declaration, {'_accepted'})
        declaration = self.declared()
        del declaration['renames']
        with self.assertRaisesRegex(ValueError, 'Invalid reviewed unit delta'):
            refresh.check_delta(self.before, self.after, declaration, {'_accepted'})

    def test_moved_retyped_colliding_or_duplicate_rename_refuses(self):
        moved = copy.deepcopy(self.after)
        moved.symbols = {0: moved.symbols[0], 1: moved.symbols[1], 2: moved.symbols[3], 3: moved.symbols[2]}
        with self.assertRaisesRegex(ValueError, 'ordered symbol-table position'):
            refresh.check_delta(self.before, moved, self.declared(), {'_accepted'})
        retyped = self.declared()
        retyped['symbol_renames'][0]['after'] = dict(retyped['symbol_renames'][0]['after'], type=0)
        with self.assertRaisesRegex(ValueError, 'change only the name'):
            refresh.check_delta(self.before, self.after, retyped, {'_accepted'})
        collide_after = Coff(old='_callee', callee='_other')
        declaration = v2(symbol_renames=[dict(before=record(self.before, '_old'),
                                              after=record(collide_after, '_callee'))])
        with self.assertRaisesRegex(ValueError, 'collides'):
            refresh.check_delta(self.before, collide_after, declaration, {'_accepted'})
        duplicate = self.declared()
        duplicate['symbol_renames'] *= 2
        with self.assertRaises(ValueError):
            refresh.check_delta(self.before, self.after, duplicate, {'_accepted'})

    def test_byte_fixup_or_accepted_change_beside_a_rename_refuses(self):
        for mutate in (lambda c: c.sections[1].update(data=b'\xe8\1\0\0\0\xe8\0\0\0\0\xc3'),
                       lambda c: c.sections[1]['relocations'].pop(),
                       lambda c: c.sections[1]['relocations'][0].update(type=6),
                       lambda c: c.sections[0].update(data=b'\xe8\0\0\0\0\xcc')):
            after = copy.deepcopy(self.after)
            mutate(after)
            with self.assertRaises(ValueError):
                refresh.check_delta(self.before, after, self.declared(), {'_accepted'})

    def test_accepted_reference_to_renamed_symbol_refuses_unless_allowed(self):
        before, after = Coff(callee='_old'), Coff(old='_new', callee='_new')
        declaration = v2(symbol_renames=[dict(before=record(before, '_old'), after=record(after, '_new'))])
        with self.assertRaisesRegex(ValueError, 'references a renamed symbol'):
            refresh.check_delta(before, after, declaration, {'_accepted'})
        result = refresh.check_delta(before, after, declaration, {'_accepted'}, allow_protected_renames=True)
        self.assertEqual(result['protected_renamed_references'],
                         [dict(function='_accepted', offset=1, before_symbol='_old', after_symbol='_new')])

    def test_accepted_definition_cannot_be_renamed(self):
        before, after = Coff(), Coff(accepted='_renamed')
        declaration = v2(symbol_renames=[dict(before=record(before, '_accepted'), after=record(after, '_renamed'))])
        with self.assertRaisesRegex(ValueError, 'touches an accepted contribution'):
            refresh.check_delta(before, after, declaration, {'_accepted'})


class RelocationRenameTests(unittest.TestCase):
    """Fallback when the new external does not keep the old ordered position."""

    def setUp(self):
        self.before = Coff()
        self.after = Coff(old='_new')
        self.after.symbols = {0: self.after.symbols[0], 1: self.after.symbols[1], 2: self.after.symbols[3],
                              3: self.after.symbols[2]}
        self.declaration = v2(removed=[record(self.before, '_old')], added=[record(self.after, '_new')],
                              renames=[dict(function='_pending', offset=off, type=20, before_symbol='_old',
                                            after_symbol='_new') for off in (1, 6)])

    def test_enumerated_rel32_renames_with_function_undef_delta_pass(self):
        result = refresh.check_delta(self.before, self.after, self.declaration, {'_accepted'})
        self.assertEqual(result['renames'], 2)

    def test_partial_wrong_type_or_accepted_rename_refuses(self):
        partial = copy.deepcopy(self.declaration)
        partial['renames'].pop()
        with self.assertRaises(ValueError):
            refresh.check_delta(self.before, self.after, partial, {'_accepted'})
        wrong = copy.deepcopy(self.declaration)
        wrong['renames'][0]['type'] = 6
        with self.assertRaisesRegex(ValueError, 'Rename differs'):
            refresh.check_delta(self.before, self.after, wrong, {'_accepted'})
        accepted = copy.deepcopy(self.declaration)
        accepted['renames'][0]['function'] = '_accepted'
        with self.assertRaisesRegex(ValueError, 'accepted contribution'):
            refresh.check_delta(self.before, self.after, accepted, {'_accepted'})

    def test_addend_bytes_stay_compared(self):
        after = copy.deepcopy(self.after)
        after.sections[1]['data'] = b'\xe8\4\0\0\0\xe8\0\0\0\0\xc3'
        with self.assertRaisesRegex(ValueError, 'Undeclared section'):
            refresh.check_delta(self.before, after, self.declaration, {'_accepted'})

    def test_function_undef_records_need_v2(self):
        declaration = v1(removed=self.declaration['removed'], added=self.declaration['added'])
        with self.assertRaises(ValueError):
            refresh.check_delta(self.before, self.after, declaration, {'_accepted'})


if __name__ == '__main__':
    unittest.main()
