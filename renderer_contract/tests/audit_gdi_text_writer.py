#!/usr/bin/env python3
"""Static and finite synthetic-endpoint audit for original HERCULES.EXE RVA 0x6440.

Only the hash-pinned 0x6440 function bytes execute in Unicorn. The formatter,
GDI, and DirectDraw surface calls are explicit synthetic providers. No Windows,
COM implementation, game process, or pixel renderer is used.
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
EXPECTED_SPAN = "5cbfc8880059aa88245af82dd01b50cf8c9811d790a53d21bd0b149a1fae9aeb"
EXPECTED_SOURCE_EVIDENCE = "6fac29e24a95b22257e7be29b4c2de0a644fc17872d7868c4ec4dfd2272fcfc3"
EXPECTED_PREDICTION = "3b0006e4cadd5217df9e143ffaaa9627fd3be3ceed86ef1356fe10d49c983786"
EXPECTED_CAPSTONE = "5.0.7"
EXPECTED_PYFILE = "2024.8.26"
EXPECTED_UNICORN = "2.1.4"
EXPECTED_UNICORN_DLL = "010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1"
BASE = 0x400000
ENTRY = 0x6440
END = 0x652C
SPAN_SIZE = END - ENTRY
SLOT149_CELL_RVA = 0x3640C
SLOT149_TARGET_VA = 0x406440
COUNTER = 0x96E408
DIRTY = 0x43A590
LOCK = 0xBA01F0
PIXELS = 0xBA0210
PITCH = 0xBA0214
MODE = 0xBA020C
HWND = 0xBA0200
BACK = 0x96E3EC
TEXT_IAT = 0xBA5658
GETDC_IAT = 0xBA5A9C
RELEASEDC_IAT = 0xBA5AA0
FORMAT = 0x410F40
TEXT_FORMAT_RVA = 0x36A10
STACK = 0x20000000
STACK_SIZE = 0x10000
SP = STACK + 0x8000
SCRATCH = 0x30000000
SCRATCH_SIZE = 0x10000
RETURN = SCRATCH + 0x8000
SURFACE = 0x31000000
VTABLE = 0x31001000
USER_GETDC = SCRATCH + 0x1000
TEXT_OUT = SCRATCH + 0x1100
USER_RELEASEDC = SCRATCH + 0x1200
SURFACE_GETDC = SCRATCH + 0x1300
SURFACE_RELEASEDC = SCRATCH + 0x1400
MAX_INSTRUCTIONS = 2000


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def norm(path: Path) -> str:
    return os.path.normcase(str(path))


def within(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def exact_path(path: Path, label: str, must_exist: bool = True) -> Path:
    if not path.is_absolute():
        raise SystemExit(f"{label} must be an explicit absolute path: {path}")
    lexical = Path(os.path.abspath(str(path)))
    try:
        resolved = path.resolve(strict=False)
    except OSError as exc:
        raise SystemExit(f"cannot resolve {label}: {exc}") from exc
    if norm(lexical) != norm(resolved):
        raise SystemExit(f"{label} redirects through a symlink/junction: {lexical} -> {resolved}")
    if must_exist and not path.exists():
        raise SystemExit(f"{label} does not exist: {path}")
    return lexical


def parse_args():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo-root", type=Path, required=True)
    ap.add_argument("--exe", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--unicorn-deps", type=Path, required=True)
    ap.add_argument("--analysis-deps", type=Path, required=True)
    args = ap.parse_args()
    args.repo_root = exact_path(args.repo_root, "--repo-root")
    script_repo_root = Path(__file__).resolve().parents[2]
    if norm(args.repo_root) != norm(script_repo_root):
        ap.error("--repo-root must be the repository containing this audit script")
    args.exe = exact_path(args.exe, "--exe")
    if not args.exe.is_file():
        ap.error("--exe must identify an existing read-only image file")
    args.work = exact_path(args.repo_root / "work", "repository work directory")
    args.private_dir = exact_path(args.work / "renderer_phase16" / "gdi_text_writer",
                                  "private package directory")
    out_lexical = exact_path(args.output, "--output", must_exist=False)
    if not out_lexical.parent.is_dir():
        ap.error("--output parent must already exist")
    if out_lexical.suffix.lower() != ".json" or out_lexical.exists():
        ap.error("--output must be a new JSON file; existing receipts are not overwritten")
    out_parent = exact_path(out_lexical.parent, "--output parent")
    args.output = out_parent / out_lexical.name
    out_resolved = args.output.resolve(strict=False)
    if norm(args.output) != norm(out_resolved):
        ap.error("--output redirects through a symlink/junction")
    if not within(args.output, args.private_dir) or not within(out_resolved, args.private_dir.resolve()):
        ap.error("--output must stay inside work/renderer_phase16/gdi_text_writer")
    assets = (args.repo_root / "assets").resolve(strict=False)
    if norm(args.output) == norm(args.exe) or within(args.output, args.exe):
        ap.error("--output may not overwrite the oracle image")
    if within(args.output, assets) or within(assets, args.output):
        ap.error("--output may not overlap assets")
    args.unicorn_deps = exact_path(args.unicorn_deps, "--unicorn-deps")
    args.analysis_deps = exact_path(args.analysis_deps, "--analysis-deps")
    if not args.unicorn_deps.is_dir() or not args.analysis_deps.is_dir():
        ap.error("dependency paths must be existing directories")
    sys.path[:0] = [str(args.unicorn_deps), str(args.analysis_deps)]
    return args


def align(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def pe_permissions(characteristics: int, unicorn_module) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= unicorn_module.UC_PROT_READ
    if characteristics & 0x80000000:
        result |= unicorn_module.UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= unicorn_module.UC_PROT_EXEC
    return result


def put32(uc, addr: int, value: int) -> None:
    uc.mem_write(addr, struct.pack("<I", value & 0xFFFFFFFF))


def get32(uc, addr: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(addr, 4)))[0]


def read_cstr(uc, addr: int, limit: int = 4096) -> bytes:
    raw = bytes(uc.mem_read(addr, limit))
    end = raw.find(b"\0")
    if end < 0:
        raise AssertionError(f"unterminated synthetic string at {addr:#x}")
    return raw[:end]


def map_image(uc, pe, unicorn_module) -> None:
    uc.mem_map(BASE, align(pe.pe.OPTIONAL_HEADER.SizeOfHeaders), unicorn_module.UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:pe.pe.OPTIONAL_HEADER.SizeOfHeaders])
    for sec in pe.sections:
        size = align(max(sec["virtual_size"], sec["raw_size"]))
        va = BASE + sec["virtual_address"]
        uc.mem_map(va, size, pe_permissions(sec["characteristics"], unicorn_module))
        if sec["raw_size"]:
            uc.mem_write(va, pe.read_rva(sec["virtual_address"], sec["raw_size"]))
        if size > sec["raw_size"]:
            uc.mem_write(va + sec["raw_size"], bytes(size - sec["raw_size"]))


class PEImage:
    """Small in-file pefile adapter; no repository PE helper is imported."""
    def __init__(self, path: Path, pefile_module):
        self.path = path
        self.data = path.read_bytes()
        self.pe = pefile_module.PE(data=self.data, fast_load=False)
        self.image_base = self.pe.OPTIONAL_HEADER.ImageBase
        self.sections = [
            {"virtual_address": section.VirtualAddress,
             "virtual_size": section.Misc_VirtualSize,
             "raw_offset": section.PointerToRawData,
             "raw_size": section.SizeOfRawData,
             "characteristics": section.Characteristics}
            for section in self.pe.sections
        ]

    def read_rva(self, rva: int, size: int) -> bytes:
        data = self.pe.get_data(rva, size)
        if len(data) != size:
            raise ValueError(f"truncated PE range at RVA {rva:#x}: wanted {size}, got {len(data)}")
        return data


def verify_tooling(capstone_module, unicorn_module, unicorn_deps: Path, analysis_deps: Path) -> dict:
    if capstone_module.__version__ != EXPECTED_CAPSTONE:
        raise SystemExit(f"expected Capstone {EXPECTED_CAPSTONE}; found {capstone_module.__version__}")
    import pefile
    if pefile.__version__ != EXPECTED_PYFILE:
        raise SystemExit(f"expected pefile {EXPECTED_PYFILE}; found {pefile.__version__}")
    if unicorn_module.__version__ != EXPECTED_UNICORN:
        raise SystemExit(f"expected Unicorn {EXPECTED_UNICORN}; found {unicorn_module.__version__}")
    module = Path(unicorn_module.__file__).resolve()
    try:
        module.relative_to(unicorn_deps.resolve())
    except ValueError as exc:
        raise SystemExit(f"Unicorn loaded outside --unicorn-deps: {module}") from exc
    for label, loaded in (("Capstone", Path(capstone_module.__file__).resolve()),
                          ("pefile", Path(pefile.__file__).resolve())):
        try:
            loaded.relative_to(analysis_deps.resolve())
        except ValueError as exc:
            raise SystemExit(f"{label} loaded outside --analysis-deps: {loaded}") from exc
    dll = module.parent / "lib" / "unicorn.dll"
    digest = sha(dll.read_bytes())
    if digest != EXPECTED_UNICORN_DLL:
        raise SystemExit(f"unexpected Unicorn native DLL SHA-256: {digest}")
    return {"python": sys.version.split()[0], "python_executable": sys.executable,
            "pefile": pefile.__version__, "pefile_module": str(Path(pefile.__file__).resolve()),
            "capstone": capstone_module.__version__,
            "capstone_module": str(Path(capstone_module.__file__).resolve()),
            "unicorn": unicorn_module.__version__,
            "unicorn_module": str(module), "unicorn_dll_sha256": digest,
            "unicorn_deps": str(unicorn_deps), "analysis_deps": str(analysis_deps)}


def decode_span(data: bytes, capstone_module):
    md = capstone_module.Cs(capstone_module.CS_ARCH_X86, capstone_module.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    rows = []
    for ins in md.disasm(data, BASE + ENTRY):
        if not ins.id:
            raise AssertionError(f"skipdata within pinned function at {ins.address - BASE:#x}")
        absolute_memory = []
        for operand in ins.operands:
            if (operand.type == capstone_module.x86_const.X86_OP_MEM
                    and operand.mem.base == 0 and operand.mem.index == 0):
                access = operand.access
                mode = ("read" if access & capstone_module.CS_AC_READ else "")
                if access & capstone_module.CS_AC_WRITE:
                    mode = (mode + "+write") if mode else "write"
                absolute_memory.append({"address": hex(operand.mem.disp & 0xFFFFFFFF),
                                        "access": mode or "unspecified"})
        rows.append({"rva": f"0x{ins.address - BASE:x}", "bytes": ins.bytes.hex(),
                     "mnemonic": ins.mnemonic, "operands": ins.op_str,
                     "absolute_memory_operands": absolute_memory})
    return rows


class WriterFixture:
    """Execute only the original 0x6440 bytes with declared synthetic call providers."""
    TARGETS = {
        FORMAT: (0x6469, "format_endpoint"),
        SURFACE_GETDC: (0x648B, "surface_getdc_endpoint"),
        TEXT_OUT: (None, "textout_endpoint"),
        USER_GETDC: (0x64DB, "user_getdc_endpoint"),
        SURFACE_RELEASEDC: (0x64CE, "surface_releasedc_endpoint"),
        USER_RELEASEDC: (0x651E, "user_releasedc_endpoint"),
    }

    def __init__(self, pe, unicorn_module, config: dict):
        self.pe, self.uc, self.config = pe, unicorn_module.Uc(
            unicorn_module.UC_ARCH_X86, unicorn_module.UC_MODE_32), config
        self.events = []
        self.global_writes = []
        self.error = None
        self.completed = False
        self.last_pc = None
        self.initial_sp = SP
        self.input_text = config["message"].encode("latin-1") + b"\0"
        self.uc.mem_map(STACK, STACK_SIZE, unicorn_module.UC_PROT_READ | unicorn_module.UC_PROT_WRITE)
        self.uc.mem_write(STACK, b"\xA5" * STACK_SIZE)
        self.uc.mem_map(SCRATCH, SCRATCH_SIZE,
                        unicorn_module.UC_PROT_READ | unicorn_module.UC_PROT_WRITE | unicorn_module.UC_PROT_EXEC)
        self.uc.mem_map(SURFACE, 0x2000,
                        unicorn_module.UC_PROT_READ | unicorn_module.UC_PROT_WRITE)
        map_image(self.uc, pe, unicorn_module)
        self.uc.mem_write(SCRATCH + 0x100, self.input_text)
        put32(self.uc, SP, RETURN)
        put32(self.uc, SP + 4, config["x"])
        put32(self.uc, SP + 8, config["y"])
        put32(self.uc, SP + 12, SCRATCH + 0x100)
        put32(self.uc, HWND, config["hwnd"])
        put32(self.uc, MODE, config["mode"])
        put32(self.uc, BACK, config["surface"])
        self.uc.mem_write(COUNTER, bytes((config["counter"] & 0xFF,)))
        put32(self.uc, DIRTY, 0x11223344)
        put32(self.uc, LOCK, 0x55667788)
        put32(self.uc, PIXELS, 0x12345678)
        put32(self.uc, PITCH, 0x23456789)
        self.frame_guard_before = {
            hex(address): bytes(self.uc.mem_read(address, 4)).hex()
            for address in (DIRTY, LOCK, PIXELS, PITCH)
        }
        put32(self.uc, GETDC_IAT, USER_GETDC)
        put32(self.uc, TEXT_IAT, TEXT_OUT)
        put32(self.uc, RELEASEDC_IAT, USER_RELEASEDC)
        if config["surface"]:
            put32(self.uc, SURFACE, VTABLE)
            put32(self.uc, VTABLE + 0x44, SURFACE_GETDC)
            put32(self.uc, VTABLE + 0x68, SURFACE_RELEASEDC)
        self.uc.reg_write(unicorn_module.x86_const.UC_X86_REG_ESP, SP)
        self.uc.reg_write(unicorn_module.x86_const.UC_X86_REG_EIP, BASE + ENTRY)
        self.uc.reg_write(unicorn_module.x86_const.UC_X86_REG_EDI, 0xDEADC0DE)
        self.uc.hook_add(unicorn_module.UC_HOOK_CODE, self.on_code)
        self.uc.hook_add(unicorn_module.UC_HOOK_MEM_WRITE, self.on_write)

    def on_write(self, uc, access, address, size, value, user_data):
        if STACK <= address and address + size <= STACK + STACK_SIZE:
            return
        if address == COUNTER and size == 1:
            self.global_writes.append({"address": hex(address), "size": size, "value": value & 0xFF})
            return
        self.error = f"unexpected guest memory write at {address:#x} size={size} value={value:#x}"
        uc.emu_stop()

    def set_return(self, cleanup: int, eax: int):
        esp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        return_address = get32(self.uc, esp)
        self.uc.reg_write(self.uc_module.x86_const.UC_X86_REG_EAX, eax & 0xFFFFFFFF)
        self.uc.reg_write(self.uc_module.x86_const.UC_X86_REG_ESP, esp + cleanup + 4)
        self.uc.reg_write(self.uc_module.x86_const.UC_X86_REG_EIP, return_address)
        self.uc.emu_stop()

    def on_code(self, uc, address, size, user_data):
        if address == RETURN:
            self.completed = True
            uc.emu_stop()
            return
        if address in self.TARGETS:
            expected_callsite, provider = self.TARGETS[address]
            previous = self.last_pc
            if expected_callsite is not None and previous != BASE + expected_callsite:
                self.error = (f"synthetic target {address:#x} arrived after {previous!r}; "
                              f"expected callsite {BASE + expected_callsite:#x}")
                uc.emu_stop()
                return
            if address == TEXT_OUT and previous not in (BASE + 0x64B7, BASE + 0x650C):
                self.error = f"synthetic TextOutA target arrived after unexpected callsite {previous!r}"
                uc.emu_stop()
                return
            self.last_pc = address
            getattr(self, provider)(previous - BASE if previous is not None else 0, size)
            return
        if not (BASE + ENTRY <= address < BASE + END):
            self.error = f"unknown PC/callee reached: {address:#x}"
            uc.emu_stop()
            return
        self.last_pc = address

    @property
    def uc_module(self):
        import unicorn
        return unicorn

    def format_endpoint(self, pc: int, size: int):
        sp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        dst, fmt_ptr = get32(self.uc, sp + 4), get32(self.uc, sp + 8)
        tag, source_ptr = get32(self.uc, sp + 12) & 0xFF, get32(self.uc, sp + 16)
        fmt = read_cstr(self.uc, fmt_ptr)
        source = read_cstr(self.uc, source_ptr)
        if fmt != b"(%c) %s " or len(source) >= 0x100:
            self.error = f"format provider received unexpected format/source: {fmt!r}, {source!r}"
            self.uc.emu_stop()
            return
        output = b"(" + bytes((tag,)) + b") " + source + b" "
        self.uc.mem_write(dst, output + b"\0")
        self.events.append({"provider": "synthetic_format_provider", "callsite_rva": hex(pc),
                            "destination": hex(dst), "format": fmt.decode("ascii"),
                            "tag_byte": tag, "source_bytes": source.hex(),
                            "output_bytes": output.hex(), "output_ascii": output.decode("latin-1")})
        self.set_return(0, len(output))

    def surface_getdc_endpoint(self, pc: int, size: int):
        sp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        this_ptr, out_ptr = get32(self.uc, sp + 4), get32(self.uc, sp + 8)
        row = {"provider": "synthetic_surface_getdc_provider", "callsite_rva": hex(pc),
               "this": hex(this_ptr), "out_hdc_pointer": hex(out_ptr),
               "hresult": self.config["surface_getdc_hresult"] & 0xFFFFFFFF,
               "writes_hdc": self.config["surface_getdc_writes_hdc"]}
        if self.config["surface_getdc_writes_hdc"]:
            put32(self.uc, out_ptr, self.config["surface_hdc"])
            row["hdc_out"] = hex(self.config["surface_hdc"])
        self.events.append(row)
        self.set_return(8, self.config["surface_getdc_hresult"])

    def textout_endpoint(self, pc: int, size: int):
        sp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        hdc, x, y, string_ptr, count = (get32(self.uc, sp + 4 * (i + 1)) for i in range(5))
        raw = bytes(self.uc.mem_read(string_ptr, min(count, 0x100)))
        if count > 0x100 or len(raw) != count:
            self.error = f"unexpected TextOutA count/pointer at {pc:#x}: {count}"
            self.uc.emu_stop()
            return
        self.events.append({"provider": "synthetic_textouta_provider", "callsite_rva": hex(pc),
                            "hdc": hex(hdc), "x_signed": struct.unpack("<i", struct.pack("<I", x))[0],
                            "y_signed": struct.unpack("<i", struct.pack("<I", y))[0],
                            "string_pointer": hex(string_ptr), "count": count,
                            "text_bytes": raw.hex(), "text_ascii": raw.decode("latin-1"),
                            "return_value": self.config["textout_result"]})
        self.set_return(20, self.config["textout_result"])

    def user_getdc_endpoint(self, pc: int, size: int):
        sp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        hwnd = get32(self.uc, sp + 4)
        self.events.append({"provider": "synthetic_user32_getdc_provider",
                            "callsite_rva": hex(pc), "hwnd_argument": hex(hwnd),
                            "hdc_return": hex(self.config["window_hdc"])})
        self.set_return(4, self.config["window_hdc"])

    def surface_releasedc_endpoint(self, pc: int, size: int):
        sp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        this_ptr, hdc = get32(self.uc, sp + 4), get32(self.uc, sp + 8)
        self.events.append({"provider": "synthetic_surface_releasedc_provider",
                            "callsite_rva": hex(pc), "this": hex(this_ptr), "hdc": hex(hdc),
                            "hresult": self.config["surface_releasedc_hresult"] & 0xFFFFFFFF})
        self.set_return(8, self.config["surface_releasedc_hresult"])

    def user_releasedc_endpoint(self, pc: int, size: int):
        sp = self.uc.reg_read(self.uc_module.x86_const.UC_X86_REG_ESP)
        hwnd, hdc = get32(self.uc, sp + 4), get32(self.uc, sp + 8)
        self.events.append({"provider": "synthetic_user32_releasedc_provider",
                            "callsite_rva": hex(pc), "hwnd_argument": hex(hwnd), "hdc": hex(hdc),
                            "return_value": self.config["releasedc_result"]})
        self.set_return(8, self.config["releasedc_result"])

    def run(self):
        import unicorn
        try:
            pc = BASE + ENTRY
            for _ in range(MAX_INSTRUCTIONS):
                self.uc.emu_start(pc, RETURN, count=MAX_INSTRUCTIONS)
                pc = self.uc.reg_read(unicorn.x86_const.UC_X86_REG_EIP)
                if pc == RETURN:
                    self.completed = True
                if self.error or self.completed:
                    break
        except unicorn.UcError as exc:
            self.error = f"Unicorn error: {exc}"
        if self.error:
            raise AssertionError(self.error)
        if not self.completed:
            raise AssertionError("original helper did not return to its sentinel")
        final_sp = self.uc.reg_read(unicorn.x86_const.UC_X86_REG_ESP)
        expected_sp = self.initial_sp + 4
        if final_sp != expected_sp:
            raise AssertionError(f"cdecl return stack mismatch: {final_sp:#x} != {expected_sp:#x}")
        if self.uc.reg_read(unicorn.x86_const.UC_X86_REG_EDI) != 0xDEADC0DE:
            raise AssertionError("callee-saved EDI was not restored")
        final_counter = bytes(self.uc.mem_read(COUNTER, 1))[0]
        expected_counter = (self.config["counter"] + 1) & 0xFF
        if final_counter != expected_counter:
            raise AssertionError(f"counter byte mutation mismatch: {final_counter} != {expected_counter}")
        frame_guard_after = {
            hex(address): bytes(self.uc.mem_read(address, 4)).hex()
            for address in (DIRTY, LOCK, PIXELS, PITCH)
        }
        if frame_guard_after != self.frame_guard_before:
            raise AssertionError("frame pointer/pitch/lock/dirty guard cells changed")
        return {"events": self.events, "global_writes": self.global_writes,
                "stack_return": {"entry_esp": hex(self.initial_sp), "final_esp": hex(final_sp),
                                 "callee_cleanup_bytes": 0,
                                 "caller_cleanup_bytes_required_after_return": 12,
                                 "fixture_stops_before_caller_cleanup": True},
                "counter_before": self.config["counter"], "counter_after": final_counter,
                "edi_restored": True, "frame_guard_cells_unchanged": True,
                "stop": "return_sentinel"}


def make_case(name: str, *, mode: int, surface: int, counter: int = 0,
              window_hdc: int = 0x70001000, surface_hresult: int = 0,
              surface_writes_hdc: bool = True, surface_hdc: int = 0x70002000,
              textout_result: int = 1, releasedc_result: int = 1,
              surface_release_hresult: int = 0) -> dict:
    return {"name": name, "mode": mode, "surface": surface, "counter": counter,
            "window_hdc": window_hdc, "surface_getdc_hresult": surface_hresult,
            "surface_getdc_writes_hdc": surface_writes_hdc, "surface_hdc": surface_hdc,
            "textout_result": textout_result, "releasedc_result": releasedc_result,
            "surface_releasedc_hresult": surface_release_hresult,
            "x": (-17) & 0xFFFFFFFF, "y": 777, "hwnd": 0x44556677,
            "message": "asset%20"}


def main() -> int:
    args = parse_args()
    import capstone
    import unicorn
    from unicorn import x86_const
    import pefile

    tools = verify_tooling(capstone, unicorn, args.unicorn_deps, args.analysis_deps)
    prediction_rel = Path("renderer_contract/fixtures/gdi_text_writer/prediction.json")
    prediction_path = exact_path(args.repo_root / prediction_rel, "shipped prediction")
    prediction_bytes = prediction_path.read_bytes()
    if sha(prediction_bytes) != EXPECTED_PREDICTION:
        raise SystemExit("shipped pre-experiment prediction hash changed")
    prediction_doc = json.loads(prediction_bytes.decode("utf-8"))
    evidence_path = args.repo_root / "renderer_contract" / "evidence" / "screen_ownership.json"
    evidence_bytes = evidence_path.read_bytes()
    if sha(evidence_bytes) != EXPECTED_SOURCE_EVIDENCE:
        raise SystemExit("historical screen_ownership evidence byte pin changed")
    evidence = json.loads(evidence_bytes.decode("utf-8-sig"))
    expected_exe = evidence["provenance"]["original_sha256"]["HERCULES.EXE"]
    expected_span = evidence["provenance"]["host_spans"]["0x6440"]
    exe_bytes = args.exe.read_bytes()
    if sha(exe_bytes) != EXPECTED_EXE or sha(exe_bytes) != expected_exe:
        raise SystemExit("--exe bytes do not match the pinned original HERCULES.EXE hash")
    pe = PEImage(args.exe, pefile)
    slot149_cell = struct.unpack("<I", pe.read_rva(SLOT149_CELL_RVA, 4))[0]
    if slot149_cell != SLOT149_TARGET_VA:
        raise SystemExit(f"slot-149 source cell changed: {slot149_cell:#x}")
    span = pe.read_rva(ENTRY, SPAN_SIZE)
    if len(span) != expected_span["size"] or sha(span) != EXPECTED_SPAN or sha(span) != expected_span["sha256"]:
        raise SystemExit("RVA 0x6440 bytes differ from pinned slot-149 helper span")
    rows = decode_span(span, capstone)
    absolute_writes = [operand["address"] for row in rows
                       for operand in row["absolute_memory_operands"]
                       if "write" in operand["access"]]
    if absolute_writes != [hex(COUNTER)]:
        raise SystemExit(f"unexpected absolute-memory write set: {absolute_writes}")
    direct_frame_refs = {hex(DIRTY), hex(LOCK), hex(PIXELS), hex(PITCH)}
    absolute_refs = {operand["address"] for row in rows
                     for operand in row["absolute_memory_operands"]}
    if direct_frame_refs & absolute_refs:
        raise SystemExit(f"function references frame gate/view cells: {sorted(direct_frame_refs & absolute_refs)}")
    ret_rows = [row for row in rows if row["mnemonic"] == "ret"]
    if [row["rva"] for row in ret_rows] != ["0x64d8", "0x652b"]:
        raise SystemExit(f"plain return sites changed: {[row['rva'] for row in ret_rows]}")
    decoded_rows_sha256 = sha(json.dumps(rows, sort_keys=True, separators=(",", ":")).encode("utf-8"))
    absolute_operand_receipt = [
        {"rva": row["rva"], **operand}
        for row in rows for operand in row["absolute_memory_operands"]
    ]
    call_sites = [int(row["rva"], 16) for row in rows if row["mnemonic"] == "call"]
    wanted_calls = [0x6469, 0x648B, 0x64B7, 0x64CE, 0x64DB, 0x650C, 0x651E]
    if call_sites != wanted_calls:
        raise SystemExit(f"unexpected original call instruction sequence: {[hex(x) for x in call_sites]}")
    format_bytes = pe.read_rva(TEXT_FORMAT_RVA, 9)
    if format_bytes != b"(%c) %s \0":
        raise SystemExit(f"unexpected prefix format string: {format_bytes!r}")
    imports = {}
    for mod in pe.pe.DIRECTORY_ENTRY_IMPORT:
        dll = mod.dll.decode("ascii", "replace").lower()
        for imp in mod.imports:
            if imp.name:
                imports[(dll, imp.name.decode("ascii", "replace"))] = imp.address
    expected_imports = {("gdi32.dll", "TextOutA"): TEXT_IAT,
                        ("user32.dll", "GetDC"): GETDC_IAT,
                        ("user32.dll", "ReleaseDC"): RELEASEDC_IAT}
    if any(imports.get(key) != expected for key, expected in expected_imports.items()):
        raise SystemExit("one or more pinned slot-149 IAT target identities changed")

    configs = [
        make_case("windowed_success", mode=0, surface=0, counter=0),
        make_case("windowed_getdc_and_textout_failure", mode=0, surface=0, counter=0xFF,
                  window_hdc=0, textout_result=0, releasedc_result=0),
        make_case("fullscreen_success_with_textout_failure", mode=1, surface=SURFACE,
                  surface_hresult=0, surface_writes_hdc=True, surface_hdc=0x70002000,
                  textout_result=0, surface_release_hresult=0x80004005),
        make_case("fullscreen_getdc_failure_no_output_write", mode=1, surface=SURFACE,
                  surface_hresult=0x80004005, surface_writes_hdc=False,
                  textout_result=0, surface_release_hresult=0x80004005),
        make_case("fullscreen_null_surface", mode=1, surface=0, counter=7,
                  textout_result=0),
    ]
    fixtures = []
    for config in configs:
        result = WriterFixture(pe, unicorn, config).run()
        events = result["events"]
        if not events or events[0]["provider"] != "synthetic_format_provider":
            raise AssertionError(f"{config['name']}: no expected formatter endpoint")
        text = next((row for row in events if row["provider"] == "synthetic_textouta_provider"), None)
        if text is None:
            raise AssertionError(f"{config['name']}: TextOutA endpoint was not reached")
        expected_tag = ord("0") + ((config["counter"] + 1) & 7)
        expected_text = b"(" + bytes((expected_tag,)) + b") asset%20 "
        if bytes.fromhex(text["text_bytes"]) != expected_text:
            raise AssertionError(f"{config['name']}: synthetic formatted text mismatch")
        if text["x_signed"] != -17 or text["y_signed"] != 777:
            raise AssertionError(f"{config['name']}: TextOutA coordinates are not raw caller values")
        if len(result["global_writes"]) != 1 or result["global_writes"][0]["address"] != hex(COUNTER):
            raise AssertionError(f"{config['name']}: unexpected persistent global write set")
        if config["mode"] == 0:
            getdc = next(row for row in events if row["provider"] == "synthetic_user32_getdc_provider")
            releasedc = next(row for row in events if row["provider"] == "synthetic_user32_releasedc_provider")
            if getdc["hwnd_argument"] != "0x0":
                raise AssertionError(f"{config['name']}: windowed GetDC argument is not NULL")
            if text["hdc"] != hex(config["window_hdc"]):
                raise AssertionError(f"{config['name']}: TextOutA did not receive the USER32 HDC return")
            if releasedc["hwnd_argument"] != hex(config["hwnd"]) or releasedc["hdc"] != text["hdc"]:
                raise AssertionError(f"{config['name']}: ReleaseDC args do not match the static contract")
        elif config["surface"]:
            acquire = next(row for row in events if row["provider"] == "synthetic_surface_getdc_provider")
            release = next(row for row in events if row["provider"] == "synthetic_surface_releasedc_provider")
            if acquire["out_hdc_pointer"] != hex(SP - 0x104):
                raise AssertionError(f"{config['name']}: COM HDC output pointer is not the consumed local slot")
            expected_hdc = hex(config["surface_hdc"] if config["surface_getdc_writes_hdc"] else 0xA5A5A5A5)
            if text["hdc"] != expected_hdc or release["hdc"] != expected_hdc:
                raise AssertionError(f"{config['name']}: TextOutA/release HDC differs from the output local")
            if release["this"] != hex(config["surface"]):
                raise AssertionError(f"{config['name']}: release did not reload the configured surface")
        elif text["hdc"] != "0xa5a5a5a5":
            raise AssertionError(f"{config['name']}: null surface did not preserve the synthetic uninitialized HDC")
        fixtures.append({"name": config["name"], "providers": [e["provider"] for e in events],
                         **result})

    expected_provider_orders = [
        ["synthetic_format_provider", "synthetic_user32_getdc_provider",
         "synthetic_textouta_provider", "synthetic_user32_releasedc_provider"],
        ["synthetic_format_provider", "synthetic_user32_getdc_provider",
         "synthetic_textouta_provider", "synthetic_user32_releasedc_provider"],
        ["synthetic_format_provider", "synthetic_surface_getdc_provider",
         "synthetic_textouta_provider", "synthetic_surface_releasedc_provider"],
        ["synthetic_format_provider", "synthetic_surface_getdc_provider",
         "synthetic_textouta_provider", "synthetic_surface_releasedc_provider"],
        ["synthetic_format_provider", "synthetic_textouta_provider"],
    ]
    for row, expected in zip(fixtures, expected_provider_orders):
        if row["providers"] != expected:
            raise AssertionError(f"{row['name']}: endpoint order {row['providers']} != {expected}")

    receipt = {
        "schema": "hercules-gdi-text-writer-audit-v1",
        "status": "PASS",
        "prediction": {"path": prediction_rel.as_posix(),
                       "resolved_path": str(prediction_path),
                       "sha256": sha(prediction_bytes),
                       "pre_experiment_prediction": prediction_doc["prediction"],
                       "falsifier": prediction_doc["falsifier"]},
        "scope": "Pinned original x86 body, stack/API argument flow, and finite synthetic endpoint outcomes only; no runtime callers, Windows behavior, or pixels/glyphs.",
        "oracle": {"path": str(args.exe), "sha256": sha(exe_bytes),
                   "image_base": hex(pe.image_base), "entry_rva": hex(ENTRY),
                   "span_size": SPAN_SIZE, "span_sha256": sha(span)},
        "source_evidence": {"path": "renderer_contract/evidence/screen_ownership.json",
                            "sha256": sha(evidence_bytes),
                            "slot149_cell_rva": hex(SLOT149_CELL_RVA),
                            "slot149_cell_target_va": hex(slot149_cell),
                            "scope": "byte-pin and callback-cell identity only; no trigger/caller conclusion"},
        "errata": {"historical_description": "windowed GetDC(original HWND)",
                   "corrected_native_call": "USER32!GetDC(literal NULL) at RVA 0x64DB",
                   "release_call": "USER32!ReleaseDC(current HWND [0xBA0200], local HDC) at RVA 0x651E",
                   "historical_screen_ownership_json_unchanged": True,
                   "caller_or_trigger": "unknown/not established"},
        "tool_provenance": tools,
        "static_contract": {
            "entry_stack": {"return_address": "[ESP+0x00]", "x": "DWORD [ESP+0x04]",
                            "y": "DWORD [ESP+0x08]", "message": "NUL-terminated byte pointer [ESP+0x0C]",
                            "callee_cleanup": 0, "caller_cleanup_required": 12,
                            "signature_shape": "three DWORD arguments, caller-cleaned / cdecl-shaped; caller-side dispatch and exact declared return type are not recovered",
                            "evidence": "third argument read at entry RVA 0x6445; x/y reused via frame-adjusted stack slots; epilog 0x6525 ADD ESP,0x104; RET 0x652B"},
            "format": {"rva": hex(TEXT_FORMAT_RVA), "bytes": format_bytes.hex(),
                       "text": format_bytes[:-1].decode("ascii"), "counter_byte_va": hex(COUNTER),
                       "formatter_target_va": hex(FORMAT),
                       "formatter_library_identity": "unresolved; no CRT/library body is reconstructed or executed",
                       "counter_order": ["read byte", "increment modulo 256", "write byte", "mask low 3 bits", "add ASCII '0'"],
                       "buffer": "256-byte local at entry ESP-0x100; four-byte local HDC slot at entry ESP-0x104",
                       "formatter_call_rva": "0x6469", "formatter_target_rva": "0x10f40",
                       "formatter_arguments": ["local buffer", "format string", "generated tag byte", "caller message pointer"],
                       "formatter_return_checked": False,
                       "text_count": "computed by scanning formatted local string to NUL; TextOutA count excludes NUL"},
            "dc_routes": {
                "windowed": {"condition": "byte [0xBA020C] == 0", "getdc": {"rva": "0x64DB", "import": "USER32.dll!GetDC", "hwnd_argument": "literal NULL (push 0)"},
                             "textout": "GDI32.dll!TextOutA at 0x650C: HDC, raw x, raw y, local formatted string, strlen count",
                             "release": {"rva": "0x651E", "import": "USER32.dll!ReleaseDC", "hwnd_argument": "DWORD [0xBA0200]", "hdc_argument": "same local returned HDC"},
                             "pairing": "GetDC argument is NULL; ReleaseDC argument is current HWND [0xBA0200]. The arguments differ.",
                             "Win32_documentation": {"NULL_GetDC_semantics": "DC for the entire screen", "source": "https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc", "scope": "API contract only; this audit does not execute Windows GDI or establish pixels/visibility"}},
                "fullscreen": {"condition": "byte [0xBA020C] != 0", "surface_cell": hex(BACK),
                               "acquire": {"rva": "0x648B", "receiver": "[0x96E3EC]", "vtable_offset": "0x44", "arguments": ["surface this", "pointer to local HDC"]},
                               "textout": "GDI32.dll!TextOutA at 0x64B7: HDC, raw x, raw y, local formatted string, strlen count",
                               "release": {"rva": "0x64CE", "receiver": "fresh reload of [0x96E3EC]", "vtable_offset": "0x68", "arguments": ["surface this", "local HDC"]},
                               "conditional_release": "fresh surface-pointer check after TextOutA; no release when null"}},
            "text_state": {"coordinates": "first and second caller DWORDs passed unchanged as signed x/y by the endpoint contract",
                            "prefix": "counter-derived ASCII digit 0..7 in `(%c) %s `; caller message used as `%s` data",
                            "font_color_background": "no font, text-color, background-color, alignment, or brush setter in this span; those effective DC/surface attributes are unresolved provider/current-context state",
                            "no_pixel_api_or_direct_frame_route": True},
            "failure_and_returns": [
                "windowed GetDC result is not tested; TextOutA and ReleaseDC are attempted even for a NULL HDC",
                "TextOutA result is not tested; cleanup call follows",
                "fullscreen surface GetDC HRESULT is not tested; TextOutA still uses local HDC output slot",
                "fullscreen ReleaseDC call follows TextOutA whenever a fresh [0x96E3EC] test is nonzero, independent of HRESULTs",
                "if fullscreen surface pointer is NULL, acquire and release are skipped but TextOutA still consumes the uninitialized local HDC slot",
                "release results and final EAX are not assigned a stable helper contract"
            ],
            "native_memory": {"written_global": {"address": hex(COUNTER), "width": 1, "effect": "incrementing counter byte"},
                               "absolute_memory_write_gate": absolute_writes,
                               "not_referenced_in_span": [hex(DIRTY), hex(LOCK), hex(PIXELS), hex(PITCH)],
                               "runtime_lock_or_dirty_gate": "none in this body; DC drawing can bypass those cells",
                               "other_surface_state_reads": [hex(MODE), hex(HWND), hex(BACK)],
                               "direct_framebuffer_copy_or_pixel_write": False},
            "decode_receipt": {"instruction_count": len(rows),
                               "normalized_rows_sha256": decoded_rows_sha256,
                               "absolute_memory_operands": absolute_operand_receipt},
            "call_sites": [{"rva": hex(rva), "target": target} for rva, target in zip(
                wanted_calls, ["local format provider target 0x10F40", "surface vtable +0x44",
                               "GDI32!TextOutA", "surface vtable +0x68", "USER32!GetDC",
                               "GDI32!TextOutA", "USER32!ReleaseDC"])],
            "iat_cells": {"GDI32.dll!TextOutA": hex(TEXT_IAT), "USER32.dll!GetDC": hex(GETDC_IAT),
                          "USER32.dll!ReleaseDC": hex(RELEASEDC_IAT),
                          "machine_checked_against_import_directory": True},
        },
        "fixtures": fixtures,
        "synthetic_endpoint_limits": {
            "providers": ["synthetic format provider recognizing only observed `(%c) %s ` format",
                          "synthetic USER32 GetDC/ReleaseDC return-value providers",
                          "synthetic GDI TextOutA argument/return provider",
                          "synthetic DirectDraw surface vtable +0x44/+0x68 HRESULT/HDC providers"],
            "format_target_identity": "unresolved for original target VA 0x410F40; fixture supplies only a named controlled provider for the pinned format call",
            "unknown_pc_or_callee": "fails immediately; only original RVA 0x6440..0x652B instructions and the exact listed external-call targets are allowed",
            "no_claims": ["no Windows GetDC/ReleaseDC semantics observed", "no DirectDraw COM implementation observed",
                          "no glyph rasterization or pixels observed", "no style/font/color values inferred",
                          "no slot-149 caller or trigger recovered", "no cross-thread or reentrant serialization established"]
        },
    }
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "PASS", "output": str(args.output), "span_sha256": sha(span),
                      "fixtures": len(fixtures), "provider_orders": [row["providers"] for row in fixtures],
                      "unknown_calls_fail": True}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
