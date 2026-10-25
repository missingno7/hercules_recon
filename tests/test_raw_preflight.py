"""Compiler-free readiness checks for recorded mixed-unit RAW candidates."""
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import promote
import reference_preflight
import similar
from test_matching import CODE, make_coff, make_pe, workspace_temporary_directory


class RawPreflightTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory('raw-preflight-')
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'src').mkdir()
        self.source = self.root / 'src/unit.c'
        self.header = self.root / 'src/unit.h'
        self.source.write_bytes(b'int sample(void) { return 0; }\n')
        self.header.write_bytes(b'/* dependency */\n')
        self.target = self.root / 'TEST.DLL'
        self.other_target = self.root / 'OTHER.DLL'
        make_pe(self.target)
        self.other_target.write_bytes(self.target.read_bytes())
        self.obj = self.root / 'unit.obj'
        make_coff(self.obj, extra_function=True)
        self.toolchain = self.root / 'toolchain'
        (self.toolchain / 'bin').mkdir(parents=True)
        (self.toolchain / 'include').mkdir()
        (self.root / 'toolchains').mkdir()
        (self.root / 'toolchains/manifest.json').write_text('{}')
        self.state = self.root / 'recovery.json'
        self.addCleanup(patch.stopall)
        for module in (match, promote, reference_preflight):
            patch.object(module, 'ROOT', self.root).start()
        patch.object(match, 'verify_toolchain', return_value='fixture-toolchain').start()
        patch.object(promote, 'STATE', self.state).start()

        self.candidate = self._candidate()
        self.state.write_text(json.dumps({'functions': [self.candidate]}))
        self.report = {'results': [self._result(self.candidate)]}

    def _candidate(self, **changes):
        row = dict(id='raw-candidate', source='src/unit.c', target='TEST.DLL',
                   target_sha256=match.digest(self.target.read_bytes()), rva='0x1000',
                   size=len(CODE), symbol='_sample', flags=['/O2'], status='CODEGEN_SIMILAR',
                   proof=dict(scope=similar.SCOPE, source_sha256=match.digest(self.source.read_bytes()),
                              masked_sha256=match.digest(CODE), co_defined_functions=['_extra']))
        row.update(changes)
        return row

    def _result(self, row):
        context = match.compile_inputs(self.source, row['flags'], self.toolchain)
        command = [str(self.toolchain / 'bin/cl.exe'), '/nologo', '/c', '/Gy', *row['flags'],
                   '/Fo' + str(self.obj.resolve()), str(self.source.resolve())]
        return dict(id=row['id'], object=str(self.obj.resolve()),
                    object_sha256=match.digest(self.obj.read_bytes()),
                    source_sha256=match.digest(self.source.read_bytes()),
                    target_sha256=match.digest(CODE), compiled_sha256=match.digest(CODE),
                    exact=True, toolchain='fixture-toolchain', command=command,
                    compile_inputs={str(path): sha for path, sha in context.items()})

    def _run(self, rows=None, report=None, preserve_reconstructed=False, original_root=None):
        with patch.object(match, 'compile_source', side_effect=AssertionError('preflight must not compile')):
            return reference_preflight.preflight_raw(rows or [self.candidate],
                                                     self.report if report is None else report,
                                                     preserve_reconstructed=preserve_reconstructed,
                                                     toolchain=self.toolchain,
                                                     original_root=original_root)

    @staticmethod
    def messages(result):
        return (result['errors'] + [message for item in result['functions'] for message in item['errors']] +
                [message for unit in result.get('units', []) for message in unit['errors']])

    def test_exact_raw_candidate_and_complete_unit_inventory_pass(self):
        result = self._run()
        self.assertTrue(result['ok'], result)
        self.assertEqual(result['compiler_calls'], 0)
        self.assertEqual(result['units'][0]['mapped_symbols'], ['_sample'])
        self.assertEqual(result['units'][0]['pinned_symbols'], ['_extra'])
        self.assertEqual(result['units'][0]['emitted_symbols'], ['_extra', '_sample'])
        self.assertEqual(result['units'][0]['unit_inventory_sha256'], match.unit_inventory(self.obj))

    def test_preserved_reconstructed_peer_must_match_full_object_contribution(self):
        peer = dict(id='reconstructed', source='src/unit.c', target='OTHER.DLL',
                    target_sha256=match.digest(self.other_target.read_bytes()), rva='0x1000',
                    size=len(CODE), symbol='_extra', flags=['/O2'], status='RECONSTRUCTED',
                    proof=dict(scope=promote.RECONSTRUCTED_PEER_SCOPE,
                               compiled_size=len(CODE), compiled_sha256=match.digest(CODE),
                               source_sha256=match.digest(self.source.read_bytes()),
                               toolchain='fixture-toolchain'))
        peer['proof']['compiled_sha256'] = '0' * 64
        self.state.write_text(json.dumps({'functions': [self.candidate, peer]}))
        result = self._run(preserve_reconstructed=True)
        self.assertFalse(result['ok'])
        self.assertTrue(any('reconstructed peer complete contribution' in message
                            for message in self.messages(result)), result)
        self.assertEqual(result['compiler_calls'], 0)

    def test_missing_pinned_co_defined_function_blocks_same_tu_inventory(self):
        candidate = copy.deepcopy(self.candidate)
        candidate['proof']['co_defined_functions'] = ['_ghost']
        self.state.write_text(json.dumps({'functions': [candidate]}))
        result = self._run(rows=[candidate])
        self.assertFalse(result['ok'])
        self.assertTrue(any('complete emitted function inventory' in message
                            for message in self.messages(result)), result)

    def test_missing_mapped_function_blocks_same_tu_inventory(self):
        missing = dict(id='mapped-missing', source='src/unit.c', target='OTHER.DLL',
                       target_sha256=match.digest(self.other_target.read_bytes()), rva='0x1000',
                       size=len(CODE), symbol='_missing', flags=['/O2'], status='FUNCTION_MATCH',
                       proof=dict(scope='complete relocation-free function COFF contribution',
                                  source_sha256=match.digest(self.source.read_bytes()),
                                  compiled_sha256=match.digest(CODE), target_sha256=match.digest(CODE),
                                  toolchain='fixture-toolchain'))
        self.state.write_text(json.dumps({'functions': [self.candidate, missing]}))
        result = self._run()
        self.assertFalse(result['ok'])
        self.assertTrue(any('complete emitted function inventory' in message
                            for message in self.messages(result)), result)

    def test_relocated_peer_without_independent_reference_metadata_is_reported(self):
        peer = dict(id='relocated', source='src/unit.c', target='OTHER.DLL',
                    target_sha256=match.digest(self.other_target.read_bytes()), rva='0x1000',
                    size=len(CODE), symbol='_extra', flags=['/O2'], status='FUNCTION_MATCH_RELOCATED',
                    proof=dict(scope='complete independently reviewed references',
                               source_sha256=match.digest(self.source.read_bytes()),
                               compiled_sha256=match.digest(CODE), target_sha256=match.digest(CODE),
                               toolchain='fixture-toolchain', unit_inventory_sha256=match.unit_inventory(self.obj),
                               context_sha256=match.context_proof(match.compile_inputs(
                                   self.source, ['/O2'], self.toolchain))))
        self.state.write_text(json.dumps({'functions': [self.candidate, peer]}))
        result = self._run()
        self.assertFalse(result['ok'])
        self.assertIn(dict(id='relocated', status='missing'), result['units'][0]['independent_references'])
        self.assertTrue(any('missing independent reference metadata' in message
                            for message in self.messages(result)), result)

    def test_stale_header_object_profile_and_target_are_rejected(self):
        self.source.write_bytes(b'#include "unit.h"\nint sample(void) { return 0; }\n')
        self.candidate['proof']['source_sha256'] = match.digest(self.source.read_bytes())
        self.state.write_text(json.dumps({'functions': [self.candidate]}))
        self.report = {'results': [self._result(self.candidate)]}
        original_header = self.header.read_bytes()
        self.header.write_bytes(original_header + b'changed\n')
        result = self._run()
        self.assertFalse(result['ok'])
        self.assertTrue(any('closure differs' in message for message in self.messages(result)), result)
        self.header.write_bytes(original_header)

        self.obj.write_bytes(self.obj.read_bytes() + b'stale')
        result = self._run()
        self.assertFalse(result['ok'])
        self.assertTrue(any('object hash is stale' in message or 'identity differs' in message
                            for message in self.messages(result)), result)
        self.obj.write_bytes(self.obj.read_bytes()[:-5])

        altered = copy.deepcopy(self.report)
        altered['results'][0]['command'][4] = '/O1'
        result = self._run(report=altered)
        self.assertFalse(result['ok'])
        self.assertTrue(any('unique result' in message or 'profile' in message
                            for message in self.messages(result)), result)

        self.target.write_bytes(self.target.read_bytes() + b'changed')
        result = self._run()
        self.assertFalse(result['ok'])
        self.assertTrue(any('recorded identity differs' in message or 'native module hash' in message
                            for message in self.messages(result)), result)

    def test_unknown_selection_and_default_reconstructed_peer_still_block(self):
        unknown = dict(self.candidate, id='not-in-ledger')
        result = self._run(rows=[unknown])
        self.assertFalse(result['ok'])
        self.assertTrue(any('unknown recovery IDs' in message for message in self.messages(result)), result)

        peer = dict(id='reconstructed', source='src/unit.c', target='OTHER.DLL',
                    target_sha256=match.digest(self.other_target.read_bytes()), rva='0x1000',
                    size=len(CODE), symbol='_extra', flags=['/O2'], status='RECONSTRUCTED',
                    proof=dict(scope=promote.RECONSTRUCTED_PEER_SCOPE,
                               compiled_size=len(CODE), compiled_sha256=match.digest(CODE),
                               source_sha256=match.digest(self.source.read_bytes()),
                               toolchain='fixture-toolchain'))
        self.state.write_text(json.dumps({'functions': [self.candidate, peer]}))
        result = self._run()
        self.assertFalse(result['ok'])
        self.assertTrue(any('supported prior proof' in message for message in self.messages(result)), result)


if __name__ == '__main__':
    unittest.main()
