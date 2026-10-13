"""Compare bounded original CC50/E680 packed-UV feedback routes."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path


def find_root(start: Path) -> Path:
    for parent in (start.resolve(), *start.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("repository root must contain scripts/pe.py and renderer_contract/")


ROOT = find_root(Path(__file__))
WORK_DIR = ROOT / "work"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--exe", required=True, type=Path,
                    help="read-only pinned original HERCULES.EXE")
parser.add_argument("--original-root", required=True, type=Path,
                    help="repository root containing scripts/pe.py and work/")
parser.add_argument("--receipt", required=True, type=Path,
                    help="JSON output somewhere below the repository work/ directory")
parser.add_argument("--deps", type=Path,
                    default=Path(r"C:\tools\hercules\renderer-oracle-python"))
parser.add_argument("--analysis-deps", type=Path,
                    default=Path(r"C:\tools\hercules\python"))
ARGS = parser.parse_args()
for name in ("exe", "original_root", "receipt", "deps", "analysis_deps"):
    setattr(ARGS, name, getattr(ARGS, name).resolve())
if ARGS.original_root != ROOT.resolve():
    parser.error(f"--original-root must name {ROOT}")
try:
    ARGS.receipt.relative_to(WORK_DIR.resolve())
except ValueError as error:
    raise SystemExit("--receipt must stay below the repository work/ directory") from error
if not ARGS.exe.is_file() or not ARGS.deps.is_dir() or not ARGS.analysis_deps.is_dir():
    parser.error("original executable or pinned dependency directory is missing")

EXPECTED_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
BASE = 0x400000
PREDICTION = {
    "family": "CC50 and E680 packed UV feedback closure",
    "prediction": "On the established 319-pixel row, CC50's 0xD322 loop will advance its UV state through EAX/EBX/ECX and decrement AX for its count, while raw source and palette result stay in DL; neither will change later UV samples. E680 mode 0x80's 0xEE6D loop and E680 mode 0x7F through DD70's 0xE56C loop will advance UV through EAX/EBX/ECX and the corrected ESI texture pointer, decrement AX for the count, and keep raw source, shade and blend bytes in EDX. None will feed the UV state; each loop will reconstruct its per-pixel EDX source/destination lanes. Zero source will still consume one iteration and skip the route's palette or shade/blend/destination/output operations.",
    "falsifier": "A controlled raw-source, CLUT, shade-LUT, or blend-LUT change alters any later UV-carrying register or texture address in one of these bounded routes; the observed earliest register divergence is outside the byte/color lane; the source/LUT/output event order differs from the stated route; a zero source performs a forbidden lookup/read/write; or a route fails to execute exactly 319 row iterations and stop within the pinned original instruction/memory boundaries.",
    "fixture": {
        "vertices_xy": [[0, 0], [320, 0], [319, 361], [0, 361]],
        "clip_ltrb": [0, 1, 320, 2],
        "uv_q16": [[0, 65536], [0, 0], [0, 14352384], [0, 14417920]],
        "visible_row_pixels": 319,
        "paired_texture_runs": [1, 255],
        "source_lut_controls": "Keep outputs equal across the raw-source pair; vary one selected LUT result in a separate pair. Also run an all-zero texture through each route.",
    },
    "routes": ["D420 -> CC50 row-3 palette", "E680 mode 0x80 selector 2 generic", "E680 mode 0x7F -> DD70 shade row 0x7F generic"],
    "scope": "Original CPU instruction slices and one coherent clipped long-row fixture; no GPU, full raster, gameplay, or unsupported E680 modes/selectors.",
}
HISTORICAL_D60C_RECEIPT = {
    "path": "work/renderer_phase7/packed_uv/packed_uv_receipt.json",
    "sha256": "4abec32814646d4ce95ef92c4196ae78396ab6b6cf7da07d866677d7c7bdc2cf",
    "runner_path": "work/renderer_phase7/packed_uv/tests/test_original_packed_uv_feedback.py",
    "runner_sha256": "1534edc9d55f980cd8dc44ec4b5f20a898879bc6999b4995dc3dfe3da2232b9b",
    "runtime_required": False,
}
SPEC = {
    "name": "phase7_long_row_feedback",
    "vertices": [(0, 0), (320, 0), (319, 361), (0, 361)],
    "clip": (0, 1, 320, 2),
    "uv_mode": "fractional_gradient",
    "attributes": [(0, 1 << 16), (0, 0), (0, 219 << 16), (0, 220 << 16)],
    "attribute_domain": "wire_unsigned_u8_q16",
}
EXPECTED_PIXELS = [(x, 1) for x in range(319)]
SOURCE_ADDRESS_DISCRIMINATOR = 0x100  # texture sample U=0,V=1 in the existing witness
DESTINATION_INITIAL = 0xA5
SHADE_BASE = 0x91C488
BLEND_BASE = 0x92E3D0
SELECTOR = 2
SHADE_MODE = 0x7F
MAX_INSTRUCTIONS = 250_000

# Loop PCs are the original instruction boundaries. Code hooks run before the
# addressed instruction; the labels identify the most useful state snapshots.
ROUTES = {
    "cc50_palette": {
        "entry": "D420 -> CC50 row 3",
        "loop_start": 0xD322,
        "hooks": {
            0xD322: "loop_start", 0xD32E: "before_source",
            0xD331: "after_source", 0xD337: "after_uv_step",
            0xD33D: "before_palette", 0xD341: "before_store",
            0xD343: "after_output_or_skip", 0xD344: "before_count_decrement",
        },
        "stages": ("loop_start", "before_source", "after_source", "after_uv_step",
                   "before_palette", "before_store", "after_output_or_skip",
                   "before_count_decrement"),
        "uv_registers": ("eax", "ebx", "ecx", "esi", "edx"),
        "source_pc": 0xD32E,
        "output_pc": 0xD341,
    },
    "e680_mode80": {
        "entry": "E680 mode 0x80, selector 2 generic",
        "loop_start": 0xEE6D,
        "hooks": {
            0xEE6D: "loop_start", 0xEE79: "before_source",
            0xEE7C: "after_source", 0xEE82: "after_uv_step",
            0xEE86: "before_destination", 0xEE88: "before_blend",
            0xEE8C: "before_store", 0xEE8E: "after_output_or_skip",
            0xEE8F: "before_count_decrement",
        },
        "stages": ("loop_start", "before_source", "after_source", "after_uv_step",
                   "before_destination", "before_blend", "before_store",
                   "after_output_or_skip", "before_count_decrement"),
        "uv_registers": ("eax", "ebx", "ecx", "esi", "edx"),
        "source_pc": 0xEE79,
        "output_pc": 0xEE8C,
    },
    "e680_shaded_7f": {
        "entry": "E680 mode 0x7F -> DD70 generic shade row 0x7F",
        "loop_start": 0xE56C,
        "hooks": {
            0xE56C: "loop_start", 0xE578: "before_source",
            0xE57B: "after_source", 0xE581: "after_uv_step",
            0xE585: "before_shade", 0xE595: "after_shade",
            0xE59B: "before_destination", 0xE59D: "before_blend",
            0xE5A1: "before_store", 0xE5A3: "after_output_or_skip",
            0xE5A4: "before_count_decrement",
        },
        "stages": ("loop_start", "before_source", "after_source", "after_uv_step",
                   "before_shade", "after_shade", "before_destination", "before_blend",
                   "before_store", "after_output_or_skip", "before_count_decrement"),
        "uv_registers": ("eax", "ebx", "ecx", "esi", "edx"),
        "source_pc": 0xE578,
        "output_pc": 0xE5A1,
    },
}

EXPECTED_SPANS = {
    "d420_dispatch_and_rowzero_function": (0xD420, 0xDD70,
        "4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e"),
    "cc50_active_palette_function": (0xCC50, 0xD415,
        "7504054c2fe867eef6602dbd439f1972b55a73540b98099498772db2f34b7fd0"),
    "cc50_pixel_loop": (0xD2C6, 0xD349,
        "86688a0be90e8b5b3c931c4f789d6dd6fc53575085f9ee700362dbf3617b6d1f"),
    "dd70_generic_body": (0xDE9F, 0xE675,
        "645a1a416be1477bd9000e017bc4f733161c70769cc34c6f13f214ec2271f236"),
    "dd70_shaded_pixel_loop": (0xE56C, 0xE5A8,
        "1d396bce03e35e84d2f2ae6d5f5dc7a392836b581ece877599de5b9ef0677bdc"),
    "e680_mode_shape_dispatch": (0xE680, 0xE7C7,
        "1ac531aa802148e641444cb62451c9903fe6e59b046329d0ea0a80f463ccc75a"),
    "e680_generic_body": (0xE7C7, 0xEF60,
        "2aad976b7e2ee3947a06fb8f89c31ef39dcdbfbf487bb1f9b07faed05d7adeae"),
    "e680_mode80_pixel_loop": (0xEE6D, 0xEE93,
        "c4c067791e0ebc638acd6707ef4a2f27c8c76095cce726bf1aa4b9666e466247"),
    "reciprocal_table_initializer": (0x72A0, 0x72CC,
        "3d59be43f3756224228dfdcc372a3bc59b7a7dbba4bbab8c79dd821cc958014b"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def optional_canonical_native_feedback_reference() -> dict:
    # This artifact is optional historical context, never an execution input.
    candidate = ROOT / "renderer_contract" / "evidence" / "native_packed_uv_feedback.json"
    if candidate.is_file():
        return {"available": True,
                "path": candidate.relative_to(ROOT).as_posix(),
                "sha256": sha(candidate.read_bytes()),
                "runtime_required": False}
    return {"available": False,
            "expected_path": "renderer_contract/evidence/native_packed_uv_feedback.json",
            "historical_private_provenance": HISTORICAL_D60C_RECEIPT,
            "runtime_required": False}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def load_public_helpers():
    public = ROOT / "renderer_contract" / "tests" / "test_original_quad_interpolation.py"
    saved_argv = sys.argv[:]
    sys.argv = [str(public), "--exe", str(ARGS.exe), "--receipt", str(ARGS.receipt),
                "--original-root", str(ROOT), "--deps", str(ARGS.deps),
                "--analysis-deps", str(ARGS.analysis_deps)]
    try:
        spec = importlib.util.spec_from_file_location("public_quad_interpolation_helpers", public)
        if spec is None or spec.loader is None:
            raise RuntimeError("could not load public quad/polygon oracle helpers")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        return module, public
    finally:
        sys.argv = saved_argv


quad, QUAD_HELPER_PATH = load_public_helpers()
geom = quad.geom
sys.path.insert(0, str(ROOT / "scripts"))
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UcError  # noqa: E402
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP,
    UC_X86_REG_ESI, UC_X86_REG_ESP,
)

REGISTERS = {
    "eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX,
    "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX,
    "esi": UC_X86_REG_ESI, "edi": UC_X86_REG_EDI,
    "ebp": UC_X86_REG_EBP, "eflags": UC_X86_REG_EFLAGS,
}
REG_ORDER = tuple(REGISTERS)


def verify_spans(pe: PE) -> list[dict]:
    records = []
    for name, (start, end, expected) in EXPECTED_SPANS.items():
        actual = sha(pe.read_rva(start, end - start))
        require(actual == expected, f"pinned {name} span changed: {actual}")
        records.append({"name": name, "rva_start": hex(start),
                        "rva_end_exclusive": hex(end), "size": end - start,
                        "sha256": actual})
    return records


def state(uc) -> dict:
    return {name: uc.reg_read(register) & 0xFFFFFFFF
            for name, register in REGISTERS.items()}


def fixture_geometry() -> None:
    # The helper's original 32-byte pitch aliases x=320 into the next row.
    # This synthetic widened pitch keeps the 319-pixel row wholly observable.
    geom.SURFACE_STRIDE = 512
    geom.SURFACE_HEIGHT = 4
    geom.SURFACE_SIZE = geom.SURFACE_STRIDE * geom.SURFACE_HEIGHT
    quad.VERTICES = list(SPEC["vertices"])
    quad.UV_Q16 = [(u, v) for u, v in SPEC["attributes"]]
    quad.CLIP = SPEC["clip"]
    geom.CODE_WINDOWS = tuple(geom.CODE_WINDOWS) + ((0xDD70, 0xE675), (0xE680, 0xEF60))


def instrumentation(native, route_name: str) -> dict:
    route = ROUTES[route_name]
    iterations: list[dict] = []
    events: list[dict] = []
    current: dict | None = None
    texture_lo, texture_hi = geom.TEXTURE, geom.TEXTURE + 0x10000
    surface_lo, surface_hi = geom.SURFACE, geom.SURFACE + geom.SURFACE_SIZE
    clut_lo, clut_hi = geom.CLUT_BASE + 3 * 256, geom.CLUT_BASE + 4 * 256
    shade_lo = SHADE_BASE + (SHADE_MODE << 8)
    shade_hi = shade_lo + 256
    blend_lo = BLEND_BASE + (SELECTOR << 16)
    blend_hi = blend_lo + 0x10000

    def on_code(uc, address, _size, _user):
        nonlocal current
        rva = address - BASE
        if rva == route["loop_start"]:
            current = {"index": len(iterations), "states": {}}
            iterations.append(current)
        label = route["hooks"].get(rva)
        if current is not None and label is not None:
            current["states"][label] = state(uc)

    def on_read(uc, _access, address, size, _value, _user):
        if not (texture_lo <= address < texture_hi or surface_lo <= address < surface_hi or
                clut_lo <= address < clut_hi or shade_lo <= address < shade_hi or
                blend_lo <= address < blend_hi):
            return
        if texture_lo <= address < texture_hi:
            kind = "source"
        elif route_name == "cc50_palette" and clut_lo <= address < clut_hi:
            kind = "palette"
        elif route_name == "e680_shaded_7f" and shade_lo <= address < shade_hi:
            kind = "shade"
        elif blend_lo <= address < blend_hi:
            kind = "blend"
        elif surface_lo <= address < surface_hi:
            kind = "destination"
        else:
            return
        raw = bytes(uc.mem_read(address, size))
        events.append({"sequence": len(events), "iteration": None if current is None else current["index"],
                       "kind": kind, "pc_rva": hex(uc.reg_read(UC_X86_REG_EIP) - BASE),
                       "address": hex(address), "size": size,
                       "value": int.from_bytes(raw, "little")})

    def on_write(uc, _access, address, size, value, _user):
        if not (surface_lo <= address < surface_hi):
            return
        events.append({"sequence": len(events), "iteration": None if current is None else current["index"],
                       "kind": "output", "pc_rva": hex(uc.reg_read(UC_X86_REG_EIP) - BASE),
                       "address": hex(address), "size": size,
                       "value": value & ((1 << (size * 8)) - 1)})

    native.uc.hook_add(UC_HOOK_CODE, on_code)
    native.uc.hook_add(UC_HOOK_MEM_READ, on_read)
    native.uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    return {"iterations": iterations, "events": events}


def texture_bytes(value: int) -> bytes:
    require(0 <= value <= 255, "source byte outside byte range")
    return bytes([value]) * 0x10000


def configure_cc50(native, source: int, palette_value: int, all_zero: bool) -> None:
    native.uc.mem_write(geom.TEXTURE, texture_bytes(0 if all_zero else source))
    palette = bytearray([0x55] * 256)
    if all_zero:
        palette[0] = 0xE1
    else:
        palette[source] = palette_value
    native.uc.mem_write(geom.CLUT_BASE + 3 * 256, bytes(palette))


def configure_e680_tables(native, route_name: str, source: int,
                           source_values: tuple[int, ...],
                           *, output: int = 0x66, shade_value: int = 0x22,
                           shade_values: tuple[int, ...] = (), all_zero: bool = False) -> None:
    mode = 0x80 if route_name == "e680_mode80" else SHADE_MODE
    geom.put32(native.uc, 0xA90A00, mode)
    geom.put32(native.uc, 0xA8E6F8, SELECTOR)
    native.uc.mem_write(geom.TEXTURE, texture_bytes(0 if all_zero else source))
    if route_name == "e680_mode80":
        for source in source_values:
            address = BLEND_BASE + (SELECTOR << 16) + (source << 8) + DESTINATION_INITIAL
            native.uc.mem_write(address, bytes([output]))
    else:
        shade_row = SHADE_BASE + (SHADE_MODE << 8)
        for source in source_values:
            native.uc.mem_write(shade_row + source, bytes([shade_value]))
        for mapped in set(shade_values or (shade_value,)):
            address = BLEND_BASE + (SELECTOR << 16) + (mapped << 8) + DESTINATION_INITIAL
            native.uc.mem_write(address, bytes([output]))


def set_extra_code_snapshots(native) -> None:
    for start, end in ((0xDD70, 0xE675), (0xE680, 0xEF60)):
        native.code_snapshot[(start, end)] = bytes(native.uc.mem_read(BASE + start, end - start))


def validate_event_order(run: dict, route_name: str, all_zero: bool) -> dict:
    events = run["events"]
    expected = {
        "cc50_palette": ["source", "palette", "output"],
        "e680_mode80": ["source", "destination", "blend", "output"],
        "e680_shaded_7f": ["source", "shade", "destination", "blend", "output"],
    }[route_name]
    by_iteration: dict[int, list[dict]] = {}
    for event in events:
        index = event["iteration"]
        require(index is not None, f"{route_name}: route data access occurred outside its loop")
        by_iteration.setdefault(index, []).append(event)
    if all_zero:
        wanted = ["source"]
    else:
        wanted = expected
    kinds_by_iteration = {i: [event["kind"] for event in by_iteration.get(i, [])]
                          for i in range(319)}
    expected_loop_events = {i: wanted for i in range(319)}
    require(kinds_by_iteration == expected_loop_events,
            f"{route_name}: source/LUT/destination/output order differs from {wanted}")
    for index in range(319):
        sample = by_iteration[index]
        source = sample[0]
        pre_sample = run["iterations"][index]["states"]["before_source"]
        require(int(source["address"], 16) == pre_sample["esi"] + pre_sample["edx"],
                f"{route_name}: captured packed UV address does not explain source load {index}")
        if all_zero:
            require(source["value"] == 0,
                    f"{route_name}: zero route sampled nonzero byte at iteration {index}")
            continue
        output = sample[-1]
        expected_surface = geom.SURFACE + geom.SURFACE_STRIDE + index
        require(int(output["address"], 16) == expected_surface,
                f"{route_name}: output write does not follow the row-major destination at {index}")
        if route_name == "cc50_palette":
            palette = sample[1]
            require(int(palette["address"], 16) == geom.CLUT_BASE + 3 * 256 + source["value"] and
                    output["value"] == palette["value"],
                    f"CC50: palette address/result does not follow the sampled source at {index}")
        else:
            destination = sample[1] if route_name == "e680_mode80" else sample[2]
            require(int(destination["address"], 16) == expected_surface and
                    destination["value"] == DESTINATION_INITIAL,
                    f"{route_name}: destination read differs at {index}")
            if route_name == "e680_mode80":
                blend = sample[2]
                blend_index = (source["value"] << 8) | destination["value"]
            else:
                shade = sample[1]
                require(int(shade["address"], 16) ==
                        SHADE_BASE + (SHADE_MODE << 8) + source["value"],
                        f"DD70: shade lookup address does not use raw source at {index}")
                blend = sample[3]
                blend_index = (shade["value"] << 8) | destination["value"]
            require(int(blend["address"], 16) ==
                    BLEND_BASE + (SELECTOR << 16) + blend_index and
                    output["value"] == blend["value"],
                    f"{route_name}: blend address/result does not match source/color and destination at {index}")
    return {"expected_per_iteration": wanted, "iterations_checked": len(by_iteration),
            "event_count": len(events), "all_319_source_addresses_match_pre_sample_registers": True,
            "lookup_indices_and_output_values_match": not all_zero,
            "zero_route_only_samples": all_zero}


def summarize_case(route_name: str, label: str, native, result: dict,
                   capture: dict, all_zero: bool) -> dict:
    route = ROUTES[route_name]
    iterations = capture["iterations"]
    source_events = [event for event in capture["events"] if event["kind"] == "source"]
    output_events = [event for event in capture["events"] if event["kind"] == "output"]
    require(len(iterations) == len(source_events) == 319,
            f"{route_name}/{label}: expected exactly 319 loop/source iterations")
    require([row["index"] for row in iterations] == list(range(319)),
            f"{route_name}/{label}: loop iteration numbering is discontinuous")
    require(all(row["states"].get("before_source") for row in iterations),
            f"{route_name}/{label}: missing pre-sample register witness")
    require(all(row["states"].get("after_uv_step") for row in iterations),
            f"{route_name}/{label}: missing post-step register witness")
    require([int(event["pc_rva"], 16) for event in source_events] == [route["source_pc"]] * 319,
            f"{route_name}/{label}: source loads came from an unexpected instruction")
    require(all(event["value"] == (0 if all_zero else event["value"]) for event in source_events),
            f"{route_name}/{label}: all-zero texture was not observed")
    require(all(0 <= event["value"] <= 255 for event in source_events),
            f"{route_name}/{label}: source load was not a byte")
    pixels = [(event["address"] and
               ((int(event["address"], 16) - geom.SURFACE) % geom.SURFACE_STRIDE,
                (int(event["address"], 16) - geom.SURFACE) // geom.SURFACE_STRIDE))
              for event in output_events]
    require(pixels == EXPECTED_PIXELS if not all_zero else not pixels,
            f"{route_name}/{label}: destination writes differ from the 319-pixel row")
    require(len(output_events) == (0 if all_zero else 319),
            f"{route_name}/{label}: unexpected output write count")
    if all_zero:
        require(all(event["value"] == 0 for event in source_events),
                f"{route_name}/{label}: zero fixture sampled a nonzero source")
    else:
        require(all(event["value"] != 0 for event in source_events),
                f"{route_name}/{label}: nonzero fixture sampled a zero source")
    order = validate_event_order(capture, route_name, all_zero)
    require(native.error is None and native.stopped,
            f"{route_name}/{label}: native route did not stop cleanly: {native.error}")
    trace_set = set(native.trace)
    if route_name == "cc50_palette":
        require({0xD420, 0xCC50, 0xD322} <= trace_set and 0xD60C not in trace_set,
                f"{route_name}/{label}: D420 did not take the active CC50 route")
        zero_target = 0xD343
    elif route_name == "e680_mode80":
        require({0xE680, 0xE7C7, 0xEE6D} <= trace_set and
                0xE7BB not in trace_set and 0xDD70 not in trace_set,
                f"{route_name}/{label}: E680 did not take the mode-0x80 generic route")
        zero_target = 0xEE8E
    else:
        require({0xE680, 0xE6AD, 0xDD70, 0xDE9F, 0xE56C} <= trace_set and
                0xE7C7 not in trace_set and 0xC700 not in trace_set,
                f"{route_name}/{label}: E680 did not take the supported 0x7F shaded generic route")
        zero_target = 0xE5A3
    if all_zero:
        require(all("after_output_or_skip" in row["states"] for row in iterations) and
                zero_target in trace_set,
                f"{route_name}/{label}: zero-test skip target was not reached")
    require(result.get("original_code_unchanged") is True,
            f"{route_name}/{label}: public helper saw original code mutation")
    code_unchanged = all(bytes(native.uc.mem_read(BASE + lo, hi - lo)) == old
                         for (lo, hi), old in native.code_snapshot.items())
    require(code_unchanged, f"{route_name}/{label}: original code snapshot changed")
    states = iterations
    selected = [0, 192, 193]
    checkpoints = {}
    for index in selected:
        row = states[index]
        checkpoints[str(index)] = {
            "pixel": [index, 1],
            "uv_before_source": {name: row["states"]["before_source"][name]
                                 for name in route["uv_registers"]},
            "uv_after_step": {name: row["states"]["after_uv_step"][name]
                              for name in route["uv_registers"] if name != "edx"},
            "source_byte": source_events[index]["value"],
            "source_offset": hex(int(source_events[index]["address"], 16) - geom.TEXTURE),
        }
        if "after_shade" in row["states"]:
            checkpoints[str(index)]["after_shade_edx"] = hex(row["states"]["after_shade"]["edx"])
    event_sample = []
    for event in capture["events"]:
        if event["iteration"] != 192:
            continue
        address = int(event["address"], 16)
        if event["kind"] == "source":
            relative = address - geom.TEXTURE
        elif event["kind"] in ("destination", "output"):
            relative = address - geom.SURFACE
        elif event["kind"] == "palette":
            relative = address - (geom.CLUT_BASE + 3 * 256)
        elif event["kind"] == "shade":
            relative = address - (SHADE_BASE + (SHADE_MODE << 8))
        else:
            relative = address - (BLEND_BASE + (SELECTOR << 16))
        event_sample.append({"kind": event["kind"], "pc_rva": event["pc_rva"],
                             "route_relative_index": hex(relative), "value": event["value"]})
    return {
        "label": label,
        "route": route_name,
        "entry": route["entry"],
        "native_instruction_count": len(native.trace),
        "native_path_markers": sorted({hex(rva) for rva in native.trace
                                        if rva in (0xD420, 0xCC50, 0xD322, 0xE680,
                                                   0xE6AD, 0xE7C7, 0xDD70, 0xDE9F,
                                                   0xE56C, 0xEE6D, 0xEE8E, 0xE5A3)}),
        "loop_iterations": len(iterations),
        "source_read_count": len(source_events),
        "output_write_count": len(output_events),
        "event_order": order,
        "checkpoints": checkpoints,
        "sample_192_event_order_and_values": event_sample,
        "original_code_unchanged": code_unchanged,
        "unexpected_access": native.error,
    }


def iterations_ax(capture: dict) -> list[int]:
    return [row["states"]["loop_start"]["eax"] & 0xFFFF
            for row in capture["iterations"]]


def first_register_divergence(left: dict, right: dict, stage_order: tuple[str, ...]) -> dict | None:
    left_rows = left["capture"]["iterations"]
    right_rows = right["capture"]["iterations"]
    for index, (a, b) in enumerate(zip(left_rows, right_rows)):
        for stage in stage_order:
            sa, sb = a["states"].get(stage), b["states"].get(stage)
            if sa is None and sb is None:
                continue
            if sa is None or sb is None:
                return {"iteration": index, "pixel": [index, 1], "stage": stage,
                        "kind": "state_presence_mismatch"}
            for register in REG_ORDER:
                if sa[register] != sb[register]:
                    return {"iteration": index, "pixel": [index, 1], "stage": stage,
                            "register": register, "left": hex(sa[register]),
                            "right": hex(sb[register])}
    return None


def compare_runs(left: dict, right: dict, route_name: str, purpose: str) -> dict:
    route = ROUTES[route_name]
    la, rb = left["capture"]["iterations"], right["capture"]["iterations"]
    require(len(la) == len(rb) == 319, f"{route_name}/{purpose}: paired runs differ in loop count")
    uv_before_source_equal = all(
        all(a["states"]["before_source"][reg] == b["states"]["before_source"][reg]
            for reg in route["uv_registers"])
        for a, b in zip(la, rb))
    uv_after_step_registers = tuple(reg for reg in route["uv_registers"] if reg != "edx")
    uv_after_step_equal = all(
        all(a["states"]["after_uv_step"][reg] == b["states"]["after_uv_step"][reg]
            for reg in uv_after_step_registers)
        for a, b in zip(la, rb))
    offsets_left = [int(e["address"], 16) - geom.TEXTURE
                    for e in left["capture"]["events"] if e["kind"] == "source"]
    offsets_right = [int(e["address"], 16) - geom.TEXTURE
                     for e in right["capture"]["events"] if e["kind"] == "source"]
    offsets_equal = offsets_left == offsets_right
    require(uv_before_source_equal and uv_after_step_equal and offsets_equal,
            f"{route_name}/{purpose}: controlled byte changed later UV state/address")
    first = first_register_divergence(left, right, route["stages"])
    return {
        "purpose": purpose,
        "left_case": left["summary"]["label"], "right_case": right["summary"]["label"],
        "loop_count": 319,
        "uv_registers_before_source_equal_all_iterations": uv_before_source_equal,
        "uv_registers_after_step_equal_all_iterations": uv_after_step_equal,
        "source_texture_offsets_equal_all_iterations": offsets_equal,
        "first_register_divergence": first,
    }


def run_case_with_capture(pe, data_region, route_name, label, **kwargs) -> dict:
    # Keep full per-iteration state only in memory for paired comparison; the
    # receipt stores selected checkpoints and the first divergence witness.
    route = ROUTES[route_name]
    native_route = "cc50_palette" if route_name == "cc50_palette" else "d60c_rowzero"
    native = geom.NativeCase(pe, data_region, SPEC, native_route)
    set_extra_code_snapshots(native)
    if route_name == "cc50_palette":
        configure_cc50(native, kwargs.get("source", 1), kwargs.get("output", 0x66),
                       kwargs.get("all_zero", False))
    else:
        source_values = kwargs.get("source_values", (1,))
        configure_e680_tables(native, route_name, kwargs.get("source", 1), source_values,
                              output=kwargs.get("output", 0x66),
                              shade_value=kwargs.get("shade_value", 0x22),
                              shade_values=kwargs.get("shade_values", ()),
                              all_zero=kwargs.get("all_zero", False))
        quad.initialize_reciprocal_table(native)
    cap = instrumentation(native, route_name)
    if route_name == "cc50_palette":
        result = native.run()
    else:
        geom.put32(native.uc, geom.FRAME, geom.SENTINEL)
        native.uc.reg_write(UC_X86_REG_ESP, geom.FRAME)
        native.uc.reg_write(UC_X86_REG_EBP, 0)
        native.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        try:
            native.uc.emu_start(BASE + 0xE680, 0,
                                timeout=2_000_000, count=MAX_INSTRUCTIONS)
        except UcError as error:
            if native.error is None:
                native.error = {"kind": "unicorn_error", "detail": str(error),
                                "rva": hex(native.uc.reg_read(geom.UC_X86_REG_EIP) - BASE)}
        result = {"original_code_unchanged": all(
            bytes(native.uc.mem_read(BASE + lo, hi - lo)) == old
            for (lo, hi), old in native.code_snapshot.items())}
    summary = summarize_case(route_name, label, native, result, cap,
                             kwargs.get("all_zero", False))
    summary["loop_contract"] = {
        "loop_pc": hex(route["loop_start"]),
        "first_ax_count": hex(iterations_ax(cap)[0]),
        "last_ax_count": hex(iterations_ax(cap)[-1]),
        "counter_width": "AX",
        "source_pc": hex(route["source_pc"]),
        "output_pc": hex(route["output_pc"]),
    }
    return {"summary": summary, "capture": cap}


def main() -> None:
    fixture_geometry()
    pe = PE(ARGS.exe)
    require(sha(pe.data) == EXPECTED_EXE_SHA256 and pe.image_base == BASE,
            "pinned original EXE identity or base mismatch")
    require(not getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None),
            "unexpected original relocation directory")
    data_section = next(section for section in pe.sections if section["name"] == ".data")
    data_region = (BASE + data_section["virtual_address"],
                   max(data_section["virtual_size"], data_section["raw_size"]))
    runtime = quad.verify_runtime(unicorn, ARGS.deps)
    spans = verify_spans(pe)

    full_runs: dict[str, dict[str, dict]] = {name: {} for name in ROUTES}
    reports: dict[str, list[dict]] = {name: [] for name in ROUTES}

    def record(route_name: str, label: str, **kwargs):
        full = run_case_with_capture(pe, data_region, route_name, label, **kwargs)
        full_runs[route_name][label] = full
        reports[route_name].append(full["summary"])
        return full

    # Raw-byte pair with outputs held constant; then table-result pair with
    # source held constant; finally an all-zero route for each family.
    c_raw1 = record("cc50_palette", "raw_1", source=1, output=0x44)
    c_raw255 = record("cc50_palette", "raw_255", source=255, output=0x44)
    c_lut1 = record("cc50_palette", "clut_44", source=1, output=0x44)
    c_lut2 = record("cc50_palette", "clut_e1", source=1, output=0xE1)
    c_zero = record("cc50_palette", "all_zero", source=0, output=0xE1, all_zero=True)

    m_raw1 = record("e680_mode80", "raw_1", source=1, source_values=(1,), output=0x66)
    m_raw255 = record("e680_mode80", "raw_255", source=255,
                      source_values=(1, 255), output=0x66)
    m_blend1 = record("e680_mode80", "blend_66", source_values=(1,), output=0x66)
    m_blend2 = record("e680_mode80", "blend_e2", source_values=(1,), output=0xE2)
    m_zero = record("e680_mode80", "all_zero", source_values=(1,), output=0x66, all_zero=True)

    s_raw1 = record("e680_shaded_7f", "raw_1", source=1, source_values=(1,),
                    output=0x66, shade_value=0x22, shade_values=(0x22,))
    s_raw255 = record("e680_shaded_7f", "raw_255", source=255,
                      source_values=(1, 255),
                      output=0x66, shade_value=0x22, shade_values=(0x22,))
    s_shade1 = record("e680_shaded_7f", "shade_22", source_values=(1,),
                      output=0x66, shade_value=0x22, shade_values=(0x22,))
    s_shade2 = record("e680_shaded_7f", "shade_d0", source_values=(1,),
                      output=0x66, shade_value=0xD0, shade_values=(0x22, 0xD0))
    s_blend1 = record("e680_shaded_7f", "blend_66", source_values=(1,),
                      output=0x66, shade_value=0x22, shade_values=(0x22,))
    s_blend2 = record("e680_shaded_7f", "blend_e4", source_values=(1,),
                      output=0xE4, shade_value=0x22, shade_values=(0x22,))
    s_zero = record("e680_shaded_7f", "all_zero", source_values=(1,),
                    output=0x66, shade_value=0x22, shade_values=(0x22,), all_zero=True)

    comparisons = {
        "cc50_raw_texel": compare_runs(c_raw1, c_raw255, "cc50_palette", "raw texel 1 vs 255, same CLUT result"),
        "cc50_clut_result": compare_runs(c_lut1, c_lut2, "cc50_palette", "same texel 1, CLUT result 0x44 vs 0xE1"),
        "e680_mode80_raw_texel": compare_runs(m_raw1, m_raw255, "e680_mode80", "raw texel 1 vs 255, same blend output"),
        "e680_mode80_blend_result": compare_runs(m_blend1, m_blend2, "e680_mode80", "same texel 1, blend result 0x66 vs 0xE2"),
        "e680_shaded_raw_texel": compare_runs(s_raw1, s_raw255, "e680_shaded_7f", "raw texel 1 vs 255, same shade and blend results"),
        "e680_shade_result": compare_runs(s_shade1, s_shade2, "e680_shaded_7f", "same texel 1, shade result 0x22 vs 0xD0, same blend output"),
        "e680_shaded_blend_result": compare_runs(s_blend1, s_blend2, "e680_shaded_7f", "same texel and shade, blend result 0x66 vs 0xE4"),
    }
    for route_name in ROUTES:
        zero = next(row for row in reports[route_name] if row["label"] == "all_zero")
        require(zero["loop_iterations"] == zero["source_read_count"] == 319 and
                zero["output_write_count"] == 0,
                f"{route_name}: all-zero route did not consume samples and skip outputs")
    require(all(all(report["loop_contract"]["first_ax_count"] == "0x13f" and
                    report["loop_contract"]["last_ax_count"] == "0x1"
                    for report in route_reports)
                for route_reports in reports.values()),
            "one route did not use AX as the 319-to-1 loop counter")

    source_scripts = {
        "public_quad_helper": {"path": str(QUAD_HELPER_PATH), "sha256": sha(QUAD_HELPER_PATH.read_bytes())},
        "public_polygon_helper": {
            "path": str(ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"),
            "sha256": sha((ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py").read_bytes())},
        "oracle_common": {
            "path": str(ROOT / "renderer_contract" / "tests" / "oracle_common.py"),
            "sha256": sha((ROOT / "renderer_contract" / "tests" / "oracle_common.py").read_bytes())},
    }
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "proof_scope": "Original D420->CC50, E680 mode-0x80 generic, and E680 mode-0x7F->DD70 generic loops on one 319-pixel synthetic row; paired source/LUT inputs, register witnesses, access order, and zero-source skips.",
        "prediction": PREDICTION,
        "historical_context": {
            "canonical_native_packed_uv_feedback": optional_canonical_native_feedback_reference(),
            "d60c_feedback_receipt": HISTORICAL_D60C_RECEIPT,
        },
        "oracle": {"path": str(pe.path), "sha256": sha(pe.data), "image_base": hex(pe.image_base)},
        "runtime": runtime,
        "tooling": {"python_executable": sys.executable,
                    "python_version": sys.version.split()[0],
                    "runner_sha256": sha(Path(__file__).read_bytes()),
                    "helpers": source_scripts},
        "fixture": {"vertices_xy": [list(point) for point in SPEC["vertices"]],
                    "clip_ltrb": list(SPEC["clip"]),
                    "uv_q16": [list(pair) for pair in SPEC["attributes"]],
                    "synthetic_surface": {"va": hex(geom.SURFACE),
                                          "stride": geom.SURFACE_STRIDE,
                                          "height": geom.SURFACE_HEIGHT,
                                          "initial_byte": hex(DESTINATION_INITIAL)},
                    "source_runs": [1, 255], "zero_run": "all-zero texture",
                    "row_pixel_count": 319, "raw_source_discriminator_texture_offset": hex(SOURCE_ADDRESS_DISCRIMINATOR)},
        "original_spans": [{"module": "HERCULES.EXE", **span} for span in spans],
        "route_contracts": {
            "cc50_palette": {"loop_rva": "0xd322", "source_read_rva": "0xd32e",
                             "palette_read_rva": "0xd33d", "write_rva": "0xd341",
                             "packed_uv_lanes": ["EAX accumulator/high word; AX count low word",
                                                 "EBX:ECX carry/update; BL supplies U", "CH supplies V"],
                             "source_and_palette_result_lane": "DL; reloaded from BL and CH for each source address",
                             "zero_branch": "D339 test DL -> D343 skips palette read and output"},
            "e680_mode80": {"loop_rva": "0xee6d", "source_read_rva": "0xee79",
                             "destination_read_rva": "0xee86", "blend_read_rva": "0xee88",
                             "write_rva": "0xee8c", "packed_uv_lanes": ["EAX accumulator/high word; AX count low word",
                                                                       "EBX/ECX packed carry/update", "ESI texture pointer with ADC correction"],
                             "source_and_blend_lanes": "DH holds source; DL holds destination then blend output; next iteration reloads DH from BL and clears DL",
                             "zero_branch": "EE82 test DH -> EE8E skips destination, blend, and output"},
            "e680_shaded_7f": {"loop_rva": "0xe56c", "source_read_rva": "0xe578",
                               "shade_read_rva": "0xe593", "destination_read_rva": "0xe59b",
                               "blend_read_rva": "0xe59d", "write_rva": "0xe5a1",
                               "packed_uv_lanes": ["EAX accumulator/high word; AX count low word",
                                                   "EBX/ECX packed carry/update", "ESI texture pointer with ADC correction"],
                               "source_shade_blend_lanes": "DH receives raw source, shade lookup result, then DL receives destination and blend output; next iteration reloads DH from BL and clears DL",
                               "zero_branch": "E581 test DH -> E5A3 skips shade, destination, blend, and output"}},
        "runs": reports,
        "controlled_comparisons": comparisons,
        "interpretation": {
            "cc50": "The long-row source address stream is independent of raw texel and palette output in this finite route; palette result affects only the destination byte.",
            "e680_mode80": "The long-row source address stream is independent of raw texel and blend output in this finite route; blend result affects only the destination byte.",
            "e680_shaded_7f": "The long-row source address stream is independent of raw texel, shade result, and blend output in this finite route; shade/blend bytes stay on the color path.",
            "cross_route_limit": "This evidence distinguishes these three routes from the existing D60C AL-feedback witness; it does not generalize to other modes, selectors, shapes, clipping domains, or source/LUT ownership states."
        },
        "limits": ["Only one wire-valid clipped 319-pixel row was executed per controlled case.",
                   "All tables, texture, stack, and destination bytes are synthetic emulator memory.",
                   "No host callback, DirectDraw, GPU, full raster, gameplay, or unsupported mode/selector was executed.",
                   "Register equality is limited to the recorded loop landmarks and these paired inputs."],
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ROOT),
                       "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
                       "receipt": str(ARGS.receipt), "instruction_budget_per_case": MAX_INSTRUCTIONS},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"],
                      "routes": {name: [{"case": row["label"], "loops": row["loop_iterations"],
                                         "reads": row["source_read_count"], "writes": row["output_write_count"],
                                         "instructions": row["native_instruction_count"]}
                                        for row in route_reports]
                                 for name, route_reports in reports.items()},
                      "comparisons": {name: {"uv_before_source_equal": row["uv_registers_before_source_equal_all_iterations"],
                                             "uv_after_step_equal": row["uv_registers_after_step_equal_all_iterations"],
                                             "source_offsets_equal": row["source_texture_offsets_equal_all_iterations"],
                                             "first_register_divergence": row["first_register_divergence"]}
                                      for name, row in comparisons.items()},
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
