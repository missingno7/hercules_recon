"""Fingerprint every supplied asset, then inspect installed and extracted PC PEs."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

from env import ROOT
from pe import PE
from pefile import PEFormatError


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def create_manifest():
    files = []
    for path in sorted((ROOT / "assets").rglob("*")):
        if path.is_file():
            files.append({"path": path.relative_to(ROOT).as_posix(),
                          "size": path.stat().st_size, "sha256": sha256(path)})
    binaries = []
    metadata_by_sha256 = {}
    for base in (ROOT / "assets/pc/HERCULES", ROOT / "work/discs/pc_cd", ROOT / "work/discs/pc_install"):
        if not base.exists():
            continue
        for path in sorted(base.rglob("*")):
            if not path.is_file():
                continue
            with path.open("rb") as stream:
                magic = stream.read(2)
            if magic != b"MZ":
                continue
            entry = {"path": path.relative_to(ROOT).as_posix(), "size": path.stat().st_size,
                     "sha256": sha256(path)}
            if entry["sha256"] in metadata_by_sha256:
                binaries.append(entry)
                continue
            try:
                metadata = PE(path).metadata()
                if path.name.upper() not in {"HERCULES.EXE", "ENG1.DLL", "ENG3.DLL", "TITLE.DLL"}:
                    versions = metadata["version_info"]
                    metadata = {k: metadata[k] for k in ("machine", "timestamp_utc", "image_base", "linker_version")}
                    metadata["version_info"] = [{k: v for k, v in version.items() if k in {"FileVersion", "ProductName"}} for version in versions]
                else:
                    # Full IAT addresses remain queryable in the generated SQLite
                    # index. Keep the canonical import identity list compact.
                    metadata["imports"] = {module["dll"]: [s["name"] or f"ordinal:{s['ordinal']}" for s in module["symbols"]]
                                           for module in metadata["imports"]}
                metadata_by_sha256[entry["sha256"]] = {"pe": metadata}
            except (ValueError, OSError, PEFormatError) as exc:
                metadata_by_sha256[entry["sha256"]] = {"non_pe_mz": str(exc)}
            binaries.append(entry)
    groups = defaultdict(list)
    for row in binaries:
        groups[row["sha256"]].append(row["path"])
    return {"schema_version": 1, "hash_algorithm": "SHA-256",
            "policy": "assets are immutable; extracted files are derived evidence",
            "files": files, "binaries": binaries,
            "metadata_by_sha256": metadata_by_sha256,
            "identical_binary_groups": [paths for paths in groups.values() if len(paths) > 1]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true", help="Verify original files against saved manifest")
    args = parser.parse_args()
    destination = ROOT / "evidence/manifest.json"
    if args.verify:
        manifest = json.loads(destination.read_text())
        failures = []
        for row in manifest["files"]:
            path = ROOT / row["path"]
            if not path.is_file() or path.stat().st_size != row["size"] or sha256(path) != row["sha256"]:
                failures.append(row["path"])
        current = {p.relative_to(ROOT).as_posix() for p in (ROOT / "assets").rglob("*") if p.is_file()}
        recorded = {row["path"] for row in manifest["files"]}
        failures.extend(sorted(current - recorded))
        if failures:
            raise SystemExit("Original evidence changed: " + ", ".join(failures))
        print(f"Verified {len(manifest['files'])} immutable original files.")
        return
    if destination.exists():
        raise SystemExit("Manifest exists: use --verify; review and explicitly remove it before replacing evidence identities.")
    manifest = create_manifest()
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Inventoried {len(manifest['files'])} original files and {len(manifest['binaries'])} MZ binaries.")


if __name__ == "__main__":
    main()
