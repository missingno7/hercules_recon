"""Read-only direct-reference audit for the host event/export dispatch gates."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

EXPECTED = {
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
TARGETS = {
    "event_enable_ba0238": 0x00BA0238,
    "event_suppress_ba0228": 0x00BA0228,
    "export_pointer_91c45c": 0x0091C45C,
    "module_resolver_gate_91c460": 0x0091C460,
    # This host cell is read/written by the bounded resolver path below and is
    # needed to qualify when the 0x91c460=1 writer can be reached.
    "resolver_in_progress_91c464_auxiliary": 0x0091C464,
}
CODE_TARGETS = {
    "host_init_5e10": 0x5E10,
    "graphics_setup_72a0": 0x72A0,
    "frame_host_initializer_1ff0": 0x1FF0,
    "host_callback_service_4040": 0x4040,
    "module_loader_5cd0": 0x5CD0,
    "module_resolver_5c20": 0x5C20,
    "event_callback_79a0": 0x79A0,
    "engine_dispatch_5ec0": 0x5EC0,
    "timer_dispatch_5f20": 0x5F20,
    "event_enable_writer_9820": 0x9820,
    "event_enable_writer_9840": 0x9840,
    "event_suppress_writer_9870": 0x9870,
}
HOST_SPANS = {
    "host_init_5e10": (0x5E10, 0x7F),
    "engine_dispatch_wrapper_5ec0": (0x5EC0, 0x54),
    "timer_dispatch_5f20": (0x5F20, 0xAD),
    "event_callback_79a0": (0x79A0, 0xFB),
    "event_gate_and_tick_79e0": (0x79E0, 0x25),
    "frame_host_initializer_1ff0": (0x1FF0, 0xC1),
    "host_callback_service_4040": (0x4040, 0x90),
    "module_resolver_5c20": (0x5C20, 0xAA),
    "module_loader_5cd0": (0x5CD0, 0x13C),
    "event_enable_writer_9820": (0x9820, 0x1E),
    "event_enable_writer_9840": (0x9840, 0x21),
    "event_suppress_writer_9870": (0x9870, 0x82),
    "callback_address_context_2e220": (0x2E220, 0x60),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def hx(value: int) -> str:
    return hex(value & 0xFFFFFFFF)


def rva_span(pe, rva: int, size: int) -> dict:
    return {"rva": hx(rva), "size": size, "sha256": sha(pe.read_rva(rva, size))}


def target_initial(pe, va: int, size: int = 4) -> dict:
    rva = va - pe.image_base
    for section in pe.sections:
        start = section["virtual_address"]
        raw_end = start + section["raw_size"]
        virtual_end = start + max(section["virtual_size"], section["raw_size"])
        if start <= rva and rva + size <= raw_end:
            raw = pe.read_rva(rva, size)
            return {"va": hx(va), "rva": hx(rva), "size": size,
                    "section": section["name"], "storage": "file_backed",
                    "bytes_hex": raw.hex(), "u32": hx(struct.unpack("<I", raw)[0]),
                    "bytes_sha256": sha(raw)}
        if start <= rva and rva + size <= virtual_end and rva >= raw_end:
            return {"va": hx(va), "rva": hx(rva), "size": size,
                    "section": section["name"], "storage": "zero_fill_virtual_tail",
                    "bytes_hex": "00" * size, "u32": "0x0",
                    "bytes_sha256": sha(b"\0" * size),
                    "limit": "PE virtual zero-fill initialization; runtime writes/aliases remain separate."}
    return {"va": hx(va), "rva": hx(rva), "size": size,
            "storage": "outside_host_mapped_sections", "initial_value": None}


def direct_memory_rows(pe, capstone, module: str, raw_text: bytes,
                       text_rva: int, targets: dict[str, int],
                       address_ranges: list[tuple[int, int]]) -> tuple[dict, dict]:
    base = pe.image_base
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    records = list(md.disasm(raw_text, base + text_rva))
    decoded = [i for i in records if i.id != 0 and i.mnemonic != ".byte"]
    coverage = sum(i.size for i in records)
    refs = {name: [] for name in targets}
    address_refs = []
    anchors = []
    for insn in decoded:
        rva = insn.address - base
        for index, operand in enumerate(insn.operands):
            if operand.type == capstone.x86.X86_OP_MEM:
                mem = operand.mem
                disp = mem.disp & 0xFFFFFFFF
                # A fixed-address operand has no base or index register. Record
                # both exact target overlaps and nearby immediate-base aliases.
                absolute = mem.base == 0 and mem.index == 0
                width = max(1, int(operand.size or 1))
                access = ("read_write" if operand.access & 3 == 3 else
                          "write" if operand.access & 2 else
                          "read" if operand.access & 1 else "unknown")
                if absolute:
                    for name, target in targets.items():
                        if disp < target + 4 and target < disp + width:
                            refs[name].append({
                                "module": module, "site_rva": hx(rva),
                                "instruction_size": insn.size,
                                "instruction_sha256": sha(bytes(insn.bytes)),
                                "mnemonic": insn.mnemonic, "op_str": insn.op_str,
                                "operand_index": index, "operand_size": width,
                                "memory_va": hx(disp), "access": access,
                                "operand_bytes_hex": bytes(insn.bytes).hex(),
                            })
                    if any(lo <= disp < hi for lo, hi in address_ranges):
                        address_refs.append({
                            "module": module, "site_rva": hx(rva),
                            "instruction_size": insn.size,
                            "instruction_sha256": sha(bytes(insn.bytes)),
                            "mnemonic": insn.mnemonic, "op_str": insn.op_str,
                            "memory_va": hx(disp), "access": access,
                            "operand_bytes_hex": bytes(insn.bytes).hex(),
                        })
            elif operand.type == capstone.x86.X86_OP_IMM:
                value = operand.imm & 0xFFFFFFFF
                if any(lo <= value < hi for lo, hi in address_ranges):
                    anchors.append({
                        "module": module, "site_rva": hx(rva),
                        "instruction_size": insn.size,
                        "instruction_sha256": sha(bytes(insn.bytes)),
                        "mnemonic": insn.mnemonic, "op_str": insn.op_str,
                        "immediate": hx(value),
                        "operand_bytes_hex": bytes(insn.bytes).hex(),
                    })
    sweep = {
        "section": ".text", "section_rva": hx(text_rva),
        "raw_size": len(raw_text), "record_count": len(records),
        "decoded_instruction_count": len(decoded),
        "skipdata_record_count": len(records) - len(decoded),
        "covered_bytes": coverage, "coverage_complete": coverage == len(raw_text),
        "limit": "Complete raw-section linear sweep, not proof of instruction alignment; embedded data may decode as instructions.",
    }
    return {"targets": refs, "address_references": address_refs,
            "immediate_anchors": anchors}, sweep


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", required=True, type=Path,
                        help="checkout with scripts/pe.py")
    parser.add_argument("--image-dir", required=True, type=Path,
                        help="directory holding the four pinned original module files")
    parser.add_argument("--output", required=True, type=Path,
                        help="compact JSON receipt under original-root/work")
    args = parser.parse_args()
    root = args.original_root.resolve()
    image_dir = args.image_dir.resolve()
    output = args.output.resolve()
    work = (root / "work").resolve()
    if work != root / "work":
        parser.error("refusing a redirected work directory")
    if not (root / "scripts" / "pe.py").is_file():
        parser.error("--original-root must contain scripts/pe.py")
    try:
        output.relative_to(work)
    except ValueError:
        parser.error("--output must stay inside original-root/work")
    if not image_dir.is_dir():
        parser.error("--image-dir must be an existing directory")
    if output.is_relative_to(image_dir):
        parser.error("--output must not be inside the original image directory")

    sys.path.insert(0, str(root / "scripts"))
    import env  # noqa: F401,E402
    import capstone  # noqa: E402
    from pe import PE  # noqa: E402

    modules = {}
    for module, expected in EXPECTED.items():
        path = image_dir / module
        data = path.read_bytes()
        digest = sha(data)
        if digest != expected:
            parser.error(f"{module} does not match pinned original SHA-256: {digest}")
        pe = PE(path)
        text = next(s for s in pe.sections if s["name"] == ".text")
        raw = pe.read_rva(text["virtual_address"], text["raw_size"])
        modules[module] = {"path": str(path), "size": len(data), "sha256": digest,
                           "image_base": hx(pe.image_base), "pe": pe,
                           "text_raw": raw, "text_rva": text["virtual_address"],
                           "sections": pe.sections}

    host = modules["HERCULES.EXE"]["pe"]
    base = host.image_base
    address_ranges = [(va - 0x80, va + 0x84) for va in TARGETS.values()]
    target_initializers = {name: target_initial(host, va)
                           for name, va in TARGETS.items()}
    module_refs = {}
    sweeps = {}
    direct_calls = {module: {name: [] for name in CODE_TARGETS}
                    for module in modules}
    code_literal_hits = {name: [] for name in CODE_TARGETS}
    for module, row in modules.items():
        module_refs[module], sweeps[module] = direct_memory_rows(
            row["pe"], capstone, module, row["text_raw"], row["text_rva"],
            TARGETS, address_ranges)
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        md.detail = True
        md.skipdata = True
        for insn in md.disasm(row["text_raw"], row["pe"].image_base + row["text_rva"]):
            if insn.id == 0 or insn.mnemonic != "call" or not insn.operands:
                continue
            op = insn.operands[0]
            if op.type == capstone.x86.X86_OP_IMM:
                dest = (op.imm - row["pe"].image_base) & 0xFFFFFFFF
                for name, target_rva in CODE_TARGETS.items():
                    if dest == target_rva:
                        direct_calls[module][name].append({
                            "site_rva": hx(insn.address - row["pe"].image_base),
                            "target_rva": hx(target_rva),
                            "instruction_sha256": sha(bytes(insn.bytes)),
                            "bytes_hex": bytes(insn.bytes).hex(),
                        })
        needle_by_name = {name: struct.pack("<I", row["pe"].image_base + target_rva)
                          for name, target_rva in CODE_TARGETS.items()}
        for section in row["pe"].sections:
            contents = row["pe"].read_rva(section["virtual_address"], section["raw_size"])
            for name, needle in needle_by_name.items():
                pos = 0
                while True:
                    hit = contents.find(needle, pos)
                    if hit < 0:
                        break
                    code_literal_hits[name].append({
                        "module": module, "section": section["name"],
                        "rva": hx(section["virtual_address"] + hit),
                    })
                    pos = hit + 1

    callback_va = base + 0x79A0
    table_va = base + 0x2E240
    table_context = host.read_rva(0x2E220, 0x60)
    dwords = []
    for offset in range(0, len(table_context), 4):
        value = struct.unpack_from("<I", table_context, offset)[0]
        target_section = None
        target_rva = value - base
        for section in host.sections:
            if section["virtual_address"] <= target_rva < section["virtual_address"] + max(section["raw_size"], section["virtual_size"]):
                target_section = section["name"]
                break
        dwords.append({"rva": hx(0x2E220 + offset), "value": hx(value),
                       "points_into_host_section": target_section,
                       "callback_target_match": value == callback_va})
    raw_literal = struct.pack("<I", callback_va)
    literal_hits = []
    for module, row in modules.items():
        pe = row["pe"]
        for section in pe.sections:
            contents = pe.read_rva(section["virtual_address"], section["raw_size"])
            pos = 0
            while True:
                hit = contents.find(raw_literal, pos)
                if hit < 0:
                    break
                literal_hits.append({"module": module, "section": section["name"],
                                     "rva": hx(section["virtual_address"] + hit)})
                pos = hit + 1

    pinned_spans = {}
    for name, (rva, size) in HOST_SPANS.items():
        pinned_spans[name] = {"module": "HERCULES.EXE", "name": name,
                              **rva_span(host, rva, size)}
    table_ref_ranges = [(table_va - 0x40, table_va + 0x64)]
    table_refs, _ = direct_memory_rows(
        host, capstone, "HERCULES.EXE", modules["HERCULES.EXE"]["text_raw"],
        modules["HERCULES.EXE"]["text_rva"], {}, table_ref_ranges)

    result = {
        "schema_version": 1,
        "scope": "Read-only PE static audit of four pinned module .text sections, selected host gate cells, the cold dispatcher, and the bounded .rdata address-literal context. No process launch, code execution, file patching, alias resolution, or framework scheduling claim.",
        "audit_script_sha256": sha(Path(__file__).read_bytes()),
        "prediction_and_falsifier": {
            "prediction": "Every reviewed direct writer capable of enabling cold dispatch is ordered after the forwarded DirectDraw call returns, so the known gate path blocks earlier dispatch within this static scope.",
            "falsifier": "An initially enabled gate/export combination, an enabling writer reachable before the forwarded DirectDraw call returns, an unbounded alias writer, or a direct bypass to export dispatch.",
            "outcome": "UNRESOLVED_AS_A_GLOBAL_ORDERING_CLAIM: the normal direct initialization path writes the export pointer after DirectDraw returns, but the event-enable and suppression setter entries have no direct callsites and only file-backed indirect-address clues. Their invocation order is not established.",
            "limits": "A separate CFG does not prove pre-initialization execution; absence of direct early writers does not exclude aliases or reentrancy; a callback address literal does not prove invocation order.",
        },
        "originals": {m: {k: v for k, v in row.items() if k in ("path", "size", "sha256", "image_base")}
                      for m, row in modules.items()},
        "full_raw_text_sweeps": sweeps,
        "initial_values": target_initializers,
        "direct_references": module_refs,
        "direct_callers_by_entry_candidate": direct_calls,
        "file_backed_code_address_literals": code_literal_hits,
        "static_order_assessment": {
            "initial_cold_state": "The five inspected host .data virtual-tail cells are zero at image initialization: BA0238, BA0228, 91C45C, 91C460, and auxiliary 91C464. Thus event entry initially skips the tick (BA0238=0), and the export dispatcher initially skips the export call (91C45C=0).",
            "normal_direct_initialization": "The bounded direct chain is 7937->5E10; 5E10 calls 72A0 at 5E2C; 72A0 reaches DirectDrawCreate at 7426 and returns; 5E10 later calls 1FF0 at 5E40; 1FF0 calls host callback service 4040 at 20A6; 4040 calls module loader 5CD0 at 40C4. With 91C464=0, 5CD0 reaches 5C20 at 5DFE, which calls LoadLibraryA/GetProcAddress and stores the export pointer at 5C9D. Therefore the observed normal direct initialization path publishes 91C45C after the forwarded DirectDraw call returns.",
            "independent_event_route": "79A0 reaches 5F20 only after BA0238 is nonzero and BA0228 is zero; 5F20 calls 5EC0. 5EC0 sets auxiliary 91C464=1, calls 41161C, reloads 91C460, calls 5C20 at 5EE5 only if that gate is nonzero, then calls the export only if 91C45C is nonzero; it clears 91C464 at 5F08. Initial zeros therefore block the cold route at startup.",
            "resolver_gate_writer": "The direct writer 5DE3 sets 91C460=1 only inside 5CD0 when 91C464 is nonzero. The other direct writer 5C28 clears 91C460 at entry to 5C20. Direct callers to 5CD0 in the full raw .text candidate scan contain only 40C4, which is on the post-DirectDraw 5E10->1FF0->4040 initialization chain. Direct callers to 5C20 are 5DFE and the gated 5EE5 path.",
            "event_enable_writers": "BA0238 may be enabled by 9820 (stores 1 or 2 from its argument) and 9840 (stores 1). These entries have no direct .text callsites in the four pinned modules, but their addresses occur as file-backed literals in HERCULES.EXE .rdata at 2E5CC and 2E62C. Their indirect invocation timing is not recovered. The 9870 entry similarly contains the BA0228 0/1 writer and occurs at .rdata 2E644.",
            "callback_literal": "The absolute code address 4079A0 occurs at .rdata RVA 2E240 in a local run of dwords that point into .text. The bounded span has no direct instruction reference to the 42E240 table address; adjacent immediate references are to 42E280 and 42E290. This is compatible with function-pointer metadata but does not identify registration semantics or invocation order.",
            "conclusion": "SUPPORTED_WITHIN_DIRECT_CALL_FLOW_ONLY: initial values close the cold route, and the observed normal direct initialization publishes the export pointer after DirectDrawCreate returns. NOT_PROVEN_PROCESS_WIDE: event-enable callbacks have only indirect address-literal evidence; an indirect/reentrant route into 5CD0 or 5C20, register-computed aliases, or host framework scheduling could alter the gates earlier. The independent CFG alone neither proves early invocation nor eliminates it.",
            "direct_sweep_limit": "Direct callers are a complete raw .text linear-sweep candidate inventory with skipdata; they are not proof that every decoded byte is instruction-aligned. The report does not follow register aliases, vtables, callback arguments, or external API reentrancy.",
        },
        "callback_literal_context": {
            "host_section": ".rdata", "context_rva": "0x2e220",
            "context_size": len(table_context), "context_sha256": sha(table_context),
            "callback_address_value": hx(callback_va), "address_literal_hits": literal_hits,
            "dwords": dwords,
            "text_references_to_context_address_range": table_refs["address_references"],
            "text_immediate_anchors_to_context_address_range": table_refs["immediate_anchors"],
            "limit": "Neighboring dwords that equal host .text addresses are address-table evidence only; no MFC/framework consumer or runtime callback order is inferred.",
        },
        "pinned_host_spans": pinned_spans,
        "provenance": {"pe_helper_path": str(root / "scripts" / "pe.py"),
                       "pe_helper_sha256": sha((root / "scripts" / "pe.py").read_bytes()),
                       "capstone_version": capstone.__version__,
                       "linear_sweep_limit": "Direct absolute operands only for the target cells; immediate anchors in nearby address ranges are retained as alias clues, not traced. Register-computed writes, callback aliases, runtime initialization, indirect calls, and host scheduling are unresolved."},
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha(output.read_bytes()),
                      "original_modules": len(modules),
                      "text_sweeps_complete": all(x["coverage_complete"] for x in sweeps.values()),
                      "target_direct_ref_counts": {
                          module: {name: len(rows) for name, rows in record["targets"].items()}
                          for module, record in module_refs.items()},
                      "table_literal_hits": literal_hits}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
