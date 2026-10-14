from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[2] / "tools"
sys.path.insert(0, str(TOOLS))
import run_real_mesh_geometry as runner


class RunnerValidationTests(unittest.TestCase):
    def _valid_linked_pair(self):
        report = {
            "result": "EXECUTED",
            "hashes": runner.EXPECTED_HASHES,
        }
        report_bytes = (json.dumps(report, sort_keys=True) + "\n").encode("utf-8")
        manifest = {
            "schema": "hercules.eng1-real-resource-captured-geometry.v2",
            "provenance": {
                "fixture_report_sha256": runner.sha(report_bytes),
                "source_fixture_report": "native_real_map_report.json",
                "exe_hash": runner.EXPECTED_HASHES["HERCULES.EXE"],
                "eng1_hash": runner.EXPECTED_HASHES["ENG1.DLL"],
                "archive_hash": runner.EXPECTED_HASHES["HERCULES.FS"],
                "asset_hash": runner.EXPECTED_HASHES["MAP_PSX.BIN"],
            },
        }
        return report_bytes, (json.dumps(manifest, sort_keys=True) + "\n").encode("utf-8")

    def test_fresh_report_manifest_hash_link(self):
        report_bytes, manifest_bytes = self._valid_linked_pair()
        report, manifest = runner.validate_report_manifest_link(report_bytes, manifest_bytes)
        self.assertEqual(report["result"], "EXECUTED")
        self.assertEqual(manifest["schema"], "hercules.eng1-real-resource-captured-geometry.v2")

    def test_stale_manifest_is_rejected_before_native_work(self):
        report_bytes, manifest_bytes = self._valid_linked_pair()
        manifest = json.loads(manifest_bytes)
        manifest["provenance"]["fixture_report_sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "SHA link mismatch"):
            runner.validate_report_manifest_link(
                report_bytes, (json.dumps(manifest) + "\n").encode("utf-8"))

    def test_unexpected_original_hash_is_rejected(self):
        report_bytes, manifest_bytes = self._valid_linked_pair()
        report = json.loads(report_bytes)
        report["hashes"]["ENG1.DLL"] = "0" * 64
        changed_report = (json.dumps(report) + "\n").encode("utf-8")
        manifest = json.loads(manifest_bytes)
        manifest["provenance"]["fixture_report_sha256"] = runner.sha(changed_report)
        changed_manifest = (json.dumps(manifest, sort_keys=True) + "\n").encode("utf-8")
        with self.assertRaisesRegex(ValueError, "hashes differ"):
            runner.validate_report_manifest_link(changed_report, changed_manifest)

    def test_bad_input_files_are_rejected_without_subprocess(self):
        scratch = Path(__file__).resolve().parents[3] / "work" / "dropin_mesh"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            root = Path(directory)
            exe = root / "HERCULES.EXE"
            eng1 = root / "ENG1.DLL"
            archive = root / "HERCULES.FS"
            exe.write_bytes(b"test only")
            eng1.write_bytes(b"test only")
            archive.write_bytes(b"test only")
            with self.assertRaisesRegex(ValueError, "HERCULES.EXE identity mismatch"):
                runner.verify_original_inputs(exe, eng1, archive)


if __name__ == "__main__":
    unittest.main()
