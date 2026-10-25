"""Synthetic compiler-free tests for reviewed-reference preflight."""
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
import reference_preflight
from env import ROOT
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file
from test_zero_storage import fixture_pe


def sha(data):
    return hashlib.sha256(data).hexdigest()


class ReferencePreflightTests(unittest.TestCase):
    def setUp(self):
        self.directory = workspace_temporary_directory("reference-preflight-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.source = self.root / "caller.c"
        self.header = self.root / "dep.h"
        self.target = self.root / "target.dll"
        self.obj = self.root / "caller.obj"
        self.bindings = self.root / "bindings.json"
        self.evidence = self.root / "review.txt"
        self.toolchain = self.root / "toolchain"
        self.extra_evidence = {}
        (self.toolchain / "bin").mkdir(parents=True)
        self.header.write_text("#define DEP_VALUE 7\n")
        self.source.write_text('#include "dep.h"\nint caller(void) { return DEP_VALUE; }\n')
        make_pe(self.target)
        self.native = b"\xe8\0\0\0\0\xc3"
        data = bytearray(self.target.read_bytes())
        data[0x400:0x406] = self.native
        self.target.write_bytes(data)
        self._write_function_object()
        self.row = self._row("preflight.caller", "_sample")
        self.record = binding("function", 0x1100, 1, (1, 20, 0))
        self.evidence.write_text("independent synthetic identity\n")
        self._write_bindings({self.row["id"]: {"_callee": self.record}})
        self.report = {"results": [self._result(self.row)]}

    def _write_function_object(self):
        code = b"\xe8\0\0\0\0\xc3"
        coff_file(self.obj, code, [(1, 1, 20)],
                  [("_sample", 0, 1, 0x20), ("_callee", 0, 0, 0x20)])

    def _row(self, row_id, symbol):
        return dict(id=row_id, source=self.source.relative_to(ROOT).as_posix(),
                    target=self.target.relative_to(ROOT).as_posix(), target_sha256=sha(self.target.read_bytes()),
                    rva="0x1000", size=6, symbol=symbol, flags=[])

    def _result(self, row):
        source = (ROOT / row["source"]).resolve()
        context = match.compile_inputs(source, row["flags"], self.toolchain)
        return dict(id=row["id"], object=str(self.obj.resolve()), object_sha256=sha(self.obj.read_bytes()),
                    source_sha256=sha(source.read_bytes()), target_sha256=sha(self.native),
                    command=[str((self.toolchain / "bin" / "cl.exe").resolve()), "/nologo", "/c", "/Gy",
                             *row["flags"], "/Fo" + str(self.obj.resolve()), str(source)],
                    compile_inputs={str(path): value for path, value in context.items()})

    def _write_bindings(self, functions):
        evidence_name = self.evidence.relative_to(ROOT).as_posix()
        evidence_files = {evidence_name: sha(self.evidence.read_bytes()), **self.extra_evidence}
        document = dict(schema=1, evidence_files=evidence_files,
                        functions={row_id: dict(symbols=symbols) for row_id, symbols in functions.items()})
        self.bindings.write_text(json.dumps(document, indent=2) + "\n")

    def _run(self, rows=None, report=None, bindings=None):
        with patch.object(match, "compile_source", side_effect=AssertionError("CL must not run")):
            return reference_preflight.preflight(rows or [self.row], bindings or self.bindings,
                                                self.report if report is None else report,
                                                toolchain=self.toolchain)

    def test_valid_profile_reports_exact_uses_and_zero_compiler_calls(self):
        result = self._run()
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["compiler_calls"], 0)
        self.assertEqual(result["functions"][0]["actual_relocations"],
                         [dict(symbol="_callee", offset=1, type=20, addend=0)])
        self.assertTrue(self._run(report=self.report["results"])["ok"])
        report_path = self.root / "objects.json"
        report_path.write_text(json.dumps(self.report) + "\n")
        self.assertTrue(self._run(report=report_path)["ok"])

    def test_binding_offset_addend_and_type_mismatches_show_actual_use(self):
        for change in ({"offset": 2}, {"addend": 1}, {"type": 6}):
            with self.subTest(change=change):
                record = dict(self.record)
                record["references"] = [dict(record["references"][0], **change)]
                self._write_bindings({self.row["id"]: {"_callee": record}})
                result = self._run()
                self.assertFalse(result["ok"])
                function = result["functions"][0]
                self.assertEqual(function["actual_relocations"][0],
                                 dict(symbol="_callee", offset=1, type=20, addend=0))
                self.assertTrue(any("references differ" in message for message in function["errors"]))

    def test_stale_object_include_and_ordered_flags_are_rejected(self):
        self.obj.write_bytes(self.obj.read_bytes() + b"stale")
        result = self._run()
        self.assertTrue(any("object hash is stale" in message for message in result["functions"][0]["errors"]))

        self._write_function_object()
        self.header.write_text("#define DEP_VALUE 8\n")
        result = self._run()
        self.assertTrue(any("source/include closure" in message for message in result["functions"][0]["errors"]))

        self.header.write_text("#define DEP_VALUE 7\n")
        altered = json.loads(json.dumps(self.report))
        altered["results"][0]["command"].insert(4, "/O2")
        result = self._run(report=altered)
        self.assertTrue(any("profile" in message for message in result["functions"][0]["errors"]))

    def test_shadow_header_is_diagnosed_even_when_selected_closure_is_unchanged(self):
        shadow = self.toolchain / "include" / "dep.h"
        shadow.parent.mkdir(parents=True)
        shadow.write_text("#define DEP_VALUE 7\n")
        result = self._run()
        self.assertTrue(any("shadowed include dep.h" in message for message in result["functions"][0]["errors"]))

    def test_same_source_and_ordered_flags_share_one_unambiguous_profile(self):
        second = dict(self.row, id="preflight.peer")
        self._write_bindings({self.row["id"]: {"_callee": self.record},
                              second["id"]: {"_callee": self.record}})
        self.report["results"].append(self._result(second))
        result = self._run(rows=[self.row, second])
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["compiler_calls"], 0)

    def test_bss_anchor_is_checked_before_owner_profile_and_against_all_selected_ranges(self):
        identity = fixture_pe(self.target)
        self.native = b"\xa1" + struct.pack("<I", 0x400000 + 0x2200) + b"\xc3"
        data = bytearray(self.target.read_bytes())
        data[0x400:0x406] = self.native
        self.target.write_bytes(data)
        self._write_function_object()
        # Replace the fixture caller with its absolute data reference.
        coff_file(self.obj, b"\xa1" + struct.pack("<i", 0) + b"\xc3", [(1, 1, 6)],
                  [("_sample", 0, 1, 0x20), ("_cell", 0, 0, 0)])
        self.row = self._row(self.row["id"], "_sample")
        owner_dir = tempfile.TemporaryDirectory(prefix="preflight-owner-", dir=ROOT / "src")
        self.addCleanup(owner_dir.cleanup)
        owner_source = Path(owner_dir.name) / "owner.c"
        owner_source.write_text("int owner;\n")
        self.extra_evidence[owner_source.relative_to(ROOT).as_posix()] = sha(owner_source.read_bytes())
        record = binding("bss", 0x2200, 4, (1, 6, 0), identity=identity,
                         owner=dict(source=owner_source.relative_to(ROOT).as_posix(), flags=[], symbol="_cell"))
        self.evidence.write_text("independent synthetic owner and anchor\n")
        self._write_bindings({self.row["id"]: {"_cell": record}})
        self.report = {"results": [self._result(self.row)]}
        result = self._run()
        self.assertTrue(result["ok"], result)
        self.assertTrue(result["functions"][0]["owner_profiles"][0]["requires_fresh_full_gate"])

        # This OR absolute-memory instruction has a correct target operand,
        # matching byte hash, and HIGHLOW fixup. The anchor shape is unsupported.
        or_anchor = b"\x81\x0d" + struct.pack("<I", 0x400000 + 0x2200) + b"\0\0\0\0"
        image = bytearray(self.target.read_bytes())
        image[0x500:0x500 + len(or_anchor)] = or_anchor
        struct.pack_into("<H", image, 0x80a, 0x3102)  # fixup for disp32 at RVA 0x1102
        self.target.write_bytes(image)
        self.row = self._row(self.row["id"], "_sample")
        unsupported = dict(record, identity=dict(rva=0x1100, size=len(or_anchor), operand=0,
                                                 sha256=sha(or_anchor)))
        self._write_bindings({self.row["id"]: {"_cell": unsupported}})
        self.report = {"results": [self._result(self.row)]}
        with patch.object(reference_preflight, "owner_profile", wraps=reference_preflight.owner_profile) as owner:
            result = self._run()
            owner.assert_not_called()
        self.assertFalse(result["ok"])
        self.assertTrue(any("Unsupported loader-zero identity instruction" in message
                            for message in result["functions"][0]["errors"]))
        self.assertEqual(result["functions"][0]["owner_profiles"], [])

        bad_anchor = dict(record, identity=dict(identity, sha256="0" * 64))
        self._write_bindings({self.row["id"]: {"_cell": bad_anchor}})
        result = self._run()
        self.assertFalse(result["ok"])
        self.assertEqual(result["functions"][0]["owner_profiles"], [])

        overlapping = dict(record, identity=dict(identity, rva=0x1000, size=5))
        self._write_bindings({self.row["id"]: {"_cell": overlapping}})
        result = self._run()
        self.assertFalse(result["ok"])
        self.assertTrue(any("overlaps a selected same-module caller" in message
                            for message in result["functions"][0]["errors"]))

        peer = dict(self.row, id="preflight.anchor-peer", rva="0x1100", size=5)
        self._write_bindings({self.row["id"]: {"_cell": record}, peer["id"]: {"_cell": record}})
        result = self._run(rows=[self.row, peer])
        self.assertTrue(any("overlaps a selected same-module caller" in message
                            for message in result["functions"][0]["errors"]))

    def test_input_mutation_during_inventory_is_caught_by_final_snapshot(self):
        original = reference_preflight.COFF
        header_before = self.header.read_text()

        def mutating_coff(path):
            value = original(path)
            original_function = value.function

            def mutate_after_inventory(symbol):
                section = original_function(symbol)
                self.header.write_text("#define DEP_VALUE 9\n")
                return section

            value.function = mutate_after_inventory
            return value

        with patch.object(reference_preflight, "COFF", side_effect=mutating_coff):
            result = self._run()
        self.header.write_text(header_before)
        self.assertFalse(result["ok"])
        self.assertTrue(any("input changed during preflight" in message
                            for message in result["functions"][0]["errors"]))


if __name__ == "__main__":
    unittest.main()
