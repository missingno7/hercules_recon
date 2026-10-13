"""Hash-pinned, read-only validation of original PE images and evidence spans."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
try:
    import env  # noqa: F401  # Adds the configured bundled dependency directory.
    from pe import PE
except ImportError as exc:  # pragma: no cover - environment diagnostic
    raise SystemExit(
        "Could not import scripts/pe.py dependencies. Set HERCULES_PYTHON_DEPS "
        "to the existing C:\\tools\\hercules\\python directory."
    ) from exc

DEFAULT_MANIFEST = Path(__file__).resolve().parents[1] / "evidence" / "original_spans.json"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def as_int(value) -> int:
    return int(value, 16) if isinstance(value, str) else int(value)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--original-root", type=Path,
                        help="Root with work/discs/pc_install and assets/pc/HERCULES.")
    parser.add_argument("--exe", type=Path, help="Explicit original HERCULES.EXE path.")
    parser.add_argument("--eng1", type=Path, help="Explicit original ENG1.DLL path.")
    parser.add_argument("--eng3", type=Path, help="Explicit original ENG3.DLL path.")
    parser.add_argument("--title", type=Path, help="Explicit original TITLE.DLL path.")
    args = parser.parse_args()

    explicit = {"HERCULES.EXE": args.exe, "ENG1.DLL": args.eng1,
                "ENG3.DLL": args.eng3, "TITLE.DLL": args.title}
    supplied = [name for name, path in explicit.items() if path is not None]
    if args.original_root is not None and supplied:
        parser.error("use --original-root or the four explicit image paths, not both")
    if args.original_root is None and supplied and len(supplied) != 4:
        parser.error("--exe, --eng1, --eng3, and --title must be supplied together")
    if args.original_root is None and not supplied:
        parser.error("supply --original-root or all four explicit image paths")
    return args, explicit


def resolve_paths(args, explicit, manifest):
    if args.original_root is not None:
        base = args.original_root.resolve()
        return {module: base / row["path_layout"]
                for module, row in manifest["original_images"].items()}
    return {module: explicit[module].resolve() for module in manifest["original_images"]}


def validate(manifest_path: Path, image_paths: dict[str, Path]) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 1:
        raise ValueError(f"unsupported manifest schema_version {manifest.get('schema_version')!r}")
    originals = manifest.get("original_images")
    if not originals:
        raise ValueError("manifest has no original_images")
    if set(image_paths) != set(originals):
        raise ValueError("resolved image set does not match manifest original_images")

    images = {}
    file_rows = {}
    failures = []
    package_checks = 0
    for item in manifest.get("package_evidence", []):
        path = (ROOT / item["path"]).resolve()
        try:
            path.relative_to(ROOT.resolve())
        except ValueError:
            failures.append({"kind": "package_path_outside_repository", "path": item["path"]})
            continue
        if not path.is_file() or sha256(path.read_bytes()) != item["sha256"]:
            failures.append({"kind": "package_evidence_hash_mismatch", "path": item["path"]})
        package_checks += 1
    for module, expected in originals.items():
        path = image_paths[module]
        if not path.is_file():
            failures.append({"kind": "missing_image", "module": module, "path": str(path)})
            continue
        raw = path.read_bytes()
        computed = sha256(raw)
        image = PE(path)
        expected_base = expected.get("image_base")
        if expected_base is not None and image.image_base != as_int(expected_base):
            failures.append({
                "kind": "image_base_mismatch", "module": module,
                "expected": expected_base, "computed": hex(image.image_base),
            })
        if computed != expected["expected_sha256"].lower():
            failures.append({
                "kind": "whole_file_hash_mismatch", "module": module,
                "expected_sha256": expected["expected_sha256"], "computed_sha256": computed,
            })
        images[module] = image
        file_rows[module] = {"expected_sha256": expected["expected_sha256"],
                             "computed_sha256": computed, "bytes": len(raw)}

    span_checks = 0
    for span in manifest.get("spans", []):
        module = span["module"]
        image = images.get(module)
        if image is None:
            continue
        expected_hash = span["expected_sha256"].lower()
        if span.get("hash_expectation") != "recorded_in_source_evidence":
            failures.append({"kind": "unknown_span_hash_provenance", "module": module,
                             "rva": span.get("rva"), "label": span.get("label")})
            continue
        try:
            raw = image.read_rva(as_int(span["rva"]), int(span["size"]))
        except Exception as exc:
            failures.append({"kind": "span_read_error", "module": module,
                             "rva": span.get("rva"), "size": span.get("size"),
                             "error": str(exc)})
            continue
        computed = sha256(raw)
        span_checks += 1
        if computed != expected_hash:
            failures.append({"kind": "span_hash_mismatch", "module": module,
                             "rva": span["rva"], "size": span["size"],
                             "label": span.get("label"), "expected_sha256": expected_hash,
                             "computed_sha256": computed})

    table = manifest.get("callback_table")
    if not table:
        raise ValueError("manifest has no callback_table")
    table_module = table["module"]
    table_image = images.get(table_module)
    table_checked = 0
    callbacks_checked = 0
    geometry_wrappers_checked = 0
    dispatch_sites_checked = 0
    copied = {row["module"]: row for row in table.get("runtime_copies", [])}
    table_targets = {int(row["slot"]): row for row in table.get("entries", [])}

    if table.get("hash_expectation") != "recorded_in_source_evidence":
        failures.append({"kind": "unknown_table_hash_provenance", "module": table_module})
    if table_image is not None:
        try:
            raw = table_image.read_rva(as_int(table["rva"]), int(table["size"]))
            computed = sha256(raw)
            table_checked = 1
            if computed != table["expected_sha256"].lower():
                failures.append({"kind": "callback_table_hash_mismatch", "expected_sha256": table["expected_sha256"], "computed_sha256": computed})
            for row in table.get("entries", []):
                slot = int(row["slot"])
                offset = slot * 4
                cell_rva = as_int(table["rva"]) + offset
                if as_int(row["byte_offset"]) != offset or as_int(row["cell_rva"]) != cell_rva:
                    failures.append({"kind": "callback_cell_layout_mismatch", "slot": slot,
                                     "expected_cell_rva": hex(cell_rva),
                                     "manifest_cell_rva": row.get("cell_rva")})
                    continue
                cell = table_image.read_rva(cell_rva, 4)
                target_va = struct.unpack("<I", cell)[0]
                target_rva = target_va - table_image.image_base
                callbacks_checked += 1
                if target_rva != as_int(row["expected_target_rva"]):
                    failures.append({"kind": "callback_target_mismatch", "slot": slot,
                                     "cell_rva": hex(cell_rva),
                                     "expected_target_rva": row["expected_target_rva"],
                                     "computed_target_rva": hex(target_rva)})
                if target_va != as_int(row["expected_target_va"]):
                    failures.append({"kind": "callback_target_va_mismatch", "slot": slot,
                                     "expected_target_va": row["expected_target_va"],
                                     "computed_target_va": hex(target_va)})
        except Exception as exc:
            failures.append({"kind": "callback_table_read_error", "error": str(exc)})

    # Runtime-copy destinations are verified as address arithmetic only: copied
    # data may live in a DLL virtual tail and is not required to be file-backed.
    for module, row in copied.items():
        if int(row["bytes"]) != int(table["size"]):
            failures.append({"kind": "copied_table_size_mismatch", "module": module,
                             "source_bytes": table["size"], "copy_bytes": row["bytes"]})

    for row in table.get("geometry_wrapper_bindings", []):
        module = row["module"]
        copy = copied.get(module)
        slot = int(row["slot"])
        expected_cell = as_int(row["copied_table_base_rva"]) + as_int(row["byte_offset"])
        if copy is None or as_int(copy["base_rva"]) != as_int(row["copied_table_base_rva"]):
            failures.append({"kind": "wrapper_copy_base_mismatch", "module": module,
                             "wrapper_rva": row["wrapper_rva"]})
        if expected_cell != as_int(row["copied_cell_rva"]) or as_int(row["byte_offset"]) != slot * 4:
            failures.append({"kind": "wrapper_cell_formula_mismatch", "module": module,
                             "wrapper_rva": row["wrapper_rva"], "slot": slot})
        target = table_targets.get(slot)
        if target is None or as_int(target["expected_target_rva"]) != as_int(row["host_target_rva"]):
            failures.append({"kind": "wrapper_host_target_mismatch", "module": module,
                             "wrapper_rva": row["wrapper_rva"], "slot": slot})
        image = images.get(module)
        if image is not None:
            try:
                computed = sha256(image.read_rva(as_int(row["wrapper_rva"]), int(row["wrapper_size"])))
                geometry_wrappers_checked += 1
                if computed != row["wrapper_expected_sha256"].lower():
                    failures.append({"kind": "wrapper_hash_mismatch", "module": module,
                                     "wrapper_rva": row["wrapper_rva"],
                                     "expected_sha256": row["wrapper_expected_sha256"],
                                     "computed_sha256": computed})
            except Exception as exc:
                failures.append({"kind": "wrapper_read_error", "module": module,
                                 "wrapper_rva": row["wrapper_rva"], "error": str(exc)})

    for row in table.get("direct_dispatch_sites", []):
        module = row["module"]
        copy = copied.get(module)
        slot = int(row["slot"])
        expected_cell = as_int(row["copied_cell_rva"])
        if copy is None or expected_cell != as_int(copy["base_rva"]) + slot * 4:
            failures.append({"kind": "dispatch_cell_formula_mismatch", "module": module,
                             "slot": slot, "instruction_rva": row["instruction_rva"]})
        binding = table_targets.get(slot)
        if binding is None or as_int(binding["expected_target_rva"]) != as_int(row["host_target_rva"]):
            failures.append({"kind": "dispatch_host_target_mismatch", "module": module,
                             "slot": slot, "instruction_rva": row["instruction_rva"]})
        # The source report records the original absolute operand; validate its
        # module-relative cell without emitting or depending on disassembly.
        operand = row["operand"]
        left, right = operand.find("["), operand.find("]")
        image = images.get(module)
        if left < 0 or right <= left or image is None:
            failures.append({"kind": "dispatch_operand_unreadable", "module": module,
                             "instruction_rva": row["instruction_rva"]})
        else:
            try:
                cell_va = int(operand[left + 1:right].strip(), 16)
                cell_rva = cell_va - image.image_base
                if cell_rva != expected_cell:
                    failures.append({"kind": "dispatch_operand_cell_mismatch", "module": module,
                                     "slot": slot, "instruction_rva": row["instruction_rva"],
                                     "expected_cell_rva": hex(expected_cell),
                                     "computed_cell_rva": hex(cell_rva)})
            except ValueError:
                failures.append({"kind": "dispatch_operand_unparsed", "module": module,
                                 "instruction_rva": row["instruction_rva"], "operand": operand})
        dispatch_sites_checked += 1

    return {
        "status": "passed" if not failures else "failed",
        "manifest": str(manifest_path),
        "manifest_sha256": sha256(manifest_path.read_bytes()),
        "hash_semantics": manifest.get("hash_semantics", {}),
        "counts": {
            "original_images": len(images),
            "original_files_hash_checked": len(file_rows),
            "span_hashes_checked": span_checks,
            "package_evidence_hashes_checked": package_checks,
            "callback_table_hashes_checked": table_checked,
            "callback_bindings_checked": callbacks_checked,
            "copied_wrapper_hashes_checked": geometry_wrappers_checked,
            "direct_dispatch_bindings_checked": dispatch_sites_checked,
        },
        "original_files": file_rows,
        "excluded_or_pending_reports": manifest.get("excluded_or_pending_reports", []),
        "failures": failures,
        "disassembly_emitted": False,
    }


def main() -> int:
    args, explicit = parse_args()
    manifest_path = args.manifest
    if not manifest_path.is_absolute():
        manifest_path = ROOT / manifest_path
    manifest_path = manifest_path.resolve()
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        image_paths = resolve_paths(args, explicit, manifest)
        report = validate(manifest_path, image_paths)
    except Exception as exc:
        report = {
            "status": "error",
            "manifest": str(manifest_path),
            "error": f"{type(exc).__name__}: {exc}",
            "disassembly_emitted": False,
        }
    print(json.dumps(report, indent=2))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
