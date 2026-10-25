"""Synthetic CLI-routing tests for compiler-free match.py preflight modes."""
import contextlib
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "scripts"))
import match
import reference_preflight
from test_matching import workspace_temporary_directory


class MatchPreflightCliTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory("match-preflight-cli-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.rows = [
            self._row("preflight.alpha", "CODEGEN_SIMILAR"),
            self._row("preflight.beta", "FUNCTION_MATCH_RAW"),
            self._row("preflight.gamma", "RECONSTRUCTED"),
        ]
        self.state = self.root / "recovery.json"
        self.state.write_text(json.dumps({"functions": self.rows}), encoding="utf-8")
        self.bindings = self.root / "bindings.json"
        self.write_bindings([row["id"] for row in self.rows])
        self.objects = self.root / "objects.json"
        self.objects.write_text('{"results": []}\n', encoding="utf-8")
        patch.object(match, "ROOT", self.root).start()
        self.addCleanup(patch.stopall)

    @staticmethod
    def _row(row_id, status):
        return dict(id=row_id, status=status, source="src/unit.c", symbol="_" + row_id.replace(".", "_"),
                    target="TEST.DLL", target_sha256="a" * 64, rva="0x1000", size=3,
                    flags=["/O2"], proof={"note": "synthetic diagnostic row"})

    def write_bindings(self, ids):
        self.bindings.write_text(json.dumps({"functions": {row_id: {} for row_id in ids}}), encoding="utf-8")

    @staticmethod
    def _diagnostic_result(rows):
        return dict(ok=True, compiler_calls=0, errors=[],
                    functions=[dict(id=row["id"], errors=[]) for row in rows])

    def run_cli(self, arguments, *, reference_mock=None, raw_mock=None):
        stdout, stderr, code = self.run_cli_outcome(arguments, reference_mock=reference_mock,
                                                    raw_mock=raw_mock)
        self.assertEqual(code, 0)
        return stdout, stderr

    def run_cli_outcome(self, arguments, *, reference_mock=None, raw_mock=None):
        stdout, stderr = io.StringIO(), io.StringIO()
        with patch.object(match, "compile_source", side_effect=AssertionError("preflight CLI must not compile")), \
                patch.object(reference_preflight, "preflight", reference_mock or Mock()), \
                patch.object(reference_preflight, "preflight_raw", raw_mock or Mock()), \
                patch.object(sys, "argv", ["match.py", *(str(arg) for arg in arguments)]), \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            try:
                match.main()
            except SystemExit as exited:
                return stdout.getvalue(), stderr.getvalue(), exited.code
        return stdout.getvalue(), stderr.getvalue(), 0

    def run_cli_error(self, arguments):
        stdout, stderr = io.StringIO(), io.StringIO()
        reference_mock, raw_mock = Mock(), Mock()
        with patch.object(match, "compile_source", side_effect=AssertionError("invalid CLI must not compile")), \
                patch.object(reference_preflight, "preflight", reference_mock), \
                patch.object(reference_preflight, "preflight_raw", raw_mock), \
                patch.object(sys, "argv", ["match.py", *(str(arg) for arg in arguments)]), \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            with self.assertRaises(SystemExit) as exited:
                match.main()
        reference_mock.assert_not_called()
        raw_mock.assert_not_called()
        self.assertEqual(exited.exception.code, 2)
        return stderr.getvalue()

    def test_reference_preflight_routes_only_explicit_multi_id_selection(self):
        bindings_before = self.bindings.read_bytes()
        state_before = self.state.read_bytes()
        mock = Mock(side_effect=lambda rows, *args: self._diagnostic_result(rows))
        stdout, _ = self.run_cli(
            ["preflight", "--bindings", self.bindings, "--objects", self.objects,
             "--select", "preflight.gamma", "preflight.alpha"],
            reference_mock=mock)

        mock.assert_called_once()
        rows, bindings, objects, _, original_root = mock.call_args.args
        self.assertEqual([row["id"] for row in rows], ["preflight.alpha", "preflight.gamma"])
        self.assertEqual(bindings, self.bindings)
        self.assertEqual(objects, self.objects)
        self.assertIsNone(original_root)
        self.assertIn("Diagnostic preflight: PASS; 2 functions; 0 compiler calls; no acceptance proof", stdout)
        self.assertEqual(self.state.read_bytes(), state_before)
        self.assertEqual(self.bindings.read_bytes(), bindings_before)

    def test_unknown_and_duplicate_selected_ids_fail_before_preflight(self):
        message = self.run_cli_error(
            ["preflight", "--bindings", self.bindings, "--objects", self.objects,
             "--select", "preflight.alpha", "missing.id"])
        self.assertIn("--select contains unknown recovery IDs", message)

        message = self.run_cli_error(
            ["preflight", "--bindings", self.bindings, "--objects", self.objects,
             "--select", "preflight.alpha", "preflight.alpha"])
        self.assertIn("--select requires preflight, unique IDs and no positional ID", message)

    def test_selected_reference_id_missing_from_bindings_fails_closed(self):
        self.write_bindings(["preflight.alpha"])
        state_before = self.state.read_bytes()
        message = self.run_cli_error(
            ["preflight", "--bindings", self.bindings, "--objects", self.objects,
             "--select", "preflight.alpha", "preflight.beta"])
        self.assertIn("Selected functions are missing reviewed bindings", message)
        self.assertEqual(self.state.read_bytes(), state_before)

    def test_reference_mode_requires_bindings_and_rejects_legacy_raw_only_flag(self):
        message = self.run_cli_error(
            ["preflight", "--objects", self.objects, "--select", "preflight.alpha"])
        self.assertIn("reference preflight requires --bindings", message)

        message = self.run_cli_error(
            ["preflight", "--objects", self.objects, "--raw-only", "--select", "preflight.alpha"])
        self.assertIn("preflight requires --objects, forbids --source/--obj/--raw-only", message)

    def test_preflight_only_flags_are_rejected_on_other_commands(self):
        cases = [
            (["references", "--select", "preflight.alpha"], "--select requires preflight"),
            (["verify", "--objects", self.objects], "--objects is only used by preflight"),
            (["verify", "--raw"], "--raw is only used by preflight"),
            (["verify", "--preserve-reconstructed"], "--preserve-reconstructed requires preflight --raw"),
            (["preflight", "--objects", self.objects, "--preserve-reconstructed"],
             "--preserve-reconstructed requires preflight --raw"),
        ]
        for arguments, expected in cases:
            with self.subTest(arguments=arguments):
                message = self.run_cli_error(arguments)
                self.assertIn(expected, message)

    def test_raw_preflight_requires_explicit_ids_and_forbids_bindings(self):
        message = self.run_cli_error(["preflight", "--raw", "--objects", self.objects])
        self.assertIn("preflight --raw requires explicit IDs and no --bindings", message)

        message = self.run_cli_error(
            ["preflight", "--raw", "--objects", self.objects, "--bindings", self.bindings,
             "--select", "preflight.beta"])
        self.assertIn("preflight --raw requires explicit IDs and no --bindings", message)

    def test_raw_preflight_routes_selected_ids_without_reference_bindings(self):
        state_before = self.state.read_bytes()
        mock = Mock(side_effect=lambda rows, *args: self._diagnostic_result(rows))
        stdout, _ = self.run_cli(
            ["preflight", "--raw", "--preserve-reconstructed", "--objects", self.objects,
             "--select", "preflight.gamma", "preflight.beta"],
            raw_mock=mock)

        mock.assert_called_once()
        rows, objects, preserve_reconstructed, _, original_root = mock.call_args.args
        self.assertEqual([row["id"] for row in rows], ["preflight.beta", "preflight.gamma"])
        self.assertEqual(objects, self.objects)
        self.assertTrue(preserve_reconstructed)
        self.assertIsNone(original_root)
        self.assertIn("Diagnostic preflight: PASS; 2 functions; 0 compiler calls; no acceptance proof", stdout)
        self.assertEqual(self.state.read_bytes(), state_before)

    def test_reference_cli_has_no_acceptance_or_cached_result_shortcut(self):
        state_before = self.state.read_bytes()
        outcomes = [self._diagnostic_result([self.rows[0]]),
                    dict(ok=False, compiler_calls=0, errors=["changed fixture"],
                         functions=[dict(id=self.rows[0]["id"], errors=["changed fixture"])])]
        mock = Mock(side_effect=outcomes)
        arguments = ["preflight", "--bindings", self.bindings, "--objects", self.objects,
                     "--select", "preflight.alpha"]

        first, _, first_code = self.run_cli_outcome(arguments, reference_mock=mock)
        # The CLI asks the preflight provider each time; it does not turn the first PASS into authority.
        second, _, second_code = self.run_cli_outcome(arguments, reference_mock=mock)
        self.assertIn("Diagnostic preflight: PASS", first)
        self.assertIn("Diagnostic preflight: BLOCKED", second)
        self.assertIn("changed fixture", second)
        self.assertEqual(first_code, 0)
        self.assertEqual(second_code, 1)
        self.assertEqual(mock.call_count, 2)
        self.assertEqual(self.state.read_bytes(), state_before)


if __name__ == "__main__":
    unittest.main()
