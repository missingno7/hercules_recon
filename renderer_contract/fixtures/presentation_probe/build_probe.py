"""Build the bounded presentation fixture; system DirectDraw needs --run-api.

The default runs source-built synthetic tests only. It never starts Hercules
or calls the operating system's DirectDraw export.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

from path_guard import (UnsafePathError, guard_work_output,
                        require_no_redirects)

DEFAULT_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def source_inventory(package: Path, contract: Path) -> dict[str, str]:
    paths = [path for path in package.rglob("*") if path.is_file() and
             path.suffix.lower() in (".py", ".c", ".h", ".def")]
    paths.extend((contract / name for name in (
        "hercules_host_abi.c", "hercules_host_abi.h", "hercules_types.h")))
    result = {}
    for path in paths:
        resolved = path.resolve(strict=True)
        if not resolved.is_relative_to(package.resolve()):
            if not resolved.is_relative_to(contract.resolve()):
                raise ValueError("source input escapes fixture and contract roots")
        key = str(path.relative_to(package)) if path.is_relative_to(package) else \
            "renderer_contract/" + path.name
        result[key] = sha256(resolved)
    return dict(sorted(result.items()))


def parse_receipt(path: Path) -> dict:
    if not path.is_file():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--toolchain-dir", type=Path, default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--run-api", action="store_true",
                        help="explicitly call real DirectDrawCreate once and Release any returned object")
    parser.add_argument("--timeout-seconds", type=int, default=10,
                        help="short source-built API-process timeout (1 through 20 seconds)")
    args = parser.parse_args()
    if args.timeout_seconds < 1 or args.timeout_seconds > 20:
        parser.error("--timeout-seconds must be between 1 and 20")

    try:
        root, work, output = guard_work_output(args.repo_root,
                                                args.output_dir)
    except UnsafePathError as exc:
        parser.error(str(exc))
    package = Path(__file__).resolve().parent
    contract = root / "renderer_contract"
    try:
        package = require_no_redirects(package)
    except (UnsafePathError, OSError) as exc:
        parser.error("fixture source path is redirected: " + str(exc))
    canonical_fixture = contract / "fixtures" / "presentation_probe"
    staged_fixture = package.is_relative_to(work) and tuple(package.parts[-3:]) == (
        "renderer_contract", "fixtures", "presentation_probe")
    if package != canonical_fixture.resolve() and not staged_fixture:
        parser.error("builder must run from renderer_contract/fixtures/presentation_probe")
    mode = "system_api" if args.run_api else "synthetic"
    helper = package / ("build_system_probe.py" if args.run_api else
                        "build_synthetic.py")
    helper_output = output / mode
    helper_output_arg = Path(os.path.abspath(helper_output))
    if not helper_output_arg.is_relative_to(work):
        parser.error("helper output escaped repository work directory")
    if helper_output.exists() and any(helper_output.iterdir()):
        parser.error("mode output must be new or empty")

    command = [sys.executable, "-B", str(helper),
               "--repo-root", str(root),
               "--output-dir", str(helper_output),
               "--toolchain-dir", str(args.toolchain_dir.resolve())]
    if args.run_api:
        command.extend(("--run-api", "--timeout-seconds",
                        str(args.timeout_seconds)))
    try:
        completed = subprocess.run(
            command, cwd=str(root), capture_output=True, text=True,
            errors="replace", timeout=360, check=False)
        helper_stdout = completed.stdout
        helper_stderr = completed.stderr
        helper_exit_code = completed.returncode
        helper_timed_out = False
    except subprocess.TimeoutExpired as exc:
        helper_stdout = exc.stdout.decode(errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
        helper_stderr = exc.stderr.decode(errors="replace") if isinstance(exc.stderr, bytes) else (exc.stderr or "")
        helper_exit_code = None
        helper_timed_out = True

    receipt_path = helper_output / ("system_probe_receipt.json" if args.run_api else
                                    "build_receipt.json")
    nested = parse_receipt(receipt_path)
    if helper_timed_out:
        final_status = "BUILDER_TIMEOUT"
    elif args.run_api:
        final_status = nested.get("final_status", "SYSTEM_API_RECEIPT_MISSING")
    else:
        final_status = ("SYNTHETIC_PASS" if helper_exit_code == 0 and
                        nested.get("smoke_result") == "PASS" else
                        "SYNTHETIC_BUILD_OR_SMOKE_FAILED")

    receipt = {
        "schema": "hercules-presentation-probe-public-receipt-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "mode": mode,
        "final_status": final_status,
        "original_hercules_forwarding": "UNRESOLVED",
        "scope": ("Explicit bounded system API probe; no original host or engine process, window, cooperative-level, surface, palette, display mode, or gameplay." if args.run_api else
                  "Source-built synthetic ABI/guard smoke only; no real DirectDraw API, original host, or engine process."),
        "repo_root": str(root),
        "work_root": str(work),
        "output_root": str(output),
        "output_path_guard": {
            "lexical_path": str(output_lexical := Path(os.path.abspath(
                args.output_dir if args.output_dir.is_absolute() else
                root / args.output_dir))),
            "resolved_path": str(output),
            "lexical_under_work": output_lexical.is_relative_to(work),
            "resolved_under_work": output.is_relative_to(work),
            "symlink_or_junction_components_rejected": True,
            "accepted_state": "new or empty directory only",
        },
        "helper": {
            "path": str(helper),
            "sha256": sha256(helper),
            "argv": command,
            "cwd": str(root),
            "exit_code": helper_exit_code,
            "timeout_seconds": 360,
            "timed_out": helper_timed_out,
            "stdout": helper_stdout,
            "stderr": helper_stderr,
            "receipt_path": str(receipt_path),
            "receipt_sha256": sha256(receipt_path) if receipt_path.is_file() else None,
        },
        "source_sha256": source_inventory(package, contract),
        "runtime": {
            "python_version": sys.version,
            "python_executable": sys.executable,
            "python_executable_sha256": sha256(Path(sys.executable)),
            "platform": platform.platform(),
        },
        "toolchain": nested.get("toolchain"),
        "system_environment_pin": nested.get("environment_pin") if args.run_api else None,
        "system_dll_sha256_after": nested.get("system_dll_sha256_after") if args.run_api else None,
        "same_process_identity_observed": nested.get("same_process_identity_observed", False) if args.run_api else False,
        "helper_receipt": nested,
    }
    receipt_file = output / "presentation_probe_receipt.json"
    receipt_file.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n",
                            encoding="utf-8")
    print("RESULT=" + final_status)
    print("RECEIPT=" + str(receipt_file))
    if helper_stdout:
        print(helper_stdout, end="")
    if helper_stderr:
        print("HELPER_STDERR=" + helper_stderr, file=sys.stderr)
    return 3 if helper_timed_out or helper_exit_code not in (0, None) or \
        final_status in ("SYSTEM_API_RECEIPT_MISSING",
                         "SYNTHETIC_BUILD_OR_SMOKE_FAILED") else 0


if __name__ == "__main__":
    raise SystemExit(main())
