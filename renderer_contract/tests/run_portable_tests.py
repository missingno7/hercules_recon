#!/usr/bin/env python3
"""Build and run the standalone renderer-contract C/C++ fixture package.

The package contains only fixed-width C/C++ utility code and synthetic tests.
No repository scripts, game executable, extracted game files, or Python
packages are used. Compiler products and the JSON receipt stay in --output.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


EXPECTED_TESTS = {
    "test_cpp_headers.cpp",
    "test_edge_steps.c",
    "test_frame.c",
    "test_host_abi.c",
    "test_packets.c",
    "test_pixels.c",
    "test_quad_route.c",
    "test_sprite_geometry.c",
    "test_sprite_pixels.c",
    "test_staging.c",
    "test_state.c",
    "test_textures.c",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def resolve_tool(value: str) -> Path:
    candidate = Path(value).expanduser()
    if candidate.parent != Path(".") or candidate.is_absolute():
        resolved = candidate.resolve()
    else:
        located = shutil.which(value)
        if located is None:
            raise FileNotFoundError("compiler not found on PATH: " + value)
        resolved = Path(located).resolve()
    if not resolved.is_file():
        raise FileNotFoundError("compiler executable does not exist: " + str(resolved))
    return resolved


def compiler_kind(path: Path) -> str:
    name = path.name.lower()
    if name in ("cl", "cl.exe"):
        return "msvc"
    return "gnu"


def run_command(argv: list[str], cwd: Path, env: dict[str, str], timeout: int) -> dict:
    try:
        result = subprocess.run(
            argv,
            cwd=str(cwd),
            env=env,
            shell=False,
            capture_output=True,
            text=True,
            errors="replace",
            timeout=timeout,
        )
        return {
            "argv": argv,
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "timed_out": False,
        }
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode(errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode(errors="replace")
        return {
            "argv": argv,
            "returncode": None,
            "stdout": stdout,
            "stderr": stderr,
            "timed_out": True,
        }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--package",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="renderer_contract directory (default: this script's package parent)",
    )
    parser.add_argument("--cc", required=True, help="C compiler executable (for example gcc.exe or cl.exe)")
    parser.add_argument("--cxx", required=True, help="C++ compiler executable (for example g++.exe or cl.exe)")
    parser.add_argument("--output", required=True, type=Path, help="new or empty directory for all build outputs and receipt")
    parser.add_argument("--timeout", type=int, default=60, help="per-command timeout in seconds (default: 60)")
    args = parser.parse_args()

    package = args.package.expanduser().resolve()
    output = args.output.expanduser().resolve()
    receipt_path = output / "portable_tests.json"
    started = time.time()
    receipt: dict = {
        "schema_version": 1,
        "purpose": "standalone synthetic renderer-contract utility fixtures; no native-equivalence claim",
        "original_game_executed": False,
        "started_unix": started,
        "package_dir": str(package),
        "output_dir": str(output),
        "receipt_path": str(receipt_path),
        "standards": {
            "gnu_c": "-std=c89 -pedantic-errors",
            "gnu_cxx": "-std=c++98 -pedantic-errors",
            "msvc_c": "MSVC C default (C89-compatible subset) with /Za",
            "msvc_cxx": "MSVC C++ mode with /Za; fixtures use the C++98 subset",
        },
        "compilers": {},
        "source_hashes": [],
        "common_c_objects": [],
        "tests": [],
        "errors": [],
    }
    output_is_safe = False

    def save_receipt() -> None:
        if not output_is_safe:
            raise RuntimeError("refusing to write a receipt before --output is validated")
        output.mkdir(parents=True, exist_ok=True)
        receipt["finished_unix"] = time.time()
        receipt["summary"] = {
            "discovered": len(receipt["tests"]),
            "passed": sum(1 for row in receipt["tests"] if row.get("status") == "passed"),
            "failed": sum(1 for row in receipt["tests"] if row.get("status") != "passed"),
        }
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")

    try:
        if args.timeout <= 0:
            raise ValueError("--timeout must be positive")
        if not package.is_dir():
            raise FileNotFoundError("package directory does not exist: " + str(package))
        if output == package or package in output.parents:
            raise ValueError("--output must not be the package directory or a directory inside it")
        if output.exists() and any(output.iterdir()):
            raise FileExistsError("--output must be a new or empty directory: " + str(output))
        output_is_safe = True

        cc = resolve_tool(args.cc)
        cxx = resolve_tool(args.cxx)
        cc_kind = compiler_kind(cc)
        cxx_kind = compiler_kind(cxx)
        if cc_kind != cxx_kind:
            raise ValueError("--cc and --cxx must both be GCC/compatible or both be MSVC cl.exe")
        if cc_kind == "msvc":
            version_argv = [[str(cc), "/?"], [str(cxx), "/?"]]
        else:
            version_argv = [[str(cc), "--version"], [str(cxx), "--version"]]

        output.mkdir(parents=True, exist_ok=True)
        temp_root = output / "tmp"
        temp_root.mkdir()
        env = os.environ.copy()
        env["PATH"] = os.pathsep.join(
            part for part in (str(cc.parent), str(cxx.parent), env.get("PATH", "")) if part
        )
        for key in ("TMP", "TEMP", "TMPDIR"):
            env[key] = str(temp_root)
        if cc_kind == "msvc":
            for key in ("CL", "_CL_", "LINK", "_LINK_"):
                env.pop(key, None)

        for label, tool, argv in (("cc", cc, version_argv[0]), ("cxx", cxx, version_argv[1])):
            version = run_command(argv, output, env, args.timeout)
            receipt["compilers"][label] = {
                "path": str(tool),
                "sha256": sha256(tool),
                "kind": cc_kind,
                "version_probe": version,
            }
            if version["timed_out"] or (version["returncode"] not in (0, None) and cc_kind != "msvc"):
                receipt["errors"].append(label + " version probe failed")

        sources = sorted(
            path for path in package.rglob("*")
            if path.is_file() and path.suffix.lower() in (".c", ".h", ".cpp")
        )
        c_sources = sorted(path for path in package.glob("*.c") if path.is_file())
        test_dir = package / "tests"
        tests = sorted(
            path for pattern in ("test_*.c", "test_*.cpp")
            for path in test_dir.glob(pattern) if path.is_file()
        ) if test_dir.is_dir() else []
        actual_test_names = {path.name for path in tests}
        if actual_test_names != EXPECTED_TESTS:
            missing = sorted(EXPECTED_TESTS - actual_test_names)
            unexpected = sorted(actual_test_names - EXPECTED_TESTS)
            raise ValueError("fixture inventory mismatch; missing=" + repr(missing) + ", unexpected=" + repr(unexpected))
        if not c_sources:
            raise FileNotFoundError("no C implementation sources found at package root")
        receipt["source_hashes"] = [
            {"path": path.relative_to(package).as_posix(), "sha256": sha256(path)}
            for path in sources
        ]

        common_dir = output / "common"
        common_dir.mkdir()
        common_objects: list[Path] = []
        for index, source in enumerate(c_sources):
            object_path = common_dir / ("%02d_%s.obj" % (index, source.stem) if cc_kind == "msvc" else "%02d_%s.o" % (index, source.stem))
            if cc_kind == "msvc":
                command = [str(cc), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TC", "/I" + str(package), "/c", str(source), "/Fo" + str(object_path)]
            else:
                command = [str(cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra", "-I", str(package), "-c", str(source), "-o", str(object_path)]
            row = {"source": source.relative_to(package).as_posix(), "source_sha256": sha256(source), "command": None}
            result = run_command(command, output, env, args.timeout)
            row["command"] = result
            row["object"] = str(object_path)
            if result["returncode"] != 0 or result["timed_out"] or not object_path.is_file():
                receipt["common_c_objects"].append(row)
                raise RuntimeError("failed compiling package C source: " + str(source.name))
            row["object_sha256"] = sha256(object_path)
            receipt["common_c_objects"].append(row)
            common_objects.append(object_path)

        for index, test_source in enumerate(tests):
            test_dir_out = output / ("%02d_%s" % (index, test_source.stem))
            test_dir_out.mkdir()
            object_suffix = ".obj" if cc_kind == "msvc" else ".o"
            test_object = test_dir_out / (test_source.stem + object_suffix)
            executable = test_dir_out / (test_source.stem + (".exe" if os.name == "nt" else ""))
            if test_source.suffix.lower() == ".cpp":
                compiler = cxx
                if cxx_kind == "msvc":
                    compile_argv = [str(compiler), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TP", "/I" + str(package), "/c", str(test_source), "/Fo" + str(test_object)]
                    link_argv = [str(cxx), "/nologo", "/Fe" + str(executable), str(test_object), *(str(obj) for obj in common_objects)]
                else:
                    compile_argv = [str(compiler), "-std=c++98", "-pedantic-errors", "-Wall", "-Wextra", "-I", str(package), "-c", str(test_source), "-o", str(test_object)]
                    link_argv = [str(cxx), "-o", str(executable), str(test_object), *(str(obj) for obj in common_objects)]
            else:
                compiler = cc
                if cc_kind == "msvc":
                    compile_argv = [str(compiler), "/nologo", "/W3", "/O2", "/MT", "/Za", "/TC", "/I" + str(package), "/c", str(test_source), "/Fo" + str(test_object)]
                    link_argv = [str(cc), "/nologo", "/Fe" + str(executable), str(test_object), *(str(obj) for obj in common_objects)]
                else:
                    compile_argv = [str(compiler), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra", "-I", str(package), "-c", str(test_source), "-o", str(test_object)]
                    link_argv = [str(cc), "-o", str(executable), str(test_object), *(str(obj) for obj in common_objects)]

            row = {
                "test": test_source.relative_to(package).as_posix(),
                "source_sha256": sha256(test_source),
                "output_dir": str(test_dir_out),
                "executable": str(executable),
                "compile": None,
                "link": None,
                "run": None,
            }
            receipt["tests"].append(row)
            compile_result = run_command(compile_argv, test_dir_out, env, args.timeout)
            row["compile"] = compile_result
            if compile_result["returncode"] != 0 or compile_result["timed_out"] or not test_object.is_file():
                row["status"] = "compile_failed"
                continue
            row["test_object_sha256"] = sha256(test_object)

            link_result = run_command(link_argv, test_dir_out, env, args.timeout)
            row["link"] = link_result
            if link_result["returncode"] != 0 or link_result["timed_out"] or not executable.is_file():
                row["status"] = "link_failed"
                continue
            row["executable_sha256"] = sha256(executable)

            run_result = run_command([str(executable)], test_dir_out, env, args.timeout)
            row["run"] = run_result
            row["status"] = "passed" if run_result["returncode"] == 0 and not run_result["timed_out"] else "run_failed"

        save_receipt()
        print(json.dumps(receipt["summary"], sort_keys=True))
        print("receipt: " + str(receipt_path))
        return 0 if receipt["summary"]["failed"] == 0 and not receipt["errors"] else 1
    except Exception as exc:
        receipt["errors"].append(type(exc).__name__ + ": " + str(exc))
        try:
            if not output_is_safe:
                raise RuntimeError("--output is not a validated destination")
            save_receipt()
            print("receipt: " + str(receipt_path), file=sys.stderr)
        except Exception as save_exc:
            print("could not write receipt: " + str(save_exc), file=sys.stderr)
        print(type(exc).__name__ + ": " + str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
