"""Bounded original-byte interpolation audit for skewed convex FT4 quads."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import sys
from pathlib import Path


def find_root(path: Path) -> Path:
    for parent in (path.resolve(), *path.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("cannot find repository root with scripts/pe.py, oracle_common.py, and work/")


ROOT = find_root(Path(__file__))
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import EXPECTED_EXE_SHA256, parse_args, verify_runtime  # noqa: E402

ARGS = parse_args("Audit compact skewed FT4s through D60C, CC50, and E680", "skewed_quads_native.json")
EXPLICIT_FLAGS = {argument.split("=", 1)[0] for argument in sys.argv[1:]}
missing_flags = {"--exe", "--receipt"} - EXPLICIT_FLAGS
if missing_flags:
    raise SystemExit("pass explicit " + " and ".join(sorted(missing_flags)))
if ARGS.original_root.resolve() != ROOT.resolve():
    raise SystemExit(f"--original-root must name {ROOT}")
try:
    ARGS.receipt.resolve().relative_to((ROOT / "work").resolve())
except ValueError as error:
    raise SystemExit("--receipt must stay under the repository's ignored work directory") from error
COMPACT_RECEIPT = ARGS.receipt.with_name("compact_evidence.json")
try:
    COMPACT_RECEIPT.resolve().relative_to((ROOT / "work").resolve())
except ValueError as error:
    raise SystemExit("compact receipt must stay under the repository's ignored work directory") from error

# Reuse the public quad runner and its original polygon-edge helpers without
# running its fixed 4x4 rectangle discriminator.
PUBLIC_QUAD_TEST = ROOT / "renderer_contract" / "tests" / "test_original_quad_interpolation.py"
saved_argv = sys.argv[:]
sys.argv = [str(PUBLIC_QUAD_TEST), "--exe", str(ARGS.exe), "--receipt", str(ARGS.receipt),
            "--original-root", str(ROOT), "--deps", str(ARGS.deps),
            "--analysis-deps", str(ARGS.analysis_deps)]
spec = importlib.util.spec_from_file_location("public_quad_interpolation_helpers", PUBLIC_QUAD_TEST)
if spec is None or spec.loader is None:
    raise RuntimeError("could not load public original quad test helpers")
quad = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = quad
spec.loader.exec_module(quad)
sys.argv = saved_argv

from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import UC_HOOK_MEM_READ, UcError  # noqa: E402
from unicorn.x86_const import UC_X86_REG_EBP, UC_X86_REG_EFLAGS, UC_X86_REG_ESP  # noqa: E402

geom = quad.geom
BASE = 0x400000
MAX_INSNS = quad.MAX_INSNS
E680_SPANS = ((0xE680, 0xEF60), (0xC8F0, 0xCAA8))
PREDICTION_JSON = r'''{
  "prediction": "For five small convex four-vertex polygons with wire-valid signed-16 XY and unsigned-byte UV, the independent original Q30 edge/span model will reproduce every source U/V read on D60C row-zero, CC50 palette, and E680 mode-0x80 selector-2 generic routes. Coverage will follow the original integer scan model; top/bottom clipping will be half-open, and the two skew quads sharing a slanted edge will have no overlap or uncovered pixel inside their union.",
  "falsifier": "Any original source read differs from the Q30 model, any of the three routes disagrees on coverage or UV by output pixel, a generic skew quad takes a rectangle shortcut, top/bottom clipping disagrees with the predicted rows, or the shared-edge pair overlaps/leaves an unexpected gap. A failed precondition, unbounded memory access, original-code mutation, or unexpected callback/state write also invalidates the fixture rather than supporting the model.",
  "competing_models": "Exact barycentric triangle splits and rectangle bilinear interpolation are diagnostic alternatives only; neither is assumed to describe skewed four-edge packets.",
  "shapes": {
    "skewed_parallelogram": {
      "vertices_xy": [[2, 1], [8, 2], [7, 8], [1, 7]],
      "uv_bytes": [[2, 3], [17, 5], [23, 19], [6, 15]],
      "clip": [0, 0, 320, 24]
    },
    "skewed_parallelogram_reverse": {
      "vertices_xy": [[2, 1], [1, 7], [7, 8], [8, 2]],
      "uv_bytes": [[2, 3], [6, 15], [23, 19], [17, 5]],
      "clip": [0, 0, 320, 24]
    },
    "skewed_top_bottom_clip": {
      "vertices_xy": [[1, 1], [9, 2], [7, 9], [2, 8]],
      "uv_bytes": [[4, 28], [42, 2], [60, 72], [7, 100]],
      "clip": [0, 3, 320, 7]
    },
    "shared_edge_left": {
      "vertices_xy": [[0, 1], [4, 2], [6, 6], [1, 7]],
      "uv_bytes": [[4, 20], [32, 8], [54, 74], [11, 98]],
      "clip": [0, 1, 320, 7]
    },
    "shared_edge_right": {
      "vertices_xy": [[4, 2], [8, 1], [10, 7], [6, 6]],
      "uv_bytes": [[32, 8], [85, 4], [109, 112], [54, 74]],
      "clip": [0, 1, 320, 7]
    }
  },
  "route_setup": {
    "d60c": "D420 with original row-zero scan converter",
    "cc50": "D420 with palette scan converter",
    "e680": "direct E680 generic fallback, mode 0x80, selector 2, synthetic blend entries for the seeded destination byte"
  },
  "scope": "Five compact convex quad inputs, three original routes, one synthetic texture/surface setup per run. No full renderer or whole-frame claims."
}'''
PREDICTION = json.loads(PREDICTION_JSON)
PREDICTION_FINGERPRINT = hashlib.sha256(json.dumps(PREDICTION, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()
TEXTURE_VALUES = "1+((u+7*v)%250)"
DESTINATION_INITIAL = 0xA5


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def ordered(pixels) -> list[tuple[int, int]]:
    return sorted({tuple(pixel) for pixel in pixels}, key=lambda p: (p[1], p[0]))


def convex_turns(vertices: list[tuple[int, int]]) -> list[int]:
    turns = []
    for i, p in enumerate(vertices):
        q = vertices[(i + 1) % len(vertices)]
        r = vertices[(i + 2) % len(vertices)]
        turns.append((q[0] - p[0]) * (r[1] - q[1]) - (q[1] - p[1]) * (r[0] - q[0]))
    return turns


def load_shapes() -> dict:
    shapes = PREDICTION["shapes"]
    for name, shape in shapes.items():
        vertices = [tuple(v) for v in shape["vertices_xy"]]
        attributes = [tuple(uv) for uv in shape["uv_bytes"]]
        if len(vertices) != 4 or len(attributes) != 4:
            raise AssertionError(f"{name}: FT4 fixture must have exactly four XY/UV pairs")
        if any(not (-32768 <= coordinate <= 32767) for point in vertices for coordinate in point):
            raise AssertionError(f"{name}: XY exceeds signed 16-bit wire range")
        if any(not (0 <= channel <= 255) for uv in attributes for channel in uv):
            raise AssertionError(f"{name}: UV exceeds unsigned 8-bit wire range")
        turns = convex_turns(vertices)
        if not (all(turn > 0 for turn in turns) or all(turn < 0 for turn in turns)):
            raise AssertionError(f"{name}: shape is not strictly convex: {turns}")
        shape["vertices_xy"] = vertices
        shape["uv_bytes"] = attributes
        shape["clip"] = tuple(shape["clip"])
        shape["turns"] = turns
    return shapes


def set_quad_globals(shape: dict) -> list[tuple[int, int]]:
    vertices = shape["vertices_xy"]
    uv_bytes = shape["uv_bytes"]
    quad.VERTICES = vertices
    quad.UV_BYTES = uv_bytes
    quad.UV_Q16 = [(u << 16, v << 16) for u, v in uv_bytes]
    quad.CLIP = shape["clip"]
    return quad.UV_Q16


def row_pixel_set_from_writes(writes: list[dict]) -> list[tuple[int, int]]:
    pixels = set()
    for event in writes:
        start = int(event["offset"])
        size = int(event.get("size", 1))
        for offset in range(start, start + size):
            if 0 <= offset < geom.SURFACE_SIZE:
                pixels.add((offset % geom.SURFACE_STRIDE, offset // geom.SURFACE_STRIDE))
    return ordered(pixels)


def model_uv(vertices, uv_q16, pixel) -> tuple[int, int]:
    return geom.expected_uv_integer_reciprocal(vertices, uv_q16, pixel)


def q30_coverage_model(vertices, clip) -> tuple[list[tuple[int, int]], list[dict]]:
    """Reproduce endpoint edge steps before SAR16, including reciprocal truncation."""
    _left_clip, top_clip, _right_clip, bottom_clip = clip
    min_y = max(min(y for _x, y in vertices), top_clip)
    max_y = min(max(y for _x, y in vertices), bottom_clip)
    pixels = []
    row_trace = []
    for y in range(min_y, max_y):
        intersections = []
        for i, low in enumerate(vertices):
            j = (i + 1) % len(vertices)
            high = vertices[j]
            if low[1] == high[1]:
                continue
            low_index, high_index = i, j
            if low[1] > high[1]:
                low, high = high, low
                low_index, high_index = high_index, low_index
            if not low[1] <= y < high[1]:
                continue
            dy = high[1] - low[1]
            reciprocal = (1 << 30) // dy
            dx_q16 = (high[0] - low[0]) << 16
            x_step_q16 = (dx_q16 * reciprocal) // (1 << 30)
            x_q16 = (low[0] << 16) + x_step_q16 * (y - low[1])
            intersections.append({"edge_indices": [low_index, high_index],
                                  "x_q16": x_q16,
                                  "x_q16_hex": hex(x_q16),
                                  "floor_x": x_q16 // 65536})
        intersections.sort(key=lambda entry: entry["x_q16"])
        if len(intersections) < 2:
            row_trace.append({"y": y, "intersections": intersections,
                              "coverage": "fewer-than-two-active-edges"})
            continue
        left, right = intersections[0], intersections[-1]
        x_start = max(left["x_q16"] // 65536, 0)
        x_end = min(right["x_q16"] // 65536, 320)
        row_pixels = [(x, y) for x in range(x_start, max(x_start, x_end))]
        pixels.extend(row_pixels)
        row_trace.append({"y": y, "intersections": intersections,
                          "left_x_q16": left["x_q16_hex"],
                          "right_x_q16": right["x_q16_hex"],
                          "integer_span_half_open": [x_start, max(x_start, x_end)]})
    return ordered(pixels), row_trace


def compare_model(pixels: list[tuple[int, int]], observed: dict,
                  vertices, uv_q16) -> dict:
    expected = {pixel: model_uv(vertices, uv_q16, pixel) for pixel in pixels}
    mismatches = [pixel for pixel in ordered(expected)
                  if pixel not in observed or tuple(observed[pixel]) != expected[pixel]]
    extras = [pixel for pixel in ordered(observed) if pixel not in expected]
    first = None
    if mismatches:
        pixel = mismatches[0]
        first = {"pixel": list(pixel), "native_uv": observed.get(pixel),
                 "q30_edge_span_uv": list(expected[pixel])}
    if first is None and extras:
        pixel = extras[0]
        first = {"pixel": list(pixel), "native_uv": observed[pixel],
                 "q30_edge_span_uv": None, "kind": "read_without_surface_pixel"}
    return {"match_count": len(expected) - len(mismatches),
            "expected_count": len(expected), "mismatch_count": len(mismatches),
            "extra_read_count": len(extras), "first_divergence": first,
            "mismatch_pixels": [list(p) for p in mismatches[:16]],
            "model_uv_by_pixel": {f"{x},{y}": list(expected[(x, y)]) for x, y in ordered(expected)}}


def actual_uv_by_pixel(pixels: list[tuple[int, int]], source_reads: list[dict]) -> tuple[dict, str]:
    if len(pixels) != len(source_reads):
        return {}, f"source read count {len(source_reads)} != emitted pixel count {len(pixels)}"
    # Original scan conversion is top-to-bottom, left-to-right. This pairing is
    # the same convention already used by the public quad fixture.
    pairs = {pixel: (int(read["u"]), int(read["v"]))
             for pixel, read in zip(pixels, source_reads)}
    return pairs, "row-major surface-write/source-read sequence pairing"


def summarize_route(case_name: str, shape: dict, route_name: str,
                    vertices, uv_q16, result: dict, source_reads: list[dict],
                    writes: list[dict], trace: list[int], code_unchanged: bool,
                    palette_read_count: int, extra: dict | None = None) -> dict:
    pixel_list = row_pixel_set_from_writes(writes)
    expected_pixels, q30_rows = q30_coverage_model(vertices, shape["clip"])
    exact_pixels = ordered(geom.native_edge_model_expected(vertices, shape["clip"]))
    actual_uv, pairing = actual_uv_by_pixel(pixel_list, source_reads)
    uv_comparison = compare_model(pixel_list, actual_uv, vertices, uv_q16) if actual_uv else {
        "match_count": 0, "expected_count": len(pixel_list),
        "mismatch_count": len(pixel_list), "extra_read_count": 0,
        "first_divergence": {"reason": pairing}, "mismatch_pixels": [],
        "model_uv_by_pixel": {},
    }
    expected_set, actual_set = set(expected_pixels), set(pixel_list)
    exact_set = set(exact_pixels)
    exact_difference = sorted(actual_set ^ exact_set, key=lambda p: (p[1], p[0]))
    exact_first = None
    if exact_difference:
        pixel = exact_difference[0]
        q30_row = next((row for row in q30_rows if row["y"] == pixel[1]), None)
        exact_first = {"pixel": list(pixel),
                       "native_has_pixel": pixel in actual_set,
                       "exact_rational_model_has_pixel": pixel in exact_set,
                       "q30_scanline": q30_row}
    markers = [hex(rva) for rva in trace
               if rva in (0xD420, 0xD549, 0xD555, 0xD583, 0xD589,
                          0xD60C, 0xCC50, 0xDC8A, 0xDC96,
                          0xE680, 0xE6CD, 0xE7BB, 0xE7C7, 0xEF59, 0xEF5B)]
    output = {
        "case": case_name, "route": route_name,
        "vertices_xy": [list(v) for v in vertices],
        "uv_bytes": [list(uv) for uv in shape["uv_bytes"]],
        "uv_q16": [list(uv) for uv in uv_q16],
        "clip_ltrb": list(shape["clip"]), "convex_turn_cross_products": shape["turns"],
        "entry_rva": result.get("entry_rva"),
        "path_markers": markers,
        "generic_d420_fast_rectangle_call_seen": 0xD549 in trace,
        "e680_generic_fallback_seen": 0xE7C7 in trace,
        "e680_rectangle_fastpath_seen": 0xE7BB in trace,
        "native_pixels": [list(p) for p in pixel_list],
        "q30_integer_scan_pixels": [list(p) for p in expected_pixels],
        "coverage_match": actual_set == expected_set,
        "coverage_missing": [list(p) for p in ordered(expected_set - actual_set)],
        "coverage_extra": [list(p) for p in ordered(actual_set - expected_set)],
        "q30_scanline_edge_trace": q30_rows,
        "exact_rational_coverage_match_diagnostic": actual_set == set(exact_pixels),
        "exact_rational_coverage_missing_diagnostic": [list(p) for p in ordered(set(exact_pixels) - actual_set)],
        "exact_rational_coverage_extra_diagnostic": [list(p) for p in ordered(actual_set - set(exact_pixels))],
        "first_exact_rational_coverage_difference_diagnostic": exact_first,
        "source_read_count": len(source_reads),
        "surface_write_event_count": len(writes),
        "palette_read_count": palette_read_count,
        "source_read_uv_by_pixel": {f"{x},{y}": list(actual_uv[(x, y)]) for x, y in ordered(actual_uv)},
        "source_reads": source_reads,
        "source_read_pairing": pairing,
        "q30_model_comparison": uv_comparison,
        "original_code_unchanged": code_unchanged,
    }
    if extra:
        output.update(extra)
    return output


def run_d420_case(pe: PE, data_region: tuple[int, int], case_name: str,
                  shape: dict, route_name: str, uv_q16: list[tuple[int, int]]) -> dict:
    vertices = shape["vertices_xy"]
    quad.VERTICES = vertices
    quad.UV_BYTES = shape["uv_bytes"]
    quad.UV_Q16 = uv_q16
    quad.CLIP = shape["clip"]
    native = quad.configure_quad(route_name, pe, data_region)
    before_data = bytes(native.uc.mem_read(native.data_va, native.data_size))
    try:
        result = native.run()
    except Exception as error:
        return {"case": case_name, "route": route_name, "status": "EXECUTION_ERROR",
                "error": str(error), "native_error": native.error,
                "recent_trace_rvas": [hex(rva) for rva in native.trace[-40:]],
                "original_code_unchanged": all(
                    bytes(native.uc.mem_read(BASE + lo, hi - lo)) == snapshot
                    for (lo, hi), snapshot in native.code_snapshot.items())}
    expected_entry = 0xD60C if route_name == "d60c_rowzero" else 0xCC50
    if expected_entry not in native.trace:
        path_error = f"expected route entry {expected_entry:#x} absent from D420 trace"
    else:
        path_error = None
    pixels = ordered(result["native_pixel_set"])
    reads = result["source_uv_reads"]
    output = summarize_route(case_name, shape, route_name, vertices, uv_q16,
                             result, reads, native.surface_writes, native.trace,
                             result["original_code_unchanged"],
                             len(result["palette_reads"]), {
                                 "expected_entry_seen": path_error is None,
                                 "route_error": path_error,
                                 "trace_instruction_count": len(native.trace),
                                 "native_pixel_list_from_public_helper": [list(p) for p in pixels],
                                 "reciprocal_initializer_writes": len(native.reciprocal_table_writes),
                                 "reciprocal_initializer_stack_writes": len(native.reciprocal_stack_writes),
                                 "data_region_changed_bytes": changed_byte_count(
                                     before_data, bytes(native.uc.mem_read(native.data_va, native.data_size))),
                             })
    return output


def changed_byte_count(before: bytes, after: bytes) -> int:
    return sum(a != b for a, b in zip(before, after))


def run_e680_case(pe: PE, data_region: tuple[int, int], case_name: str,
                  shape: dict, uv_q16: list[tuple[int, int]]) -> dict:
    vertices = shape["vertices_xy"]
    quad.VERTICES = vertices
    quad.UV_BYTES = shape["uv_bytes"]
    quad.UV_Q16 = uv_q16
    quad.CLIP = shape["clip"]
    native = quad.configure_quad("d60c_rowzero", pe, data_region)
    native.code_snapshot.update({
        (start, end): bytes(native.uc.mem_read(BASE + start, end - start))
        for start, end in E680_SPANS
    })
    try:
        quad.initialize_reciprocal_table(native)
        table = bytes(native.uc.mem_read(geom.RECIP_TABLE, geom.RECIP_COUNT * 4))
        table_writes = len(native.reciprocal_table_writes)
        stack_writes = len(native.reciprocal_stack_writes)
        if (table_writes != geom.RECIP_COUNT or
                any(w["size"] != 4 or not geom.RECIP_TABLE <= w["address"] < geom.RECIP_TABLE + geom.RECIP_COUNT * 4
                    for w in native.reciprocal_table_writes) or
                any(w["size"] != 4 or not geom.STACK <= w["address"] < geom.STACK + geom.STACK_SIZE
                    for w in native.reciprocal_stack_writes)):
            raise AssertionError("reciprocal initializer write boundary/count differs from public contract")
        if int.from_bytes(table[:4], "little") != 0:
            raise AssertionError("reciprocal initializer table[0] is not zero")
        for index in (1, 2, 3, geom.RECIP_COUNT - 1):
            actual = int.from_bytes(table[index * 4:index * 4 + 4], "little")
            if actual != (0x40000000 // index):
                raise AssertionError(f"reciprocal initializer table[{index}] mismatch")
    except Exception as error:
        return {"case": case_name, "route": "e680_mode_0x80_selector_2_generic",
                "status": "INITIALIZER_ERROR", "error": str(error),
                "native_error": native.error,
                "recent_trace_rvas": [hex(rva) for rva in native.trace[-40:]]}

    geom.put32(native.uc, quad.MODE_CELL, 0x80)
    geom.put32(native.uc, quad.TPAGE_CELL, 2)
    selector_base = quad.BLEND_BASE + (2 << 16)
    seeded_blend = {}
    for source in range(1, 251):
        address = selector_base + (source << 8) + DESTINATION_INITIAL
        value = (0x80 + source) & 0xFF
        native.uc.mem_write(address, bytes([value]))
        seeded_blend[address] = value
    blend_reads = []

    def capture_blend_read(uc, _access, address, size, _value, _user):
        if selector_base <= address < selector_base + 0x10000:
            blend_reads.append({
                "pc_rva": hex(uc.reg_read(geom.UC_X86_REG_EIP) - BASE),
                "address": hex(address), "size": size,
                "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little"),
            })

    native.uc.hook_add(UC_HOOK_MEM_READ, capture_blend_read)
    native.code_snapshot[(0xE680, 0xEF60)] = bytes(native.uc.mem_read(BASE + 0xE680, 0xEF60 - 0xE680))
    data_after_initializer = bytes(native.uc.mem_read(native.data_va, native.data_size))
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
    if not native.stopped or native.error:
        return {"case": case_name, "route": "e680_mode_0x80_selector_2_generic",
                "status": "EXECUTION_ERROR", "native_error": native.error,
                "stopped_at_sentinel": native.stopped,
                "recent_trace_rvas": [hex(rva) for rva in native.trace[-40:]],
                "blend_reads": blend_reads}
    code_unchanged = all(
        bytes(native.uc.mem_read(BASE + lo, hi - lo)) == snapshot
        for (lo, hi), snapshot in native.code_snapshot.items())
    source_reads = native.source_reads
    write_pixels = row_pixel_set_from_writes(native.surface_writes)
    expected_pixels, _q30_rows = q30_coverage_model(vertices, shape["clip"])
    actual_uv, pairing = actual_uv_by_pixel(write_pixels, source_reads)
    expected_set, actual_set = set(expected_pixels), set(write_pixels)
    output = summarize_route(
        case_name, shape, "e680_mode_0x80_selector_2_generic", vertices, uv_q16,
        {"entry_rva": "0xe680"}, source_reads, native.surface_writes, native.trace,
        code_unchanged, len(native.palette_reads), {
            "stopped_at_sentinel": native.stopped,
            "trace_instruction_count": len(native.trace),
            "reciprocal_initializer_writes": table_writes,
            "reciprocal_initializer_stack_writes": stack_writes,
            "blend_read_count": len(blend_reads),
            "synthetic_blend_entries_seeded": len(seeded_blend),
            "blend_reads": blend_reads,
            "blend_indices_all_seeded": all(
                int(read["address"], 16) in seeded_blend and
                read["value"] == seeded_blend[int(read["address"], 16)]
                for read in blend_reads),
            "data_region_changed_bytes_after_initializer": sum(
                1 for before, after in zip(
                    data_after_initializer,
                    bytes(native.uc.mem_read(native.data_va, native.data_size)))
                if before != after
            ),
            "expected_pixel_count": len(expected_set),
            "actual_pixel_count": len(actual_set),
            "source_read_pairing": pairing,
        })
    if len(blend_reads) != len(source_reads) or not output["blend_indices_all_seeded"]:
        output["blend_route_precondition_error"] = "blend table reads did not match seeded source/destination entries"
    return output


def route_uv(route: dict) -> dict:
    return {tuple(map(int, key.split(","))): tuple(value)
            for key, value in route.get("source_read_uv_by_pixel", {}).items()}


def first_pair_divergence(left: dict, right: dict) -> dict | None:
    a, b = route_uv(left), route_uv(right)
    common = sorted(set(a) & set(b), key=lambda p: (p[1], p[0]))
    for pixel in common:
        if a[pixel] != b[pixel]:
            return {"pixel": list(pixel), "left_uv": list(a[pixel]), "right_uv": list(b[pixel])}
    if set(a) != set(b):
        extra_a = sorted(set(a) - set(b), key=lambda p: (p[1], p[0]))
        extra_b = sorted(set(b) - set(a), key=lambda p: (p[1], p[0]))
        return {"kind": "coverage_key_mismatch",
                "first_left_only": list(extra_a[0]) if extra_a else None,
                "first_right_only": list(extra_b[0]) if extra_b else None}
    return None


def span_record(pe: PE, name: str, start: int, end: int) -> dict:
    code = pe.read_rva(start, end - start)
    return {"name": name, "start_rva": hex(start), "end_rva_exclusive": hex(end),
            "size": len(code), "sha256": sha(code)}


def main() -> None:
    pe = PE(ARGS.exe)
    if sha(pe.data) != EXPECTED_EXE_SHA256 or pe.image_base != BASE:
        raise AssertionError("pinned original EXE identity/base mismatch")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("unexpected original relocation directory")
    data_sec = next(section for section in pe.sections if section["name"] == ".data")
    data_region = (BASE + data_sec["virtual_address"],
                   max(data_sec["virtual_size"], data_sec["raw_size"]))
    runtime = verify_runtime(unicorn, ARGS.deps)
    shapes = load_shapes()

    # Add only the original E680 generic body and its known rectangle callee;
    # any out-of-scope call remains a bounded unexpected-code failure.
    geom.CODE_WINDOWS = tuple(geom.CODE_WINDOWS) + E680_SPANS
    route_results = {}
    for name, shape in shapes.items():
        uv_q16 = [(u << 16, v << 16) for u, v in shape["uv_bytes"]]
        route_results[name] = {}
        for route in ("d60c_rowzero", "cc50_palette"):
            route_results[name][route] = run_d420_case(pe, data_region, name, shape, route, uv_q16)
        route_results[name]["e680_mode_0x80_selector_2_generic"] = run_e680_case(
            pe, data_region, name, shape, uv_q16)

    route_names = ("d60c_rowzero", "cc50_palette", "e680_mode_0x80_selector_2_generic")
    all_route_uv_equal = {}
    model_match_by_route = {}
    for name, routes in route_results.items():
        all_route_uv_equal[name] = {
            f"{left}_equals_{right}": first_pair_divergence(routes[left], routes[right])
            for left, right in ((route_names[0], route_names[1]),
                                (route_names[0], route_names[2]),
                                (route_names[1], route_names[2]))
        }
        model_match_by_route[name] = {
            route: {"match_count": result.get("q30_model_comparison", {}).get("match_count", 0),
                    "expected_count": result.get("q30_model_comparison", {}).get("expected_count", 0),
                    "mismatch_count": result.get("q30_model_comparison", {}).get("mismatch_count", 0),
                    "first_divergence": result.get("q30_model_comparison", {}).get("first_divergence")}
            for route, result in routes.items()
        }

    shared = {}
    if "shared_edge_left" in route_results and "shared_edge_right" in route_results:
        left_shape, right_shape = shapes["shared_edge_left"], shapes["shared_edge_right"]
        left_model = set(q30_coverage_model(left_shape["vertices_xy"], left_shape["clip"])[0])
        right_model = set(q30_coverage_model(right_shape["vertices_xy"], right_shape["clip"])[0])
        expected_union = left_model | right_model
        boundary_rows = []
        for y in range(2, 6):
            dy = 6 - 2
            reciprocal = (1 << 30) // dy
            dx_q16 = (6 - 4) << 16
            edge_step = (dx_q16 * reciprocal) // (1 << 30)
            edge_x_q16 = (4 << 16) + edge_step * (y - 2)
            boundary_x = edge_x_q16 // 65536
            boundary_rows.append({"y": y, "shared_edge_x_q16": hex(edge_x_q16),
                                  "floor_x": boundary_x,
                                  "left_expected_inside_pixel": boundary_x - 1,
                                  "right_expected_inside_pixel": boundary_x})
        for route in route_names:
            left_pixels = {tuple(p) for p in route_results["shared_edge_left"][route].get("native_pixels", [])}
            right_pixels = {tuple(p) for p in route_results["shared_edge_right"][route].get("native_pixels", [])}
            union = left_pixels | right_pixels
            overlap = left_pixels & right_pixels
            boundary_ownership = []
            for boundary in boundary_rows:
                y, edge_x = boundary["y"], boundary["floor_x"]
                boundary_ownership.append({
                    **boundary,
                    "left_contains_inside_pixel": (edge_x - 1, y) in left_pixels,
                    "right_contains_inside_pixel": (edge_x, y) in right_pixels,
                    "cross_edge_gap": (edge_x - 1, y) not in left_pixels or (edge_x, y) not in right_pixels,
                })
            shared[route] = {
                "left_coverage_count": len(left_pixels), "right_coverage_count": len(right_pixels),
                "overlap_count": len(overlap), "overlap_pixels": [list(p) for p in ordered(overlap)],
                "union_count": len(union), "union_matches_two_quad_q30_model": union == expected_union,
                "union_missing_from_q30_model": [list(p) for p in ordered(expected_union - union)],
                "union_extra_over_q30_model": [list(p) for p in ordered(union - expected_union)],
                "shared_edge_boundary_ownership": boundary_ownership,
            }

    clipped_rows = {}
    clipped_shape = shapes["skewed_top_bottom_clip"]
    for route in route_names:
        pixels = [tuple(p) for p in route_results["skewed_top_bottom_clip"][route].get("native_pixels", [])]
        rows = sorted({y for _, y in pixels})
        clipped_rows[route] = {"rows": rows, "expected_rows": [3, 4, 5, 6],
                               "rows_match": rows == [3, 4, 5, 6],
                               "all_inside_half_open_clip": all(3 <= y < 7 for _, y in pixels)}

    exact_rational_diagnostics = {
        case_name: {route: result.get("first_exact_rational_coverage_difference_diagnostic")
                    for route, result in routes.items()
                    if result.get("first_exact_rational_coverage_difference_diagnostic") is not None}
        for case_name, routes in route_results.items()
    }

    spans = [
        span_record(pe, "D420 polygon and route dispatcher", 0xD420, 0xDD70),
        span_record(pe, "D60C row-zero edge/span converter", 0xD60C, 0xDD70),
        span_record(pe, "CC50 palette edge/span converter", 0xCC50, 0xD415),
        span_record(pe, "E680 mode dispatcher and generic fallback", 0xE680, 0xEF60),
        span_record(pe, "C8F0 rectangle callee (bounded but not expected on skew quads)", 0xC8F0, 0xCAA8),
        span_record(pe, "reciprocal table initializer", 0x72A0, 0x72CC),
    ]
    errors = [
        {"case": case_name, "route": route_name, "status": result.get("status")}
        for case_name, routes in route_results.items()
        for route_name, result in routes.items()
        if result.get("status") in ("EXECUTION_ERROR", "INITIALIZER_ERROR")
    ]
    q30_all_match = all(
        result.get("q30_model_comparison", {}).get("mismatch_count") == 0 and
        result.get("q30_model_comparison", {}).get("extra_read_count") == 0
        for routes in route_results.values() for result in routes.values()
        if result.get("status") not in ("EXECUTION_ERROR", "INITIALIZER_ERROR")
    )
    coverage_all_match = all(
        result.get("coverage_match") is True
        for routes in route_results.values() for result in routes.values()
        if result.get("status") not in ("EXECUTION_ERROR", "INITIALIZER_ERROR")
    )
    route_equal_all = all(
        divergence is None
        for comparisons in all_route_uv_equal.values()
        for divergence in comparisons.values()
    )
    code_all_unchanged = all(
        result.get("original_code_unchanged") is True
        for routes in route_results.values() for result in routes.values()
        if result.get("status") not in ("EXECUTION_ERROR", "INITIALIZER_ERROR")
    )
    route_preconditions_ok = all(
        result.get("expected_entry_seen") is True and
        result.get("generic_d420_fast_rectangle_call_seen") is False and
        result.get("source_read_count") == len(result.get("native_pixels", []))
        for routes in route_results.values()
        for route_name, result in routes.items()
        if route_name in ("d60c_rowzero", "cc50_palette")
    ) and all(
        result.get("stopped_at_sentinel") is True and
        result.get("e680_generic_fallback_seen") is True and
        result.get("e680_rectangle_fastpath_seen") is False and
        result.get("blend_indices_all_seeded") is True and
        result.get("blend_read_count") == result.get("source_read_count")
        for routes in route_results.values()
        for route_name, result in routes.items()
        if route_name == "e680_mode_0x80_selector_2_generic"
    )
    shared_checks_ok = all(
        data["overlap_count"] == 0 and data["union_matches_two_quad_q30_model"] and
        all(not row["cross_edge_gap"] for row in data["shared_edge_boundary_ownership"])
        for data in shared.values()
    )
    clip_checks_ok = all(data["rows_match"] and data["all_inside_half_open_clip"]
                         for data in clipped_rows.values())
    strict_checks_ok = (not errors and code_all_unchanged and route_preconditions_ok and
                        shared_checks_ok and clip_checks_ok)
    status = "BOUNDED_EQUIVALENT" if q30_all_match and coverage_all_match and route_equal_all and strict_checks_ok else (
        "BOUNDED_DIVERGENCE" if not errors else "BOUNDED_INCOMPLETE")
    receipt = {
        "schema_version": 1,
        "status": status,
        "scope": "Five compact convex, wire-valid FT4 inputs across original D60C, CC50, and E680 mode-0x80 selector-2 routes; compare coverage and source UV reads to the public four-edge Q30 model.",
        "oracle": {"path": str(pe.path), "sha256": sha(pe.data), "image_base": hex(pe.image_base)},
        "runtime": runtime,
        "tooling": {
            "python_executable": sys.executable,
            "python_version": sys.version.split()[0],
            "runner_path": str(Path(__file__).resolve()),
            "runner_sha256": sha(Path(__file__).read_bytes()),
            "public_quad_test_path": str(PUBLIC_QUAD_TEST),
            "public_quad_test_sha256": sha(PUBLIC_QUAD_TEST.read_bytes()),
            "public_polygon_helper_path": str(quad.POLYGON_TEST),
            "public_polygon_helper_sha256": sha(quad.POLYGON_TEST.read_bytes()),
            "prediction_source": "embedded in this runner; canonical JSON fingerprint",
            "prediction_sha256": PREDICTION_FINGERPRINT,
        },
        "preconditions_and_side_effects": {
            "wire_domain": "signed 16-bit XY and unsigned 8-bit UV, expanded to Q16 exactly as packet fields",
            "reciprocal_table": "original RVA 0x72A0..0x72CB initializer; zero-filled .data table at A8E918, 2048 entries floor(2^30/i), entry zero=0; public helper bounds writes to table plus synthetic stack",
            "synthetic_surface": {"va": hex(geom.SURFACE), "stride": geom.SURFACE_STRIDE,
                                  "size": geom.SURFACE_SIZE, "initial_byte": hex(DESTINATION_INITIAL)},
            "synthetic_texture": {"va": hex(geom.TEXTURE), "bytes": 65536, "value_formula": TEXTURE_VALUES},
            "cc50_palette": "public NativeCase seeds row-3 CLUT with byte indices; native palette reads are recorded per case",
            "e680_mode": "0x80, selector 2; destination starts at 0xa5; 250 reachable synthetic blend entries are seeded for sources 1..250",
            "callback_boundary": "No host callback was supplied. Original code execution is limited to hash-pinned in-image spans and must return to the synthetic sentinel; unexpected code/memory access is reported by the public helper.",
            "original_bytes": "hash-pinned original PE and per-case code snapshots; no original code is written",
        },
        "model_contract": {
            "coverage_model": "public integer scan model: y in [top,bottom), floor signed Q16 endpoints, x span [floor(left),floor(right)), screen clip [0,320)",
            "uv_model": "four-edge interpolation using R[i]=floor(2^30/i), signed Q16 edge increments, integer reciprocal span steps, and high-byte texture indices",
            "acceptance_prediction": PREDICTION["prediction"],
            "falsifier": PREDICTION["falsifier"],
            "no_alternative_assumption": "Bilinear and triangle-split models are not acceptance conditions for skewed quads.",
        },
        "original_spans": spans,
        "cases": route_results,
        "q30_model_match_summary": model_match_by_route,
        "cross_route_uv_comparison": all_route_uv_equal,
        "shared_boundary": shared,
        "top_bottom_clip": clipped_rows,
        "exact_rational_coverage_diagnostics": exact_rational_diagnostics,
        "summary": {"q30_model_all_match": q30_all_match,
                    "coverage_all_match": coverage_all_match,
                    "route_uv_streams_equal": route_equal_all,
                    "original_code_all_unchanged": code_all_unchanged,
                    "route_preconditions_ok": route_preconditions_ok,
                    "shared_edge_checks_ok": shared_checks_ok,
                    "clip_checks_ok": clip_checks_ok,
                    "strict_checks_ok": strict_checks_ok,
                    "execution_errors": errors,
                    "fixture_count": len(shapes),
                    "route_count": len(route_names),
                    "total_original_route_runs": len(shapes) * len(route_names)},
        "proof_levels": {
            "STATIC_OBSERVED": "Pinned original D420, D60C, CC50, E680, C8F0, and reciprocal-initializer spans.",
            "BOUNDED_ORIGINAL_BYTES": "Original x86 executes under the pinned 32-bit Unicorn runtime with synthetic callbacks/tables/surface; reads, writes, and route markers are captured.",
            "MODEL_CHECK": "The small selected source-read streams and coverage sets are compared to public fixed-Q30 edge/span models; mismatches report earliest pixel.",
            "UNRESOLVED": ["other convex/non-convex polygons", "other clip or coordinate ranges", "other E680 modes/selectors", "full-frame or GPU output equivalence"],
        },
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
                       "receipt": str(ARGS.receipt), "compact_receipt": str(COMPACT_RECEIPT),
                       "instruction_budget_per_run": MAX_INSNS},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    compact = {
        "schema_version": 1, "status": status,
        "oracle_sha256": sha(pe.data),
        "prediction_sha256": receipt["tooling"]["prediction_sha256"],
        "runner_sha256": receipt["tooling"]["runner_sha256"],
        "full_trace": str(ARGS.receipt), "full_trace_sha256": sha(ARGS.receipt.read_bytes()),
        "model_results": receipt["summary"],
        "earliest_q30_divergences": model_match_by_route,
        "shared_boundary": shared,
        "top_bottom_clip": clipped_rows,
        "exact_rational_coverage_diagnostics": {
            case_name: {
                route_name: {
                    "pixel": diag["pixel"],
                    "native_has_pixel": diag["native_has_pixel"],
                    "exact_rational_model_has_pixel": diag["exact_rational_model_has_pixel"],
                    "q30_scanline": {
                        key: diag["q30_scanline"][key]
                        for key in ("y", "left_x_q16", "right_x_q16", "integer_span_half_open")
                        if key in diag.get("q30_scanline", {})
                    },
                }
                for route_name, diag in route_map.items() if diag is not None
            }
            for case_name, route_map in exact_rational_diagnostics.items()
            if any(diag is not None for diag in route_map.values())
        },
        "fixtures": {
            name: {
                "vertices_xy_i16": [list(point) for point in shape["vertices_xy"]],
                "uv_bytes": [list(uv) for uv in shape["uv_bytes"]],
                "clip_ltrb": list(shape["clip"]),
                "routes": list(route_names),
            }
            for name, shape in shapes.items()
        },
        "route_uv_equality": all_route_uv_equal,
        "original_spans": spans,
        "proof_scopes": receipt["proof_levels"],
        "prediction": PREDICTION["prediction"],
        "falsifier": PREDICTION["falsifier"],
        "proof_boundary": "Bounded selected original-byte FT4 fixtures only; no global interpolation claim.",
    }
    COMPACT_RECEIPT.write_text(json.dumps(compact, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": status, "summary": receipt["summary"],
                      "shared_boundary": shared, "top_bottom_clip": clipped_rows,
                      "receipt": str(ARGS.receipt), "compact_receipt": str(COMPACT_RECEIPT)}, indent=2))


if __name__ == "__main__":
    main()
