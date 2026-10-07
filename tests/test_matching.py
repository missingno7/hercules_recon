"""Adversarial gate checks using generated fixtures, never game-byte fixtures.

Run with: python -m unittest discover -s tests -p test_matching.py -v
The normal project Python analysis dependencies must be available.
"""
from __future__ import annotations

import base64
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "scripts"))
import match
import promote


# A synthetic xor eax,eax; ret followed by synthetic alignment bytes. This is
# deliberately unrelated to any recovered source or original game fixture.
CODE = bytes.fromhex("31c0c3") + b"\x90" * 13


def digest(data):
    return hashlib.sha256(data).hexdigest()


def workspace_temporary_directory(prefix):
    parent = (PROJECT_ROOT / "work/test-fixtures").resolve()
    parent.mkdir(parents=True, exist_ok=True)
    directory = tempfile.TemporaryDirectory(prefix=prefix, dir=parent)
    # Verify the absolute cleanup target before TemporaryDirectory can remove it.
    if not Path(directory.name).resolve().is_relative_to(parent):
        raise ValueError("Temporary fixture escaped the intended workspace")
    return directory


def make_coff(path, *, relocation=False, executable=True, extra_function=False, extra_data=False):
    """A one-function i386 /Gy-style object with one explicit section."""
    relocations = struct.pack("<IIH", 0, 0, 0x0006) if relocation else b""
    flags = 0x60501020 if executable else 0x40501040
    contributions = [(b".text", CODE, flags, relocations)]
    if extra_function:
        contributions.append((b".text", CODE, 0x60501020, b""))
    if extra_data:
        contributions.append((b".data", b"data", 0xC0300040, b""))
    offset = 20 + 40 * len(contributions)
    sections, contents = bytearray(), bytearray()
    for name, data, characteristics, relocs in contributions:
        sections.extend(struct.pack("<8sIIIIIIHHI", name, 0, 0, len(data), offset,
                                    offset + len(data) if relocs else 0,
                                    0, len(relocs) // 10, 0, characteristics))
        contents.extend(data + relocs)
        offset += len(data) + len(relocs)
    symbols = struct.pack("<8sIhHBB", b"_sample\0", 0, 1, 0x20, 2, 0)
    if extra_function:
        symbols += struct.pack("<8sIhHBB", b"_extra\0\0", 0, 2, 0x20, 2, 0)
    header = struct.pack("<HHIIIHH", 0x14C, len(contributions), 0, offset,
                         1 + int(extra_function), 0, 0)
    path.write_bytes(header + sections + contents + symbols + struct.pack("<I", 4))


def make_pe(path, *, base_relocation=False):
    """A minimal file-backed PE32 with executable text and non-executable data."""
    result = bytearray(0xA00)
    result[:2] = b"MZ"
    struct.pack_into("<I", result, 0x3C, 0x80)
    result[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", result, 0x84, 0x14C, 3, 0, 0, 0, 224, 0x102)
    optional = 0x98
    struct.pack_into("<HBB", result, optional, 0x10B, 5, 0)
    struct.pack_into("<III", result, optional + 4, 0x200, 0x400, 0)
    struct.pack_into("<III", result, optional + 16, 0x1000, 0x1000, 0x2000)
    struct.pack_into("<III", result, optional + 28, 0x400000, 0x1000, 0x200)
    struct.pack_into("<HH", result, optional + 40, 4, 0)
    struct.pack_into("<HH", result, optional + 48, 4, 0)
    struct.pack_into("<II", result, optional + 56, 0x4000, 0x400)
    struct.pack_into("<H", result, optional + 68, 3)
    struct.pack_into("<IIIIII", result, optional + 72,
                     0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    if base_relocation:
        struct.pack_into("<II", result, optional + 96 + 5 * 8, 0x3000, 12)
    for index, (name, rva, offset, flags) in enumerate((
        (b".text", 0x1000, 0x400, 0x60000020),
        (b".data", 0x2000, 0x600, 0xC0000040),
        (b".reloc", 0x3000, 0x800, 0x42000040),
    )):
        struct.pack_into("<8sIIIIIIHHI", result, optional + 224 + 40 * index,
                         name, 0x200, rva, 0x200, offset, 0, 0, 0, 0, flags)
    result[0x400:0x400 + len(CODE)] = CODE
    result[0x600:0x600 + len(CODE)] = CODE
    if base_relocation:
        # HIGHLOW relocation overlaps the otherwise equal target text bytes.
        struct.pack_into("<IIHH", result, 0x800, 0x1000, 12, 0x3000, 0)
    path.write_bytes(result)


class MatchingGateTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory("gate-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.addCleanup(patch.stopall)
        patch.object(match, "ROOT", self.root).start()
        self.obj = self.root / "sample.obj"
        self.target = self.root / "sample.dll"
        make_coff(self.obj)
        make_pe(self.target)

    def compare(self, *, rva=0x1000, size=len(CODE)):
        return match.compare(self.obj, "_sample", self.target, rva, size)

    def assert_refused(self, callback):
        # A rejection can be a validation exception or an explicit non-match;
        # parser crashes (IndexError etc.) do not satisfy this assertion.
        try:
            result = callback()
        except ValueError:
            return
        self.assertFalse(result["exact"])

    def test_equal_complete_function_contribution(self):
        result = self.compare()
        self.assertTrue(result["exact"])
        self.assertEqual(result["compiled_size"], len(CODE))
        self.assertEqual(result["different_bytes"], 0)

    def test_body_prefix_does_not_hide_trailing_contribution(self):
        result = self.compare(size=3)
        self.assertFalse(result["exact"])
        self.assertEqual(result["compiled_size"], len(CODE))
        self.assertEqual(result["target_size"], 3)

    def test_equal_bytes_in_nonexecutable_target_are_refused(self):
        self.assert_refused(lambda: self.compare(rva=0x2000))

    def test_noncode_comdat_is_not_a_function(self):
        make_coff(self.obj, executable=False)
        self.assert_refused(self.compare)

    def test_unresolved_object_relocation_prevents_exact(self):
        make_coff(self.obj, relocation=True)
        self.assert_refused(self.compare)

    def test_target_base_relocation_prevents_relocation_free_claim(self):
        make_pe(self.target, base_relocation=True)
        self.assert_refused(self.compare)

    def test_promotion_accepts_exactly_registered_emitted_functions(self):
        promote.validate_candidate_object(self.obj, {"_sample"})

    def test_promotion_refuses_extra_unreviewed_function(self):
        make_coff(self.obj, extra_function=True)
        with self.assertRaises(ValueError):
            promote.validate_candidate_object(self.obj, {"_sample"})

    def test_promotion_refuses_extra_unreviewed_data(self):
        make_coff(self.obj, extra_data=True)
        with self.assertRaises(ValueError):
            promote.validate_candidate_object(self.obj, {"_sample"})

    def test_source_mutation_during_compile_cannot_receive_a_proof(self):
        source = self.root / "sample.c"
        source.write_text("int sample(void) { return 0; }\n", encoding="ascii")
        row = {"id": "sample", "status": "RECONSTRUCTED", "source": "sample.c", "target": "sample.dll",
               "target_sha256": digest(self.target.read_bytes()), "symbol": "_sample",
               "flags": [], "rva": "0x1000", "size": len(CODE)}

        def change_source(*args, **kwargs):
            source.write_text("int sample(void) { return 1; }\n", encoding="ascii")
            return self.obj, ["synthetic-compiler"]

        with patch.object(match, "compile_source", side_effect=change_source), \
                patch.object(match, "verify_toolchain", return_value="test-fixture"):
            with self.assertRaises(ValueError):
                match.check(row)


class PromotionRecoveryTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory("recovery-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.state = self.root / "recovery.json"
        self.source = self.root / "src/title/sample.c"
        self.journal = self.root / "work/promotion-journal.json"
        self.source.parent.mkdir(parents=True)
        self.journal.parent.mkdir(parents=True)
        self.addCleanup(patch.stopall)
        patch.object(promote, "ROOT", self.root).start()
        patch.object(promote, "STATE", self.state).start()
        patch.object(promote, "JOURNAL", self.journal).start()

    def prepare_partial_transaction(self, source_before=b"old source\n"):
        old_state, new_state = b'{"revision": 1}\n', b'{"revision": 2}\n'
        new_source = b"new source\n"
        self.source.write_bytes(new_source)
        self.state.write_bytes(old_state)  # crash between the two replacements
        rows = []
        for path, before, after in ((self.source, source_before, new_source),
                                    (self.state, old_state, new_state)):
            rows.append({"path": path.relative_to(self.root).as_posix(),
                         "before": base64.b64encode(before).decode() if before is not None else None,
                         "before_sha256": digest(before) if before is not None else None,
                         "after_sha256": digest(after)})
        self.journal.write_text(json.dumps(rows), encoding="ascii")
        return old_state

    def test_partial_replacement_rolls_back_source_and_state(self):
        old_state = self.prepare_partial_transaction()
        promote.recover()
        self.assertEqual(self.source.read_bytes(), b"old source\n")
        self.assertEqual(self.state.read_bytes(), old_state)
        self.assertFalse(self.journal.exists())

    def test_new_source_is_removed_when_rolling_back(self):
        old_state = self.prepare_partial_transaction(source_before=None)
        promote.recover()
        self.assertFalse(self.source.exists())
        self.assertEqual(self.state.read_bytes(), old_state)
        self.assertFalse(self.journal.exists())

    def test_concurrent_edit_is_not_overwritten_during_recovery(self):
        old_state = self.prepare_partial_transaction()
        self.source.write_bytes(b"another agent's edit\n")
        before_journal = self.journal.read_bytes()
        with self.assertRaises(ValueError):
            promote.recover()
        self.assertEqual(self.source.read_bytes(), b"another agent's edit\n")
        self.assertEqual(self.state.read_bytes(), old_state)
        self.assertEqual(self.journal.read_bytes(), before_journal)


if __name__ == "__main__":
    unittest.main()
