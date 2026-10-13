"""Pinned, read-only support for the private original-code renderer oracles."""
from __future__ import annotations

import argparse
import hashlib
import os
import sys
from pathlib import Path

EXPECTED_EXE_SHA256 = (
    "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
)
EXPECTED_UNICORN_VERSION = "2.1.4"
EXPECTED_UNICORN_DLL_SHA256 = (
    "010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1"
)
DEFAULT_DEPS = Path(r"C:\tools\hercules\renderer-oracle-python")
DEFAULT_ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_args(description: str, default_receipt_name: str):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--exe", required=True, type=Path, help="pinned original HERCULES.EXE")
    parser.add_argument(
        "--original-root",
        type=Path,
        help="repository root containing scripts/pe.py and work/ (inferred from --exe if omitted)",
    )
    parser.add_argument(
        "--deps", type=Path, default=DEFAULT_DEPS,
        help=f"Unicorn 2.1.4 target install (default: {DEFAULT_DEPS})",
    )
    parser.add_argument(
        "--analysis-deps", type=Path, default=DEFAULT_ANALYSIS_DEPS,
        help=f"existing pefile dependency directory (default: {DEFAULT_ANALYSIS_DEPS})",
    )
    parser.add_argument(
        "--receipt", type=Path,
        help=f"ignored-work output path (default: work/renderer_bounded_oracle/{default_receipt_name})",
    )
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    if args.original_root is None:
        args.original_root = Path(__file__).resolve().parents[2]
    else:
        args.original_root = args.original_root.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if args.receipt is None:
        args.receipt = (
            args.original_root / "work" / "renderer_bounded_oracle" / default_receipt_name
        )
    else:
        args.receipt = args.receipt.resolve()
    try:
        args.receipt.resolve().relative_to((args.original_root / "work").resolve())
    except ValueError:
        parser.error("--receipt must stay inside the repository's ignored work directory")
    if sha256(args.exe.read_bytes()) != EXPECTED_EXE_SHA256:
        parser.error("--exe does not match the pinned original HERCULES.EXE SHA-256")
    if not args.deps.is_dir():
        parser.error(f"Unicorn dependency directory does not exist: {args.deps}")
    if not args.analysis_deps.is_dir():
        parser.error(f"analysis dependency directory does not exist: {args.analysis_deps}")

    scripts = args.original_root / "scripts"
    if not (scripts / "pe.py").is_file():
        parser.error(f"cannot find scripts/pe.py under --original-root: {args.original_root}")
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    sys.path[:0] = [str(args.deps), str(scripts)]
    return args


def verify_runtime(unicorn_module, expected_dependency_root: Path | None = None) -> dict:
    if unicorn_module.__version__ != EXPECTED_UNICORN_VERSION:
        raise RuntimeError(
            f"expected Unicorn {EXPECTED_UNICORN_VERSION}, found {unicorn_module.__version__}"
        )
    module_path = Path(unicorn_module.__file__).resolve()
    if expected_dependency_root is not None:
        try:
            module_path.relative_to(expected_dependency_root.resolve())
        except ValueError as error:
            raise RuntimeError(
                f"Unicorn module did not load from the pinned dependency root: {module_path}"
            ) from error
    dll_path = module_path.parent / "lib" / "unicorn.dll"
    dll_hash = sha256(dll_path.read_bytes())
    if dll_hash != EXPECTED_UNICORN_DLL_SHA256:
        raise RuntimeError(f"unexpected Unicorn native DLL SHA-256: {dll_hash}")
    return {
        "engine": "Unicorn CPU emulator",
        "unicorn_version": unicorn_module.__version__,
        "unicorn_module_path": str(module_path),
        "native_dll_path": str(dll_path),
        "native_dll_sha256": dll_hash,
        "dependency_origin": {
            "package_page": "https://pypi.org/project/unicorn/2.1.4/",
            "wheel_url": "https://files.pythonhosted.org/packages/70/3d/de7be9bd1addabe6d8a1369381f8a080400c349850e978689c5e18287957/unicorn-2.1.4-cp37-abi3-win_amd64.whl",
            "wheel_sha256_expected_and_downloaded": "d7107500c64ce5c168fbff6bef9485b5db1350050036f4cea568650cf8bdbdf5",
        },
    }


def write_receipt(path: Path, receipt: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(__import__("json").dumps(receipt, indent=2) + "\n", encoding="utf-8")
