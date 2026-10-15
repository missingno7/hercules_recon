"""Differential oracle: replay the shim harness's order-table image through
the ORIGINAL HERCULES.EXE AC60 bytes under Unicorn and compare with what the
shim pre-walk reported.

Read-only: the EXE is parsed and mapped into the emulator, never loaded or
modified. All eleven AC60 callees are stubbed (record + return), so pixels and
presentation are not executed; traversal, E1/F3 state writes and the A8F0
command-01 edge are the original code.
Usage: python ac60_oracle.py <HERCULES.EXE> <harness-out-dir> <receipt.json>
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

for dep in (r"C:\tools\hercules\python", r"C:\tools\hercules\renderer-oracle-python"):
    sys.path.insert(0, dep)
import pefile  # noqa: E402
from unicorn import (UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_READ,  # noqa: E402
                     UC_HOOK_MEM_WRITE, UC_MODE_32, Uc)
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP  # noqa: E402

PINNED = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
AC60 = 0x40AC60
CALLEES = {0x406620: "present_6620", 0x406EE0: "clear_6ee0", 0x406FF0: "lock_6ff0",
           0x4070C0: "remap_70c0", 0x407140: "unlock_7140", 0x40A8F0: "move_a8f0",
           0x40BD80: "notify_bd80", 0x40C8F0: "sprite_c8f0", 0x40CAB0: "sprite_cab0",
           0x40D420: "poly_d420", 0x40E680: "poly_e680"}
RET_MAGIC = 0x00F00000
STACK_TOP = 0x00110000
REGION = (0x20000000, 0x10000)
POLY = {1, 2, 3}       # RENDERER_AC60_EVENT_FT3/FT4/GT4
SPRITE = {4, 5, 6}
DEFERRED = 9


def main():
    exe_path, out_dir, receipt_path = sys.argv[1:4]
    data = Path(exe_path).read_bytes()
    assert hashlib.sha256(data).hexdigest() == PINNED, "unexpected EXE"
    shim = json.loads((Path(out_dir) / "shim_frame.json").read_text())
    image = (Path(out_dir) / "ot_image.bin").read_bytes()

    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF)
    uc.mem_write(base, data[:pe.OPTIONAL_HEADER.SizeOfHeaders])
    for s in pe.sections:
        uc.mem_write(base + s.VirtualAddress, s.get_data())
    uc.mem_map(REGION[0], REGION[1])
    uc.mem_write(shim["image_base"], image)
    uc.mem_map(STACK_TOP - 0x10000, 0x10000)
    uc.mem_map(RET_MAGIC, 0x1000)
    esp = STACK_TOP - 0x100
    uc.mem_write(esp, struct.pack("<II", RET_MAGIC, shim["ot_address"]))
    uc.reg_write(UC_X86_REG_ESP, esp)
    for cell in (0xBA01FC, 0x96E698):  # raster enabled, no clear request
        uc.mem_write(cell, b"\0")

    nodes = {p["node"] for p in shim["primitives"]}
    order, calls, writes = [], [], []
    current = [None]

    def on_read(_uc, _access, address, size, _value, _user):
        # AC60 may read a tag byte more than once per primitive; consumption
        # order is the sequence of distinct consecutive nodes.
        if size == 1 and address - 7 in nodes and current[0] != address - 7:
            order.append(address - 7)
            current[0] = address - 7

    def on_write(_uc, _access, address, size, value, _user):
        writes.append([address, size, value])

    def on_code(machine, address, _size, _user):
        if address in CALLEES:
            sp = machine.reg_read(UC_X86_REG_ESP)
            words = struct.unpack("<6I", machine.mem_read(sp, 24))
            calls.append({"callee": CALLEES[address], "node": current[0], "args": list(words[1:5])})
            machine.reg_write(UC_X86_REG_EIP, words[0])
            machine.reg_write(UC_X86_REG_ESP, sp + 4)

    uc.hook_add(UC_HOOK_MEM_READ, on_read, begin=REGION[0], end=REGION[0] + REGION[1] - 1)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write, begin=REGION[0], end=REGION[0] + REGION[1] - 1)
    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.emu_start(AC60, RET_MAGIC, count=2_000_000)

    def u32(a):
        return struct.unpack("<I", uc.mem_read(a, 4))[0]

    def i32(a):
        return struct.unpack("<i", uc.mem_read(a, 4))[0]

    original_state = {"A8E6F4": u32(0xA8E6F4), "A8E6F8": u32(0xA8E6F8), "A8E6FC": u32(0xA8E6FC),
                      "A909B0": u32(0xA909B0), "A909B4": u32(0xA909B4),
                      "43A488": i32(0x43A488), "43A48C": i32(0x43A48C)}

    checks = []

    def check(name, ok, detail):
        checks.append({"check": name, "ok": bool(ok), "detail": detail})

    shim_order = [p["node"] for p in shim["primitives"]]
    check("order_original_eq_shim_prewalk", order == shim_order, {"original": order, "shim": shim_order})
    check("order_original_eq_synthetic_host", order == shim["fake_consumption"], None)
    check("original_never_writes_order_table", not writes, writes[:8])
    check("final_state_cells", original_state == shim["final_state"],
          {"original": original_state, "shim": shim["final_state"]})

    a8f0 = [[c["args"][0], c["args"][1], c["args"][2]] for c in calls if c["callee"] == "move_a8f0"]
    a8f0 = [[a, struct.unpack("<i", struct.pack("<I", b))[0], struct.unpack("<i", struct.pack("<I", c))[0]]
            for a, b, c in a8f0]
    check("a8f0_args_eq_synthetic_host", a8f0 == shim["fake_a8f0"], {"original": a8f0, "shim": shim["fake_a8f0"]})
    deferred = [(p["node"], p["deferred"]) for p in shim["primitives"] if p["staged_kind"] == DEFERRED]
    check("a8f0_args_eq_staged_destination",
          all(any(a == [n + 0x10, d[0], d[1]] for a in a8f0) for n, d in deferred), deferred)

    per_node = {}
    for c in calls:
        if c["node"] is not None and c["callee"].split("_")[0] in ("poly", "sprite", "move"):
            per_node.setdefault(c["node"], []).append(c["callee"])
    route_by_semi = {}
    route_ok = True
    for p in shim["primitives"]:
        got = per_node.get(p["node"], [])
        if p["staged_kind"] in POLY | SPRITE:
            family = "poly" if p["staged_kind"] in POLY else "sprite"
            ok = len(got) == 1 and got[0].startswith(family)
            if ok:
                key = (family, p["semitransparent"])
                ok = route_by_semi.setdefault(key, got[0]) == got[0]
        elif p["staged_kind"] == DEFERRED:
            ok = got == ["move_a8f0"]
        else:
            ok = not got
        route_ok &= ok
        p["original_calls"] = got
    check("raster_route_per_primitive", route_ok,
          [{"node": p["node"], "staged_kind": p["staged_kind"], "semi": p["semitransparent"],
            "original": p["original_calls"]} for p in shim["primitives"]])
    head = [c["callee"] for c in calls if c["node"] is None]
    tail = [c["callee"] for c in calls[-4:]]
    check("frame_prologue", head == ["lock_6ff0"], head)
    check("frame_epilogue", tail == ["remap_70c0", "unlock_7140", "present_6620", "notify_bd80"]
          and calls[-4]["args"][0] == 0xFF, tail)

    receipt = {
        "kind": "host_shim_ac60_differential",
        "exe_sha256": PINNED,
        "ot_image_sha256": hashlib.sha256(image).hexdigest(),
        "primitives": len(shim_order),
        "route_by_semitransparent": {f"{k[0]}:{k[1]}": v for k, v in sorted(route_by_semi.items())},
        "all_ok": all(c["ok"] for c in checks),
        "checks": checks,
    }
    Path(receipt_path).write_text(json.dumps(receipt, indent=1) + "\n")
    for c in checks:
        print(("PASS " if c["ok"] else "FAIL ") + c["check"])
    print("ALL_OK" if receipt["all_ok"] else "NOT_OK")
    return 0 if receipt["all_ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
