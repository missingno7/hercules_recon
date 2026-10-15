"""Reproduce the REA (rea-agents) evaluation against the pinned originals.

  python prototypes/rea_eval/run_probe.py --rea C:\\tools\\rea-agents-6.5.0\\node_modules\\rea-agents\\scripts\\rea.mjs
      --ghidra C:\\tools\\ghidra_12.1_PUBLIC --exe <unpatched HERCULES.EXE> --eng1 <ENG1.DLL>
      --output work/rea_probe

REA opens each target as an ephemeral, source-immutable Ghidra import (it
copies the file into its own temp runtime). Raw MCP transcripts stay in
--output; only the compact summary is meant to be kept as evidence. REA is
never registered with an agent (`rea setup` is not run).
"""
import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
BASE = 0x400000
DOCUMENTED_AC60_CALLEES = {0x6620, 0x6EE0, 0x6FF0, 0x70C0, 0x7140, 0xA8F0, 0xBD80,
                           0xC8F0, 0xCAB0, 0xD420, 0xE680}
# ENG1 direct copied-slot transfers documented in renderer_contract/compatibility.md.
DOCUMENTED_ENG1_SITES = {80: 0x2C5F4, 81: 0x2C60A, 89: 0x2C625, 129: 0x2C7FA,
                         131: 0x2C81F, 143: 0x2C835}
ENG1_TABLE_ROOT = 0x10070F20


def session(args, plan, name, seeds=None):
    out = Path(args.output)
    plan_path = out / f"plan_{name}.json"
    log_path = out / f"session_{name}.jsonl"
    plan_path.write_text(json.dumps(plan))
    env = dict(os.environ, GHIDRA_INSTALL_DIR=args.ghidra)
    env.pop("REA_GHIDRA_SEED_FILE", None)
    if seeds:
        env["REA_GHIDRA_SEED_FILE"] = str(seeds)
    t0 = time.time()
    subprocess.run(["node", str(HERE / "rea_mcp.mjs"), args.rea, str(plan_path), str(log_path)],
                   env=env, check=True, capture_output=True)
    elapsed = time.time() - t0
    recs = {}
    for line in log_path.read_text().splitlines():
        r = json.loads(line)
        recs[r["label"]] = r
    return recs, elapsed


def result(rec):
    return json.loads(rec["text"])["normalized_result"]


def procedures(rec):
    return {int(p["address"], 16) for p in result(rec) if not p["address"].startswith("EXT")}


def table_targets(exe_bytes):
    words = struct.unpack_from("<250I", exe_bytes, 0x361B8 - 0x35000 + 0x34000)
    return words


def main():
    ap = argparse.ArgumentParser()
    for k in ("rea", "ghidra", "exe", "eng1", "output"):
        ap.add_argument("--" + k, required=True)
    args = ap.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    exe = str(Path(args.exe).resolve())
    eng1 = str(Path(args.eng1).resolve())
    exe_bytes = Path(exe).read_bytes()

    seeds_table = out / "seeds_table.tsv"
    seeds_all = out / "seeds_all.tsv"
    for path, extra in ((seeds_table, ["--table-only"]), (seeds_all, [])):
        subprocess.run([sys.executable, str(HERE / "make_seeds.py"), exe, str(ROOT / "recovery.json"),
                        str(path)] + extra, check=True, capture_output=True)

    open_exe = {"tool": "open_binary", "args": {"path": exe, "provider_id": "ghidra"}}
    read_table = {"tool": "read_bytes", "args": {"address": "0x4361b8", "length": 1000}, "label": "table"}
    base_plan = [open_exe, {"tool": "list_procedures", "args": {}}, read_table,
                 {"tool": "resolve_containing_procedure", "args": {"address": "0x40ac60"}, "label": "ac60_contained"},
                 {"tool": "xrefs", "args": {"address": "0x40ac60"}, "label": "ac60_xrefs"}]
    summary = {"rea_version": None, "ghidra": args.ghidra, "exe_sha256": hashlib.sha256(exe_bytes).hexdigest(),
               "sessions": {}}

    words = None
    for name, seeds in (("unseeded", None), ("table_seeded", seeds_table), ("all_seeded", seeds_all)):
        recs, secs = session(args, base_plan, name, seeds)
        summary["rea_version"] = recs["initialize"]["result"]["version"]
        procs = procedures(recs["list_procedures"])
        if words is None:
            words = struct.unpack("<250I", bytes.fromhex(result(recs["table"])["bytes_hex"]))
        targets = {w for w in words if 0x401000 <= w < 0x42DE00}
        summary["sessions"][name] = {
            "seconds": round(secs, 1),
            "procedures": len(procs),
            "table_unique_text_targets": len(targets),
            "table_targets_recognized": len(targets & procs),
            "ac60_is_procedure": result(recs["ac60_contained"])["found"],
            "ac60_data_xrefs": result(recs["ac60_xrefs"]),
        }

    # Calibration on callback 89 (seeded) plus the 0x79A0 owner investigation.
    plan = [open_exe,
            {"tool": "analyze_function", "args": {"procedure": "0x40ac60"}, "label": "ac60"},
            {"tool": "procedure_pseudo_code", "args": {"procedure": "0x40ac60"}, "label": "ac60_pc"},
            {"tool": "procedure_pseudo_code", "args": {"procedure": "0x4079a0"}, "label": "idle_pc"},
            {"tool": "read_function_instructions", "args": {"procedure": "0x4079a0"}, "label": "idle_ins"},
            {"tool": "read_bytes", "args": {"address": "0x42e170", "length": 0x110}, "label": "vt_bytes"}]
    for a in range(0x42E100, 0x42E2A0, 4):
        plan.append({"tool": "xrefs", "args": {"address": hex(a)}, "label": f"x{a:x}"})
    recs, secs = session(args, plan, "calibration", seeds_all)
    dossier = result(recs["ac60"])
    callees = {int(c["address"], 16) - BASE for c in dossier["callees"]}
    facet_bytes = {k: len(json.dumps(v)) for k, v in dossier.items()}
    summary["ac60_calibration"] = {
        "seconds": round(secs, 1),
        "body": dossier["procedure"]["body"]["ranges"],
        "callees_match_documented": callees == DOCUMENTED_AC60_CALLEES,
        "unresolved_calls": dossier["unresolved_calls"],
        "reported_calling_convention": dossier["native_api"]["calling_convention"],
        "dossier_bytes": len(recs["ac60"]["text"]),
        "largest_facets": dict(sorted(facet_bytes.items(), key=lambda kv: -kv[1])[:4]),
        "pseudocode_bytes": len(result(recs["ac60_pc"])),
    }
    span_refs = {k[1:]: result(r) for k, r in recs.items() if k.startswith("x4") and result(r)}
    ins = result(recs["idle_ins"])["instructions"]
    summary["idle_owner_probe"] = {
        "references_into_0x42e100_0x42e2a0": span_refs,
        "idle_last_instruction": ins[-1] if isinstance(ins[-1], str) else ins[-1],
        "idle_pseudocode_head": result(recs["idle_pc"]).strip().splitlines()[:2],
    }

    # ENG1: independent-engine view of the copied-table slots.
    plan = [{"tool": "open_binary", "args": {"path": eng1, "provider_id": "ghidra"}},
            {"tool": "list_procedures", "args": {}}]
    plan += [{"tool": "xrefs", "args": {"address": hex(ENG1_TABLE_ROOT + 4 * s)}, "label": f"s{s}"}
             for s in range(250)]
    recs, secs = session(args, plan, "eng1")
    slot_sites = {int(k[1:]): [int(a, 16) - 0x10000000 for a in result(r)]
                  for k, r in recs.items() if k.startswith("s") and k[1:].isdigit() and result(r)}
    recovery = json.loads((ROOT / "recovery.json").read_text(encoding="utf-8"))["functions"]
    eng1_entries = [f for f in recovery if f["target"].endswith("ENG1.DLL")]
    procs = procedures(recs["list_procedures"])
    summary["eng1"] = {
        "seconds": round(secs, 1),
        "procedures": len(procs),
        "slots_referenced": len(slot_sites),
        "slots_with_multiple_sites": [s for s, v in slot_sites.items() if len(v) != 1],
        "documented_sites_match": all(slot_sites.get(s) == [v] for s, v in DOCUMENTED_ENG1_SITES.items()),
        "slot92_sites": [hex(v) for v in slot_sites.get(92, [])],
        "recovery_entries": len(eng1_entries),
        "recovery_starts_recognized": sum(1 for f in eng1_entries if 0x10000000 + int(f["rva"], 16) in procs),
    }
    (out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")
    print(json.dumps(summary, indent=1))


if __name__ == "__main__":
    main()
