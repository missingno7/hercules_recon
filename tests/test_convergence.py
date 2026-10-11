"""Prevent diagnostic coverage and old blocker notes from inventing strict gain."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import convergence


def row(rva, status, size=16, scope=None):
    return dict(target='oracle/TEST.DLL', rva=hex(rva), size=size, status=status,
                proof=dict(scope=scope))


class CoverageTests(unittest.TestCase):
    def test_status_partition_and_untracked_are_separate(self):
        rows = [row(0x1000, 'FUNCTION_MATCH'), row(0x1010, 'CODEGEN_SIMILAR', scope=convergence.SCOPE),
                row(0x1020, 'CODEGEN_SIMILAR'), row(0x1030, 'RECONSTRUCTED')]
        mapping = dict(module='TEST.DLL', functions=[dict(rva=hex(r), size=16) for r in range(0x1000, 0x1050, 16)])
        report = convergence.build(dict(functions=rows), [mapping])
        metrics = report['modules']['TEST.DLL']
        for c in ('strict', 'masked', 'legacy_similar', 'reconstructed_non_strict', 'untracked_map_leads'):
            self.assertEqual(metrics[c], dict(functions=1, bytes=16))

    def test_no_map_does_not_mean_zero_missing(self):
        metrics = convergence.build(dict(functions=[row(0x1000, 'FUNCTION_MATCH')]))['modules']['TEST.DLL']
        self.assertIsNone(metrics['untracked_map_leads'])

    def test_closed_peers_removed_from_codegen_ceiling(self):
        rows = [row(0x1000, 'FUNCTION_MATCH'), row(0x1010, 'CODEGEN_SIMILAR', scope=convergence.SCOPE)]
        family = dict(id='old-note', kind='question', members=[dict(module='TEST.DLL', rva=hex(r)) for r in (0x1000, 0x1010, 0x1020)],
                      evidence=['note'], next_discriminator='new evidence')
        report = convergence.build(dict(functions=rows), annotations=dict(families=[family]))
        self.assertEqual(report['research_families'][0]['pending_codegen_byte_ceiling'], 0)
        self.assertEqual(report['research_families'][0]['unmeasured_members'], 1)

    def test_stale_and_overlapping_maps_rejected(self):
        state = dict(functions=[row(0x1010, 'FUNCTION_MATCH')])
        for spans in ([dict(rva='0x1010', size=32)], [dict(rva='0x1000', size=32)],
                      [dict(rva='0x1020', size=32), dict(rva='0x1030', size=16)]):
            with self.subTest(spans=spans), self.assertRaises(ValueError):
                convergence.build(state, [dict(module='TEST.DLL', functions=spans)])

    def test_wrong_module_map_cannot_supply_other_modules_missing_bytes(self):
        report = convergence.build(dict(functions=[row(0x1000, 'FUNCTION_MATCH')]),
                                   [dict(module='OTHER.DLL', functions=[dict(rva='0x1000', size=32)])])
        self.assertIsNone(report['modules']['TEST.DLL']['untracked_map_leads'])
        self.assertEqual(report['modules']['OTHER.DLL']['untracked_map_leads']['bytes'], 32)


if __name__ == '__main__':
    unittest.main()
