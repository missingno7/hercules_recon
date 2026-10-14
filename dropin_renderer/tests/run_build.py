"""Pinned local x86 CMake/CTest build; all products stay in a fresh work tree."""
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
from pathguard import _under_work, _no_reparse_components  # noqa: E402


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--gpu", action="store_true")
    args = parser.parse_args()
    output = _under_work(ROOT, args.output)
    if output.exists():
        raise ValueError("build output must be fresh")
    manifest_path = ROOT / "dropin_renderer/docs/local_toolchain.json"
    manifest = json.loads(manifest_path.read_bytes())
    for tool in manifest["files"]:
        if tool.get("optional_gpu") and not args.gpu:
            continue
        path = Path(tool["path"])
        _no_reparse_components(path)
        if not path.is_file() or sha(path) != tool["sha256"]:
            raise ValueError("local tool identity changed: " + str(path))
    source_paths = [path for directory in ("dropin_renderer", "renderer_contract")
                    for path in (ROOT / directory).rglob("*")
                    if path.is_file() and path.suffix in (".c", ".h", ".cpp", ".hpp", ".def", ".py")]
    source_paths += [ROOT / "dropin_renderer/CMakeLists.txt", ROOT / "renderer_contract/CMakeLists.txt"]
    sources = {str(path.relative_to(ROOT)).replace("\\", "/"): sha(path) for path in source_paths}
    tool_bin = Path(manifest["compiler_bin"])
    env = os.environ.copy()
    env["PATH"] = str(tool_bin) + os.pathsep + env.get("PATH", "")
    cmake = str(tool_bin / "cmake.exe")
    commands = [
        [cmake, "-S", str(ROOT / "dropin_renderer"), "-B", str(output),
         "-G", "MinGW Makefiles", "-DCMAKE_BUILD_TYPE=Release",
         "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
         "-DPython3_EXECUTABLE=" + str(Path(sys.executable).resolve()),
         "-DCMAKE_C_COMPILER=" + str(tool_bin / "gcc.exe"),
         "-DCMAKE_CXX_COMPILER=" + str(tool_bin / "g++.exe"),
         "-DCMAKE_MAKE_PROGRAM=" + str(tool_bin / "mingw32-make.exe"),
         "-DDROPIN_BUILD_SDL_GPU_DIAGNOSTIC=" + ("ON" if args.gpu else "OFF")],
        [cmake, "--build", str(output), "--parallel", "4"],
        [str(tool_bin / "ctest.exe"), "--test-dir", str(output), "--output-on-failure",
         "--output-junit", str(output / "tests.xml")],
    ]
    if args.gpu:
        commands[0].append("-DSDL3_DIR=" + manifest["sdl_cmake_dir"])
    output.mkdir(parents=True)
    receipt = {"schema_version": 1, "purpose": __doc__, "original_game_executed": False,
               "toolchain_manifest_sha256": sha(manifest_path), "source_sha256": sources,
               "commands": [], "passed": False}
    try:
        for index, command in enumerate(commands):
            result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True,
                                    text=True, errors="replace", timeout=180, shell=False)
            log = result.stdout + result.stderr
            (output / f"stage_{index}.log").write_bytes(log.encode())
            receipt["commands"].append({"argv": command, "returncode": result.returncode,
                                        "log_sha256": hashlib.sha256(log.encode()).hexdigest()})
            print(log, end="", flush=True)
            if result.returncode:
                return result.returncode
        if any(sha(ROOT / path) != digest for path, digest in sources.items()):
            raise ValueError("source changed during build verification")
        sys.path.insert(0, str(ROOT / "scripts"))
        import env as analysis_environment
        import pefile
        image_path = output / "ddraw.dll"
        image = pefile.PE(str(image_path))
        exported_names = [symbol.name.decode("ascii") for symbol in image.DIRECTORY_ENTRY_EXPORT.symbols]
        imported_names = [entry.dll.decode("ascii").lower() for entry in image.DIRECTORY_ENTRY_IMPORT]
        if (image.FILE_HEADER.Machine != 0x14c or image.OPTIONAL_HEADER.Magic != 0x10b
                or exported_names != ["DirectDrawCreate"]
                or set(imported_names) - {"kernel32.dll", "msvcrt.dll"}):
            raise ValueError("production proxy PE32/export/runtime boundary failed")
        receipt["proxy_image"] = {"sha256": sha(image_path), "machine": "i386",
                                  "exports": exported_names, "imports": imported_names}
        receipt["fixture_executables"] = {path.name: sha(path) for path in output.glob("*.exe")}
        receipt["passed"] = True
        return 0
    finally:
        (output / "build_receipt.json").write_bytes((json.dumps(receipt, indent=2) + "\n").encode())


if __name__ == "__main__":
    raise SystemExit(main())
