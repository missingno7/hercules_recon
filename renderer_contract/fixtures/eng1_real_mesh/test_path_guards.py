"""Read-only rejection checks for ENG1 replay output destinations."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys
import unittest
sys.dont_write_bytecode = True

from pathguard import create_fresh_output, repo_path


class OutputPathGuardTests(unittest.TestCase):
    root = None
    existing_output = None

    def test_assets_rejected(self):
        with self.assertRaises(ValueError):
            create_fresh_output(self.root, self.root / "assets" / "eng1-guard-must-not-create")

    def test_existing_work_path_rejected(self):
        existing = self.existing_output
        self.assertIsNotNone(existing)
        self.assertTrue(existing.is_dir())
        with self.assertRaisesRegex(ValueError, "already exists"):
            create_fresh_output(self.root, existing)

    def test_external_path_rejected(self):
        external = self.root.parent / "eng1-guard-must-not-create"
        with self.assertRaises(ValueError):
            create_fresh_output(self.root, external)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--existing-output", required=True, type=Path)
    options, unittest_args = parser.parse_known_args()
    OutputPathGuardTests.root = repo_path(options.repo_root)
    OutputPathGuardTests.existing_output = options.existing_output.resolve(strict=True)
    unittest.main(argv=[sys.argv[0], *unittest_args])
