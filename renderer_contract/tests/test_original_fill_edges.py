"""Bounded original D420 -> C2E0 constant-fill polygon fixtures."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import sys
from pathlib import Path


def find_root(path: Path) -> Path:
    for parent in (path.resolve(), *path.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("repository root with scripts/pe.py and public oracle_common.py not found")


ROOT = find_root(Path(__file__))
TESTS = ROOT / "renderer_contract" / "tests"
sys.path.insert(0, str(TESTS))
from oracle_common import (  # noqa: E402
    DEFAULT_ANALYSIS_DEPS, DEFAULT_DEPS, EXPECTED_EXE_SHA256,
    verify_runtime, write_receipt,
)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--receipt", type=Path,
                        default=ROOT / "work" / "renderer_next" / "fill_edges" / "native_report.json")
    parser.add_argument("--compact-receipt", type=Path,
                        default=ROOT / "work" / "renderer_next" / "fill_edges" / "compact_evidence.json")
    parser.add_argument("--deps", type=Path, default=DEFAULT_DEPS)
    parser.add_argument("--analysis-deps", type=Path, default=DEFAULT_ANALYSIS_DEPS)
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    args.receipt = args.receipt.resolve()
    args.compact_receipt = args.compact_receipt.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    try:
        args.receipt.relative_to((ROOT / "work").resolve())
        args.compact_receipt.relative_to((ROOT / "work").resolve())
    except ValueError:
        parser.error("--receipt and --compact-receipt must stay under the repository's ignored work directory")
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != EXPECTED_EXE_SHA256:
        parser.error("--exe does not match the pinned original HERCULES.EXE SHA-256")
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("pinned Unicorn and analysis dependency directories must exist")
    return args


ARGS = parse_args()
os.environ["HERCULES_PYTHON_DEPS"] = str(ARGS.analysis_deps)
sys.path[:0] = [str(ARGS.deps), str(ROOT / "scripts")]

# Reuse the public original-polygon model helpers for PE mapping, source
# pointers, reciprocal-table setup, clipping model, and synthetic memory. Import
# the checked-in test module as a library without running its main cases.
POLYGON_MODEL_HELPERS = ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"
_saved_argv = sys.argv[:]
sys.argv = [str(POLYGON_MODEL_HELPERS), "--exe", str(ARGS.exe),
            "--receipt", str(ARGS.receipt), "--original-root", str(ROOT),
            "--deps", str(ARGS.deps), "--analysis-deps", str(ARGS.analysis_deps)]
spec = importlib.util.spec_from_file_location("public_polygon_edge_model_helpers", POLYGON_MODEL_HELPERS)
if spec is None or spec.loader is None:
    raise RuntimeError("cannot load private polygon edge fixture")
poly = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = poly
spec.loader.exec_module(poly)
sys.argv = _saved_argv

from pe import PE  # noqa: E402
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_32  # noqa: E402
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI,
)

BASE = poly.BASE
F3_CELLS = {
    poly.F3_LEFT: "left",
    poly.F3_TOP: "top",
    poly.F3_RIGHT: "right",
    poly.F3_BOTTOM: "bottom",
}
C2E0_SPAN = (0xC2E0, 0xC6F8)
D420_FILL_ROUTE = (0xD555, 0xD60C)

# Allow the original C2E0 body only in addition to the windows used by the
# reused polygon fixture. Its original span is snapshotted for mutation checks.
poly.CODE_WINDOWS = tuple(poly.CODE_WINDOWS) + (C2E0_SPAN,)


class FillCase(poly.NativeCase):
    def __init__(self, pe: PE, data_region: tuple[int, int], spec: dict,
                 route: str, source_byte: int):
        self.f3_reads = []
        self.endpoint_trace = []
        self.pending_d60_endpoint = None
        super().__init__(pe, data_region, spec, route)
        # Row 3 uses the D420 sample -> C2E0 route only when the gate is clear.
        # Row zero selects the existing D60C path for coverage comparison.
        poly.put32(self.uc, poly.GATE, 0)
        self.uc.mem_write(poly.TEXTURE, bytes([source_byte]) * 0x10000)
        self.source_byte = source_byte
        self.code_snapshot[C2E0_SPAN] = bytes(
            self.uc.mem_read(BASE + C2E0_SPAN[0], C2E0_SPAN[1] - C2E0_SPAN[0]))
        self.uc.hook_add(UC_HOOK_CODE, self._edge_endpoints)

    @staticmethod
    def _word(value: int) -> dict:
        signed = value if value < 0x80000000 else value - 0x100000000
        return {"raw32": hex(value), "signed": signed, "sar16": signed >> 16}

    @staticmethod
    def _signed32(value: int) -> int:
        return value if value < 0x80000000 else value - 0x100000000

    @staticmethod
    def _scanline(uc) -> dict:
        # A90944 is the destination row base. A90968 is a clipped Y bound,
        # not the current scanline, so derive the row from the live row base.
        row_base = int.from_bytes(bytes(uc.mem_read(0xA90944, 4)), "little")
        offset = row_base - poly.SURFACE
        if offset < 0 or offset >= poly.SURFACE_SIZE or offset % poly.SURFACE_STRIDE:
            raise AssertionError(f"unexpected destination row base {row_base:#x}")
        return {"row": offset // poly.SURFACE_STRIDE, "row_base": hex(row_base)}

    def _edge_endpoints(self, uc, address, size, _):
        rva = address - BASE
        if rva not in (0xC5F3, 0xC615, 0xDAD6, 0xDB23, 0xDB64):
            return
        if self.spec["name"] not in ("shared_edge_upper", "negative_and_vertical_clip"):
            return
        scanline = self._scanline(uc)
        if rva == 0xC5F3:
            # The compare and signed branches below this site sort ECX/ESI,
            # then SAR each endpoint. Capture both before either SAR executes.
            endpoints = sorted((uc.reg_read(UC_X86_REG_ECX),
                                uc.reg_read(UC_X86_REG_ESI)),
                               key=lambda value: self._word(value)["signed"])
            self.endpoint_trace.append({"pc_rva": "0xc5f3", **scanline,
                                        "stage": "C2E0 sorted raw endpoints before SAR16",
                                        "left": self._word(endpoints[0]),
                                        "right": self._word(endpoints[1])})
        elif rva == 0xC615:
            left = self._signed32(uc.reg_read(UC_X86_REG_EDX))
            right = self._signed32(uc.reg_read(UC_X86_REG_EAX))
            self.endpoint_trace.append({"pc_rva": "0xc615", **scanline,
                                        "stage": "C2E0 endpoints after SAR16",
                                        "left": left, "right": right})
        elif rva in (0xDAD6, 0xDB23):
            left = uc.reg_read(UC_X86_REG_ECX)
            right = uc.reg_read(UC_X86_REG_EAX)
            self.pending_d60_endpoint = {"raw_left": self._word(left),
                                         "raw_right": self._word(right),
                                         "raw_pc_rva": hex(rva), **scanline}
        elif rva == 0xDB64:
            left = self._signed32(uc.reg_read(UC_X86_REG_ECX))
            right = self._signed32(uc.reg_read(UC_X86_REG_EAX))
            self.endpoint_trace.append({
                "pc_rva": "0xdb64", **scanline,
                "stage": "D60C ordered endpoints after SAR16",
                "left": left, "right": right,
                "preceding_raw_endpoints": self.pending_d60_endpoint,
            })

    def _read(self, uc, access, address, size, value, _):
        if address in F3_CELLS:
            self.f3_reads.append({
                "pc_rva": hex(uc.reg_read(poly.UC_X86_REG_EIP) - BASE),
                "cell_va": hex(address),
                "cell": F3_CELLS[address],
                "size": size,
                "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little"),
            })
        return super()._read(uc, access, address, size, value, _)

    def run(self) -> dict:
        base_result = super().run()
        result = {key: base_result[key] for key in (
            "case", "route", "entry_rva", "runtime_precondition", "vertices_xy",
            "f3_clip_values", "expected_pixel_set_integer_scan_floor_span_model")}
        surface = bytes(self.uc.mem_read(poly.SURFACE, poly.SURFACE_SIZE))
        initial = self.initial_surface
        changed = []
        for offset, value in enumerate(surface):
            if value != initial[offset]:
                changed.append({"x": offset % poly.SURFACE_STRIDE,
                                "y": offset // poly.SURFACE_STRIDE,
                                "value": value})
        changed_pixels = [(p["x"], p["y"]) for p in changed]
        expected_pixels = {tuple(p) for p in result["expected_pixel_set_integer_scan_floor_span_model"]}
        changed_set = set(changed_pixels)
        path_markers = sorted({hex(rva) for rva in self.trace
                               if rva in (0xD420, 0xD555, 0xD577, 0xD595, 0xD5DA,
                                          0xD600, 0xD605, 0xC2E0, 0xC3C2, 0xC3F4,
                                          0xC400, 0xC4A6, 0xC4B7, 0xC54E, 0xC55F,
                                          0xC5F3, 0xC63F, 0xC671, 0xC678, 0xC67F,
                                          0xC6F1, 0xD60C, 0xDC8A, 0xDC96, 0xD549)})
        data_after = bytes(self.uc.mem_read(self.data_va, self.data_size))
        data_before = bytes(self.code_data_snapshot)
        changed_data = []
        changed_bytes_outside_reciprocal_table = 0
        changed_reciprocal_table_bytes = 0
        table_start = poly.RECIP_TABLE - self.data_va
        table_end = table_start + poly.RECIP_COUNT * 4
        begin = None
        for i, (before, after) in enumerate(zip(data_before, data_after)):
            if table_start <= i < table_end:
                if before != after:
                    changed_reciprocal_table_bytes += 1
                if begin is not None:
                    changed_data.append({"offset": hex(begin), "length": i - begin})
                    begin = None
                continue
            if before != after:
                changed_bytes_outside_reciprocal_table += 1
                if begin is None:
                    begin = i
            elif begin is not None:
                changed_data.append({"offset": hex(begin), "length": i - begin})
                begin = None
        if begin is not None:
            changed_data.append({"offset": hex(begin), "length": len(data_after) - begin})
        result.update({
            "requested_route": self.route,
            "input_vertex_count": len(self.spec["vertices"]),
            "gate_value": 0,
            "encoded_row_before_normalization": hex(poly.ROWZERO_ENCODED if self.rowzero else poly.CLUT3_ENCODED),
            "normalized_row_after_dispatch": hex(int.from_bytes(
                bytes(self.uc.mem_read(poly.ROW_CODE, 4)), "little")),
            "sample_count": len(self.source_reads),
            "source_samples": self.source_reads,
            "palette_read_count": len(self.palette_reads),
            "actual_pixels_from_final_surface_diff": changed,
            "actual_pixel_coordinates_from_final_surface_diff": [list(p) for p in changed_pixels],
            "final_surface_diff_matches_integer_scan_model": changed_set == expected_pixels,
            "integer_scan_model_only": [list(p) for p in sorted(expected_pixels - changed_set, key=lambda p: (p[1], p[0]))],
            "final_surface_diff_only": [list(p) for p in sorted(changed_set - expected_pixels, key=lambda p: (p[1], p[0]))],
            "fill_values": sorted({p["value"] for p in changed}),
            "surface_write_event_count": len(self.surface_writes),
            "f3_runtime_reads": self.f3_reads,
            "span_endpoint_trace": self.endpoint_trace,
            "path_markers": path_markers,
            "c2e0_entry_count": sum(1 for rva in self.trace if rva == 0xC2E0),
            "cab0_fastpath_callsite_seen": 0xD549 in self.trace,
            "data_changed_ranges_outside_reciprocal_table": changed_data,
            "changed_data_bytes_outside_reciprocal_table": changed_bytes_outside_reciprocal_table,
            "changed_reciprocal_table_bytes": changed_reciprocal_table_bytes,
            "original_code_unchanged": True,
        })
        self.result = result
        return result


SPEC_SHAPES = {
    "shared_edge_upper": {
        "vertices": [(2, 2), (8, 2), (8, 8)],
        "clip": (0, 0, 320, 24),
    },
    "shared_edge_lower": {
        "vertices": [(2, 2), (8, 8), (2, 8)],
        "clip": (0, 0, 320, 24),
    },
    "triangle_reverse": {
        "vertices": [(2, 8), (8, 2), (2, 2)],
        "clip": (0, 0, 320, 24),
    },
    "quad_irregular": {
        "vertices": [(2, 2), (8, 3), (7, 9), (3, 8)],
        "clip": (0, 0, 320, 24),
    },
    "negative_and_vertical_clip": {
        "vertices": [(-2, -2), (10, 2), (2, 12)],
        # left/right are deliberately restrictive sentinels. C2E0 is expected
        # to use top/bottom and its hard screen x bounds, not these two cells.
        "clip": (4, 3, 6, 8),
    },
}
BASE_TRIANGLE = {
    "vertices": [(2, 2), (8, 2), (2, 8)],
    "clip": (0, 0, 320, 24),
}


def load_data_region(pe: PE) -> tuple[int, int]:
    for section in pe.sections:
        if section["name"] == ".data":
            return BASE + section["virtual_address"], section["virtual_size"]
    raise RuntimeError("pinned original image has no .data section")


def changed_pixels(result: dict) -> set[tuple[int, int]]:
    return {(p["x"], p["y"]) for p in result["actual_pixels_from_final_surface_diff"]}


def run_one(pe: PE, data_region: tuple[int, int], name: str, spec: dict,
            route: str, source_byte: int) -> dict:
    configured = dict(spec)
    configured["name"] = name
    # AC60-facing vertices use raw signed integer coordinates; fixed UVs are all 0.
    # NativeCase materializes zero U/V lanes when this stays None; omit the
    # polygon_edges UV model because C2E0 is a constant-fill route.
    configured["attributes"] = None
    case = FillCase(pe, data_region, configured, route, source_byte)
    case.code_data_snapshot = bytes(case.uc.mem_read(case.data_va, case.data_size))
    return case.run()


def span_record(pe: PE, name: str, start: int, end: int) -> dict:
    code = pe.read_rva(start, end - start)
    return {"name": name, "start_rva": hex(start), "end_rva_exclusive": hex(end),
            "size": len(code), "sha256": hashlib.sha256(code).hexdigest()}


def endpoint_precision_evidence(c2_cases: dict, d60_cases: dict) -> dict:
    c2 = c2_cases["shared_edge_upper"]["span_endpoint_trace"]
    d60 = d60_cases["shared_edge_upper"]["span_endpoint_trace"]
    c2_raw_by_row = {row["row"]: row for row in c2 if row["pc_rva"] == "0xc5f3"}
    d60_by_row = {row["row"]: row for row in d60}
    rows = []
    for y in range(2, 8):
        c2_left = int(c2_raw_by_row[y]["left"]["raw32"], 16)
        d60_left = int(d60_by_row[y]["preceding_raw_endpoints"]["raw_left"]["raw32"], 16)
        c2_right = int(c2_raw_by_row[y]["right"]["raw32"], 16)
        d60_right = int(d60_by_row[y]["preceding_raw_endpoints"]["raw_right"]["raw32"], 16)
        if (c2_left != y * 0x10000 or d60_left != 0x20000 + (y - 2) * 0xFFFF or
                c2_right != 0x80000 or d60_right != 0x80000):
            raise AssertionError(f"unexpected shared-edge endpoint precision at row {y}")
        if (c2_raw_by_row[y]["left"]["sar16"] != c2_left >> 16 or
                d60_by_row[y]["preceding_raw_endpoints"]["raw_left"]["sar16"] != d60_left >> 16):
            raise AssertionError(f"unexpected SAR16 endpoint result at row {y}")
        rows.append({
            "row": y,
            "c2e0_raw_left_q16": hex(c2_left),
            "c2e0_floor_left": c2_left >> 16,
            "d60c_raw_left_q16": hex(d60_left),
            "d60c_floor_left": d60_left >> 16,
            "c2e0_raw_right_q16": hex(c2_right),
            "d60c_raw_right_q16": hex(d60_right),
        })
    c2_clip_rows = {row["row"]: row for row in c2_cases["negative_and_vertical_clip"]["span_endpoint_trace"]
                    if row["pc_rva"] == "0xc5f3"}
    d60_clip_rows = {row["row"]: row for row in d60_cases["negative_and_vertical_clip"]["span_endpoint_trace"]}
    c2_clip_right = int(c2_clip_rows[7]["right"]["raw32"], 16)
    d60_clip_right = int(d60_clip_rows[7]["preceding_raw_endpoints"]["raw_right"]["raw32"], 16)
    if c2_clip_right != 0x60004 or d60_clip_right != 0x5FFFF:
        raise AssertionError("unexpected clipped-case right-edge precision at row 7")
    c2_clip_pixels = changed_pixels(c2_cases["negative_and_vertical_clip"])
    d60_clip_pixels = changed_pixels(d60_cases["negative_and_vertical_clip"])
    return {
        "checked_hypothesis": "C2E0 endpoint conversion adds 0xffff before SAR16, producing ceiling-like endpoints unlike D60C",
        "result": "FALSIFIED",
        "falsifier_observed": "both paths use plain SAR16; C2E0 edge accumulation advances exactly 0x10000 per row while D60C advances 0xffff per row for this six-row diagonal",
        "shared_edge_upper_rows": rows,
        "derived_effect": "at rows 3..7 C2E0 floors the diagonal to x=row; D60C floors its slightly sub-integer reciprocal result to x=row-1, transferring one shared-edge pixel per row",
        "coverage_counts": {
            "c2e0_upper": len(changed_pixels(c2_cases["shared_edge_upper"])),
            "d60c_upper": len(changed_pixels(d60_cases["shared_edge_upper"])),
        },
        "negative_clip_row_7_right_edge": {
            "c2e0_raw_q16": hex(c2_clip_right),
            "c2e0_floor": c2_clip_right >> 16,
            "d60c_raw_q16": hex(d60_clip_right),
            "d60c_floor": d60_clip_right >> 16,
            "c2e0_only_pixel_5_7": (5, 7) in c2_clip_pixels - d60_clip_pixels,
        },
    }


def disassemble(pe: PE, start: int, end: int) -> list[dict]:
    md = poly.capstone.Cs(poly.capstone.CS_ARCH_X86, poly.capstone.CS_MODE_32)
    return [{"rva": hex(insn.address - BASE), "instruction": f"{insn.mnemonic} {insn.op_str}".strip()}
            for insn in md.disasm(pe.read_rva(start, end - start), BASE + start)]


def main() -> None:
    pe = PE(ARGS.exe)
    if pe.image_base != BASE:
        raise RuntimeError(f"unexpected image base {pe.image_base:#x}")
    if hashlib.sha256(pe.data).hexdigest() != EXPECTED_EXE_SHA256:
        raise RuntimeError("PE parser bytes differ from pinned oracle hash")
    runtime = verify_runtime(poly.unicorn, ARGS.deps)
    data_region = load_data_region(pe)

    # Five logical discriminators: sampled byte controls an opaque fill (nonzero
    # and zero); signed winding; count-four non-rectangle fallback; shared edge;
    # F3 top/bottom versus the left/right cells while coordinates are negative.
    c2_cases = {}
    d60_cases = {}
    c2_inputs = [
        ("triangle_nonzero", BASE_TRIANGLE, 0x5A),
        ("triangle_zero", BASE_TRIANGLE, 0x00),
        ("triangle_reverse", SPEC_SHAPES["triangle_reverse"], 0x5A),
        ("quad_irregular", SPEC_SHAPES["quad_irregular"], 0x5A),
        ("shared_edge_upper", SPEC_SHAPES["shared_edge_upper"], 0x5A),
        ("shared_edge_lower", SPEC_SHAPES["shared_edge_lower"], 0x5A),
        ("negative_and_vertical_clip", SPEC_SHAPES["negative_and_vertical_clip"], 0x5A),
    ]
    for name, spec, texel in c2_inputs:
        c2_cases[name] = run_one(pe, data_region, name, spec, "c2e0_fill", texel)
        observed = c2_cases[name]
        trace = {int(rva, 16) for rva in observed["path_markers"]}
        if (observed["input_vertex_count"] not in (3, 4) or
                observed["normalized_row_after_dispatch"] != "0x3" or
                observed["sample_count"] != 1 or
                observed["source_samples"][0]["pc"] != "0xd5da" or
                observed["palette_read_count"] != 0 or
                observed["c2e0_entry_count"] != 1 or
                0xD600 not in trace or observed["cab0_fastpath_callsite_seen"]):
            raise AssertionError(f"{name}: D420 did not follow the expected sampled C2E0 route")
        expected_fill = texel
        if observed["fill_values"] != [expected_fill] or not observed["actual_pixel_coordinates_from_final_surface_diff"]:
            raise AssertionError(f"{name}: C2E0 did not fill with the one sampled source byte")
        if texel != 0:
            d60_cases[name] = run_one(pe, data_region, name, spec, "d60c_rowzero", texel)

    base_pixels = changed_pixels(c2_cases["triangle_nonzero"])
    reverse_pixels = changed_pixels(c2_cases["triangle_reverse"])
    upper_pixels = changed_pixels(c2_cases["shared_edge_upper"])
    lower_pixels = changed_pixels(c2_cases["shared_edge_lower"])
    shared_union = upper_pixels | lower_pixels
    shared_overlap = upper_pixels & lower_pixels
    expected_rectangle = {(x, y) for y in range(2, 8) for x in range(2, 8)}
    clipped_pixels = changed_pixels(c2_cases["negative_and_vertical_clip"])
    d60_upper = changed_pixels(d60_cases["shared_edge_upper"])
    d60_lower = changed_pixels(d60_cases["shared_edge_lower"])
    d60_shared_union = d60_upper | d60_lower
    d60_shared_overlap = d60_upper & d60_lower

    comparisons = {}
    for name, native in c2_cases.items():
        if name in d60_cases:
            c2_set = changed_pixels(native)
            d60_set = changed_pixels(d60_cases[name])
            comparisons[name] = {
                "c2e0_matches_d60c_coverage": c2_set == d60_set,
                "c2e0_only": [list(p) for p in sorted(c2_set - d60_set, key=lambda p: (p[1], p[0]))],
                "d60c_only": [list(p) for p in sorted(d60_set - c2_set, key=lambda p: (p[1], p[0]))],
            }

    refs_c2e0 = poly.static_refs(pe, C2E0_SPAN[0], C2E0_SPAN[1])
    refs_d60c = poly.static_refs(pe, 0xD60C, 0xDD70)
    endpoint_precision = endpoint_precision_evidence(c2_cases, d60_cases)
    spans = [
        span_record(pe, "D420 complete polygon/dispatch entry", 0xD420, 0xDD70),
        span_record(pe, "D420 row normalization, sample and C2E0 dispatch", 0xD555, 0xD605),
        span_record(pe, "D420 sampled-byte expression and C2E0 call", 0xD595, 0xD605),
        span_record(pe, "C2E0 complete constant-fill polygon body", C2E0_SPAN[0], C2E0_SPAN[1]),
        span_record(pe, "C2E0 bounds and vertical clipping", 0xC3C2, 0xC462),
        span_record(pe, "C2E0 signed edge stepping", 0xC469, 0xC5F3),
        span_record(pe, "C2E0 scanline fill loop", 0xC5F3, 0xC6F8),
        span_record(pe, "C2E0 endpoint ordering and plain SAR16 conversion", 0xC5F3, 0xC60D),
        span_record(pe, "D60C complete row-zero comparison body", 0xD60C, 0xDD70),
    ]

    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_ORIGINAL_BYTES",
        "scope": "Finite original D420 gate-zero, row-3 sampled-byte dispatch into C2E0 constant-fill polygon body; paired D60C row-zero runs compare finite coverage only.",
        "prediction_sha256": hashlib.sha256((ROOT / "work" / "renderer_next" / "fill_edges" / "prediction.json").read_bytes()).hexdigest(),
        "oracle": {"path": str(ARGS.exe), "sha256": hashlib.sha256(pe.data).hexdigest(), "image_base": hex(pe.image_base)},
        "fixture_inputs": {
            "driver_path": str(Path(__file__).resolve()),
            "driver_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "prediction_path": str(ROOT / "work" / "renderer_next" / "fill_edges" / "prediction.json"),
            "public_oracle_common_path": str(ROOT / "renderer_contract" / "tests" / "oracle_common.py"),
            "public_oracle_common_sha256": hashlib.sha256((ROOT / "renderer_contract" / "tests" / "oracle_common.py").read_bytes()).hexdigest(),
            "public_polygon_model_helpers_path": str(POLYGON_MODEL_HELPERS),
            "public_polygon_model_helpers_sha256": hashlib.sha256(POLYGON_MODEL_HELPERS.read_bytes()).hexdigest(),
            "reused_public_helper_components": ["PE mapping", "raw32 source/surface pointers", "reciprocal-table initializer", "synthetic texture and surface", "bounded read/write/code hooks", "independent integer-scan coverage model"],
        },
        "runtime": {"engine": runtime["engine"], "unicorn_version": runtime["unicorn_version"],
                    "native_dll_sha256": runtime["native_dll_sha256"],
                    "analysis_capstone_version": poly.capstone.__version__,
                    "architecture": "x86 32-bit", "instruction_budget_per_case": poly.MAX_INSNS,
                    "original_code_written": False},
        "spans": spans,
        "dispatch": {
            "entry_rva": "0xd420",
            "three_vertices": "count != 4 bypasses the D420 CAB0 rectangle predicate and proceeds at D555",
            "four_vertices": "count == 4 tests the rectangle predicate; the irregular fixture must fail it before reaching D555",
            "gate_cell_va": hex(poly.GATE),
            "gate_value": 0,
            "encoded_nonzero_row": hex(poly.CLUT3_ENCODED),
            "normalized_row": 3,
            "sample_pc_rva": "0xd5da",
            "sampled_color_cell_va": hex(0xA90A08),
            "fill_call_pc_rva": "0xd600",
            "fill_target_rva": "0xc2e0",
            "zero_source_is_expected_to_fill": True,
        },
        "static_f3_absolute_reads": {
            "c2e0": refs_c2e0,
            "d60c_comparison": refs_d60c,
            "observed_cells": sorted({r["cell"] for case in c2_cases.values() for r in case["f3_runtime_reads"]}),
        },
        "edge_and_clip_contract_observed": {
            "signed_edge_step_rvas": ["0xc4b8", "0xc560"],
            "edge_division": "signed IDIV of Q16 x delta by signed integer y delta; integer scan advances are accumulated",
            "endpoint_precision_followup": endpoint_precision,
            "static_endpoint_conversion": {
                "c2e0_rva_range": "0xc5f3..0xc60d",
                "c2e0_instructions": disassemble(pe, 0xC5F3, 0xC60D),
                "d60c_endpoint_floor_sites": ["0xdad6", "0xdaf1", "0xdb23", "0xdb3e"],
                "conclusion": "No 0xffff bias precedes C2E0 SAR16; both edge endpoints use arithmetic shifts, while their accumulated Q16 edge values differ.",
            },
            "vertical_interval": "min/max y clipped using F3 top (A909B4) and bottom (43A48C); finite fixtures determine endpoint inclusion",
            "horizontal_clip": "C2E0 body has direct F3 left/right references only if present in static_f3_absolute_reads; runtime surfaces also probe sentinel left/right values; screen bounds are separately coded as 0 and 320",
            "shared_edge": {
                "upper_count": len(upper_pixels), "lower_count": len(lower_pixels),
                "overlap_count": len(shared_overlap), "union_count": len(shared_union),
                "matches_2_to_8_half_open_rectangle": shared_union == expected_rectangle,
                "overlap_pixels": [list(p) for p in sorted(shared_overlap, key=lambda p: (p[1], p[0]))],
                "d60c_upper_count": len(d60_upper), "d60c_lower_count": len(d60_lower),
                "d60c_overlap_count": len(d60_shared_overlap), "d60c_union_count": len(d60_shared_union),
                "d60c_matches_2_to_8_half_open_rectangle": d60_shared_union == expected_rectangle,
                "c2e0_and_d60c_assign_shared_diagonal_to_same_triangle": upper_pixels == d60_upper,
            },
            "reverse_winding_same_coverage_as_baseline": reverse_pixels == base_pixels,
            "clipped_case_x_below_f3_left": any(x < 4 for x, _ in clipped_pixels),
            "clipped_case_x_above_f3_right": any(x >= 6 for x, _ in clipped_pixels),
            "clipped_case_rows_within_f3_top_bottom": all(3 <= y < 8 for _, y in clipped_pixels),
        },
        "coverage_comparisons_to_d60c": comparisons,
        "cases": {"c2e0_fill": c2_cases, "d60c_rowzero": d60_cases},
        "limits": [
            "Five finite discriminator families only; no full rasterizer or gameplay execution.",
            "D420 and C2E0 are executed as original x86 bytes with synthetic globals, source texture, destination surface, and stack.",
            "The paired D60C run establishes coverage differences only; its transparent zero-sample semantics differ from C2E0 fill semantics.",
            "No arbitrary overflow/coordinate domain, whole-function source identity, compiler match, GPU contract, or whole-module claim.",
        ],
        "invocation": {"script": str(Path(__file__).resolve()), "receipt": str(ARGS.receipt)},
    }
    write_receipt(ARGS.receipt, receipt)
    compact = {
        "schema_version": 1,
        "status": "STATIC_OBSERVED_AND_BOUNDED_ORIGINAL_BYTES",
        "oracle": receipt["oracle"],
        "prediction": {
            "prediction_sha256": receipt["prediction_sha256"],
            "primary_prediction": "one D420 source sample followed by one C2E0 fill; selected coverage matches D60C",
            "result": "route and fill confirmed; coverage-match portion falsified on shared-edge ownership and clipped row-7 endpoint",
            "followup_hypothesis": endpoint_precision["checked_hypothesis"],
            "followup_result": endpoint_precision["result"],
            "falsifier": "plain SAR16 at C2E0 endpoints and raw edge progressions of +0x10000 versus D60C +0xffff",
        },
        "route_counts": {
            "c2e0_cases": len(c2_cases),
            "paired_d60c_cases": len(d60_cases),
            "source_sample_reads_per_c2e0_case": sorted({case["sample_count"] for case in c2_cases.values()}),
            "c2e0_entries_per_c2e0_case": sorted({case["c2e0_entry_count"] for case in c2_cases.values()}),
            "fill_call_site": "D420 direct call at RVA 0xd600",
            "host_callback_claim": "not part of this route's evidence; the tested path directly calls in-image C2E0",
        },
        "representative_native_coverage": {
            "triangle_nonzero": {
                "changed_pixels": len(changed_pixels(c2_cases["triangle_nonzero"])),
                "surface_write_events": c2_cases["triangle_nonzero"]["surface_write_event_count"],
                "fill_values": c2_cases["triangle_nonzero"]["fill_values"],
            },
            "triangle_zero": {
                "changed_pixels": len(changed_pixels(c2_cases["triangle_zero"])),
                "surface_write_events": c2_cases["triangle_zero"]["surface_write_event_count"],
                "fill_values": c2_cases["triangle_zero"]["fill_values"],
            },
            "quad_irregular": {
                "changed_pixels": len(changed_pixels(c2_cases["quad_irregular"])),
                "surface_write_events": c2_cases["quad_irregular"]["surface_write_event_count"],
            },
            "shared_edge_pair": receipt["edge_and_clip_contract_observed"]["shared_edge"],
        },
        "endpoint_precision": endpoint_precision,
        "original_spans": [
            span for span in spans if span["name"] in (
                "D420 row normalization, sample and C2E0 dispatch",
                "D420 sampled-byte expression and C2E0 call",
                "C2E0 complete constant-fill polygon body",
                "C2E0 endpoint ordering and plain SAR16 conversion",
                "D60C complete row-zero comparison body",
            )
        ],
        "provenance": {
            "audit_script": receipt["fixture_inputs"]["driver_path"],
            "audit_script_sha256": receipt["fixture_inputs"]["driver_sha256"],
            "oracle_common": receipt["fixture_inputs"]["public_oracle_common_path"],
            "oracle_common_sha256": receipt["fixture_inputs"]["public_oracle_common_sha256"],
            "public_model_helpers": receipt["fixture_inputs"]["public_polygon_model_helpers_path"],
            "public_model_helpers_sha256": receipt["fixture_inputs"]["public_polygon_model_helpers_sha256"],
            "full_trace": str(ARGS.receipt),
            "full_trace_sha256": hashlib.sha256(ARGS.receipt.read_bytes()).hexdigest(),
        },
        "proof_boundary": "Finite original-byte fixtures only; no full rasterizer, arbitrary coordinate domain, host callback contract, source reconstruction, or canonical edit claim.",
        "invocation": {
            "script": str(Path(__file__).resolve()),
            "exe_argument": str(ARGS.exe),
            "receipt_argument": str(ARGS.receipt),
            "compact_receipt": str(ARGS.compact_receipt),
        },
    }
    write_receipt(ARGS.compact_receipt, compact)
    print(json.dumps({
        "status": receipt["status"],
        "c2e0_cases": len(c2_cases),
        "d60c_comparisons": len(d60_cases),
        "coverage_matches": {name: entry["c2e0_matches_d60c_coverage"] for name, entry in comparisons.items()},
        "shared_edge": receipt["edge_and_clip_contract_observed"]["shared_edge"],
        "reversed_winding_matches": receipt["edge_and_clip_contract_observed"]["reverse_winding_same_coverage_as_baseline"],
        "f3_static_cells": sorted({F3_CELLS[int(r["address"], 16)] for r in refs_c2e0}),
        "receipt": str(ARGS.receipt),
        "compact_receipt": str(ARGS.compact_receipt),
    }, indent=2))


if __name__ == "__main__":
    main()
