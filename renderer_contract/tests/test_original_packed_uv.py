"""Finite original-byte D60C packed-UV counterexample and register witness."""
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
    raise RuntimeError("could not find repository root with scripts/pe.py and renderer_contract/")


ROOT = find_root(Path(__file__))
PUBLIC_TEST = ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path,
                        help="read-only pinned original HERCULES.EXE")
    parser.add_argument("--original-root", required=True, type=Path,
                        help="repository root containing scripts/pe.py and work/")
    parser.add_argument("--receipt", required=True, type=Path,
                        help="compact JSON output under the repository's ignored work/ directory")
    parser.add_argument("--deps", type=Path,
                        default=Path(r"C:\tools\hercules\renderer-oracle-python"))
    parser.add_argument("--analysis-deps", type=Path,
                        default=Path(r"C:\tools\hercules\python"))
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    args.original_root = args.original_root.resolve()
    args.receipt = args.receipt.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if args.original_root != ROOT.resolve():
        parser.error("--original-root must be the repository root found from this test")
    try:
        args.receipt.relative_to((ROOT / "work").resolve())
    except ValueError as error:
        raise SystemExit("--receipt must stay under ignored repository work/") from error
    if not args.exe.is_file():
        parser.error("--exe does not exist")
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("pinned Unicorn or analysis dependency directory is missing")
    return args


ARGS = parse_args()
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
sys.path.insert(0, str(ROOT / "scripts"))

# Import the established original-byte harness without invoking its CLI main.
# The helper validates the original hash, module dependencies, and output path.
saved_argv = sys.argv[:]
sys.argv = [str(PUBLIC_TEST), "--exe", str(ARGS.exe),
            "--receipt", str(ARGS.receipt),
            "--original-root", str(ROOT), "--deps", str(ARGS.deps),
            "--analysis-deps", str(ARGS.analysis_deps)]
spec = importlib.util.spec_from_file_location("public_polygon_edge_harness", PUBLIC_TEST)
if spec is None or spec.loader is None:
    raise RuntimeError("could not load public original polygon helper")
geom = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = geom
try:
    spec.loader.exec_module(geom)
finally:
    sys.argv = saved_argv

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_ESI,
)


# Widen only synthetic destination memory so screen x=318 stays observable
# instead of aliasing a later y row in the public helper's narrow fixture.
geom.SURFACE_STRIDE = 512
geom.SURFACE_SIZE = geom.SURFACE_STRIDE * geom.SURFACE_HEIGHT


EXE_SHA256 = geom.EXE_SHA
BASE = geom.BASE
TEXTURE = geom.TEXTURE
EXPECTED_SAMPLES = 36
TARGET_PIXEL = (318, 2)
U_CARRY_PIXEL = (317, 2)
ROUTE = "d60c_rowzero"

# Native circular workspace order. This fixture tests the D420/D60C
# interpolation route, not packet staging or an AC60 callback.
SPEC = {
    "name": "packed_uv_skewed_right_edge",
    "vertices": [(315, 1), (324, 2), (323, 8), (314, 7)],
    "clip": (0, 0, 320, 10),
    "uv_mode": "fractional_gradient",
    "attributes": [(250 << 16, 15 << 16), (5 << 16, 25 << 16),
                   (10 << 16, 230 << 16), (240 << 16, 220 << 16)],
    "attribute_domain": "wire_unsigned_u8_q16",
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
LOOP_HOOKS = {
    0xDC8A: "before_texture_read",
    0xDC8D: "after_texture_read",
    0xDC8F: "after_add_ebx_edx",
    0xDC91: "after_adc_cl_dl_and_inc_edi",
    0xDC98: "before_add_eax_ebp",
    0xDC9A: "after_add_eax_ebp",
    0xDC9C: "after_adc_ch_dh",
}
PINNED_SPANS = (
    "reciprocal_table_initializer",
    "d420_dispatch_and_rowzero_function",
    "d60c_rowzero_function",
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise AssertionError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def is_strictly_convex(vertices) -> bool:
    turns = []
    for i, p in enumerate(vertices):
        q = vertices[(i + 1) % 4]
        r = vertices[(i + 2) % 4]
        turns.append((q[0] - p[0]) * (r[1] - q[1]) -
                     (q[1] - p[1]) * (r[0] - q[0]))
    return all(turn > 0 for turn in turns) or all(turn < 0 for turn in turns)


def load_original_spans(pe) -> list[dict]:
    rows = []
    for name in PINNED_SPANS:
        start, end, expected = geom.EXPECTED_SPANS[name]
        actual = sha256(pe.read_rva(start, end - start))
        require(actual == expected, f"pinned {name} span hash changed: {actual}")
        rows.append({"name": name, "rva_start": hex(start),
                     "rva_end_exclusive": hex(end), "size": end - start,
                     "sha256": actual})
    loop_start, loop_end = 0xDC35, 0xDCA0
    rows.append({"name": "d60c_packed_uv_pixel_loop",
                 "rva_start": hex(loop_start), "rva_end_exclusive": hex(loop_end),
                 "size": loop_end - loop_start,
                 "sha256": sha256(pe.read_rva(loop_start, loop_end - loop_start))})
    return rows


def capture_run(pe, data_region, texel: int) -> dict:
    case = geom.NativeCase(pe, data_region, SPEC, ROUTE)
    # Constant nonzero texels ensure every original sample reaches the store.
    case.uc.mem_write(TEXTURE, bytes([texel]) * 0x10000)
    iterations = []
    current = None

    def on_code(uc, address, _size, _user_data):
        nonlocal current
        rva = address - BASE
        label = LOOP_HOOKS.get(rva)
        if label is None:
            return
        registers = {name: uc.reg_read(reg) & 0xFFFFFFFF
                     for name, reg in REGISTERS.items()}
        if rva == 0xDC8A:
            current = {"sample_index": len(iterations), "states": {}}
            iterations.append(current)
        if current is not None:
            current["states"][label] = registers

    case.uc.hook_add(UC_HOOK_CODE, on_code)
    result = case.run()
    require(case.error is None, f"texel {texel}: original harness reported access error {case.error}")
    require(case.stopped and case.reciprocal_init_stopped,
            f"texel {texel}: original route did not reach bounded sentinels")
    require(result["route"] == ROUTE and result["entry_rva"] == "0xd420",
            f"texel {texel}: unexpected native route/entry")
    require(result["original_code_unchanged"] is True,
            f"texel {texel}: original code snapshot changed")
    require("0xd60c" in result["path_markers"] and
            "0xdc8a" in result["path_markers"] and
            "0xdc96" in result["path_markers"],
            f"texel {texel}: expected D420/D60C source-read/store path not observed")
    require(0xD60C in case.trace and 0xD549 not in case.trace,
            f"texel {texel}: D60C not reached or rectangle fast path was taken")
    precondition = result["runtime_precondition"]
    require(precondition["reciprocal_initializer_span"] == ["0x72a0", "0x72cc"] and
            precondition["entry_count"] == geom.RECIP_COUNT and
            precondition["initializer_instruction_count"] > 0,
            f"texel {texel}: reciprocal table precondition was not established")
    require(len(case.reciprocal_table_writes) == geom.RECIP_COUNT,
            f"texel {texel}: reciprocal initializer wrote an unexpected number of entries")
    expected_entries = {str(i): hex(0x40000000 // i)
                        for i in (1, 2, 3, geom.RECIP_COUNT - 1)}
    for index, value in expected_entries.items():
        require(precondition["checked_entries"].get(index) == value,
                f"texel {texel}: reciprocal table[{index}] mismatch")

    require(len(iterations) == len(case.source_reads) == len(case.surface_writes),
            f"texel {texel}: loop hooks, source reads, and stores are not one-to-one")
    require(len(iterations) == EXPECTED_SAMPLES,
            f"texel {texel}: expected {EXPECTED_SAMPLES} original samples, got {len(iterations)}")
    require(all(read["pc"] == "0xdc8a" for read in case.source_reads),
            f"texel {texel}: a source read came from an unexpected instruction")
    require(all(write["pc"] == "0xdc96" for write in case.surface_writes),
            f"texel {texel}: a destination store came from an unexpected instruction")
    require(all(read["value"] == texel for read in case.source_reads),
            f"texel {texel}: synthetic texture contents were not observed as requested")

    # Check lengths before pairing read/write events; pair in execution order.
    require(len(case.source_reads) == len(case.surface_writes) == len(iterations),
            f"texel {texel}: cannot pair a truncated source/store stream")
    for index, (iteration, read, write) in enumerate(
            zip(iterations, case.source_reads, case.surface_writes)):
        require(iteration["sample_index"] == index,
                f"texel {texel}: loop sample index discontinuity")
        offset = write["offset"]
        pixel = [offset % geom.SURFACE_STRIDE, offset // geom.SURFACE_STRIDE]
        iteration["pixel"] = pixel
        iteration["uv"] = [read["u"], read["v"]]
        iteration["sampled_texel"] = read["value"]
        require("before_texture_read" in iteration["states"] and
                "after_add_ebx_edx" in iteration["states"] and
                "after_adc_cl_dl_and_inc_edi" in iteration["states"],
                f"texel {texel}: incomplete packed-loop register capture")

    stream = [[row["pixel"], row["uv"]] for row in iterations]
    require(len(stream) == EXPECTED_SAMPLES, f"texel {texel}: malformed sample stream")
    return {
        "texel": texel,
        "native_run_status": "PASS_SENTINEL_AND_D60C_ROUTE",
        "sample_count": len(iterations),
        "source_read_count": len(case.source_reads),
        "surface_write_count": len(case.surface_writes),
        "route": result["route"],
        "entry_rva": result["entry_rva"],
        "path_markers": result["path_markers"],
        "runtime_precondition": precondition,
        "original_code_unchanged": result["original_code_unchanged"],
        "unexpected_access": case.error,
        "stream": stream,
        "iterations": iterations,
    }


def target_row(run: dict, pixel: tuple[int, int]) -> dict:
    found = [row for row in run["iterations"] if tuple(row["pixel"]) == pixel]
    require(len(found) == 1, f"texel {run['texel']}: target pixel {pixel} is not unique")
    return found[0]


def validate_carry(run: dict) -> dict:
    row = target_row(run, U_CARRY_PIXEL)
    target = target_row(run, TARGET_PIXEL)
    before = row["states"]["before_texture_read"]
    after_add = row["states"]["after_add_ebx_edx"]
    after_adc = row["states"]["after_adc_cl_dl_and_inc_edi"]
    next_before = target["states"]["before_texture_read"]
    unsigned_sum = before["ebx"] + before["edx"]
    expected_ebx = unsigned_sum & 0xFFFFFFFF
    expected_cf = 1 if unsigned_sum > 0xFFFFFFFF else 0
    observed_cf = after_add["eflags"] & 1
    old_cl = before["ecx"] & 0xFF
    dl = before["edx"] & 0xFF
    expected_next_cl = (old_cl + dl + expected_cf) & 0xFF
    observed_after_adc_cl = after_adc["ecx"] & 0xFF
    observed_next_cl = next_before["ecx"] & 0xFF
    require(after_add["ebx"] == expected_ebx and observed_cf == expected_cf == 1,
            "captured ADD EBX,EDX result/carry does not match 32-bit arithmetic")
    require(expected_next_cl == observed_after_adc_cl == observed_next_cl == 151,
            "captured ADC CL,DL does not explain native U at (318,2)")
    require(before["ecx"] & 0xFF == 0xAF and dl == 0xE7 and after_add["ebx"] == 0x0000F79C,
            "packed U register witness values changed")
    return {
        "pixel_before_target": list(U_CARRY_PIXEL),
        "before_read": before,
        "after_add_ebx_edx": after_add,
        "after_adc_cl_dl": after_adc,
        "target_before_read": next_before,
        "add_ebx_edx": {
            "unbounded_sum_hex": hex(unsigned_sum),
            "expected_result_hex": hex(expected_ebx),
            "expected_carry": expected_cf,
            "observed_result_hex": hex(after_add["ebx"]),
            "observed_carry": observed_cf,
            "matches": after_add["ebx"] == expected_ebx and observed_cf == expected_cf,
        },
        "adc_cl_dl": {
            "old_cl": old_cl,
            "dl": dl,
            "incoming_carry": expected_cf,
            "expected_cl": expected_next_cl,
            "observed_cl_after_adc": observed_after_adc_cl,
            "observed_cl_at_next_read": observed_next_cl,
            "matches": expected_next_cl == observed_after_adc_cl == observed_next_cl,
        },
    }


def compact_variant(run: dict) -> dict:
    return {
        "texel": run["texel"],
        "native_run_status": run["native_run_status"],
        "sample_count": run["sample_count"],
        "source_read_count": run["source_read_count"],
        "surface_write_count": run["surface_write_count"],
        "ordered_pixel_uv_sha256": sha256(json.dumps(
            run["stream"], separators=(",", ":")).encode("ascii")),
        "first_pixel_uv": run["stream"][0],
        "last_pixel_uv": run["stream"][-1],
        "original_code_unchanged": run["original_code_unchanged"],
        "runtime_precondition": run["runtime_precondition"],
        "unexpected_access": run["unexpected_access"],
    }


def main() -> None:
    from pe import PE

    require(sha256(ARGS.exe.read_bytes()) == EXE_SHA256,
            "--exe does not match the pinned original HERCULES.EXE SHA-256")
    pe = PE(ARGS.exe)
    require(pe.image_base == BASE, "original image base changed")
    require(is_strictly_convex(SPEC["vertices"]), "fixed FT4 fixture is not strictly convex")
    spans = load_original_spans(pe)
    runtime = geom.verify_runtime(geom.unicorn, ARGS.deps)
    data_region = next((BASE + section["virtual_address"],
                        max(section["virtual_size"], section["raw_size"]))
                       for section in pe.sections if section["name"] == ".data")

    runs = [capture_run(pe, data_region, texel) for texel in (1, 255)]
    left, right = runs
    # Require complete equal lengths before comparing or zipping any run rows.
    require(len(left["stream"]) == len(right["stream"]) == EXPECTED_SAMPLES,
            "texture variants do not both have the full expected stream length")
    require(len(left["iterations"]) == len(right["iterations"]) == EXPECTED_SAMPLES,
            "texture variants do not both have the full register-capture length")
    require(left["stream"] == right["stream"],
            "the two fixed texture variants differ in this fixture's ordered pixel/UV stream")
    require(all(run["original_code_unchanged"] is True and run["unexpected_access"] is None
                for run in runs),
            "an original code snapshot changed or an access error was reported")

    native_target = target_row(left, TARGET_PIXEL)
    separate_q16 = geom.expected_uv_integer_reciprocal(
        SPEC["vertices"], SPEC["attributes"], TARGET_PIXEL)
    require(tuple(native_target["uv"]) == (151, 39),
            f"native target UV changed: {native_target['uv']}")
    require(tuple(separate_q16) == (150, 39),
            f"separate-component Q16 model changed: {separate_q16}")
    carry = validate_carry(left)

    x317_rows = [target_row(run, U_CARRY_PIXEL) for run in runs]
    x317_feedback = []
    for run, row in zip(runs, x317_rows):
        states = row["states"]
        after_read = states["after_texture_read"]
        after_add_v = states["after_add_eax_ebp"]
        after_adc_v = states["after_adc_ch_dh"]
        require((after_read["eax"] & 0xFF) == run["texel"],
                f"texel {run['texel']}: MOV AL,[ESI+ECX] did not expose the sampled byte")
        x317_feedback.append({
            "texel": run["texel"],
            "before_texture_read_eax": states["before_texture_read"]["eax"],
            "after_texture_read_eax": after_read["eax"],
            "after_add_eax_ebp": after_add_v["eax"],
            "after_adc_ch_dh": after_adc_v["ecx"],
            "uv_at_pixel": row["uv"],
        })
    require(x317_feedback[0]["after_add_eax_ebp"] != x317_feedback[1]["after_add_eax_ebp"],
            "fixed texel variants did not produce the captured EAX feedback difference")
    require(x317_feedback[0]["uv_at_pixel"] == x317_feedback[1]["uv_at_pixel"],
            "fixed texel variants differ at the selected feedback pixel")

    source_spans = [
        {"module": "HERCULES.EXE", "name": row["name"],
         "rva": row["rva_start"], "rva_start": row["rva_start"],
         "rva_end_exclusive": row["rva_end_exclusive"], "size": row["size"],
         "sha256": row["sha256"]}
        for row in spans
    ]
    receipt = {
        "schema_version": 1,
        "status": "FINITE_ORIGINAL_COUNTEREXAMPLE",
        "scope": "One strictly convex native-order FT4 workspace fixture through original D420->D60C row-zero. Two constant nonzero texture variants exercise the same 36 sample coordinates. Surface and texture storage are synthetic Unicorn memory; no game process or frame is run.",
        "model_comparison": {
            "status": "FINITE_COUNTEREXAMPLE_CONFIRMED",
            "native_target_uv": native_target["uv"],
            "separate_component_q16_model_uv": list(separate_q16),
            "comparison": "The public separate-component Q16 model is one U byte below the original native read at the selected pixel.",
            "acceptance_claim": "This receipt verifies the finite counterexample and the captured packed-carry arithmetic only; it does not accept a generic iterator or rasterizer model.",
        },
        "prediction": {
            "separate_component_q16": "The independent public model predicts UV (150,39) at pixel (318,2); the original packed recurrence is expected to provide a finite counterexample.",
            "register_feedback": "The original loop loads the sampled source byte into AL before ADD EAX,EBP and ADC CH,DH. This test records the finite EAX effect for texels 1 and 255; it makes no prediction about whether that effect changes a later carry into V outside this fixture.",
        },
        "falsifier": {
            "counterexample": "Falsified if the original target UV is not (151,39), the independent Q16 result is not (150,39), or captured 32-bit ADD/ADC arithmetic does not yield CL=151 at the next read.",
            "finite_texture_pair": "The exact two-run observation fails if the 1 and 255 fixtures do not each produce 36 reads and stores with equal complete ordered pixel/UV streams. No universal texel-independence claim is part of this test.",
        },
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
                    "public_helper": str(PUBLIC_TEST),
                    "public_helper_sha256": sha256(PUBLIC_TEST.read_bytes()),
                    "model_source": {
                        "path": str(PUBLIC_TEST),
                        "sha256": sha256(PUBLIC_TEST.read_bytes()),
                        "function": "expected_uv_integer_reciprocal",
                    }},
        "original_spans": source_spans,
        "fixture": {
            "name": SPEC["name"],
            "native_workspace_order_xy": [list(vertex) for vertex in SPEC["vertices"]],
            "native_workspace_uv_bytes": [[u >> 16, v >> 16] for u, v in SPEC["attributes"]],
            "attribute_domain": SPEC["attribute_domain"],
            "f3_clip_ltrb": list(SPEC["clip"]),
            "strict_convex": True,
            "route": ROUTE,
            "entry_rva": "0xd420",
            "target_pixel": list(TARGET_PIXEL),
            "texture_variants": [1, 255],
            "synthetic_surface_stride": geom.SURFACE_STRIDE,
            "packet_staging_scope": "Native-order D420 workspace is preloaded directly; packet decoding and AC60 staging are not tested here.",
        },
        "native_runs": [compact_variant(run) for run in runs],
        "packed_u_counterexample": {
            "pixel": list(TARGET_PIXEL),
            "native_uv": native_target["uv"],
            "separate_component_q16_uv": list(separate_q16),
            "native_minus_model_u": native_target["uv"][0] - separate_q16[0],
            "carry_witness_at_previous_pixel": carry,
            "acceptance": "PASS" if carry["add_ebx_edx"]["matches"] and carry["adc_cl_dl"]["matches"] else "FAIL",
        },
        "texture_feedback_observation": {
            "source_instruction": "0xdc8a MOV AL,[ESI+ECX]",
            "following_v_update": ["0xdc98 ADD EAX,EBP", "0xdc9a ADC CH,DH"],
            "registers_at_pixel_317_2": x317_feedback,
            "complete_ordered_uv_stream_equal_for_texel_1_and_255": left["stream"] == right["stream"],
            "equal_stream_length": len(left["stream"]) == len(right["stream"]) == EXPECTED_SAMPLES,
            "interpretation": "The source byte changes captured EAX state in this fixture, while both finite runs emit the same 36 UV addresses. Whether texel feedback can change a later carry into CH under another span/state remains unresolved.",
        },
        "acceptance_gates": {
            "native_routes_and_preconditions_pass": True,
            "both_original_code_snapshots_unchanged": all(run["original_code_unchanged"] for run in runs),
            "each_run_has_36_reads_and_writes": all(
                run["source_read_count"] == run["surface_write_count"] == EXPECTED_SAMPLES
                for run in runs),
            "native_target_is_151_39": native_target["uv"] == [151, 39],
            "separate_model_is_150_39": separate_q16 == (150, 39),
            "captured_carry_math_matches": carry["add_ebx_edx"]["matches"] and carry["adc_cl_dl"]["matches"],
            "full_variant_streams_equal_after_length_check": left["stream"] == right["stream"],
            "unexpected_access_rejected": all(run["unexpected_access"] is None for run in runs),
        },
        "limits": [
            "Finite original-byte witness only; this is not a generic FT4 scan implementation.",
            "No claim of universal texel independence; only the two exact constant-texture runs are compared.",
            "Preloaded native-order workspace; packet decode, AC60 traversal, shaded routes, and frame composition are outside scope.",
            "Synthetic stride 512 makes x=318 observable without row aliasing; actual host surface addressing is not proven.",
        ],
        "invocation": {"original_root": str(ROOT), "receipt": str(ARGS.receipt)},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n",
                            encoding="utf-8", newline="\n")
    print(json.dumps({"status": receipt["status"],
                      "target": receipt["packed_u_counterexample"],
                      "variants": receipt["native_runs"],
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
