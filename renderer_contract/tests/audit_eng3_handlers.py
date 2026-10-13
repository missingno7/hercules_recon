#!/usr/bin/env python3
"""Rerunnable, read-only audit for ENG3 handler-table entries 0 through 3."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import env  # noqa: F401 - installs the pinned local analysis dependencies
from pe import PE
import capstone
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM, X86_REG_ECX, X86_REG_EDX

LANE = Path(__file__).resolve().parent
PKG = ROOT / "renderer_contract" / "evidence"
TABLE_RVA = 0x35010
TABLE_TARGETS = [0x9030, 0x90E0, 0x22DE0, 0x22E40, 0x22F00, 0x23440]
TABLE_SHA256 = "22545ddc623f92f362d7b73001d2a6c293e03292253b20ee8be8bd970f742e73"
ENG3_SHA256 = "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7"
EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"

# Ranges are body boundaries only where the last decoded instruction is RET.
BODY_SPANS = {
    "handler_0": (0x9030, 0xA0),
    "handler_1_ft4_writer": (0x90E0, 0x33A),
    "handler_0_writer_a": (0x9420, 0x320),
    "handler_0_writer_b": (0x99B0, 0x357),
    "handler_2": (0x22DE0, 0x59),
    "handler_3": (0x22E40, 0xBE),
    "shared_object_renderer": (0x8DA0, 0x28D),
    "object_state_sync": (0xA410, 0x59),
    "geometry_prepare": (0x24770, 0x61),
    "state_predicate_helper": (0x24A80, 0x6F),
}

EXPECTED_CALLS = {
    0x9030: [0x9420, 0x90E0, 0x99B0],
    0x90E0: [0x23B00],
    0x9420: [0x23B00],
    0x99B0: [0x23B00],
    0x22DE0: [0x24A80, 0xA410, 0x24A80, 0x8DA0],
    0x22E40: [0x24A80, 0x1FF40, 0x1FD20, 0x1FCD0, 0x24770, 0xA410, 0x8DA0],
    0xA410: [0xA9A0, 0xA2D0],
    0x24770: [0x1FCA0, 0x1FD00, 0x1FD80, 0x1FDD0],
    0x24A80: [0x24BB0, 0x1FD80, 0x1FDD0],
}
FT4_WRITERS = [0x90E0, 0x9420, 0x99B0]
FT4_PACKET_OFFSETS = [0x00, 0x04, 0x08, 0x0C, 0x0E, 0x10, 0x14,
                      0x16, 0x18, 0x1C, 0x20, 0x24]
HELPER_RESULT_FLOW = {
    0x90E0: (0x90FB, 0x93B4, 0x93B8),
    0x9420: (0x943B, 0x96DE, 0x96E2),
    0x99B0: (0x99C5, 0x9CA5, 0x9CA9),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def guarded_work_path(value: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = ROOT / path
    path = path.resolve()
    work = (ROOT / "work").resolve()
    try:
        path.relative_to(work)
    except ValueError as exc:
        raise SystemExit(f"receipt must resolve under {work}: {path}") from exc
    return path


def decode(md, image: PE, start: int, size: int):
    raw = image.read_rva(start, size)
    insns = list(md.disasm(raw, start))
    if not insns or insns[-1].address + insns[-1].size != start + size:
        raise AssertionError(f"incomplete instruction boundary at {start:#x}+{size:#x}")
    return insns


def direct_calls(insns):
    return [ins.operands[0].imm for ins in insns
            if ins.mnemonic == "call" and ins.operands
            and ins.operands[0].type == X86_OP_IMM]


def insn_record(ins):
    return {"rva": hex(ins.address), "size": ins.size, "sha256": sha(bytes(ins.bytes)),
            "mnemonic": ins.mnemonic, "operands": ins.op_str}


def packet_write_offsets(insns, md):
    offsets = {"ecx": set(), "edx": set()}
    for ins in insns:
        if not ins.operands or ins.operands[0].type != X86_OP_MEM:
            continue
        op = ins.operands[0]
        if not (op.access & capstone.CS_AC_WRITE):
            continue
        if op.mem.base == X86_REG_ECX:
            offsets["ecx"].add(op.mem.disp)
        elif op.mem.base == X86_REG_EDX:
            offsets["edx"].add(op.mem.disp)
    return {base: sorted(values) for base, values in offsets.items()}


def packet_write_sites(insns):
    return [insn_record(ins) for ins in insns
            if ins.operands and ins.operands[0].type == X86_OP_MEM
            and ins.operands[0].access & capstone.CS_AC_WRITE
            and ins.operands[0].mem.base in (X86_REG_ECX, X86_REG_EDX)]


def check_call_site(insns, site, target):
    ins = next((item for item in insns if item.address == site), None)
    if (ins is None or ins.mnemonic != "call" or not ins.operands
            or ins.operands[0].type != X86_OP_IMM
            or ins.operands[0].imm != target):
        raise AssertionError(f"expected call {site:#x} -> {target:#x}")
    return ins


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--eng3", type=Path)
    parser.add_argument("--exe", type=Path)
    parser.add_argument("--receipt", default="work/renderer_bounded_oracle/eng3_handlers.json")
    args = parser.parse_args()

    module_packets_path = PKG / "module_packets.json"
    geometry_path = PKG / "geometry.json"
    compatibility_path = PKG / "compatibility.json"
    module_packets = load_json(module_packets_path)
    geometry = load_json(geometry_path)
    compatibility = load_json(compatibility_path)

    eng3_default = Path(geometry["oracles"]["ENG3.DLL"]["path"])
    exe_default = Path(geometry["oracles"]["HERCULES.EXE"]["path"])
    eng3_path = (args.eng3 or eng3_default).resolve()
    exe_path = (args.exe or exe_default).resolve()
    eng3 = PE(eng3_path)
    exe = PE(exe_path)
    eng3_hash, exe_hash = sha(eng3.data), sha(exe.data)
    if eng3_hash != ENG3_SHA256 or exe_hash != EXE_SHA256:
        raise SystemExit("original hash mismatch; refusing to emit evidence")
    if eng3.image_base != 0x10000000 or exe.image_base != 0x400000:
        raise SystemExit("unexpected image base")

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True

    table_raw = eng3.read_rva(TABLE_RVA, len(TABLE_TARGETS) * 4)
    table_hash = sha(table_raw)
    table_targets = [v - eng3.image_base for v in struct.unpack("<6I", table_raw)]
    if table_hash != TABLE_SHA256 or table_targets != TABLE_TARGETS:
        raise AssertionError("ENG3 handler table differs from public indexed evidence")

    spans = {}
    disassemblies = {}
    for name, (rva, size) in BODY_SPANS.items():
        raw = eng3.read_rva(rva, size)
        insns = decode(md, eng3, rva, size)
        if insns[-1].mnemonic not in ("ret", "retn"):
            raise AssertionError(f"{name} does not end in RET")
        spans[name] = {"module": "ENG3.DLL", "rva": hex(rva), "size": size,
                       "sha256": sha(raw), "boundary": "body through final RET"}
        disassemblies[rva] = insns

    for start, expected in EXPECTED_CALLS.items():
        actual = direct_calls(disassemblies[start])
        if actual != expected:
            raise AssertionError(f"direct-call mismatch at {start:#x}: {actual!r}")

    writer_results = []
    for rva in FT4_WRITERS:
        writes = packet_write_offsets(disassemblies[rva], md)
        if writes["ecx"] != FT4_PACKET_OFFSETS or writes["edx"] != [0]:
            raise AssertionError(f"unexpected FT4 packet write offsets at {rva:#x}: {writes}")
        insns = disassemblies[rva]
        stride_sites = [i for i in insns if i.mnemonic == "add" and
                        i.op_str in ("ecx, 0x28", "ecx, 40")]
        cursor_sites = [i for i in insns if i.mnemonic == "mov" and
                        i.op_str == "dword ptr [0x10043164], ecx"]
        if len(stride_sites) != 1 or len(cursor_sites) != 1:
            raise AssertionError(f"missing unique cursor stride/store at {rva:#x}")
        helper_calls = [i for i in insns if i.mnemonic == "call" and i.operands
                        and i.operands[0].type == X86_OP_IMM and i.operands[0].imm == 0x23B00]
        if len(helper_calls) != 1:
            raise AssertionError(f"missing command/color helper at {rva:#x}")
        result_store, command_load, command_write = HELPER_RESULT_FLOW[rva]
        flow_insns = {i.address: i for i in insns}
        expected_local = "0x4c" if rva == 0x99B0 else "0x44"
        expected_load = "0x48" if rva == 0x99B0 else "0x44"
        if (flow_insns[result_store].mnemonic != "mov"
                or flow_insns[result_store].op_str != f"dword ptr [esp + {expected_local}], eax"
                or flow_insns[command_load].mnemonic != "mov"
                or flow_insns[command_load].op_str != f"edx, dword ptr [esp + {expected_load}]"
                or flow_insns[command_write].mnemonic != "mov"
                or flow_insns[command_write].op_str != "dword ptr [ecx + 4], edx"):
            raise AssertionError(f"helper-result-to-command-field flow changed at {rva:#x}")
        if rva == 0x99B0:
            stack_cleanup = flow_insns.get(0x99D6)
            if stack_cleanup is None or stack_cleanup.bytes.hex() != "83c404":
                raise AssertionError("helper argument cleanup changed at 0x99b0")
        helper_result_flow = {
            "helper_call": insn_record(helper_calls[0]),
            "helper_return_stack_store": insn_record(flow_insns[result_store]),
            "helper_result_stack_load": insn_record(flow_insns[command_load]),
            "packet_command_dword_store": insn_record(flow_insns[command_write]),
        }
        link_sites = [i for i in insns if i.mnemonic == "mov" and
                      i.op_str in ("dword ptr [ecx], esi", "dword ptr [edx], ecx")]
        writer_results.append({
            "producer_rva": hex(rva),
            "family": "FT4-compatible linked primitive",
            "command_word_helper_rva": "0x23b00",
            "helper_result_flow": helper_result_flow,
            "command_tags_from_reused_helper_evidence": ["0x2c", "0x2e"],
            "packet_write_offsets": [hex(x) for x in FT4_PACKET_OFFSETS],
            "packet_write_sites": packet_write_sites(insns),
            "head_link_writes": [insn_record(i) for i in link_sites],
            "cursor_stride": 0x28,
            "stride_site": insn_record(stride_sites[0]),
            "global_cursor_store": insn_record(cursor_sites[0]),
            "highest_observed_packet_byte_written": "0x25",
            "proof_level": "STATIC_OBSERVED",
        })

    # Check the table-copy dispatch context and the shared renderer's child
    # family selection. These are dispatch observations, not an execution proof.
    shared = disassemblies[0x8DA0]
    child_dispatch = next((i for i in shared if i.address == 0x8FED), None)
    object_kind = next((i for i in shared if i.address == 0x8FBE), None)
    direct_ft4 = next((i for i in shared if i.address == 0x901D), None)
    if not child_dispatch or child_dispatch.bytes.hex() != "ff148510500310":
        raise AssertionError("shared child dispatch site changed")
    if not object_kind or object_kind.bytes.hex() != "8a4623":
        raise AssertionError("shared child selector byte read changed")
    if not direct_ft4 or direct_ft4.bytes.hex() != "e87ea10100":
        raise AssertionError("shared direct FT4 route changed")

    # Verify the wrapper-local copied-table cells reached by handler 3 and its
    # geometry helper against the original host interface table.
    copied_base = int(geometry["copied_wrapper_evidence"]["ENG3.DLL"]
                     ["inferred_copied_table_base_rva"], 16)
    source_base = int(compatibility["interface"]["source_table_rva"], 16)
    callback_sites = {
        0x1FCD0: {"call_site": 0x22EBB, "name": "callback_103"},
        0x1FD20: {"call_site": 0x22EB3, "name": "callback_108"},
        0x1FDD0: {"call_site": 0x247BB, "name": "callback_121"},
        0x1FF40: {"call_site": 0x22EA6, "name": "callback_164"},
    }
    wrapper_results = []
    for wrapper_rva, meta in callback_sites.items():
        parent_start = next(start for start, insns in disassemblies.items()
                            if any(i.address == meta["call_site"] for i in insns))
        parent_call = check_call_site(disassemblies[parent_start], meta["call_site"], wrapper_rva)
        wrapper_ins = list(md.disasm(eng3.read_rva(wrapper_rva, 16), wrapper_rva))
        transfer = next((i for i in wrapper_ins if i.mnemonic in ("call", "jmp")
                         and i.operands and i.operands[0].type == X86_OP_MEM), None)
        if transfer is None:
            raise AssertionError(f"missing indirect callback transfer at {wrapper_rva:#x}")
        mem = transfer.operands[0].mem
        if mem.base != 0 or mem.index != 0:
            raise AssertionError(f"unexpected callback cell addressing at {wrapper_rva:#x}")
        cell_va = mem.disp
        cell_rva = cell_va - eng3.image_base
        slot_delta = cell_rva - copied_base
        if slot_delta < 0 or slot_delta % 4:
            raise AssertionError(f"callback cell outside copied table at {wrapper_rva:#x}")
        slot = slot_delta // 4
        target_va = struct.unpack("<I", exe.read_rva(source_base + slot * 4, 4))[0]
        wrapper_results.append({
            "parent_call_site": hex(meta["call_site"]),
            "parent_call": insn_record(parent_call),
            "wrapper_rva": hex(wrapper_rva),
            "wrapper_transfer": insn_record(transfer),
            "copied_table_cell_rva": hex(cell_rva),
            "slot": slot,
            "original_host_target_rva": hex(target_va - exe.image_base),
            "classification": "host callback boundary; semantic body outside this packet audit",
        })

    module_packet_bytes = module_packets_path.read_bytes()
    geometry_bytes = geometry_path.read_bytes()
    compatibility_bytes = compatibility_path.read_bytes()

    sync = disassemblies[0xA410]
    state_sites = {
        "read_state_byte": (0xA415, "660fb64622"),
        "read_source_y": (0xA450, "668b4636"),
        "read_source_x": (0xA454, "668b4e34"),
        "write_state_y": (0xA45B, "66894642"),
        "write_state_x": (0xA45F, "66894e40"),
        "write_state_mode": (0xA463, "66895644"),
    }
    observed_state_sites = {}
    for name, (site, prefix) in state_sites.items():
        ins = next((item for item in sync if item.address == site), None)
        if ins is None or not ins.bytes.hex().startswith(prefix):
            raise AssertionError(f"object state synchronization site changed: {site:#x}")
        observed_state_sites[name] = insn_record(ins)

    handler_call_sites = {}
    for entry in (0x9030, 0x22DE0, 0x22E40):
        handler_call_sites[hex(entry)] = [insn_record(i) for i in disassemblies[entry]
                                          if i.mnemonic == "call" and i.operands
                                          and i.operands[0].type == X86_OP_IMM]
    result = {
        "schema_version": 1,
        "scope": "Bounded STATIC_OBSERVED trace of ENG3 handler-table entries 0-3 and their direct packet/render delegates; no original execution.",
        "prediction": {
            "scope": "ENG3 handler-table indices 0-3 and their bounded direct render delegates.",
            "prediction": "State/delegation routes and known AC60 packet families; computed child selection may remain unresolved.",
            "falsifier": "A pinned direct route writes a command or packet layout outside the represented families.",
            "private_initial_record": "work/renderer_phase4/eng3_handlers/prediction.json",
        },
        "proof_level": "STATIC_OBSERVED",
        "tooling": {"capstone": capstone.__version__, "pe_reader": "scripts/pe.py",
                    "dependency_setup": "scripts/env.py"},
        "originals": {
            "ENG3.DLL": {"path": str(eng3_path), "sha256": eng3_hash,
                         "image_base": hex(eng3.image_base)},
            "HERCULES.EXE": {"path": str(exe_path), "sha256": exe_hash,
                             "image_base": hex(exe.image_base)},
        },
        "public_inputs": {
            "renderer_contract/evidence/module_packets.json": sha(module_packet_bytes),
            "renderer_contract/evidence/geometry.json": sha(geometry_bytes),
            "renderer_contract/evidence/compatibility.json": sha(compatibility_bytes),
        },
        "handler_table": {"rva": hex(TABLE_RVA), "size": len(table_raw),
                          "sha256": table_hash,
                          "target_rvas_by_index": [hex(x) for x in table_targets]},
        "native_spans": spans,
        "handlers": [
            {"index": 0, "entry_rva": "0x9030",
             "classification": "data-dependent selector routing to three FT4-compatible linked packet writers",
             "direct_call_targets": ["0x9420", "0x90e0", "0x99b0"],
             "direct_call_sites": handler_call_sites["0x9030"],
             "packet_producers": ["0x9420", "0x90e0", "0x99b0"],
             "proof_level": "STATIC_OBSERVED"},
            {"index": 1, "entry_rva": "0x90e0",
             "classification": "FT4-compatible linked packet writer",
             "packet_producers": ["0x90e0"],
             "direct_call_sites": [insn_record(i) for i in disassemblies[0x90E0]
                                   if i.mnemonic == "call" and i.operands
                                   and i.operands[0].type == X86_OP_IMM],
             "proof_level": "STATIC_OBSERVED"},
            {"index": 2, "entry_rva": "0x22de0",
             "classification": "conditional object-state synchronization and shared object-renderer delegation; no new packet layout at this entry",
             "direct_call_targets": ["0x24a80", "0xa410", "0x24a80", "0x8da0"],
             "direct_call_sites": handler_call_sites["0x22de0"],
             "render_delegate": "0x8da0", "proof_level": "STATIC_OBSERVED"},
            {"index": 3, "entry_rva": "0x22e40",
             "classification": "geometry/state preparation followed by shared object-renderer delegation; no new packet layout at this entry",
             "direct_call_targets": ["0x24a80", "0x1ff40", "0x1fd20", "0x1fcd0", "0x24770", "0xa410", "0x8da0"],
             "direct_call_sites": handler_call_sites["0x22e40"],
             "render_delegate": "0x8da0", "proof_level": "STATIC_OBSERVED"},
        ],
        "ft4_packet_producers": writer_results,
        "shared_object_renderer": {
            "entry_rva": "0x8da0",
            "direct_ft4_route": insn_record(direct_ft4),
            "object_kind_read": insn_record(object_kind),
            "indexed_handler_call": insn_record(child_dispatch),
            "selection": "byte [object+0x23] is used as the index for the same handler table at 0x35010 when nonzero; the zero branch calls handler 0 directly",
            "interpretation": "downstream producer family is data-dependent; observed route can reach existing FT4 writer 0x231a0, handler 0, or a selected table entry; no new layout established",
            "proof_level": "STATIC_OBSERVED",
        },
        "object_state_sync_sites": observed_state_sites,
        "callback_boundaries": wrapper_results,
        "limits": [
            "The 0x28 value is the observed packet cursor increment, not a general allocator capacity guarantee.",
            "The packet field set is observed from these ENG3 writers and reuses the public FT4 contract; it does not establish a new wire family.",
            "The shared renderer's indexed child route is data-dependent; this receipt does not claim every child entry emits a packet.",
            "No universal absence claim is made for other ENG3 producers or computed routes.",
            "No BOUNDED_EQUIVALENT or runtime-execution proof is claimed.",
        ],
    }

    receipt = guarded_work_path(args.receipt)
    receipt.parent.mkdir(parents=True, exist_ok=True)
    receipt.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": str(receipt), "eng3_sha256": eng3_hash,
                      "handler_table_sha256": table_hash,
                      "writer_count": len(writer_results), "proof_level": "STATIC_OBSERVED"},
                     indent=2))


if __name__ == "__main__":
    main()
