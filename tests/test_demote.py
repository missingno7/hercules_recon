import copy
import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import demote  # noqa: E402
from similar import SCOPE  # noqa: E402


def row(fid, status='FUNCTION_MATCH_RELOCATED', source='src/pc/a.cpp'):
    masked = dict(scope=SCOPE, source_sha256='s' * 64, masked_sha256='m' * 64, toolchain='msvc5_rtm',
                  co_defined_functions=['_old'])
    return dict(id=fid, status=status, source=source, symbol='_' + fid, flags=['/O2'], link_symbols={},
                proof=dict(scope='relocated', source_sha256='s' * 64, compiled_sha256='c' * 64,
                           bindings_path='evidence/b.json', bindings_sha256='b' * 64),
                prior_masked_proof=masked)


def review(**verdicts):
    return json.dumps(dict(rows={k: dict(verdict=v, missing_link='x' if v == 'DEMOTE' else '')
                                 for k, v in verdicts.items()})).encode()


class Withdraw(unittest.TestCase):
    def test_demote_restores_prior_masked_proof_and_keeps_history(self):
        state = dict(functions=[row('a'), row('b')])
        original = copy.deepcopy(state['functions'][0])
        groups = demote.withdraw(state, ['a'], 'evidence/r.json', review(a='DEMOTE'), '2026-10-27')
        a, b = state['functions']
        self.assertEqual(groups, {('src/pc/a.cpp', ('/O2',))})
        self.assertEqual(a['status'], 'CODEGEN_SIMILAR')
        self.assertEqual(a['proof'], original['prior_masked_proof'])
        self.assertNotIn('prior_masked_proof', a)
        history = a['withdrawn_acceptance'][0]
        self.assertEqual(history['proof'], original['proof'])
        self.assertEqual(history['status'], 'FUNCTION_MATCH_RELOCATED')
        self.assertEqual(history['review_path'], 'evidence/r.json')
        self.assertIn('--bindings evidence/b.json', history['restore'])
        self.assertEqual(b['status'], 'FUNCTION_MATCH_RELOCATED')  # unaffected rows untouched

    def test_keep_verdict_or_missing_reason_refuses(self):
        for verdicts in (dict(a='KEEP-STRICT'), dict()):
            state = dict(functions=[row('a')])
            with self.assertRaises(ValueError):
                demote.withdraw(state, ['a'], 'evidence/r.json', review(**verdicts), 'd')

    def test_only_relocated_rows_with_own_masked_proof(self):
        for broken in (row('a', status='FUNCTION_MATCH'), dict(row('a'), prior_masked_proof=None),
                       dict(row('a'), prior_masked_proof=dict(row('a')['prior_masked_proof'], source_sha256='t' * 64))):
            with self.assertRaises(ValueError):
                demote.withdraw(dict(functions=[broken]), ['a'], 'evidence/r.json', review(a='DEMOTE'), 'd')

    def test_unknown_or_duplicate_ids_refuse(self):
        for ids in (['z'], ['a', 'a'], []):
            with self.assertRaises(ValueError):
                demote.withdraw(dict(functions=[row('a')]), ids, 'evidence/r.json', review(a='DEMOTE', z='DEMOTE'), 'd')


if __name__ == '__main__':
    unittest.main()
