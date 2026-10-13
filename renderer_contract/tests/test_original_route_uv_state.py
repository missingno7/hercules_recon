"""Build C89 route models and compare them with full original CPU traces."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def find_root(start: Path) -> Path:
    for parent in (start.resolve(), *start.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "hercules_types.h").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("repository root not found")


ROOT = find_root(Path(__file__))
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
BASE = 0x400000
REG_NAMES = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "eflags")
LOOP_START = {"cc50_palette": 0xD322, "e680_mode80": 0xEE6D,
              "e680_shaded_7f": 0xE56C}
EXIT_RVA = {"cc50_palette": 0xD348, "e680_mode80": 0xEE93,
            "e680_shaded_7f": 0xE5A8}
ROUTE_ID = {"cc50_palette": 0, "e680_mode80": 1, "e680_shaded_7f": 2}
PREDICTION = {
    "family": "route-specific raw-register UV transitions for finite CC50 and E680 loops",
    "prediction": "Starting from each original loop's captured pre-source 32-bit register state and sampled byte, three route-specific C89 transitions can reproduce the original instructions through the post-UV-step boundary. They will preserve exact x86 partial-register, carry, count, and pointer-correction semantics. CC50, E680 mode 0x80, and E680 mode 0x7F/DD70 will remain separate models; color-path bytes will be route-specific and will not be generalized into one affine iterator. The 17 existing native cases have clipped counts no greater than 320.",
    "falsifier": "Any compiler output differs from original full-register state at the selected post-step boundary or next loop start; any source/color lane is assigned to a shared UV role without instruction evidence; any carry, partial-register update, count decrement, or ESI correction differs; a modeled transition needs unstated geometry or route setup; or C89/VC5/strict GCC cannot reproduce the captured states without unsupported compiler steering.",
    "fixture": {
        "source_cases": "Reuse the existing 17-case native route fixture and its selected loop landmarks.",
        "count_domain": "The C API accepts positive AX values 1 through 320. Captured fixture inputs start at 319 and count down through 1; AX=320 was not captured.",
        "route_boundaries": ["CC50: D322 pre-source loop body through D348 boundary",
                             "E680 mode 0x80: EE6D loop body through EE93 boundary",
                             "E680 mode 0x7F/DD70: E56C loop body through E5A8 boundary"],
    },
    "scope": "Raw captured register-state transitions for the listed original loops only. No geometry reconstruction, generic affine iterator, GPU, host integration, or unproven setup inputs.",
}

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--package", type=Path, default=Path("renderer_contract"),
                    help="package directory containing model sources (relative to repository root)")
parser.add_argument("--exe", required=True, type=Path)
parser.add_argument("--original-root", required=True, type=Path)
parser.add_argument("--receipt", required=True, type=Path)
parser.add_argument("--build-dir", required=True, type=Path,
                    help="new or empty compiler-output directory below repository work/")
parser.add_argument("--fixture-helper", type=Path,
                    default=ROOT / "renderer_contract" / "tests" / "test_original_route_uv_feedback.py",
                    help="canonical original-byte route fixture helper")
parser.add_argument("--trace-helper", type=Path,
                    help="portable helper that captures full loop-boundary states")
parser.add_argument("--gcc", type=Path, default=Path(r"C:\msys64\mingw64\bin\gcc.exe"))
parser.add_argument("--vc5-toolchain", type=Path,
                    help="optional pinned VC5 toolchain; omit when the caller will run its compiler separately")
parser.add_argument("--deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
ARGS = parser.parse_args()
def resolve_repo_path(path: Path) -> Path:
    return path.resolve() if path.is_absolute() else (ROOT / path).resolve()


for key in ("exe", "original_root", "gcc", "deps", "analysis_deps"):
    setattr(ARGS, key, getattr(ARGS, key).resolve())
ARGS.package = resolve_repo_path(ARGS.package)
ARGS.receipt = resolve_repo_path(ARGS.receipt)
ARGS.build_dir = resolve_repo_path(ARGS.build_dir)
ARGS.fixture_helper = resolve_repo_path(ARGS.fixture_helper)
ARGS.trace_helper = resolve_repo_path(
    ARGS.trace_helper or (ARGS.package / "tests" / "capture_original_route_states.py"))
if ARGS.vc5_toolchain is not None:
    ARGS.vc5_toolchain = resolve_repo_path(ARGS.vc5_toolchain)
PACKAGE = ARGS.package
MODEL_C = PACKAGE / "hercules_route_uv_models.c"
MODEL_H = PACKAGE / "hercules_route_uv_models.h"
TEST_C = PACKAGE / "fixtures" / "route_uv" / "replay_native_trace.c"
UNIT_TEST_C = PACKAGE / "tests" / "test_route_uv.c"
TRACE_RUNNER = ARGS.trace_helper
if ARGS.original_root != ROOT.resolve():
    parser.error("--original-root must name the discovered repository root")
for key in ("receipt", "build_dir"):
    try:
        getattr(ARGS, key).relative_to((ROOT / "work").resolve())
    except ValueError as error:
        raise SystemExit(f"--{key.replace('_', '-')} must stay below repository work/") from error
for path in (PACKAGE, MODEL_C, MODEL_H, TEST_C, UNIT_TEST_C,
             ARGS.fixture_helper, TRACE_RUNNER):
    try:
        path.resolve().relative_to(ROOT.resolve())
    except ValueError:
        parser.error(f"input path must stay inside the repository: {path}")
if not PACKAGE.is_dir() or not ARGS.exe.is_file() or any(not path.is_file() for path in
                               (MODEL_C, MODEL_H, TEST_C, UNIT_TEST_C,
                                ARGS.fixture_helper, TRACE_RUNNER, ARGS.exe)):
    parser.error("package sources, canonical fixture, trace helper, or original executable are missing")
if not ARGS.gcc.is_file() or not ARGS.deps.is_dir() or not ARGS.analysis_deps.is_dir():
    parser.error("strict GCC or pinned oracle dependency directory is missing")
if ARGS.build_dir.exists() and any(ARGS.build_dir.iterdir()):
    parser.error("--build-dir must be new or empty")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_process(command, *, cwd: Path, input_text: str | None = None,
                env: dict | None = None, timeout: int = 180):
    return subprocess.run(command, cwd=str(cwd), input=input_text, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          env=env, timeout=timeout, check=False)


def event_map(run: dict) -> dict[int, list[dict]]:
    result: dict[int, list[dict]] = {}
    for event in run["events"]:
        index = event["iteration"]
        if index is not None:
            result.setdefault(index, []).append(event)
    return result


def one_event(events: list[dict], kind: str) -> dict | None:
    matches = [event for event in events if event["kind"] == kind]
    if len(matches) > 1:
        raise AssertionError(f"native trace has multiple {kind} events in one pixel")
    return matches[0] if matches else None


def expected_inputs(trace: dict):
    rows = []
    transitions = []
    carry_totals = {name: {"eax": 0, "ebx": 0, "pointer_in": 0,
                          "pointer_out": 0} for name in ROUTE_ID}
    for run in trace["runs"]:
        route = run["route"]
        if route not in ROUTE_ID:
            raise AssertionError(f"unrecognized route {route}")
        events_for_pixel = event_map(run)
        states = run["loop_states"]
        post_uv_states = run["post_uv_states"]
        if not states or len(states) != len(post_uv_states):
            raise AssertionError(f"{route}/{run['label']}: full loop/post-UV transition counts differ or are empty")
        initial_count = states[0]["eax"] & 0xffff
        if not 1 <= initial_count <= 320 or len(states) != initial_count:
            raise AssertionError(f"{route}/{run['label']}: loop count is outside its captured 1..320 boundary")
        if run["exit_state"] is None:
            raise AssertionError(f"{route}/{run['label']}: missing native loop exit state")
        for index, raw in enumerate(states):
            pixel_events = events_for_pixel.get(index, [])
            source = one_event(pixel_events, "source")
            if source is None:
                raise AssertionError(f"{route}/{run['label']}/{index}: source event missing")
            palette = one_event(pixel_events, "palette")
            destination = one_event(pixel_events, "destination")
            shade = one_event(pixel_events, "shade")
            blend = one_event(pixel_events, "blend")
            output = one_event(pixel_events, "output")
            source_byte = source["value"]
            expected_next = states[index + 1] if index + 1 < len(states) else run["exit_state"]
            raw_count = raw["eax"] & 0xffff
            if raw_count != initial_count - index:
                raise AssertionError(f"{route}/{run['label']}/{index}: AX is outside the captured countdown")
            if ((expected_next["eax"] >> 16) != (post_uv_states[index]["eax"] >> 16) or
                    (expected_next["eax"] & 0xffff) != raw_count - 1):
                raise AssertionError(f"{route}/{run['label']}/{index}: native AX/EAX partial update is inconsistent")
            globals_read = (run["step_globals"][index] if "step_globals" in run
                            else run["globals"])
            config0 = globals_read["step32"]
            config1 = globals_read["step8"] if route == "cc50_palette" else globals_read["pointer_correction"]
            config2 = globals_read["shade_base"] if route == "e680_shaded_7f" else 0
            destination_byte = destination["value"] if destination is not None else 0
            shade_byte = shade["value"] if shade is not None else 0
            output_byte = output["value"] if output is not None else 0
            values = [ROUTE_ID[route]] + [raw[name] for name in REG_NAMES] + [
                config0, config1, config2, 0,
                source_byte, destination_byte, shade_byte, output_byte,
            ]
            if len(values) != 18:
                raise AssertionError("C row must contain 18 fixed-width values")
            rows.append(" ".join(f"{value:x}" for value in values))
            expected_source_address = int(source["address"], 16)
            if expected_source_address > 0xffffffff:
                raise AssertionError("native source address exceeds raw 32-bit address space")
            expected_lookup = {
                "palette": int(palette["address"], 16) if palette is not None else 0,
                "shade": int(shade["address"], 16) if shade is not None else 0,
                "blend": int(blend["address"], 16) if blend is not None else 0,
            }
            transitions.append({
                "route": route, "label": run["label"], "iteration": index,
                "raw": raw, "expected_post_uv": post_uv_states[index],
                "expected_next": expected_next,
                "next_pc": BASE + (LOOP_START[route] if raw_count > 1 else EXIT_RVA[route]),
                "source_address": expected_source_address,
                "destination_address": raw["edi"],
                "lookup_addresses": expected_lookup,
                "store_required": 1 if output is not None else 0,
                "loop_continues": 1 if raw_count > 1 else 0,
            })
    return "\n".join(rows) + "\n", transitions, carry_totals


def compare_output(stdout: str, transitions: list[dict]) -> dict:
    lines = stdout.splitlines()
    if len(lines) != len(transitions):
        raise AssertionError(f"C model emitted {len(lines)} rows for {len(transitions)} native transitions")
    post_uv_matches = 0
    next_state_matches = 0
    source_matches = 0
    addresses_checked = 0
    stores_checked = 0
    carry_totals = {name: {"eax": 0, "ebx": 0, "pointer_in": 0,
                          "pointer_out": 0} for name in ROUTE_ID}
    route_carry_samples = {name: {"eax": set(), "ebx": set(),
                                  "pointer_in": set(), "pointer_out": set()}
                           for name in ROUTE_ID}
    for line, expected in zip(lines, transitions):
        fields = line.split(",")
        if len(fields) != 31:
            raise AssertionError(f"model output has {len(fields)} columns, expected 31")
        numeric = [int(value, 16) for value in fields[:25]]
        metadata = [int(value, 10) for value in fields[25:]]
        if numeric[0] != 0:
            raise AssertionError(f"C model rejected captured count in {expected['route']}/{expected['label']}/{expected['iteration']}")
        next_regs = dict(zip(REG_NAMES, numeric[1:10]))
        next_pc = numeric[10]
        post_regs = dict(zip(REG_NAMES, numeric[11:20]))
        source_address, palette_address, shade_address, blend_address, destination_address = numeric[20:25]
        (eax_carry, ebx_carry, pointer_carry_in, pointer_carry_out,
         store_required, loop_continues) = metadata
        if expected["route"] != "cc50_palette" and pointer_carry_in != ebx_carry:
            raise AssertionError("ESI ADC carry-in differs from the preceding EBX ADC carry-out")
        for name in REG_NAMES:
            if post_regs[name] != expected["expected_post_uv"][name]:
                raise AssertionError(f"post-UV mismatch {expected['route']}/{expected['label']}/{expected['iteration']} {name}: C={post_regs[name]:08x} native={expected['expected_post_uv'][name]:08x}")
        post_uv_matches += 1
        for name in REG_NAMES:
            if next_regs[name] != expected["expected_next"][name]:
                raise AssertionError(f"next-state mismatch {expected['route']}/{expected['label']}/{expected['iteration']} {name}: C={next_regs[name]:08x} native={expected['expected_next'][name]:08x}")
        if next_pc != expected["next_pc"]:
            raise AssertionError(f"next PC mismatch {expected['route']}/{expected['label']}/{expected['iteration']}: C={next_pc:08x} native={expected['next_pc']:08x}")
        next_state_matches += 1
        if source_address != expected["source_address"]:
            raise AssertionError(f"source-address mismatch {expected['route']}/{expected['label']}/{expected['iteration']}: C={source_address:08x} native={expected['source_address']:08x}")
        source_matches += 1
        if destination_address != expected["destination_address"]:
            raise AssertionError("destination address mismatch")
        route = expected["route"]
        for key, actual in (("palette", palette_address), ("shade", shade_address), ("blend", blend_address)):
            if actual != expected["lookup_addresses"][key]:
                raise AssertionError(f"{key} address mismatch {route}/{expected['label']}/{expected['iteration']}: C={actual:08x} native={expected['lookup_addresses'][key]:08x}")
            if actual != 0:
                addresses_checked += 1
        if store_required != expected["store_required"] or loop_continues != expected["loop_continues"]:
            raise AssertionError(f"store/count branch mismatch {route}/{expected['label']}/{expected['iteration']}")
        stores_checked += 1
        for name, value in (("eax", eax_carry), ("ebx", ebx_carry),
                            ("pointer_in", pointer_carry_in),
                            ("pointer_out", pointer_carry_out)):
            if value not in (0, 1):
                raise AssertionError("carry output is not a single bit")
            carry_totals[route][name] += value
            route_carry_samples[route][name].add(value)
    return {"transitions": len(lines), "post_uv_full_register_matches": post_uv_matches,
            "next_full_register_matches": next_state_matches,
            "source_address_matches": source_matches,
            "lookup_addresses_checked": addresses_checked,
            "store_and_loop_decisions_checked": stores_checked,
            "carry_one_counts": carry_totals,
            "carry_values_observed": {route: {lane: sorted(values)
                                               for lane, values in lanes.items()}
                                       for route, lanes in route_carry_samples.items()}}


def compile_gcc(build_dir: Path) -> dict:
    binary = build_dir / "route_uv_gcc.exe"
    command = [str(ARGS.gcc), "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
               "-I", str(ROOT / "renderer_contract"), "-I", str(PACKAGE),
               str(MODEL_C), str(TEST_C), "-o", str(binary)]
    version = run_process([str(ARGS.gcc), "--version"], cwd=build_dir)
    result = run_process(command, cwd=build_dir)
    if result.returncode != 0:
        raise RuntimeError("strict GCC build failed:\n" + result.stdout)
    return {"compiler": str(ARGS.gcc), "version": version.stdout.splitlines()[0],
            "sha256": sha(ARGS.gcc), "flags": command[1:9],
            "binary": str(binary), "binary_sha256": sha(binary), "output": result.stdout}


def compile_vc5(build_dir: Path) -> dict:
    vc5 = ARGS.vc5_toolchain
    compiler = vc5 / "bin" / "cl.exe"
    binary = build_dir / "route_uv_vc5.exe"
    temp_dir = build_dir / "tmp"
    temp_dir.mkdir(exist_ok=True)
    env = os.environ.copy()
    for name in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(name, None)
    env["PATH"] = str(vc5 / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(vc5 / "include")
    env["LIB"] = str(vc5 / "lib")
    env["TEMP"] = str(temp_dir)
    env["TMP"] = str(temp_dir)
    command = [str(compiler), "/nologo", "/W3", "/O2", "/MT",
               "/I" + str(ROOT / "renderer_contract"), "/I" + str(PACKAGE),
               "/Fo" + str(build_dir) + os.sep, "/Fe" + str(binary),
               str(MODEL_C), str(TEST_C)]
    try:
        result = run_process(command, cwd=build_dir, env=env)
    except OSError as error:
        return {"status": "BLOCKED_BEFORE_PROCESS_START", "compiler": str(compiler),
                "compiler_sha256": sha(compiler) if compiler.is_file() else None,
                "reason": str(error)}
    if result.returncode != 0 or not binary.is_file():
        return {"status": "BUILD_FAILED", "compiler": str(compiler),
                "compiler_sha256": sha(compiler) if compiler.is_file() else None,
                "exit_code": result.returncode, "output": result.stdout,
                "flags": command[1:-2], "environment": {"INCLUDE": env["INCLUDE"],
                                                           "LIB": env["LIB"],
                                                           "TEMP": env["TEMP"],
                                                           "TMP": env["TMP"]}}
    return {"status": "BUILT", "compiler": str(compiler),
            "compiler_sha256": sha(compiler), "version": "VC5 5.00.7022",
            "binary": str(binary), "binary_sha256": sha(binary),
            "flags": command[1:-2], "environment": {"INCLUDE": env["INCLUDE"],
                                                       "LIB": env["LIB"],
                                                       "TEMP": env["TEMP"],
                                                       "TMP": env["TMP"]},
            "output": result.stdout}


def main() -> int:
    ARGS.build_dir.mkdir(parents=True, exist_ok=True)
    gcc_dir = ARGS.build_dir / "gcc"
    vc5_dir = ARGS.build_dir / "vc5"
    gcc_dir.mkdir(exist_ok=True)
    vc5_dir.mkdir(exist_ok=True)
    trace_path = ARGS.build_dir / "native_trace.json"
    capture_command = [sys.executable, "-B", str(TRACE_RUNNER),
                       "--package", str(PACKAGE),
                       "--fixture-helper", str(ARGS.fixture_helper),
                       "--exe", str(ARGS.exe), "--original-root", str(ROOT),
                       "--receipt", str(trace_path), "--deps", str(ARGS.deps),
                       "--analysis-deps", str(ARGS.analysis_deps)]
    capture = run_process(capture_command, cwd=ROOT, timeout=300)
    if capture.returncode != 0:
        raise RuntimeError("native trace capture failed:\n" + capture.stdout)
    trace = json.loads(trace_path.read_text(encoding="utf-8"))
    if trace["oracle"]["sha256"] != EXE_SHA256 or trace["oracle"]["image_base"] != hex(BASE):
        raise RuntimeError("native trace receipt has the wrong original input identity")
    c_input, transitions, _ = expected_inputs(trace)
    gcc = compile_gcc(gcc_dir)
    gcc_result = run_process([gcc["binary"]], cwd=gcc_dir, input_text=c_input, timeout=120)
    if gcc_result.returncode != 0:
        raise RuntimeError("strict GCC model run failed:\n" + gcc_result.stdout)
    gcc_comparison = compare_output(gcc_result.stdout, transitions)

    unit_binary = gcc_dir / "test_route_uv.exe"
    unit_command = [str(ARGS.gcc), "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT / "renderer_contract"), "-I", str(PACKAGE),
                    str(MODEL_C), str(UNIT_TEST_C), "-o", str(unit_binary)]
    unit_build = run_process(unit_command, cwd=gcc_dir)
    if unit_build.returncode != 0:
        raise RuntimeError("strict GCC standalone route unit build failed:\n" + unit_build.stdout)
    unit_run = run_process([str(unit_binary)], cwd=gcc_dir)
    if unit_run.returncode != 0 or unit_run.stdout.strip() != "route UV standalone checks passed":
        raise RuntimeError("standalone route UV unit checks failed:\n" + unit_run.stdout)

    vc5 = (compile_vc5(vc5_dir) if ARGS.vc5_toolchain is not None else
           {"status": "NOT_REQUESTED", "reason": "VC5 compilation is left to the integrator's authorized host invocation."})
    vc5_comparison = None
    if vc5["status"] == "BUILT":
        vc5_result = run_process([vc5["binary"]], cwd=vc5_dir, input_text=c_input, timeout=120)
        if vc5_result.returncode != 0:
            vc5["run_status"] = "RUN_FAILED"
            vc5["run_output"] = vc5_result.stdout
        else:
            try:
                vc5_comparison = compare_output(vc5_result.stdout, transitions)
                vc5["run_status"] = "PASS"
            except AssertionError as error:
                vc5["run_status"] = "COMPARISON_FAILED"
                vc5["comparison_error"] = str(error)
    vc5_requested = ARGS.vc5_toolchain is not None
    vc5_passed = (not vc5_requested or
                  (vc5["status"] == "BUILT" and vc5.get("run_status") == "PASS" and
                   vc5_comparison is not None and
                   vc5_comparison.get("transitions") == len(transitions)))
    status = ("BOUNDED_EQUIVALENT" if vc5_requested and vc5_passed else
              "GCC_EQUIVALENT_VC5_NOT_REQUESTED" if not vc5_requested else
              "GCC_EQUIVALENT_VC5_FAILED")
    receipt = {
        "schema_version": 1,
        "status": status,
        "prediction": PREDICTION,
        "scope": "Three route-specific C89 register transitions compared against the full captured original CPU states in the established 17 native cases. Inputs are raw loop-entry registers, observed source/LUT bytes, and original global step values. No geometry or setup reconstruction is performed.",
        "oracle": {"path": str(ARGS.exe), "sha256": trace["oracle"]["sha256"],
                   "image_base": trace["oracle"]["image_base"]},
        "runtime": trace["runtime"],
        "original_spans": trace["original_spans"],
        "native_trace": {"path": str(trace_path), "sha256": sha(trace_path),
                         "runner": trace["runner"]},
        "sources": {"header": {"path": str(MODEL_H), "sha256": sha(MODEL_H)},
                    "implementation": {"path": str(MODEL_C), "sha256": sha(MODEL_C)},
                    "native_trace_driver": {"path": str(TEST_C), "sha256": sha(TEST_C)},
                    "standalone_test": {"path": str(UNIT_TEST_C), "sha256": sha(UNIT_TEST_C)},
                    "trace_runner": {"path": str(TRACE_RUNNER), "sha256": sha(TRACE_RUNNER)},
                    "fixture_helper": {"path": str(ARGS.fixture_helper), "sha256": sha(ARGS.fixture_helper)},
                    "shared_types": {"path": str(ROOT / "renderer_contract" / "hercules_types.h"),
                                     "sha256": sha(ROOT / "renderer_contract" / "hercules_types.h")}},
        "fixture": {"native_case_count": len(trace["runs"]),
                    "transition_count": len(transitions),
                    "cases_by_route": {route: sum(1 for run in trace["runs"] if run["route"] == route)
                                        for route in ROUTE_ID},
                    "count_range": [min(row["raw"]["eax"] & 0xffff for row in transitions),
                                    max(row["raw"]["eax"] & 0xffff for row in transitions)],
                    "zero_source_cases": [f"{run['route']}/{run['label']}" for run in trace["runs"]
                                          if run["all_zero"]]},
        "gcc": {"build": gcc, "comparison": gcc_comparison,
                "standalone_test": {"command": unit_command, "build_output": unit_build.stdout,
                                    "binary": str(unit_binary), "binary_sha256": sha(unit_binary),
                                    "run_output": unit_run.stdout}},
        "vc5": {"build": vc5, "comparison": vc5_comparison},
        "limits": ["Raw loop-entry state only; no geometry, setup, clipping, or packet reconstruction.",
                   "Only the captured count range and existing route/mode/selector cases are modeled.",
                   "Source/LUT bytes are supplied as route-specific observations; the C functions do not execute memory callbacks or emulate lookup tables.",
                   "No general affine iterator, GPU, full raster, host integration, gameplay, or whole-renderer equivalence is claimed."],
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ROOT),
                       "package": str(PACKAGE), "fixture_helper": str(ARGS.fixture_helper),
                       "trace_helper": str(TRACE_RUNNER),
                       "receipt": str(ARGS.receipt), "build_dir": str(ARGS.build_dir),
                       "python": sys.executable, "python_version": sys.version.split()[0],
                       "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps)},
        "elapsed_seconds": round(time.time() - STARTED, 3),
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "gcc": gcc_comparison,
                      "standalone_test": unit_run.stdout.strip(),
                      "vc5_status": vc5["status"], "vc5_comparison": vc5_comparison,
                      "receipt": str(ARGS.receipt)}, indent=2))
    return 0 if (gcc_comparison["transitions"] == len(transitions) and
                 unit_run.returncode == 0 and vc5_passed) else 1


STARTED = time.time()


if __name__ == "__main__":
    raise SystemExit(main())
