"""Bounded unchanged-original AC60/A8F0/BD80 live-mutation fixture."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import struct
import sys
from pathlib import Path


SCRIPT = Path(__file__).resolve()


def find_root(path: Path) -> Path:
    for parent in path.parents:
        if (
            (parent / "scripts" / "pe.py").is_file()
            and (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file()
        ):
            return parent
    raise RuntimeError(f"cannot locate repository root from {path}")


ROOT = find_root(SCRIPT)
TESTS = ROOT / "renderer_contract" / "tests"
sys.path.insert(0, str(TESTS))
import oracle_common as common  # noqa: E402


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path, help="pinned original HERCULES.EXE")
    parser.add_argument("--original-root", type=Path, default=ROOT)
    parser.add_argument("--deps", type=Path, default=common.DEFAULT_DEPS)
    parser.add_argument("--analysis-deps", type=Path, default=common.DEFAULT_ANALYSIS_DEPS)
    parser.add_argument(
        "--receipt", type=Path,
        default=ROOT / "work" / "renderer_phase11" / "live_capture" / "live_capture.json",
    )
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    args.original_root = args.original_root.resolve()
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    args.receipt = args.receipt.resolve()
    if args.original_root != ROOT:
        parser.error(f"this fixture root is fixed at {ROOT}")
    if (ROOT / "work").resolve() != ROOT / "work":
        parser.error("refusing a redirected work directory")
    try:
        args.receipt.relative_to((ROOT / "work").resolve())
    except ValueError:
        parser.error("--receipt must stay inside the repository's ignored work directory")
    if args.receipt == args.exe or args.exe.parent in args.receipt.parents:
        parser.error("--receipt must be outside the original image directory")
    if not args.exe.is_file() or sha256(args.exe.read_bytes()) != common.EXPECTED_EXE_SHA256:
        parser.error("--exe must be the pinned original HERCULES.EXE SHA-256")
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("pinned Unicorn and existing analysis dependency directories are required")
    return args


ARGS = parse_args()

# Reuse the already-published, unchanged AC60 fixture's PE mapper and packet
# construction helpers. Its parser receives the same guarded paths; its main()
# is never called, and this experiment does not edit that source.
MIXED_RUNNER_PATH = TESTS / "original_ac60_mixed_runner.py"
_saved_argv = sys.argv[:]
sys.argv = [
    str(MIXED_RUNNER_PATH), "--exe", str(ARGS.exe),
    "--original-root", str(ROOT), "--deps", str(ARGS.deps),
    "--analysis-deps", str(ARGS.analysis_deps),
    "--receipt", str(ARGS.receipt),
]
_spec = importlib.util.spec_from_file_location("original_ac60_mixed_runner", MIXED_RUNNER_PATH)
if _spec is None or _spec.loader is None:
    raise RuntimeError(f"cannot import existing fixture API: {MIXED_RUNNER_PATH}")
R = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(R)
sys.argv = _saved_argv

Uc = R.Uc
UcError = R.UcError
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE  # noqa: E402
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX,
    UC_X86_REG_ECX,
    UC_X86_REG_EIP,
    UC_X86_REG_ESI,
    UC_X86_REG_ESP,
)


EXE = R.EXE_BASE
AC60 = EXE + 0xAC60
AC60_END = EXE + 0xB2F0
A8F0 = EXE + 0xA8F0
A8F0_END = EXE + 0xABF3
BD80 = EXE + 0xBD80
BD80_END = EXE + 0xBD96
CALLBACK = R.UNKNOWN_CALLBACK
SENTINEL = R.RETURN_SENTINEL
CALLBACK_CELL = R.CALLBACK_CELL
GATE_CELL = R.NOTIFICATION_GATE
STATE_CELLS = {0x00A8E6FC, 0x00A8E6F4, 0x00A8E6F8}
MAX_INSNS = 100_000
TEXTURE_BASE = R.TEXTURE_BASE
COMPOSITION_PIXELS = 0x34000000
COMPOSITION_PIXELS_SIZE = 0x14000
COMPOSITION_DIB_INFO = 0x35000000
COMPOSITION_HDC = 0x36000000
STRETCH_DIBITS_STUB = 0x10004000
GDI_FLUSH_STUB = 0x10004010
CONTROLLED_GDI = {
    STRETCH_DIBITS_STUB: ("StretchDIBits", 13, 52),
    GDI_FLUSH_STUB: ("GdiFlush", 0, 0),
}
ORIGINAL_SERVICE_RANGES = (
    (0x6EE0, 0x6FF0), (0x6FF0, 0x70B7), (0x70C0, 0x7136),
    (0x7140, 0x716E), (0x6620, 0x66F5),
)
PREDICTION = {
    "prediction": "Unchanged original AC60 will execute command 0x01 through original A8F0 for a finite zero-height rectangle, enter original BD80, synchronously dispatch the fixture callback only when the live A8E788 gate is nonzero, return to AC60, and reload the command's current link at ACA2 before consuming the next packet. AC60 will also reach its final BD80 call after the 0x40 terminator and reread the gate there.",
    "falsifiers": [
        "A8F0 does not reach BD80 for the zero-height rectangle, or makes a VRAM write before notification.",
        "The enabled synthetic callback endpoint is not reached from original BD80 with return PCs 0x40ABE9 or 0x40B2E7 on the native stack.",
        "After the endpoint service mutates command 0x01's link, AC60 consumes the pre-dispatch next node rather than the new link at ACA2.",
        "After the endpoint service mutates a future E1 payload, the original E1 boundary and state stores still observe its original value.",
        "Clearing A8E788 inside the first synchronous service does not affect BD80's later gate reload.",
    ],
    "scope": "Private bounded original-byte CPU fixture; four small synthetic chains, no gameplay, hardware, renderer pixels, or installed callback target claim.",
}


def prediction_data():
    prediction_path = SCRIPT.parent / "prediction.json"
    if prediction_path.is_file():
        raw = prediction_path.read_bytes()
        return json.loads(raw.decode("utf-8-sig")), raw
    raw = (json.dumps(PREDICTION, indent=2) + "\n").encode("utf-8")
    return PREDICTION, raw


def u32(machine, address: int) -> int:
    return struct.unpack("<I", bytes(machine.mem_read(address, 4)))[0]


def s32(machine, address: int) -> int:
    value = u32(machine, address)
    return value - (1 << 32) if value & 0x80000000 else value


def addr_name(address: int, names: dict[int, str]) -> str:
    return names.get(address, f"unmapped:{address:#x}")


def make_fixture(runner):
    addresses = {
        "e1_current": runner.INPUT_BASE + 0x1000,
        "cmd01": runner.INPUT_BASE + 0x1020,
        "e1_future": runner.INPUT_BASE + 0x1040,
        "ot_gate": runner.INPUT_BASE + 0x1060,
        "e1_alt": runner.INPUT_BASE + 0x1080,
    }
    values = {
        "e1_current": 0xE100017B,
        "e1_future": 0xE10001BB,
        "e1_alt": 0xE1000052,
    }
    packets = {
        "e1_current": runner.pack_packet(
            0xE1, 8, addresses["cmd01"], {4: struct.pack("<I", values["e1_current"])},
        ),
        "cmd01": runner.pack_packet(
            0x01, 0x18, addresses["e1_future"], {
                0x0C: runner.put_s16(15), 0x0E: runner.put_s16(240),
                0x10: runner.put_s16(5), 0x12: runner.put_s16(224),
                0x14: runner.put_s16(8), 0x16: runner.put_s16(0),
            },
        ),
        "e1_future": runner.pack_packet(
            0xE1, 8, addresses["ot_gate"], {4: struct.pack("<I", values["e1_future"])},
        ),
        "ot_gate": runner.pack_packet(0x40, 8, 0, {}),
        "e1_alt": runner.pack_packet(
            0xE1, 8, addresses["ot_gate"], {4: struct.pack("<I", values["e1_alt"])},
        ),
    }
    return addresses, values, packets


def snapshot_chain(machine, first: int, addresses, packet_bytes, max_nodes: int = 8):
    names = {address: name for name, address in addresses.items()}
    snapshots = []
    current = first
    seen = set()
    for _ in range(max_nodes):
        if current == 0:
            break
        if current in seen:
            raise AssertionError(f"fixture contains a link cycle at {current:#x}")
        seen.add(current)
        name = addr_name(current, names)
        raw = bytes(machine.mem_read(current, len(packet_bytes[name])))
        node = {
            "name": name,
            "address": hex(current),
            "tag": hex(raw[7]),
            "next": hex(struct.unpack("<I", raw[:4])[0]),
            "raw_packet_sha256": sha256(raw),
        }
        if raw[7] == 0xE1:
            node["e1_payload_dword"] = hex(struct.unpack("<I", raw[4:8])[0])
        if raw[7] == 0x01:
            node["destination_s16"] = list(struct.unpack("<2h", raw[0x0C:0x10]))
            node["embedded_rect_s16"] = list(struct.unpack("<4h", raw[0x10:0x18]))
        snapshots.append(node)
        current = struct.unpack("<I", raw[:4])[0]
    else:
        raise AssertionError(f"fixture chain exceeded {max_nodes} nodes")
    return snapshots


def first_divergence(expected, actual):
    count = max(len(expected), len(actual))
    for index in range(count):
        if index >= len(expected):
            return {"index": index, "kind": "unexpected_extra_packet", "actual": actual[index]}
        if index >= len(actual):
            return {"index": index, "kind": "missing_packet", "expected": expected[index]}
        left, right = expected[index], actual[index]
        if left["name"] != right["name"]:
            return {
                "index": index,
                "kind": "packet_order",
                "expected": left["name"],
                "consumed": right["name"],
            }
        if "e1_payload_dword" in left and left["e1_payload_dword"] != right.get("e1_payload_dword"):
            return {
                "index": index,
                "kind": "packet_field",
                "packet": left["name"],
                "field": "E1 DWORD at packet+4 (tag byte remains E1)",
                "expected": left["e1_payload_dword"],
                "consumed": right.get("e1_payload_dword"),
            }
        for key in ("destination_s16", "embedded_rect_s16"):
            if key in left and left[key] != right.get(key):
                return {
                    "index": index, "kind": "packet_field", "packet": left["name"],
                    "field": key, "expected": left[key], "consumed": right.get(key),
                }
    return None


def execute_case(pe, scenario: str):
    runner = R
    addresses, values, packets = make_fixture(runner)
    name_by_address = {address: name for name, address in addresses.items()}
    uc = runner.Uc(runner.UC_ARCH_X86, runner.UC_MODE_32)
    mapped = runner.map_original_sections(uc, pe)
    uc.mem_map(runner.INPUT_BASE, 0x4000, runner.UC_PROT_READ | runner.UC_PROT_WRITE)
    uc.mem_map(runner.STACK_BASE, runner.STACK_SIZE, runner.UC_PROT_READ | runner.UC_PROT_WRITE)
    uc.mem_map(COMPOSITION_PIXELS, COMPOSITION_PIXELS_SIZE, runner.UC_PROT_READ | runner.UC_PROT_WRITE)
    uc.mem_map(COMPOSITION_DIB_INFO, 0x1000, runner.UC_PROT_READ | runner.UC_PROT_WRITE)
    uc.mem_map(STRETCH_DIBITS_STUB, 0x1000, runner.UC_PROT_READ | runner.UC_PROT_WRITE | runner.UC_PROT_EXEC)
    uc.mem_map(SENTINEL, runner.PAGE, runner.UC_PROT_EXEC)
    # The mapped callback page contains no service code. The code hook below
    # intercepts this exact endpoint before Unicorn decodes/fetches an opcode.
    uc.mem_map(CALLBACK, runner.PAGE, runner.UC_PROT_EXEC)

    ot_va = runner.INPUT_BASE + 0x100
    uc.mem_write(ot_va, struct.pack("<I", addresses["e1_current"]))
    for name, address in addresses.items():
        uc.mem_write(address, packets[name])

    data_section = next(section for section in pe.sections if section["name"] == ".data")
    data_va = EXE + data_section["virtual_address"]
    data_size = runner.align_up(max(data_section["virtual_size"], data_section["raw_size"]))
    before_data = bytes(uc.mem_read(data_va, data_size))
    code_spans = {
        "AC60": (EXE + 0xAC60, 0xB2F0 - 0xAC60),
        "A8F0": (EXE + 0xA8F0, 0xABF3 - 0xA8F0),
        "BD80": (EXE + 0xBD80, 0xBD96 - 0xBD80),
        "LOCK_6FF0": (EXE + 0x6FF0, 0xD0),
        "REMAP_70C0": (EXE + 0x70C0, 0x80),
        "UNLOCK_7140": (EXE + 0x7140, 0x30),
        "PRESENT_6620": (EXE + 0x6620, 0xE0),
    }
    before_code = {
        name: bytes(uc.mem_read(va, size)) for name, (va, size) in code_spans.items()
    }

    fixture_cells = {
        0x00BA01FC: b"\x00",  # execute the original AC60 CPU-surface tail
        0x00BA020C: b"\x00",  # windowed DIB mode
        0x0096E3D8: struct.pack("<I", COMPOSITION_PIXELS),
        0x0096E3F4: struct.pack("<I", COMPOSITION_DIB_INFO),
        0x0096E400: struct.pack("<I", COMPOSITION_HDC),
        0x00BA0220: struct.pack("<I", 320),
        0x00BA0224: struct.pack("<I", 240),
        0x00BA5664: struct.pack("<I", STRETCH_DIBITS_STUB),
        0x00BA56F4: struct.pack("<I", GDI_FLUSH_STUB),
        0x0043A590: b"\x01",  # explicit seeded present request
        CALLBACK_CELL: struct.pack("<I", CALLBACK),
        GATE_CELL: struct.pack("<I", 0 if scenario == "gate_off" else 1),
    }
    old_fixture_cells = {
        address: bytes(uc.mem_read(address, len(value)))
        for address, value in fixture_cells.items()
    }
    for address, value in fixture_cells.items():
        uc.mem_write(address, value)

    predispatch = snapshot_chain(uc, addresses["e1_current"], addresses, packets)
    sp = runner.STACK_BASE + runner.STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<II", SENTINEL, ot_va))
    uc.reg_write(runner.UC_X86_REG_ESP, sp)
    uc.reg_write(runner.UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(runner.UC_X86_REG_EIP, AC60)

    state = {"stop": None, "detail": None, "instructions": 0}
    trace = []
    trace_steps = []
    packet_boundaries = []
    e1_reads = []
    live_link_reloads = []
    a8f0_calls = []
    a8f0_notification_sites = []
    bd80_entries = []
    bd80_gate_reloads = []
    callback_services = []
    writes = []
    data_end = data_va + data_size
    text_section = next(section for section in pe.sections if section["name"] == ".text")
    text_start = EXE + text_section["virtual_address"]
    text_end = text_start + runner.align_up(max(text_section["virtual_size"], text_section["raw_size"]))
    scratch_ranges = [
        (runner.INPUT_BASE, runner.INPUT_BASE + 0x4000),
        (runner.STACK_BASE, runner.STACK_BASE + runner.STACK_SIZE),
        (COMPOSITION_PIXELS, COMPOSITION_PIXELS + COMPOSITION_PIXELS_SIZE),
        (COMPOSITION_DIB_INFO, COMPOSITION_DIB_INFO + 0x1000),
        (STRETCH_DIBITS_STUB, STRETCH_DIBITS_STUB + 0x1000),
    ]
    external_services = []
    surface_writes = []

    def stop(kind, detail=None):
        state["stop"] = kind
        state["detail"] = detail
        uc.emu_stop()

    def on_code(machine, address, _size, _user):
        state["instructions"] += 1
        if state["instructions"] > MAX_INSNS:
            stop("instruction_budget", {"budget": MAX_INSNS})
            return
        if address == SENTINEL:
            stop("sentinel_return")
            return
        if address in CONTROLLED_GDI:
            service_name, argument_count, stack_pop = CONTROLLED_GDI[address]
            esp = machine.reg_read(UC_X86_REG_ESP)
            return_address = u32(machine, esp)
            arguments = [u32(machine, esp + 4 + index * 4) for index in range(argument_count)]
            external_services.append({
                "provider": "controlled synthetic Win32 endpoint; no OS/GDI call",
                "service": service_name,
                "return_address_rva": hex(return_address - EXE),
                "stack_arguments": [hex(value) for value in arguments],
                "return_value": 1,
                "instruction_count": state["instructions"],
            })
            machine.reg_write(UC_X86_REG_EAX, 1)
            machine.reg_write(UC_X86_REG_ESP, esp + 4 + stack_pop)
            machine.reg_write(UC_X86_REG_EIP, return_address)
            return
        if address == CALLBACK:
            esp = machine.reg_read(UC_X86_REG_ESP)
            return_address = u32(machine, esp)
            service = {
                "endpoint": hex(address),
                "endpoint_classification": "synthetic host service; no installed callback target claim",
                "esp": hex(esp),
                "stack_dwords": [hex(u32(machine, esp + offset)) for offset in (0, 4, 8, 12)],
                "return_address": hex(return_address),
                "return_rva": hex(return_address - EXE),
                "mutation": None,
                "instruction_count": state["instructions"],
            }
            index = len(callback_services)
            if index == 0 and scenario == "rewrite_link":
                old = u32(machine, addresses["cmd01"])
                machine.mem_write(addresses["cmd01"], struct.pack("<I", addresses["e1_alt"]))
                service["mutation"] = {
                    "address": hex(addresses["cmd01"]), "old": hex(old),
                    "new": hex(addresses["e1_alt"]), "meaning": "rewrite current command-01 next link",
                }
            elif index == 0 and scenario == "mutate_future_e1":
                old = u32(machine, addresses["e1_future"] + 4)
                new = 0xE1000042
                machine.mem_write(addresses["e1_future"] + 4, struct.pack("<I", new))
                service["mutation"] = {
                    "address": hex(addresses["e1_future"] + 4), "old": hex(old),
                    "new": hex(new), "meaning": "rewrite future E1 payload DWORD, retaining tag E1",
                }
            elif index == 0 and scenario == "disable_gate":
                old = u32(machine, GATE_CELL)
                machine.mem_write(GATE_CELL, struct.pack("<I", 0))
                service["mutation"] = {
                    "address": hex(GATE_CELL), "old": hex(old), "new": "0x0",
                    "meaning": "disable primary notification gate for the later BD80 call",
                }
            callback_services.append(service)
            stop("synthetic_callback", {"return_address": return_address})
            return

        rva = address - EXE
        if address == EXE + 0xACA2:
            current = machine.reg_read(UC_X86_REG_ESI)
            live_link_reloads.append({
                "pc_rva": "0xaca2",
                "current_packet": addr_name(current, name_by_address),
                "current_packet_va": hex(current),
                "link_value_before_original_mov": hex(u32(machine, current)),
                "instruction_count": state["instructions"],
            })
        elif address == EXE + 0xACA4:
            current = machine.reg_read(UC_X86_REG_ESI)
            if live_link_reloads:
                live_link_reloads[-1]["loaded_esi_after_original_mov"] = hex(current)
                live_link_reloads[-1]["loaded_packet_after_original_mov"] = addr_name(current, name_by_address)
        elif address == EXE + 0xACB9:
            packet = machine.reg_read(UC_X86_REG_ESI)
            tag = bytes(machine.mem_read(packet + 7, 1))[0]
            raw = {
                "name": addr_name(packet, name_by_address),
                "address": hex(packet),
                "tag": hex(tag),
                "next": hex(u32(machine, packet)),
                "instruction_boundary_rva": "0xacb9",
                "instruction_count": state["instructions"],
            }
            if tag == 0xE1:
                raw["e1_payload_dword"] = hex(u32(machine, packet + 4))
            if tag == 0x01:
                raw["destination_s16"] = list(struct.unpack("<2h", bytes(machine.mem_read(packet + 0x0C, 4))))
                raw["embedded_rect_s16"] = list(struct.unpack("<4h", bytes(machine.mem_read(packet + 0x10, 8))))
            packet_boundaries.append(raw)
        elif address == EXE + 0xB19C:
            packet = machine.reg_read(UC_X86_REG_ESI)
            e1_reads.append({
                "pc_rva": "0xb19c",
                "packet": addr_name(packet, name_by_address),
                "packet_va": hex(packet),
                "payload_dword_read_by_next_original_instruction": hex(u32(machine, packet + 4)),
                "instruction_count": state["instructions"],
            })
        elif address == A8F0:
            esp = machine.reg_read(UC_X86_REG_ESP)
            rect_va = u32(machine, esp + 4)
            event = {
                "entry_rva": "0xa8f0",
                "callsite_return_rva": hex(u32(machine, esp) - EXE),
                "esp": hex(esp),
                "stack_dwords": [hex(u32(machine, esp + offset)) for offset in (0, 4, 8, 12)],
                "rect_pointer": hex(rect_va),
                "destination_s16": [s32(machine, esp + 8), s32(machine, esp + 12)],
                "rect_s16": list(struct.unpack("<4h", bytes(machine.mem_read(rect_va, 8)))),
                "instruction_count": state["instructions"],
            }
            a8f0_calls.append(event)
        elif address == EXE + 0xABE4:
            a8f0_notification_sites.append({
                "pc_rva": "0xabe4",
                "meaning": "original zero-height path reaches call BD80 without entering either copy loop",
                "instruction_count": state["instructions"],
            })
        elif address == BD80:
            esp = machine.reg_read(UC_X86_REG_ESP)
            bd80_entries.append({
                "pc_rva": "0xbd80",
                "call_return_rva": hex(u32(machine, esp) - EXE),
                "esp": hex(esp),
                "callback_pointer_at_entry": hex(u32(machine, CALLBACK_CELL)),
                "gate_memory_at_entry": hex(u32(machine, GATE_CELL)),
                "instruction_count": state["instructions"],
            })
        elif address == EXE + 0xBD8F:
            bd80_gate_reloads.append({
                "pc_rva": "0xbd8f",
                "gate_value_loaded_by_original_mov": hex(machine.reg_read(UC_X86_REG_ECX)),
                "instruction_count": state["instructions"],
            })

        allowed = (
            AC60 <= address < AC60_END
            or A8F0 <= address < A8F0_END
            or BD80 <= address < BD80_END
            or any(start <= rva < end for start, end in ORIGINAL_SERVICE_RANGES)
        )
        if not allowed:
            stop("unexpected_code", {"pc": hex(address), "rva": hex(rva)})
            return
        trace.append(hex(rva))
        trace_steps.append({"rva": hex(rva), "instruction_count": state["instructions"]})

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if text_start <= address and address + size <= text_end:
            stop("write_to_original_code", {"pc": hex(pc), "address": hex(address), "size": size})
            return
        if data_va <= address and address + size <= data_end:
            event = {
                "pc_rva": hex(pc - EXE), "address": hex(address), "size": size,
                "value": hex(value & ((1 << (size * 8)) - 1)),
                "instruction_count": state["instructions"],
            }
            writes.append(event)
            state_write = pc in (EXE + 0xB1B0, EXE + 0xB1BC, EXE + 0xB1CA) and address in STATE_CELLS and size == 4
            fixture_write = any(
                start <= address and address + size <= start + len(value)
                for start, value in fixture_cells.items()
            )
            surface_write = any(
                start <= address and address + size <= start + width
                for start, width in ((0x00BA0210, 4), (0x00BA0214, 4), (0x00BA01F0, 1))
            )
            if not (state_write or fixture_write or surface_write):
                stop("unexpected_original_data_write", event)
            return
        if COMPOSITION_PIXELS <= address and address + size <= COMPOSITION_PIXELS + COMPOSITION_PIXELS_SIZE:
            surface_writes.append({
                "writer_rva": hex(pc - EXE), "offset": hex(address - COMPOSITION_PIXELS),
                "size": size, "value": hex(value & ((1 << (size * 8)) - 1)),
                "instruction_count": state["instructions"],
            })
            return
        if any(start <= address and address + size <= end for start, end in scratch_ranges):
            if runner.INPUT_BASE <= address < runner.INPUT_BASE + 0x4000:
                stop("original_write_to_packet_fixture", {
                    "pc": hex(pc), "address": hex(address), "size": size,
                })
                return
            return
        stop("unexpected_write", {"pc": hex(pc), "address": hex(address), "size": size})

    def on_invalid(machine, access, address, size, _value, _user):
        stop("invalid_memory", {
            "pc": hex(machine.reg_read(UC_X86_REG_EIP)), "access": access,
            "address": hex(address), "size": size,
        })
        return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)

    while True:
        state["stop"] = None
        state["detail"] = None
        try:
            uc.emu_start(uc.reg_read(UC_X86_REG_EIP), 0, timeout=2_000_000)
        except UcError as error:
            if state["stop"] is None:
                state["stop"] = "unicorn_error"
                state["detail"] = str(error)
        if state["stop"] == "synthetic_callback":
            esp = uc.reg_read(UC_X86_REG_ESP)
            return_address = u32(uc, esp)
            # The explicitly synthetic host endpoint returns by RET semantics;
            # no synthetic machine instruction is placed in the mapped page.
            uc.reg_write(UC_X86_REG_ESP, esp + 4)
            uc.reg_write(UC_X86_REG_EIP, return_address)
            continue
        if state["stop"] == "sentinel_return":
            break
        raise AssertionError(f"original fixture stopped unexpectedly: {state}")

    expected_returns = ["0xabe9", "0xb2e7"]
    if [entry["call_return_rva"] for entry in bd80_entries] != expected_returns:
        raise AssertionError(f"unexpected original BD80 call boundaries: {bd80_entries}")
    if [entry["callsite_return_rva"] for entry in a8f0_calls] != ["0xb1ec"]:
        raise AssertionError(f"unexpected command-01 A8F0 entry: {a8f0_calls}")
    if a8f0_calls[0]["rect_s16"] != [5, 224, 8, 0]:
        raise AssertionError(f"zero-height command RECT mismatch: {a8f0_calls}")
    if len(a8f0_notification_sites) != 1:
        raise AssertionError(f"zero-height A8F0 path did not reach its original BD80 call: {a8f0_notification_sites}")
    if any(0xA8F0 <= int(event["pc_rva"], 16) < 0xABF3 for event in writes):
        raise AssertionError(f"zero-height A8F0 made an unexpected data/VRAM write: {writes}")
    if [entry["gate_value_loaded_by_original_mov"] for entry in bd80_gate_reloads] != (
        ["0x0", "0x0"] if scenario == "gate_off" else
        ["0x1", "0x0"] if scenario == "disable_gate" else
        ["0x1", "0x1"]
    ):
        raise AssertionError(f"original BD80 did not reload the expected live gate: {bd80_gate_reloads}")
    expected_callback_count = 0 if scenario == "gate_off" else (1 if scenario == "disable_gate" else 2)
    if len(callback_services) != expected_callback_count:
        raise AssertionError(f"synthetic endpoint invocation count mismatch for {scenario}: {callback_services}")
    if [event["return_rva"] for event in callback_services] != (
        [] if scenario == "gate_off" else
        ["0xabe9"] if scenario == "disable_gate" else
        ["0xabe9", "0xb2e7"]
    ):
        raise AssertionError(f"synthetic callback stack return addresses mismatch: {callback_services}")
    if [event["name"] for event in packet_boundaries] != (
        ["e1_current", "cmd01", "e1_future", "ot_gate"]
        if scenario != "rewrite_link" else
        ["e1_current", "cmd01", "e1_alt", "ot_gate"]
    ):
        raise AssertionError(f"unexpected native-consumed packet order: {packet_boundaries}")
    if [entry["packet"] for entry in e1_reads] != (
        ["e1_current", "e1_future"] if scenario != "rewrite_link" else
        ["e1_current", "e1_alt"]
    ):
        raise AssertionError(f"unexpected original E1 read boundaries: {e1_reads}")
    expected_e1_values = {
        "e1_current": values["e1_current"],
        "e1_future": 0xE1000042 if scenario == "mutate_future_e1" else values["e1_future"],
        "e1_alt": values["e1_alt"],
    }
    for event in e1_reads:
        wanted = expected_e1_values[event["packet"]]
        if int(event["payload_dword_read_by_next_original_instruction"], 16) != wanted:
            raise AssertionError(f"original E1 read did not observe live payload: {event}, expected {wanted:#x}")
    expected_write_sequence = []
    for event in e1_reads:
        value = int(event["payload_dword_read_by_next_original_instruction"], 16)
        expected_write_sequence.extend([
            ("0xb1b0", "0xa8e6fc", 4, hex(1 if value & 0x180 == 0x180 else 0)),
            ("0xb1bc", "0xa8e6f4", 4, hex(value & 0x1F)),
            ("0xb1ca", "0xa8e6f8", 4, hex((value >> 5) & 3)),
        ])
    actual_write_sequence = [
        (event["pc_rva"], event["address"], event["size"], event["value"])
        for event in writes
        if int(event["pc_rva"], 16) in (0xB1B0, 0xB1BC, 0xB1CA)
    ]
    if actual_write_sequence != expected_write_sequence:
        raise AssertionError(f"original E1 state write trace mismatch: {actual_write_sequence}")
    if not live_link_reloads or not any(
        event.get("current_packet") == "cmd01" for event in live_link_reloads
    ):
        raise AssertionError(f"AC60 command link did not cross its original ACA2 reload: {live_link_reloads}")

    actual_stream = packet_boundaries
    divergence = first_divergence(predispatch, actual_stream)
    if scenario == "rewrite_link":
        move_reload = next(event for event in live_link_reloads if event.get("current_packet") == "cmd01")
        if move_reload.get("loaded_packet_after_original_mov") != "e1_alt":
            raise AssertionError(f"AC60 did not use callback-rewritten live link at ACA2: {move_reload}")
        if not divergence or divergence.get("index") != 2 or divergence.get("consumed") != "e1_alt":
            raise AssertionError(f"expected earliest consumed-stream divergence at index 2: {divergence}")
    elif scenario == "mutate_future_e1":
        if not divergence or divergence.get("index") != 2 or divergence.get("kind") != "packet_field":
            raise AssertionError(f"expected earliest future-E1 field divergence at index 2: {divergence}")
    elif divergence is not None:
        raise AssertionError(f"unmutated fixture diverged from predispatch snapshot: {divergence}")

    if scenario == "gate_off" and any(event["mutation"] is not None for event in callback_services):
        raise AssertionError("gate-off baseline unexpectedly called host service")
    if scenario == "disable_gate" and u32(uc, GATE_CELL) != 0:
        raise AssertionError("synthetic callback did not disable live primary gate")

    after_data = bytes(uc.mem_read(data_va, data_size))
    post_run_fixture_state = {
        "cmd01_next": hex(u32(uc, addresses["cmd01"])),
        "future_e1_payload": hex(u32(uc, addresses["e1_future"] + 4)),
        "notification_gate": hex(u32(uc, GATE_CELL)),
    }
    changed = {
        data_va + offset for offset, (before, after) in enumerate(zip(before_data, after_data))
        if before != after
    }
    expected_changed = {
        address + i
        for address, value in fixture_cells.items()
        for i in range(len(value))
    }
    expected_changed.update(address + i for address in STATE_CELLS for i in range(4))
    expected_changed.update(range(0x00BA0210, 0x00BA0218))
    expected_changed.add(0x00BA01F0)
    if not changed <= expected_changed:
        raise AssertionError(f".data writes escaped fixture and three E1 state cells: {[hex(a) for a in sorted(changed - expected_changed)]}")
    code_unchanged = {
        name: bytes(uc.mem_read(va, size)) == before_code[name]
        for name, (va, size) in code_spans.items()
    }
    if not all(code_unchanged.values()):
        raise AssertionError(f"original code span changed: {code_unchanged}")

    # Restore all data addresses observed changed plus every host fixture cell,
    # then verify the mapped original .data bytes exactly match their baseline.
    for address in changed:
        uc.mem_write(address, before_data[address - data_va:address - data_va + 1])
    for address, before in old_fixture_cells.items():
        uc.mem_write(address, before)
    if bytes(uc.mem_read(data_va, data_size)) != before_data:
        raise AssertionError("mapped .data did not restore byte-for-byte after the fixture")

    return {
        "scenario": scenario,
        "predispatch_snapshot": predispatch,
        "native_consumed_packet_boundaries": packet_boundaries,
        "earliest_stream_divergence": divergence,
        "e1_original_read_boundaries": e1_reads,
        "live_ac60_link_reload_boundaries": live_link_reloads,
        "a8f0_original_entry": a8f0_calls,
        "a8f0_reaches_original_notification_call": a8f0_notification_sites,
        "bd80_original_call_boundaries": bd80_entries,
        "bd80_live_gate_reload_boundaries": bd80_gate_reloads,
        "synthetic_host_service_calls": callback_services,
        "original_data_write_events": writes,
        "final_fixture_bytes_before_restore": post_run_fixture_state,
        "original_instruction_count": state["instructions"],
        "original_instruction_trace_rvas": trace,
        "original_instruction_trace_events": trace_steps,
        "distinct_original_pcs_executed": len(set(trace)),
        "original_code_spans_unchanged": code_unchanged,
        "data_write_addresses_within_fixture_and_e1_cells": True,
        "no_original_a8f0_data_or_vram_writes": True,
        "private_mapped_data_restored_byte_for_byte": True,
        "controlled_external_services": external_services,
        "cpu_surface_write_events": surface_writes,
        "cpu_surface_write_byte_count": sum(event["size"] for event in surface_writes),
        "cpu_surface_final_sha256": sha256(bytes(uc.mem_read(COMPOSITION_PIXELS, COMPOSITION_PIXELS_SIZE))),
        "surface_route_fixture": {
            "mode": "windowed DIB",
            "pixel_token": hex(COMPOSITION_PIXELS),
            "dib_info_token": hex(COMPOSITION_DIB_INFO),
            "hdc_token": hex(COMPOSITION_HDC),
            "seeded_present_request": True,
            "graphics_endpoint_provider": "controlled synthetic Win32 endpoint; no OS/GDI call",
        },
        "mapped_original_sections": [
            {"name": section["name"], "va": hex(section["va"]), "size": hex(section["size"])}
            for section in mapped
        ],
    }


def main():
    pe = R.PE(ARGS.exe)
    if pe.image_base != EXE or getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("expected fixed-base original PE with no base relocations")
    runtime = common.verify_runtime(R.unicorn, ARGS.deps)
    prediction, prediction_bytes = prediction_data()
    cases = [
        execute_case(pe, "gate_off"),
        execute_case(pe, "rewrite_link"),
        execute_case(pe, "mutate_future_e1"),
        execute_case(pe, "disable_gate"),
    ]
    script_hash = sha256(SCRIPT.read_bytes())
    runner_hash = sha256(MIXED_RUNNER_PATH.read_bytes())
    common_hash = sha256((TESTS / "oracle_common.py").read_bytes())
    spans = {
        "AC60": {"rva": "0xac60", "size": 0xB2F0 - 0xAC60},
        "A8F0": {"rva": "0xa8f0", "size": 0xABF3 - 0xA8F0},
        "BD80": {"rva": "0xbd80", "size": 0xBD96 - 0xBD80},
        "LOCK_6FF0": {"rva": "0x6ff0", "size": 0xD0},
        "REMAP_70C0": {"rva": "0x70c0", "size": 0x80},
        "UNLOCK_7140": {"rva": "0x7140", "size": 0x30},
        "PRESENT_6620": {"rva": "0x6620", "size": 0xE0},
    }
    for name, details in spans.items():
        rva, size = int(details["rva"], 16), details["size"]
        details["sha256"] = sha256(pe.read_rva(rva, size))
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": "Four finite state-only AC60 chains execute unchanged original AC60, A8F0 and BD80 plus the original 6FF0/70C0/7140/6620 CPU-surface/presenter route in windowed-DIB mode. A callback endpoint mutates fixture state; synthetic StretchDIBits/GdiFlush providers return controlled values. The event stream checks connected native instruction order but establishes no raster pixels, glyphs, OS behavior, gameplay, or global writer closure.",
        "prediction": prediction,
        "oracle": {
            "path": str(ARGS.exe),
            "sha256": common.EXPECTED_EXE_SHA256,
            "image_base": hex(pe.image_base),
            "base_relocations": 0,
            "executed_spans": spans,
        },
        "fixture": {
            "entry_rva": "0xac60",
            "initial_order": ["e1_current", "cmd01", "e1_future", "ot_gate"],
            "command01_rect_signed_words": [5, 224, 8, 0],
            "command01_destination_signed_words": [15, 240],
            "host_device_tail_gate_BA01FC": 0,
            "windowed_dib_surface": True,
            "present_request_seeded": True,
            "notification_pointer": hex(CALLBACK),
            "callback_page_has_no_machine_code": True,
            "callback_behavior": "Synthetic host service records stack and mutates only the selected fixture bytes, then returns by setting ESP=ESP+4 and EIP=[ESP]; no target instruction is emitted or executed at the endpoint.",
            "scenarios": [case["scenario"] for case in cases],
        },
        "execution": {
            **runtime,
            "architecture": "x86 32-bit",
            "sections_mapped_at_original_virtual_addresses": True,
            "instruction_budget_per_case": MAX_INSNS,
            "unknown_pc_memory_write_or_invalid_access_fails": True,
            "original_code_is_unmodified": True,
            "only_fixture_and_three_E1_state_cells_may_change_in_original_data": True,
            "callback_and_return_sentinel_are_host_intercepted": True,
        },
        "source_provenance": {
            "this_runner": str(SCRIPT),
            "this_runner_sha256": script_hash,
            "reused_original_mixed_fixture_api": str(MIXED_RUNNER_PATH),
            "reused_fixture_api_sha256": runner_hash,
            "oracle_common": str(TESTS / "oracle_common.py"),
            "oracle_common_sha256": common_hash,
            "prediction_sha256": sha256(prediction_bytes),
        },
        "cases": cases,
        "interpretation_limits": [
            "The predispatch snapshot is passive observation and does not mutate or replace original traversal; it is compared with packets selected by unchanged AC60 at RVA 0xACB9.",
            "The synthetic service is a fixture endpoint, not a recovered or installed primary callback implementation. Its selected mutation demonstrates that original AC60/A8F0 exposes live state across the synchronous call boundary.",
            "The command-01 rectangle has zero height. Original A8F0 reaches its BD80 call and makes no memory writes; no positive-area VRAM copy output is tested here.",
            "AC60 reacquiring the link at ACA2 imposes a compatibility constraint on any future consumer of mutable packet lists. This fixture does not test GPU asynchronous visibility, pixels, resource updates, or hardware equivalence.",
            "Four small well-formed chains only; no allocator extent, malformed list, installed callback target, source identity, FUNCTION_MATCH, module layout, or whole-image equality is established.",
        ],
        "invocation": {
            "exe": str(ARGS.exe),
            "original_root": str(ROOT),
            "deps": str(ARGS.deps),
            "analysis_deps": str(ARGS.analysis_deps),
            "receipt": str(ARGS.receipt),
        },
    }
    common.write_receipt(ARGS.receipt, receipt)
    compact = {
        "status": receipt["status"],
        "oracle_sha256": receipt["oracle"]["sha256"],
        "scenarios": [
            {
                "name": case["scenario"],
                "consumed": [node["name"] for node in case["native_consumed_packet_boundaries"]],
                "earliest_stream_divergence": case["earliest_stream_divergence"],
                "callback_returns": [event["return_rva"] for event in case["synthetic_host_service_calls"]],
                "live_gate_reloads": [event["gate_value_loaded_by_original_mov"] for event in case["bd80_live_gate_reload_boundaries"]],
            }
            for case in cases
        ],
        "receipt": str(ARGS.receipt),
    }
    print(json.dumps(compact, indent=2))


if __name__ == "__main__":
    main()
