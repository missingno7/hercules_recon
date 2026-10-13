#!/usr/bin/env python3
"""Build and run renderer-contract C tests with the pinned VC5 toolchain.

All compiler products and process temporary files stay under
work/renderer_contract_tests. The package directory is supplied by the caller;
tests default to the sibling ``tests/test_*.c`` directory.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import traceback


ROOT = Path(__file__).resolve().parents[2]
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
OUTPUT_ROOT = ROOT / "work" / "renderer_contract_tests"
REPORT_PATH = OUTPUT_ROOT / "verification.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_extra(value: str) -> tuple[str | None, Path]:
    """Parse SOURCE or TEST=SOURCE; TEST matches the test filename or stem."""
    if "=" in value:
        test_name, source_text = value.split("=", 1)
        if not test_name or not source_text:
            raise argparse.ArgumentTypeError("expected TEST=SOURCE")
        return test_name, Path(source_text).expanduser().resolve()
    return None, Path(value).expanduser().resolve()


def timeout_text(value: bytes | str | None) -> str:
    if value is None:
        return ""
    return value.decode(errors="replace") if isinstance(value, bytes) else value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "package", nargs="?", type=Path,
        help="directory containing package C sources (default: this script's parent, or repo/renderer_contract)",
    )
    parser.add_argument(
        "--tests-dir", type=Path,
        help="test directory (default: package parent/tests)",
    )
    parser.add_argument(
        "--extra-source", action="append", default=[], type=parse_extra, metavar="[TEST=]SOURCE",
        help="additional C source to link; omit TEST to apply to all tests, or use a test filename/stem",
    )
    parser.add_argument("--timeout", type=int, default=60, help="per-test timeout in seconds")
    args = parser.parse_args()

    if args.package:
        package = args.package.expanduser().resolve()
    else:
        script_parent = Path(__file__).resolve().parents[1]
        fallback = ROOT / "renderer_contract"
        package = script_parent if list(script_parent.glob("*.c")) else fallback
    report_path = REPORT_PATH.resolve()
    if args.tests_dir:
        tests_dir = args.tests_dir.expanduser().resolve()
    else:
        nested_tests = package / "tests"
        tests_dir = (nested_tests if nested_tests.is_dir() else package.parent / "tests").resolve()
    output_root = OUTPUT_ROOT.resolve()
    started = time.time()

    receipt: dict = {
        "schema_version": 1,
        "purpose": "private renderer-contract package fixture build and execution",
        "original_exe_executed": False,
        "started_unix": started,
        "repository_root": str(ROOT),
        "package_dir": str(package),
        "tests_dir": str(tests_dir),
        "output_root": str(output_root),
        "report_path": str(report_path),
        "toolchain": str(TOOLCHAIN),
        "toolchain_id": None,
        "compiler_sha256": None,
        "fixed_flags": ["/nologo", "/W3", "/O2", "/MT"],
        "extra_sources": [],
        "tests": [],
        "errors": [],
    }

    def save_report() -> None:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        receipt["finished_unix"] = time.time()
        report_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")

    try:
        if args.timeout <= 0:
            raise ValueError("--timeout must be positive")
        if not package.is_dir():
            raise FileNotFoundError(f"package directory does not exist: {package}")
        if not tests_dir.is_dir():
            raise FileNotFoundError(f"tests directory does not exist: {tests_dir}")

        package_sources = sorted(p.resolve() for p in package.glob("*.c") if p.is_file())
        test_sources = sorted(p.resolve() for pattern in ("test_*.c", "test_*.cpp")
                              for p in tests_dir.glob(pattern) if p.is_file())
        if not package_sources:
            raise FileNotFoundError(f"no package/*.c source files found in {package}")
        if not test_sources:
            raise FileNotFoundError(f"no tests/test_*.c source files found in {tests_dir}")

        for test_name, source in args.extra_source:
            if not source.is_file():
                raise FileNotFoundError(f"optional extra source does not exist: {source}")
            if test_name and test_name not in {
                value for test in test_sources for value in (test.name, test.stem)
            }:
                raise ValueError(f"optional extra source names no discovered test: {test_name}")
            receipt["extra_sources"].append({"test": test_name, "path": str(source), "sha256": sha256(source)})

        sys.path.insert(0, str(ROOT / "scripts"))
        match = importlib.import_module("match")
        receipt["toolchain_id"] = match.verify_toolchain(TOOLCHAIN)
        compiler = TOOLCHAIN / "bin" / "cl.exe"
        if not compiler.is_file():
            raise FileNotFoundError(f"pinned compiler is missing: {compiler}")
        receipt["compiler_sha256"] = sha256(compiler)
        receipt["package_sources"] = [
            {"path": str(source), "sha256": sha256(source)} for source in package_sources
        ]
        receipt["package_headers"] = [
            {"path": str(header), "sha256": sha256(header)}
            for header in sorted(package.glob("*.h"))
        ]
        receipt["tests_discovered"] = [
            {"path": str(source), "sha256": sha256(source)} for source in test_sources
        ]

        output_root.mkdir(parents=True, exist_ok=True)
        env_base = os.environ.copy()
        env_base.pop("CL", None)
        env_base.pop("_CL_", None)
        env_base.pop("LINK", None)
        env_base.pop("_LINK_", None)
        env_base["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env_base.get("PATH", "")
        env_base["INCLUDE"] = str(TOOLCHAIN / "include")
        env_base["LIB"] = str(TOOLCHAIN / "lib")

        failed = False
        for index, test_source in enumerate(test_sources):
            test_out = output_root / f"{index:03d}_{test_source.stem}"
            temp_dir = test_out / "tmp"
            test_out.mkdir(parents=True, exist_ok=True)
            temp_dir.mkdir(parents=True, exist_ok=True)
            exe = test_out / f"{test_source.stem}.exe"
            applicable_extras = [
                source for test_name, source in args.extra_source
                if test_name is None or test_name in {test_source.name, test_source.stem}
            ]
            sources = [test_source, *package_sources, *applicable_extras]
            before_hashes = {str(source): sha256(source) for source in sources}
            command = [
                str(compiler), "/nologo", "/W3", "/O2", "/MT",
                "/I" + str(package), "/I" + str(tests_dir),
                "/Fe" + str(exe), *(str(source) for source in sources),
            ]
            env = env_base.copy()
            env["TEMP"] = str(temp_dir)
            env["TMP"] = str(temp_dir)
            row = {
                "test": str(test_source),
                "output_dir": str(test_out),
                "exe": str(exe),
                "sources": [{"path": str(source), "sha256": before_hashes[str(source)]}
                            for source in sources],
                "command_argv": command,
                "command_display": subprocess.list2cmdline(command),
                "environment": {"TEMP": str(temp_dir), "TMP": str(temp_dir),
                                "INCLUDE": env["INCLUDE"], "LIB": env["LIB"]},
            }
            receipt["tests"].append(row)
            try:
                build = subprocess.run(
                    command, env=env, cwd=test_out, capture_output=True, text=True,
                    errors="replace", timeout=args.timeout,
                )
                row["build"] = {
                    "returncode": build.returncode,
                    "stdout": build.stdout,
                    "stderr": build.stderr,
                }
                if build.returncode != 0 or not exe.is_file():
                    row["status"] = "build_failed"
                    failed = True
                else:
                    row["exe_sha256"] = sha256(exe)
                    try:
                        run = subprocess.run(
                            [str(exe)], env=env, cwd=test_out, capture_output=True,
                            text=True, errors="replace", timeout=args.timeout,
                        )
                        row["run"] = {
                            "command_argv": [str(exe)],
                            "command_display": subprocess.list2cmdline([str(exe)]),
                            "returncode": run.returncode,
                            "stdout": run.stdout,
                            "stderr": run.stderr,
                        }
                        row["status"] = "passed" if run.returncode == 0 else "test_failed"
                        failed |= run.returncode != 0
                    except subprocess.TimeoutExpired as exc:
                        row["status"] = "test_timeout"
                        row["run"] = {"timeout_seconds": args.timeout,
                                      "stdout": timeout_text(exc.stdout),
                                      "stderr": timeout_text(exc.stderr)}
                        failed = True
                    except OSError as exc:
                        row["status"] = "test_launch_failed"
                        row["run"] = {"launch_error_type": type(exc).__name__,
                                      "launch_error": str(exc)}
                        failed = True
            except subprocess.TimeoutExpired as exc:
                row["status"] = "build_timeout"
                row["build"] = {"timeout_seconds": args.timeout,
                                 "stdout": timeout_text(exc.stdout),
                                 "stderr": timeout_text(exc.stderr)}
                failed = True
            except OSError as exc:
                row["status"] = "compiler_launch_failed"
                row["build"] = {"launch_error_type": type(exc).__name__,
                                 "launch_error": str(exc)}
                failed = True
            row["source_hashes_after"] = {
                source: sha256(Path(source)) for source in before_hashes
            }
            if any(row["source_hashes_after"][source] != digest
                   for source, digest in before_hashes.items()):
                row["status"] = "source_changed_during_build"
                failed = True
        receipt["summary"] = {
            "discovered": len(test_sources),
            "passed": sum(row.get("status") == "passed" for row in receipt["tests"]),
            "failed": sum(row.get("status") != "passed" for row in receipt["tests"]),
        }
        save_report()
        print(json.dumps(receipt["summary"]))
        for row in receipt["tests"]:
            print(Path(row["test"]).name + ": " + row["status"])
            if "run" in row and isinstance(row["run"].get("stdout"), str):
                lines = row["run"]["stdout"].strip().splitlines()
                if lines:
                    print(lines[-1])
        return 1 if failed else 0
    except Exception as exc:
        receipt["errors"].append({"type": type(exc).__name__, "message": str(exc)})
        receipt["traceback"] = traceback.format_exc()
        save_report()
        print(f"package build runner failed: {type(exc).__name__}: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
