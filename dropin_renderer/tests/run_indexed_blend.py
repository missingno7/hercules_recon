"""Compare connected original uploads/blended rectangles with owned CPU/GPU output."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "renderer_contract/fixtures/eng1_real_mesh"))
from pathguard import _under_work, input_file

CASES = tuple(f"{mode}_s{selector}" for mode in ("raw", "shaded") for selector in range(4)) + \
    ("raw_transparent", "shaded_transparent", "shaded_zero")
SHADE_RECIPE = "shade[0x31]=0; otherwise ((index*29+71) XOR (index>>1) XOR ((index*index)>>5)) & 0xff"
BLEND_RECIPE = "(source*37 + destination*13 + selector*59 + ((source XOR destination)*7) + ((source*destination)>>4) + 17) & 0xff; row-major source,destination"

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def sha(path):
    return digest(path.read_bytes())

def fixture_writes(native):
    """Recreate explicitly declared synthetic writer inputs, never game tables."""
    descriptors = native["synthetic_table_writers"]
    shade = descriptors["shade_row"]
    blend = descriptors["selected_blend_lut"]
    if shade["recipe"] != SHADE_RECIPE or blend["recipe"] != BLEND_RECIPE:
        raise ValueError("unknown synthetic writer recipe")
    shade_bytes = bytes(0 if i == 0x31 else ((i * 29 + 71) ^ (i >> 1) ^ ((i * i) >> 5)) & 255
                        for i in range(256))
    selector = native["input"]["selector"]
    if selector not in range(4) or blend["selector"] != selector:
        raise ValueError("selector mismatch")
    blend_bytes = bytes((s * 37 + d * 13 + selector * 59 + ((s ^ d) * 7) + ((s * d) >> 4) + 17) & 255
                        for s in range(256) for d in range(256))
    writes = [(shade, shade_bytes), (blend, blend_bytes)]
    for descriptor, raw in writes:
        if len(raw) != descriptor["size"] or digest(raw) != descriptor["sha256"]:
            raise ValueError("synthetic writer hash mismatch")
    if int(shade["address"], 16) != 0x91c488 + shade["row_index"] * 256 or \
            int(blend["address"], 16) != 0x92e3d0 + selector * 65536:
        raise ValueError("synthetic writer address mismatch")
    return [(int(descriptor["address"], 16), raw) for descriptor, raw in writes]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--cli", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--gpu", action="store_true")
    args = parser.parse_args()
    output = _under_work(ROOT, args.output)
    if output.exists():
        raise ValueError("output must be fresh")
    exe = input_file(args.exe, "pinned original EXE")
    if sha(exe) != "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d":
        raise ValueError("original image mismatch")
    cli = input_file(args.cli, "indexed diagnostic")
    build_path = input_file(cli.parent / "build_receipt.json", "source-built receipt")
    build = json.loads(build_path.read_bytes())
    if cli.name != "indexed_rectangle_cli.exe" or not build.get("passed") or \
            build.get("fixture_executables", {}).get(cli.name) != sha(cli):
        raise ValueError("CLI is not the verified source-built fixture")
    for name, expected in build["source_sha256"].items():
        if sha(ROOT / name) != expected:
            raise ValueError("compiled source receipt is stale: " + name)
    output.mkdir(parents=True)
    runner = ROOT / "renderer_contract/tests/test_original_uploaded_blend.py"
    cases = []
    for name in CASES:
        native_path = output / (name + "_native.json")
        native_command = [sys.executable, "-B", str(runner), "--exe", str(exe),
                          "--receipt", str(native_path), "--case", name]
        result = subprocess.run(native_command, cwd=ROOT, capture_output=True, text=True, timeout=30)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        native = json.loads(native_path.read_bytes())
        if native["status"] != "BOUNDED_ORIGINAL_UPLOADED_BLEND_PASS" or native["case"] != name or \
                native["runner"]["sha256"] != sha(runner) or native["oracle"]["exe_sha256"] != sha(exe):
            raise ValueError("native receipt identity mismatch")
        inputs = native["input"]
        writes = fixture_writes(native)
        input_path = output / (name + ".txt")
        fields = ["HERCULES_INDEXED_RECTANGLE_V2", str(int(inputs["packet_address"], 16)), inputs["packet_hex"],
                  *(str(value) for value in inputs["upload_rect_words_xywh"]),
                  inputs["upload_payload_hex"], str(inputs["initial_surface_byte"]), str(len(writes))]
        for address, raw in writes:
            fields.extend((str(address), raw.hex()))
        input_path.write_bytes(("\n".join(fields) + "\n").encode("ascii"))
        frame_path = output / (name + ".idx")
        command = [str(cli), str(input_path), str(frame_path)] + (["--gpu"] if args.gpu else [])
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=30)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        modern = json.loads(result.stdout)
        if modern["status"] != "PASS" or native["frame_bytes"] != 76800 or len(frame_path.read_bytes()) != 76800 or sha(frame_path) != native["frame_sha256"]:
            raise ValueError("complete original/modern index buffer mismatch: " + name)
        if modern["pixel_stores"] != len(native["surface_stores"]) or not modern["presentation_requested"]:
            raise ValueError("pixel count/presentation mismatch: " + name)
        if modern["blend_selector"] != inputs["selector"] or modern["pixel_mode"] != (1 if name.startswith("raw") else 2):
            raise ValueError("selected route mismatch: " + name)
        if args.gpu and modern["gpu"] != "exact_indices":
            raise ValueError("GPU comparison did not pass")
        cases.append({"case": name, "native_receipt_sha256": sha(native_path), "input_sha256": sha(input_path),
                      "frame_sha256": sha(frame_path), "modern": modern,
                      "native_command": native_command, "modern_command": command})
    receipt = {"schema_version": 1, "status": "ORIGINAL_CPU_GPU_BLEND_PASS" if args.gpu else "ORIGINAL_CPU_BLEND_PASS",
               "original_game_process_executed": False, "build_receipt_sha256": sha(build_path),
               "cli_sha256": sha(cli), "native_runner_sha256": sha(runner), "runner_sha256": sha(Path(__file__)),
               "source_sha256": build["source_sha256"], "cases": cases,
               "scope": "Eleven controlled synthetic uploads/FT4s; finite indexed shaded/raw blend and transparent behavior; no host, game palette or general polygon proof"}
    (output / "comparison.json").write_bytes((json.dumps(receipt, indent=2) + "\n").encode())
    print(json.dumps({"status": receipt["status"], "cases": [{"case": c["case"], **c["modern"]} for c in cases]}, indent=2))

if __name__ == "__main__":
    main()
