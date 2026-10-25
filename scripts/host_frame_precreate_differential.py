"""Fresh bounded CMainFrame::PreCreateWindow source-versus-retail runner.

It compiles only the current canonical class unit, links the pinned VC5 static MFC/CRT/import archives with
fixture-only link boundaries, compares the fresh COFF contributions, then calls
only PreCreateWindow directly under Unicorn. It never starts Windows, MFC, the
game, a window, or a real API.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

for _p in (Path(r"C:\tools\hercules\renderer-oracle-python"),
           Path(r"C:\tools\hercules\python")):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

ORACLE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
CL_SHA256 = "bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb"
LINK_SHA256 = "279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1"
NATIVE_BASE = 0x400000
NATIVE_FUNCTION = NATIVE_BASE + 0x9510
FUNCTION_SYMBOL = "?PreCreateWindow@CMainFrame@@MAEHAAUtagCREATESTRUCTA@@@Z"
NATIVE_MFC = {
    NATIVE_BASE + 0x2C272: "AfxGetModuleState",
    NATIVE_BASE + 0x23322: "AfxRegisterWndClass",
    NATIVE_BASE + 0x2A01F: "CFrameWnd::PreCreateWindow",
}

STACK_BASE = 0xC00000
STACK_SIZE = 0x20000
STACK_TOP = STACK_BASE + STACK_SIZE - 0x100
SENTINEL = STACK_BASE + 0x1FFF0
IMPORT_BASE = 0xD00000
DATA_BASE = 0xE00000
RECEIVER = DATA_BASE + 0x3000
CREATE = DATA_BASE + 0x2010
MODULE_STATE = DATA_BASE
RESOURCE_HANDLE = 0x76543210
CLASS_NAME = DATA_BASE + 0x100
CURSOR_HANDLE = 0x11112222
ICON_HANDLE = 0x33334444
CREATE_SIZE = 48
RECEIVER_SIZE = 188
API_ARGUMENT_BYTES = {"GetSystemMetrics": 4, "SystemParametersInfoA": 16,
                      "LoadCursorA": 8, "LoadIconA": 8}

CASES = [
    {"name": "center_nonzero_origin", "metrics": {45: 10, 7: 10, 46: 8, 8: 10, 15: 25, 4: 20},
     "work": (-120, 50, 1180, 950), "spi_success": 1, "base_result": 1},
    {"name": "exact_fit_clamps_each_axis", "metrics": {45: 10, 7: 10, 46: 8, 8: 10, 15: 25, 4: 20},
     "work": (100, 200, 460, 521), "spi_success": 1, "base_result": 1},
    {"name": "both_axes_clamp", "metrics": {45: 10, 7: 10, 46: 8, 8: 10, 15: 25, 4: 20},
     "work": (-8, 12, 300, 252), "spi_success": 1, "base_result": 1},
    {"name": "height_only_clamps", "metrics": {45: 10, 7: 10, 46: 8, 8: 10, 15: 25, 4: 20},
     "work": (-400, 80, 600, 360), "spi_success": 1, "base_result": 1},
    {"name": "false_spi_with_written_output", "metrics": {45: 2, 7: 3, 46: 4, 8: 5, 15: 10, 4: 15},
     "work": (33, -27, 933, 673), "spi_success": 0, "base_result": 1},
    {"name": "small_frame_and_base_false", "metrics": {45: 1, 7: 4, 46: 2, 8: 3, 15: 10, 4: 15},
     "work": (-220, 140, 580, 740), "spi_success": 1, "base_result": 0},
    {"name": "center_preserves_negative_result", "metrics": {45: 10, 7: 10, 46: 8, 8: 10, 15: 25, 4: 20},
     "work": (17, -31, 417, 350), "spi_success": 1, "base_result": 1},
]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def repository_root(start: Path) -> Path:
    for candidate in (start, *start.parents):
        if ((candidate / "scripts").is_dir() and (candidate / "src/pc").is_dir()
                and (candidate / "toolchains" / "manifest.json").is_file()):
            return candidate
    raise RuntimeError("cannot derive repository root from runner installation path")


def aligned(value: int, boundary: int = 0x1000) -> int:
    return (value + boundary - 1) & ~(boundary - 1)


def source_pins(root: Path) -> dict[str, str]:
    evidence_path = root / "evidence" / "host_frame_class_recovery.json"
    evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    pins = {row["path"].replace("\\", "/"): row["sha256"] for row in evidence["sources"]}
    required = {"src/pc/host_window_class.cpp", "src/pc/host_system_keys.h"}
    if not required.issubset(pins):
        raise RuntimeError(f"durable host-frame evidence lacks source pins: {required - set(pins)}")
    return {str((root / name).resolve()): pins[name] for name in sorted(required)}


def preprocessor_dependencies(compiler: Path, source: Path, includes: list[Path],
                              env: dict[str, str], cwd: Path) -> tuple[list[Path], dict]:
    command = [str(compiler), "/nologo", "/E", "/showIncludes", "/Gy", "/O2", "/MT", "/GX",
               *["/I" + str(p) for p in includes], str(source)]
    proc = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                          text=True, errors="replace", timeout=90)
    output = proc.stdout + proc.stderr
    paths = []
    for line in output.splitlines():
        # VC5's /E output identifies every opened source/header with a #line
        # directive; it does not implement the later /showIncludes prose format.
        match = re.match(r'^\s*#(?:line\s+)?\d+\s+"([^"\r\n]+)"', line)
        if match:
            path = Path(match.group(1).replace("/", os.sep)).resolve()
            if path.is_file():
                paths.append(path)
    unique = sorted(set(paths), key=lambda p: str(p).lower())
    record = {"command": command, "cwd": str(cwd), "returncode": proc.returncode,
              "stdout_sha256": hashlib.sha256(proc.stdout.encode()).hexdigest(),
              "stderr_sha256": hashlib.sha256(proc.stderr.encode()).hexdigest(),
              "include_count": len(unique), "includes": [str(p) for p in unique]}
    if proc.returncode != 0 or not unique:
        raise RuntimeError(f"pinned compiler dependency scan failed for {source}: {record}")
    return unique, record


def compile_one(compiler: Path, source: Path, obj: Path, includes: list[Path],
                env: dict[str, str], cwd: Path) -> dict:
    command = [str(compiler), "/nologo", "/c", "/Gy", "/O2", "/MT", "/GX",
               *["/I" + str(p) for p in includes], "/Fo" + str(obj), str(source)]
    proc = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                          text=True, errors="replace", timeout=90)
    record = {"source": str(source), "source_sha256": sha(source),
              "command": command, "cwd": str(cwd), "returncode": proc.returncode,
              "stdout": proc.stdout, "stderr": proc.stderr,
              "object": str(obj), "object_sha256": sha(obj) if obj.is_file() else None}
    if proc.returncode != 0 or not obj.is_file():
        raise RuntimeError(f"VC5 compilation failed: {record}")
    return record


def map_symbols(path: Path) -> dict[str, int]:
    result = {}
    pattern = re.compile(r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})\s+f\s+")
    for line in path.read_text(encoding="ascii", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            result[match.group(1)] = int(match.group(2), 16)
    return result


def link_one(linker: Path, label: str, objects: list[Path], archives: list[Path],
             env: dict[str, str], cwd: Path) -> dict:
    output = cwd / f"{label}.dll"
    map_path = cwd / f"{label}.map"
    log_path = cwd / f"{label}.link.log"
    command = [str(linker), "/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
               "/INCREMENTAL:NO", "/OPT:REF", "/VERBOSE",
               "/MAP:" + str(map_path), "/OUT:" + str(output),
               *[str(x) for x in objects], *[str(x) for x in archives]]
    proc = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                          text=True, errors="replace", timeout=120)
    log = proc.stdout + proc.stderr
    log_path.write_text(log, encoding="utf-8", newline="\n")
    unresolved = [line.strip() for line in log.splitlines()
                  if "unresolved external symbol" in line]
    link_errors = re.findall(r"(?:fatal )?error (LNK\d+)", log)
    allocator_rows = [line.strip() for line in log.splitlines()
                      if any(x in line.lower() for x in
                             ("operator new", "operator delete", "afxmem.obj", "new.obj", "delete.obj"))]
    return {"command": command, "cwd": str(cwd), "returncode": proc.returncode,
            "stdout_sha256": hashlib.sha256(proc.stdout.encode()).hexdigest(),
            "stderr_sha256": hashlib.sha256(proc.stderr.encode()).hexdigest(),
            "log": str(log_path), "log_sha256": sha(log_path),
            "unresolved_diagnostics": unresolved, "link_errors": link_errors,
            "allocator_selection_excerpt": allocator_rows[:120],
            "dll": str(output), "dll_sha256": sha(output) if output.is_file() else None,
            "map": str(map_path), "map_sha256": sha(map_path) if map_path.is_file() else None,
            "forbidden_options_absent": all(x not in " ".join(command).upper()
                                             for x in ("/FORCE", "/FIXED", "/BASE:", "/INCLUDE:"))}


class Machine:
    def __init__(self, label: str, image: Path, function: int,
                 mfc_endpoints: dict[int, str]):
        from pefile import PE
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
        from unicorn.unicorn_const import UC_PROT_ALL
        from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
            UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
            UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
        self.regs = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                     UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
                     UC_X86_REG_EFLAGS, UC_X86_REG_EIP)
        self.eax, self.ecx, self.esp, self.eip = (UC_X86_REG_EAX, UC_X86_REG_ECX,
                                                   UC_X86_REG_ESP, UC_X86_REG_EIP)
        self.label, self.path = label, image
        self.raw = image.read_bytes()
        self.pe = PE(data=self.raw, fast_load=False)
        if self.pe.FILE_HEADER.Machine != 0x14C or self.pe.OPTIONAL_HEADER.Magic != 0x10B:
            raise RuntimeError(f"{label} fixture image is not i386 PE32")
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.function = function
        self.mfc = mfc_endpoints
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.uc.mem_map(self.base, aligned(self.pe.OPTIONAL_HEADER.SizeOfImage), UC_PROT_ALL)
        self.uc.mem_write(self.base, self.pe.get_memory_mapped_image())
        self.uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_ALL)
        self.uc.mem_map(IMPORT_BASE, 0x10000, UC_PROT_ALL)
        self.uc.mem_map(DATA_BASE, 0x10000, UC_PROT_ALL)
        self.uc.mem_write(MODULE_STATE, bytes(0x100))
        self.uc.mem_write(MODULE_STATE + 0xC, struct.pack("<I", RESOURCE_HANDLE))
        self.uc.mem_write(CLASS_NAME, b"HerculesSyntheticClass\0")
        self.imports: dict[int, dict] = {}
        self.imports_by_name: dict[str, list[dict]] = {}
        ordinal = 1
        if not hasattr(self.pe, "DIRECTORY_ENTRY_IMPORT"):
            raise RuntimeError(f"{label} has no import table")
        for descriptor in self.pe.DIRECTORY_ENTRY_IMPORT:
            dll = descriptor.dll.decode("ascii", "replace")
            for item in descriptor.imports:
                name = item.name.decode("ascii", "replace") if item.name else f"ordinal:{item.ordinal}"
                endpoint = IMPORT_BASE + ordinal * 0x10
                row = {"dll": dll, "name": name, "iat": item.address, "endpoint": endpoint}
                self.imports[endpoint] = row
                self.imports_by_name.setdefault(name, []).append(row)
                self.uc.mem_write(item.address, struct.pack("<I", endpoint))
                ordinal += 1
        self.events: list[dict] = []
        self.api_calls: list[dict] = []
        self.unapproved_imports: list[dict] = []
        self.outside_writes: list[dict] = []
        self.current_case: dict | None = None
        self.allowed_code = [(function, function + 272)] + [(a, a + 1) for a in mfc_endpoints]
        self.uc.hook_add(UC_HOOK_CODE, self._code_hook)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self._write_hook)

    def read(self, address: int, size: int) -> bytes:
        return bytes(self.uc.mem_read(address, size))

    def _args(self, esp: int, count: int) -> list[int]:
        if not count:
            return []
        return list(struct.unpack("<" + "I" * count, self.uc.mem_read(esp + 4, count * 4)))

    def _ret_stdcall(self, esp: int, result: int, pop_bytes: int) -> None:
        return_address = struct.unpack("<I", self.uc.mem_read(esp, 4))[0]
        self.uc.reg_write(self.eax, result & 0xFFFFFFFF)
        self.uc.reg_write(self.esp, esp + 4 + pop_bytes)
        self.uc.reg_write(self.eip, return_address)

    def _mfc_hook(self, address: int) -> None:
        label = self.mfc[address]
        esp = self.uc.reg_read(self.esp)
        if label == "AfxGetModuleState":
            self.events.append({"kind": "mfc", "name": label, "return": hex(MODULE_STATE)})
            self._ret_stdcall(esp, MODULE_STATE, 0)
        elif label == "AfxRegisterWndClass":
            args = self._args(esp, 4)
            self.events.append({"kind": "mfc", "name": label,
                                "style": hex(args[0]), "cursor": hex(args[1]),
                                "brush": hex(args[2]), "icon": hex(args[3]),
                                "return": hex(CLASS_NAME)})
            self._ret_stdcall(esp, CLASS_NAME, 16)
        elif label == "CFrameWnd::PreCreateWindow":
            receiver = self.uc.reg_read(self.ecx)
            create = struct.unpack("<I", self.uc.mem_read(esp + 4, 4))[0]
            self.events.append({"kind": "mfc", "name": label,
                                "receiver": hex(receiver), "create": hex(create),
                                "input_hex": self.read(create, CREATE_SIZE).hex(),
                                "return": self.current_case["base_result"]})
            self._ret_stdcall(esp, self.current_case["base_result"], 4)
        else:
            raise RuntimeError(f"unexpected synthetic MFC entry: {label}")

    def _api_hook(self, address: int) -> None:
        item = self.imports[address]
        if item["dll"].lower() != "user32.dll" or item["name"] not in API_ARGUMENT_BYTES:
            self.unapproved_imports.append({"dll": item["dll"], "name": item["name"],
                                            "iat": hex(item["iat"])})
            raise RuntimeError(f"fail-on-entry unapproved import {item['dll']}!{item['name']}")
        name = item["name"]
        esp = self.uc.reg_read(self.esp)
        args = self._args(esp, API_ARGUMENT_BYTES[name] // 4)
        result = 0
        event: dict = {"kind": "api", "dll": item["dll"], "name": name}
        if name == "GetSystemMetrics":
            index = args[0]
            if index not in self.current_case["metrics"]:
                raise RuntimeError(f"unexpected system metric {index}")
            result = self.current_case["metrics"][index]
            event.update({"index": index, "return": result})
        elif name == "SystemParametersInfoA":
            action, ui_param, output, flags = args
            if action != 0x30 or ui_param != 0 or flags != 0:
                raise RuntimeError(f"unexpected SPI_GETWORKAREA arguments {args!r}")
            work = self.current_case["work"]
            before = self.read(output, 16)
            self.uc.mem_write(output, struct.pack("<4i", *work))
            result = self.current_case["spi_success"]
            event.update({"action": action, "ui_param": ui_param, "output": hex(output),
                          "flags": flags, "before_hex": before.hex(),
                          "written_rect": list(work), "return": result})
        elif name == "LoadCursorA":
            instance, cursor_id = args
            if instance != 0 or cursor_id != 0x7F00:
                raise RuntimeError(f"unexpected LoadCursorA arguments {args!r}")
            result = CURSOR_HANDLE
            event.update({"instance": hex(instance), "cursor_id": hex(cursor_id),
                          "return": hex(result)})
        elif name == "LoadIconA":
            instance, icon_id = args
            if instance != RESOURCE_HANDLE or icon_id != 0x80:
                raise RuntimeError(f"unexpected LoadIconA arguments {args!r}")
            result = ICON_HANDLE
            event.update({"instance": hex(instance), "icon_id": hex(icon_id),
                          "return": hex(result)})
        self.events.append(event)
        self.api_calls.append(event)
        self._ret_stdcall(esp, result, API_ARGUMENT_BYTES[name])

    def _code_hook(self, uc, address: int, size: int, _):
        if address in self.mfc:
            self._mfc_hook(address)
            return
        if address in self.imports:
            self._api_hook(address)
            return
        if not any(start <= address < end for start, end in self.allowed_code):
            raise RuntimeError(f"fail-on-entry unapproved code in {self.label}: {address:#x}")

    def _write_hook(self, uc, access, address: int, size: int, value: int, _):
        in_stack = STACK_BASE <= address and address + size <= STACK_BASE + STACK_SIZE
        in_create = CREATE - 16 <= address and address + size <= CREATE + CREATE_SIZE + 16
        in_receiver = RECEIVER - 16 <= address and address + size <= RECEIVER + RECEIVER_SIZE + 16
        if not (in_stack or in_create or in_receiver):
            self.outside_writes.append({"address": hex(address), "size": size,
                                        "value": hex(value & ((1 << (8 * min(size, 8))) - 1))})

    def run_case(self, case: dict) -> dict:
        self.current_case = case
        self.events, self.api_calls, self.unapproved_imports, self.outside_writes = [], [], [], []
        receiver_seed = bytes((0xD3 + i * 13) & 0xFF for i in range(RECEIVER_SIZE))
        receiver_guard_before = bytes.fromhex("a17e5bc409d263e8f15aa6cc30849b72")
        receiver_guard_after = bytes.fromhex("72b98430cca65af1e863d209c45b7ea1")
        create_guard_before = bytes.fromhex("112233445566778899aabbccddeeff00")
        create_guard_after = bytes.fromhex("00ffeeddccbbaa998877665544332211")
        self.uc.mem_write(RECEIVER - 16, receiver_guard_before + receiver_seed + receiver_guard_after)
        create_seed = struct.pack("<12I", 0x11111111, 0x22222222, 0x33333333,
            0x44444444, 0x55555555, 0x66666666, 0x77777777, 0x88888888,
            0x50050800 | 0x00010000 | 0x00040000, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB)
        self.uc.mem_write(CREATE - 16, create_guard_before + create_seed + create_guard_after)
        receiver_before = self.read(RECEIVER, RECEIVER_SIZE)
        create_before = self.read(CREATE, CREATE_SIZE)
        pre_receiver = self.read(RECEIVER - 16, 16)
        post_receiver = self.read(RECEIVER + RECEIVER_SIZE, 16)
        pre_create = self.read(CREATE - 16, 16)
        post_create = self.read(CREATE + CREATE_SIZE, 16)
        stack = STACK_TOP - 16
        self.uc.mem_write(stack, struct.pack("<4I", SENTINEL, CREATE, 0, 0))
        values = [0x11111111, 0x22222222, RECEIVER, 0x44444444,
                  0x55555555, 0x66666666, 0x77777777, stack, 0x202, self.function]
        for reg, value in zip(self.regs, values):
            self.uc.reg_write(reg, value)
        self.uc.emu_start(self.function, SENTINEL, count=100000)
        end_pc = self.uc.reg_read(self.eip)
        end_sp = self.uc.reg_read(self.esp)
        end_eax = self.uc.reg_read(self.eax)
        receiver_after = self.read(RECEIVER, RECEIVER_SIZE)
        create_after = self.read(CREATE, CREATE_SIZE)
        result = {
            "events": self.events.copy(), "api_calls": self.api_calls.copy(),
            "create_before_hex": create_before.hex(), "create_after_hex": create_after.hex(),
            "create_changed_offsets": [i for i, (a, b) in enumerate(zip(create_before, create_after)) if a != b],
            "receiver_before_sha256": hashlib.sha256(receiver_before).hexdigest(),
            "receiver_after_sha256": hashlib.sha256(receiver_after).hexdigest(),
            "receiver_unchanged": receiver_before == receiver_after,
            "receiver_guards_unchanged": (self.read(RECEIVER - 16, 16) == pre_receiver and
                                           self.read(RECEIVER + RECEIVER_SIZE, 16) == post_receiver),
            "create_guards_unchanged": (self.read(CREATE - 16, 16) == pre_create and
                                         self.read(CREATE + CREATE_SIZE, 16) == post_create),
            "outside_region_writes": self.outside_writes.copy(),
            "unapproved_imports_entered": self.unapproved_imports.copy(),
            "eax": end_eax, "esp_delta": end_sp - stack,
            "returned_to_sentinel": end_pc == SENTINEL, "eip": hex(end_pc),
        }
        result["abi_contract_ok"] = (result["returned_to_sentinel"] and result["esp_delta"] == 8 and
            result["receiver_unchanged"] and result["receiver_guards_unchanged"] and
            result["create_guards_unchanged"] and not result["outside_region_writes"] and
            not result["unapproved_imports_entered"])
        if not result["abi_contract_ok"]:
            raise RuntimeError(f"{self.label}/{case['name']} ABI/canary contract failed: {result}")
        return result


def compare_object(root: Path, obj: Path, oracle: Path) -> list[dict]:
    sys.path.insert(0, str(root / "scripts"))
    from coff import COFF
    from masked import masked_compare
    from pe import PE
    evidence = json.loads((root / "evidence" / "host_frame_class_recovery.json").read_text(encoding="utf-8"))
    coff = COFF(obj)
    pe = PE(oracle)
    rows = []
    for f in evidence["functions"]:
        symbol, rva, size = f["symbol"], int(f["rva"], 16), f["complete_size"]
        section = coff.function(symbol)
        if section["size"] != size:
            raise RuntimeError(f"fresh {symbol} contribution extent {section['size']} != measured {size}")
        target = pe.read_rva(rva, size)
        raw = section["data"] == target
        diagnostic = masked_compare(obj, symbol, oracle, rva, size)
        if not diagnostic["masked_equal"] or diagnostic["conflicts"] or diagnostic["unsupported"]:
            raise RuntimeError(f"fresh {symbol} failed the relocation diagnostic: {diagnostic}")
        rows.append({"symbol": symbol, "rva": hex(rva), "size": size,
                     "candidate_sha256": hashlib.sha256(section["data"]).hexdigest(),
                     "target_sha256": hashlib.sha256(target).hexdigest(),
                     "raw_exact": raw,
                     "raw_differing_bytes": sum(a != b for a, b in zip(section["data"], target)),
                     "coff_relocations": section["relocations"],
                     "relocation_diagnostic_equal": diagnostic["masked_equal"],
                     "relocation_diagnostic_differences": diagnostic["differing_bytes"],
                     "diagnostic_conflicts": diagnostic["conflicts"],
                     "unsupported_relocations": diagnostic["unsupported"]})
    precreate = next(row for row in rows if row["symbol"] == FUNCTION_SYMBOL)
    if precreate["raw_exact"]:
        raise RuntimeError("unexpected raw-equality result changed; review the proof contract")
    return rows


def semantic_differential(oracle: Path, candidate_dll: Path, candidate_map: Path,
                          link_report: dict) -> dict:
    from pefile import PE
    native_pe = PE(str(oracle), fast_load=False)
    candidate_pe = PE(str(candidate_dll), fast_load=False)
    symbols = map_symbols(candidate_map)
    needed = [FUNCTION_SYMBOL,
              "?AfxGetModuleState@@YGPAVAFX_MODULE_STATE@@XZ",
              "?AfxRegisterWndClass@@YGPBDIPAUHICON__@@PAUHBRUSH__@@0@Z",
              "?PreCreateWindow@CFrameWnd@@MAEHAAUtagCREATESTRUCTA@@@Z"]
    missing = [s for s in needed if s not in symbols]
    if missing:
        raise RuntimeError(f"bounded linked map is missing required function addresses: {missing}")
    candidate_mfc = {
        symbols["?AfxGetModuleState@@YGPAVAFX_MODULE_STATE@@XZ"]: "AfxGetModuleState",
        symbols["?AfxRegisterWndClass@@YGPBDIPAUHICON__@@PAUHBRUSH__@@0@Z"]: "AfxRegisterWndClass",
        symbols["?PreCreateWindow@CFrameWnd@@MAEHAAUtagCREATESTRUCTA@@@Z"]: "CFrameWnd::PreCreateWindow",
    }
    candidate_function = symbols[FUNCTION_SYMBOL]
    if not native_pe.OPTIONAL_HEADER.ImageBase == NATIVE_BASE:
        raise RuntimeError("retail PE image base changed")
    if candidate_function - candidate_pe.OPTIONAL_HEADER.ImageBase > candidate_pe.OPTIONAL_HEADER.SizeOfImage:
        raise RuntimeError("candidate function address lies outside its linked image")
    native = Machine("pinned-retail", oracle, NATIVE_FUNCTION, NATIVE_MFC)
    candidate = Machine("fresh-source-fixture", candidate_dll, candidate_function, candidate_mfc)
    rows = []
    for case in CASES:
        original_result = native.run_case(case)
        candidate_result = candidate.run_case(case)
        equal = original_result == candidate_result
        rows.append({"case": case, "retail": original_result,
                     "candidate": candidate_result, "semantic_equal": equal})
    if not all(x["semantic_equal"] for x in rows):
        raise RuntimeError("at least one finite retail/source semantic case differs")
    return {
        "case_count": len(rows), "all_cases_equal": True,
        "addresses": {"native_function": hex(NATIVE_FUNCTION),
                      "candidate_function": hex(candidate_function),
                      "candidate_image_base": hex(candidate.base),
                      "native_mfc_endpoints": {hex(k): v for k, v in NATIVE_MFC.items()},
                      "candidate_mfc_endpoints": {hex(k): v for k, v in candidate_mfc.items()}},
        "import_boundary": {"approved": ["USER32!GetSystemMetrics", "USER32!SystemParametersInfoA",
                                             "USER32!LoadCursorA", "USER32!LoadIconA"],
                            "every other import fails on entry": True,
                            "candidate_import_count": len(candidate.imports),
                            "retail_import_count": len(native.imports),
                            "real_api_entries": 0},
        "code_boundary": {"allowed": ["PreCreateWindow", *NATIVE_MFC.values()],
                           "all_other_code_fail_on_entry": "including fixture main and GetMessageMap",
                           "instruction_cap": 100000},
        "mfc_boundaries": {"AfxGetModuleState": "synthetic stable pointer with +0xc resource handle",
                           "AfxRegisterWndClass": "synthetic class-name pointer; records arguments",
                           "CFrameWnd::PreCreateWindow": "synthetic unchanged-structure pass-through with per-case BOOL"},
        "link_and_runtime_limits": {
            "natural_no_map_unresolved": link_report["natural_no_map"]["unresolved_diagnostics"],
            "fixture_only_map_definition": "const AFX_MSGMAP CMainFrame::messageMap = { 0, 0 }; never entered/read",
            "fixture_only_main": "inert infinite loop used only to satisfy the linked CRT startup reference; fail-on-entry and never called",
            "allocator": "MFC-before-CRT archive order selects nafxcw.lib(afxmem.obj); factory and allocators are excluded",
            "no_windows_loader_or_process": True},
        "finite_limits": [
            "SPI_GETWORKAREA failure without output writes is excluded because retail then reads an uninitialized RECT.",
            "False-SPI case supplies a complete synthetic RECT before FALSE; it only tests ignored return within that bound.",
            "CFrameWnd::PreCreateWindow is intercepted identically in both images; this checks the override/base seam only.",
            "The 188-byte receiver is synthetic canary storage and is never constructed as a CMainFrame.",
            "Synthetic metric values, work area, handles, and class name do not represent actual host settings or resources.",
        ],
        "cases": rows,
    }


def run(root: Path, original_root: Path, output: Path) -> dict:
    root = root.resolve()
    original_root = original_root.resolve()
    output = output.resolve()
    if not output.is_relative_to((root / "work").resolve()):
        raise ValueError("--output must be within the repository ignored work/ directory")
    output.mkdir(parents=True, exist_ok=True)
    build = output / "build"
    build.mkdir(parents=True, exist_ok=True)
    source = root / 'src/pc/host_window_class.cpp'
    header = root / 'src/pc/host_system_keys.h'
    oracle = original_root / "work" / "discs" / "pc_install" / "HERCULES.EXE"
    evidence = root / "evidence" / "host_frame_class_recovery.json"
    manifest = root / "toolchains" / "manifest.json"
    runner_path = Path(__file__).resolve()
    helper_inputs = [root / "scripts" / name for name in ("match.py", "coff.py", "masked.py", "pe.py")]
    compiler, linker = TOOLCHAIN / "bin" / "cl.exe", TOOLCHAIN / "bin" / "link.exe"
    required = (source, header, oracle, evidence, manifest, compiler, linker, runner_path, *helper_inputs)
    missing = [str(p) for p in required if not p.is_file()]
    if missing:
        raise FileNotFoundError("required canonical/pinned input missing: " + ", ".join(missing))
    oracle_hash_before = sha(oracle)
    if oracle_hash_before != ORACLE_SHA256:
        raise RuntimeError(f"pinned retail oracle hash changed: {sha(oracle)}")
    if sha(compiler) != CL_SHA256 or sha(linker) != LINK_SHA256:
        raise RuntimeError("pinned VC5 compiler/linker hash mismatch")
    sys.path.insert(0, str(root / "scripts"))
    from match import verify_toolchain
    if verify_toolchain(TOOLCHAIN) != "msvc5_rtm":
        raise RuntimeError("toolchain identity check did not identify msvc5_rtm")
    pins = source_pins(root)
    actual_pins = {str(source.resolve()): sha(source), str(header.resolve()): sha(header)}
    if actual_pins != pins:
        raise RuntimeError(f"canonical source/header do not match durable pins: {actual_pins} != {pins}")

    driver = build / "fixture_driver.cpp"
    map_boundary = build / "fixture_message_map_boundary.cpp"
    entry_guard = build / "fixture_entry_guard.cpp"
    driver.write_text(
        '#include "host_system_keys.h"\n'
        'class FixtureAccess : public CMainFrame {\n'
        'public:\n'
        '  BOOL CallPreCreateWindow(CREATESTRUCT& cs) { return CMainFrame::PreCreateWindow(cs); }\n'
        '};\n'
        'extern "C" __declspec(dllexport) BOOL __cdecl FixtureRun(CMainFrame* receiver, CREATESTRUCT* cs) {\n'
        '  return ((FixtureAccess*)receiver)->CallPreCreateWindow(*cs);\n'
        '}\n', encoding="utf-8", newline="\n")
    map_boundary.write_text(
        '#include "host_system_keys.h"\n'
        'const AFX_MSGMAP CMainFrame::messageMap = { 0, 0 };\n',
        encoding="utf-8", newline="\n")
    entry_guard.write_text(
        'int main(void) { for (;;) { } return 0; }\n',
        encoding="utf-8", newline="\n")
    sources = [source, driver, map_boundary, entry_guard]
    include_dirs = [TOOLCHAIN / "include", TOOLCHAIN / "mfc" / "include", root / "src/pc"]
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(p) for p in include_dirs)
    env["LIB"] = ";".join([str(TOOLCHAIN / "lib"), str(TOOLCHAIN / "mfc" / "lib")])
    dep_rows, all_headers = [], set()
    for src in sources:
        deps, row = preprocessor_dependencies(compiler, src, include_dirs, env, build)
        dep_rows.append(row)
        all_headers.update(deps)
    if header.resolve() not in all_headers:
        raise RuntimeError("preprocessor dependency scan did not include the canonical class header")
    source_hashes_before = {str(p.resolve()): sha(p) for p in sources}
    header_hashes_before = {str(p.resolve()): sha(p) for p in sorted(all_headers, key=lambda p: str(p).lower())}
    if {str(p.resolve()): sha(p) for p in (source, header)} != pins:
        raise RuntimeError("canonical inputs changed during the dependency scan")
    library_names = ["nafxcw.lib", "libcmt.lib", "kernel32.lib", "user32.lib", "gdi32.lib",
                     "comdlg32.lib", "winspool.lib", "advapi32.lib", "shell32.lib",
                     "comctl32.lib", "uuid.lib", "oldnames.lib"]
    archives = [TOOLCHAIN / "mfc" / "lib" / library_names[0],
                TOOLCHAIN / "lib" / library_names[1]]
    archives += [TOOLCHAIN / "lib" / name for name in library_names[2:]]
    tool_inputs = [compiler, linker, manifest, evidence, runner_path, *helper_inputs, *archives]
    tool_hashes_before = {str(p.resolve()): sha(p) for p in tool_inputs}
    source_objects = []
    compiles = []
    for src in sources:
        obj = build / (src.stem + ".obj")
        compiles.append(compile_one(compiler, src, obj, include_dirs, env, build))
        source_objects.append(obj)
    source_hashes_after = {str(p.resolve()): sha(p) for p in sources}
    header_hashes_after = {str(p.resolve()): sha(p) for p in sorted(all_headers, key=lambda p: str(p).lower())}
    tool_hashes_after = {str(p.resolve()): sha(p) for p in tool_inputs}
    if source_hashes_before != source_hashes_after or header_hashes_before != header_hashes_after:
        raise RuntimeError("a source or compiler-included header changed during the fresh build")
    if tool_hashes_before != tool_hashes_after:
        raise RuntimeError("a pinned compiler/library/oracle-evidence input changed during the build")

    class_obj = source_objects[0]
    code_comparisons = compare_object(root, class_obj, oracle)
    natural = link_one(linker, "natural_no_message_map", source_objects[:2] + [source_objects[3]],
                       archives, env, build)
    if natural["dll_sha256"] is not None:
        raise RuntimeError("natural no-map link unexpectedly supplied missing game data")
    bounded = link_one(linker, "bounded_with_unentered_message_map", source_objects,
                       archives, env, build)
    if bounded["returncode"] != 0 or bounded["dll_sha256"] is None:
        raise RuntimeError(f"bounded no-force source fixture did not link: {bounded}")
    expected_natural = [x for x in natural["unresolved_diagnostics"]
                        if "CMainFrame::messageMap" in x or "?messageMap@CMainFrame" in x]
    if len(expected_natural) != 1 or len(natural["unresolved_diagnostics"]) != 1:
        raise RuntimeError(f"natural link unresolved set changed: {natural['unresolved_diagnostics']}")
    if bounded["unresolved_diagnostics"]:
        raise RuntimeError(f"bounded link retains unresolved symbols: {bounded['unresolved_diagnostics']}")
    semantic = semantic_differential(oracle, Path(bounded["dll"]), Path(bounded["map"]),
                                     {"natural_no_map": natural})
    # Include the oracle in the final unchanged snapshot; it is read only.
    after_all = {str(p.resolve()): sha(p) for p in sources + list(all_headers) + tool_inputs + [oracle]}
    before_all = {**source_hashes_before, **header_hashes_before, **tool_hashes_before,
                  str(oracle.resolve()): oracle_hash_before}
    if before_all != after_all:
        raise RuntimeError("one or more source, SDK, pinned library, tool, or oracle inputs changed")
    report = {
        "scope": "Fresh canonical CMainFrame class TU contribution and finite direct-call semantic diagnostic. No recovery status or whole-image equality is asserted.",
        "oracle": {"path": str(oracle), "sha256": sha(oracle)},
        "durable_source_pin": {"path": str(evidence), "sha256": sha(evidence), "expected_sources": pins},
        "runner": {"path": str(runner_path), "sha256": sha(runner_path)},
        "canonical_sources": {str(p.resolve()): sha(p) for p in (source, header)},
        "toolchain": {"id": "msvc5_rtm", "manifest": str(manifest), "manifest_sha256": sha(manifest),
                      "compiler": str(compiler), "compiler_sha256": sha(compiler),
                      "linker": str(linker), "linker_sha256": sha(linker),
                      "flags": ["/Gy", "/O2", "/MT", "/GX"]},
        "dependency_scan": dep_rows,
        "include_hashes": header_hashes_before,
        "input_hashes_before_after": {"before": before_all, "after": after_all, "unchanged": True},
        "generated_fixture_sources": [{"path": str(p), "sha256": sha(p)} for p in (driver, map_boundary, entry_guard)],
        "fresh_compiles": compiles,
        "fresh_class_object": {"path": str(class_obj), "sha256": sha(class_obj)},
        "function_contribution_comparisons": code_comparisons,
        "raw_function_exact": all(x["raw_exact"] for x in code_comparisons),
        "link": {"archive_order": [str(p) for p in archives],
                 "archive_hashes": {str(p): sha(p) for p in archives},
                 "natural_no_map": natural, "bounded_fixture": bounded,
                 "message_map_boundary": "fixture-only zero AFX_MSGMAP data to permit linkage; it is never read and GetMessageMap is fail-on-entry",
                 "entry_guard": "fixture-only inert main; Unicorn fails on entry, and instruction cap is a backstop",
                 "allocator_boundary": "MFC-before-CRT required to avoid duplicate global new/delete; this method fixture never enters factory or allocator",
                 "no_force_alias_fixed_or_base_controls": True},
        "semantic_differential": semantic,
        "strict_outcome": "FUNCTION_MATCH_NOT_CLAIMED; semantic equality and relocation diagnostics do not satisfy raw function equality",
    }
    out = output / "host_frame_precreate_differential.json"
    out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    report["report_path"] = str(out)
    report["report_sha256"] = sha(out)
    summary = {"report": str(out), "oracle_sha256": report["oracle"]["sha256"],
               "fresh_object_sha256": report["fresh_class_object"]["sha256"],
               "precreate_raw_exact": next(x["raw_exact"] for x in code_comparisons if x["symbol"] == FUNCTION_SYMBOL),
               "class_functions_diagnostic_equal": all(x["relocation_diagnostic_equal"] for x in code_comparisons),
               "natural_link_missing_only_message_map": len(natural["unresolved_diagnostics"]) == 1,
               "bounded_link_returncode": bounded["returncode"],
               "cases": semantic["case_count"], "all_cases_equal": semantic["all_cases_equal"],
               "real_api_entries": semantic["import_boundary"]["real_api_entries"],
               "all_inputs_unchanged": report["input_hashes_before_after"]["unchanged"]}
    print(json.dumps(summary, indent=2))
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        help="repository holding canonical src/ sources; derived from this script by default")
    parser.add_argument("--original-root", type=Path, default=Path(r"D:\Prog\hercules_recon"),
                        help="read-only root containing work/discs/pc_install/HERCULES.EXE")
    parser.add_argument("--output", type=Path,
                        help="diagnostic output under work/; default work/host_frame_precreate_public")
    args = parser.parse_args()
    root = args.source_root.resolve() if args.source_root else repository_root(Path(__file__).resolve())
    output = args.output.resolve() if args.output else root / "work" / "host_frame_precreate_public"
    run(root, args.original_root, output)


if __name__ == "__main__":
    main()
