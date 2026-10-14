"""Build or launch the source-built windowed Hercules resource checkpoint."""
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
    parser.add_argument("--build", type=Path, default=ROOT / "work/game_checkpoint_build")
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--hidden", action="store_true")
    parser.add_argument("--screenshot", type=Path)
    args = parser.parse_args()
    build = _under_work(ROOT, args.build)
    archive = args.archive
    if archive is None:
        # A Git worktree can use the primary checkout's read-only originals.
        common = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=ROOT,
                                         text=True).strip()
        primary = (ROOT / common).resolve().parent
        archive = primary / "assets/pc/HERCULES/HERCULES.FS"
    archive = input_file(archive, "original game archive")
    if sha(archive) != "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4":
        raise ValueError("archive identity mismatch")
    if not build.exists():
        subprocess.run([sys.executable, "-B", str(ROOT / "dropin_renderer/tests/run_build.py"),
                        "--gpu", "--output", str(build)], cwd=ROOT, check=True)
    receipt = json.loads(input_file(build / "build_receipt.json", "verified build receipt").read_bytes())
    exe = input_file(build / "hercules_preview.exe", "source-built preview")
    if not receipt.get("passed") or receipt.get("fixture_executables", {}).get(exe.name) != sha(exe):
        raise ValueError("preview is not a verified build")
    for name, digest in receipt["source_sha256"].items():
        if sha(ROOT / name) != digest:
            raise ValueError("build source changed: " + name + "; select a fresh --build directory")
    manifest = json.loads((ROOT / "dropin_renderer/docs/local_toolchain.json").read_bytes())
    if sha(ROOT / "dropin_renderer/docs/local_toolchain.json") != receipt["toolchain_manifest_sha256"]:
        raise ValueError("build toolchain receipt changed")
    for component in manifest["files"]:
        path = Path(component["path"])
        if sha(input_file(path, "pinned local component")) != component["sha256"]:
            raise ValueError("component identity changed: " + str(path))
    env = os.environ.copy()
    env["PATH"] = manifest["compiler_bin"] + os.pathsep + env.get("PATH", "")
    command = [str(exe), "--archive", str(archive)]
    for enabled, switch in ((args.smoke, "--smoke"), (args.hidden, "--hidden")):
        if enabled:
            command.append(switch)
    if args.screenshot:
        command += ["--screenshot", str(_under_work(ROOT, args.screenshot))]
    return subprocess.run(command, cwd=ROOT, env=env, shell=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
