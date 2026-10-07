"""Fresh historical compile, full COFF contribution comparison, strict regression."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from functools import lru_cache

from env import ROOT
from pe import PE
from coff import COFF

DEFAULT_TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")


def digest(data):
    return hashlib.sha256(data).hexdigest()


@lru_cache(maxsize=None)
def verify_toolchain(toolchain):
    manifest = json.loads((ROOT / "toolchains/manifest.json").read_text())
    entries = [t for t in manifest["toolchains"] if t["directory"] == toolchain.name]
    if len(entries) != 1:
        raise ValueError("Unpinned toolchain; record and review its provenance first")
    for row in entries[0]["files"]:
        path = toolchain / row["path"]
        if not path.is_file() or path.stat().st_size != row["size"] or digest(path.read_bytes()) != row["sha256"]:
            raise ValueError(f"Toolchain identity changed: {path}")
    return entries[0]["id"]


def compile_source(source, flags, toolchain=DEFAULT_TOOLCHAIN):
    verify_toolchain(toolchain)
    scratch = ROOT / "build/match"
    scratch.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="compile-", dir=scratch))
    obj = output / "candidate.obj"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    command = [str(toolchain / "bin/cl.exe"), "/nologo", "/c", "/Gy", *flags,
               "/Fo" + str(obj), str(Path(source).resolve())]
    result = subprocess.run(command, env=env, cwd=output, capture_output=True, text=True)
    (output / "compile.log").write_text(result.stdout + result.stderr)
    if result.returncode != 0 or not obj.is_file():
        raise RuntimeError(f"Historical compiler failed ({result.returncode}); see {output / 'compile.log'}")
    return obj, command


def compare(obj, symbol, target, rva, size):
    contribution = COFF(obj).function(symbol)
    actual = contribution["data"]
    pe = PE(target)
    if size <= 0 or not any(s["characteristics"] & 0x20000000 and
                           s["virtual_address"] <= rva and
                           rva + size <= s["virtual_address"] + min(s["virtual_size"], s["raw_size"])
                           for s in pe.sections):
        raise ValueError("Target interval must be wholly inside file-backed executable code")
    target_relocations = [entry.rva - rva for block in getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", [])
                          for entry in block.entries if entry.type and
                          entry.rva < rva + size and entry.rva + 4 > rva]
    expected = pe.read_rva(rva, size)
    differences = [i for i in range(max(len(expected), len(actual)))
                   if expected[i:i+1] != actual[i:i+1]]
    regions = []
    for offset in differences:
        if regions and regions[-1][1] == offset:
            regions[-1][1] = offset + 1
        else:
            regions.append([offset, offset + 1])
    relocations = contribution["relocations"]
    exact = expected == actual and not relocations and not target_relocations
    return {"exact": exact, "scope": "complete relocation-free function COFF contribution",
            "target_size": len(expected), "compiled_size": len(actual),
            "target_sha256": digest(expected), "compiled_sha256": digest(actual),
            "different_bytes": len(differences), "mismatch_regions": regions,
            "relocations": relocations, "target_relocations": target_relocations,
            "object": str(obj.resolve())}


def check(row, source=None, toolchain=DEFAULT_TOOLCHAIN):
    target = ROOT / row["target"]
    if digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError(f"Target hash mismatch: {target}")
    canonical = source is None
    source = Path(source) if source else ROOT / row["source"]
    source_before = source.read_bytes()
    if canonical and row["status"] == "FUNCTION_MATCH" and digest(source_before) != row["proof"]["source_sha256"]:
        raise ValueError("Canonical source differs from accepted proof; review and promote it before verification")
    obj, command = compile_source(source, row["flags"], toolchain)
    if source.read_bytes() != source_before:
        raise ValueError("Source changed during compilation")
    result = compare(obj, row["symbol"], target, int(row["rva"], 0), row["size"])
    if digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError("Target changed during verification")
    result.update(id=row["id"], source_sha256=digest(source_before), command=command,
                  toolchain=verify_toolchain(toolchain))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["verify", "candidate", "object"])
    parser.add_argument("id", nargs="?")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--obj", type=Path)
    parser.add_argument("--toolchain", type=Path, default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    state = json.loads((ROOT / "recovery.json").read_text())
    rows = [r for r in state["functions"] if r["id"] == args.id] if args.id else state["functions"]
    if args.id and not rows:
        raise SystemExit(f"Unknown function {args.id}")
    if args.command == "object":
        if len(rows) != 1 or args.obj is None:
            parser.error("object requires one function id and --obj")
        row = rows[0]
        results = [compare(args.obj, row["symbol"], ROOT / row["target"], int(row["rva"], 0), row["size"])]
    elif args.command == "candidate":
        if len(rows) != 1 or args.source is None:
            parser.error("candidate requires one function id and --source")
        results = [check(rows[0], args.source, args.toolchain)]
    else:
        rows = [r for r in rows if r["status"] == "FUNCTION_MATCH"]
        if not rows:
            raise SystemExit("No accepted functions to verify")
        results = [check(r, toolchain=args.toolchain) for r in rows]
    if args.report:
        if not any(args.report.resolve().is_relative_to((ROOT / directory).resolve()) for directory in ("work", "build")):
            parser.error("Generated reports must go under work/ or build/")
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(results, indent=2) + "\n")
    for result in results:
        print(f"{result.get('id', args.id)}: {'EXACT' if result['exact'] else 'MISMATCH'} "
              f"{result['compiled_size']}/{result['target_size']} bytes; "
              f"{result['different_bytes']} differing bytes; "
              f"{len(result['relocations'])} object relocations, {len(result['target_relocations'])} target relocations")
        if result["mismatch_regions"]:
            print("  First mismatch intervals: " + str(result["mismatch_regions"][:12]))
    if not all(r["exact"] for r in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
