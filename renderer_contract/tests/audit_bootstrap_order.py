"""Bounded, read-only bootstrap-order CFG audit for the pinned Hercules host."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path


HOST_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
SPAN_SPECS = {
    "host_init_5e10": (0x5E10, 0x7F),
    "engine_dispatch_wrapper_5ec0": (0x5EC0, 0x54),
    "timer_dispatch_5f20": (0x5F20, 0xAD),
    "palette_setup_71a0": (0x71A0, 0xFB),
    "graphics_setup_72a0": (0x72A0, 0x214),
    "event_callback_79a0": (0x79A0, 0xFB),
    "init_callsite_7937": (0x7937, 5),
    "directdraw_callsite_7420": (0x7420, 0x12),
    "event_tick_callsite_79fc": (0x79FC, 5),
    "timer_first_dispatch_callsite_5f97": (0x5F97, 5),
    "timer_second_dispatch_callsite_5fc5": (0x5FC5, 5),
    "source_pointer_read_5ef3": (0x5EF3, 6),
    "indirect_export_call_5f03": (0x5F03, 2),
    "callback_address_literal_2e240": (0x2E240, 4),
}
DIRECT_TARGETS = (0x5E10, 0x5EC0, 0x5F20, 0x72A0, 0x71A0, 0x10A20, 0x5F03)
CFG_SPECS = {
    "host_init_5e10": (0x5E10, 0x7F),
    "engine_dispatch_wrapper_5ec0": (0x5EC0, 0x54),
    "timer_dispatch_5f20": (0x5F20, 0xAD),
    "palette_setup_71a0": (0x71A0, 0xFB),
    "graphics_setup_72a0": (0x72A0, 0x214),
    "event_callback_79a0": (0x79A0, 0xFB),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def hx(value: int | None) -> str | None:
    return None if value is None else hex(value)


def cfg_for(pe, capstone, base: int, name: str, rva: int, size: int) -> dict:
    """Build a complete direct-flow CFG over one explicitly pinned code span.

    Calls are fallthrough for intraprocedural flow; conditional/direct branches,
    returns, and edges leaving the pinned range are represented explicitly.
    This intentionally does not synthesize exception-dispatch or indirect edges.
    """
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    raw = pe.read_rva(rva, size)
    insns = list(md.disasm(raw, base + rva))
    by_addr = {i.address: i for i in insns}
    end = base + rva + size
    if not insns or insns[0].address != base + rva or insns[-1].address + insns[-1].size != end:
        raise ValueError(f"CFG span {name} does not decode exactly to its pinned boundary")

    def imm_target(insn):
        if not insn.operands or insn.operands[0].type != capstone.x86.X86_OP_IMM:
            return None
        return insn.operands[0].imm

    leaders = {base + rva}
    for insn in insns:
        target = imm_target(insn)
        if insn.mnemonic.startswith("j") and target is not None:
            if base + rva <= target < end and target in by_addr:
                leaders.add(target)
            following = insn.address + insn.size
            if following < end and following in by_addr:
                leaders.add(following)
        elif insn.mnemonic.startswith("ret"):
            following = insn.address + insn.size
            if following < end and following in by_addr:
                leaders.add(following)

    sorted_leaders = sorted(leaders)
    leader_for = {}
    blocks = {}
    for idx, start in enumerate(sorted_leaders):
        stop = sorted_leaders[idx + 1] if idx + 1 < len(sorted_leaders) else end
        block_insns = [i for i in insns if start <= i.address < stop]
        if not block_insns:
            continue
        for i in block_insns:
            leader_for[i.address] = start
        blocks[start] = block_insns

    edges = {start: [] for start in blocks}
    branch_rows = []
    for start, block_insns in blocks.items():
        last = block_insns[-1]
        fall = last.address + last.size
        target = imm_target(last)
        if last.mnemonic.startswith("ret"):
            term = "return"
        elif last.mnemonic.startswith("j") and target is not None:
            if last.mnemonic == "jmp":
                successors = [target]
                term = "jump"
            else:
                successors = [target, fall]
                term = "conditional"
            for succ in successors:
                if base + rva <= succ < end and succ in leader_for:
                    edges[start].append(leader_for[succ])
            branch_rows.append({
                "site_rva": hx(last.address - base), "mnemonic": last.mnemonic,
                "target_rva": hx(target - base),
                "fallthrough_rva": hx(fall - base) if last.mnemonic != "jmp" else None,
                "target_inside_span": base + rva <= target < end,
            })
        elif last.mnemonic == "jmp":
            term = "indirect_jump"
        else:
            term = "fallthrough"
            if fall < end and fall in leader_for:
                edges[start].append(leader_for[fall])
        blocks[start] = block_insns
        # Keep the final basic block's direct successors; call targets are separate facts.
        blocks[start] = block_insns

    entry = base + rva
    reachable = set()
    todo = [entry]
    while todo:
        node = todo.pop()
        if node in reachable or node not in edges:
            continue
        reachable.add(node)
        todo.extend(edges[node])

    # Verify whether a particular direct call block is unavoidable on every
    # entry-to-return path in this bounded normal CFG.
    calls = []
    for insn in insns:
        if insn.mnemonic == "call":
            target = imm_target(insn)
            calls.append({"site_rva": hx(insn.address - base),
                          "target_rva": hx(target - base) if target is not None else None,
                          "indirect": target is None,
                          "reachable": leader_for.get(insn.address) in reachable})
    returns = [start for start, body in blocks.items() if body[-1].mnemonic.startswith("ret")]
    block_rows = []
    for start in sorted(blocks):
        body = blocks[start]
        last = body[-1]
        if last.mnemonic.startswith("ret"):
            term = "return"
        elif last.mnemonic == "jmp":
            term = "jump" if imm_target(last) is not None else "indirect_jump"
        elif last.mnemonic.startswith("j"):
            term = "conditional"
        else:
            term = "fallthrough"
        block_rows.append({"start_rva": hx(start - base),
                           "end_rva_exclusive": hx(body[-1].address + body[-1].size - base),
                           "terminator": term,
                           "successors": [hx(x - base) for x in edges[start]],
                           "reachable_from_entry": start in reachable})

    def all_returns_require(site_rva: int) -> bool:
        site_va = base + site_rva
        target_block = leader_for.get(site_va)
        if target_block not in reachable:
            return False
        todo = [entry]
        seen = set()
        while todo:
            node = todo.pop()
            if node == target_block or node in seen:
                continue
            seen.add(node)
            body = blocks.get(node, [])
            if body and body[-1].mnemonic.startswith("ret"):
                return False
            todo.extend(edges.get(node, []))
        return not any(ret in seen for ret in returns)

    return {
        "span_rva": hx(rva), "span_size": size,
        "entry_rva": hx(rva), "instruction_count": len(insns),
        "basic_blocks": block_rows, "conditional_and_direct_jump_edges": branch_rows,
        "direct_or_indirect_calls": calls,
        "reachable_return_blocks": [hx(x - base) for x in returns if x in reachable],
        "entry_reaches_return": any(x in reachable for x in returns),
        "all_normal_returns_require_call": {
            hx(site): all_returns_require(site)
            for site in ({0x5E2C} if name == "host_init_5e10" else
                         {0x741A, 0x7426} if name == "graphics_setup_72a0" else set())
        },
        "cfg_limit": "Direct x86 control flow only; calls fall through intraprocedurally. Indirect calls/jumps, SEH dispatch, API reentrancy, and callbacks are unresolved edges.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-root", type=Path, required=True,
                        help="checkout root containing scripts/pe.py and work/")
    parser.add_argument("--exe", type=Path, required=True,
                        help="read-only pinned original HERCULES.EXE")
    parser.add_argument("--output", type=Path, required=True,
                        help="JSON receipt under original-root/work")
    args = parser.parse_args()
    root, exe, output = args.original_root.resolve(), args.exe.resolve(), args.output.resolve()
    work = (root / "work").resolve()
    if work != root / "work":
        parser.error("refusing a redirected work directory")
    if not (root / "scripts" / "pe.py").is_file():
        parser.error("--original-root must contain scripts/pe.py")
    if not output.is_relative_to(work) or output == exe:
        parser.error("--output must be a distinct file under original-root/work")

    sys.path.insert(0, str(root / "scripts"))
    import env  # noqa: F401,E402
    import capstone  # noqa: E402
    from pe import PE  # noqa: E402

    image = exe.read_bytes()
    host_sha = sha(image)
    if host_sha != HOST_SHA256:
        parser.error(f"refusing non-pinned host PE: SHA-256 is {host_sha}")
    pe = PE(exe)
    base = pe.image_base
    text = next(s for s in pe.sections if s["name"] == ".text")
    text_data = pe.read_rva(text["virtual_address"], text["raw_size"])
    sweep = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    sweep.detail = True
    sweep.skipdata = True
    instructions = list(sweep.disasm(text_data, base + text["virtual_address"]))
    decoded = [i for i in instructions if i.id != 0 and i.mnemonic != ".byte"]
    coverage = sum(i.size for i in instructions)
    if coverage != text["raw_size"]:
        raise RuntimeError("skipdata linear sweep did not cover the complete raw .text section")

    def immediate_target(insn):
        for op in insn.operands:
            if op.type == capstone.x86.X86_OP_IMM:
                return op.imm
        return None

    direct_refs = {}
    for target_rva in DIRECT_TARGETS:
        refs = []
        for insn in decoded:
            if insn.mnemonic not in ("call", "jmp"):
                continue
            target = immediate_target(insn)
            if target == base + target_rva:
                refs.append({"site_rva": hx(insn.address - base),
                             "mnemonic": insn.mnemonic,
                             "target_rva": hx(target_rva)})
        direct_refs[hx(target_rva)] = refs

    spans = {}
    for name, (rva, size) in SPAN_SPECS.items():
        raw = pe.read_rva(rva, size)
        spans[name] = {"module": "HERCULES.EXE", "name": name,
                       "rva": hx(rva), "size": size, "sha256": sha(raw)}

    cfgs = {name: cfg_for(pe, capstone, base, name, *spec)
            for name, spec in CFG_SPECS.items()}

    # Raw, file-backed occurrences are a candidate address-literal inventory,
    # separate from instruction references and not proof of invocation.
    code_address_hits = {}
    for target_rva in (0x79A0, 0x5E10, 0x5EC0, 0x5F20, 0x72A0, 0x71A0):
        needle = struct.pack("<I", base + target_rva)
        hits = []
        for section in pe.sections:
            raw = pe.read_rva(section["virtual_address"], section["raw_size"])
            at = 0
            while True:
                pos = raw.find(needle, at)
                if pos < 0:
                    break
                hits.append({"section": section["name"],
                             "rva": hx(section["virtual_address"] + pos)})
                at = pos + 1
        code_address_hits[hx(target_rva)] = hits

    # Native machine-flow observations tied to pinned branch/call instructions.
    observations = {
        "init_graphics_call": {"rva": hx(0x5E2C), "target_rva": hx(0x72A0)},
        "graphics_palette_call": {"rva": hx(0x741A), "target_rva": hx(0x71A0)},
        "directdraw_import_thunk_call": {"rva": hx(0x7426), "target_rva": hx(0x10A20),
                                         "iat_rva": hx(0x7A5648)},
        "directdraw_result_branch": {"rva": hx(0x742B), "bytes_sha256": sha(pe.read_rva(0x742B, 6)),
                                     "zero_result_target_rva": hx(0x743C),
                                     "nonzero_result_logging_then_rejoin_rva": hx(0x742F),
                                     "both_paths_rejoin_rva": hx(0x743C)},
        "init_callback_call": {"rva": hx(0x7937), "target_rva": hx(0x5E10)},
        "event_to_tick_call": {"rva": hx(0x79FC), "target_rva": hx(0x5F20)},
        "tick_to_dispatcher_calls": [
            {"rva": hx(0x5F97), "target_rva": hx(0x5EC0)},
            {"rva": hx(0x5FC5), "target_rva": hx(0x5EC0)},
        ],
        "dispatcher_to_export": {"rva": hx(0x5F03), "instruction_sha256": spans["indirect_export_call_5f03"]["sha256"],
                                  "kind": "indirect call through EAX loaded from [0x91c45c]",
                                  "preceded_by_nonnull_guard_rva": hx(0x5EEF),
                                  "source_table_pointer_read_rva": hx(0x5EF3),
                                  "engine_entry_gate": "If [0x91c45c] is null, branch to 0x5f08 and skip source-table read/call."},
        "event_callback_gate": {
            "flag_0": "At 0x79ea..0x79f1, [0xba0238]==0 branches around 0x79fc tick call.",
            "flag_1": "At 0x79f3..0x79fa, [0xba0228]!=0 branches around 0x79fc tick call.",
            "indirect_registration_clue": "File-backed .rdata contains the absolute code address 0x4079a0 at RVA 0x2e240; it is an address literal, not an observed caller or runtime invocation."},
        "publication_requirement": {
            "static_contract": "For any later engine call that reaches 0x5f03, a selected table clone must already be stored through [0x4365a0] before 0x5ef3 loads that pointer.",
            "HRESULT": "The native code compares the DirectDrawCreate result with zero only to choose an error-log path; both outcomes rejoin at 0x743c and continue. It does not gate pointer publication or engine dispatch on a successful HRESULT.",
            "temporal_limit": "The direct host CFG places the DirectDrawCreate call on every normal return path from 0x72a0, and 0x72a0 is called by 0x5e10. It does not prove that indirect callbacks/API reentrancy cannot invoke 0x79a0 before the call returns."},
    }

    result = {
        "schema_version": 1,
        "scope": "Read-only static audit of bounded host bootstrap/tick control flow and a complete raw .text direct-reference inventory; no launch, execution, patching, or source changes.",
        "audit_script_sha256": sha(Path(__file__).read_bytes()),
        "pe_helper": {"path": str(root / "scripts" / "pe.py"),
                      "sha256": sha((root / "scripts" / "pe.py").read_bytes()),
                      "capstone_version": capstone.__version__},
        "host": {"path": str(exe), "size": len(image), "sha256": host_sha,
                 "image_base": hx(base), "machine": hx(pe.pe.FILE_HEADER.Machine)},
        "linear_sweep": {"section": ".text", "rva": hx(text["virtual_address"]),
                         "raw_size": text["raw_size"], "record_count": len(instructions),
                         "decoded_instruction_count": len(decoded),
                         "skipdata_record_count": len(instructions) - len(decoded),
                         "covered_bytes": coverage, "coverage_complete": coverage == text["raw_size"],
                         "scope_limit": "Linear sweep is a candidate call-reference inventory; x86 decoding does not prove code alignment for every byte, and embedded data may decode as instructions."},
        "direct_call_or_jump_refs": direct_refs,
        "file_backed_code_address_literals": code_address_hits,
        "pinned_spans": spans,
        "bounded_cfg": cfgs,
        "observations": observations,
        "conclusion": {
            "caller_inventory": "The complete raw .text sweep found one direct call to 0x5e10 (0x7937), one to 0x72a0 (0x5e2c), one to 0x71a0 (0x741a), and two direct calls to dispatcher 0x5ec0 (0x5f97, 0x5fc5). The engine export invocation at 0x5f03 is indirect and is reachable only through the guarded dispatcher path in these bounded spans.",
            "direct_cfg_order": "The direct initialization path is 0x7937 -> 0x5e10 -> 0x72a0 -> 0x71a0 -> DirectDrawCreate thunk 0x10a20 at 0x7426; 0x72a0 returns before 0x5e10 completes. A separate callback path is 0x79a0 -> 0x79fc -> 0x5f20 -> 0x5ec0 -> guarded indirect 0x5f03; that routine chain does not call 0x5e10, 0x72a0, or DirectDrawCreate.",
            "failed_result": "The 0x742b zero/nonzero branch selects only whether an error message is logged. The nonzero branch rejoins before the subsequent DirectDraw-object calls; no HRESULT-based early return or dispatch guard exists in 0x72a0/0x5e10.",
            "publication": "A clone intended to affect any eventual cold engine-table copy must be published by the time the guarded dispatcher reaches 0x5ef3. A policy that publishes only for zero HRESULT is stricter than the observed host contract and can leave the original table in place on the nonzero continuation path. Publishing after the forwarded call returns for either result covers the direct initialization path, but may be too late if the independent callback path is invoked reentrantly or before initialization completes.",
            "unresolved": "The event callback entry 0x79a0 has a file-backed code-address literal at .rdata RVA 0x2e240 and no direct .text callers in the linear sweep. Its indirect framework caller and invocation order relative to 0x7937/0x7426 are not recovered here. Its direct routine chain can reach the guarded cold export call without traversing DirectDrawCreate; whether that chain is actually invoked before the DirectDraw call returns or before it is ever called is unresolved. Thus this audit does not support an unconditional process-wide 'DirectDrawCreate precedes first table copy' claim.",
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha(output.read_bytes()),
                      "host_sha256": host_sha, "decoded_text_instructions": len(decoded),
                      "text_coverage_complete": coverage == text["raw_size"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
