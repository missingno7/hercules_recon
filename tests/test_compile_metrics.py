"""Opt-in historical compiler invocation metrics, using a fake subprocess."""
import hashlib
import json
import os
from pathlib import Path
import sys
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
import unittest
from unittest.mock import patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "scripts"))
import match


class CompileMetricsTests(unittest.TestCase):
    def setUp(self):
        self.repo_root = Path(match.__file__).resolve().parents[1]
        work = self.repo_root / "work"
        work.mkdir(exist_ok=True)
        self.fixture = tempfile.TemporaryDirectory(prefix="compile-metrics-fixture-", dir=work)
        self.addCleanup(self.fixture.cleanup)
        self.root = Path(self.fixture.name)
        self.source = self.root / "fixture.c"
        self.source.write_bytes(b"int fixture(void) { return 7; }\n")
        self.root_patch = patch.object(match, "ROOT", self.root)
        self.root_patch.start()
        self.addCleanup(self.root_patch.stop)
        self.toolchain_patch = patch.object(match, "verify_toolchain", return_value="test-toolchain-v1")
        self.toolchain_patch.start()
        self.addCleanup(self.toolchain_patch.stop)
        self.toolchain = Path("test-toolchain")

    def metrics_directory(self):
        holder = tempfile.TemporaryDirectory(prefix="compile-metrics-events-", dir=self.repo_root / "work")
        self.addCleanup(holder.cleanup)
        return Path(holder.name) / "events"

    @staticmethod
    def fake_run(command, **kwargs):
        obj = Path(next(arg[3:] for arg in command if arg.startswith("/Fo")))
        obj.write_bytes(b"fixture object")
        return subprocess.CompletedProcess(command, 0, "compiler output", "")

    def events(self, directory):
        return [json.loads(path.read_text(encoding="utf-8")) for path in sorted(directory.glob("*.json"))]

    def test_success_records_source_command_and_timing(self):
        directory = self.metrics_directory()
        source_hash = hashlib.sha256(self.source.read_bytes()).hexdigest()
        with patch.dict(os.environ, {"HERCULES_COMPILE_METRICS_DIR": str(directory)}), \
                patch.object(match.subprocess, "run", side_effect=self.fake_run) as run:
            obj, command = match.compile_source(self.source, ["/O2", "/DTEST=1"], self.toolchain)

        run.assert_called_once()
        self.assertEqual(obj.read_bytes(), b"fixture object")
        [event] = self.events(directory)
        self.assertEqual(event["command"], command)
        self.assertEqual(event["flags"], ["/O2", "/DTEST=1"])
        self.assertEqual(event["source_path"], str(self.source.resolve()))
        self.assertEqual(event["source_sha256"], source_hash)
        self.assertEqual(event["toolchain_id"], "test-toolchain-v1")
        self.assertEqual(event["returncode"], 0)
        self.assertIsNone(event["error"])
        self.assertTrue(event["started_utc"].endswith("+00:00"))
        self.assertTrue(event["ended_utc"].endswith("+00:00"))
        self.assertGreaterEqual(event["elapsed_seconds"], 0)
        event_files = list(directory.glob("*.json"))
        self.fixture.cleanup()
        self.assertTrue(all(path.is_file() for path in event_files))

    def test_nonzero_exit_records_failure_before_raising(self):
        directory = self.metrics_directory()

        def failed_run(command, **kwargs):
            return subprocess.CompletedProcess(command, 23, "out", "bad input")

        with patch.dict(os.environ, {"HERCULES_COMPILE_METRICS_DIR": str(directory)}), \
                patch.object(match.subprocess, "run", side_effect=failed_run) as run:
            with self.assertRaisesRegex(RuntimeError, r"failed \(23\)"):
                match.compile_source(self.source, ["/O2"], self.toolchain)

        run.assert_called_once()
        [event] = self.events(directory)
        self.assertEqual(event["returncode"], 23)
        self.assertEqual(event["error"], "compiler exited with status 23")

    def test_subprocess_exception_records_attempt_and_reraises(self):
        directory = self.metrics_directory()
        with patch.dict(os.environ, {"HERCULES_COMPILE_METRICS_DIR": str(directory)}), \
                patch.object(match.subprocess, "run", side_effect=OSError("fixture launch failure")):
            with self.assertRaisesRegex(OSError, "fixture launch failure"):
                match.compile_source(self.source, ["/O2"], self.toolchain)

        [event] = self.events(directory)
        self.assertIsNone(event["returncode"])
        self.assertEqual(event["error"], "OSError: fixture launch failure")

    def test_concurrent_calls_create_distinct_event_files(self):
        directory = self.metrics_directory()
        with patch.dict(os.environ, {"HERCULES_COMPILE_METRICS_DIR": str(directory)}), \
                patch.object(match.subprocess, "run", side_effect=self.fake_run) as run:
            with ThreadPoolExecutor(max_workers=2) as pool:
                results = list(pool.map(lambda _: match.compile_source(self.source, ["/O2"], self.toolchain), range(2)))

        self.assertEqual(len(results), 2)
        self.assertEqual(run.call_count, 2)
        files = list(directory.glob("*.json"))
        self.assertEqual(len(files), 2)
        self.assertEqual(len({path.name for path in files}), 2)
        self.assertEqual(len(self.events(directory)), 2)

    def test_invalid_destination_is_rejected_before_subprocess(self):
        invalid = self.repo_root / "tests" / "invalid-compile-metrics"
        with patch.dict(os.environ, {"HERCULES_COMPILE_METRICS_DIR": str(invalid)}), \
                patch.object(match.subprocess, "run") as run:
            with self.assertRaisesRegex(ValueError, "under repository work/ or build/"):
                match.compile_source(self.source, ["/O2"], self.toolchain)
        run.assert_not_called()

    def test_unset_metrics_writes_nothing(self):
        work = self.repo_root / "work"
        existing = {path.resolve() for path in work.glob("compile-metrics-events-*/events/*.json")}
        with patch.dict(os.environ):
            os.environ.pop("HERCULES_COMPILE_METRICS_DIR", None)
            with patch.object(match.subprocess, "run", side_effect=self.fake_run) as run:
                match.compile_source(self.source, ["/O2"], self.toolchain)
        run.assert_called_once()
        self.assertEqual({path.resolve() for path in work.glob("compile-metrics-events-*/events/*.json")}, existing)


if __name__ == "__main__":
    unittest.main()
