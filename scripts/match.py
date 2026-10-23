"""Fresh historical compile, full COFF contribution comparison, strict regression."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile
import time
from functools import lru_cache

from env import ROOT
from pe import PE
from coff import COFF

DEFAULT_TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def unit_inventory(obj):
    """Complete timestamp-independent object identity, not native data equality."""
    coff = COFF(obj)
    content = dict(sections=coff.sections, symbols=coff.symbols)
    return digest(json.dumps(content, sort_keys=True, default=lambda value: value.hex()).encode())


def context_proof(context):
    return {(p.relative_to(ROOT).as_posix() if p.is_relative_to(ROOT) else str(p)): h
            for p, h in context.items()}


def validate_unit_proof(row, context, obj=None):
    proof = row.get('proof', {})
    if 'unit_inventory_sha256' in proof:
        if not proof.get('context_sha256') or context_proof(context) != proof['context_sha256']:
            raise ValueError('Accepted unit source/header context changed: ' + row['id'])
        if obj is not None and unit_inventory(obj) != proof['unit_inventory_sha256']:
            raise ValueError('Accepted complete unit inventory changed: ' + row['id'])


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


def check(row, source=None, toolchain=DEFAULT_TOOLCHAIN, original_root=None):
    # Source and acceptance state remain local to this worktree. Only the
    # read location of the hash-pinned original may come from another checkout.
    target = (Path(original_root).resolve() if original_root is not None else ROOT) / row["target"]
    if digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError(f"Target hash mismatch: {target}")
    canonical = source is None
    source = Path(source) if source else ROOT / row["source"]
    source_before = source.read_bytes()
    if canonical and row["status"] == "FUNCTION_MATCH" and digest(source_before) != row["proof"]["source_sha256"]:
        raise ValueError("Canonical source differs from accepted proof; review and promote it before verification")
    context = compile_inputs(source, row['flags'], toolchain) if row.get('proof', {}).get('unit_inventory_sha256') else {}
    if context:
        validate_unit_proof(row, context)
    obj, command = compile_source(source, row["flags"], toolchain)
    if source.read_bytes() != source_before:
        raise ValueError("Source changed during compilation")
    result = compare(obj, row["symbol"], target, int(row["rva"], 0), row["size"])
    if digest(target.read_bytes()) != row["target_sha256"]:
        raise ValueError("Target changed during verification")
    if context:
        if compile_inputs(source, row['flags'], toolchain) != context:
            raise ValueError('Compile context changed during verification')
        validate_unit_proof(row, context, obj)
    result.update(id=row["id"], source_sha256=digest(source_before), command=command,
                  toolchain=verify_toolchain(toolchain))
    if original_root is not None:
        result["read_only_original_path"] = str(target.resolve())
    return result


def compile_inputs(source, flags, toolchain):
    """Conservative literal-include closure; unsupported input modes fail closed.

    Visit even inactive literal includes. This is not a preprocessor and must not
    silently certify macro includes, forced includes, PCHs or response files.
    """
    include_dirs = []
    index = 0
    while index < len(flags):
        flag = flags[index]
        if flag.startswith(('/I', '-I')):
            value = flag[2:]
            if not value:
                index += 1
                if index == len(flags):
                    raise ValueError('Missing include path')
                value = flags[index]
            path = Path(value.strip('"'))
            if not path.is_absolute():
                raise ValueError('Grouped verification requires absolute include paths')
            include_dirs.append(path)
        elif not re.fullmatch(r'[/-](?:O[12dx]|O[bgistwy][12+-]?|G[3456dryzX]|G[fy][+-]?|W[0-4]|w|Z[ade]|MTd?|MLd?|MDd?|TC|TP|D[^\s]+|U\w+)', flag):
            raise ValueError('Unsupported grouped compile input flag: ' + flag)
        index += 1
    include_dirs.append(toolchain / 'include')
    result = {}

    def visit(path):
        path = path.resolve()
        if path in result:
            return
        data = path.read_bytes()
        result[path] = digest(data)
        text = data.decode('latin1').replace('\\\r\n', '').replace('\\\n', '')
        # Preserve strings while removing comments, including multiline comments.
        text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                      lambda m: ' ' + '\n' * m[0].count('\n') if m[0].startswith('/') else m[0], text, flags=re.S)
        for directive in re.finditer(r'^\s*#\s*(include\w*|import)\b\s*([^\n]*)', text, re.M):
            name = re.fullmatch(r'"([^"\n]+)"|<([^>\n]+)>', directive[2].strip())
            if directive[1] != 'include' or name is None:
                raise ValueError('Unsupported include directive in ' + str(path))
            roots = ([path.parent] if name[1] else []) + include_dirs
            choices = [root / (name[1] or name[2]) for root in roots]
            found = next((p for p in choices if p.is_file()), None)
            if found is None:
                raise ValueError('Unresolved include in ' + str(path) + ': ' + directive[2])
            visit(found)
    visit(source)
    return result


def verify_rows(rows, toolchain=DEFAULT_TOOLCHAIN, original_root=None):
    """One fresh compile per immutable source/ordered-profile group, per run only."""
    started = time.perf_counter()
    toolchain = toolchain.resolve()
    manifest = ROOT / 'toolchains/manifest.json'
    manifest_before = manifest.read_bytes()
    verify_toolchain.cache_clear()
    toolchain_id = verify_toolchain(toolchain)
    groups, targets = {}, {}
    for row in rows:
        source, target = (ROOT / row['source']).resolve(), ((original_root or ROOT) / row['target']).resolve()
        if row['status'] != 'FUNCTION_MATCH':
            raise ValueError('Strict verification requires accepted rows')
        if digest(source.read_bytes()) != row['proof']['source_sha256']:
            raise ValueError('Canonical source differs from accepted proof: ' + row['id'])
        if target in targets and targets[target] != row['target_sha256']:
            raise ValueError('Conflicting target identities')
        targets[target] = row['target_sha256']
        groups.setdefault((source, tuple(row['flags'])), []).append(row)
    contexts = {key: compile_inputs(*key, toolchain) for key in groups}
    for key, members in groups.items():
        if any(contexts[key][key[0]] != row['proof']['source_sha256'] for row in members):
            raise ValueError('Source changed while freezing compile inputs')
        for row in members:
            validate_unit_proof(row, contexts[key])

    def stable():
        if manifest.read_bytes() != manifest_before:
            raise ValueError('Toolchain manifest changed during verification')
        for target, expected in targets.items():
            if digest(target.read_bytes()) != expected:
                raise ValueError('Target hash mismatch or mutation: ' + str(target))
        for key, expected in contexts.items():
            if compile_inputs(*key, toolchain) != expected:
                raise ValueError('Compile inputs changed during verification: ' + str(key[0]))

    stable()
    results, objects = [], {}
    for (source, flags), members in groups.items():
        obj, command = compile_source(source, list(flags), toolchain)
        objects[obj] = digest(obj.read_bytes())
        stable()
        # Full object identity guards all sections, symbols, auxiliaries and
        # relocations against mutation; it is not a cross-build identity/cache.
        for row in members:
            validate_unit_proof(row, contexts[(source, flags)], obj)
            result = compare(obj, row['symbol'], (original_root or ROOT) / row['target'], int(row['rva'], 0), row['size'])
            result.update(id=row['id'], source_sha256=contexts[(source, flags)][source],
                          command=command, toolchain=toolchain_id,
                          compile_inputs={str(p): h for p, h in contexts[(source, flags)].items()},
                          object_sha256=objects[obj])
            results.append(result)
        if digest(obj.read_bytes()) != objects[obj]:
            raise ValueError('Object changed during verification')
    stable()
    if any(digest(p.read_bytes()) != h for p, h in objects.items()):
        raise ValueError('Object changed during verification')
    verify_toolchain.cache_clear()
    verify_toolchain(toolchain)
    stable()
    by_id = {r['id']: r for r in results}
    if len(by_id) != len(rows):
        raise ValueError('Duplicate recovery IDs')
    return [by_id[r['id']] for r in rows], dict(compiler_calls=len(groups), rows=len(rows),
                                               elapsed_seconds=time.perf_counter() - started)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["verify", "candidate", "object"])
    parser.add_argument("id", nargs="?")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--obj", type=Path)
    parser.add_argument("--toolchain", type=Path, default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--original-root", type=Path,
                        help="Read hash-pinned originals from this checkout; retain local canonical source, recovery state and strict gate")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    state_before = (ROOT / "recovery.json").read_bytes()
    state = json.loads(state_before)
    rows = [r for r in state["functions"] if r["id"] == args.id] if args.id else state["functions"]
    if args.id and not rows:
        raise SystemExit(f"Unknown function {args.id}")
    if args.command == "object":
        if len(rows) != 1 or args.obj is None:
            parser.error("object requires one function id and --obj")
        row = rows[0]
        target = (args.original_root.resolve() if args.original_root else ROOT) / row["target"]
        if digest(target.read_bytes()) != row["target_sha256"]:
            raise ValueError(f"Target hash mismatch: {target}")
        results = [compare(args.obj, row["symbol"], target, int(row["rva"], 0), row["size"])]
    elif args.command == "candidate":
        if len(rows) != 1 or args.source is None:
            parser.error("candidate requires one function id and --source")
        results = [check(rows[0], args.source, args.toolchain, args.original_root)]
    else:
        rows = [r for r in rows if r["status"] == "FUNCTION_MATCH"]
        if not rows:
            raise SystemExit("No accepted functions to verify")
        results, statistics = verify_rows(rows, args.toolchain, args.original_root)
        print(f"Fresh verification: {statistics['compiler_calls']} compiler calls for {statistics['rows']} rows; "
              f"{statistics['elapsed_seconds']:.3f} seconds")
    if (ROOT / "recovery.json").read_bytes() != state_before:
        raise ValueError('Recovery state changed during verification')
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
