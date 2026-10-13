#!/usr/bin/env python3
"""Verify temporal-state evidence and pinned original spans without execution."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys

ANALYSIS_DEPS = Path(r"C:\tools\hercules\python")
EXPECTED_ORIGINALS = {
    "ENG1.DLL": ("01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757", 0x10000000),
    "HERCULES.EXE": ("587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d", 0x400000),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def file_sha(path: Path) -> str:
    return sha256(path.read_bytes())


def lexical_absolute(path: Path, base: Path | None = None) -> Path:
    if not path.is_absolute():
        if base is None:
            raise ValueError(f"relative path requires an explicit base: {path}")
        path = base / path
    return Path(os.path.abspath(os.fspath(path)))


def is_under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return path != root
    except ValueError:
        return False


def overlaps(path: Path, other: Path) -> bool:
    return path == other or is_under(path, other) or is_under(other, path)


def path_is_redirect(path: Path) -> bool:
    if path.is_symlink():
        return True
    is_junction = getattr(path, "is_junction", None)
    return bool(is_junction and is_junction())


def reject_redirect_components(root_lex: Path, target_lex: Path) -> None:
    if not (target_lex == root_lex or is_under(target_lex, root_lex)):
        raise ValueError(f"path escapes lexical root: {target_lex}")
    relative = target_lex.relative_to(root_lex)
    current = root_lex
    for component in relative.parts:
        current = current / component
        if current.exists() and path_is_redirect(current):
            raise ValueError(f"redirect/reparse point is not allowed in output path: {current}")


def validate_locations(repo_root: Path, eng1: Path, exe: Path, output: Path) -> dict:
    repo_lex = lexical_absolute(repo_root)
    if not repo_lex.is_dir():
        raise ValueError(f"repository root is not a directory: {repo_lex}")
    # Resolve non-strictly because Windows sandboxed file handles can deny the
    # strict final-path query even when ordinary reads are allowed. Walk every
    # existing component explicitly so junctions/symlinks are still rejected.
    reject_redirect_components(Path(repo_lex.anchor), repo_lex)
    repo_real = repo_lex.resolve(strict=False)
    if os.path.normcase(os.fspath(repo_lex)) != os.path.normcase(os.fspath(repo_real)):
        raise ValueError(f"repository root redirects through a symlink/junction: {repo_lex} -> {repo_real}")
    if not (repo_real / "scripts" / "pe.py").is_file():
        raise ValueError(f"repository root does not contain scripts/pe.py: {repo_real}")

    original_args = {"ENG1.DLL": eng1, "HERCULES.EXE": exe}
    original_paths = {}
    for label, raw_path in original_args.items():
        lexical = lexical_absolute(raw_path, repo_lex)
        if not lexical.is_file():
            raise ValueError(f"missing explicit original {label}: {lexical}")
        reject_redirect_components(Path(lexical.anchor), lexical)
        original_paths[label] = {"lexical": lexical, "resolved": lexical.resolve(strict=False)}

    output_lex = lexical_absolute(output, repo_lex)
    output_real = output_lex.resolve(strict=False)
    work_lex = repo_lex / "work"
    work_real = (repo_real / "work").resolve(strict=False)
    if not is_under(output_lex, work_lex):
        raise ValueError(f"output must be lexically below {work_lex}: {output_lex}")
    if not is_under(output_real, work_real):
        raise ValueError(f"output must resolve below {work_real}: {output_real}")
    if output_lex.exists() and output_lex.is_dir():
        raise ValueError(f"output must be a file path, not a directory: {output_lex}")
    if output_lex.exists() and output_lex.stat().st_nlink > 1:
        raise ValueError(f"multiply-linked output cannot be overwritten: {output_lex}")
    reject_redirect_components(repo_lex, output_lex)

    assets_roots = {repo_lex / "assets", (repo_real / "assets").resolve(strict=False)}
    for item in original_paths.values():
        for ancestor in (item["lexical"], item["resolved"]):
            for parent in (ancestor, *ancestor.parents):
                if parent.name.lower() == "assets":
                    assets_roots.add(parent)

    for path in (output_lex, output_real):
        for label, item in original_paths.items():
            for original in (item["lexical"], item["resolved"]):
                if overlaps(path, original):
                    raise ValueError(f"output overlaps original {label}: {path} / {original}")
                if output_lex.exists() and output_lex.samefile(original):
                    raise ValueError(f"output is a hard link to original {label}: {output_lex}")
            for original_dir in (item["lexical"].parent, item["resolved"].parent):
                if overlaps(path, original_dir):
                    raise ValueError(f"output overlaps original-image directory for {label}: {original_dir}")
        for assets_root in assets_roots:
            if overlaps(path, assets_root):
                raise ValueError(f"output overlaps assets directory: {assets_root}")

    if output_lex.resolve(strict=False) != output_real:
        raise ValueError(f"output redirects: {output_lex} -> {output_real}")
    return {
        "repo_lexical": repo_lex,
        "repo_resolved": repo_real,
        "work_lexical": work_lex,
        "work_resolved": work_real,
        "original_paths": original_paths,
        "output_lexical": output_lex,
        "output_resolved": output_real,
    }


def import_pe(repo_root: Path):
    os.environ["HERCULES_PYTHON_DEPS"] = str(ANALYSIS_DEPS)
    scripts = str(repo_root / "scripts")
    if scripts not in sys.path:
        sys.path.insert(0, scripts)
    import env  # noqa: F401 - configures existing local dependency path
    from pe import PE
    return PE


def evidence_file() -> Path:
    return Path(__file__).resolve().parents[1] / "evidence" / "temporal_state.json"


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path,
                        help="explicit hash-pinned original HERCULES.EXE path")
    parser.add_argument("--eng1", required=True, type=Path,
                        help="explicit hash-pinned original ENG1.DLL path")
    parser.add_argument("--output", required=True, type=Path,
                        help="receipt path, lexically and physically under repo-root/work")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    locations = validate_locations(args.repo_root, args.eng1, args.exe, args.output)
    repo_root = locations["repo_resolved"]
    evidence_path = evidence_file()
    if not evidence_path.is_file():
        raise SystemExit(f"missing temporal evidence file beside test package: {evidence_path}")
    evidence = json.loads(evidence_path.read_text(encoding="utf-8-sig"))
    if evidence.get("schema") != "renderer_temporal_state_v1":
        raise ValueError("unexpected temporal evidence schema")
    if evidence["static_observed"].get("classification") != "STATIC_OBSERVED":
        raise ValueError("static observation classification changed")
    if evidence["bounded_equivalent"].get("classification") != "BOUNDED_EQUIVALENT":
        raise ValueError("controlled observation classification changed")
    prediction_path = Path(__file__).resolve().parents[1] / "fixtures" / "temporal" / "prediction.md"
    if not prediction_path.is_file():
        raise ValueError(f"missing preregistered prediction: {prediction_path}")
    prediction_sha = file_sha(prediction_path)
    if prediction_sha != evidence["bounded_equivalent"].get("prediction_sha256"):
        raise ValueError(f"prediction fixture hash mismatch: {prediction_sha}")

    PE = import_pe(repo_root)
    image_objects = {}
    originals = {}
    for label, path_entry in locations["original_paths"].items():
        path = path_entry["resolved"]
        image = PE(path)
        actual_sha = sha256(image.data)
        expected_sha, expected_base = EXPECTED_ORIGINALS[label]
        expected = evidence["oracle_pins"][label]
        if actual_sha != expected_sha or actual_sha != expected["sha256"]:
            raise ValueError(f"{label} hash mismatch: {actual_sha}")
        if image.image_base != expected_base or image.image_base != int(expected["image_base"], 16):
            raise ValueError(f"{label} image-base mismatch: {image.image_base:#x}")
        image_objects[label] = image
        originals[label] = {"path": str(path), "sha256": actual_sha,
                            "image_base": hex(image.image_base)}

    verified_spans = []
    for span in evidence["static_observed"]["spans"]:
        image = image_objects[span["image"]]
        actual = sha256(image.read_rva(int(span["rva"], 16), int(span["size"])))
        if actual != span["sha256"]:
            raise ValueError(f"span mismatch {span['image']} {span['label']} {span['rva']}: {actual}")
        verified_spans.append({"image": span["image"], "label": span["label"],
                               "rva": span["rva"], "size": span["size"],
                               "sha256": actual})

    verified_inputs = {}
    for relative, expected_sha in evidence["static_observed"]["input_sha256"].items():
        path = repo_root / Path(relative)
        actual_sha = file_sha(path)
        if actual_sha != expected_sha:
            raise ValueError(f"evidence input hash mismatch for {relative}: {actual_sha}")
        verified_inputs[relative] = actual_sha

    output = locations["output_resolved"]
    output.parent.mkdir(parents=True, exist_ok=True)
    # Recheck after directory creation to catch a newly introduced redirection.
    post = validate_locations(args.repo_root, args.eng1, args.exe, args.output)
    if post["output_resolved"] != output:
        raise ValueError("output path changed during audit setup")
    receipt = {
        "schema": "renderer_temporal_state_static_audit_v1",
        "status": "PASS_STATIC_TEMPORAL_EVIDENCE",
        "repo_root": {"lexical": str(locations["repo_lexical"]),
                      "resolved": str(locations["repo_resolved"])},
        "output": str(output),
        "originals": originals,
        "verified_span_count": len(verified_spans),
        "verified_spans": verified_spans,
        "verified_input_count": len(verified_inputs),
        "verified_inputs": verified_inputs,
        "prediction_fixture": {"path": str(prediction_path), "sha256": prediction_sha},
        "classifications": ["STATIC_OBSERVED", "BOUNDED_EQUIVALENT"],
        "original_code_executed": False,
        "scope": "Static hash and schema audit only; controlled original execution is in test_original_camera_history.py.",
    }
    output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "verified_spans": len(verified_spans),
                      "verified_inputs": len(verified_inputs), "output": str(output)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
