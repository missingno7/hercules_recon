"""Fresh bounded native trace plus VC5 source replay for ENG1 2E3B0."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

LANE = Path(__file__).resolve().parent
sys.path.insert(0, str(LANE))
sys.dont_write_bytecode = True
from pathguard import discover_repo_root, input_file, owned_output, repo_path

TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
TOOL_HASHES = {
    "cl.exe": "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb",
    "link.exe": "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1",
}
ORIGINAL_HASHES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "HERCULES.FS": "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4",
    "MAP_PSX.BIN": "78bc5a60ecce1c99e752fb30a16bc5ace4ee80f686365e1da8136a0decbd69cb",
    "host_transport": "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8",
    "trig_table": "e62146d85a8a12fad1c8982008754fa515be2e521a982f65d991ed19e64a2737",
}
MAP_OFFSET = 18845296
MAP_LENGTH = 1304576
HOST_TABLE_RVA = 0x361B8
HOST_TABLE_SIZE = 1000
TRIG_TABLE_RVA = 0x37480
TRIG_TABLE_SIZE = 0x2000


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def c_ints(values) -> str:
    return "{" + ",".join(str(int(x)) for x in values) + "}"


def c_bytes(raw: bytes) -> str:
    return "{" + ",".join("0x%02X" % x for x in raw) + "}"


def c_u32(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08X}UL"


def build_native_mesh_cases(report: dict) -> list[dict]:
    main = report["selected_real_resource_case"]
    cases = [{"mesh_call_index": main["mesh_call_index"], "mesh": main["mesh"],
              "point_outputs": main["point_outputs"], "packets": main["packets"]}]
    cases.extend(report["auxiliary_real_resource_cases"])
    winding = {row["mesh_call_index"]: int(row["value"], 16)
               for row in report["selected_and_auxiliary_winding_writes"]}
    prep = {row["mesh_call_index"]: row
            for row in report["selected_and_auxiliary_2e3b0_results"]}
    center = report["camera_context"]["center"]
    focal = report["camera_context"]["focal"]
    records = []
    for case in cases:
        mesh = case["mesh"]
        call = case["mesh_call_index"]
        prep_row = prep.get(call)
        if prep_row is None:
            raise RuntimeError(f"call {call} lacks original 2E3B0 entry/return data")
        points_by_index = {row["source_point_index"]: row for row in case["point_outputs"]}
        source_by_index = {row["index"]: row for row in mesh["source_points"]}
        if (set(points_by_index) != set(range(mesh["point_count"])) or
                set(source_by_index) != set(range(mesh["point_count"]))):
            raise RuntimeError(f"call {call} lacks the complete native point identity chain")
        point_rows = []
        for index in range(mesh["point_count"]):
            native = points_by_index[index]
            source = source_by_index[index]
            if (int(native["source_address"], 16) != int(source["address"], 16) or
                    int(native["source_packed_dword"], 16) != int(source["packed_dword"], 16)):
                raise RuntimeError(f"call {call} point {index} source read/output identity changed")
            xyz = source["source_xyz"]
            expected_input = [xyz[0] if winding[call] else 64 - xyz[0], xyz[1], xyz[2]]
            if native["host_input_signed_words"] != expected_input:
                raise RuntimeError(f"call {call} point {index} packed XYZ derivation changed")
            point_rows.append({
                "source_offset": source["file_offset"], "source": xyz,
                "input": native["host_input_signed_words"],
                "camera": native["camera_xyz_signed"],
                "xy": native["scratch_xy_after_return"],
                "config": {
                    "matrix": native["camera_transform"]["matrix_words_row_major"],
                    "translation": native["camera_transform"]["translation_dwords"],
                    "center": center, "focal": focal,
                },
            })
        packets = case["packets"]
        if len(packets) > 8 or mesh["point_count"] > 16:
            raise RuntimeError("real-resource case exceeds the bounded source fixture arrays")
        face_indices = [int(packet["face_record_index"]) for packet in packets]
        if len(face_indices) != len(set(face_indices)):
            raise RuntimeError("bounded fixture requires unique emitted face-record indices")
        records.append({
            "mesh_call_index": call, "mesh_index": mesh["mesh_index"],
            "cell_file_offset": mesh["cell_file_offset"],
            "face_point_indices": [face["point_indices"] for face in mesh["source_faces"]],
            "packed": int(mesh["packed_cell"], 16), "layer": mesh["layer"],
            "entry_depth": prep_row["visit_depth_before"],
            "depth": prep_row["visit_depth_after"],
            "shift": mesh["shift_026"], "limit": mesh["limit_027"],
            "prep_result": prep_row["return_value_signed"],
            "entry_winding": mesh["winding_024"],
            "effective_winding": winding[call],
            "point_count": mesh["point_count"], "face_count": mesh["face_count"],
            "points": point_rows,
            "packets": [{
                "face_index": int(packet["face_record_index"]),
                "bucket": packet["ot_insert"]["bucket_index_from_fixture_base"],
                "raw": bytes.fromhex(packet["packet_raw_40_bytes"]),
                "tag": struct.unpack_from("<I", bytes.fromhex(packet["packet_raw_40_bytes"]))[0],
            } for packet in packets],
        })
    return records


def emit_native_mesh_header(cases: list[dict], output: Path) -> Path:
    lines = [f"#define NATIVE_CASE_COUNT {len(cases)}",
             "static const NativeCase g_native_cases[NATIVE_CASE_COUNT] = {"]
    for case_index, case in enumerate(cases):
        lines.append("    {")
        lines.append("      %s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," % (
            c_u32(case["packed"]), case["layer"], case["entry_depth"], case["depth"],
            case["shift"], case["limit"], case["prep_result"], case["point_count"],
            case["face_count"], len(case["packets"]), case["entry_winding"],
            case["effective_winding"]))
        lines.append("      {")
        for point in case["points"]:
            config = point["config"]
            matrix = ",".join(str(value) for value in config["matrix"])
            translation = ",".join(str(value) + "L" for value in config["translation"])
            lines.append("        {%d,{%d,%d,%d},{%d,%d,%d},{%d,%d,%d},{%d,%d},{{%s},{%s},%dL,%dL,%s}}," % (
                point["source_offset"], *point["source"], *point["input"],
                *point["camera"], *point["xy"], matrix, translation,
                config["center"][0], config["center"][1], c_u32(config["focal"])))
        lines.append("      },")
        lines.append("      {")
        if not case["packets"]:
            lines.append("        {0,0,{0}},")
        for packet in case["packets"]:
            byte_text = ",".join("0x%02X" % byte for byte in packet["raw"])
            lines.append("        {%d,%s,{%s}}," % (packet["bucket"], c_u32(packet["tag"]), byte_text))
        lines.append("      }")
        lines.append("    }" + ("," if case_index + 1 < len(cases) else ""))
    lines.extend(["};", ""])
    header = output / "phase20_native_source_case_data.h"
    header.write_text("\n".join(lines), encoding="ascii", newline="\n")
    return header


def expected_input_xyz(event: dict) -> list[int]:
    lanes = {0xBA2600: None, 0xBA2602: None, 0xBA2604: None}
    for read in event["reads"]:
        address = int(read["address"], 16)
        if address not in lanes or int(read["size"]) != 2:
            continue
        pc = int(read["pc"], 16)
        if pc not in (0x40B6F4, 0x40B6FE, 0x40B709):
            continue
        value = bytes.fromhex(read["value"])
        if len(value) != 2:
            continue
        lanes[address] = struct.unpack("<h", value)[0]
    if any(value is None for value in lanes.values()):
        raise RuntimeError(f"2E3B0 event {event['mesh_call_index']} missing native B6F0 WORD reads")
    return [lanes[0xBA2600], lanes[0xBA2602], lanes[0xBA2604]]


def callback_path(event: dict) -> list[int]:
    mapping = {
        "herc+0xbb00": 0xBB00,
        "herc+0xbc30": 0xBC30,
        "herc+0xb880": 0xB880,
        "herc+0xb8f0": 0xB8F0,
        "herc+0xb6f0": 0xB6F0,
        "herc+0xb920": 0xB920,
        "herc+0xb740": 0xB740,
        "herc+0xb840": 0xB840,
        "herc+0xb720": 0xB720,
        "herc+0xb810": 0xB810,
    }
    return [mapping[pc.lower()] for pc in event["instruction_pcs"]
            if pc.lower() in mapping]


def emit_case_header(trace: dict, native_report: dict, output: Path) -> dict:
    map_base = int(native_report["map_location"]["map_base"], 16)
    map_len = int(native_report["map_location"]["length"])
    camera = native_report["camera_context"]
    events = trace["events"]
    if len(events) != 3:
        raise RuntimeError(f"expected the three pinned 2E3B0 calls, got {len(events)}")
    rows = []
    for event in events:
        entry_visit = bytes.fromhex(event["entry_visit_hex"])[:0x74]
        return_visit = bytes.fromhex(event["return_visit_hex"])[:0x74]
        entry_matrix = bytes.fromhex(event["entry_host_basis_0x40_hex"])
        return_matrix = bytes.fromhex(event["return_host_basis_0x40_hex"])
        entry_scratch = bytes.fromhex(event["entry_matrix_scratch_hex"])
        return_scratch = bytes.fromhex(event["return_matrix_scratch_hex"])
        if (len(entry_visit) != 0x74 or len(return_visit) != 0x74 or
                len(entry_matrix) != 0x40 or len(return_matrix) != 0x40 or
                len(entry_scratch) != 0x60 or len(return_scratch) != 0x60):
            raise RuntimeError("native 2E3B0 snapshot extent changed")
        pointers = struct.unpack_from("<III", entry_visit)
        offsets = [pointer - map_base for pointer in pointers]
        if any(offset < 0 or offset + 16 > map_len for offset in offsets):
            raise RuntimeError(f"call {event['mesh_call_index']} visit pointer escaped archived map")
        returned = struct.unpack_from("<III", return_visit)
        if returned != pointers:
            raise RuntimeError("2E3B0 changed one of the three observed resource pointers")
        camera_entry = event["entry_camera_globals"]
        camera_return = event["return_camera_globals"]
        scale = [int(x) for x in event["entry_scale_longs"]]
        path = callback_path(event)
        row = {
            "call_index": int(event["mesh_call_index"]),
            "mesh_index": int(event["mesh_index"]),
            "return_value": int(event["return_value_signed"]),
            "return_depth": int(event["visit_depth_after"]),
            "pointer_offsets": offsets,
            "camera_angle_x": int(event["entry_camera_angle_x"]),
            "scale": scale,
            "b6f0_input": expected_input_xyz(event),
            "entry_matrix_words": camera_entry["matrix_words_row_major"],
            "return_matrix_words": camera_return["matrix_words_row_major"],
            "entry_translation": camera_entry["translation_dwords"],
            "return_translation": camera_return["translation_dwords"],
            "return_camera": camera_return["camera_xyz_dwords"],
            "return_screen": camera_return["screen_xy_words"],
            "callback_path": path,
            "entry_visit": entry_visit,
            "return_visit": return_visit,
            "entry_matrix": entry_matrix,
            "return_matrix": return_matrix,
            "entry_scratch": entry_scratch,
            "return_scratch": return_scratch,
        }
        rows.append(row)

    lines = [
        "#define NATIVE_PREP_CASE_COUNT %d" % len(rows),
        "#define NATIVE_CAMERA_CENTER_X %d" % int(camera["center"][0]),
        "#define NATIVE_CAMERA_CENTER_Y %d" % int(camera["center"][1]),
        "#define NATIVE_CAMERA_FOCAL %d" % int(camera["focal"]),
        "static const NativePrepCase g_native_prep_cases[NATIVE_PREP_CASE_COUNT] = {",
    ]
    for row_index, row in enumerate(rows):
        entries = [
            str(row["call_index"]), str(row["mesh_index"]), str(row["return_value"]),
            str(row["return_depth"]),
            *["%dUL" % x for x in row["pointer_offsets"]],
            str(row["camera_angle_x"]), c_ints(row["scale"]),
            c_ints(row["b6f0_input"]), c_ints(row["entry_matrix_words"]),
            c_ints(row["return_matrix_words"]), c_ints(row["entry_translation"]),
            c_ints(row["return_translation"]), c_ints(row["return_camera"]),
            c_ints(row["return_screen"]),
            str(len(row["callback_path"])), c_ints(row["callback_path"]),
            c_bytes(row["entry_visit"]), c_bytes(row["return_visit"]),
            c_bytes(row["entry_matrix"]), c_bytes(row["return_matrix"]),
            c_bytes(row["entry_scratch"]), c_bytes(row["return_scratch"]),
        ]
        lines.append("    {" + ",".join(entries) + "}" +
                     ("," if row_index + 1 < len(rows) else ""))
    lines.extend(["};", ""])
    header = output / "prep_case_data.h"
    header.write_bytes("\n".join(lines).encode("ascii"))
    return {"header": header, "cases": [{
        "call_index": row["call_index"], "mesh_index": row["mesh_index"],
        "input_xyz": row["b6f0_input"], "return_depth": row["return_depth"],
        "callback_path": ["0x%04x" % x for x in row["callback_path"]],
    } for row in rows]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng1", required=True, type=Path)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    repo = repo_path(args.repo_root)
    if repo != discover_repo_root(LANE):
        raise ValueError("fixture lane and --repo-root disagree")
    exe = input_file(args.exe, "HERCULES.EXE")
    eng1 = input_file(args.eng1, "ENG1.DLL")
    archive = input_file(args.archive, "HERCULES.FS")
    for path, label in ((exe, "HERCULES.EXE"), (eng1, "ENG1.DLL"),
                        (archive, "HERCULES.FS")):
        if sha(path.read_bytes()) != ORIGINAL_HASHES[label]:
            raise RuntimeError(f"original input identity mismatch: {label}")
    if exe.parent != eng1.parent:
        raise ValueError("the bounded fixture requires EXE and ENG1 in one originals directory")

    requested_output = args.output if args.output.is_absolute() else repo / args.output
    native_command = [
        sys.executable, str(LANE / "run_native_prep_trace.py"),
        "--repo-root", str(repo), "--exe", str(exe), "--eng1", str(eng1),
        "--archive", str(archive), "--output", str(requested_output),
    ]
    native = subprocess.run(native_command, cwd=repo, capture_output=True,
                            text=True, timeout=180)
    out = requested_output.resolve(strict=False)
    if native.returncode:
        if out.is_dir():
            (out / "native_runner.log").write_text(native.stdout + native.stderr,
                                                   encoding="utf-8")
        print(native.stdout, end="")
        print(native.stderr, end="", file=sys.stderr)
        return native.returncode
    out = owned_output(repo, requested_output)
    (out / "native_runner.log").write_text(native.stdout + native.stderr,
                                           encoding="utf-8")

    sys.path.insert(0, str(repo))
    sys.path.insert(0, r"C:\tools\hercules\python")
    from scripts.pe import PE
    with archive.open("rb") as stream:
        stream.seek(MAP_OFFSET)
        map_data = stream.read(MAP_LENGTH)
    transport = PE(str(exe)).read_rva(HOST_TABLE_RVA, HOST_TABLE_SIZE)
    trig = PE(str(exe)).read_rva(TRIG_TABLE_RVA, TRIG_TABLE_SIZE)
    for data, name in ((map_data, "MAP_PSX.BIN"),
                       (transport, "host_transport"), (trig, "trig_table")):
        if sha(data) != ORIGINAL_HASHES[name]:
            raise RuntimeError(f"original extracted input hash mismatch: {name}")
    map_path, transport_path, trig_path = (out / "map_psx.bin",
                                           out / "host_transport.bin",
                                           out / "eng1_trig_table.bin")
    map_path.write_bytes(map_data)
    transport_path.write_bytes(transport)
    trig_path.write_bytes(trig)
    trace_path = out / "prep_instruction_trace.json"
    report_path = out / "native_real_map_report.json"
    trace_data = json.loads(trace_path.read_text(encoding="utf-8"))
    native_report = json.loads(report_path.read_text(encoding="utf-8"))
    if native_report.get("result") != "EXECUTED":
        raise RuntimeError("native resource trace did not reach its measured completion")
    if native_report.get("hashes", {}).get("MAP_PSX.BIN") != ORIGINAL_HASHES["MAP_PSX.BIN"]:
        raise RuntimeError("native trace used a different archived map identity")
    generated = emit_case_header(trace_data, native_report, out)
    mesh_cases = build_native_mesh_cases(native_report)
    mesh_case_header = emit_native_mesh_header(mesh_cases, out)
    geometry_evidence_path = repo / "renderer_contract" / "evidence" / "geometry.json"
    closure_evidence_path = repo / "renderer_contract" / "evidence" / "eng1_resource_closure.json"
    geometry_evidence = json.loads(geometry_evidence_path.read_bytes())
    closure_evidence = json.loads(closure_evidence_path.read_bytes())
    eng1_image = PE(str(eng1))
    tail_pins = []
    for pin in closure_evidence["native_trace"]["tail_branch_return_pins"]:
        target = int(pin["target_rva"].split("+", 1)[1], 16)
        size = int(pin["tail_size"], 16)
        tail_bytes = eng1_image.read_rva(target, size)
        if len(tail_bytes) != size:
            raise RuntimeError(f"could not pin complete observed branch tail at {pin['target_rva']}")
        tail_pins.append({**pin, "tail_bytes_sha256": sha(tail_bytes)})

    src = repo / "src/eng1/behavioral"
    source_files = [
        LANE / "prep_source_compare.c",
        src / "mesh_prepare.c",
        src / "host_wrappers.c",
        src / "host_wrappers_extra.c",
        repo / "renderer_contract/hercules_projection.c",
    ]
    integrated_files = [
        LANE / "prep_emitter_compare.c",
        mesh_case_header,
        src / "mesh_emit.c",
        src / "primitive_emitters_a.c",
    ]
    source_hashes = {str(path.relative_to(repo)): sha(path.read_bytes())
                     for path in source_files + integrated_files}
    for rel in ("src/eng1/behavioral/mesh_visit.h",
                "calibration/engine_interface.h",
                "calibration/engine_dispatch_externs.h",
                "calibration/resource_context.h",
                "renderer_contract/hercules_projection.h",
                "renderer_contract/hercules_host_abi.h",
                "renderer_contract/hercules_types.h"):
        path = repo / rel
        source_hashes[rel] = sha(path.read_bytes())
    source_hashes.update({
        "prep_case_data.h": sha(generated["header"].read_bytes()),
        "run_native_prep_trace.py": sha((LANE / "run_native_prep_trace.py").read_bytes()),
        "run_prep_source_compare.py": sha(Path(__file__).read_bytes()),
        "pathguard.py": sha((LANE / "pathguard.py").read_bytes()),
        "prediction.md": sha((LANE / "prediction.md").read_bytes()),
        "generated_native_source_case_data.h": sha(mesh_case_header.read_bytes()),
        "generated_native_source_cases_from_report": sha(report_path.read_bytes()),
        "renderer_contract/evidence/geometry.json": sha(geometry_evidence_path.read_bytes()),
        "renderer_contract/evidence/eng1_resource_closure.json": sha(closure_evidence_path.read_bytes()),
        "native_report": sha(report_path.read_bytes()),
        "native_trace": sha(trace_path.read_bytes()),
        "host_transport.bin": sha(transport_path.read_bytes()),
        "map_psx.bin": sha(map_path.read_bytes()),
        "eng1_trig_table.bin": sha(trig_path.read_bytes()),
        "HERCULES.EXE": sha(exe.read_bytes()),
        "ENG1.DLL": sha(eng1.read_bytes()),
        "HERCULES.FS": sha(archive.read_bytes()),
    })
    for name, expected in TOOL_HASHES.items():
        tool = TOOLCHAIN / "bin" / name
        actual = sha(tool.read_bytes())
        if actual != expected:
            raise RuntimeError(f"pinned compiler tool hash mismatch: {name}")
        source_hashes["tool_" + name] = actual

    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(TOOLCHAIN / "include")
    env["LIB"] = str(TOOLCHAIN / "lib")
    executable = out / "prep_source_compare.exe"
    command = [
        str(TOOLCHAIN / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/MT",
        "/I" + str(repo), "/I" + str(out), "/Fe" + str(executable),
        *[str(path) for path in source_files],
        "/link", "/INCREMENTAL:NO", "/OPT:REF", "kernel32.lib",
    ]
    build = subprocess.run(command, cwd=out, env=env, capture_output=True,
                           text=True, timeout=90)
    (out / "compile.log").write_text(build.stdout + build.stderr,
                                      encoding="utf-8")
    if build.returncode:
        result = {"result": "COMPILE_FAILURE", "command": command,
                  "compiler_output": build.stdout + build.stderr,
                  "source_hashes": source_hashes}
        (out / "prep_source_compare_report.json").write_text(
            json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2))
        return 1
    run = subprocess.run([str(executable), str(map_path), str(transport_path),
                          str(trig_path)], cwd=out, env=env, capture_output=True,
                         text=True, timeout=30)
    (out / "source_run.log").write_text(run.stdout + run.stderr,
                                        encoding="utf-8")
    combined_exe = out / "prep_emitter_compare.exe"
    combined_command = [
        str(TOOLCHAIN / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/MT",
        "/I" + str(repo), "/I" + str(out), "/I" + str(LANE),
        "/Fe" + str(combined_exe), str(LANE / "prep_emitter_compare.c"),
        "/link", "/INCREMENTAL:NO", "/OPT:REF", "kernel32.lib",
    ]
    combined_build = subprocess.run(combined_command, cwd=out, env=env,
                                   capture_output=True, text=True, timeout=90)
    (out / "combined_compile.log").write_text(
        combined_build.stdout + combined_build.stderr, encoding="utf-8")
    if combined_build.returncode:
        combined = {"result": "COMPILE_FAILURE", "command": combined_command,
                    "compiler_output": combined_build.stdout + combined_build.stderr}
        combined_run = None
    else:
        combined_run = subprocess.run(
            [str(combined_exe), str(map_path), str(transport_path), str(trig_path)],
            cwd=out, env=env, capture_output=True, text=True, timeout=30)
        (out / "combined_source_run.log").write_text(
            combined_run.stdout + combined_run.stderr, encoding="utf-8")
        combined = {
            "result": "PASS" if combined_run.returncode == 0 else "SOURCE_DIFFERENTIAL_FAILED",
            "command": combined_command,
            "compile_exit_code": combined_build.returncode,
            "run_exit_code": combined_run.returncode,
            "stdout": combined_run.stdout,
            "stderr": combined_run.stderr,
            "executable_sha256": sha(combined_exe.read_bytes()),
        }
    result = {
        "result": "PASS" if run.returncode == 0 and combined.get("result") == "PASS" else "SOURCE_DIFFERENTIAL_FAILED",
        "scope": "Three original ENG1 2E3B0 producer calls over the real archived FOREST MAP bytes; camera/context inputs are controlled and named. The current mesh_prepare.c source is compiled once with its existing ENG1 wrapper providers and strict service spies, and once in a private test adapter feeding the unchanged mesh_emit.c/primitive_emitters_a.c resource-backed FT4 source. The B920/B740 path uses shared hercules_projection_eval; unsupported angle/scale inputs fail visibly. The candidate derives the cold-cache transform/depth/config, reproduces warm-cache reuse, callback order, B810 visit overlap and return, then its state feeds source point projection, face culling, packets and OT insertion. This does not claim a full engine or strict function match.",
        "cases": generated["cases"],
        "stdout": run.stdout,
        "stderr": run.stderr,
        "compile_command": command,
        "compile_exit_code": build.returncode,
        "run_exit_code": run.returncode,
        "prep_only_source_test": {"result": "PASS" if run.returncode == 0 else "SOURCE_DIFFERENTIAL_FAILED",
                                  "stdout": run.stdout,
                                  "stderr": run.stderr,
                                  "executable_sha256": sha(executable.read_bytes())},
        "resource_emitter_integration": combined,
        "source_hashes": source_hashes,
        "original_hashes": ORIGINAL_HASHES,
        "native_trace": {"report_sha256": source_hashes["native_report"],
                         "instruction_trace_sha256": source_hashes["native_trace"],
                         "observed_call_paths": [{
                             "mesh_call_index": int(event["mesh_call_index"]),
                             "mesh_index": int(event["mesh_index"]),
                             "entry_rva": "0x2e3b0",
                             "probe_last_pcs": event["instruction_pcs"][-8:],
                             "last_executed_instruction": event["instruction_pcs"][-1],
                             "caller_continuation_rva": event["return_pc"],
                         } for event in trace_data["events"]],
                         "observed_probe_extent": closure_evidence["native_trace"]["span_start_size_rva"]["ENG1+0x2E3B0_native_prep_observation"],
                         "historical_first_return_extent": geometry_evidence["eng1_original_producer_spans"]["0x2e3b0"],
                         "historical_first_return_extents": geometry_evidence["eng1_original_producer_spans"],
                         "historical_branch_tail_pins": tail_pins,
                         "historical_branch_tail_pins_source_sha256": sha(closure_evidence_path.read_bytes()),
                         "recovery_extent_unchanged": True},
        "limitations": [
            "Only observed call 2/23/74 states with zero Euler angles and uniform 0x2000 scale are accepted by the host spies; other callback inputs fail visibly.",
            "The captured camera center/focal and preexisting host matrix for warm-cache calls are controlled fixture inputs, not claimed live startup state.",
            "No reachable selected case exercises negative/high-bit camera Z; signed WORD narrowing is compiled and the B810 overlap/order is compared, but no out-of-domain wrap discriminator is claimed.",
            "The first-return recovery extent is not enlarged by the observed later branch-tail path.",
        ],
    }
    (out / "prep_source_compare_report.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if run.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
