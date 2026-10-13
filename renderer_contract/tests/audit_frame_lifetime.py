#!/usr/bin/env python3
"""Verify original image identities and selected frame-lifecycle spans.

This is a static hash validator. It does not execute the original binaries or
re-disassemble them; the byte-level interpretation is documented in the paired
frame_lifetime.md report.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


EXPECTED_MODULES = {
    "HERCULES.EXE": ("587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d", "0x400000"),
    "ENG1.DLL": ("01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757", "0x10000000"),
    "ENG3.DLL": ("aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7", "0x10000000"),
    "TITLE.DLL": ("b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b", "0x10000000"),
}

# 23 complete or explicitly bounded original spans. Values originate from
# renderer_contract/evidence/frame.json and the new 5600/1d4e0/4660 pins.
SPANS = [
    ("HERCULES.EXE", "host_chain_initializer_AC20", 0xAC20, 0x20, "f96f2d7ab105a7bf13f0eaf9738ea9919ce0a8e6734f82eb15f5067938396a09"),
    ("HERCULES.EXE", "host_order_table_traversal_AC60", 0xAC60, 0x690, "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af"),
    ("ENG1.DLL", "bootstrap_allocation_requests", 0x1F0F0, 0x130, "3873fe0120fd045ed084e40648c9cd3a7937eb425ae6b0b66e5739e006dcb3e3"),
    ("ENG1.DLL", "bootstrap_cleanup", 0x1F220, 0x100, "ee33bfc33aed913f73786391a67d2bb4936362568c77ba0ea482419d3881b925"),
    ("ENG1.DLL", "paired_view_setup", 0x1F320, 0x380, "75dd4af37b63815680219a31a6be97ec35d64858fbba2a81cde40aaac6ddb52e"),
    ("ENG1.DLL", "per_frame_OT_and_arena_cursor_reset", 0x5600, 0x60, "77cdf850d931dce33e05476aaccfe4a563f34a74da2c70964ba8a423b6d0aad3"),
    ("ENG1.DLL", "full_frame_dispatch", 0x20300, 0x370, "71f7f168b63e75e99fedb3f292420de5aa7682b328ef9d9eb527939ac8b86779"),
    ("ENG1.DLL", "chain_init_wrapper", 0x2C600, 0x20, "5da8d177d24e5b78b2257bdf280a41888383225e733fd31dbe1ee034590e0bb9"),
    ("ENG1.DLL", "submit_wrapper", 0x2C620, 0x10, "56974b1cae70836a100d47ef3cde2793206aaae329bd71894283ed9dbbfbc1bb"),
    ("ENG3.DLL", "frame_and_arena_allocation_requests", 0x1A330, 0x127, "9e6922fd2fca4cf55f17c3db8b17d7ad4c3a0991ebde689c472e5b3927fb8ade"),
    ("ENG3.DLL", "release_and_reset", 0x1A460, 0xFD, "142b00e4e00535664a5072b5f79dbdd94a1a68adf086e8d95690644c7b6ff560"),
    ("ENG3.DLL", "pair_setup_and_arena_cursor_selection", 0x1A560, 0x383, "d45a79d562c672ff849d2b868c5fbc2a7cfbfdb47e44eafd34656e301af6ab64"),
    ("ENG3.DLL", "active_view_OT_and_arena_cursor_reset", 0xA700, 0x58, "4617375e1f8e2f8e654d3963036aed55272204ec348670b3b76bd7488251f2b8"),
    ("ENG3.DLL", "frame_submit_swap_loop", 0x1ABB0, 0x351, "774b9c49e904780c1c7e7699bb07801b67a1cda9b49f71538470371352819906"),
    ("ENG3.DLL", "chain_init_wrapper", 0x1FC10, 0x20, "6a7a4bacb895ab303c27eb3c75cfa9ab17d9835e2c86ec03c0ea73a0fc04606d"),
    ("ENG3.DLL", "submit_wrapper", 0x1FC30, 0x10, "60df4fbb3ef8da7836d7cc270fae184e9ec6da51fddc6eeffb1f8955f304214e"),
    ("TITLE.DLL", "frame_and_arena_allocation_requests", 0x1D380, 0x160, "3915f8ca42cc84cf1a2692c92c284ded48333becc966aceaadfad109fc80f6d5"),
    ("TITLE.DLL", "cleanup_and_bulk_release_neighborhood", 0x1D4E0, 0xB0, "427ba7798032f70151f59acf5327528e18658625e3ee936f08804972b1b16601"),
    ("TITLE.DLL", "paired_view_setup_and_pool_seed", 0x1D590, 0x1E0, "60badfca1f7228464fa69e73ead4131064db27d072551be8bc60f7d45749691c"),
    ("TITLE.DLL", "per_frame_OT_and_arena_cursor_reset", 0x4660, 0x50, "5f92b5e22a98fe9a8e1218b25eb971d911d6000c0ffec38de48a0d85c99e2321"),
    ("TITLE.DLL", "full_frame_loop", 0x5FB0, 0x3A0, "e046a7853a269b56d38767480b6fe6b52a7b9324dc9eacc1ff21af0048a4b2e7"),
    ("TITLE.DLL", "chain_init_wrapper", 0xC690, 0x20, "f3408ddb192054d6613e5ae6f366706045fd733467bf044a583aa87a3b330dd1"),
    ("TITLE.DLL", "submit_wrapper", 0xC6B0, 0x10, "aaf98bd8e9b93be18c2442d2c7c392a4b9fa05f16d8a6882aa5fb7585dde7b7a"),
]

PROVENANCE_INPUTS = [
    "renderer_contract/frame.md",
    "renderer_contract/evidence/frame.json",
    "renderer_contract/evidence/packets.json",
    "renderer_contract/evidence/eng3_handlers.json",
    "renderer_contract/evidence/title_handlers.json",
    "evidence/frame_bootstrap_contract.json",
    "evidence/engine_dispatch_contract.json",
    "evidence/eng1_pipeline.json",
    "evidence/eng3_interface_wrappers.json",
    "evidence/title_closure.json",
]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def discover_repo_root(start: Path) -> Path:
    for candidate in (start, *start.parents):
        if ((candidate / "scripts" / "pe.py").is_file()
                and (candidate / "renderer_contract" / "frame.md").is_file()):
            return candidate.resolve()
    raise ValueError("cannot discover checkout root; pass --repo-root")


def is_under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    roots = parser.add_mutually_exclusive_group(required=True)
    roots.add_argument("--original-root", type=Path,
                       help="original-data root; checkout root is discovered from this script")
    roots.add_argument("--repo-root", type=Path,
                       help="checkout root used to guard the receipt output")
    parser.add_argument("--exe", required=True, type=Path, help="explicit original HERCULES.EXE path")
    parser.add_argument("--eng1", required=True, type=Path, help="explicit original ENG1.DLL path")
    parser.add_argument("--eng3", required=True, type=Path, help="explicit original ENG3.DLL path")
    parser.add_argument("--title", required=True, type=Path, help="explicit original TITLE.DLL path")
    parser.add_argument("--receipt", required=True, type=Path,
                        help="JSON output path, required to resolve under checkout work/")
    args = parser.parse_args()
    return parser, args


def main() -> int:
    parser, args = parse_args()
    if args.repo_root is not None:
        repo_root = args.repo_root.resolve()
    else:
        repo_root = discover_repo_root(Path(__file__).resolve().parent)
    if not (repo_root / "scripts" / "pe.py").is_file():
        parser.error("repo root must contain scripts/pe.py")

    if args.original_root is not None:
        original_root = args.original_root.resolve()
        image_args = {"HERCULES.EXE": args.exe, "ENG1.DLL": args.eng1,
                      "ENG3.DLL": args.eng3, "TITLE.DLL": args.title}
        image_paths = {}
        for module, path in image_args.items():
            resolved = path.resolve()
            if not is_under(resolved, original_root):
                parser.error(f"{module} path is outside --original-root")
            image_paths[module] = resolved
    else:
        # Paths are explicit and may be outside the checkout (the originals
        # are commonly stored beside a managed source worktree).
        image_paths = {"HERCULES.EXE": args.exe.resolve(), "ENG1.DLL": args.eng1.resolve(),
                       "ENG3.DLL": args.eng3.resolve(), "TITLE.DLL": args.title.resolve()}
        original_root = None

    output = args.receipt if args.receipt.is_absolute() else repo_root / args.receipt
    output = output.resolve()
    work_root = (repo_root / "work").resolve()
    if output == work_root or not is_under(output, work_root):
        parser.error("--receipt must resolve to a file path under <repo-root>/work/")
    for image_path in image_paths.values():
        if not image_path.is_file():
            parser.error(f"missing original image: {image_path}")
        image_directory = image_path.parent.resolve()
        if (is_under(output, image_directory) or is_under(image_directory, output)):
            parser.error(f"receipt path overlaps original image directory: {image_directory}")

    # The repo-local adapter is read-only. Import after validating explicit
    # paths and output bounds so a bad invocation cannot create output.
    sys.path.insert(0, str(repo_root / "scripts"))
    try:
        import env  # noqa: F401 - prepends the configured local dependency path
        from pe import PE
    except ImportError as exc:  # pragma: no cover - environment diagnostic
        raise SystemExit(
            "Could not import scripts/pe.py dependencies. Set HERCULES_PYTHON_DEPS "
            "to the existing C:\\tools\\hercules\\python directory."
        ) from exc

    images = {}
    originals = {}
    failures = []
    for module, path in image_paths.items():
        image = PE(path)
        computed = sha256(image.data)
        expected, expected_base = EXPECTED_MODULES[module]
        image_base = "0x%x" % image.image_base
        originals[module] = {"path": str(path), "expected_sha256": expected,
                             "computed_sha256": computed, "image_base": image_base}
        if computed != expected:
            failures.append({"kind": "image_hash_mismatch", "module": module,
                             "expected": expected, "computed": computed})
        if image_base != expected_base:
            failures.append({"kind": "image_base_mismatch", "module": module,
                             "expected": expected_base, "computed": image_base})
        images[module] = image

    span_rows = []
    for module, label, rva, size, expected in SPANS:
        try:
            computed = sha256(images[module].read_rva(rva, size))
        except Exception as exc:
            computed = None
            failures.append({"kind": "span_read_error", "module": module,
                             "label": label, "rva": "0x%x" % rva,
                             "size": size, "error": str(exc)})
        span_rows.append({"module": module, "label": label, "rva": "0x%x" % rva,
                          "size": size, "expected_sha256": expected,
                          "computed_sha256": computed})
        if computed is not None and computed != expected:
            failures.append({"kind": "span_hash_mismatch", "module": module,
                             "label": label, "rva": "0x%x" % rva,
                             "expected": expected, "computed": computed})

    provenance = []
    for relative in PROVENANCE_INPUTS:
        source = repo_root / relative
        if source.is_file():
            provenance.append({"path": relative, "sha256": sha256(source.read_bytes())})
        else:
            failures.append({"kind": "missing_provenance_input", "path": relative})

    result = {
        "schema_version": 1,
        "status": "passed" if not failures else "failed",
        "scope": "Read-only static hash validation; no original binary execution.",
        "counts": {"original_images": len(originals), "original_spans": len(span_rows),
                   "provenance_files": len(provenance)},
        "original_root": str(original_root) if original_root is not None else None,
        "repo_root": str(repo_root),
        "originals": originals,
        "spans": span_rows,
        "provenance_inputs": provenance,
        "failures": failures,
        "limitations": [
            "A passing hash check confirms byte identity for these spans, not semantic interpretation.",
            "Allocation requests and cursor arithmetic do not establish backing capacity or maximum writes.",
            "AC60 is statically synchronous; downstream callback retention is not excluded.",
        ],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "receipt": str(output),
                      "receipt_sha256": sha256(output.read_bytes()),
                      "images": len(originals), "spans": len(span_rows),
                      "failures": len(failures)}, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
