"""Strict output-path checks for the public ENG1 real-resource replay."""
from __future__ import annotations

import json
import os
from pathlib import Path
import stat

MARKER = ".eng1_real_mesh_output.json"


def discover_repo_root(start: str | Path) -> Path:
    origin = Path(start).resolve(strict=True)
    candidates = [origin, *origin.parents]
    for candidate in candidates:
        if ((candidate / "renderer_contract" / "fixtures" / "eng1_association" / "run_fixture.py").is_file()
                and (candidate / "work").is_dir()):
            return candidate.resolve(strict=True)
    raise ValueError(f"could not discover repository root from {origin}")


def _no_reparse_components(path: Path) -> None:
    absolute = Path(os.path.abspath(path))
    current = Path(absolute.anchor)
    for component in absolute.parts[1:]:
        current = current / component
        try:
            info = current.lstat()
        except FileNotFoundError:
            continue
        if stat.S_ISLNK(info.st_mode) or (getattr(info, "st_file_attributes", 0) & 0x400):
            raise ValueError(f"reparse/symlink path component is not allowed: {current}")


def repo_path(value: str | Path) -> Path:
    root = Path(value).resolve(strict=True)
    if not (root / "renderer_contract" / "fixtures" / "eng1_association" / "run_fixture.py").is_file():
        raise ValueError(f"--repo-root does not identify a Hercules repository: {root}")
    work = root / "work"
    if not work.is_dir():
        raise ValueError(f"repository work directory is missing: {work}")
    _no_reparse_components(root)
    _no_reparse_components(work)
    return root


def _under_work(root: Path, output: str | Path) -> Path:
    root = repo_path(root)
    raw = Path(output)
    if not raw.is_absolute():
        raw = root / raw
    _no_reparse_components(raw)
    resolved = raw.resolve(strict=False)
    work = (root / "work").resolve(strict=True)
    try:
        resolved.relative_to(work)
    except ValueError:
        raise ValueError(f"output must be strictly under {work}: {resolved}")
    if resolved == work:
        raise ValueError("the work directory itself cannot be an output")
    return resolved


def create_fresh_output(root: Path, output: str | Path) -> Path:
    target = _under_work(root, output)
    if target.exists():
        raise ValueError(f"fresh output path already exists: {target}")
    target.mkdir(parents=True, exist_ok=False)
    marker = {"schema": "hercules.eng1-real-mesh-output.v1",
              "repo_root": str(root.resolve()), "output": str(target),
              "owner": "run_native_real_map.py"}
    (target / MARKER).write_text(json.dumps(marker, indent=2) + "\n", encoding="utf-8")
    return target


def owned_output(root: Path, output: str | Path) -> Path:
    target = _under_work(root, output)
    marker_path = target / MARKER
    if not target.is_dir() or not marker_path.is_file():
        raise ValueError(f"output is not an initialized ENG1 replay directory: {target}")
    marker = json.loads(marker_path.read_text(encoding="utf-8"))
    if (marker.get("schema") != "hercules.eng1-real-mesh-output.v1" or
            marker.get("repo_root") != str(root.resolve()) or
            marker.get("output") != str(target) or
            marker.get("owner") != "run_native_real_map.py"):
        raise ValueError(f"ENG1 replay output ownership marker mismatch: {marker_path}")
    return target


def create_fresh_child(parent: Path, name: str) -> Path:
    if Path(name).name != name or name in (".", ".."):
        raise ValueError(f"invalid output child name: {name!r}")
    child = parent / name
    _no_reparse_components(child)
    if child.exists():
        raise ValueError(f"fresh output child already exists: {child}")
    child.mkdir(parents=False, exist_ok=False)
    return child


def input_file(path: str | Path, label: str) -> Path:
    candidate = Path(path).resolve(strict=True)
    if not candidate.is_file():
        raise ValueError(f"{label} is not a file: {candidate}")
    _no_reparse_components(candidate)
    return candidate


def file_below(path: str | Path, root: Path, label: str) -> Path:
    candidate = Path(path).resolve(strict=True)
    _no_reparse_components(candidate)
    try:
        candidate.relative_to(root.resolve(strict=True))
    except ValueError:
        raise ValueError(f"{label} must be under the replay output: {candidate}")
    if not candidate.is_file():
        raise ValueError(f"{label} is not a file: {candidate}")
    return candidate
