"""Fresh unchanged-original upload -> captured tile -> CPU/GPU index comparison."""
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


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


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
    build_path = input_file(cli.parent / "build_receipt.json", "source-built diagnostic receipt")
    build = json.loads(build_path.read_bytes())
    if cli.name != "indexed_rectangle_cli.exe" or not build.get("passed") or \
            build.get("fixture_executables", {}).get(cli.name) != sha(cli):
        raise ValueError("CLI is not the verified source-built fixture")
    for name, digest in build["source_sha256"].items():
        if sha(ROOT / name) != digest:
            raise ValueError("compiled fixture source receipt is stale: " + name)
    output.mkdir(parents=True)
    cases = []
    runner = ROOT / "renderer_contract/tests/test_original_uploaded_rectangle.py"
    for case in ("base", "offset_rectangle", "transparent"):
        native_path = output / (case + "_native.json")
        native_command = [sys.executable, "-B", str(runner), "--exe", str(exe),
                          "--receipt", str(native_path), "--case", case]
        native_run = subprocess.run(native_command, cwd=ROOT, capture_output=True, text=True, timeout=30)
        if native_run.returncode:
            raise RuntimeError(native_run.stdout + native_run.stderr)
        native = json.loads(native_path.read_bytes())
        if native.get("status") != "BOUNDED_ORIGINAL_UPLOAD_RECTANGLE_PASS" or native.get("case") != case:
            raise ValueError("original receipt status/identity mismatch")
        observed = native["input"]
        input_path = output / (case + ".txt")
        fields = ["HERCULES_INDEXED_RECTANGLE_V1", str(observed["packet_address"]), observed["packet_hex"],
                  *(str(value) for value in observed["upload_rect"]),
                  observed["upload_payload_hex"], str(observed["initial_pixel"])]
        input_path.write_bytes(("\n".join(fields) + "\n").encode("ascii"))
        frame_path = output / (case + ".idx")
        command = [str(cli), str(input_path), str(frame_path)]
        if args.gpu:
            command.append("--gpu")
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=30)
        if result.returncode == 77:
            raise RuntimeError("requested GPU comparison skipped: " + result.stderr.strip())
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        modern = json.loads(result.stdout)
        if len(frame_path.read_bytes()) != 76800 or sha(frame_path) != native["frame_sha256"]:
            raise ValueError("modern/original complete index buffer mismatch: " + case)
        if modern["pixel_stores"] != len(native["surface_stores"]) or not modern["presentation_requested"]:
            raise ValueError("store count/presentation effect mismatch: " + case)
        if args.gpu and modern["gpu"] != "exact_indices":
            raise ValueError("GPU index comparison did not pass")
        cases.append({"case": case, "native_receipt_sha256": sha(native_path),
                      "input_sha256": sha(input_path), "frame_sha256": sha(frame_path),
                      "modern": modern, "native_command": native_command, "modern_command": command})
    receipt = {"schema_version": 1, "status": "ORIGINAL_CPU_GPU_INDEXED_PASS" if args.gpu else "ORIGINAL_CPU_INDEXED_PASS",
               "original_game_process_executed": False, "build_receipt_sha256": sha(build_path),
               "cli_sha256": sha(cli), "native_runner_sha256": sha(runner),
               "source_sha256": build["source_sha256"], "cases": cases,
               "scope": "Three controlled detached uploads/FT4 rectangles, raw index equality; no palette conversion, host, complete frame or general renderer claim"}
    (output / "comparison.json").write_bytes((json.dumps(receipt, indent=2) + "\n").encode())
    print(json.dumps({"status": receipt["status"], "cases": [{"case": row["case"], **row["modern"]} for row in cases]}, indent=2))


if __name__ == "__main__":
    main()
