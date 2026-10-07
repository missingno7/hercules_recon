"""Fingerprint reviewed shared functions and the secondary PSX decimal counterpart.

This reads evidence only. It never compiles, promotes, or awards recovery status.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct

from env import ROOT
from pe import PE
from capstone import Cs, CS_ARCH_MIPS, CS_MODE_MIPS32, CS_MODE_LITTLE_ENDIAN


def sha(data):
    return hashlib.sha256(data).hexdigest()


def psx_decimal(revision, manifest):
    entry = next(e for e in manifest["revisions"][revision]["executables"] if e["path"] == "EX/TITLE")
    path = ROOT / "work/discs" / ("psx_" + revision) / "EX/TITLE"
    data = path.read_bytes()
    if sha(data) != entry["sha256"]:
        raise ValueError(f"PSX executable identity changed: {path}")
    load = struct.unpack_from("<I", data, 0x18)[0]
    hits = [offset for offset in range(0x800, len(data) - 4, 4)
            if struct.unpack_from("<II", data, offset) == (0x3c021062, 0x34424dd3)]
    if len(hits) != 1:
        raise ValueError("Expected one reviewed division-by-1000 sequence")
    offset = hits[0] - 12
    address = load + offset - 0x800
    decoder = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    instructions = list(decoder.disasm(data[offset:offset + 512], address))
    if [i.mnemonic for i in instructions[:3]] != ["move", "move", "lw"]:
        raise ValueError("Decimal entry-context hypothesis changed")
    last = next(index + 1 for index, ins in enumerate(instructions) if ins.mnemonic == "jr" and ins.op_str == "$ra")
    instructions = instructions[:last + 1]  # include the return's delay slot
    size = instructions[-1].address + 4 - address
    if size != 388:
        raise ValueError("Reviewed PSX function extent changed")
    def normalize(match):
        value = int(match[0], 16)
        return f"local+{value-address:#x}" if address <= value < address + size else match[0]
    normalized = [i.mnemonic + " " + re.sub(r"0x[0-9a-f]+", normalize, i.op_str) for i in instructions]
    return {
        "revision": revision, "path": path.relative_to(ROOT).as_posix(),
        "executable_sha256": entry["sha256"], "file_offset": hex(offset),
        "address": hex(address), "body_size": size,
        "body_sha256": sha(data[offset:offset + size]),
        "internal_target_normalized_disassembly_sha256": sha("\n".join(normalized).encode()),
    }


def collect():
    state = json.loads((ROOT / "recovery.json").read_text())
    images = {}
    for row in state["functions"]:
        if row["target"] not in images:
            images[row["target"]] = PE(ROOT / row["target"])
        if sha(images[row["target"]].data) != row["target_sha256"]:
            raise ValueError("PC oracle identity changed")
    groups = defaultdict(list)
    for row in state["functions"]:
        if row["status"] != "FUNCTION_MATCH":
            continue
        code = images[row["target"]].read_rva(int(row["rva"], 0), row["size"])
        if sha(code) != row["proof"]["compiled_sha256"]:
            raise ValueError("Recorded compiled proof and target interval differ")
        groups[(row["source"], row["symbol"])].append({
            "id": row["id"], "module": Path(row["target"]).name,
            "rva": row["rva"], "body_size": row["body_size"], "size_with_padding": row["size"],
            "target_sha256": row["target_sha256"], "contribution_sha256": sha(code)})
    rows = {r["id"]: r for r in state["functions"]}
    left, right = [rows[key] for key in ("eng1.measure_relative_vector", "eng3.measure_relative_vector")]
    codes = [images[r["target"]].read_rva(int(r["rva"], 0), r["size"]) for r in (left, right)]
    differences = [{"offset": hex(i), "eng1": hex(a), "eng3": hex(b)}
                   for i, (a, b) in enumerate(zip(*codes)) if a != b]
    psx_manifest = json.loads((ROOT / "evidence/psx.json").read_text())
    decimal = [psx_decimal(revision, psx_manifest) for revision in ("usa_original", "usa_rerelease")]
    return {
        "schema_version": 1,
        "scope": "Measured correspondence; recovery.json alone controls acceptance. No PSX matching target.",
        "exact_contribution_groups": [dict(source=source, symbol=symbol, instances=instances)
                                      for (source, symbol), instances in groups.items()],
        "relative_vector_variant": {
            "eng1_rva": left["rva"], "eng3_rva": right["rva"], "size": left["size"],
            "differences": differences,
            "interpretation": "Only the two short-shift immediates 2 and 3 exchange positions. The same source is exact for ENG1, not ENG3. Source-spelling versus translation-unit/compiler context is unresolved."
        },
        "decimal_correspondence": {
            "pc_id": "title.decimal_digits", "pc_rva": rows["title.decimal_digits"]["rva"],
            "psx": decimal,
            "psx_revisions_equal_after_internal_target_normalization": decimal[0]["internal_target_normalized_disassembly_sha256"] == decimal[1]["internal_target_normalized_disassembly_sha256"],
            "reviewed_semantics": [
                "Four sprite pointers and a signed integer fifth argument; no calls.",
                "Signed division by 1000, 100 and 10; subtract products to obtain digits.",
                "16-bit frame fields at offset 0x34; all four add 90 after digit writes.",
                "32-bit flags at offset 0x54; bit 31 cleared/set for value thresholds 1000, 100, 10.",
                "PSX retains separate place-value products and repeated subtraction from the original value.",
                "These features support the PC source hypothesis but do not resolve the PC code-generation mismatch."
            ]
        }
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build/shared_evidence.json")
    args = parser.parse_args()
    if not any(args.output.resolve().is_relative_to((ROOT / folder).resolve()) for folder in ("build", "work", "evidence")):
        parser.error("Output must be under build/, work/ or evidence/")
    report = collect()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes((json.dumps(report, indent=2) + "\n").encode())
    print(f"{len(report['exact_contribution_groups'])} source functions; "
          f"{sum(len(g['instances']) for g in report['exact_contribution_groups'])} exact module instances.")
    print("Vector variant: " + str(report["relative_vector_variant"]["differences"]))
    print("PSX decimal entries: " + ", ".join(p["address"] for p in report["decimal_correspondence"]["psx"]))
