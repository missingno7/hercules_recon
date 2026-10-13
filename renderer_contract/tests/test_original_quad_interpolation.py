"""Finite original-byte four-vertex interpolation discriminator."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import sys
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime  # noqa: E402

ARGS = parse_args("Compare original quad edge/span, bilinear, and triangle UV models",
                  "quad_interpolation_native.json")
EXPLICIT_FLAGS = {argument.split("=", 1)[0] for argument in sys.argv[1:]}
missing_flags = {"--exe", "--receipt"} - EXPLICIT_FLAGS
if missing_flags:
    raise SystemExit("pass explicit " + " and ".join(sorted(missing_flags)))
if ARGS.original_root.resolve() != ROOT.resolve():
    raise SystemExit(f"--original-root must name {ROOT}")
# Reuse the established PE mapper, reciprocal initializer, clipped edge/span
# model, and NativeCase memory hooks. Importing this public harness does not
# execute its main() and its argument spelling is identical.
POLYGON_TEST = ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"
sys.path.insert(0, str(POLYGON_TEST.parent))
spec = importlib.util.spec_from_file_location("public_polygon_edge_helpers", POLYGON_TEST)
if spec is None or spec.loader is None:
    raise RuntimeError("could not load established polygon-edge oracle helpers")
geom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(geom)

from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import UcError, UC_HOOK_MEM_READ  # noqa: E402
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EBP, UC_X86_REG_EFLAGS, UC_X86_REG_ESP,
)

BASE = 0x400000
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
MODE_CELL = 0xA90A00
TPAGE_CELL = 0xA8E6F8
BLEND_BASE = 0x92E3D0
MAX_INSNS = 250000
VERTICES = [(1, 1), (5, 1), (5, 5), (1, 5)]
UV_BYTES = [(0, 0), (4, 0), (7, 7), (0, 4)]
UV_Q16 = [(u << 16, v << 16) for u, v in UV_BYTES]
CLIP = (0, 0, 320, 240)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def pixel_order(pixels):
    return sorted((tuple(p) for p in pixels), key=lambda p: (p[1], p[0]))


def q16_byte(value: Fraction) -> int:
    return (value // 65536) & 0xFF


def exact_triangle_uv(vertices, attrs, pixel):
    area = geom.cross(vertices[0], vertices[1], vertices[2])
    if area == 0:
        return None
    p = (Fraction(pixel[0]), Fraction(pixel[1]))
    weights = (
        geom.cross(vertices[1], vertices[2], p) / area,
        geom.cross(vertices[2], vertices[0], p) / area,
        geom.cross(vertices[0], vertices[1], p) / area,
    )
    if any(weight < 0 for weight in weights):
        return None
    uv = tuple(sum(weights[i] * attrs[i][component] for i in range(3))
               for component in range(2))
    return tuple(q16_byte(value) for value in uv)


def triangle_split_uv(vertices, attrs, pixel, diagonal):
    if diagonal == "0-2":
        triangles = ((0, 1, 2), (0, 2, 3))
    else:
        triangles = ((0, 1, 3), (1, 2, 3))
    for indices in triangles:
        result = exact_triangle_uv([vertices[i] for i in indices],
                                  [attrs[i] for i in indices], pixel)
        if result is not None:
            return result
    return None


def bilinear_uv(vertices, attrs, pixel):
    left, right = min(v[0] for v in vertices), max(v[0] for v in vertices)
    top, bottom = min(v[1] for v in vertices), max(v[1] for v in vertices)
    s = Fraction(pixel[0] - left, right - left)
    t = Fraction(pixel[1] - top, bottom - top)
    weights = ((1 - s) * (1 - t), s * (1 - t), s * t, (1 - s) * t)
    uv = tuple(sum(weights[i] * attrs[i][component] for i in range(4))
               for component in range(2))
    return tuple(q16_byte(value) for value in uv)


def model_rows(pixels):
    rows = {}
    for p in pixels:
        rows[p] = {
            "edge_span_fixed_q16": geom.expected_uv_integer_reciprocal(
                VERTICES, UV_Q16, p),
            "bilinear_rectangle": bilinear_uv(VERTICES, UV_Q16, p),
            "triangle_diagonal_0_2": triangle_split_uv(
                VERTICES, UV_Q16, p, "0-2"),
            "triangle_diagonal_1_3": triangle_split_uv(
                VERTICES, UV_Q16, p, "1-3"),
        }
    return rows


def configure_quad(route: str, pe: PE, data_region):
    case_spec = {
        "name": f"quad_{route}", "vertices": VERTICES, "clip": CLIP,
        "uv_mode": "fractional_gradient",
        # Do not pass attributes to the inherited triangle-only diagnostic.
        # The four input Q16 lanes are installed explicitly below and compared
        # here with independent quad-specific models.
    }
    native = geom.NativeCase(pe, data_region, case_spec, route)
    for i, (u, v) in enumerate(UV_Q16):
        geom.put32(native.uc, geom.U_BASE + 4 * i, u)
        geom.put32(native.uc, geom.V_BASE + 4 * i, v)
    return native


def run_d420(route: str, pe: PE, data_region):
    native = configure_quad(route, pe, data_region)
    result = native.run()
    expected_entry = 0xD60C if route == "d60c_rowzero" else 0xCC50
    if expected_entry not in native.trace:
        raise AssertionError(f"{route}: expected its generic D420/CC50 route; trace={native.trace[-24:]}")
    if 0xD549 in native.trace:
        raise AssertionError(f"{route}: rectangle direct-copy shortcut unexpectedly matched")
    pixels = pixel_order(result["native_pixel_set"])
    reads = result["source_uv_reads"]
    if len(reads) != len(pixels) or len(pixels) != 16:
        raise AssertionError(f"{route}: expected 16 one-byte source reads/writes, got {len(reads)}/{len(pixels)}")
    if not result["coverage_match"]:
        raise AssertionError(f"{route}: 4x4 coverage differs from the independent edge coverage model")
    uv_by_pixel = {p: (read["u"], read["v"]) for p, read in zip(pixels, reads)}
    return {
        "route": route,
        "entry_rva": result["entry_rva"],
        "trace_summary": {
            "instruction_count": len(native.trace),
            "rva_sequence_sha256": sha(json.dumps(native.trace, separators=(",", ":")).encode("ascii")),
            "path_rvas": [hex(rva) for rva in native.trace
                          if rva in (0xCC50, 0xD420, 0xD549, 0xD555, 0xD583,
                                     0xD589, 0xD60C, 0xDC8A, 0xDC96)],
        },
        "path_markers": result["path_markers"],
        "coverage_match": result["coverage_match"],
        "pixel_count": len(pixels),
        "pixels": [[x, y] for x, y in pixels],
        "source_reads": [{"pixel": list(p), "u": uv_by_pixel[p][0], "v": uv_by_pixel[p][1],
                          "pc": reads[i]["pc"], "texture_offset": hex(reads[i]["offset"]),
                          "source_value": reads[i]["value"]}
                         for i, p in enumerate(pixels)],
        "source_uv_by_pixel": uv_by_pixel,
        "surface_write_count": result["surface_write_count"],
        "palette_read_count": len(result["palette_reads"]),
        "original_code_unchanged": result["original_code_unchanged"],
    }


def initialize_reciprocal_table(native):
    native.uc.reg_write(UC_X86_REG_ESP, geom.FRAME)
    native.uc.reg_write(UC_X86_REG_EBP, 0)
    native.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    try:
        native.uc.emu_start(BASE + geom.RECIP_INIT_START, 0,
                            timeout=2_000_000, count=32768)
    except UcError as error:
        if native.error is None:
            native.error = {"kind": "reciprocal_initializer_error", "detail": str(error)}
    if native.error or not native.reciprocal_init_stopped:
        raise AssertionError(f"E680 reciprocal initializer failed: {native.error!r}")
    table = bytes(native.uc.mem_read(geom.RECIP_TABLE, geom.RECIP_COUNT * 4))
    if int.from_bytes(table[0:4], "little") != 0:
        raise AssertionError("reciprocal table[0] initialization mismatch")
    for index in (1, 2, 3, geom.RECIP_COUNT - 1):
        got = int.from_bytes(table[index * 4:index * 4 + 4], "little")
        if got != (0x40000000 // index):
            raise AssertionError(f"reciprocal table[{index}] mismatch")
    native.trace.clear()


def run_e680(pe: PE, data_region):
    # E680 generic fallback is a separate original dispatcher/body, so extend
    # the existing harness's bounded execution allow-list by the exact E680
    # body and its known fast-path callee. The failed quad predicate must keep
    # execution away from that callee.
    geom.CODE_WINDOWS = tuple(geom.CODE_WINDOWS) + ((0xE680, 0xEF60), (0xC8F0, 0xCAA8))
    native = configure_quad("d60c_rowzero", pe, data_region)
    code_spans = ((0xE680, 0xEF60), (0xC8F0, 0xCAA8))
    native.code_snapshot.update({
        (start, end): bytes(native.uc.mem_read(BASE + start, end - start))
        for start, end in code_spans
    })
    initialize_reciprocal_table(native)

    # The generic mode-0x80 path indexes its blend table by old-destination
    # byte and source texel. Seed only the 58 bytes reachable from the chosen
    # synthetic source range and initial 0xa5 surface; all state is emulator-only.
    geom.put32(native.uc, MODE_CELL, 0x80)
    geom.put32(native.uc, TPAGE_CELL, 2)
    selector_base = BLEND_BASE + (2 << 16)
    blend_values = {}
    for source in range(1, 59):
        address = selector_base + (source << 8) + 0xA5
        value = (0x80 + source) & 0xFF
        native.uc.mem_write(address, bytes([value]))
        blend_values[address] = value
    blend_reads = []

    def capture_blend_read(uc, _access, address, size, _value, _user):
        if selector_base <= address < selector_base + 0x10000:
            blend_reads.append({
                "pc": hex(uc.reg_read(geom.UC_X86_REG_EIP) - BASE),
                "address": hex(address), "size": size,
                "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little"),
            })

    native.uc.hook_add(UC_HOOK_MEM_READ, capture_blend_read)
    native.code_snapshot[(0xE680, 0xEF60)] = bytes(native.uc.mem_read(BASE + 0xE680, 0xEF60 - 0xE680))
    native.uc.reg_write(UC_X86_REG_ESP, geom.FRAME)
    native.uc.reg_write(UC_X86_REG_EBP, 0)
    native.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    geom.put32(native.uc, geom.FRAME, geom.SENTINEL)
    try:
        native.uc.emu_start(BASE + 0xE680, 0, timeout=2_000_000, count=MAX_INSNS)
    except UcError as error:
        if native.error is None:
            native.error = {"kind": "unicorn_error", "detail": str(error),
                            "rva": hex(native.uc.reg_read(geom.UC_X86_REG_EIP) - BASE)}
    if native.error or not native.stopped:
        raise AssertionError(f"E680 generic quad failed: error={native.error!r}; trace={native.trace[-30:]}")
    if 0xE7C7 not in native.trace or 0xE7BB in native.trace:
        raise AssertionError(f"E680 did not use generic fallback: trace={native.trace[-40:]}")
    for (start, end), before in native.code_snapshot.items():
        if bytes(native.uc.mem_read(BASE + start, end - start)) != before:
            raise AssertionError(f"original code changed at {start:#x}..{end:#x}")

    actual_writes = native.surface_writes
    pixels = pixel_order([(event["offset"] % geom.SURFACE_STRIDE,
                           event["offset"] // geom.SURFACE_STRIDE)
                          for event in actual_writes])
    source_reads = native.source_reads
    if len(source_reads) != 16 or len(actual_writes) != 16 or len(pixels) != 16:
        raise AssertionError(f"E680 expected 16 reads/writes; got reads={len(source_reads)} "
                             f"writes={len(actual_writes)} pixels={len(pixels)}")
    if len(blend_reads) != 16 or any(
            int(read["address"], 16) not in blend_values or
            read["value"] != blend_values[int(read["address"], 16)]
            for read in blend_reads):
        raise AssertionError(f"E680 blend lookups did not use the seeded selector-2 entries: {blend_reads!r}")
    expected = pixel_order(geom.native_edge_model_expected(VERTICES, CLIP))
    if pixels != expected:
        raise AssertionError(f"E680 4x4 coverage mismatch: {pixels!r} != {expected!r}")
    uv_by_pixel = {p: (read["u"], read["v"]) for p, read in zip(pixels, source_reads)}
    return {
        "route": "e680_mode_0x80_selector_2_generic",
        "entry_rva": "0xe680",
        "trace_summary": {
            "instruction_count": len(native.trace),
            "rva_sequence_sha256": sha(json.dumps(native.trace, separators=(",", ":")).encode("ascii")),
            "path_rvas": [hex(rva) for rva in native.trace
                          if rva in (0xE680, 0xE6CD, 0xE7BB, 0xE7C7, 0xEF59, 0xEF5B, 0xEF5F)],
        },
        "path_markers": {"generic_fallback_e7c7": True, "fastpath_e7bb": False},
        "coverage_match": True, "pixel_count": len(pixels),
        "pixels": [[x, y] for x, y in pixels],
        "source_reads": [{"pixel": list(p), "u": uv_by_pixel[p][0], "v": uv_by_pixel[p][1],
                          "pc": reads["pc"], "texture_offset": hex(reads["offset"]),
                          "source_value": reads["value"]}
                         for p, reads in zip(pixels, source_reads)],
        "source_uv_by_pixel": uv_by_pixel,
        "surface_write_count": len(actual_writes),
        "blend_table_read_count": len(blend_reads),
        "blend_reads": blend_reads,
        "synthetic_blend_entries_seeded": len(blend_values),
        "original_code_unchanged": True,
    }


def compare_models(routes):
    rows = model_rows(pixel_order(routes[0]["source_uv_by_pixel"].keys()))
    comparison = {}
    for route in routes:
        native = route["source_uv_by_pixel"]
        comparison[route["route"]] = {
            "sample_count": len(native),
            "edge_span_match_count": sum(native[p] == rows[p]["edge_span_fixed_q16"] for p in rows),
            "bilinear_match_count": sum(native[p] == rows[p]["bilinear_rectangle"] for p in rows),
            "triangle_0_2_match_count": sum(native[p] == rows[p]["triangle_diagonal_0_2"] for p in rows),
            "triangle_1_3_match_count": sum(native[p] == rows[p]["triangle_diagonal_1_3"] for p in rows),
        }
    first = {}
    first_pixel_order = pixel_order(rows)
    for name in ("bilinear_rectangle", "triangle_diagonal_0_2", "triangle_diagonal_1_3"):
        point = next((p for p in first_pixel_order
                      if rows[p]["edge_span_fixed_q16"] != rows[p][name]), None)
        first[name] = None if point is None else {
            "pixel": list(point), "edge_span_fixed_q16": rows[point]["edge_span_fixed_q16"],
            "competing_model_uv": rows[point][name],
        }
    first_native = {}
    first_native_alternatives = {}
    for route in routes:
        native = route["source_uv_by_pixel"]
        point = next((p for p in first_pixel_order
                      if native[p] != rows[p]["edge_span_fixed_q16"]), None)
        first_native[route["route"]] = None if point is None else {
            "pixel": list(point), "native_uv": native[point],
            "edge_span_uv": rows[point]["edge_span_fixed_q16"],
        }
        first_native_alternatives[route["route"]] = {}
        for name in ("bilinear_rectangle", "triangle_diagonal_0_2", "triangle_diagonal_1_3"):
            mismatch = next((p for p in first_pixel_order if native[p] != rows[p][name]), None)
            first_native_alternatives[route["route"]][name] = None if mismatch is None else {
                "pixel": list(mismatch), "native_uv": native[mismatch],
                "model_uv": rows[mismatch][name],
            }
    return {"models_by_pixel": {f"{x},{y}": rows[(x, y)] for x, y in first_pixel_order},
            "match_counts_by_route": comparison,
            "first_edge_span_vs_alternative": first,
            "first_native_vs_edge_span": first_native,
            "first_native_vs_alternative": first_native_alternatives}


def main():
    pe = PE(ARGS.exe)
    if sha(pe.data) != EXE_SHA256 or pe.image_base != BASE:
        raise AssertionError("pinned original EXE identity/base mismatch")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("unexpected original relocation directory")
    data_sec = next(s for s in pe.sections if s["name"] == ".data")
    data_region = (BASE + data_sec["virtual_address"],
                   max(data_sec["virtual_size"], data_sec["raw_size"]))
    runtime = verify_runtime(unicorn, ARGS.deps)
    routes = [run_d420("d60c_rowzero", pe, data_region),
              run_d420("cc50_palette", pe, data_region),
              run_e680(pe, data_region)]
    span_specs = {
        "D420 entry/row dispatcher": (0xD420, 0xDD70),
        "D60C row-zero scan converter": (0xD60C, 0xDD70),
        "CC50 palette scan converter": (0xCC50, 0xD415),
        "E680 mode-0x80 generic dispatcher/body": (0xE680, 0xEF60),
        "C8F0 fast rectangle callee (not reached by this case)": (0xC8F0, 0xCAA8),
        "reciprocal initializer": (0x72A0, 0x72CC),
    }
    original_spans = {
        name: {"rva": hex(start), "end_rva_exclusive": hex(end),
               "size": end - start, "sha256": sha(pe.read_rva(start, end - start))}
        for name, (start, end) in span_specs.items()
    }
    models = compare_models(routes)
    route_streams = {route["route"]: route["source_uv_by_pixel"] for route in routes}
    route_equality = {
        "rowzero_equals_palette": route_streams["d60c_rowzero"] == route_streams["cc50_palette"],
        "rowzero_equals_e680": route_streams["d60c_rowzero"] == route_streams["e680_mode_0x80_selector_2_generic"],
        "palette_equals_e680": route_streams["cc50_palette"] == route_streams["e680_mode_0x80_selector_2_generic"],
    }
    for route_name, counts in models["match_counts_by_route"].items():
        if counts["sample_count"] != 16 or counts["edge_span_match_count"] != 16:
            raise AssertionError(f"{route_name}: native source reads did not match the fixed-Q16 edge/span model: {counts}")
    if not all(route_equality.values()):
        raise AssertionError(f"the three native routes produced different UV streams: {route_equality}")
    serialized_routes = []
    for route in routes:
        serial = dict(route)
        serial["source_uv_by_pixel"] = {
            f"{p[0]},{p[1]}": list(uv) for p, uv in route["source_uv_by_pixel"].items()
        }
        serialized_routes.append(serial)
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "proof_scope": "Three finite original-byte generic quad executions; exact source UV read streams and covered pixel sets only.",
        "oracle": {"path": str(pe.path), "sha256": sha(pe.data), "image_base": hex(pe.image_base)},
        "runtime": runtime,
        "tooling": {"python_executable": sys.executable,
                    "python_version": sys.version.split()[0],
                    "pe_parser": "scripts/pe.py via oracle_common",
                    "helper": "renderer_contract/tests/test_original_polygon_edges.py",
                    "helper_sha256": sha(POLYGON_TEST.read_bytes()), "test_sha256": sha(Path(__file__).read_bytes())},
        "geometry": {"screen_vertex_order": VERTICES, "wire_uv_bytes": UV_BYTES,
                     "q16_uv_lanes": UV_Q16, "clip": CLIP,
                     "coverage_expected": [[x, y] for x, y in pixel_order(geom.native_edge_model_expected(VERTICES, CLIP))],
                     "rectangle_shortcut_expected_to_fail": True},
        "original_spans": original_spans,
        "routes": serialized_routes,
        "model_comparison": models,
        "route_uv_equality": route_equality,
        "proof_levels": {
            "STATIC_OBSERVED": "Hash-pinned D420, D60C, CC50, E680, and reciprocal initializer bytes.",
            "BOUNDED_EQUIVALENT": "Original x86 instruction execution under Unicorn for this synthetic quad only; captured read addresses/pixels are compared against independent models.",
            "UNRESOLVED": ["other polygons and clipping/overflow domains", "other E680 mode or blend selectors", "full-frame or pixel-output equivalence"],
        },
        "limitations": [
            "Only one 4x4 axis-aligned screen footprint and one wire-valid UV assignment were exercised.",
            "The E680 case uses mode 0x80 and selector 2, and must take E7C7 generic fallback; it does not characterize the E7BB C8F0 rectangle fast path.",
            "Texture, destination, palette/blend state, reciprocal table, and surface are synthetic emulator memory; original code bytes are read-only and checked unchanged.",
            "The triangle models are independent affine barycentric splits, not assumptions about PSX packet semantics.",
        ],
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
                       "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
                       "receipt": str(ARGS.receipt), "instruction_budget": MAX_INSNS},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "routes": [
        {"route": route["route"], "pixels": route["pixel_count"],
         "reads": len(route["source_reads"]), "path": route["path_markers"],
         "trace_count": route["trace_summary"]["instruction_count"]}
        for route in routes], "match_counts": models["match_counts_by_route"],
        "first_edge_discriminator": models["first_edge_span_vs_alternative"],
        "route_uv_equality": route_equality, "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
