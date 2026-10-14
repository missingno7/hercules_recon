"""Original A710 -> AC60/D420/CAB0 in one bounded, synthetic CPU image.

Prediction: detached word uploads feed the known rectangle samples exactly;
transparent accepted input sets CC37's request with a zero initial dirty byte.
Falsifiers: wrong upload/sample/store, no transparent request, unknown endpoint
or changed original code. No game/OS process or strict matching acceptance.
"""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "renderer_contract/tests"))
from oracle_common import verify_runtime


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--case", choices=("base", "offset_rectangle", "transparent"), required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(ROOT / "renderer_contract/fixtures/eng1_real_mesh"))
    from pathguard import _under_work, input_file
    args.exe = input_file(args.exe, "original EXE")
    receipt_path = _under_work(ROOT, args.receipt)
    if receipt_path.exists():
        raise ValueError("receipt must be fresh")
    assert hashlib.sha256(args.exe.read_bytes()).hexdigest() == "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
    helper = ROOT / "renderer_contract/fixtures/composition/raster/tests/run_original_composition_raster.py"
    old_argv = sys.argv[:]
    sys.argv = [str(helper), "--repo-root", str(ROOT), "--exe", str(args.exe),
                "--deps", "C:/tools/hercules/renderer-oracle-python",
                "--analysis-deps", "C:/tools/hercules/python"]
    spec = importlib.util.spec_from_file_location("uploaded_rectangle_mapper", helper)
    assert spec and spec.loader
    R = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(R)
    sys.argv = old_argv
    runtime = verify_runtime(R.unicorn, Path("C:/tools/hercules/renderer-oracle-python"))
    pe = R.PE(args.exe)
    uc = R.Uc(R.UC_ARCH_X86, R.UC_MODE_32)
    R.map_original(uc, pe)
    for address, length, permissions in (
        (R.INPUT, 0x4000, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.STACK, R.STACK_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.PIXELS, R.PIXEL_SIZE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.DIB_INFO, R.PAGE, R.UC_PROT_READ | R.UC_PROT_WRITE),
        (R.STRETCH_STUB, R.PAGE, R.UC_PROT_READ | R.UC_PROT_EXEC),
        (R.SENTINEL, R.PAGE, R.UC_PROT_READ | R.UC_PROT_EXEC)):
        uc.mem_map(address, length, permissions)
    x, y, width, height, u, v = (4, 5, 3, 2, 2, 1) if args.case == "offset_rectangle" else (1, 1, 2, 2, 0, 0)
    payload = bytes((0x11, 0x22, 0x33, 0, 0x44, 0x55, 0x66, 0)) if width == 3 else bytes((0, 0x5a, 0x77, 0x6b))
    if args.case == "transparent":
        payload = bytes(4)
    upload_rect = (u // 2, v, (width + 1) // 2, height)
    rect_address, source_address = R.INPUT + 0x2000, R.INPUT + 0x2100
    uc.mem_write(rect_address, struct.pack("<4h", *upload_rect))
    uc.mem_write(source_address, payload)
    uc.mem_write(R.TEXTURE, bytes(65536))
    uc.mem_write(R.PIXELS, bytes((0xa5,)) * R.PIXEL_SIZE)
    packet_address, end_address, ot = R.INPUT + 0x1000, R.INPUT + 0x1040, R.INPUT + 0x100
    packet = R.packet(0x2c, 40, end_address, {
        4: bytes((0x64, 0x75, 0x86)), 8: R.s16(x), 10: R.s16(y),
        12: bytes((u, v)), 14: R.u16(0), 16: R.s16(x + width), 18: R.s16(y),
        20: bytes((u + width, v)), 22: R.u16(0), 24: R.s16(x), 26: R.s16(y + height),
        28: bytes((u, v + height)), 30: R.u16(0), 32: R.s16(x + width),
        34: R.s16(y + height), 36: bytes((u + width, v + height)), 38: R.u16(0)})
    uc.mem_write(packet_address, packet)
    uc.mem_write(end_address, R.packet(0x40, 8, 0, {}))
    R.put32(uc, ot, packet_address)
    cells = {
        0x00BA01FC: b"\0", 0x00BA020C: b"\0", 0x0096E698: b"\0",
        0x0096E3D8: struct.pack("<I", R.PIXELS), 0x0096E3F4: struct.pack("<I", R.DIB_INFO),
        0x0096E400: struct.pack("<I", R.HDC_TOKEN), 0x00BA0220: struct.pack("<I", 320),
        0x00BA0224: struct.pack("<I", 240), 0x00BA5664: struct.pack("<I", R.STRETCH_STUB),
        0x00BA56F4: struct.pack("<I", R.FLUSH_STUB), 0x0043A590: b"\0",
        0x00A90A04: struct.pack("<I", 0x7800), 0x0096E3E0: bytes(4),
        0x00436A0C: b"\x80", 0x00A8E780: bytes(4), 0x00A8E788: bytes(4)}
    for address, value in cells.items():
        uc.mem_write(address, value)
    spans = [(lo, hi - lo) for lo, hi, _ in R.ALLOWED] + [(0xA710, 0x1d5)]
    before = [bytes(uc.mem_read(R.BASE + lo, size)) for lo, size in spans]
    state = {"phase": "upload", "stop": None, "instructions": 0}
    uploads, reads, stores, dirty, services, routes = [], [], [], [], [], []

    def code(machine, address, size, context):
        state["instructions"] += 1
        if state["instructions"] > 100000:
            raise AssertionError("instruction budget")
        if address == R.SENTINEL:
            state["stop"] = "returned"
            machine.emu_stop()
            return
        if address in (R.STRETCH_STUB, R.FLUSH_STUB):
            assert state["phase"] == "frame"
            services.append("StretchDIBits" if address == R.STRETCH_STUB else "GdiFlush")
            stack = machine.reg_read(R.UC_X86_REG_ESP)
            machine.reg_write(R.UC_X86_REG_EIP, R.u32(machine, stack))
            machine.reg_write(R.UC_X86_REG_ESP, stack + (56 if address == R.STRETCH_STUB else 4))
            machine.reg_write(R.UC_X86_REG_EAX, 1)
            return
        rva = address - R.BASE
        assert any(lo <= rva < lo + size for lo, size in spans), hex(rva)
        if state["phase"] == "frame" and rva in (0xD420, 0xCAB0):
            routes.append(hex(rva))

    def read(machine, access, address, size, value, context):
        if state["phase"] == "frame" and R.TEXTURE <= address < R.TEXTURE + 65536:
            reads.append({"address": address, "bytes": bytes(machine.mem_read(address, size)).hex(),
                          "pc_rva": hex(machine.reg_read(R.UC_X86_REG_EIP) - R.BASE)})

    def write(machine, access, address, size, value, context):
        raw = int(value & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
        pc = machine.reg_read(R.UC_X86_REG_EIP) - R.BASE
        if R.TEXTURE <= address < R.TEXTURE + 65536:
            assert state["phase"] == "upload"
            uploads.append({"address": address, "bytes": raw.hex(), "pc_rva": hex(pc)})
        elif R.PIXELS <= address < R.PIXELS + R.PIXEL_SIZE:
            assert state["phase"] == "frame"
            stores.append({"offset": address - R.PIXELS, "bytes": raw.hex(), "pc_rva": hex(pc)})
        if address == 0x43a590:
            dirty.append({"phase": state["phase"], "pc_rva": hex(pc), "value": value})

    def invalid(machine, access, address, size, value, context):
        raise AssertionError(f"unmapped endpoint {address:#x}")

    uc.hook_add(R.UC_HOOK_CODE, code)
    uc.hook_add(R.UC_HOOK_MEM_READ, read)
    uc.hook_add(R.UC_HOOK_MEM_WRITE, write)
    uc.hook_add(R.UC_HOOK_MEM_INVALID, invalid)
    stack = R.STACK + R.STACK_SIZE - 0x100
    uc.mem_write(stack, struct.pack("<III", R.SENTINEL, rect_address, source_address))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(R.BASE + 0xA710, 0, count=100001)
    assert state["stop"] == "returned"
    expected_texture = bytearray(65536)
    for row in range(height):
        start = (v + row) * 256 + u
        row_bytes = payload[row * upload_rect[2] * 2:(row + 1) * upload_rect[2] * 2]
        expected_texture[start:start + len(row_bytes)] = row_bytes
    assert bytes(uc.mem_read(R.TEXTURE, 65536)) == expected_texture
    state.update(phase="frame", stop=None)
    uc.mem_write(stack, struct.pack("<II", R.SENTINEL, ot))
    uc.reg_write(R.UC_X86_REG_ESP, stack)
    uc.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    uc.emu_start(R.BASE + 0xAC60, 0, count=100001)
    assert state["stop"] == "returned"
    expected = bytearray((0xa5,)) * (320 * 240)
    for row in range(height):
        for column in range(width):
            texel = expected_texture[(v + row) * 256 + u + column]
            if texel:
                expected[(y + row) * 320 + x + column] = texel
    output = bytes(uc.mem_read(R.PIXELS, 320 * 240))
    assert output == expected
    assert routes == ["0xd420", "0xcab0"]
    assert services == ["StretchDIBits", "GdiFlush"]
    assert any(event["pc_rva"] == "0xcc37" and event["value"] == 1 for event in dirty), dirty
    assert before == [bytes(uc.mem_read(R.BASE + lo, size)) for lo, size in spans]
    if args.case == "transparent":
        assert not stores
    receipt = {"schema_version": 1, "status": "BOUNDED_ORIGINAL_UPLOAD_RECTANGLE_PASS",
               "original_process_executed": False, "prediction": __doc__, "case": args.case,
               "oracle_sha256": hashlib.sha256(pe.data).hexdigest(), "runtime": runtime,
               "helper_sha256": hashlib.sha256(helper.read_bytes()).hexdigest(),
               "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "spans": [{"rva": hex(lo), "size": size, "sha256": hashlib.sha256(raw).hexdigest()}
                         for (lo, size), raw in zip(spans, before)],
               "input": {"packet_address": packet_address, "packet_hex": packet.hex(),
                         "upload_rect": upload_rect, "upload_payload_hex": payload.hex(), "initial_pixel": 0xa5},
               "upload_stores": uploads, "texture_reads": reads, "surface_stores": stores,
               "dirty_events": dirty, "services": services, "routes": routes,
               "frame_sha256": hashlib.sha256(output).hexdigest(),
               "scope": "three separately controlled finite cases; no palette conversion, visible image, game or whole raster proof"}
    receipt_path.parent.mkdir(parents=True, exist_ok=True)
    receipt_path.write_bytes((json.dumps(receipt, indent=2) + "\n").encode())
    print(json.dumps({"status": receipt["status"], "case": args.case, "pixel_stores": len(stores), "dirty_events": dirty}))


if __name__ == "__main__":
    main()
