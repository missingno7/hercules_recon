from __future__ import annotations

import hashlib
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
sys.dont_write_bytecode = True

LANE = Path(__file__).resolve().parent
from pathguard import (create_fresh_child, discover_repo_root, file_below,
                       owned_output, input_file, repo_path)

SCRIPT_REPO = discover_repo_root(LANE)
ROOT = SCRIPT_REPO
sys.path.insert(0, str(ROOT))
sys.path.insert(0, r"C:\tools\hercules\python")
from scripts.pe import PE

TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
TOOLCHAIN_HASHES = {
    "cl.exe": "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb",
    "link.exe": "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1",
}
HARNESS = LANE / "test_real_mesh_source_compare.c"
OUT = None
MAP_OUT = None
TRANSPORT_OUT = None
HEADER_OUT = None
OUTPUT_REPORT = None

EXPECTED_HASHES = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "HERCULES.FS": "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4",
    "MAP_PSX.BIN": "78bc5a60ecce1c99e752fb30a16bc5ace4ee80f686365e1da8136a0decbd69cb",
}
MAP_OFFSET = 18845296
MAP_LENGTH = 1304576
HOST_TABLE_RVA = 0x361B8
HOST_TABLE_BYTES = 1000
HOST_TABLE_SHA256 = "52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def c_u32(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08X}UL"


def build_cases(report):
    main_case = report["selected_real_resource_case"]
    cases = [{"mesh_call_index": main_case["mesh_call_index"],
              "mesh": main_case["mesh"], "point_outputs": main_case["point_outputs"],
              "packets": main_case["packets"]}]
    cases.extend(report["auxiliary_real_resource_cases"])
    write_by_call = {e["mesh_call_index"]: int(e["value"], 16)
                     for e in report["selected_and_auxiliary_winding_writes"]}
    prep_by_call = {e["mesh_call_index"]: e
                    for e in report["selected_and_auxiliary_2e3b0_results"]}
    records = []
    center = report["camera_context"]["center"]
    focal = report["camera_context"]["focal"]
    for case in cases:
        mesh = case["mesh"]
        call_index = case["mesh_call_index"]
        prep = prep_by_call.get(call_index)
        if prep is None:
            raise RuntimeError(f"call {call_index} lacks original 2E3B0 entry/return data")
        points_by_index = {e["source_point_index"]: e for e in case["point_outputs"]}
        source_by_index = {e["index"]: e for e in mesh["source_points"]}
        if (set(points_by_index) != set(range(mesh["point_count"])) or
                set(source_by_index) != set(range(mesh["point_count"]))):
            raise RuntimeError(f"call {call_index} lacks captured native point outputs")
        point_rows = []
        for i in range(mesh["point_count"]):
            native_point = points_by_index[i]
            source_point = source_by_index[i]
            if (int(native_point["source_address"], 16) != int(source_point["address"], 16) or
                    int(native_point["source_packed_dword"], 16) != int(source_point["packed_dword"], 16)):
                raise RuntimeError(f"call {call_index} point {i} read/output identity chain changed")
            raw_xyz = source_point["source_xyz"]
            expected_x = raw_xyz[0] if write_by_call[call_index] else 64 - raw_xyz[0]
            expected_input = [expected_x, raw_xyz[1], raw_xyz[2]]
            if native_point["host_input_signed_words"] != expected_input:
                raise RuntimeError(f"call {call_index} point {i} packed XYZ to B6F0 derivation changed")
            point_rows.append({
                "source_offset": source_point["file_offset"],
                "source": raw_xyz,
                "input": native_point["host_input_signed_words"],
                "camera": native_point["camera_xyz_signed"],
                "xy": native_point["scratch_xy_after_return"],
                "config": {
                    "matrix": native_point["camera_transform"]["matrix_words_row_major"],
                    "translation": native_point["camera_transform"]["translation_dwords"],
                    "center": center,
                    "focal": focal,
                }})
        packet_rows = case["packets"]
        if len(packet_rows) > 8 or mesh["point_count"] > 16:
            raise RuntimeError("case exceeds fixed private fixture arrays")
        faces_seen = {int(p["face_record_index"]) for p in packet_rows}
        if len(faces_seen) != len(packet_rows):
            raise RuntimeError("test cases require unique emitted face indices")
        records.append({
            "mesh_call_index": call_index,
            "mesh_index": mesh["mesh_index"],
            "cell_file_offset": mesh["cell_file_offset"],
            "face_point_indices": [face["point_indices"] for face in mesh["source_faces"]],
            "packed": int(mesh["packed_cell"], 16),
            "layer": mesh["layer"],
            "entry_depth": prep["visit_depth_before"],
            "depth": prep["visit_depth_after"],
            "shift": mesh["shift_026"],
            "limit": mesh["limit_027"],
            # The captured original path enters 2E600, so 2E3B0 returned 0.
            # Its actual depth is retained as a data-only preparation boundary.
            "prep_result": prep["return_value_signed"],
            "entry_winding": mesh["winding_024"],
            "effective_winding": write_by_call[call_index],
            "point_count": mesh["point_count"],
            "face_count": mesh["face_count"],
            "points": point_rows,
            "packets": [
                {"face_index": int(p["face_record_index"]),
                 "bucket": p["ot_insert"]["bucket_index_from_fixture_base"],
                 "raw": bytes.fromhex(p["packet_raw_40_bytes"]),
                 "tag": struct.unpack_from("<I", bytes.fromhex(p["packet_raw_40_bytes"]))[0]}
                for p in packet_rows],
        })
    return records


def emit_header(cases):
    lines = ["#define NATIVE_CASE_COUNT %d" % len(cases),
             "static const NativeCase g_native_cases[NATIVE_CASE_COUNT] = {"]
    for ci, case in enumerate(cases):
        lines.append("    {")
        lines.append("      %s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," % (
            c_u32(case["packed"]),case["layer"],case["entry_depth"],case["depth"],case["shift"],case["limit"],
            case["prep_result"],case["point_count"],case["face_count"],len(case["packets"]),
            case["entry_winding"],case["effective_winding"]))
        lines.append("      {")
        for point in case["points"]:
            cfg=point["config"]
            matrix=",".join(str(x) for x in cfg["matrix"])
            translation=",".join(str(x)+"L" for x in cfg["translation"])
            lines.append("        {%d,{%d,%d,%d},{%d,%d,%d},{%d,%d,%d},{%d,%d},{{%s},{%s},%dL,%dL,%s}}," % (
                point["source_offset"], *point["source"], *point["input"],
                *point["camera"], *point["xy"], matrix, translation,
                cfg["center"][0], cfg["center"][1], c_u32(cfg["focal"])))
        lines.append("      },")
        lines.append("      {")
        if not case["packets"]:
            lines.append("        {0,0,{0}},")
        for packet in case["packets"]:
            byte_text = ",".join("0x%02X" % b for b in packet["raw"])
            lines.append("        {%d,%s,{%s}}," % (packet["bucket"],c_u32(packet["tag"]),byte_text))
        lines.append("      }")
        lines.append("    }%s" % ("," if ci+1<len(cases) else ""))
    lines.extend(["};", ""])
    HEADER_OUT.write_text("\n".join(lines), encoding="ascii", newline="\n")


def main(argv=None) -> int:
    global ROOT, OUT, MAP_OUT, TRANSPORT_OUT, HEADER_OUT, OUTPUT_REPORT
    parser = argparse.ArgumentParser(description="Compile and run the bounded ENG1 real-resource source comparison.")
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--eng1", required=True, type=Path)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path,
                        help="existing fresh output directory created by run_native_real_map.py")
    parser.add_argument("--report", type=Path,
                        help="native trace report; defaults to <output>/native_real_map_report.json")
    parser.add_argument("--manifest", type=Path,
                        help="geometry manifest; defaults to <output>/real_resource_projection_manifest.json")
    args = parser.parse_args(argv)
    ROOT = repo_path(args.repo_root)
    if ROOT != SCRIPT_REPO:
        raise ValueError(f"fixture location and --repo-root disagree: {SCRIPT_REPO} vs {ROOT}")
    output_root = owned_output(ROOT, args.output)
    REPORT = file_below(args.report or output_root / "native_real_map_report.json",
                        output_root, "native report")
    MANIFEST = file_below(args.manifest or output_root / "real_resource_projection_manifest.json",
                          output_root, "geometry manifest")
    if REPORT.name != "native_real_map_report.json" or MANIFEST.name != "real_resource_projection_manifest.json":
        raise ValueError("source comparison accepts only the fresh native report and generated manifest names")
    EXE = input_file(args.exe, "HERCULES.EXE")
    ENG1 = input_file(args.eng1, "ENG1.DLL")
    ARCHIVE = input_file(args.archive, "HERCULES.FS")
    OUT = create_fresh_child(output_root, "source_compare_vc5")
    MAP_OUT = OUT / "map_psx.bin"
    TRANSPORT_OUT = OUT / "host_transport.bin"
    HEADER_OUT = OUT / "native_source_case_data.h"
    OUTPUT_REPORT = OUT / "source_compare_report.json"
    report_bytes = REPORT.read_bytes()
    manifest_bytes = MANIFEST.read_bytes()
    report = json.loads(report_bytes)
    manifest = json.loads(manifest_bytes)
    if manifest["provenance"]["fixture_report_sha256"] != sha(report_bytes):
        raise RuntimeError("frozen manifest/report SHA link mismatch")

    source_hashes = {
        "native_report": sha(report_bytes),
        "manifest": sha(manifest_bytes),
        "harness": sha(HARNESS.read_bytes()),
        "native_trace_runner": sha((LANE / "run_native_real_map.py").read_bytes()),
        "manifest_generator": sha((LANE / "make_real_manifest.py").read_bytes()),
        "native_oracle_fixture": sha((ROOT / "renderer_contract/fixtures/eng1_association/run_fixture.py").read_bytes()),
        "output_pathguard": sha((LANE / "pathguard.py").read_bytes()),
        "output_pathguard_tests": sha((LANE / "test_path_guards.py").read_bytes()),
        "prediction": sha((LANE / "prediction.md").read_bytes()),
        "mesh_emit.c": sha((ROOT / "src/eng1/behavioral/mesh_emit.c").read_bytes()),
        "primitive_emitters_a.c": sha((ROOT / "src/eng1/behavioral/primitive_emitters_a.c").read_bytes()),
        "mesh_visit.h": sha((ROOT / "src/eng1/behavioral/mesh_visit.h").read_bytes()),
        "hercules_projection.h": sha((ROOT / "renderer_contract/hercules_projection.h").read_bytes()),
        "hercules_projection.c": sha((ROOT / "renderer_contract/hercules_projection.c").read_bytes()),
        "engine_interface.h": sha((ROOT / "calibration/engine_interface.h").read_bytes()),
        "engine_dispatch_externs.h": sha((ROOT / "calibration/engine_dispatch_externs.h").read_bytes()),
        "resource_context.h": sha((ROOT / "calibration/resource_context.h").read_bytes()),
    }
    for tool_name, expected_hash in TOOLCHAIN_HASHES.items():
        tool_path = TOOLCHAIN / "bin" / tool_name
        tool_hash = sha(tool_path.read_bytes())
        if tool_hash != expected_hash:
            raise RuntimeError(f"pinned VC5 tool identity mismatch: {tool_name}")
        source_hashes["tool_" + tool_name] = tool_hash
    if {name:sha(path.read_bytes()) for name,path in (
            ("HERCULES.EXE",EXE),("ENG1.DLL",ENG1),("HERCULES.FS",ARCHIVE))} != {
            name:EXPECTED_HASHES[name] for name in ("HERCULES.EXE","ENG1.DLL","HERCULES.FS") }:
        raise RuntimeError("one or more original oracle/archive hashes changed")
    with ARCHIVE.open("rb") as stream:
        stream.seek(MAP_OFFSET)
        map_data=stream.read(MAP_LENGTH)
    if len(map_data)!=MAP_LENGTH or sha(map_data)!=EXPECTED_HASHES["MAP_PSX.BIN"]:
        raise RuntimeError("archived MAP_PSX.BIN bytes do not match recorded asset identity")
    MAP_OUT.write_bytes(map_data)
    transport = PE(str(EXE)).read_rva(HOST_TABLE_RVA, HOST_TABLE_BYTES)
    if len(transport) != HOST_TABLE_BYTES or sha(transport) != HOST_TABLE_SHA256:
        raise RuntimeError("original full 250-DWORD host transport identity mismatch")
    TRANSPORT_OUT.write_bytes(transport)
    source_hashes["original_host_transport"] = sha(transport)
    cases=build_cases(report)
    emit_header(cases)
    source_hashes["generated_case_header"] = sha(HEADER_OUT.read_bytes())
    source_hashes["source_compare_runner"] = sha(Path(__file__).read_bytes())

    env=os.environ.copy()
    for key in ("CL","_CL_","LINK","_LINK_"):
        env.pop(key,None)
    env["PATH"]=str(TOOLCHAIN/"bin")+os.pathsep+env.get("PATH","")
    env["INCLUDE"]=str(TOOLCHAIN/"include")
    env["LIB"]=str(TOOLCHAIN/"lib")
    executable=OUT/"mesh_source_compare.exe"
    command=[str(TOOLCHAIN/"bin/cl.exe"),"/nologo","/O2","/Gy","/MT",
             "/I"+str(ROOT),"/I"+str(OUT),"/Fe"+str(executable),str(HARNESS),
             "/link","/INCREMENTAL:NO","/OPT:REF","kernel32.lib"]
    try:
        build=subprocess.run(command,cwd=OUT,env=env,capture_output=True,text=True,timeout=60)
    except OSError as exc:
        report_out={"result":"BUILD_LAUNCH_BLOCKED","scope":"Fresh pinned VC5 x86 source semantics comparison using original archived resource bytes and captured native point/projection services.","error":repr(exc),"command":command,"input_hashes":source_hashes}
        OUTPUT_REPORT.write_text(json.dumps(report_out,indent=2)+"\n",encoding="utf-8")
        print(json.dumps(report_out,indent=2));return 2
    (OUT/"compile.log").write_text(build.stdout+build.stderr,encoding="utf-8")
    if build.returncode:
        report_out={"result":"COMPILE_FAILURE","command":command,"compile_exit_code":build.returncode,"compile_output":build.stdout+build.stderr,"input_hashes":source_hashes}
        OUTPUT_REPORT.write_text(json.dumps(report_out,indent=2)+"\n",encoding="utf-8")
        print(json.dumps(report_out,indent=2));return 1
    run=subprocess.run([str(executable),str(MAP_OUT),str(TRANSPORT_OUT)],cwd=OUT,env=env,capture_output=True,text=True,timeout=60)
    (OUT/"run.log").write_text(run.stdout+run.stderr,encoding="utf-8")
    result={"result":"PASS" if run.returncode==0 else "SOURCE_DIFFERENTIAL_FAILED",
            "scope":"The unchanged original HERCULES.EXE/ENG1.DLL resource path independently ran 20670/208F0 resource setup, 2DA90 traversal, 2DF20 mesh decode, 2E3B0 preparation, 2E600 points, host B740/B920/B720, FT4 packet writes and OT links against archived MAP_PSX.BIN. A fresh VC5 RTM x86 candidate compiles mesh_emit.c and primitive_emitters_a.c with hercules_projection_eval and compares three actual resource calls. The C candidate reads source points, faces, UV/material records from the archived map; computes camera XYZ and screen XY independently from each captured per-point B6F0 input/config; checks native B740/B920 outputs; and writes only C-derived XY before packet emission. It copies the measured 250-DWORD host transport first, then substitutes the controlled context and bounded scratch backing. The 2E3B0 matrix/translation preparation remains an explicit native data-only boundary in the C candidate.",
            "toolchain":"MSVC 5.00 RTM 11.00.7022; /O2 /Gy /MT", "command":command,
            "compile_exit_code":build.returncode,"run_exit_code":run.returncode,
            "stdout":run.stdout,"stderr":run.stderr,"input_hashes":source_hashes,
            "original_hashes":EXPECTED_HASHES,"archive_offset":MAP_OFFSET,"map_length":MAP_LENGTH,
            "source_status":{"behavior_verified":["Three named real-resource 2DF20/2E600 FT4 cases for B6F0 input derivation, C-derived B740/B920-compatible camera/XY, point-to-face association, culling, packet bytes/40-byte allocation stride and OT bucket insertion."],"original_trace_only":["HERCULES.EXE/ENG1.DLL resource consumer 20670/208F0, caller 2DA90, original 2E3B0 and native callback packet/OT execution."],"source_reconstructed_unverified":["The C comparison does not independently reconstruct 2E3B0’s matrix/translation preparation or HERC B880/B8F0/B970/B990 live setup."],"strict_match":"not claimed"},
            "native_resource_trace":{"entry_chain":["ENG1+0x20670 resource rebind","ENG1+0x208F0 resource row consumer","ENG1+0x2DA90 finite traversal","ENG1+0x2DF20 mesh producer","ENG1+0x2E3B0 prep","ENG1+0x2E600 point producer","HERC B6F0/B740/B920/B720","ENG1 FT4 callback and OT writer"],"executed_span_labels":report["executed_spans"],"mesh_calls":report["mesh_call_count"],"packet_writes":report["packet_write_count"],"ot_writes":report["ot_write_count"],"2e3b0_selected_auxiliary":report["selected_and_auxiliary_2e3b0_results"],"resource_setup":report["native_resource_setup"],"captured_camera_context":report["camera_context"]},
            "case_inputs":[{"mesh_call_index":c["mesh_call_index"],"mesh_index":c["mesh_index"],"cell_file_offset":c["cell_file_offset"],"packed_cell":hex(c["packed"]),"source_indices":list(range(c["point_count"])),"face_point_indices":c["face_point_indices"],"point_count":c["point_count"],"face_count":c["face_count"],"entry_depth_before_2e3b0":c["entry_depth"],"depth_after_2e3b0":c["depth"],"2e3b0_return":c["prep_result"],"entry_winding":c["entry_winding"],"effective_winding":c["effective_winding"],"packet_count":len(c["packets"])} for c in cases],
            "generated_map_slice_sha256":sha(MAP_OUT.read_bytes()),
            "generated_host_transport_sha256":sha(TRANSPORT_OUT.read_bytes()),
            "generated_case_header_sha256":sha(HEADER_OUT.read_bytes()),
            "executable_sha256":sha(executable.read_bytes()),
            "limitations":["The source comparison does not compile 2DA90 traversal or 2E3B0’s matrix/translation preparation and HERC B880/B8F0/B970/B990 state setup. Per-point config and actual 2E3B0 result/depth are native observations supplied as data; prep remains SOURCE_RECONSTRUCTED/unverified in the C candidate.","The candidate evaluates shared B740/B920 arithmetic from exact per-point B6F0 inputs and captured state; it does not derive the live camera transform from frame/gameplay state.","Only three named real resource/camera calls are compared; broad map/cell coverage and allocation capacity are not established.","The bounded OT backing is seeded with captured original previous-head words; the C candidate then reproduces packet tags and inserts its own packet address at the observed buckets."]}
    OUTPUT_REPORT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(result,indent=2))
    return 0 if run.returncode==0 else 1


if __name__=="__main__":
    raise SystemExit(main())
