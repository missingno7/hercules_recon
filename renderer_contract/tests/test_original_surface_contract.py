#!/usr/bin/env python3
"""Bounded static and synthetic-service audit of original 6FF0/7140/6620.

The original PE is mapped as data/code in Unicorn; only these original routines
and their bounded transition/error helper execute. DirectDraw and GDI methods
are synthetic spies. No Windows API, COM implementation, game process, or GPU
is loaded. Run with --exe, --repo-root and --output; output must be under work/.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

EXPECTED_EXE = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
EXPECTED_UNICORN_VERSION = "2.1.4"
EXPECTED_UNICORN_DLL_SHA256 = "010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1"
EXPECTED_CAPSTONE_VERSION = "5.0.7"
BASE = 0x400000
STACK = 0x20000000
STACK_SIZE = 0x10000
SP = STACK + 0x8000
RETURN = 0x10008000
SCRATCH = 0x10000000
PIXELS_DD = 0x30000000
PIXELS_DIB = 0x31000000
PRIMARY = 0x10001000
BACK = 0x10001100
VT_PRIMARY = 0x10002000
VT_BACK = 0x10002100
STUBS = {
    "islost": 0x10003000,
    "restore": 0x10003010,
    "lock": 0x10003020,
    "unlock": 0x10003030,
    "flip": 0x10003040,
    "stretch_dibits": 0x10003050,
    "gdi_flush": 0x10003060,
}
LOCK = 0x6FF0
UNLOCK = 0x7140
PRESENT = 0x6620
ADDR = {
    "lock_state": 0xBA01F0,
    "ddraw_mode": 0xBA020C,
    "pixels": 0xBA0210,
    "pitch": 0xBA0214,
    "dirty": 0x43A590,
    "primary": 0x96E3E8,
    "back": 0x96E3EC,
    "dib_pixels": 0x96E3D8,
    "dib_info": 0x96E3F4,
    "hdc": 0x96E400,
    "transition_guard": 0x96E40C,
    "requested_mode": 0x96E3D0,
    "gdi_stretch_iat": 0xBA5664,
    "gdi_flush_iat": 0xBA56F4,
    "gdi_destination_width": 0xBA0220,
    "gdi_destination_height": 0xBA0224,
}
ALLOWED_DATA_WRITE_PCS = {
    0x707E, 0x7083, 0x7089, 0x709B, 0x70A5, 0x70AB,
    0x7166, 0x6637, 0x6EC4,
}
MAX_INSTRUCTIONS = 20_000
ALLOWED_ORIGINAL_RANGES = (
    (0x6620, 0x66F5), (0x6BB0, 0x6EBA), (0x6EC0, 0x6ED2),
    (0x6FF0, 0x70B7), (0x7140, 0x716E),
)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def align(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path, help="checkout containing renderer_contract/ and scripts/")
    parser.add_argument("--output", type=Path, required=True, help="receipt path strictly under <repo-root>/work")
    parser.add_argument("--surface-header", type=Path, help="header proposal; defaults to renderer_contract/hercules_surface.h")
    parser.add_argument("--unicorn-deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
    parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    args.repo_root = args.repo_root.resolve()
    args.output = args.output.resolve()
    args.surface_header = (args.surface_header or (args.repo_root / "renderer_contract" / "hercules_surface.h")).resolve()
    args.unicorn_deps = args.unicorn_deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if not args.exe.is_file():
        parser.error("--exe does not exist")
    if sha(args.exe.read_bytes()) != EXPECTED_EXE:
        parser.error("--exe is not the pinned unpatched HERCULES.EXE")
    try:
        args.output.relative_to((args.repo_root / "work").resolve())
    except ValueError:
        parser.error("--output must resolve strictly under <repo-root>/work")
    if args.output == args.exe:
        parser.error("--output must not equal --exe")
    try:
        args.output.relative_to(args.exe.parent)
    except ValueError:
        pass
    else:
        parser.error("--output must be outside the original executable's directory")
    for path in (args.repo_root / "scripts" / "pe.py", args.unicorn_deps, args.analysis_deps, args.surface_header):
        if not path.exists():
            parser.error(f"required local dependency/path is missing: {path}")
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    sys.path[:0] = [str(args.unicorn_deps), str(args.repo_root / "scripts")]
    return args


ARGS = parse_args()
from pe import PE  # noqa: E402
import capstone  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_MODE_32,
    UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EAX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP,
)


def pe_permissions(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def put32(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def read32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


class SpyMachine:
    def __init__(self, pe: PE, name: str, config: dict):
        self.pe, self.name, self.config = pe, name, config
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.events = []
        self.data_writes = []
        self.trace = []
        self.error = None
        self.stopped = False
        self.uc.mem_map(BASE, align(pe.pe.OPTIONAL_HEADER.SizeOfHeaders), UC_PROT_READ)
        self.uc.mem_write(BASE, pe.data[:pe.pe.OPTIONAL_HEADER.SizeOfHeaders])
        for section in pe.sections:
            size = align(max(section["virtual_size"], section["raw_size"]))
            va = BASE + section["virtual_address"]
            self.uc.mem_map(va, size, pe_permissions(section["characteristics"]))
            if section["raw_size"]:
                self.uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
            if size > section["raw_size"]:
                self.uc.mem_write(va + section["raw_size"], bytes(size - section["raw_size"]))
        self.code_before = {
            rva: bytes(self.uc.mem_read(BASE + rva, size))
            for rva, size in ((0x6FF0, 208), (0x7140, 48), (0x6620, 224),
                              (0x6BB0, 0x310), (0x6EC0, 0x12))
        }
        self.uc.mem_map(SCRATCH, 0x10000, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        self.uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(PIXELS_DD, 0x10000, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(PIXELS_DIB, 0x10000, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_write(RETURN, b"\x90")
        self._setup()
        self.uc.hook_add(UC_HOOK_CODE, self._code)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self._write)

    def _setup(self):
        u = self.uc
        # Synthetic DirectDraw objects and method table entries.
        u.mem_write(PRIMARY, struct.pack("<I", VT_PRIMARY))
        u.mem_write(BACK, struct.pack("<I", VT_BACK))
        primary = [0] * 33
        back = [0] * 33
        primary[0x60 // 4] = STUBS["islost"]
        primary[0x6C // 4] = STUBS["restore"]
        primary[0x2C // 4] = STUBS["flip"]
        back[0x64 // 4] = STUBS["lock"]
        back[0x80 // 4] = STUBS["unlock"]
        u.mem_write(VT_PRIMARY, struct.pack("<33I", *primary))
        u.mem_write(VT_BACK, struct.pack("<33I", *back))
        pops = {"islost": 4, "restore": 4, "lock": 20, "unlock": 8,
                "flip": 12, "stretch_dibits": 52, "gdi_flush": 0}
        for name, addr in STUBS.items():
            pop = pops[name]
            u.mem_write(addr, b"\xc2" + struct.pack("<H", pop) if pop else b"\xc3")
        for key, value in self.config.get("memory", {}).items():
            put32(u, ADDR[key], value)
        # Non-default sentinels make accidental dependence on the incoming EAX
        # visible; the native ABI does not declare EAX as a status result.
        u.reg_write(UC_X86_REG_EAX, self.config.get("entry_eax", 0xA5A50000))

    def _code(self, uc, address, size, _):
        if address == RETURN:
            self.stopped = True
            uc.emu_stop()
            return
        if address >= BASE and address < 0x800000:
            rva = address - BASE
            if not any(lo <= rva < hi for lo, hi in ALLOWED_ORIGINAL_RANGES):
                self.error = f"unexpected original PC RVA {rva:#x}"
                uc.emu_stop()
                return
            self.trace.append(rva)
        elif address in STUBS.values():
            name = next(key for key, value in STUBS.items() if value == address)
            esp = uc.reg_read(UC_X86_REG_ESP)
            argc = {"islost": 1, "restore": 1, "lock": 5, "unlock": 2,
                    "flip": 3, "stretch_dibits": 13, "gdi_flush": 0}[name]
            args = [read32(uc, esp + 4 + 4 * i) for i in range(argc)]
            self.events.append({"service": name, "args": [hex(x) for x in args]})
            if name == "lock":
                if self.config.get("lock_hresult", 0) == 0:
                    desc = args[2]
                    put32(uc, desc + 0x10, self.config.get("returned_pitch", 336))
                    put32(uc, desc + 0x24, self.config.get("returned_pixels", PIXELS_DD))
            uc.reg_write(UC_X86_REG_EAX, self.config.get(name + "_hresult", 0))
        else:
            self.error = f"unexpected PC {address:#x}"
            uc.emu_stop()

    def _write(self, uc, access, address, size, value, _):
        pc = uc.reg_read(UC_X86_REG_EIP) - BASE
        if STACK <= address and address + size <= STACK + STACK_SIZE:
            return
        if address in (ADDR["lock_state"], ADDR["ddraw_mode"], ADDR["pixels"], ADDR["pitch"],
                       ADDR["dirty"], ADDR["requested_mode"]):
            event = {"pc_rva": hex(pc), "address": hex(address), "size": size,
                     "value": hex(value & ((1 << (size * 8)) - 1))}
            self.data_writes.append(event)
            if pc not in ALLOWED_DATA_WRITE_PCS:
                self.error = f"unexpected host-data write PC {pc:#x}"
                uc.emu_stop()
            return
        self.error = f"unexpected write at PC {pc:#x} to {address:#x}"
        uc.emu_stop()

    def run(self, rva: int):
        put32(self.uc, SP, RETURN)
        self.uc.reg_write(UC_X86_REG_ESP, SP)
        self.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        self.uc.reg_write(UC_X86_REG_EIP, BASE + rva)
        try:
            self.uc.emu_start(BASE + rva, 0, timeout=2_000_000, count=MAX_INSTRUCTIONS)
        except UcError as exc:
            if not self.error:
                self.error = str(exc)
        if self.error:
            raise RuntimeError(f"{self.name}: {self.error}")
        if not self.stopped:
            raise RuntimeError(f"{self.name}: did not reach return sentinel")
        for rva, before in self.code_before.items():
            if bytes(self.uc.mem_read(BASE + rva, len(before))) != before:
                raise RuntimeError(f"{self.name}: original code changed at RVA {rva:#x}")
        return {
            "case": self.name,
            "entry_rva": hex(rva),
            "trace_count": len(self.trace),
            "trace_ranges": sorted({hex(pc) for pc in self.trace if pc in (0x6FF0, 0x7140, 0x6620, 0x6BB0, 0x6EC0)}),
            "service_events": self.events,
            "host_data_writes": self.data_writes,
            "final": {key: hex(read32(self.uc, ADDR[key])) if key not in ("lock_state", "ddraw_mode", "dirty")
                      else hex(self.uc.mem_read(ADDR[key], 1)[0]) for key in
                      ("pixels", "pitch", "lock_state", "ddraw_mode", "dirty", "requested_mode")},
            "eax_after": hex(self.uc.reg_read(UC_X86_REG_EAX)),
            "original_code_unchanged": True,
        }


def run_case(pe: PE, name: str, rva: int, config: dict) -> dict:
    return SpyMachine(pe, name, config).run(rva)


def require_case(case: dict, writes: list[tuple[str, str, str]], services: list[str]) -> None:
    actual_writes = [(x["pc_rva"], x["address"], x["value"]) for x in case["host_data_writes"]]
    if actual_writes != writes:
        raise RuntimeError(f"{case['case']}: write sequence {actual_writes!r} != {writes!r}")
    actual_services = [x["service"] for x in case["service_events"]]
    if actual_services != services:
        raise RuntimeError(f"{case['case']}: service sequence {actual_services!r} != {services!r}")


def require_service_args(case: dict, service: str, expected: list[str]) -> None:
    row = next((x for x in case["service_events"] if x["service"] == service), None)
    if row is None or row["args"] != expected:
        raise RuntimeError(f"{case['case']}: {service} args {None if row is None else row['args']!r} != {expected!r}")


def static_facts(pe: PE) -> dict:
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    text = next(s for s in pe.sections if s["name"] == ".text")
    insns = list(md.disasm(pe.read_rva(text["virtual_address"], text["raw_size"]), BASE + text["virtual_address"]))
    targets = {0x6FF0: "lock", 0x7140: "unlock", 0x6620: "present"}
    callers = {name: [] for name in targets.values()}
    for item in insns:
        if item.mnemonic == "call" and item.operands and item.operands[0].type == capstone.x86.X86_OP_IMM:
            rva = item.operands[0].imm - BASE
            if rva in targets:
                callers[targets[rva]].append(item.address - BASE)
    expected = {
        "lock": [0x55E8, 0x6D0E, 0x6D59, 0x6D79, 0x7415, 0xAC92],
        "unlock": [0x5672, 0x69AB, 0x6BF6, 0x6D44, 0x6D5E, 0x6DC7, 0xB2D8],
        "present": [0x5677, 0x6764, 0x69BE, 0x6D6A, 0x6E9F, 0x9772, 0xB2DD],
    }
    for name in callers:
        if callers[name] != expected[name]:
            raise RuntimeError(f"{name} direct-call inventory changed: {callers[name]}")
    after = next(i for i, item in enumerate(insns) if item.address - BASE == 0x55ED)
    observed = insns[after]
    if observed.mnemonic != "mov" or observed.op_str != "al, byte ptr [0xba01f0]":
        raise RuntimeError("movie caller no longer tests the published lock-state byte after 6FF0")
    instructions_by_rva = {item.address - BASE: item for item in insns}
    mode_loads = {}
    for rva, expected_text in ((0x7001, "mov al, byte ptr [0xba020c]"),
                               (0x7149, "mov al, byte ptr [0xba020c]"),
                               (0x664B, "mov dl, byte ptr [0xba020c]")):
        item = instructions_by_rva.get(rva)
        actual = None if item is None else item.mnemonic + " " + item.op_str
        if actual != expected_text:
            raise RuntimeError(f"mode cell access width changed at RVA {rva:#x}: {actual}")
        mode_loads[hex(rva)] = actual
    spans = {}
    for name, rva, size in (("lock", 0x6FF0, 208), ("unlock", 0x7140, 48), ("present", 0x6620, 224)):
        spans[name] = {"module": "HERCULES.EXE", "rva": hex(rva), "size": size, "sha256": sha(pe.read_rva(rva, size))}
    return {"spans": spans, "direct_callers": {k: [hex(x) for x in v] for k, v in callers.items()},
            "movie_lock_consumer": {"rva": "0x55ed", "instruction": observed.mnemonic + " " + observed.op_str},
            "mode_cell_loads": mode_loads,
            "known_vtable_offsets": {"IsLost": "0x60", "Restore": "0x6c", "Lock": "0x64",
                                     "Unlock": "0x80", "Flip": "0x2c"}}


def main():
    if unicorn.__version__ != EXPECTED_UNICORN_VERSION:
        raise RuntimeError(f"expected Unicorn {EXPECTED_UNICORN_VERSION}, found {unicorn.__version__}")
    unicorn_path = Path(unicorn.__file__).resolve()
    try:
        unicorn_path.relative_to(ARGS.unicorn_deps)
    except ValueError as error:
        raise RuntimeError(f"Unicorn did not load from the pinned dependency root: {unicorn_path}") from error
    unicorn_dll = unicorn_path.parent / "lib" / "unicorn.dll"
    if sha(unicorn_dll.read_bytes()) != EXPECTED_UNICORN_DLL_SHA256:
        raise RuntimeError("unexpected Unicorn native DLL SHA-256")
    if capstone.__version__ != EXPECTED_CAPSTONE_VERSION:
        raise RuntimeError(f"expected Capstone {EXPECTED_CAPSTONE_VERSION}, found {capstone.__version__}")
    try:
        Path(capstone.__file__).resolve().relative_to(ARGS.analysis_deps)
    except ValueError as error:
        raise RuntimeError("Capstone did not load from the pinned analysis dependency root") from error
    pe = PE(ARGS.exe)
    facts = static_facts(pe)
    cases = [
        run_case(pe, "dib_lock_publishes_320_pitch", LOCK, {"memory": {
            "ddraw_mode": 0, "lock_state": 0, "dib_pixels": PIXELS_DIB,
            "pixels": 0x22222222, "pitch": 0x33333333}}),
        run_case(pe, "dd_lock_publishes_driver_pointer_and_pitch", LOCK, {"memory": {
            "ddraw_mode": 1, "lock_state": 0, "primary": PRIMARY, "back": BACK,
            "pixels": 0x22222222, "pitch": 0x33333333}}),
        run_case(pe, "dd_missing_backbuffer_publishes_zero_view", LOCK, {"memory": {
            "ddraw_mode": 1, "lock_state": 0, "primary": 0, "back": 0,
            "pixels": 0x22222222, "pitch": 0x33333333}}),
        run_case(pe, "lost_primary_restores_then_locks_even_if_restore_fails", LOCK, {"memory": {
            "ddraw_mode": 1, "lock_state": 0, "primary": PRIMARY, "back": BACK,
            "pixels": 0x22222222, "pitch": 0x33333333},
            "islost_hresult": 0x887601C2, "restore_hresult": 0x88760000}),
        run_case(pe, "lock_failure_keeps_published_view_under_transition_guard", LOCK, {"memory": {
            "ddraw_mode": 1, "lock_state": 0, "primary": PRIMARY, "back": BACK,
            "transition_guard": 1, "requested_mode": 1,
            "pixels": 0x22222222, "pitch": 0x33333333}, "lock_hresult": 0x88760000}),
        run_case(pe, "already_locked_is_noop", LOCK, {"memory": {
            "ddraw_mode": 1, "lock_state": 1, "primary": PRIMARY, "back": BACK,
            "pixels": 0x22222222, "pitch": 0x33333333}}),
        run_case(pe, "unlock_clears_lock_even_when_com_unlock_fails", UNLOCK, {"memory": {
            "ddraw_mode": 1, "lock_state": 1, "back": BACK}, "unlock_hresult": 0x88760000}),
        run_case(pe, "fullscreen_flip_failure_still_clears_dirty", PRESENT, {"memory": {
            "dirty": 1, "hdc": 0x11112222, "dib_pixels": PIXELS_DIB,
            "ddraw_mode": 1, "primary": PRIMARY}, "flip_hresult": 0x88760000}),
        run_case(pe, "fullscreen_missing_primary_clears_dirty_without_flip", PRESENT, {"memory": {
            "dirty": 1, "hdc": 0x11112222, "dib_pixels": PIXELS_DIB,
            "ddraw_mode": 1, "primary": 0}}),
        run_case(pe, "windowed_gdi_uses_dib_globals_and_clears_dirty", PRESENT, {"memory": {
            "dirty": 1, "hdc": 0x11112222, "dib_pixels": PIXELS_DIB,
            "dib_info": 0x22223333, "ddraw_mode": 0,
            "gdi_destination_width": 320, "gdi_destination_height": 240,
            "gdi_stretch_iat": STUBS["stretch_dibits"], "gdi_flush_iat": STUBS["gdi_flush"]}}),
    ]
    expected = {
        "dib_lock_publishes_320_pitch": ([('0x709b', '0xba0214', '0x140'), ('0x70a5', '0xba0210', hex(PIXELS_DIB)), ('0x70ab', '0xba01f0', '0x1')], []),
        "dd_lock_publishes_driver_pointer_and_pitch": ([('0x707e', '0xba0214', '0x150'), ('0x7083', '0xba0210', hex(PIXELS_DD)), ('0x7089', '0xba01f0', '0x1')], ['islost', 'lock']),
        "dd_missing_backbuffer_publishes_zero_view": ([('0x707e', '0xba0214', '0x0'), ('0x7083', '0xba0210', '0x0'), ('0x7089', '0xba01f0', '0x1')], []),
        "lost_primary_restores_then_locks_even_if_restore_fails": ([('0x707e', '0xba0214', '0x150'), ('0x7083', '0xba0210', hex(PIXELS_DD)), ('0x7089', '0xba01f0', '0x1')], ['islost', 'restore', 'lock']),
        "lock_failure_keeps_published_view_under_transition_guard": ([('0x6ec4', '0x96e3d0', '0x0')], ['islost', 'lock']),
        "already_locked_is_noop": ([], []),
        "unlock_clears_lock_even_when_com_unlock_fails": ([('0x7166', '0xba01f0', '0x0')], ['unlock']),
        "fullscreen_flip_failure_still_clears_dirty": ([('0x6637', '0x43a590', '0x0')], ['flip']),
        "fullscreen_missing_primary_clears_dirty_without_flip": ([('0x6637', '0x43a590', '0x0')], []),
        "windowed_gdi_uses_dib_globals_and_clears_dirty": ([('0x6637', '0x43a590', '0x0')], ['stretch_dibits', 'gdi_flush']),
    }
    for case in cases:
        require_case(case, *expected[case["case"]])
    cases_by_name = {case["case"]: case for case in cases}
    require_service_args(cases_by_name["dd_lock_publishes_driver_pointer_and_pitch"], "lock",
                         [hex(BACK), "0x0", "0x20007f94", "0x1", "0x0"])
    require_service_args(cases_by_name["dd_lock_publishes_driver_pointer_and_pitch"], "islost",
                         [hex(PRIMARY)])
    require_service_args(cases_by_name["lost_primary_restores_then_locks_even_if_restore_fails"], "islost",
                         [hex(PRIMARY)])
    require_service_args(cases_by_name["lost_primary_restores_then_locks_even_if_restore_fails"], "restore",
                         [hex(PRIMARY)])
    require_service_args(cases_by_name["unlock_clears_lock_even_when_com_unlock_fails"], "unlock",
                         [hex(BACK), "0x0"])
    require_service_args(cases_by_name["fullscreen_flip_failure_still_clears_dirty"], "flip",
                         [hex(PRIMARY), "0x0", "0x1"])
    require_service_args(cases_by_name["windowed_gdi_uses_dib_globals_and_clears_dirty"], "stretch_dibits",
                         ["0x11112222", "0x0", "0x0", "0x140", "0xf0", "0x0", "0x0",
                          "0x140", "0xf0", hex(PIXELS_DIB), "0x22223333", "0x0", "0xcc0020"])
    source_evidence = {}
    for relative in ("renderer_contract/presentation.md", "renderer_contract/closure.md",
                     "renderer_contract/ownership.md", "renderer_contract/evidence/presentation.json",
                     "renderer_contract/evidence/closure.json", "renderer_contract/evidence/palette_lifetime.json",
                     "renderer_contract/evidence/state_ownership.json", "renderer_contract/evidence/frame.json"):
        path = ARGS.repo_root / relative
        if path.is_file():
            source_evidence[relative] = sha(path.read_bytes())
    receipt = {
        "scope": "Pinned unchanged original x86 routines under Unicorn with synthetic COM/GDI spies; no OS, real DirectDraw, HWND, game process, or GPU.",
        "oracle": {"path": str(ARGS.exe), "sha256": sha(pe.data)},
        "tooling": {"python": sys.version.split()[0], "unicorn": unicorn.__version__,
                    "unicorn_module_path": str(unicorn_path),
                    "capstone": capstone.__version__, "capstone_module_path": str(Path(capstone.__file__).resolve()),
                    "unicorn_dll_sha256": sha(unicorn_dll.read_bytes()),
                    "instruction_budget_per_case": MAX_INSTRUCTIONS},
        "audit_script_sha256": sha(Path(__file__).read_bytes()),
        "snapshot_header_sha256": sha(ARGS.surface_header.read_bytes()),
        "source_evidence_sha256": source_evidence,
        "static": facts,
        "cases": cases,
        "limits": [
            "Lock-failure fixture sets the host transition-in-progress guard so 6BB0 returns before its window/display transition; this isolates 6FF0's first failure writes, not the complete live-mode transition.",
            "The DIB path's `stretch_dibits` arguments are recorded only as synthetic ABI arguments; no GDI raster result is modeled.",
            "A successful Lock descriptor is supplied by a synthetic COM spy, and no hardware pitch/format behavior is inferred.",
        ],
    }
    ARGS.output.parent.mkdir(parents=True, exist_ok=True)
    ARGS.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(ARGS.output), "case_count": len(cases),
                      "oracle_sha256": receipt["oracle"]["sha256"],
                      "span_hashes": facts["spans"]}, indent=2))


if __name__ == "__main__":
    main()
