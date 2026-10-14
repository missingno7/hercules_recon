"""Guard-page discriminator for AC60's transitive command-01 observation span.

Prediction: original AC60 can enter A8F0 with a 16-byte packet, but A8F0
reads rectangle words beyond it, including height at +22. Falsifier: the
16/22-byte cases return without a read fault, or the 24-byte zero-height
case does not return through the unchanged, no-device original helpers.
No OS/game process, device, copied instruction bytes or source acceptance.
"""
from __future__ import annotations

import importlib.util
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "extent_ac60", ROOT / "renderer_contract/tests/original_ac60_mixed_runner.py")
assert spec and spec.loader
R = importlib.util.module_from_spec(spec)
spec.loader.exec_module(R)
from unicorn import UC_HOOK_MEM_READ  # noqa: E402


def run_case(pe, available):
    uc = R.Uc(R.UC_ARCH_X86, R.UC_MODE_32)
    R.map_original_sections(uc, pe)
    uc.mem_map(R.INPUT_BASE, R.PAGE, R.UC_PROT_READ | R.UC_PROT_WRITE)
    uc.mem_map(R.STACK_BASE, R.STACK_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE)
    uc.mem_map(R.RETURN_SENTINEL, R.PAGE, R.UC_PROT_READ | R.UC_PROT_EXEC)
    packet = R.INPUT_BASE + R.PAGE - available
    wire = bytearray(24)
    wire[7] = 1
    struct.pack_into("<6h", wire, 12, 16, 160, 4, 140, 5, 0)
    uc.mem_write(packet, bytes(wire[:available]))
    ot = R.INPUT_BASE + 0x100
    uc.mem_write(ot, struct.pack("<I", packet))
    for va, value in {
        0x00BA01FC: b"\0", 0x0096E698: b"\0", 0x00BA020C: b"\0",
        0x0043A590: b"\0", 0x0096E3D8: b"\0" * 4,
        R.CALLBACK_CELL: b"\0" * 4, R.NOTIFICATION_GATE: b"\0" * 4,
    }.items():
        uc.mem_write(va, value)
    stack = R.STACK_BASE + R.STACK_SIZE - 0x100
    uc.mem_write(stack, struct.pack("<II", R.RETURN_SENTINEL, ot))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    spans = [(R.EXE_BASE + R.AC60_RVA, R.AC60_END_RVA - R.AC60_RVA),
             (R.MOVE_TARGET, 0x303)]
    before = [bytes(uc.mem_read(va, size)) for va, size in spans]
    state = {"available": available, "entered_a8f0": False,
             "stop": None, "reads": [], "invalid": None, "instructions": 0}

    def code(machine, address, size, context):
        state["instructions"] += 1
        if state["instructions"] > 100000:
            state["stop"] = "budget"
            machine.emu_stop()
        elif address == R.RETURN_SENTINEL:
            state["stop"] = "returned"
            machine.emu_stop()
        elif address == R.MOVE_TARGET:
            state["entered_a8f0"] = True
            sp = machine.reg_read(R.UC_X86_REG_ESP)
            args = struct.unpack("<4I", bytes(machine.mem_read(sp, 16)))
            assert args == (R.EXE_BASE + 0xB1EC, packet + 16, 16, 160), args
        elif not (R.MOVE_TARGET <= address < R.MOVE_TARGET + 0x303 or
                  any(lo <= address < hi for lo, hi, name in R.ALLOWED_PC_RANGES)):
            state["stop"] = "unknown_pc"
            machine.emu_stop()

    def read(machine, access, address, size, value, context):
        if packet <= address < packet + 24:
            state["reads"].append({"offset": address - packet, "size": size,
                                  "pc_rva": hex(machine.reg_read(R.UC_X86_REG_EIP) - R.EXE_BASE)})

    def invalid(machine, access, address, size, value, context):
        state["invalid"] = {"offset": address - packet, "size": size,
                            "pc_rva": hex(machine.reg_read(R.UC_X86_REG_EIP) - R.EXE_BASE)}
        state["stop"] = "unmapped"
        return False

    uc.hook_add(R.UC_HOOK_CODE, code)
    uc.hook_add(UC_HOOK_MEM_READ, read)
    uc.hook_add(R.UC_HOOK_MEM_INVALID, invalid)
    try:
        uc.emu_start(R.EXE_BASE + R.AC60_RVA, 0, count=100001)
    except R.UcError:
        if state["stop"] != "unmapped":
            raise
    assert state["entered_a8f0"], state
    assert before == [bytes(uc.mem_read(va, size)) for va, size in spans]
    if available < 24:
        assert state["stop"] == "unmapped", state
        assert state["invalid"]["offset"] >= available, state
        pc = int(state["invalid"]["pc_rva"], 16)
        assert 0xA8F0 <= pc < 0xABF3, state
    else:
        assert state["stop"] == "returned", state
        assert any(row["offset"] == 22 and row["size"] == 2 for row in state["reads"]), state
    return state


def main():
    runtime = R.verify_runtime(R.unicorn, R.ARGS.deps)
    pe = R.PE(R.ARGS.exe)
    cases = [run_case(pe, size) for size in (16, 22, 24)]
    receipt = {
        "schema_version": 1, "status": "BOUNDED_EQUIVALENT",
        "purpose": "guard-page command-01 transitive read discriminator",
        "original_game_process_executed": False,
        "prediction": __doc__, "oracle_sha256": hashlib.sha256(R.ARGS.exe.read_bytes()).hexdigest(),
        "runtime": runtime, "cases": cases,
        "scope": "three controlled command-01 spans; zero-height no-device path only; no copy/pixel or producer-capacity proof",
    }
    R.write_receipt(R.ARGS.receipt, receipt)
    print(json.dumps({"status": receipt["status"], "cases": cases}, indent=2))


if __name__ == "__main__":
    main()
