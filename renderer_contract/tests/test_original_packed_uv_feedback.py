"""Finite D60C test for texture-byte feedback into later V samples."""
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
PUBLIC_HELPER = ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"
SPAN_KEYS = ("reciprocal_table_initializer", "d420_dispatch_and_rowzero_function",
             "d60c_rowzero_function")
SPAN_START, SPAN_END = 0xDC35, 0xDCA0
MAX_SAMPLE_COUNT = 320
TARGET_PREVIOUS_PIXEL = (192, 1)
TARGET_PIXEL = (193, 1)
ROUTE = "d60c_rowzero"

# Convex trapezoid with a 319-sample row at y=1. V changes by -1 across that
# row and by +219 over 361 rows on both side edges. All XY are signed-i16 and
# all UV components are unsigned bytes.
SPEC = {
    "name": "long_span_texel_feedback_v_boundary",
    "vertices": [(0, 0), (320, 0), (319, 361), (0, 361)],
    "clip": (0, 1, 320, 2),
    "uv_mode": "fractional_gradient",
    "attributes": [(0, 1 << 16), (0, 0), (0, 219 << 16), (0, 220 << 16)],
    "attribute_domain": "wire_unsigned_u8_q16",
}

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--exe", required=True, type=Path,
                    help="read-only pinned original HERCULES.EXE")
parser.add_argument("--original-root", required=True, type=Path,
                    help="repository root containing scripts/pe.py and work/")
parser.add_argument("--receipt", required=True, type=Path,
                    help="JSON output under the repository's ignored work/")
parser.add_argument("--deps", type=Path,
                    default=Path(r"C:\tools\hercules\renderer-oracle-python"))
parser.add_argument("--analysis-deps", type=Path,
                    default=Path(r"C:\tools\hercules\python"))
ARGS = parser.parse_args()
ARGS.exe = ARGS.exe.resolve()
ARGS.original_root = ARGS.original_root.resolve()
ARGS.receipt = ARGS.receipt.resolve()
ARGS.deps = ARGS.deps.resolve()
ARGS.analysis_deps = ARGS.analysis_deps.resolve()
if ARGS.original_root != ROOT.resolve():
    parser.error("--original-root must match the repository root discovered from this test")
try:
    ARGS.receipt.relative_to((ROOT / "work").resolve())
except ValueError as error:
    raise SystemExit("--receipt must remain under ignored repository work/") from error
if not ARGS.exe.is_file() or not ARGS.deps.is_dir() or not ARGS.analysis_deps.is_dir():
    parser.error("original executable or pinned Python dependency path is missing")

sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
sys.path.insert(0, str(ROOT / "scripts"))

# Import the public original-byte oracle without running its report-producing
# main. It checks the exact EXE identity and establishes the expected runtime.
saved_argv = sys.argv[:]
sys.argv = [str(PUBLIC_HELPER), "--exe", str(ARGS.exe),
            "--receipt", str(ARGS.receipt), "--original-root", str(ROOT),
            "--deps", str(ARGS.deps), "--analysis-deps", str(ARGS.analysis_deps)]
helper_spec = importlib.util.spec_from_file_location("public_polygon_edge_helper", PUBLIC_HELPER)
if helper_spec is None or helper_spec.loader is None:
    raise RuntimeError("could not load public polygon-edge helper")
geom = importlib.util.module_from_spec(helper_spec)
sys.modules[helper_spec.name] = geom
try:
    helper_spec.loader.exec_module(geom)
finally:
    sys.argv = saved_argv

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_ESI,
)

# Original 8-bit framebuffer pitch. The helper's default 32-byte scratch pitch
# aliases screen x=320 into the next row and cannot represent this fixture.
geom.SURFACE_STRIDE = 320
geom.SURFACE_SIZE = geom.SURFACE_STRIDE * geom.SURFACE_HEIGHT

EXE_SHA256 = geom.EXE_SHA
BASE = geom.BASE
TEXTURE = geom.TEXTURE
GLOBALS = {
    "u_step": 0xA90938,
    "v_step": 0xA9093C,
    "destination": 0xA90940,
    "u_start": 0xA90990,
    "v_start": 0xA90994,
    "remaining_pixels": 0xA9096C,
    "texture_base": 0xA909FC,
}
REGISTERS = {
    "eax": UC_X86_REG_EAX,
    "ebx": UC_X86_REG_EBX,
    "ecx": UC_X86_REG_ECX,
    "edx": UC_X86_REG_EDX,
    "esi": UC_X86_REG_ESI,
    "edi": UC_X86_REG_EDI,
    "ebp": UC_X86_REG_EBP,
    "eflags": UC_X86_REG_EFLAGS,
}
HOOKS = {
    0xDC8A: "before_texture_read",
    0xDC8D: "after_texture_read",
    0xDC8F: "after_add_ebx_edx",
    0xDC91: "after_adc_cl_dl_and_inc_edi",
    0xDC98: "before_add_eax_ebp",
    0xDC9A: "after_add_eax_ebp",
    0xDC9C: "after_adc_ch_dh",
    0xDC9E: "after_dec_bp_before_jg",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def load_static_spans(pe) -> list[dict]:
    spans = []
    for key in SPAN_KEYS:
        start, end, expected_sha = geom.EXPECTED_SPANS[key]
        actual_sha = sha256(pe.read_rva(start, end - start))
        require(actual_sha == expected_sha, f"pinned span {key} changed: {actual_sha}")
        spans.append({"module": "HERCULES.EXE", "name": key,
                      "rva": hex(start), "rva_end_exclusive": hex(end),
                      "size": end - start, "sha256": actual_sha})
    loop = pe.read_rva(SPAN_START, SPAN_END - SPAN_START)
    spans.append({"module": "HERCULES.EXE", "name": "d60c_packed_uv_loop",
                  "rva": hex(SPAN_START), "rva_end_exclusive": hex(SPAN_END),
                  "size": len(loop), "sha256": sha256(loop)})
    return spans


def strict_convex(vertices) -> bool:
    turns = []
    for i, point in enumerate(vertices):
        nxt = vertices[(i + 1) % 4]
        after = vertices[(i + 2) % 4]
        turns.append((nxt[0] - point[0]) * (after[1] - nxt[1]) -
                     (nxt[1] - point[1]) * (after[0] - nxt[0]))
    return all(turn > 0 for turn in turns) or all(turn < 0 for turn in turns)


def capture_run(pe, data_region, texel: int) -> dict:
    case = geom.NativeCase(pe, data_region, SPEC, ROUTE)
    case.uc.mem_write(TEXTURE, bytes([texel]) * 0x10000)
    setup = None
    iterations = []
    current = None

    def on_code(uc, address, _size, _user_data):
        nonlocal setup, current
        rva = address - BASE
        if rva == SPAN_START:
            setup = {
                "globals": {
                    name: int.from_bytes(bytes(uc.mem_read(cell, 4)), "little")
                    for name, cell in GLOBALS.items()
                },
                "registers": {name: uc.reg_read(register) & 0xFFFFFFFF
                              for name, register in REGISTERS.items()},
            }
        label = HOOKS.get(rva)
        if label is None:
            return
        registers = {name: uc.reg_read(register) & 0xFFFFFFFF
                     for name, register in REGISTERS.items()}
        if rva == 0xDC8A:
            current = {"index": len(iterations), "states": {}}
            iterations.append(current)
        if current is not None:
            current["states"][label] = registers

    case.uc.hook_add(UC_HOOK_CODE, on_code)
    result = case.run()
    require(setup is not None, f"texel {texel}: loop setup globals were not captured")
    require(case.error is None, f"texel {texel}: unexpected native memory access {case.error}")
    require(case.stopped and case.reciprocal_init_stopped,
            f"texel {texel}: native bounded execution missed a sentinel")
    require(result["route"] == ROUTE and result["entry_rva"] == "0xd420",
            f"texel {texel}: wrong D420 route")
    require(result["original_code_unchanged"] is True,
            f"texel {texel}: original code snapshot changed")
    require(0xD60C in case.trace and 0xD549 not in case.trace,
            f"texel {texel}: generic D60C body not reached")
    require({"0xd60c", "0xdc35", "0xdc8a", "0xdc96"}.issubset(set(result["path_markers"])
            | {hex(rva) for rva in case.trace}),
            f"texel {texel}: D60C setup/read/store markers are incomplete")

    pre = result["runtime_precondition"]
    require(pre["reciprocal_initializer_span"] == ["0x72a0", "0x72cc"] and
            pre["entry_count"] == geom.RECIP_COUNT and
            pre["initializer_instruction_count"] > 0 and
            len(case.reciprocal_table_writes) == geom.RECIP_COUNT,
            f"texel {texel}: reciprocal initializer precondition failed")
    for index in (1, 2, 3, geom.RECIP_COUNT - 1):
        expected = 0 if index == 0 else 0x40000000 // index
        require(int(pre["checked_entries"][str(index)], 16) == expected,
                f"texel {texel}: reciprocal entry {index} mismatch")

    require(len(iterations) == len(case.source_reads) == len(case.surface_writes),
            f"texel {texel}: loop/read/store event counts disagree")
    require(0 < len(iterations) <= MAX_SAMPLE_COUNT,
            f"texel {texel}: span count is outside the chosen bounded case")
    require(all(event["pc"] == "0xdc8a" for event in case.source_reads),
            f"texel {texel}: source read did not come from the packed loop")
    require(all(event["pc"] == "0xdc96" for event in case.surface_writes),
            f"texel {texel}: store did not come from the packed loop")
    require(all(event["value"] == texel for event in case.source_reads),
            f"texel {texel}: observed texture bytes differ from selected constant")

    # Check exact setup packing before pairing the native ordered event lists.
    g = setup["globals"]
    require(g["remaining_pixels"] == len(iterations) == 319,
            f"texel {texel}: expected 319 packed samples, got {g['remaining_pixels']}")
    v_step = g["v_step"]
    v_start = g["v_start"]
    expected_ebp = (((v_step & 0xFFFF) << 16) | (g["remaining_pixels"] & 0xFFFF)) & 0xFFFFFFFF
    expected_eax = (v_start << 16) & 0xFFFFFFFF
    first = iterations[0]["states"]["before_texture_read"]
    require(first["ebp"] == expected_ebp and first["eax"] == expected_eax,
            f"texel {texel}: packed EAX/EBP setup differs from decoded globals")
    for index, (iteration, read, write) in enumerate(
            zip(iterations, case.source_reads, case.surface_writes)):
        require(iteration["index"] == index,
                f"texel {texel}: sample index discontinuity")
        before = iteration["states"]["before_texture_read"]
        after_dec = iteration["states"]["after_dec_bp_before_jg"]
        expected_rem = len(iterations) - index
        require((before["ebp"] & 0xFFFF) == expected_rem and
                (before["ebp"] >> 16) == (v_step & 0xFFFF),
                f"texel {texel}: packed pixel countdown/step lane changed at sample {index}")
        require((after_dec["ebp"] & 0xFFFF) == expected_rem - 1 and
                (after_dec["ebp"] >> 16) == (v_step & 0xFFFF),
                f"texel {texel}: DEC BP did not preserve the high step lane at sample {index}")
        offset = write["offset"]
        iteration["pixel"] = [offset % geom.SURFACE_STRIDE,
                               offset // geom.SURFACE_STRIDE]
        iteration["uv"] = [read["u"], read["v"]]
        iteration["sampled_texel"] = read["value"]
        require(iteration["pixel"] == [index, 1],
                f"texel {texel}: expected consecutive x samples at y=1; got {iteration['pixel']}")
    require((iterations[-1]["states"]["after_dec_bp_before_jg"]["ebp"] & 0xFFFF) == 0,
            f"texel {texel}: BP did not reach zero after the last sample")

    return {
        "texel": texel,
        "native_run_status": "PASS_SENTINEL_AND_D60C_ROUTE",
        "route": result["route"],
        "entry_rva": result["entry_rva"],
        "path_markers": result["path_markers"],
        "setup": setup,
        "runtime_precondition": pre,
        "sample_count": len(iterations),
        "source_read_count": len(case.source_reads),
        "surface_write_count": len(case.surface_writes),
        "original_code_unchanged": result["original_code_unchanged"],
        "unexpected_access": case.error,
        "iterations": iterations,
        "stream": [[row["pixel"], row["uv"]] for row in iterations],
    }


def validate_step_setup(run: dict) -> dict:
    setup = run["setup"]["globals"]
    first = run["iterations"][0]["states"]["before_texture_read"]
    u_start = setup["u_start"]
    v_start = setup["v_start"]
    u_step = setup["u_step"]
    v_step = setup["v_step"]
    expected_ebx = (u_start << 16) & 0xFFFFFFFF
    expected_ecx = ((u_start >> 16) & 0xFF) | (((v_start >> 16) & 0xFF) << 8)
    rotated_u = ((u_step << 16) | (u_step >> 16)) & 0xFFFFFFFF
    expected_edx = (rotated_u & 0xFFFF00FF) | (((v_step >> 16) & 0xFF) << 8)
    expected_eax = (v_start << 16) & 0xFFFFFFFF
    expected_ebp = (((v_step & 0xFFFF) << 16) | setup["remaining_pixels"]) & 0xFFFFFFFF
    expected = {"eax": expected_eax, "ebx": expected_ebx,
                "ecx": expected_ecx, "edx": expected_edx,
                "ebp": expected_ebp}
    observed = {name: first[name] for name in expected}
    require(observed == expected,
            f"loop initial register packing mismatch: expected {expected}, got {observed}")
    require(v_start == 0x00019B4D and v_step == 0xFFFFFF32 and
            setup["remaining_pixels"] == 319,
            f"fixture did not produce the planned V boundary state: {setup}")
    return {"globals": setup, "expected_first_loop_regs": expected,
            "observed_first_loop_regs": observed,
            "interpretation": {
                "eax": "V start low 16-bit fractional word in EAX high word; AL is overwritten by each texel.",
                "ebp": "V-step fractional word in EBP high word and remaining pixel count in BP low word.",
                "edx": "ROL16(U-step), with DH replaced by V-step integer low byte.",
                "ecx": "CL starts at U integer low byte and CH at V integer low byte.",
                "ebx": "U start fractional word shifted into the upper half.",
            }}


def validate_texture_cause(run_one: dict, run_ff: dict) -> dict:
    require(len(run_one["stream"]) == len(run_ff["stream"]) == 319,
            "variant streams must be full-length before comparing rows")
    first_difference = None
    for index, (row_one, row_ff) in enumerate(zip(run_one["stream"], run_ff["stream"])):
        if row_one != row_ff:
            first_difference = {"sample_index": index,
                                "texel_1": row_one, "texel_255": row_ff}
            break
    require(first_difference is not None,
            "the selected texel variants did not change any emitted UV in the bounded row")
    require(first_difference["sample_index"] == 193 and
            first_difference["texel_1"][0] == [193, 1] and
            first_difference["texel_255"][0] == [193, 1],
            f"first texel-dependent UV differs at an unexpected location: {first_difference}")
    previous_one = next(row for row in run_one["iterations"]
                        if row["pixel"] == list(TARGET_PREVIOUS_PIXEL))
    previous_ff = next(row for row in run_ff["iterations"]
                       if row["pixel"] == list(TARGET_PREVIOUS_PIXEL))
    require(previous_one["uv"] == previous_ff["uv"],
            "variants diverged before the packed V carry sample")

    cause_rows = []
    for run, previous in ((run_one, previous_one), (run_ff, previous_ff)):
        states = previous["states"]
        after_read = states["after_texture_read"]
        before_add = states["before_add_eax_ebp"]
        after_add = states["after_add_eax_ebp"]
        after_adc = states["after_adc_ch_dh"]
        read_eax = before_add["eax"]
        packed_ebp = before_add["ebp"]
        sum32 = read_eax + packed_ebp
        result32 = sum32 & 0xFFFFFFFF
        carry32 = 1 if sum32 > 0xFFFFFFFF else 0
        observed_cf = after_add["eflags"] & 1
        old_ch = (before_add["ecx"] >> 8) & 0xFF
        dh = (before_add["edx"] >> 8) & 0xFF
        expected_ch = (old_ch + dh + carry32) & 0xFF
        observed_ch = (after_adc["ecx"] >> 8) & 0xFF
        expected_stream_v = run["stream"][193][1][1]
        require((after_read["eax"] & 0xFF) == run["texel"],
                f"texel {run['texel']}: AL did not equal the sampled texture byte")
        require(read_eax == after_read["eax"] and after_add["eax"] == result32 and
                observed_cf == carry32,
                f"texel {run['texel']}: ADD EAX,EBP result/carry mismatch")
        require(expected_ch == observed_ch == expected_stream_v,
                f"texel {run['texel']}: ADC CH,DH next-V mismatch: "
                f"old_ch={old_ch:#x}, dh={dh:#x}, carry={carry32}, "
                f"expected_ch={expected_ch:#x}, observed_ch={observed_ch:#x}, "
                f"next_v={expected_stream_v:#x}, next_uv={run['stream'][193]}")
        cause_rows.append({
            "texel": run["texel"],
            "previous_pixel": list(TARGET_PREVIOUS_PIXEL),
            "previous_uv": previous["uv"],
            "eax_after_texture_read": after_read["eax"],
            "eax_before_v_add": read_eax,
            "ebp_before_v_add": packed_ebp,
            "add_eax_ebp_result": result32,
            "add_eax_ebp_carry": carry32,
            "observed_add_carry": observed_cf,
            "old_ch": old_ch,
            "dh_v_step_integer_byte": dh,
            "expected_ch_after_adc": expected_ch,
            "observed_ch_after_adc": observed_ch,
            "next_pixel_native_v": expected_stream_v,
            "after_dec_bp": states["after_dec_bp_before_jg"]["ebp"],
        })
    require(cause_rows[0]["add_eax_ebp_carry"] == 0 and
            cause_rows[1]["add_eax_ebp_carry"] == 1,
            "expected texel-dependent carry split did not occur")
    require(cause_rows[0]["next_pixel_native_v"] == 0 and
            cause_rows[1]["next_pixel_native_v"] == 1,
            "expected next-sample V difference did not occur")
    return {"first_divergence": first_difference,
            "causal_pixel": list(TARGET_PREVIOUS_PIXEL),
            "register_recurrence": cause_rows}


def main() -> None:
    from pe import PE

    require(sha256(ARGS.exe.read_bytes()) == EXE_SHA256,
            "original HERCULES.EXE pin mismatch")
    pe = PE(ARGS.exe)
    require(pe.image_base == BASE, "original image base mismatch")
    require(strict_convex(SPEC["vertices"]), "fixture must be strictly convex")
    spans = load_static_spans(pe)
    runtime = geom.verify_runtime(geom.unicorn, ARGS.deps)
    data_region = next((BASE + section["virtual_address"],
                        max(section["virtual_size"], section["raw_size"]))
                       for section in pe.sections if section["name"] == ".data")

    runs = [capture_run(pe, data_region, texel) for texel in (1, 255)]
    run_one, run_ff = runs
    setup_contract = validate_step_setup(run_one)
    require(run_one["setup"] == run_ff["setup"],
            "constant-texture variants changed scan setup state")
    causal = validate_texture_cause(run_one, run_ff)
    model_target = geom.expected_uv_integer_reciprocal(
        SPEC["vertices"], SPEC["attributes"], TARGET_PIXEL)

    summary_runs = []
    for run in runs:
        uv_stream = run["stream"]
        summary_runs.append({
            "texel": run["texel"],
            "native_run_status": run["native_run_status"],
            "route": run["route"],
            "entry_rva": run["entry_rva"],
            "path_markers": run["path_markers"],
            "sample_count": run["sample_count"],
            "source_read_count": run["source_read_count"],
            "surface_write_count": run["surface_write_count"],
            "ordered_pixel_uv_sha256": sha256(json.dumps(
                uv_stream, separators=(",", ":")).encode("ascii")),
            "first_pixel_uv": uv_stream[0],
            "pixel_192_1": next(row for row in uv_stream if row[0] == [192, 1]),
            "pixel_193_1": next(row for row in uv_stream if row[0] == [193, 1]),
            "last_pixel_uv": uv_stream[-1],
            "original_code_unchanged": run["original_code_unchanged"],
            "unexpected_access": run["unexpected_access"],
            "packed_setup": setup_contract["globals"],
        })

    receipt = {
        "schema_version": 1,
        "status": "FINITE_TEXTURE_DEPENDENT_V_CARRY_CONFIRMED",
        "proof_scope": "One wire-valid convex FT4, one D60C row of 319 visible pixels, and two constant nonzero texture runs (1 and 255) under pinned original D420->D60C row-zero code.",
        "prediction": {
            "byte_lane": "DC35..DCA0 packs the horizontal V fractional step in EBP high16 and remaining span count in BP low16. MOV AL,[ESI+ECX] replaces the prior EAX low byte before ADD EAX,EBP; ADC CH,DH consumes the resulting 32-bit carry.",
            "fixture": "For the selected y=1 row, the low16 accumulator with texel 255 reaches 0xffff immediately before the packed count addition at sample 192; texel 1 does not. The V fractional high word is staged so this one-bit low16 carry crosses the 32-bit boundary only for texel 255.",
        },
        "falsifier": "Reject the causal prediction if native setup globals or packed first-loop registers differ from the decoded layout, if the two full ordered streams do not first diverge at (193,1), or if the captured ADD/ADC carry calculations fail to reproduce the next native V byte.",
        "oracle": {"path": str(ARGS.exe), "sha256": sha256(pe.data),
                   "image_base": hex(pe.image_base)},
        "runtime": {"engine": runtime["engine"],
                    "unicorn_version": runtime["unicorn_version"],
                    "native_dll_sha256": runtime["native_dll_sha256"],
                    "dependency_origin": runtime["dependency_origin"]},
        "tooling": {"python_executable": sys.executable,
                    "python_version": sys.version.split()[0],
                    "runner": str(Path(__file__).resolve()),
                    "runner_sha256": sha256(Path(__file__).read_bytes()),
                    "public_helper": str(PUBLIC_HELPER),
                    "public_helper_sha256": sha256(PUBLIC_HELPER.read_bytes()),
                    "model_source": {
                        "path": str(PUBLIC_HELPER),
                        "sha256": sha256(PUBLIC_HELPER.read_bytes()),
                        "function": "expected_uv_integer_reciprocal"}},
        "original_spans": spans,
        "fixture": {
            "name": SPEC["name"],
            "native_workspace_xy": [list(vertex) for vertex in SPEC["vertices"]],
            "native_workspace_uv_bytes": [[u >> 16, v >> 16]
                                           for u, v in SPEC["attributes"]],
            "attribute_domain": SPEC["attribute_domain"],
            "f3_clip_ltrb": list(SPEC["clip"]),
            "strict_convex": True,
            "screen_pitch": geom.SURFACE_STRIDE,
            "visible_pixel_range_y1_inclusive": [0, 318],
            "packet_staging_scope": "Native-order workspace is preloaded directly by the established original helper; packet decode and AC60 traversal are outside this fixture.",
        },
        "decoded_loop_contract": {
            "initialization_rvas": ["0xdc35", "0xdc3b", "0xdc3e", "0xdc44",
                                    "0xdc47", "0xdc4c", "0xdc4f", "0xdc51",
                                    "0xdc57", "0xdc5a", "0xdc5f", "0xdc62",
                                    "0xdc64", "0xdc6a", "0xdc70", "0xdc71",
                                    "0xdc77", "0xdc7a", "0xdc7f", "0xdc82",
                                    "0xdc87"],
            "loop_rvas": {"texture_read": "0xdc8a", "u_add": "0xdc8d",
                           "u_adc": "0xdc8f", "pixel_count_decrement": "0xdc9c",
                           "loop_branch": "0xdc9e", "v_add": "0xdc98",
                           "v_adc": "0xdc9a"},
            "packing": setup_contract,
            "per_sample_v_equations": [
                "Eread = (EAX & 0xffffff00) | sampled_texel",
                "The next MOV AL replaces only this byte; EAX[31:8] survives the next iteration.",
                "S = Eread + EBP (unsigned 32-bit addition)",
                "EAX_next = S mod 2^32; CF32 = floor(S / 2^32)",
                "CH_next = (CH + DH + CF32) mod 256",
                "BP_next = (BP - 1) mod 65536; JG repeats while signed BP > 0",
            ],
            "count_contract": "DC7A loads A9096C into EAX and DC7F copies AX into BP. EBP high16 remains the V-step fractional word initialized from A9093C low16; BP low16 is remaining sample count. For this 319-pixel row BP runs 0x013f down to zero.",
        },
        "native_runs": summary_runs,
        "first_texture_dependent_v_difference": causal,
        "independent_model_at_first_difference": {
            "function": "expected_uv_integer_reciprocal",
            "pixel": list(TARGET_PIXEL),
            "model_uv": list(model_target),
            "interpretation": "The geometric Q16 helper has no sampled-texel input, so it returns the same result for both texture runs and cannot represent the native finite V difference.",
        },
        "acceptance_gates": {
            "setup_packing_matches": True,
            "pixel_count_is_319_and_bp_reaches_zero": True,
            "both_original_code_snapshots_unchanged": all(run["original_code_unchanged"] for run in runs),
            "both_runs_have_319_reads_and_writes": all(
                run["source_read_count"] == run["surface_write_count"] == 319 for run in runs),
            "first_difference_is_193_1": causal["first_divergence"]["texel_1"][0] == [193, 1],
            "register_carry_matches_native_v": True,
            "unexpected_access_rejected": all(run["unexpected_access"] is None for run in runs),
        },
        "limits": [
            "One clipped scanline and two constant texture values; no general all-texture proof.",
            "No scan iterator, C implementation, renderer replacement, or generic rasterizer acceptance.",
            "Synthetic source texture and destination memory; no game process or full frame runs.",
            "Packet staging, AC60 traversal, other routes, and hardware presentation are outside scope.",
        ],
        "invocation": {"original_root": str(ROOT), "receipt": str(ARGS.receipt)},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n",
                            encoding="utf-8", newline="\n")
    print(json.dumps({"status": receipt["status"],
                      "first_difference": causal["first_divergence"],
                      "cause": causal["register_recurrence"],
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
