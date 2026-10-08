"""Fresh archive-name diagnostics retaining the full inline table; never acceptance."""
import argparse
import hashlib
import json
import struct
import sys
import time
from difflib import SequenceMatcher
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from coff import COFF
from env import ROOT as PROJECT_ROOT
from macro_compare import structure, tokens_and_calls
from match import compile_source, compare, digest
from pe import PE


def rva(value):
    return int(value, 0)


def evaluate(source, spec_path, output, obj=None):
    spec = json.loads(spec_path.read_text(encoding="utf-8-sig"))
    target = PROJECT_ROOT / spec["target"]
    pe = PE(target)
    if digest(pe.data) != spec["target_sha256"]:
        raise ValueError("Oracle identity changed")

    started = time.perf_counter()
    source_before = source.read_bytes()
    command = None
    if obj is None:
        obj, command = compile_source(source, spec["flags"])
    coff = COFF(obj)
    rows = []

    for function in spec["functions"]:
        name = function["symbol"]
        start = rva(function["rva"])
        raw = compare(obj, name, target, start, function["size"])
        if raw["target_sha256"] != function["target_sha256"]:
            raise ValueError("Reviewed target extent changed: " + name)
        contribution = coff.function(name)
        candidate_symbol = next(
            symbol for symbol in coff.symbols.values()
            if symbol["name"] == name and symbol["section"] > 0
        )
        diagnostic = bytearray(contribution["data"])
        resolved = []
        unresolved = []

        for relocation in contribution["relocations"]:
            offset = relocation["offset"]
            symbol_name = relocation["symbol"]
            addend = struct.unpack_from("<I", diagnostic, offset)[0]
            destination = None
            resolution = None
            if symbol_name in spec.get("symbol_rvas", {}):
                destination = rva(spec["symbol_rvas"][symbol_name])
                resolution = "reviewed target RVA mapping"
            else:
                local = [
                    symbol for symbol in coff.symbols.values()
                    if symbol["name"] == symbol_name
                    and symbol["section"] == candidate_symbol["section"]
                ]
                if len(local) == 1:
                    destination = start + local[0]["value"]
                    resolution = "same-function COMDAT symbol"

            if destination is None:
                unresolved.append({
                    "offset": offset,
                    "symbol": symbol_name,
                    "type": relocation["type"],
                    "reason": "no reviewed target mapping or unique same-COMDAT symbol",
                })
                continue

            if relocation["type"] == 0x14:
                value = destination - start - offset - 4 + addend
            elif relocation["type"] == 6:
                value = pe.image_base + destination + addend
            else:
                unresolved.append({
                    "offset": offset,
                    "symbol": symbol_name,
                    "type": relocation["type"],
                    "reason": "unsupported relocation kind",
                })
                continue
            struct.pack_into("<I", diagnostic, offset, value & 0xffffffff)
            resolved.append({
                "offset": offset,
                "symbol": symbol_name,
                "type": relocation["type"],
                "resolution": resolution,
            })

        original = pe.read_rva(start, function["size"])
        normalized_differences = sum(
            diagnostic[index:index + 1] != original[index:index + 1]
            for index in range(max(len(diagnostic), len(original)))
        )
        has_inline_data = bool(function.get("data_spans"))
        if has_inline_data:
            instruction_percent = None
            cfg_equal = None
            original_calls = None
            candidate_calls = None
        else:
            original_tokens, original_calls, _ = tokens_and_calls(
                original, pe.image_base + start
            )
            candidate_tokens, candidate_calls, _ = tokens_and_calls(
                bytes(diagnostic), pe.image_base + start
            )
            lcs = sum(
                block.size for block in SequenceMatcher(
                    None, original_tokens, candidate_tokens, autojunk=False
                ).get_matching_blocks()
            )
            instruction_percent = round(
                100 * lcs / max(len(original_tokens), len(candidate_tokens)), 3
            )
            cfg_equal = structure(original, pe.image_base + start) == structure(
                bytes(diagnostic), pe.image_base + start
            )

        rows.append({
            "symbol": name,
            "rva": function["rva"],
            "target_size": raw["target_size"],
            "candidate_size": raw["compiled_size"],
            "target_sha256": raw["target_sha256"],
            "candidate_contribution_sha256": digest(contribution["data"]),
            "strict_equal": raw["exact"],
            "raw_different_bytes": raw["different_bytes"],
            "raw_mismatch_regions": raw["mismatch_regions"],
            "normalized_different_bytes_diagnostic_only": normalized_differences,
            "normalized_evaluation_complete": not unresolved,
            "target_relocation_count": len(raw["target_relocations"]),
            "candidate_relocation_count": len(contribution["relocations"]),
            "resolved_relocations": resolved,
            "unresolved_relocations": unresolved,
            "inline_data_spans": function.get("data_spans", []),
            "instruction_percent": instruction_percent,
            "ordered_cfg_equal": cfg_equal,
            "target_direct_calls": original_calls,
            "candidate_direct_calls": candidate_calls,
        })

    if source.read_bytes() != source_before:
        raise ValueError("Source changed during evaluation")

    report = {
        "schema_version": 1,
        "scope": "Full raw COFF contribution comparison with diagnostic-only relocation resolution. The normalizer inline table is retained in raw bytes and excluded from linear CFG/instruction metrics.",
        "source": str(source.relative_to(PROJECT_ROOT)),
        "source_sha256": digest(source_before),
        "source_bytes": len(source_before),
        "toolchain": "msvc5_rtm",
        "flags": spec["flags"],
        "command": command,
        "object": str(Path(obj).resolve()),
        "object_sha256": digest(Path(obj).read_bytes()),
        "target_sha256": spec["target_sha256"],
        "functions": rows,
        "elapsed_seconds": round(time.perf_counter() - started, 3),
        "summary": {
            "functions": len(rows),
            "exact": sum(row["strict_equal"] for row in rows),
            "full_bytes": sum(row["target_size"] for row in rows),
            "candidate_full_bytes": sum(row["candidate_size"] for row in rows),
            "normalized_equal_diagnostic_only": sum(
                row["normalized_different_bytes_diagnostic_only"] == 0
                and row["normalized_evaluation_complete"]
                for row in rows
            ),
        },
    }
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["summary"]))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--spec", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--object", type=Path)
    args = parser.parse_args()
    source = args.source.resolve()
    spec = args.spec.resolve()
    output = args.output.resolve()
    if not any(output.is_relative_to(PROJECT_ROOT / folder) for folder in ("work", "build", "candidates")):
        parser.error("Generated reports belong under work/, build/ or candidates/")
    output.parent.mkdir(parents=True, exist_ok=True)
    evaluate(source, spec, output, args.object.resolve() if args.object else None)


if __name__ == "__main__":
    main()
