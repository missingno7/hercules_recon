"""Path checks shared by the source-only presentation probe builders."""
from __future__ import annotations

import os
from pathlib import Path


class UnsafePathError(ValueError):
    pass


def _normal(path: Path) -> str:
    return os.path.normcase(os.path.normpath(str(path)))


def _is_redirect(path: Path) -> bool:
    try:
        if path.is_symlink():
            return True
    except OSError:
        return True
    isjunction = getattr(os.path, "isjunction", None)
    if isjunction is not None:
        try:
            if isjunction(str(path)):
                return True
        except OSError:
            return True
    return False


def _reject_redirect_chain(path: Path) -> None:
    current = Path(path.anchor)
    for component in path.parts[1:]:
        current = current / component
        if _is_redirect(current):
            raise UnsafePathError("reparse-point path is not allowed: " +
                                  str(current))


def guard_work_output(repo_root_arg: Path, output_arg: Path) -> tuple[Path, Path, Path]:
    """Require lexical and resolved output paths below repo_root/work.

    Reject traversal syntax, symlinks, junctions, and other resolved escapes
    before creating anything. Existing empty output directories are allowed.
    """
    root_lexical = Path(os.path.abspath(repo_root_arg))
    if _is_redirect(root_lexical):
        raise UnsafePathError("repository root may not be a symlink or junction")
    root = root_lexical.resolve(strict=True)
    if _normal(root_lexical) != _normal(root):
        raise UnsafePathError("repository root resolves through a redirect")
    if not root.is_dir():
        raise UnsafePathError("repository root is not a directory")

    work_lexical = root / "work"
    if not work_lexical.is_dir() or _is_redirect(work_lexical):
        raise UnsafePathError("repository work directory is missing or redirected")
    work = work_lexical.resolve(strict=True)
    if _normal(work_lexical) != _normal(work):
        raise UnsafePathError("repository work directory resolves elsewhere")

    if ".." in output_arg.parts:
        raise UnsafePathError("output path may not contain '..'")
    output_lexical = Path(os.path.abspath(
        output_arg if output_arg.is_absolute() else root / output_arg))
    try:
        relative = output_lexical.relative_to(work_lexical)
    except ValueError as exc:
        raise UnsafePathError("output lexical path must be below repository work") from exc
    if not relative.parts:
        raise UnsafePathError("output may not be the work directory itself")

    _reject_redirect_chain(output_lexical)
    output = output_lexical.resolve(strict=False)
    try:
        resolved_relative = output.relative_to(work)
    except ValueError as exc:
        raise UnsafePathError("output resolved path must be below repository work") from exc
    if not resolved_relative.parts:
        raise UnsafePathError("output may not resolve to the work directory")
    if output.exists() and not output.is_dir():
        raise UnsafePathError("output path exists and is not a directory")
    if output.exists() and any(output.iterdir()):
        raise UnsafePathError("output must be new or empty")
    return root, work, output


def require_no_redirects(path: Path) -> Path:
    """Reject path components that are symlinks or Windows junctions."""
    absolute = Path(os.path.abspath(path))
    _reject_redirect_chain(absolute)
    return absolute.resolve(strict=True)
