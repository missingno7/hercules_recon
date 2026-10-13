#!/usr/bin/env python3
"""Probe small original-PC reciprocal/edge boundaries; never rasterize a full frame.

Example (from the repository root):
  python renderer_contract/tests/test_original_reciprocal_domain.py \
    --exe work/discs/pc_install/HERCULES.EXE --original-root . \
    --receipt work/renderer_phase4/reciprocal_domain/native_boundary_receipt.json

The public fill-edge fixture is imported as a helper to reuse its pinned PE,
Unicorn, and synthetic-memory setup. This file only adds boundary cases and
independent arithmetic checks.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import sys
from pathlib import Path
from typing import Any


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def i32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value if value < 0x80000000 else value - 0x100000000


def trunc_divmod(dividend: int, divisor: int) -> tuple[int, int]:
    """Signed IDIV quotient/remainder, rounded toward zero as x86 specifies."""
    if divisor == 0:
        raise ZeroDivisionError("native IDIV fixture unexpectedly reached a zero divisor")
    quotient = abs(dividend) // abs(divisor)
    if (dividend < 0) != (divisor < 0):
        quotient = -quotient
    return quotient, dividend - quotient * divisor


def expected_reciprocal_table() -> list[int]:
    return [0] + [(1 << 30) // index for index in range(1, 2048)]


def parse_args() -> argparse.Namespace:
    here = Path(__file__).resolve()
    default_root = next((p for p in here.parents if (p / "scripts" / "pe.py").is_file()), None)
    if default_root is None:
        raise RuntimeError("cannot locate repository root; pass --original-root")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, default=default_root,
                        help="current repository/worktree root with scripts/pe.py and public tests")
    parser.add_argument("--exe", type=Path, required=True, help="pinned original HERCULES.EXE")
    parser.add_argument("--deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
    parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
    parser.add_argument("--receipt", type=Path,
                        help="output receipt path (relative paths are rooted at --original-root)")
    args = parser.parse_args()
    args.original_root = args.original_root.resolve()
    args.exe = args.exe.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if args.receipt is None:
        args.receipt = args.original_root / "work/renderer_bounded_oracle/reciprocal_domain.json"
    elif not args.receipt.is_absolute():
        args.receipt = args.original_root / args.receipt
    args.receipt = args.receipt.resolve()
    try:
        args.receipt.relative_to((args.original_root / "work").resolve())
    except ValueError:
        parser.error("--receipt must stay under <original-root>/work")
    if not (args.original_root / "scripts" / "pe.py").is_file():
        parser.error("--original-root must contain scripts/pe.py")
    if not args.exe.is_file() or not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("pinned EXE, Unicorn dependency, and analysis dependency paths must exist")
    return args


def load_public_fill_helper(args: argparse.Namespace):
    runner = args.original_root / "renderer_contract" / "tests" / "test_original_fill_edges.py"
    receipt = args.receipt
    compact = receipt.with_name("import_only_compact.json")
    original_argv = sys.argv[:]
    sys.argv = [str(runner), "--exe", str(args.exe), "--receipt", str(receipt),
                "--compact-receipt", str(compact), "--deps", str(args.deps),
                "--analysis-deps", str(args.analysis_deps)]
    spec = importlib.util.spec_from_file_location("public_fill_edge_fixture_helpers", runner)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import public fixture helper: {runner}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    try:
        spec.loader.exec_module(module)
    finally:
        sys.argv = original_argv
    return module


def compact_surface_result(result: dict[str, Any] | None) -> dict[str, Any] | None:
    if result is None:
        return None
    return {
        "case": result.get("case"),
        "route": result.get("route", result.get("requested_route")),
        "pixel_count": len(result.get("native_pixel_set", result.get("actual_pixel_coordinates_from_final_surface_diff", []))),
        "sample_count": result.get("sample_count", len(result.get("source_uv_reads", result.get("source_samples", [])))),
        "fill_values": result.get("fill_values"),
        "coverage_match": result.get("coverage_match", result.get("final_surface_diff_matches_integer_scan_model")),
        "original_code_unchanged": result.get("original_code_unchanged"),
        "trace_length": result.get("trace_length"),
    }


def capture_module_and_run(args: argparse.Namespace) -> dict[str, Any]:
    fill = load_public_fill_helper(args)
    poly = fill.poly
    from pe import PE  # noqa: PLC0415
    from unicorn.x86_const import (  # noqa: PLC0415
        UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
        UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_EDI,
    )

    runtime = fill.verify_runtime(poly.unicorn, args.deps)
    exe_bytes = args.exe.read_bytes()
    exe_hash = sha(exe_bytes)
    if exe_hash != fill.EXPECTED_EXE_SHA256:
        raise AssertionError(f"selected EXE SHA-256 is not pinned: {exe_hash}")
    pe = PE(args.exe)
    if pe.image_base != poly.BASE:
        raise AssertionError(f"unexpected image base {pe.image_base:#x}")
    data_region = fill.load_data_region(pe)

    # Use a 512-byte stride so a one-row width-319 fixture remains one row,
    # and a one-row probe at y=2046 remains inside bounded synthetic memory.
    poly.SURFACE_STRIDE = 512
    poly.SURFACE_SIZE = 512 * 2050
    poly.SCRATCH_SIZE = 0x200000
    if poly.SURFACE + poly.SURFACE_SIZE > poly.SCRATCH + poly.SCRATCH_SIZE:
        raise AssertionError("synthetic surface escapes bounded scratch mapping")

    TABLE_BYTES = poly.RECIP_COUNT * 4
    table_expected = expected_reciprocal_table()
    case_table_checks: list[dict[str, Any]] = []

    def capture_code_event(self, uc, address):
        rva = address - poly.BASE
        if rva == 0xD420 and self.table_init_check is None:
            table = bytes(uc.mem_read(poly.RECIP_TABLE, TABLE_BYTES))
            actual = [int.from_bytes(table[i * 4:i * 4 + 4], "little")
                      for i in range(poly.RECIP_COUNT)]
            mismatches = [i for i, (a, e) in enumerate(zip(actual, table_expected)) if a != e]
            self.table_init_check = {
                "entry_count": len(actual), "formula_match_count": len(actual) - len(mismatches),
                "mismatch_indices": mismatches[:8],
                "selected_entries": {str(i): hex(actual[i]) for i in (0, 1, 2, 319, 2047)},
                "initializer_instruction_count": len(self.reciprocal_init_trace),
                "initializer_write_count": len(self.reciprocal_table_writes),
            }
            case_table_checks.append(self.table_init_check)
        if rva in (0xD6CF, 0xD6E4, 0xD6F7, 0xC3CB, 0xC3DA, 0xC3EE):
            flags = uc.reg_read(UC_X86_REG_EFLAGS)
            signed_ge = ((flags >> 7) & 1) == ((flags >> 11) & 1)
            self.guard_events.append({
                "pc_rva": hex(rva), "jge_taken": bool(signed_ge),
                "registers": {"eax": hex(uc.reg_read(UC_X86_REG_EAX)),
                              "ebx": hex(uc.reg_read(UC_X86_REG_EBX)),
                              "ecx": hex(uc.reg_read(UC_X86_REG_ECX)),
                              "edx": hex(uc.reg_read(UC_X86_REG_EDX)),
                              "esi": hex(uc.reg_read(UC_X86_REG_ESI)),
                              "edi": hex(uc.reg_read(UC_X86_REG_EDI))},
            })
        if rva in (0xC4B8, 0xC560):
            divisor_reg = UC_X86_REG_ESI if rva == 0xC4B8 else UC_X86_REG_ECX
            low = uc.reg_read(UC_X86_REG_EAX)
            high = uc.reg_read(UC_X86_REG_EDX)
            dividend = ((high << 32) | low)
            if dividend & (1 << 63):
                dividend -= 1 << 64
            quotient, remainder = trunc_divmod(dividend, i32(uc.reg_read(divisor_reg)))
            self.division_events.append({
                "pc_rva": hex(rva), "divisor_register": "esi" if rva == 0xC4B8 else "ecx",
                "signed_divisor": i32(uc.reg_read(divisor_reg)),
                "dividend_signed64": dividend,
                "dividend_eax_hex": hex(low), "dividend_edx_hex": hex(high),
                "expected_quotient_trunc_zero": quotient,
                "expected_remainder_trunc_zero": remainder,
            })
        division_result_sites = {0xC4BA: "0xc4b8", 0xC562: "0xc560"}
        if rva in division_result_sites:
            self.division_results.append({
                "after_idiv_rva": division_result_sites[rva],
                "quotient_eax_signed32": i32(uc.reg_read(UC_X86_REG_EAX)),
                "remainder_edx_signed32": i32(uc.reg_read(UC_X86_REG_EDX)),
            })
        q30_sites = {
            0xD810: ("first_edge", "operands"), 0xD816: ("first_edge", "product"),
            0xD81A: ("first_edge", "shifted"), 0xD995: ("second_edge", "operands"),
            0xD99B: ("second_edge", "product"), 0xD99F: ("second_edge", "shifted"),
        }
        if rva in q30_sites:
            name, stage = q30_sites[rva]
            if stage == "operands":
                delta = i32(uc.reg_read(UC_X86_REG_EAX))
                reciprocal = int.from_bytes(bytes(uc.mem_read(0xA9097C, 4)), "little")
                self.q30_pending[name] = {"delta_signed32": delta, "reciprocal_u32": reciprocal,
                                          "expected_signed64_product": delta * i32(reciprocal)}
            elif stage == "product":
                low, high = uc.reg_read(UC_X86_REG_EAX), uc.reg_read(UC_X86_REG_EDX)
                product = (high << 32) | low
                if product & (1 << 63):
                    product -= 1 << 64
                pending = self.q30_pending[name]
                pending["actual_signed64_product"] = product
                pending["product_matches_independent"] = product == pending["expected_signed64_product"]
            else:
                pending = self.q30_pending[name]
                expected_bits = ((pending["expected_signed64_product"] >> 30) & 0xFFFFFFFF)
                pending["expected_shrd30_result_bits"] = hex(expected_bits)
                pending["actual_shrd30_result_bits"] = hex(uc.reg_read(UC_X86_REG_EAX))
                pending["shrd30_matches_independent"] = uc.reg_read(UC_X86_REG_EAX) == expected_bits
                self.q30_events.append({"step": name, **pending})

    def capture_read_event(self, uc, address, size):
        if poly.RECIP_TABLE <= address < poly.RECIP_TABLE + TABLE_BYTES:
            index = (address - poly.RECIP_TABLE) // 4
            pc = uc.reg_read(UC_X86_REG_EIP) - poly.BASE
            self.table_accesses.append({
                "pc_rva": hex(pc), "index": index, "address": hex(address), "size": size,
                "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little"),
            })

    class BoundaryCase(poly.NativeCase):
        def __init__(self, *a, **kw):
            self.table_accesses = []
            self.guard_events = []
            self.division_events = []
            self.division_results = []
            self.q30_pending = {}
            self.q30_events = []
            self.table_init_check = None
            super().__init__(*a, **kw)

        def _code(self, uc, address, size, user):
            capture_code_event(self, uc, address)
            super()._code(uc, address, size, user)

        def _read(self, uc, access, address, size, value, user):
            capture_read_event(self, uc, address, size)
            super()._read(uc, access, address, size, value, user)

    class C2BoundaryCase(fill.FillCase):
        def __init__(self, *a, **kw):
            self.table_accesses = []
            self.guard_events = []
            self.division_events = []
            self.division_results = []
            self.q30_pending = {}
            self.q30_events = []
            self.table_init_check = None
            super().__init__(*a, **kw)

        def _code(self, uc, address, size, user):
            # Run the same capture logic as BoundaryCase while preserving the
            # public FillCase's C2 code window and endpoint instrumentation.
            capture_code_event(self, uc, address)
            super()._code(uc, address, size, user)

        def _read(self, uc, access, address, size, value, user):
            capture_read_event(self, uc, address, size)
            super()._read(uc, access, address, size, value, user)

    def spec(name: str, vertices: list[tuple[int, int]], clip: tuple[int, int, int, int]):
        return {"name": name, "vertices": vertices, "clip": clip, "attributes": None}

    # The fixtures run only a selected scan row for tall geometry. Width-319
    # reaches one row of 319 pixels; no case loops over thousands of scan rows.
    specs = [
        ("d60c_dy0_flat", "d60c_rowzero", [(1, 10), (5, 10), (3, 10)], (0, 0, 320, 24), "d60c"),
        ("d60c_dy2047_last_slot", "d60c_rowzero", [(1, 0), (5, 0), (3, 2047)], (0, 2046, 320, 2047), "d60c"),
        ("d60c_dy2048_guard", "d60c_rowzero", [(1, 0), (5, 0), (3, 2048)], (0, 2047, 320, 2048), "d60c"),
        ("d60c_span0_vertical", "d60c_rowzero", [(5, 2), (5, 3), (5, 4)], (0, 2, 320, 4), "d60c"),
        ("d60c_span319_max_screen", "d60c_rowzero", [(0, 2), (319, 2), (159, 4)], (0, 2, 320, 3), "d60c"),
        ("d60c_xspan2047_pre_crop", "d60c_rowzero", [(1, 2), (2048, 2), (1, 4)], (0, 2, 320, 4), "d60c"),
        ("d60c_xspan2048_extent_guard", "d60c_rowzero", [(1, 2), (2049, 2), (1, 4)], (0, 2, 320, 4), "d60c"),
        ("c2_dy0_no_idiv", "c2e0_fill", [(1, 10), (5, 10), (3, 10)], (0, 0, 320, 24), "c2"),
        ("c2_dy2047_signed_idiv", "c2e0_fill", [(1, 0), (5, 0), (3, 2047)], (0, 2046, 320, 2047), "c2"),
        ("c2_dy2048_guard", "c2e0_fill", [(1, 0), (5, 0), (3, 2048)], (0, 2047, 320, 2048), "c2"),
        ("c2_xspan2047_pre_idiv", "c2e0_fill", [(1, 2), (2048, 2), (1, 4)], (0, 2, 320, 4), "c2"),
        ("c2_xspan2048_extent_guard", "c2e0_fill", [(1, 2), (2049, 2), (1, 4)], (0, 2, 320, 4), "c2"),
    ]

    case_rows = []
    initializer_cases = []
    for name, route, vertices, clip, kind in specs:
        fixture = spec(name, vertices, clip)
        if kind == "c2":
            case = C2BoundaryCase(pe, data_region, fixture, route, source_byte=0x5A)
            # Match the public finite fill fixture: snapshot the original .data
            # after synthetic globals are staged and before the initializer.
            case.code_data_snapshot = bytes(case.uc.mem_read(case.data_va, case.data_size))
        else:
            case = BoundaryCase(pe, data_region, fixture, route)
        status = "RETURNED"
        exception = None
        result = None
        try:
            result = case.run()
        except Exception as error:  # capture native stop/fault evidence rather than losing the case
            status = "EXCEPTION"
            exception = {"type": type(error).__name__, "message": str(error),
                         "error_record": case.error,
                         "eip_rva": hex(case.uc.reg_read(poly.UC_X86_REG_EIP) - poly.BASE)}
        if case.table_init_check is not None:
            initializer_cases.append(case.table_init_check)
            if case.table_init_check["formula_match_count"] != 2048 or case.table_init_check["initializer_write_count"] != 2048:
                raise AssertionError(f"original table initializer disagrees with independent model in {name}")
        case_rows.append({
            "name": name, "route": route, "vertices_xy": [list(v) for v in vertices], "clip": list(clip),
            "runner_outcome": status, "exception": exception,
            "instruction_count": len(case.trace),
            "returned_to_sentinel": bool(case.stopped),
            "guard_events": case.guard_events,
            "reciprocal_table_reads": case.table_accesses,
            "c2_signed_idiv_events": case.division_events,
            "c2_signed_idiv_results": case.division_results,
            "d60c_q30_multiply_shift_events": case.q30_events,
            "surface_write_count": len(case.surface_writes),
            "source_sample_count": len(case.source_reads),
            "source_sample_values": [item["value"] for item in case.source_reads],
            "pixel_count": len(result.get("native_pixel_set", result.get("actual_pixel_coordinates_from_final_surface_diff", []))) if result else None,
            "original_code_unchanged": result.get("original_code_unchanged") if result else None,
            "path_markers": result.get("path_markers", []) if result else [],
            "path_rvas_tail": [hex(x) for x in case.trace[-10:]],
        })

    if len(initializer_cases) != len(specs):
        raise AssertionError(f"only {len(initializer_cases)}/{len(specs)} cases reached D420 after table initialization")
    if any(row["runner_outcome"] != "RETURNED" or not row["returned_to_sentinel"] for row in case_rows):
        raise AssertionError("one or more bounded fixtures did not return normally to the sentinel")
    for row in case_rows:
        for access in row["reciprocal_table_reads"]:
            index = access["index"]
            access["use"] = "span_width" if access["pc_rva"] == "0xdb8e" else "edge_delta_y"
            access["expected_value_from_independent_formula"] = table_expected[index]
            access["formula_match"] = access["value"] == table_expected[index]
            if not access["formula_match"]:
                raise AssertionError(f"native table read disagrees at index {index}")
        for result in row["c2_signed_idiv_results"]:
            source = next((event for event in row["c2_signed_idiv_events"]
                           if event["pc_rva"] == result["after_idiv_rva"]), None)
            if source is None:
                raise AssertionError(f"IDIV result has no preceding dividend record: {result}")
            result["quotient_matches_independent"] = result["quotient_eax_signed32"] == source["expected_quotient_trunc_zero"]
            result["remainder_matches_independent"] = result["remainder_edx_signed32"] == source["expected_remainder_trunc_zero"]
            if not result["quotient_matches_independent"] or not result["remainder_matches_independent"]:
                raise AssertionError(f"native signed IDIV differs from independent trunc-zero model: {result}")
        for event in row["d60c_q30_multiply_shift_events"]:
            if not event.get("product_matches_independent") or not event.get("shrd30_matches_independent"):
                raise AssertionError(f"native IMUL/SHRD differs from independent 64-bit model: {event}")

    case_by_name = {row["name"]: row for row in case_rows}
    def index_list(case_name: str, use: str | None = None) -> list[int]:
        reads = case_by_name[case_name]["reciprocal_table_reads"]
        if use is not None:
            reads = [r for r in reads if r["use"] == use]
        return [r["index"] for r in reads]
    def guard_taken(case_name: str, pc: str) -> bool:
        return any(event["pc_rva"] == pc and event["jge_taken"] for event in case_by_name[case_name]["guard_events"])

    assertions = [
        {"name": "original_initializer_all_entries", "pass": all(x["formula_match_count"] == 2048 and x["initializer_write_count"] == 2048 for x in initializer_cases),
         "observed": "all 2048 initialized entries match R[0]=0; floor(2^30/i) for i=1..2047 in each isolated emulator"},
        {"name": "d60c_dy0_skips_reciprocal", "pass": not index_list("d60c_dy0_flat") and case_by_name["d60c_dy0_flat"]["surface_write_count"] == 0,
         "observed": "flat zero-height D60C fixture returns without table reads or writes"},
        {"name": "d60c_dy2047_uses_last_entry", "pass": index_list("d60c_dy2047_last_slot") == [2047,2047] and all(x["value"] == 0x80100 for x in case_by_name["d60c_dy2047_last_slot"]["reciprocal_table_reads"]),
         "observed": "two edge lookups use R[2047]=0x80100; this clipped high-Y fixture makes no surface writes"},
        {"name": "d60c_dy2048_guard", "pass": guard_taken("d60c_dy2048_guard", "0xd6cf") and not index_list("d60c_dy2048_guard"),
         "observed": "signed JGE extent guard taken at D6CF before any reciprocal lookup"},
        {"name": "d60c_zero_width_span_skips_span_lookup", "pass": not index_list("d60c_span0_vertical", "span_width") and case_by_name["d60c_span0_vertical"]["surface_write_count"] == 0,
         "observed": "vertical zero-width fixture can read edge reciprocals but skips DB8E span lookup"},
        {"name": "d60c_span319_reads_last_screen_width", "pass": index_list("d60c_span319_max_screen", "span_width") == [319] and case_by_name["d60c_span319_max_screen"]["pixel_count"] == 319,
         "observed": "one-row width-319 fixture reads R[319] and emits 319 pixels"},
        {"name": "d60c_span2047_lookup_precedes_crop", "pass": index_list("d60c_xspan2047_pre_crop", "span_width") == [2047,1023] and case_by_name["d60c_xspan2047_pre_crop"]["pixel_count"] == 638,
         "observed": "wide fixture reads both row widths before visible output is cropped; 2047 is not rejected by the extent guard"},
        {"name": "d60c_span2048_extent_guard", "pass": guard_taken("d60c_xspan2048_extent_guard", "0xd6e4") and not index_list("d60c_xspan2048_extent_guard"),
         "observed": "X extent 2048 takes the D6E4 guard before table lookup"},
        {"name": "c2_dy0_avoids_idiv", "pass": not case_by_name["c2_dy0_no_idiv"]["c2_signed_idiv_events"] and case_by_name["c2_dy0_no_idiv"]["runner_outcome"] == "RETURNED",
         "observed": "horizontal zero-height fixture returns without IDIV or divide fault"},
        {"name": "c2_dy2047_signed_idiv", "pass": [x["signed_divisor"] for x in case_by_name["c2_dy2047_signed_idiv"]["c2_signed_idiv_events"]] == [2047,2047] and [x["dividend_signed64"] for x in case_by_name["c2_dy2047_signed_idiv"]["c2_signed_idiv_events"]] == [131072,-131072] and [(x["quotient_eax_signed32"], x["remainder_edx_signed32"]) for x in case_by_name["c2_dy2047_signed_idiv"]["c2_signed_idiv_results"]] == [(64,64),(-64,-64)],
         "observed": "two signed divisions by 2047 use opposite signed Q16 x deltas"},
        {"name": "c2_dy2048_guard", "pass": guard_taken("c2_dy2048_guard", "0xc3cb") and not case_by_name["c2_dy2048_guard"]["c2_signed_idiv_events"],
         "observed": "Y extent 2048 takes C3CB guard before IDIV"},
        {"name": "c2_xspan2047_shift_is_in_range", "pass": not guard_taken("c2_xspan2047_pre_idiv", "0xc3da") and any(x["dividend_signed64"] == -2047 * 65536 for x in case_by_name["c2_xspan2047_pre_idiv"]["c2_signed_idiv_events"]) and abs(-2047 * 65536) < 0x80000000,
         "observed": "X extent 2047 reaches IDIV; its negative SHL-16 dividend remains signed-32 representable"},
        {"name": "c2_xspan2048_guard", "pass": guard_taken("c2_xspan2048_extent_guard", "0xc3da") and not case_by_name["c2_xspan2048_extent_guard"]["c2_signed_idiv_events"],
         "observed": "X extent 2048 takes C3DA guard before IDIV"},
    ]
    q30_boundary = case_by_name["d60c_dy2047_last_slot"]["d60c_q30_multiply_shift_events"]
    q30_wide = case_by_name["d60c_xspan2047_pre_crop"]["d60c_q30_multiply_shift_events"]
    assertions.extend([
        {"name": "d60c_q30_2047_edge_steps", "pass": [(x["delta_signed32"], x["actual_shrd30_result_bits"]) for x in q30_boundary] == [(131072,"0x40"),(-131072,"0xffffffbf")],
         "observed": "original IMUL then SHRD-30 outputs agree with signed 64-bit products for the dy=2047 pair"},
        {"name": "d60c_q30_wide_edge_step", "pass": any(x["delta_signed32"] == -134152192 and x["actual_shrd30_result_bits"] == "0xfc008000" for x in q30_wide),
         "observed": "accepted |deltaX|=2047 edge step agrees with explicit signed product and SHRD-30 bit extraction"},
    ])
    if not all(row["pass"] for row in assertions):
        failed = [row for row in assertions if not row["pass"]]
        raise AssertionError(f"boundary assertion failed: {failed}")

    # STATIC_OBSERVED code hashes and selected instructions keep this receipt
    # compact while exposing the bounds that make each probe interpretable.
    import capstone  # noqa: PLC0415
    static_ranges = {
        "reciprocal_initializer": (0x72A0, 0x72CC),
        "CC50_body": (0xCC50, 0xD415),
        "D60C_rowzero": (0xD60C, 0xDD70),
        "DD70_body": (0xDD70, 0xE675),
        "E680_body": (0xE680, 0xEF60),
        "C2E0_fill": (0xC2E0, 0xC6F8),
    }
    static_spans = {}
    for name, (start, end) in static_ranges.items():
        code = pe.read_rva(start, end - start)
        static_spans[name] = {"module": "HERCULES.EXE", "start_rva": hex(start),
                              "end_rva_exclusive": hex(end), "size": len(code),
                              "sha256": sha(code), "proof_level": "STATIC_OBSERVED"}
    site_map = {
        "D60C": [0xD6C7,0xD6C9,0xD6CF,0xD6DC,0xD6DE,0xD6E4,0xD6EA,0xD6EC,0xD6F2,0xD6F7,
                 0xD7F3,0xD7FE,0xD805,0xD80B,0xD810,0xD816,0xD81A,
                 0xD978,0xD983,0xD98A,0xD990,0xD995,0xD99B,0xD99F,
                 0xDB6E,0xDB74,0xDB7C,0xDB82,0xDB88,0xDB8E],
        "CC50": [0xCD42,0xCD48,0xCD55,0xCD57,0xCD5D,0xCD63,0xCD65,0xCE6C,0xCE77,
                 0xCFF1,0xCFFC,0xD1F5,0xD1FB,0xD201,0xD207],
        "DD70": [0xDF7C,0xDF7E,0xDF86,0xDF91,0xDF93,0xDF95,0xDF9B,0xDFA1,
                 0xE0AA,0xE0B5,0xE22F,0xE23A,0xE433,0xE439,0xE43F,0xE445],
        "E680": [0xE889,0xE88B,0xE893,0xE899,0xE8A0,0xE8A2,0xE8A8,0xE8AE,
                 0xE9B7,0xE9C2,0xEB3C,0xEB47,0xED40,0xED46,0xED4C,0xED52],
        "C2E0": [0xC3C2,0xC3C4,0xC3C6,0xC3CB,0xC3D1,0xC3D3,0xC3D5,0xC3DA,0xC3E0,0xC3E2,0xC3E8,
                 0xC4AD,0xC4B4,0xC4B7,0xC4B8,0xC4BA,0xC555,0xC55C,0xC55F,0xC560,0xC562],
    }
    static_sites = {}
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for family, addresses in site_map.items():
        low, high = {"D60C":(0xD60C,0xDD70),"CC50":(0xCC50,0xD415),"DD70":(0xDD70,0xE675),
                     "E680":(0xE680,0xEF60),"C2E0":(0xC2E0,0xC6F8)}[family]
        by_address = {ins.address - poly.BASE: ins for ins in md.disasm(pe.read_rva(low, high-low), poly.BASE+low)}
        static_sites[family] = []
        for rva in addresses:
            ins = by_address.get(rva)
            if ins is None:
                raise AssertionError(f"static disassembly did not decode {family} RVA {rva:#x}")
            static_sites[family].append({"rva": hex(rva), "bytes": ins.bytes.hex(),
                                         "mnemonic": ins.mnemonic, "operands": ins.op_str})

    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_OBSERVED",
        "runner_status": "BOUNDED_ORIGINAL_BYTES",
        "proof_scope": "Finite original HERCULES.EXE boundary probes using synthetic vertices, surface, texture and globals; no whole raster or game execution.",
        "prediction_source": "renderer_contract/reciprocal_domain.md",
        "oracle": {"path": str(args.exe), "sha256": exe_hash, "image_base": hex(pe.image_base)},
        "runtime": runtime,
        "analysis": {"capstone_version": capstone.__version__, "pe_parser": "scripts/pe.py",
                     "public_fixture_helpers": "renderer_contract/tests/test_original_fill_edges.py and test_original_polygon_edges.py"},
        "source_inputs": {
            "public_polygon_model": {"path": "renderer_contract/tests/test_original_polygon_edges.py",
                                     "sha256": sha((args.original_root / "renderer_contract/tests/test_original_polygon_edges.py").read_bytes())},
            "public_fill_fixture": {"path": "renderer_contract/tests/test_original_fill_edges.py",
                                    "sha256": sha((args.original_root / "renderer_contract/tests/test_original_fill_edges.py").read_bytes())},
            "phase3_polygon_receipt": {"path": "renderer_contract/evidence/native_polygon_edges.json",
                                       "sha256": sha((args.original_root / "renderer_contract/evidence/native_polygon_edges.json").read_bytes())},
            "phase3_fill_receipt": {"path": "renderer_contract/evidence/native_fill_edges.json",
                                    "sha256": sha((args.original_root / "renderer_contract/evidence/native_fill_edges.json").read_bytes())},
        },
        "reciprocal_table": {
            "address": hex(poly.RECIP_TABLE), "entries": poly.RECIP_COUNT,
            "initializer_span": static_spans["reciprocal_initializer"],
            "independent_formula": "R[0]=0; R[i]=floor(2^30/i) for i=1..2047 (Python integer division, checked against all initialized entries).",
            "initializer_formula_pass_count": sum(item["formula_match_count"] for item in initializer_cases),
            "initializer_formula_check_count": len(initializer_cases) * poly.RECIP_COUNT,
            "per_case_checks": initializer_cases,
            "note": "Runtime table values are synthetic initialized data, not original-code spans or a canonical data table claim.",
        },
        "static_spans": static_spans,
        "static_sites": static_sites,
        "cases": case_rows,
        "assertions": assertions,
        "arithmetic_contract": {
            "table_indexing": "D60C/CC50/DD70/E680 use an 11-bit masked index at the recorded loads; dynamic D60C probes record address-derived indices and values.",
            "table_edge_step": "D60C dynamically records signed 32-bit delta, signed IMUL 64-bit product and SHRD 30 output; independent arithmetic is finite and boundary-specific. CC50/DD70/E680 are statically observed at analogous masked loads only.",
            "C2E0_edge_step": "CDQ/IDIV signed division after 32-bit SHL EAX,16; divide events and post-IDIV quotient/remainder are checked for finite accepted inputs.",
            "overflow_scope": "No claim about arbitrary wrapping. The accepted X-domain and delta-shift boundary are checked only for listed fixtures; values outside the guard/clamp domain are recorded as rejected paths.",
        },
        "limits": [
            "No 2,048-row frame was rasterized: tall cases clip to one scan row and wide cases are at most 319 pixels or exit before the span loop.",
            "D60C, CC50, DD70 and E680 are covered statically; dynamic D60C and C2E0 boundary probes do not establish identical behavior of the four masked-table consumers.",
            "C2E0 source-byte fills are only probed for edge-step boundary behavior; no general constant-fill implementation is inferred.",
            "No GPU, SDL, host callback, gameplay, OS endpoint, asset, canonical source, or Git state was modified.",
        ],
        "invocation": {"receipt": str(args.receipt), "instruction_budget_per_case": poly.MAX_INSNS,
                       "case_count": len(case_rows), "emulator_instances": len(case_rows)},
    }
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": str(args.receipt), "status": receipt["status"],
                      "runner_status": receipt["runner_status"], "cases": len(case_rows),
                      "initializer_checks": receipt["reciprocal_table"]["initializer_formula_check_count"],
                      "case_outcomes": {row["name"]: {"outcome": row["runner_outcome"],
                          "table_indices": [x["index"] for x in row["reciprocal_table_reads"]],
                          "idiv_divisors": [x["signed_divisor"] for x in row["c2_signed_idiv_events"]],
                          "pixels": row["pixel_count"]} for row in case_rows}}, indent=2))
    return receipt


def main() -> int:
    args = parse_args()
    sys.path[:0] = [str(args.analysis_deps), str(args.deps), str(args.original_root / "scripts")]
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    capture_module_and_run(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
