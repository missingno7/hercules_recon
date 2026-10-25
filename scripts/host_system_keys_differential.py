"""Fresh, bounded verification runner for the reconstructed SYSKEY handlers.

This runner is intended to be copied unchanged to scripts/. It compiles the
canonical source and its measured storage owners, links one inert fixture image,
compares the same handler objects with the pinned retail contributions, and
executes only direct synthetic member calls in Unicorn. No process entry point,
message loop, real API, or game code is started.
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

for _dependency_path in (Path(r"C:\tools\hercules\renderer-oracle-python"),
                         Path(r"C:\tools\hercules\python")):
    if str(_dependency_path) not in sys.path:
        sys.path.insert(0, str(_dependency_path))


ORACLE_DEFAULT = Path(r"D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE")
ORACLE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
BASE = 0x400000
STACK_BASE = 0xC00000
STACK_SIZE = 0x20000
STACK_TOP = STACK_BASE + STACK_SIZE - 0x100
SENTINEL = STACK_BASE + 0x1FFF0
IMPORT_BASE = 0xD00000
MFC_MEMBER_INDEX = 148
MFC_SYMBOLS = {
    "default": "?Default@CWnd@@IAEJXZ",
    "on_wnd_msg": "?OnWndMsg@CWnd@@MAEHIIJPAJ@Z",
}

# First-party handler bytes are bounded by the primary function map.
HANDLERS = {
    "down": {"symbol": "?OnSysKeyDown@CMainFrame@@QAEXIII@Z",
             "rva": 0x9AB0, "size": 112},
    "up": {"symbol": "?OnSysKeyUp@CMainFrame@@QAEXIII@Z",
           "rva": 0x9B20, "size": 32},
}

ORIGINAL_CELLS = {
    "input_state": (0x96E618, 1),
    "input_gate": (0x92C488, 1),
    "threshold": (0xBA0238, 1),
    "mode_busy": (0xBA0228, 1),
    "word_busy": (0xBA0230, 4),
    "pending": (0xBA0234, 4),
    "fatal_suppress": (0xBA020C, 1),
    "display_mode": (0x96E3D0, 1),
}
CELL_WIDTHS = {name: width for name, (_, width) in ORIGINAL_CELLS.items()}
FIVE_PREFIXES = {
    "input_state": ("_g_exe_96e618", "BYTE", 0x96E618, 1),
    "input_gate": ("_g_exe_92c488", "BYTE", 0x92C488, 1),
    "threshold": ("_g_exe_ba0238", "signed char", 0xBA0238, 1),
    "mode_busy": ("_g_exe_ba0228", "BYTE", 0xBA0228, 1),
    "word_busy": ("_g_exe_ba0230", "DWORD", 0xBA0230, 4),
}

STACK_REGS = (
    "UC_X86_REG_EAX", "UC_X86_REG_EBX", "UC_X86_REG_ECX", "UC_X86_REG_EDX",
    "UC_X86_REG_ESI", "UC_X86_REG_EDI", "UC_X86_REG_EBP", "UC_X86_REG_ESP",
    "UC_X86_REG_EFLAGS", "UC_X86_REG_EIP",
)
CASES = [
    {"name": "down_active_precedes_all", "handler": "down", "initial": {"input_state": 1, "fatal_suppress": 1, "pending": 0x12345678}, "key": 13},
    {"name": "down_fatal_override", "handler": "down", "initial": {"input_gate": 0, "threshold": 0, "fatal_suppress": 1, "display_mode": 0xA5}, "key": 0x41},
    {"name": "down_gate_zero", "handler": "down", "initial": {"threshold": 2}, "key": 13},
    {"name": "down_wrong_key", "handler": "down", "initial": {"input_gate": 1, "threshold": 2}, "key": 0x41},
    {"name": "down_threshold_one", "handler": "down", "initial": {"input_gate": 1, "threshold": 1}, "key": 13},
    {"name": "down_threshold_negative_signed", "handler": "down", "initial": {"input_gate": 1, "threshold": 0xFE}, "key": 13},
    {"name": "down_mode_busy", "handler": "down", "initial": {"input_gate": 1, "threshold": 2, "mode_busy": 1}, "key": 13},
    {"name": "down_word_busy", "handler": "down", "initial": {"input_gate": 1, "threshold": 2, "word_busy": 0x100}, "key": 13},
    {"name": "down_accepted_threshold_two", "handler": "down", "initial": {"input_gate": 1, "threshold": 2, "display_mode": 0xA5}, "key": 13},
    {"name": "down_accepted_ignored_args", "handler": "down", "initial": {"input_gate": 1, "threshold": 127}, "key": 13, "repeat": 0xFFFFFFFF, "flags": 0x87654321},
    {"name": "down_active_repeat", "handler": "down", "initial": {"input_state": 1}, "key": 13},
    {"name": "up_clears_input_state", "handler": "up", "initial": {"input_state": 1, "pending": 5}, "key": 13},
    {"name": "up_idle_defaults", "handler": "up", "initial": {"pending": 5}, "key": 13},
    {"name": "up_ignores_message_arguments", "handler": "up", "initial": {"input_state": 1}, "key": 0x41, "repeat": 0xFFFFFFFF, "flags": 0xFFFFFFFF},
]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def repository_root(start: Path) -> Path:
    for candidate in (start, *start.parents):
        if (candidate / "scripts").is_dir() and (candidate / "toolchains" / "manifest.json").is_file():
            return candidate
    raise RuntimeError("cannot derive repository root from runner/script installation location")


def run(command: list[str], cwd: Path, env: dict[str, str]) -> dict:
    proc = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                          text=True, errors="replace")
    return {"command": command, "cwd": str(cwd), "returncode": proc.returncode,
            "stdout": proc.stdout, "stderr": proc.stderr}


def parse_map(path: Path) -> dict[str, int]:
    result = {}
    for line in path.read_text(encoding="ascii", errors="replace").splitlines():
        m = re.match(r"^\s*[0-9A-Fa-f]{4}:([0-9A-Fa-f]{8})\s+(\S+)\s+([0-9A-Fa-f]{8})", line)
        if m:
            result[m.group(2)] = int(m.group(3), 16)
    return result


def source_inventory(root: Path, toolchain: Path) -> tuple[list[Path], list[Path]]:
    source_dir = root / "src/pc"
    sources = [
        source_dir / "host_system_keys.cpp",
        source_dir / "host_system_key_storage.c",
        source_dir / "display_mode_setter.c",
        source_dir / "graphics_storage.cpp",
        source_dir / "fatal_storage.c",
        source_dir / "host_startup_storage.c",
    ]
    headers = [
        source_dir / "host_system_keys.h",
        source_dir / "host_startup_storage.h",
        toolchain / "mfc" / "include" / "afxwin.h",
        toolchain / "mfc" / "include" / "afxmsg_.h",
        toolchain / "include" / "windows.h",
        toolchain / "include" / "mmsystem.h",
        toolchain / "include" / "ddraw.h",
    ]
    missing = [str(p) for p in sources + headers if not p.is_file()]
    if missing:
        raise FileNotFoundError("canonical runner inputs are not present yet: " + ", ".join(missing))
    return sources, headers


def declaration_check(storage: Path) -> list[dict]:
    text = storage.read_text(encoding="utf-8")
    rows = []
    for name, (symbol, ctype, _va, width) in FIVE_PREFIXES.items():
        spelling = symbol[1:]
        pattern = rf"(?m)^\s*{re.escape(ctype)}\s+{re.escape(spelling)}\s*(?:=\s*0)?\s*;\s*(?:/\*.*?\*/\s*)?$"
        if not re.search(pattern, text):
            raise RuntimeError(f"storage declaration is not a measured zero-initialized {ctype} {spelling};")
        rows.append({"name": spelling, "type": ctype, "width": width,
                     "definition": "external C object with measured effective zero initial bytes; initializer spelling is not evidence"})
    return rows


def verify_coff_bss(storage_obj: Path, storage_source: Path) -> list[dict]:
    from coff import COFF
    coff = COFF(storage_obj)
    found = []
    for _, (symbol, ctype, _va, width) in FIVE_PREFIXES.items():
        record = [s for s in coff.symbols.values() if s["name"] == symbol]
        if len(record) != 1 or record[0]["storage"] != 2:
            raise RuntimeError(f"expected one externally-defined COFF symbol {symbol}")
        s = record[0]
        if s["section"] == 0:
            if s["value"] != width:
                raise RuntimeError(f"COMMON symbol {symbol} has unexpected extent {s['value']}")
            storage = "COMMON"
            offset = None
        else:
            section = coff.sections[s["section"] - 1]
            if not section["flags"] & 0x80 or section["flags"] & 0x40:
                raise RuntimeError(f"{symbol} is not in an uninitialized-only section")
            if section["name"] != ".bss" and not section["name"].startswith(".bss$"):
                raise RuntimeError(f"{symbol} is in unexpected zero-fill section {section['name']}")
            if not section["flags"] & 0x80000000 or section["data"]:
                raise RuntimeError(f"{symbol} is not in writable rawless BSS")
            if s["value"] + width > section["size"]:
                raise RuntimeError(f"{symbol} extends beyond its COFF BSS section")
            storage = section["name"]
            offset = s["value"]
        found.append({"symbol": symbol, "type": ctype, "width": width,
                      "storage": storage, "section_offset": offset})
    return found


def mfc_member_provenance(toolchain: Path, original_root: Path) -> dict:
    sys.path.insert(0, str(original_root / "scripts"))
    from crt_copy_evidence import members
    from coff import COFF
    archive = toolchain / "mfc" / "lib" / "nafxcw.lib"
    payloads = list(members(archive.read_bytes()))
    if len(payloads) <= MFC_MEMBER_INDEX:
        raise RuntimeError("pinned nafxcw archive does not contain mapped member 148")
    payload = payloads[MFC_MEMBER_INDEX]
    member_path = original_root / "work" / "host_window_input_recovery" / "mfc_member_148.obj"
    member_path.parent.mkdir(parents=True, exist_ok=True)
    member_path.write_bytes(payload)
    coff = COFF(member_path)
    names = sorted({s["name"] for s in coff.symbols.values() if s["storage"] in (2, 3)})
    for symbol in MFC_SYMBOLS.values():
        if symbol not in names:
            raise RuntimeError(f"member 148 does not contain expected MFC symbol {symbol}")
    library_map = Path(r"D:\Prog\hercules_recon\work\library_map\library_map.json")
    data = json.loads(library_map.read_text(encoding="utf-8"))
    rows = data["contributions"]["HERCULES.EXE"]
    witnesses = {}
    for label, symbol in MFC_SYMBOLS.items():
        matches = [r for r in rows if r["library"] == "mfc/lib/nafxcw.lib" and
                   r["member"] == MFC_MEMBER_INDEX and symbol in r["names"]]
        if len(matches) != 1:
            raise RuntimeError(f"library map does not uniquely identify {symbol} in member 148")
        witnesses[label] = matches[0]
    return {
        "library": str(archive), "library_sha256": sha(archive),
        "member_index": MFC_MEMBER_INDEX, "member_sha256": hashlib.sha256(payload).hexdigest(),
        "member_size": len(payload), "member_symbols": [s for s in names if s in MFC_SYMBOLS.values()],
        "library_map": str(library_map), "library_map_sha256": sha(library_map),
        "mapped_original_contributions": witnesses,
        "boundary": "Pinned MFC CWnd::Default is linked as the real static-library contribution, then intercepted at entry in the bounded fixture; its implementation and any window dispatch beyond that boundary are outside the semantic cases.",
    }


def compile_link(root: Path, output: Path, oracle: Path) -> dict:
    sys.path.insert(0, str(root / "scripts"))
    from match import verify_toolchain
    from coff import COFF
    from masked import masked_compare
    import match
    import pefile

    if sha(oracle) != ORACLE_SHA256:
        raise RuntimeError(f"pinned oracle hash mismatch: {sha(oracle)}")
    if verify_toolchain(TOOLCHAIN) != "msvc5_rtm":
        raise RuntimeError("unexpected historical compiler identity")
    sources, headers = source_inventory(root, TOOLCHAIN)
    source_storage = root / 'src/pc/host_system_key_storage.c'
    storage_declarations = declaration_check(source_storage)
    native_pe = pefile.PE(str(oracle), fast_load=False)
    native_mapped = native_pe.get_memory_mapped_image()
    native_prefixes = []
    for name, (_symbol, ctype, native_va, width) in FIVE_PREFIXES.items():
        native_rva = native_va - native_pe.OPTIONAL_HEADER.ImageBase
        initial = native_mapped[native_rva:native_rva + width]
        relocations = [entry.rva for block in getattr(native_pe, "DIRECTORY_ENTRY_BASERELOC", [])
                       for entry in block.entries
                       if entry.type and entry.rva < native_rva + width and entry.rva + 4 > native_rva]
        if len(initial) != width or any(initial) or relocations:
            raise RuntimeError(f"native prefix {name} lacks zero initial bytes or has a base relocation")
        native_prefixes.append({"name": name, "native_va": hex(native_va), "type": ctype,
                                "width": width, "initial_bytes_hex": initial.hex(),
                                "base_relocations": relocations})

    out = output.resolve()
    work = (root / "work").resolve()
    if not out.is_relative_to(work):
        raise ValueError("diagnostic outputs must stay under repository work/")
    out.mkdir(parents=True, exist_ok=True)
    build = out / "build"
    build.mkdir(parents=True, exist_ok=True)
    runner_source = build / "runner_main.cpp"
    runner_source.write_text(
        '#include "host_system_keys.h"\n'
        'int main() {\n'
        '  CMainFrame *receiver = (CMainFrame *)0x12340000;\n'
        '  receiver->OnSysKeyDown(VK_RETURN, 1, 0);\n'
        '  receiver->OnSysKeyUp(VK_RETURN, 1, 0);\n'
        '  return 0;\n'
        '}\n', encoding="utf-8", newline="\n")
    sources.append(runner_source)
    input_hashes_before = {str(p.resolve()): sha(p) for p in sources + headers}

    include_dirs = [TOOLCHAIN / "include", TOOLCHAIN / "mfc" / "include", root / "src/pc"]
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(p) for p in include_dirs)
    env["LIB"] = ";".join([str(TOOLCHAIN / "lib"), str(TOOLCHAIN / "mfc" / "lib")])
    compiler = TOOLCHAIN / "bin" / "cl.exe"
    objects = []
    builds = []
    for source in sources:
        obj = build / (source.stem + ".obj")
        command = [str(compiler), "/nologo", "/c", "/Gy", "/O2", "/MT",
                   *["/I" + str(p) for p in include_dirs], "/Fo" + str(obj), str(source.resolve())]
        row = run(command, build, env)
        row.update(source=str(source.resolve()), source_sha256=sha(source))
        if row["returncode"] or not obj.is_file():
            builds.append(row)
            raise RuntimeError(f"compile failed for {source.name}; details in build_report.json")
        row.update(object=str(obj.resolve()), object_sha256=sha(obj))
        if source.name == "host_system_key_storage.c":
            row["five_cell_storage"] = verify_coff_bss(obj, source)
        builds.append(row)
        objects.append(obj)

    exe, map_file = build / "host_system_keys_fixture.exe", build / "host_system_keys_fixture.map"
    link_command = [str(compiler), "/nologo", "/O2", "/Gy", "/Fe" + str(exe),
                    *[str(p.resolve()) for p in objects], "/link", "/INCREMENTAL:NO",
                    "/OPT:REF", "/MAP:" + str(map_file.resolve())]
    link = run(link_command, build, env)
    if link["returncode"] or not exe.is_file() or not map_file.is_file():
        raise RuntimeError("fixture link failed; details in build_report.json")
    input_hashes_after = {str(p.resolve()): sha(p) for p in sources + headers}
    if input_hashes_before != input_hashes_after:
        raise RuntimeError("a canonical source/header changed during fresh compile/link")

    handler_source = root / 'src/pc/host_system_keys.cpp'
    handler_obj = build / "host_system_keys.obj"
    coff = COFF(handler_obj)
    comparisons = []
    for label, spec in HANDLERS.items():
        if spec["symbol"] not in [s["name"] for s in coff.symbols.values()]:
            raise RuntimeError(f"fresh handler object lacks expected method {spec['symbol']}")
        raw = match.compare(handler_obj, spec["symbol"], oracle, spec["rva"], spec["size"])
        diagnostic = masked_compare(handler_obj, spec["symbol"], oracle,
                                    spec["rva"], spec["size"])
        comparisons.append({"handler": label, "symbol": spec["symbol"],
                            "target_rva": hex(spec["rva"]), "required_extent": spec["size"],
                            "raw": raw, "relocation_diagnostic_only": diagnostic})

    symbols = parse_map(map_file)
    expected_symbols = ["?OnSysKeyDown@CMainFrame@@QAEXIII@Z",
                        "?OnSysKeyUp@CMainFrame@@QAEXIII@Z",
                        "?Default@CWnd@@IAEJXZ", "_cleanup_native_6ec0"]
    expected_symbols.extend(row[0] for row in FIVE_PREFIXES.values())
    missing = [name for name in expected_symbols if name not in symbols]
    if missing:
        raise RuntimeError("fixture map lacks required linked symbols: " + ", ".join(missing))
    # The linked image's storage prefix must still have loader-zero bytes.
    import pefile
    linked_pe = pefile.PE(str(exe), fast_load=False)
    mapped = linked_pe.get_memory_mapped_image()
    map_zeroes = []
    for name, (symbol, ctype, _va, width) in FIVE_PREFIXES.items():
        address = symbols[symbol]
        if address < linked_pe.OPTIONAL_HEADER.ImageBase:
            raise RuntimeError(f"map symbol {symbol} is not an absolute image address")
        rva = address - linked_pe.OPTIONAL_HEADER.ImageBase
        data = mapped[rva:rva + width]
        if len(data) != width or any(data):
            raise RuntimeError(f"linked uninitialized prefix {symbol} is not zero-filled")
        map_zeroes.append({"symbol": symbol, "type": ctype, "width": width,
                           "linked_address": hex(address), "initial_bytes_hex": data.hex()})

    mfc = mfc_member_provenance(TOOLCHAIN, root)
    report = {
        "scope": "fresh SDK-import object plus actual storage owners, inert link fixture, 14 direct handler cases; no startup/message loop/real API",
        "oracle": str(oracle.resolve()), "oracle_sha256": sha(oracle),
        "toolchain": {"id": verify_toolchain(TOOLCHAIN), "root": str(TOOLCHAIN),
                      "compiler": str(compiler), "compiler_sha256": sha(compiler),
                      "toolchain_manifest_sha256": sha(root / "toolchains" / "manifest.json")},
        "sources": [{"path": str(p.resolve()), "sha256": sha(p)} for p in sources],
        "headers": [{"path": str(p.resolve()), "sha256": sha(p)} for p in headers],
        "input_snapshot_before_after": {"before": input_hashes_before,
                                         "after": input_hashes_after,
                                         "unchanged": input_hashes_before == input_hashes_after},
        "compile_commands": builds,
        "link": link,
        "handler_object": str(handler_obj.resolve()), "handler_object_sha256": sha(handler_obj),
        "fixture_exe": str(exe.resolve()), "fixture_exe_sha256": sha(exe),
        "fixture_map": str(map_file.resolve()), "fixture_map_sha256": sha(map_file),
        "function_comparisons": comparisons,
        "storage_declarations": storage_declarations,
        "storage_zero_prefixes": {"coff": next(x["five_cell_storage"] for x in builds if Path(x["source"]).name == "host_system_key_storage.c"),
                                  "linked_image": map_zeroes,
                                  "pinned_native": native_prefixes,
                                  "source_initializer_spelling_proves_no_extent_or_storage": True},
        "mfc": mfc,
        "limits": [
            "CWnd::Default is linked from pinned nafxcw.lib member 148 and intercepted at its entry; its body effects are not compared.",
            "The linked CMainFrame runtime identity, CFrameWnd base and188-byte extent are measured; the full original C++ declaration and complete window behavior remain unproved.",
            "Invalid device, partial SDK output and window activation semantics are outside these two handlers.",
            "A relocation-normalized match is diagnostic only; raw handler equality requires full bytes and no unresolved relocations.",
        ],
    }
    return report, symbols, exe, map_file


class Machine:
    def __init__(self, mode: str, image: Path, function_addresses: dict[str, int],
                 cells: dict[str, int], default_address: int, cleanup_address: int):
        import pefile
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
        from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
            UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
            UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)
        self.regs = [UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                     UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
                     UC_X86_REG_EFLAGS, UC_X86_REG_EIP]
        self.ecx_reg = UC_X86_REG_ECX
        self.esp_reg, self.eip_reg, self.eax_reg = UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX
        self.mode, self.image = mode, image
        self.data = image.read_bytes()
        self.image_sha256 = hashlib.sha256(self.data).hexdigest()
        self.pe = pefile.PE(data=self.data, fast_load=False)
        if self.pe.OPTIONAL_HEADER.ImageBase != BASE:
            raise RuntimeError(f"unexpected image base {self.pe.OPTIONAL_HEADER.ImageBase:#x}")
        self.functions, self.cells = function_addresses, cells
        self.default_address, self.cleanup_address = default_address, cleanup_address
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        image_size = max(0x800000, self.pe.OPTIONAL_HEADER.SizeOfImage + 0x10000)
        image_size = (image_size + 0xFFF) & ~0xFFF
        self.uc.mem_map(BASE, image_size)
        for section in self.pe.sections:
            raw = self.data[section.PointerToRawData:section.PointerToRawData + section.SizeOfRawData]
            if raw:
                self.uc.mem_write(BASE + section.VirtualAddress, raw)
        self.uc.mem_map(STACK_BASE, STACK_SIZE)
        self.uc.mem_map(IMPORT_BASE, 0x10000)
        self.import_endpoints = {}
        ordinal = 1
        for dll in self.pe.DIRECTORY_ENTRY_IMPORT:
            dll_name = dll.dll.decode("ascii", "replace")
            for item in dll.imports:
                endpoint = IMPORT_BASE + ordinal * 0x10
                name = item.name.decode("ascii", "replace") if item.name else f"ordinal:{item.ordinal}"
                self.import_endpoints[endpoint] = {"dll": dll_name, "name": name, "iat": item.address}
                self.uc.mem_write(item.address, struct.pack("<I", endpoint))
                ordinal += 1
        self.events = []
        self.import_calls = []
        self.outside_writes = []
        self.capture = False
        self.uc.hook_add(UC_HOOK_CODE, self.code_hook)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.write_hook)
        self.regconst = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                         UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
                         UC_X86_REG_EFLAGS, UC_X86_REG_EIP)

    def read(self, name: str) -> int:
        address, width = ORIGINAL_CELLS[name]
        return int.from_bytes(self.uc.mem_read(self.cells[name], width), "little")

    def write(self, name: str, value: int) -> None:
        _, width = ORIGINAL_CELLS[name]
        self.uc.mem_write(self.cells[name], int(value).to_bytes(width, "little", signed=False))

    def read_address(self, address: int, width: int) -> bytes:
        return bytes(self.uc.mem_read(address, width))

    def code_hook(self, uc, address, size, _):
        if address == self.default_address:
            receiver = uc.reg_read(self.ecx_reg)
            self.events.append(["call", "CWnd::Default", receiver])
            esp = uc.reg_read(self.esp_reg)
            ret = struct.unpack("<I", uc.mem_read(esp, 4))[0]
            uc.reg_write(self.eax_reg, 0)
            uc.reg_write(self.esp_reg, esp + 4)
            uc.reg_write(self.eip_reg, ret)
            return
        if address == self.cleanup_address:
            esp = uc.reg_read(self.esp_reg)
            value = struct.unpack("<I", uc.mem_read(esp + 4, 4))[0]
            self.events.append(["call", "cleanup_native_6ec0", value])
            return
        if address in self.import_endpoints:
            record = self.import_endpoints[address]
            self.import_calls.append(record)
            raise RuntimeError(f"fail-on-entry imported endpoint reached: {record['dll']}!{record['name']}")

    def write_hook(self, uc, access, address, size, value, _):
        if not self.capture:
            return
        for name, cell in self.cells.items():
            width = CELL_WIDTHS[name]
            if cell <= address and address + size <= cell + width:
                self.events.append(["write", name, address - cell, size,
                                   value & ((1 << (size * 8)) - 1)])
                return
        if not (STACK_BASE <= address and address + size <= STACK_BASE + STACK_SIZE):
            self.outside_writes.append({"address": hex(address), "size": size,
                                        "value": value & ((1 << (min(size, 8) * 8)) - 1)})

    def call(self, case: dict) -> dict:
        self.events, self.import_calls, self.outside_writes = [], [], []
        for name in self.cells:
            self.write(name, 0)
        for name, value in case["initial"].items():
            self.write(name, value)
        before = {name: self.read(name) for name in self.cells}
        stack = STACK_TOP - 16
        receiver = 0x12340000
        self.uc.mem_write(stack, struct.pack("<4I", SENTINEL, case["key"],
                        case.get("repeat", 1), case.get("flags", 0)))
        start = self.functions[case["handler"]]
        regs = [0x11111111, 0x22222222, receiver, 0x44444444, 0x55555555,
                0x66666666, 0x77777777, stack, 0x202, start]
        for reg, value in zip(self.regconst, regs):
            self.uc.reg_write(reg, value)
        self.capture = True
        self.uc.emu_start(start, SENTINEL, count=50000)
        self.capture = False
        end_pc = self.uc.reg_read(self.eip_reg)
        end_sp = self.uc.reg_read(self.esp_reg)
        if end_pc != SENTINEL:
            raise RuntimeError(f"{self.mode}/{case['name']} did not return; EIP={end_pc:#x}")
        after = {name: self.read(name) for name in self.cells}
        return {"before": before, "after": after, "events": self.events.copy(),
                "api_calls": self.import_calls.copy(), "receiver": receiver,
                "outside_writes": self.outside_writes.copy(),
                "esp_delta": end_sp - stack, "eip_sentinel": end_pc == SENTINEL}


def run_semantics(root: Path, report: dict, symbols: dict[str, int], exe: Path,
                  map_file: Path, oracle: Path, output: Path) -> dict:
    sys.path.insert(0, str(root / "scripts"))
    import pefile

    candidate_cells = {
        "input_state": symbols["_g_exe_96e618"],
        "input_gate": symbols["_g_exe_92c488"],
        "threshold": symbols["_g_exe_ba0238"],
        "mode_busy": symbols["_g_exe_ba0228"],
        "word_busy": symbols["_g_exe_ba0230"],
        "pending": symbols["_g_host_input_pending_rva_7a0234"],
        "fatal_suppress": symbols["_g_fatal_suppress_dialog"],
        "display_mode": symbols["_g_exe_96e3d0"],
    }
    original_functions = {name: BASE + spec["rva"] for name, spec in HANDLERS.items()}
    candidate_functions = {name: symbols[spec["symbol"]] for name, spec in HANDLERS.items()}
    original = Machine("pinned-original", oracle, original_functions,
                       {name: address for name, (address, _width) in ORIGINAL_CELLS.items()},
                       BASE + 0x225EC, BASE + 0x6EC0)
    candidate = Machine("source-linked-fixture", exe, candidate_functions, candidate_cells,
                        symbols["?Default@CWnd@@IAEJXZ"], symbols["_cleanup_native_6ec0"])

    initial_prefixes = []
    for name, (symbol, ctype, original_va, width) in FIVE_PREFIXES.items():
        original_bytes = original.read_address(original_va, width)
        candidate_bytes = candidate.read_address(candidate_cells[name], width)
        if any(original_bytes) or any(candidate_bytes):
            raise RuntimeError(f"five-prefix initial state is not zero for {symbol}")
        initial_prefixes.append({"symbol": symbol, "type": ctype, "width": width,
                                 "original_va": hex(original_va),
                                 "original_initial_hex": original_bytes.hex(),
                                 "candidate_address": hex(candidate_cells[name]),
                                 "candidate_initial_hex": candidate_bytes.hex(),
                                 "equal": original_bytes == candidate_bytes})

    rows = []
    for case in CASES:
        left = original.call(case)
        right = candidate.call(case)
        abi_ok = (left["esp_delta"] == 16 and right["esp_delta"] == 16 and
                  left["eip_sentinel"] and right["eip_sentinel"] and
                  left["receiver"] == right["receiver"] and
                  not left["api_calls"] and not right["api_calls"] and
                  not left["outside_writes"] and not right["outside_writes"])
        equal = left == right and abi_ok
        rows.append({"case": case, "original": left, "candidate": right,
                     "abi_contract_ok": abi_ok, "equal": equal})

    return {
        "case_count": len(rows), "all_cases_equal": all(row["equal"] for row in rows),
        "api_boundary": {"candidate_import_count": len(candidate.import_endpoints),
                         "original_import_count": len(original.import_endpoints),
                         "all_imported_endpoints_fail_on_entry": True,
                         "entered_real_imports": 0,
                         "default_calls_are_synthetic_boundary_events": True},
        "candidate_symbols": {"down": hex(candidate_functions["down"]),
                              "up": hex(candidate_functions["up"]),
                              "default": hex(candidate.default_address),
                              "cleanup_native_6ec0": hex(candidate.cleanup_address),
                              "cells": {k: hex(v) for k, v in candidate_cells.items()}},
        "initial_zero_prefixes": initial_prefixes,
        "cases": rows,
        "limits": [
            "Invalid-device and partial SDK output states do not arise in this handler-only contract.",
            "The MFC Default implementation is not entered; calls are recorded, then return synthetic zero LRESULT.",
            "The physical receiver value is a member-call ABI witness only, not a recovered full window object or original class identity.",
            "The five measured cells are distinct accessed prefixes; no aggregate extent is inferred.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        help="repository containing the canonical src/ sources; defaults from this script's installation")
    parser.add_argument("--native-exe", type=Path, default=ORACLE_DEFAULT,
                        help="read-only pinned retail oracle location")
    parser.add_argument("--output", type=Path,
                        help="diagnostic destination under work/; defaults to work/host_system_keys_public")
    args = parser.parse_args()
    root = (args.source_root.resolve() if args.source_root else repository_root(Path(__file__).resolve()))
    oracle = args.native_exe.resolve()
    output = (args.output.resolve() if args.output else root / "work" / "host_system_keys_public")
    if not output.is_relative_to((root / "work").resolve()):
        raise SystemExit("--output must remain under the repository's ignored work/ directory")
    output.mkdir(parents=True, exist_ok=True)
    report_path = output / "host_system_keys_differential.json"
    try:
        build, symbols, exe, map_file = compile_link(root, output, oracle)
        semantic = run_semantics(root, build, symbols, exe, map_file, oracle, output)
        build["semantics"] = semantic
        build["runner"] = {"path": str(Path(__file__).resolve()), "sha256": sha(Path(__file__))}
        build["report"] = str(report_path.resolve())
        build["raw_function_exact"] = all(x["raw"]["exact"] for x in build["function_comparisons"])
        build["all_semantic_cases_equal"] = semantic["all_cases_equal"]
        build["strict_outcome"] = "FUNCTION_MATCH_NOT_CLAIMED"
        report_path.write_text(json.dumps(build, indent=2) + "\n", encoding="utf-8", newline="\n")
        summary = {"report": str(report_path), "oracle_sha256": build["oracle_sha256"],
                   "handler_object_sha256": build["handler_object_sha256"],
                   "raw": [{"handler": x["handler"], "raw_exact": x["raw"]["exact"],
                            "compiled_bytes": x["raw"]["compiled_size"],
                            "target_bytes": x["raw"]["target_size"],
                            "masked_diagnostic_equal": x["relocation_diagnostic_only"]["masked_equal"]}
                           for x in build["function_comparisons"]],
                   "cases": semantic["case_count"], "all_cases_equal": semantic["all_cases_equal"],
                   "entered_real_imports": semantic["api_boundary"]["entered_real_imports"]}
        print(json.dumps(summary, indent=2))
        if not semantic["all_cases_equal"]:
            raise SystemExit(1)
    except Exception as exc:
        failure = {"scope": "bounded host SYSKEY runner failed before completing its proof",
                   "error": f"{type(exc).__name__}: {exc}",
                   "oracle": str(oracle), "oracle_sha256": sha(oracle) if oracle.is_file() else None,
                   "runner": str(Path(__file__).resolve()),
                   "runner_sha256": sha(Path(__file__))}
        report_path.write_text(json.dumps(failure, indent=2) + "\n", encoding="utf-8", newline="\n")
        raise


if __name__ == "__main__":
    main()
