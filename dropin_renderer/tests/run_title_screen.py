"""Compare archived pixels and CIF colors through original, shared CPU and GPU paths."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
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
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--cli", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--gpu", action="store_true")
    args = parser.parse_args()
    output = _under_work(ROOT, args.output)
    if output.exists():
        raise ValueError("output must be fresh")
    exe = input_file(args.exe, "original EXE")
    archive = input_file(args.archive, "original archive")
    cli = input_file(args.cli, "source-built title diagnostic")
    build_path = input_file(cli.parent / "build_receipt.json", "build receipt")
    build = json.loads(build_path.read_bytes())
    if cli.name != "title_resource_cli.exe" or not build.get("passed") or \
            build.get("fixture_executables", {}).get(cli.name) != sha(cli):
        raise ValueError("CLI is not the verified source-built fixture")
    for name, digest in build["source_sha256"].items():
        if sha(ROOT / name) != digest:
            raise ValueError("compiled fixture source changed: " + name)
    manifest = json.loads((ROOT / "dropin_renderer/docs/local_toolchain.json").read_bytes())
    env = os.environ.copy()
    env["PATH"] = manifest["compiler_bin"] + os.pathsep + env.get("PATH", "")
    output.mkdir(parents=True)
    commands = []
    receipts = {}
    for name, filename in (("pixels", "test_original_title_screen.py"),
                           ("colors", "test_original_cif_palette.py")):
        runner = ROOT / "renderer_contract/tests" / filename
        path = output / (name + ".json")
        command = [sys.executable, "-B", str(runner), "--exe", str(exe), "--archive", str(archive),
                   "--receipt", str(path), "--output", str(output / (name + "_data"))]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=120)
        (output / (name + ".log")).write_bytes((result.stdout + result.stderr).encode())
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        receipts[name] = json.loads(path.read_bytes())
        commands.append({"argv": command, "runner_sha256": sha(runner), "receipt_sha256": sha(path)})
    pixels = receipts["pixels"]["execution"]["frame_consumption"]
    colors = receipts["colors"]["execution"]["palette_writer"]
    native_frame = input_file(ROOT / pixels["visible_frame_output"], "original visible indices")
    native_palette = input_file(ROOT / colors["raw_logical_palette_output"]["path"], "original logical palette")
    if sha(native_frame) != pixels["visible_frame_sha256"] or sha(native_palette) != colors["logical_entries_sha256"]:
        raise ValueError("original output receipt mismatch")
    modern_dir = output / "modern"
    command = [str(cli), str(archive), str(modern_dir)] + (["--gpu"] if args.gpu else [])
    result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    modern = json.loads(result.stdout)
    if modern.get("status") != "PASS" or modern["sprite_count"] != 2 or modern["pixel_stores"] != 76800:
        raise ValueError("modern resource consumption failed")
    if (modern_dir / "frame.idx").read_bytes() != native_frame.read_bytes() or \
            (modern_dir / "palette.entries").read_bytes() != native_palette.read_bytes():
        raise ValueError("full original/shared frame or palette mismatch")
    palette = native_palette.read_bytes()
    rgba = b"".join(palette[index * 4:index * 4 + 3] + b"\xff" for index in native_frame.read_bytes())
    if len(rgba) != 307200 or (modern_dir / "frame.rgba").read_bytes() != rgba:
        raise ValueError("logical RGB to opaque modern RGBA mismatch")
    if not modern["presentation_requested"] or args.gpu and modern["gpu"] != "exact_indices":
        raise ValueError("requested GPU or presentation effect comparison failed")
    commands.append({"argv": command, "result": modern})
    receipt = {"schema_version": 1, "status": "ORIGINAL_RESOURCE_CPU_GPU_PASS" if args.gpu else "ORIGINAL_RESOURCE_CPU_PASS",
               "original_game_process_executed": False, "build_receipt_sha256": sha(build_path),
               "cli_sha256": sha(cli), "frame_bytes": 76800, "palette_bytes": 1024,
               "frame_sha256": sha(native_frame), "palette_sha256": sha(native_palette),
               "rgba_sha256": hashlib.sha256(rgba).hexdigest(), "commands": commands,
               "scope": "Actual MENUB and MAINMENU CIF resources; controlled two-sprite chain, full indexed frame and palette equality. Opaque RGBA is modern presentation policy. No native TITLE producer, gameplay, host execution or complete renderer claim."}
    (output / "comparison.json").write_bytes((json.dumps(receipt, indent=2) + "\n").encode())
    print(json.dumps({"status": receipt["status"], "modern": modern}, indent=2))


if __name__ == "__main__":
    main()
