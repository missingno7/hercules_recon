"""Rerun the E680 pointer-correction geometry and raw-boundary discriminators."""
import argparse
import importlib.util
import json
import os
import subprocess
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
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--package", type=Path, default=Path("renderer_contract"),
                    help="copied model package containing route-model C sources")
parser.add_argument("--fixture-helper", type=Path,
                    default=ROOT / "renderer_contract" / "tests" / "test_original_skewed_quads.py",
                    help="public original-byte skewed-quad fixture helper")
parser.add_argument("--exe", required=True, type=Path)
parser.add_argument("--original-root", required=True, type=Path)
parser.add_argument("--receipt", required=True, type=Path)
parser.add_argument("--build-dir", required=True, type=Path,
                    help="new or empty compiler-output directory below repository work/")
parser.add_argument("--gcc", type=Path, default=Path(r"C:\msys64\mingw64\bin\gcc.exe"))
parser.add_argument("--deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
CLI = parser.parse_args()


def resolve_repo_path(path: Path) -> Path:
    return path.resolve() if path.is_absolute() else (ROOT / path).resolve()


for key in ("fixture_helper", "exe", "original_root", "receipt", "build_dir", "gcc", "deps", "analysis_deps"):
    setattr(CLI, key, resolve_repo_path(getattr(CLI, key)))
CLI.package = resolve_repo_path(CLI.package)
if CLI.original_root != ROOT.resolve():
    parser.error("--original-root must name the repository root discovered from this test")
if not CLI.exe.is_file() or not CLI.fixture_helper.is_file() or not CLI.gcc.is_file():
    parser.error("pinned executable, fixture helper, or strict GCC is missing")
if not CLI.deps.is_dir() or not CLI.analysis_deps.is_dir():
    parser.error("pinned Unicorn or analysis dependency directory is missing")
for output in (CLI.receipt, CLI.build_dir):
    try:
        output.relative_to((ROOT / "work").resolve())
    except ValueError as error:
        parser.error("--receipt and --build-dir must stay below repository work/")
    protected = (CLI.exe, CLI.exe.parent, ROOT / "work" / "discs")
    for source in protected:
        source = source.resolve()
        if output == source or output in source.parents or source in output.parents:
            parser.error(f"output path overlaps an original input: {output}")
try:
    CLI.fixture_helper.relative_to(ROOT.resolve())
    CLI.package.relative_to(ROOT.resolve())
except ValueError:
    parser.error("package and fixture helper must remain inside the repository")
if not CLI.package.is_dir() or any(not (CLI.package / name).is_file() for name in
                                   ("hercules_route_uv_models.c", "hercules_route_uv_models.h")):
    parser.error("route model package sources are missing")
if CLI.build_dir.exists() and any(CLI.build_dir.iterdir()):
    parser.error("--build-dir must be new or empty")

TEST = CLI.fixture_helper
saved_argv = sys.argv[:]
sys.argv = [str(TEST), "--exe", str(CLI.exe), "--receipt", str(CLI.receipt),
            "--original-root", str(ROOT), "--deps", str(CLI.deps),
            "--analysis-deps", str(CLI.analysis_deps)]
spec = importlib.util.spec_from_file_location("public_skewed_quad_runner", TEST)
if spec is None or spec.loader is None:
    raise RuntimeError("public skewed-quad runner is unavailable")
runner = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = runner
spec.loader.exec_module(runner)
sys.argv = saved_argv
PACKAGE = CLI.package

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                               UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
                               UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EFLAGS,
                               UC_X86_REG_EIP)

args = runner.ARGS
runtime = runner.quad.verify_runtime(runner.unicorn, args.deps)
pe = runner.PE(args.exe)
if runner.sha(pe.data) != runner.EXPECTED_EXE_SHA256 or pe.image_base != runner.BASE:
    raise RuntimeError("pinned original EXE identity mismatch")
data_section = next(section for section in pe.sections if section["name"] == ".data")
data_region = (runner.BASE + data_section["virtual_address"],
               max(data_section["virtual_size"], data_section["raw_size"]))

captured = []
post_uv = []
next_states = []
original_native_case = runner.geom.NativeCase
registers = {"eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX,
             "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX,
             "esi": UC_X86_REG_ESI, "edi": UC_X86_REG_EDI,
             "ebp": UC_X86_REG_EBP, "esp": UC_X86_REG_ESP,
             "eflags": UC_X86_REG_EFLAGS}


def native_case_with_loop_hook(*positional, **keywords):
    native = original_native_case(*positional, **keywords)

    def on_loop(uc, address, _size, _user):
        if address == runner.BASE + 0xEE6D:
            row = {name: uc.reg_read(register) & 0xffffffff
                   for name, register in registers.items()}
            row["pointer_correction"] = int.from_bytes(uc.mem_read(0xA9099C, 4), "little")
            row["step32"] = int.from_bytes(uc.mem_read(0xA90998, 4), "little")
            captured.append(row)
        elif address == runner.BASE + 0xEE82:
            row = {name: uc.reg_read(register) & 0xffffffff
                   for name, register in registers.items()}
            row["pointer_correction"] = int.from_bytes(uc.mem_read(0xA9099C, 4), "little")
            row["step32"] = int.from_bytes(uc.mem_read(0xA90998, 4), "little")
            post_uv.append(row)
        elif address == runner.BASE + 0xEE91:
            next_states.append({name: uc.reg_read(register) & 0xffffffff
                                for name, register in registers.items()})

    native.uc.hook_add(UC_HOOK_CODE, on_loop)
    return native


runner.geom.NativeCase = native_case_with_loop_hook
runner.geom.CODE_WINDOWS = tuple(runner.geom.CODE_WINDOWS) + runner.E680_SPANS
shape = runner.load_shapes()["skewed_top_bottom_clip"]
uv_q16 = runner.set_quad_globals(shape)
route = runner.run_e680_case(pe, data_region,
                             "skewed_top_bottom_clip_pointer_discriminator",
                             shape, uv_q16)
runner.geom.NativeCase = original_native_case
nonzero = [row for row in captured if row["pointer_correction"] != 0]
carry_rows = []
for row in captured:
    eax_sum = row["eax"] + row["step32"]
    eax_carry = int(eax_sum > 0xffffffff)
    ebx_sum = row["ebx"] + row["ecx"] + eax_carry
    ebx_carry = int(ebx_sum > 0xffffffff)
    esi_sum = row["esi"] + row["pointer_correction"] + ebx_carry
    pointer_carry = int(esi_sum > 0xffffffff)
    carry_rows.append({"eax_carry": eax_carry, "ebx_carry": ebx_carry,
                       "pointer_carry_in": ebx_carry,
                       "pointer_carry_out": pointer_carry})

if not (len(captured) == len(post_uv) == len(next_states) == len(route["source_reads"])):
    raise RuntimeError("native loop, post-UV, next-state, and source-read counts differ")
package = PACKAGE
build_dir = CLI.build_dir
build_dir.mkdir(parents=True, exist_ok=True)
compiler = CLI.gcc
binary = build_dir / "replay_native_trace.exe"
compile_command = [str(compiler), "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
                   "-I", str(ROOT / "renderer_contract"), "-I", str(package),
                   str(package / "hercules_route_uv_models.c"),
                   str(package / "fixtures" / "route_uv" / "replay_native_trace.c"),
                   "-o", str(binary)]
build_result = subprocess.run(compile_command, cwd=str(build_dir), text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
if build_result.returncode != 0:
    raise RuntimeError("strict C89 replay build failed: " + build_result.stdout)
blend_iter = iter(route["blend_reads"])
input_rows = []
expected_blends = []
for index, raw in enumerate(captured):
    sample = route["source_reads"][index]
    source = int(sample["value"])
    if source != 0:
        blend = next(blend_iter)
        blend_address = int(blend["address"], 16)
        destination = (blend_address - raw["ebp"] - (source << 8)) & 0xff
        blend_result = int(blend["value"])
    else:
        blend_address = 0
        destination = 0
        blend_result = 0
    expected_blends.append(blend_address)
    values = [1] + [raw[name] for name in
                     ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "eflags")] + [
        raw["step32"], raw["pointer_correction"], 0, 0,
        source, destination, 0, blend_result]
    if len(values) != 18:
        raise RuntimeError("native loop input row does not have 18 values")
    input_rows.append(" ".join(f"{value:x}" for value in values))
if next(blend_iter, None) is not None:
    raise RuntimeError("unmatched original blend read remains after source pairing")
model_result = subprocess.run([str(binary)], cwd=str(build_dir),
                              input="\n".join(input_rows) + "\n", text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
if model_result.returncode != 0:
    raise RuntimeError("native geometry C replay failed: " + model_result.stdout)
model_lines = model_result.stdout.splitlines()
if len(model_lines) != len(captured):
    raise RuntimeError("C replay row count differs from native loop count")
geometry_comparisons = []
for index, (line, raw) in enumerate(zip(model_lines, captured)):
    fields = line.split(",")
    if len(fields) != 31 or int(fields[0], 16) != 0:
        raise RuntimeError(f"C replay output row {index} has an invalid shape/status")
    numbers = [int(value, 16) for value in fields[:25]]
    metadata = [int(value, 10) for value in fields[25:]]
    next_registers = dict(zip(registers, numbers[1:10]))
    post_registers = dict(zip(registers, numbers[11:20]))
    expected_post = post_uv[index]
    expected_next = next_states[index]
    source = int(route["source_reads"][index]["value"])
    source_address = runner.geom.TEXTURE + int(route["source_reads"][index]["offset"])
    expected_pc = runner.BASE + (0xEE6D if (raw["eax"] & 0xffff) > 1 else 0xEE93)
    checks = {
        "post_uv_registers": post_registers == {name: expected_post[name] for name in registers},
        "post_dec_registers": next_registers == {name: expected_next[name] for name in registers},
        "next_pc": numbers[10] == expected_pc,
        "source_address": numbers[20] == source_address,
        "texture_offset": (source_address - runner.geom.TEXTURE) == route["source_reads"][index]["offset"],
        "blend_address": numbers[23] == expected_blends[index],
        "destination_address": numbers[24] == raw["edi"],
        "pointer_carry_input": metadata[2] == carry_rows[index]["pointer_carry_in"],
        "pointer_carry_output": metadata[3] == carry_rows[index]["pointer_carry_out"],
        "store_decision": metadata[4] == int(source != 0),
        "loop_decision": metadata[5] == int((raw["eax"] & 0xffff) > 1),
    }
    geometry_comparisons.append({"index": index, "all_match": all(checks.values()),
                                 "checks": checks})
if any(not row["all_match"] for row in geometry_comparisons):
    raise RuntimeError("native skewed-quad transitions diverged from strict C route model: " +
                       json.dumps([row for row in geometry_comparisons if not row["all_match"]][:4]))

# Geometry produces carry-in=1 but no ESI ADC carry-out. Exercise the exact
# loop instructions once with a preregistered, explicitly raw-only boundary.
fixture_dir = package / "fixtures" / "route_uv"
prediction_path = fixture_dir / "pointer_discriminator_prediction.json"
prediction = json.loads(prediction_path.read_text(encoding="utf-8"))
falsified_prediction_path = fixture_dir / "raw_boundary_prediction_falsified.json"
raw_prediction_path = fixture_dir / "raw_boundary_prediction_carryout.json"
raw_prediction = json.loads(raw_prediction_path.read_text(encoding="utf-8"))
raw_registers = {"eax": 0xffff0001, "ebx": 0xffffffff, "ecx": 0,
                 "edx": 0, "esi": 0x0fff0100,
                 "edi": runner.geom.SURFACE, "ebp": runner.quad.BLEND_BASE,
                 "esp": runner.geom.STACK, "eflags": 0x202}
if (runner.geom.SCRATCH != 0x10000000 or runner.geom.SURFACE != 0x10020000 or
        runner.quad.BLEND_BASE != 0x0092e3d0 or runner.geom.STACK != 0x20000000):
    raise RuntimeError("pinned public scratch addresses differ from preregistered raw input")
raw_spec = {"name": "raw_boundary_only_pointer_carry", "vertices": [(0, 0), (1, 0), (1, 1), (0, 1)],
            "clip": (0, 0, 320, 240), "uv_mode": "constant"}
raw_native = original_native_case(pe, data_region, raw_spec, "d60c_rowzero")
raw_native.code_snapshot[(0xE680, 0xEF60)] = bytes(
    raw_native.uc.mem_read(runner.BASE + 0xE680, 0xEF60 - 0xE680))
runner.geom.put32(raw_native.uc, 0xA90998, 0x00010000)
runner.geom.put32(raw_native.uc, 0xA9099C, 0xf0010000)
raw_source_address = runner.geom.SCRATCH
raw_destination_address = runner.geom.SURFACE
raw_blend_address = runner.quad.BLEND_BASE + (7 << 8) + 0x3c
data_start, data_size = data_region
if not data_start <= raw_blend_address < data_start + data_size:
    raise RuntimeError("raw blend entry is outside the pinned .data section")
raw_native.uc.mem_write(raw_source_address, b"\x07")
raw_native.uc.mem_write(raw_destination_address, b"\x3c")
raw_native.uc.mem_write(raw_blend_address, b"\xa7")
for name, register in registers.items():
    raw_native.uc.reg_write(register, raw_registers[name])
raw_capture = {"post_uv": None, "next": None, "exit_pc": None, "events": []}


def raw_code_hook(uc, address, _size, _user):
    if address == runner.BASE + 0xEE82:
        raw_capture["post_uv"] = {name: uc.reg_read(register) & 0xffffffff
                                  for name, register in registers.items()}
    elif address == runner.BASE + 0xEE91:
        raw_capture["next"] = {name: uc.reg_read(register) & 0xffffffff
                               for name, register in registers.items()}
    elif address == runner.BASE + 0xEE93:
        raw_capture["exit_pc"] = address
        uc.emu_stop()


def raw_read_hook(uc, _access, address, size, _value, _user):
    pc = uc.reg_read(UC_X86_REG_EIP)
    if pc in (runner.BASE + 0xEE79, runner.BASE + 0xEE88):
        raw_capture["events"].append({
            "kind": "source" if pc == runner.BASE + 0xEE79 else "blend",
            "pc": pc, "address": address, "size": size,
            "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little"),
        })


raw_native.uc.hook_add(UC_HOOK_CODE, raw_code_hook)
raw_native.uc.hook_add(UC_HOOK_MEM_READ, raw_read_hook)
raw_native.uc.emu_start(runner.BASE + 0xEE6D, 0, timeout=2_000_000, count=128)
if raw_native.error is not None or raw_capture["exit_pc"] != runner.BASE + 0xEE93:
    raise RuntimeError("raw original loop did not stop at its pinned exit: " +
                       json.dumps({"error": raw_native.error, "capture": raw_capture}))
raw_code_unchanged = bytes(raw_native.uc.mem_read(
    runner.BASE + 0xE680, 0xEF60 - 0xE680)) == raw_native.code_snapshot[(0xE680, 0xEF60)]
raw_source = next(event for event in raw_capture["events"] if event["kind"] == "source")
raw_blend = next(event for event in raw_capture["events"] if event["kind"] == "blend")
raw_values = [1] + [raw_registers[name] for name in
                    ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "eflags")] + [
    0x00010000, 0xf0010000, 0, 0, 7, 0x3c, 0, 0xa7]
raw_line = " ".join(f"{value:x}" for value in raw_values) + "\n"
raw_model_result = subprocess.run([str(binary)], cwd=str(build_dir), input=raw_line,
                                  text=True, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, check=False)
if raw_model_result.returncode != 0:
    raise RuntimeError("raw boundary C replay failed: " + raw_model_result.stdout)
raw_fields = raw_model_result.stdout.strip().split(",")
if len(raw_fields) != 31 or int(raw_fields[0], 16) != 0:
    raise RuntimeError("raw boundary C replay output is malformed")
raw_numbers = [int(value, 16) for value in raw_fields[:25]]
raw_metadata = [int(value, 10) for value in raw_fields[25:]]
raw_model_post = dict(zip(registers, raw_numbers[11:20]))
raw_model_next = dict(zip(registers, raw_numbers[1:10]))
raw_expected_blend = runner.quad.BLEND_BASE + (7 << 8) + 0x3c
raw_model_checks = {
    "post_uv_registers": raw_model_post == raw_capture["post_uv"],
    "post_dec_registers": raw_model_next == raw_capture["next"],
    "next_pc": raw_numbers[10] == runner.BASE + 0xEE93,
    "source_address": raw_numbers[20] == raw_source_address == raw_source["address"],
    "blend_address": raw_numbers[23] == raw_expected_blend == raw_blend["address"],
    "destination_address": raw_numbers[24] == raw_destination_address,
    "carry_chain": raw_metadata[:4] == [1, 1, 1, 1],
    "store_and_exit": raw_metadata[4:] == [1, 0],
    "native_surface_store": len(raw_native.surface_writes) == 1 and
                            raw_native.surface_writes[0]["offset"] == 0 and
                            raw_native.surface_writes[0]["value"] == 0xa7,
    "native_surface_read": len(raw_native.surface_reads) == 1 and
                           raw_native.surface_reads[0]["offset"] == 0 and
                           raw_native.surface_reads[0]["value"] == 0x3c,
    "original_code_unchanged": raw_code_unchanged,
}
if not all(raw_model_checks.values()):
    raise RuntimeError("raw boundary carry discriminator failed: " +
                       json.dumps({"checks": raw_model_checks, "metadata": raw_metadata,
                                   "post_uv": raw_capture["post_uv"], "next": raw_capture["next"]}))

receipt = {
    "schema_version": 1,
    "status": "GEOMETRY_AND_RAW_BOUNDARY_EQUIVALENT" if route.get("original_code_unchanged") and captured and all(row["all_match"] for row in geometry_comparisons) and all(raw_model_checks.values()) else "INCOMPLETE",
    "prediction": prediction,
    "prediction_source_sha256": runner.sha(prediction_path.read_bytes()),
    "classification": "GEOMETRY_REACHABLE",
    "oracle": {"path": str(CLI.exe), "sha256": runner.sha(pe.data), "image_base": hex(pe.image_base)},
    "runtime": runtime,
    "original_spans": [
        runner.span_record(pe, "E680 generic body", 0xE680, 0xEF60),
        runner.span_record(pe, "E680 mode-0x80 loop", 0xEE6D, 0xEE93),
    ],
    "tooling": {
        "python_executable": sys.executable,
        "python_version": sys.version.split()[0],
        "runner_path": str(Path(__file__).resolve()),
        "runner_sha256": runner.sha(Path(__file__).read_bytes()),
        "fixture_helper_path": str(TEST),
        "fixture_helper_sha256": runner.sha(TEST.read_bytes()),
        "gcc_path": str(compiler),
        "gcc_sha256": runner.sha(compiler.read_bytes()),
        "gcc_version": subprocess.run([str(compiler), "--version"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False).stdout.splitlines()[0],
        "model_header_sha256": runner.sha((package / "hercules_route_uv_models.h").read_bytes()),
        "model_source_sha256": runner.sha((package / "hercules_route_uv_models.c").read_bytes()),
        "trace_driver_sha256": runner.sha((package / "fixtures" / "route_uv" / "replay_native_trace.c").read_bytes()),
    },
    "fixture": {
        "runner": str(TEST), "runner_sha256": runner.sha(TEST.read_bytes()),
        "case": "skewed_top_bottom_clip",
        "vertices_xy": [list(point) for point in shape["vertices_xy"]],
        "uv_bytes": [list(point) for point in shape["uv_bytes"]],
        "clip_ltrb": list(shape["clip"]),
        "convex_turn_cross_products": shape["turns"],
    },
    "route_result": route,
    "loop_entries": captured,
    "post_uv_states": post_uv,
    "post_dec_states": next_states,
    "carry_rows": carry_rows,
    "strict_c89_differential": {
        "compiler": str(compiler), "compiler_sha256": runner.sha(compiler.read_bytes()),
        "flags": compile_command[1:6], "binary_sha256": runner.sha(binary.read_bytes()),
        "record_count": len(input_rows), "comparison_count": len(geometry_comparisons),
        "all_match": all(row["all_match"] for row in geometry_comparisons),
        "carry_values": {name: sorted({row[name] for row in carry_rows})
                         for name in ("eax_carry", "ebx_carry", "pointer_carry_in", "pointer_carry_out")},
        "build_output": build_result.stdout,
        "row_comparisons": geometry_comparisons,
    },
    "raw_boundary_only": {
        "classification": "RAW_BOUNDARY_ONLY",
        "falsified_prediction": json.loads(falsified_prediction_path.read_text(encoding="utf-8")),
        "falsified_prediction_path": str(falsified_prediction_path),
        "falsified_prediction_sha256": runner.sha(falsified_prediction_path.read_bytes()),
        "prediction_path": str(raw_prediction_path),
        "prediction_sha256": runner.sha(raw_prediction_path.read_bytes()),
        "falsified_prior_prediction": {
            "path": str(falsified_prediction_path),
            "sha256": runner.sha(falsified_prediction_path.read_bytes()),
            "result": "With A9099C=0xF0000000, native ESI was 0xFFFF0101 and pointer carry-out was 0; the preregistered carry-out prediction was false.",
        },
        "entry_rva": "0xEE6D", "exit_rva": "0xEE93",
        "instruction_count_budget": 128,
        "initial_registers": {name: hex(value) for name, value in raw_registers.items()},
        "globals": {"A90998": "0x00010000", "A9099C": "0xF0010000"},
        "synthetic_memory": {"source_address": hex(raw_source_address),
                             "source_byte": raw_source["value"],
                             "destination_address": hex(raw_destination_address),
                             "destination_byte": raw_native.surface_reads[0]["value"],
                             "blend_address": hex(raw_expected_blend),
                             "blend_byte": raw_blend["value"],
                             "output_byte": raw_native.surface_writes[0]["value"]},
        "post_uv_registers": raw_capture["post_uv"],
        "post_dec_registers": raw_capture["next"],
        "memory_events": raw_capture["events"],
        "strict_c89_model_checks": raw_model_checks,
        "original_code_unchanged": raw_code_unchanged,
        "scope": "One controlled raw state only; this is not a geometry-reachable correction magnitude or packet input.",
    },
    "nonzero_pointer_correction_entries": nonzero,
    "summary": {
        "loop_entry_count": len(captured),
        "nonzero_pointer_correction_count": len(nonzero),
        "pointer_correction_values": sorted({row["pointer_correction"] for row in captured}),
        "carry_values": {name: sorted({row[name] for row in carry_rows})
                         for name in ("eax_carry", "ebx_carry", "pointer_carry_in", "pointer_carry_out")},
        "post_uv_count": len(post_uv),
        "post_dec_count": len(next_states),
        "native_code_unchanged": route.get("original_code_unchanged"),
        "generic_fallback_seen": route.get("e680_generic_fallback_seen"),
    },
    "scope": "Geometry-reachable evidence is one public short convex wire-valid skewed FT4 fixture through E680 mode 0x80 selector 2. The separate RAW_BOUNDARY_ONLY case is one controlled raw loop state, count=1, used solely to exercise pointer carry-out; its correction magnitude is not claimed geometry-reachable. No general pointer-correction or geometry claim follows.",
}
CLI.receipt.parent.mkdir(parents=True, exist_ok=True)
CLI.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"status": receipt["status"], "loop_entry_count": len(captured),
                  "nonzero_pointer_correction_count": len(nonzero),
                  "pointer_correction_values": receipt["summary"]["pointer_correction_values"],
                  "carry_values": receipt["strict_c89_differential"]["carry_values"],
                  "raw_boundary_carries": raw_metadata[:4],
                  "model_all_match": receipt["strict_c89_differential"]["all_match"],
                  "receipt": str(CLI.receipt)}, indent=2))
