"""Regenerate a compact-query SQLite index from immutable PC originals.

Linear decode references are leads, not proven instructions/function boundaries.
The database is disposable; reviewed findings live in evidence/source_map.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import struct

from env import ROOT
from pe import PE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

MODULES = ("HERCULES.EXE", "ENG1.DLL", "ENG3.DLL", "TITLE.DLL")
DEFAULT_DB = ROOT / "work/archaeology.sqlite"
BASELINE_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"


def evidence_path(name):
    if name != "HERCULES.EXE":
        return ROOT / "assets/pc/HERCULES" / name
    path = ROOT / "work/discs/pc_install/HERCULES.EXE"
    if not path.is_file():
        raise ValueError("Extract the original PC CD installer into work/discs/pc_install first; the installed HERCULES.EXE is modified and is not the baseline")
    if hashlib.sha256(path.read_bytes()).hexdigest() != BASELINE_EXE_SHA256:
        raise ValueError("PC CD installer HERCULES.EXE hash mismatch; re-extract and verify the immutable CD evidence")
    return path


def decoder():
    result = Cs(CS_ARCH_X86, CS_MODE_32)
    result.detail = True
    result.skipdata = True
    return result


def strings(pe):
    """Search mapped non-code data, including resource UTF-16 strings."""
    for s in pe.sections:
        if s["characteristics"] & 0x20000000 or s["name"] == ".reloc":
            continue
        raw = pe.data[s["raw_offset"]:s["raw_offset"]+s["raw_size"]]
        for encoding, pattern in (("ascii", rb"[\x09\x0a\x0d\x20-\x7e]{5,}"),
                                  ("utf-16le", rb"(?:[\x09\x0a\x0d\x20-\x7e]\x00){5,}")):
            for match in re.finditer(pattern, raw):
                rva = s["virtual_address"] + match.start()
                yield (rva, len(match.group()), encoding, match.group().decode(encoding))


def build(dbpath):
    if not dbpath.resolve().is_relative_to((ROOT / "work").resolve()):
        raise ValueError("Generated archaeology databases must stay inside ignored work/")
    paths = {name: evidence_path(name) for name in MODULES}
    dbpath.parent.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(dbpath)
    db.executescript("""
        DROP TABLE IF EXISTS modules; DROP TABLE IF EXISTS strings;
        DROP TABLE IF EXISTS instructions; DROP TABLE IF EXISTS refs;
        CREATE TABLE modules(name TEXT PRIMARY KEY,path TEXT,sha256 TEXT,base INTEGER,metadata TEXT);
        CREATE TABLE strings(module TEXT,rva INTEGER,byte_length INTEGER,encoding TEXT,value TEXT);
        CREATE TABLE instructions(module TEXT,rva INTEGER,size INTEGER,hex TEXT,mnemonic TEXT,operands TEXT);
        CREATE TABLE refs(module TEXT,source_rva INTEGER,target_rva INTEGER,kind TEXT);
        CREATE INDEX strings_address ON strings(module,rva);
        CREATE INDEX refs_source ON refs(module,source_rva);
        CREATE INDEX refs_target ON refs(module,target_rva);
        CREATE INDEX instructions_address ON instructions(module,rva);
    """)
    for name in MODULES:
        path = paths[name]
        pe = PE(path)
        base = pe.image_base
        image_end = base + pe.pe.OPTIONAL_HEADER.SizeOfImage
        db.execute("INSERT INTO modules VALUES(?,?,?,?,?)",
                   (name, path.relative_to(ROOT).as_posix(), hashlib.sha256(pe.data).hexdigest(),
                    base, json.dumps(pe.metadata())))
        db.executemany("INSERT INTO strings VALUES(?,?,?,?,?)", ((name, *s) for s in strings(pe)))
        instructions, refs = [], set()
        for s in pe.sections:
            if not s["characteristics"] & 0x20000000:
                continue
            raw = pe.data[s["raw_offset"]:s["raw_offset"]+s["raw_size"]]
            for ins in decoder().disasm(raw, base + s["virtual_address"]):
                instructions.append((name, ins.address-base, ins.size, ins.bytes.hex(), ins.mnemonic, ins.op_str))
                if not ins.id:
                    continue
                for operand in ins.operands:
                    if operand.type == X86_OP_IMM:
                        target = operand.imm & 0xffffffff
                        kind = "linear_call" if ins.mnemonic == "call" else "linear_branch" if ins.mnemonic.startswith("j") else "linear_immediate"
                    elif operand.type == X86_OP_MEM:
                        target = operand.mem.disp & 0xffffffff
                        kind = "linear_memory"
                    else:
                        continue
                    if base <= target < image_end:
                        refs.add((name, ins.address-base, target-base, kind))
        # HIGHLOW relocations are more reliable absolute-pointer evidence than
        # scanning arbitrary four-byte windows. They can describe code or data.
        for block in getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", []):
            for relocation in block.entries:
                if relocation.type != 3:
                    continue
                target = struct.unpack("<I", pe.read_rva(relocation.rva, 4))[0]
                if base <= target < image_end:
                    refs.add((name, relocation.rva, target-base, "pe_highlow_pointer"))
        db.executemany("INSERT INTO instructions VALUES(?,?,?,?,?,?)", instructions)
        db.executemany("INSERT INTO refs VALUES(?,?,?,?)", sorted(refs))
        db.commit()
        count = db.execute("SELECT COUNT(*) FROM strings WHERE module=?", (name,)).fetchone()[0]
        print(f"{name}: {count} strings, {len(instructions)} linear instructions, {len(refs)} reference leads")
    db.close()


def open_verified(dbpath, module=None):
    db = sqlite3.connect(f"file:{dbpath.as_posix()}?mode=ro", uri=True)
    db.row_factory = sqlite3.Row
    rows = db.execute("SELECT * FROM modules" + (" WHERE name=?" if module else ""), (module,) if module else ()).fetchall()
    if not rows:
        raise ValueError(f"Module not indexed: {module}")
    for row in rows:
        if row["name"] == "HERCULES.EXE" and (row["sha256"] != BASELINE_EXE_SHA256 or
                row["path"] != "work/discs/pc_install/HERCULES.EXE"):
            raise ValueError("Index uses a non-baseline EXE; regenerate with archaeology.py index")
        if hashlib.sha256((ROOT / row["path"]).read_bytes()).hexdigest() != row["sha256"]:
            raise ValueError(f"Stale index for {row['name']}; regenerate after resolving evidence change")
    return db


def search(db, term, module, limit):
    query = "SELECT * FROM strings WHERE value LIKE ?"
    args = ["%" + term + "%"]
    if module:
        query += " AND module=?"
        args.append(module)
    query += " ORDER BY module,rva LIMIT ?"
    args.append(limit)
    result = []
    for row in db.execute(query, args):
        record = dict(row)
        record["rva"] = hex(row["rva"])
        record["references"] = [dict(r) for r in db.execute(
            "SELECT source_rva,kind FROM refs WHERE module=? AND target_rva>=? AND target_rva<? ORDER BY source_rva LIMIT 20",
            (row["module"], row["rva"], row["rva"]+row["byte_length"]))]
        result.append(record)
    return result


def context(db, module, rva, size):
    module_row = db.execute("SELECT * FROM modules WHERE name=?", (module,)).fetchone()
    pe = PE(ROOT / module_row["path"])
    base = pe.image_base
    end = rva + size
    instructions = [dict(rva=hex(i.address-base), hex=i.bytes.hex(), mnemonic=i.mnemonic, operands=i.op_str)
                    for i in decoder().disasm(pe.read_rva(rva, size), base+rva)]
    incoming = [dict(r) for r in db.execute(
        "SELECT source_rva,target_rva,kind FROM refs WHERE module=? AND target_rva>=? AND target_rva<? ORDER BY source_rva",
        (module, rva, end))]
    outgoing = [dict(r) for r in db.execute(
        "SELECT source_rva,target_rva,kind FROM refs WHERE module=? AND source_rva>=? AND source_rva<? ORDER BY source_rva",
        (module, rva, end))]
    referenced_strings = [dict(r) for r in db.execute(
        "SELECT DISTINCT s.rva,s.value FROM strings s JOIN refs r ON s.module=r.module "
        "AND r.target_rva>=s.rva AND r.target_rva<s.rva+s.byte_length "
        "WHERE r.module=? AND r.source_rva>=? AND r.source_rva<?", (module, rva, end))]
    return {"module": module, "rva": hex(rva), "size": size,
            "warning": "Explicit range, not an inferred function boundary. Linear-decode xrefs are leads; indirect calls are not resolved.",
            "instructions": instructions, "incoming_references": incoming,
            "outgoing_references": outgoing, "referenced_strings": referenced_strings}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", type=Path, default=DEFAULT_DB)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("index")
    strings_parser = commands.add_parser("strings")
    strings_parser.add_argument("term")
    strings_parser.add_argument("--module", choices=MODULES)
    strings_parser.add_argument("--limit", type=int, default=20)
    range_parser = commands.add_parser("context")
    range_parser.add_argument("module", choices=MODULES)
    range_parser.add_argument("rva", type=lambda n: int(n, 0))
    range_parser.add_argument("size", type=lambda n: int(n, 0))
    function_parser = commands.add_parser("function")
    function_parser.add_argument("id", help="Reviewed function ID from recovery.json")
    args = parser.parse_args()
    if args.command == "index":
        build(args.db)
    elif args.command == "function":
        state = json.loads((ROOT / "recovery.json").read_text())
        matches = [row for row in state["functions"] if row["id"] == args.id]
        if len(matches) != 1:
            parser.error("Unknown or ambiguous function ID")
        row = matches[0]
        module = Path(row["target"]).name
        db = open_verified(args.db, module)
        indexed = db.execute("SELECT sha256 FROM modules WHERE name=?", (module,)).fetchone()[0]
        if indexed != row["target_sha256"]:
            raise ValueError("Recovery state and index refer to different target identities")
        rva = int(row["rva"], 0)
        result = context(db, module, rva, row["size"])
        print(f"{row['id']} | {row['status']} | {module} RVA {row['rva']} | {row['size']} bytes")
        print(f"Source: {row['source']} ({row['symbol']}); flags: {' '.join(row['flags'])}")
        print(row["evidence"])
        for instruction in result["instructions"]:
            print(f"{instruction['rva']:>9}  {instruction['hex']:<20} {instruction['mnemonic']:<7} {instruction['operands']}")
        incoming = [r for r in result["incoming_references"] if not rva <= r["source_rva"] < rva + row["size"]]
        print("External incoming leads: " + ", ".join(hex(r["source_rva"]) for r in incoming[:20]))
        print("Outgoing leads: " + ", ".join(f"{hex(r['target_rva'])} ({r['kind']})" for r in result["outgoing_references"][:20]))
        for string in result["referenced_strings"][:20]:
            print(f"String {hex(string['rva'])}: {string['value']}")
        print(result["warning"])
        db.close()
    else:
        db = open_verified(args.db, args.module)
        if args.command == "strings":
            result = search(db, args.term, args.module, args.limit)
        else:
            if not 0 < args.size <= 65536:
                parser.error("size must be 1..65536 bytes")
            result = context(db, args.module, args.rva, args.size)
        print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
