"""Public, self-contained VC5 build and bounded direct-call callback comparison."""
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


def repository_root(script: Path) -> Path:
    """Find the checkout by its installed matching/verification scripts."""
    for parent in script.resolve().parents:
        if (parent / "scripts" / "match.py").is_file() and (parent / "src/pc").is_dir():
            return parent
    raise RuntimeError("cannot locate repository root from installed scripts")


ROOT = repository_root(Path(__file__))
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original-root', type=Path, default=Path(r'D:\Prog\hercules_recon'))
parser.add_argument('--output', type=Path, default=ROOT / 'work/host_frame_command_ui_public')
args = parser.parse_args()
LANE = args.output.resolve()
if not LANE.is_relative_to((ROOT / 'work').resolve()):
    raise SystemExit('--output must remain under the repository work/ directory')
LANE.mkdir(parents=True, exist_ok=True)
ORIGINAL_ROOT = args.original_root.resolve()
ORACLE = ORIGINAL_ROOT / "work" / "discs" / "pc_install" / "HERCULES.EXE"
ORACLE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
NATIVE_SPAN_RECEIPT = "work/host_frame_ui_callbacks/host_frame_ui_callbacks_canonical_header_report.json"
NATIVE_SPAN_RECEIPT_SHA256 = "1c8cf238a44f443e69d0b5db779d33c9ee1a53734a3c6b070d24ab603aa15983"
SOURCE = ROOT / "src" / "pc" / "host_frame_command_ui.cpp"
CANONICAL_HEADER = ROOT / 'src/pc/host_system_keys.h'
STARTUP_HEADER = ROOT / 'src/pc/host_startup_storage.h'
SYSTEM_KEY_OWNER = ROOT / 'src/pc/host_system_key_storage.c'
STARTUP_OWNER = ROOT / 'src/pc/host_startup_storage.c'
GRAPHICS_OWNER = ROOT / 'src/pc/graphics_storage.cpp'
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
TOOLCHAIN_MANIFEST = ROOT / "toolchains" / "manifest.json"
SDK_HEADERS = [
    TOOLCHAIN / "mfc" / "include" / "afxwin.h",
    TOOLCHAIN / "mfc" / "include" / "afxmsg_.h",
    TOOLCHAIN / "include" / "windows.h",
    TOOLCHAIN / "include" / "mmsystem.h",
    TOOLCHAIN / "include" / "ddraw.h",
]
LINK_ARCHIVES = [
    TOOLCHAIN / "mfc" / "lib" / "nafxcw.lib",
    TOOLCHAIN / "lib" / "libcmt.lib",
    TOOLCHAIN / "lib" / "kernel32.lib",
    TOOLCHAIN / "lib" / "user32.lib",
    TOOLCHAIN / "lib" / "gdi32.lib",
    TOOLCHAIN / "lib" / "comdlg32.lib",
    TOOLCHAIN / "lib" / "winspool.lib",
    TOOLCHAIN / "lib" / "advapi32.lib",
    TOOLCHAIN / "lib" / "shell32.lib",
    TOOLCHAIN / "lib" / "comctl32.lib",
    TOOLCHAIN / "lib" / "uuid.lib",
    TOOLCHAIN / "lib" / "oldnames.lib",
]
BUILD = LANE / "public_build"
ANCHOR = BUILD / "fixture_anchor.cpp"
REPORT = LANE / "host_frame_command_ui_differential.json"
COMPILER = TOOLCHAIN / "bin" / "cl.exe"
LINKER = TOOLCHAIN / "bin" / "link.exe"

# Complete target spans and their pinned receipt digests. Each span is exactly
# 32 bytes, including the native NOP padding. No runtime receipt file is needed.
FUNCTIONS = {
    "signed_byte": {
        "symbol_prefix": "OnRva_00009A90", "rva": 0x9A90, "size": 32,
        "native_sha256": "044b0568b6849eaeebe4ab043b2efcd1f1c37421e91953d72d8b4493bdbe652e",
        "cell": "g_exe_92c488", "va": 0x92C488, "width": 1,
        "values": [0, 1, 0x7F, 0x80, 0xFF],
    },
    "full_dword": {
        "symbol_prefix": "OnRva_00009BF0", "rva": 0x9BF0, "size": 32,
        "native_sha256": "8ba1d93101b434341ef53cf47f518e65a9c123ab61ad8e150544461f4e7600a3",
        "cell": "g_startup_apply_guard_rva_51c450", "va": 0x91C450, "width": 4,
        "values": [0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF],
    },
    "inverted_dword": {
        "symbol_prefix": "OnRva_00009C10", "rva": 0x9C10, "size": 32,
        "native_sha256": "25f37a65d37d052984a587ab841e591143b14d117faf3c5b12dc1eaf416f84ca",
        "cell": "g_exe_91c44c", "va": 0x91C44C, "width": 4,
        "values": [0, 1, 0x7FFFFFFF, 0x80000000, 0x80000001, 0xFFFFFFFF],
    },
}
SOURCE_GLOBALS = {
    "g_exe_92c488": "_g_exe_92c488",
    "g_startup_apply_guard_rva_51c450": "_g_startup_apply_guard_rva_51c450",
    "g_exe_91c44c": "_g_exe_91c44c",
}

BASE = 0x400000
STACK_BASE = 0x60000000
STACK_SIZE = 0x10000
STACK_ENTRY = STACK_BASE + 0x8000
DATA_BASE = 0x61000000
DATA_SIZE = 0x10000
VTABLE = DATA_BASE + 0x100
CMDUI = DATA_BASE + 0x300
FRAME = DATA_BASE + 0x500
FRAME_SIZE = 188
ENABLE_ENDPOINT = DATA_BASE + 0x800
RETURN_SENTINEL = DATA_BASE + 0x9000
STACK_PATTERN = bytes([0xD3]) * 0x80
FRAME_PATTERN = bytes([0xA6]) * FRAME_SIZE
CMDUI_PATTERN = bytes([0xB7]) * 0x40
VTABLE_PATTERN = bytes([0xC8]) * 0x40
STACK_WRITE_LOW = STACK_ENTRY - 0x100
STACK_WRITE_HIGH = STACK_ENTRY + len(STACK_PATTERN)
INSTRUCTION_CAP = 256
NONVOLATILE_SEED = {
    "ebx": 0x13579BDF,
    "esi": 0x2468ACE0,
    "edi": 0x0BADCAFE,
    "ebp": 0xF00DFACE,
}

for _path in (Path(r"C:\tools\hercules\renderer-oracle-python"),
              Path(r"C:\tools\hercules\python")):
    if str(_path) not in sys.path:
        sys.path.insert(0, str(_path))
sys.path.insert(0, str(ROOT / "scripts"))

from coff import COFF
from match import verify_toolchain
from pefile import PE as PeFile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.unicorn_const import UC_PROT_ALL
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
)

REGISTERS = {
    "eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX,
    "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX,
    "esi": UC_X86_REG_ESI, "edi": UC_X86_REG_EDI,
    "ebp": UC_X86_REG_EBP, "esp": UC_X86_REG_ESP,
    "eip": UC_X86_REG_EIP, "eflags": UC_X86_REG_EFLAGS,
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def align(value: int, boundary: int = 0x1000) -> int:
    return (value + boundary - 1) & ~(boundary - 1)


def command(argv: list[str], cwd: Path, env: dict[str, str], log: Path) -> dict:
    proc = subprocess.run(argv, cwd=cwd, env=env, capture_output=True,
                          text=True, errors="replace", timeout=120)
    log.write_text(proc.stdout + proc.stderr, encoding="utf-8", newline="\n")
    row = {"command": argv, "cwd": str(cwd), "returncode": proc.returncode,
           "stdout": proc.stdout, "stderr": proc.stderr, "log": str(log),
           "log_sha256": sha(log)}
    if proc.returncode:
        raise RuntimeError(f"command failed: {json.dumps(row, indent=2)}")
    return row


def map_symbols(path: Path) -> dict[str, int]:
    result = {}
    pattern = re.compile(r"^\s*[0-9A-Fa-f]{4}:([0-9A-Fa-f]{8})\s+(\S+)\s+([0-9A-Fa-f]{8})")
    for line in path.read_text(encoding="ascii", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            result[match.group(2)] = int(match.group(3), 16)
    return result


def source_symbol(coff: COFF, prefix: str) -> str:
    matches = sorted({row["name"] for row in coff.symbols.values()
                      if row["storage"] == 2 and prefix + "@CMainFrame@@" in row["name"]})
    if len(matches) != 1:
        raise RuntimeError(f"expected one defined CMainFrame symbol for {prefix}, found {matches}")
    return matches[0]


def write_generated_anchor() -> str:
    text = (
        '#include "' + os.path.relpath(CANONICAL_HEADER, BUILD).replace('\\', '/') + '"\n\n'
        'extern "C" __declspec(dllexport) void __cdecl host_frame_ui_fixture_anchor(\n'
        '    CMainFrame *frame, CCmdUI *command)\n'
        '{\n'
        '    frame->OnRva_00009A90(command);\n'
        '    frame->OnRva_00009BF0(command);\n'
        '    frame->OnRva_00009C10(command);\n'
        '}\n'
    )
    BUILD.mkdir(parents=True, exist_ok=True)
    ANCHOR.write_bytes(text.encode("utf-8"))
    return text


def compile_and_link() -> dict:
    if not SOURCE.is_file():
        raise RuntimeError(f"canonical callback source is not published yet: {SOURCE}")
    anchor_text = write_generated_anchor()
    if sha(ORACLE) != ORACLE_SHA256:
        raise RuntimeError("pinned HERCULES.EXE oracle hash changed")
    if verify_toolchain(TOOLCHAIN) != "msvc5_rtm":
        raise RuntimeError("pinned compiler identity is not msvc5_rtm")

    base_inputs = [Path(__file__).resolve(), ROOT / 'scripts/match.py',
                   ROOT / 'scripts/coff.py', SOURCE, ORACLE,
                   CANONICAL_HEADER, STARTUP_HEADER, SYSTEM_KEY_OWNER,
                   STARTUP_OWNER, GRAPHICS_OWNER, TOOLCHAIN_MANIFEST,
                   COMPILER, LINKER, *SDK_HEADERS, *LINK_ARCHIVES]
    before = {str(path.resolve()): sha(path) for path in base_inputs}
    before[str(ANCHOR.resolve())] = sha(ANCHOR)
    include_dirs = [TOOLCHAIN / "include", TOOLCHAIN / "mfc" / "include",
                    ROOT / "src/pc"]
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = ";".join(str(path) for path in include_dirs)
    env["LIB"] = ";".join([str(TOOLCHAIN / "lib"), str(TOOLCHAIN / "mfc" / "lib")])

    object_sources = [
        (SOURCE, ["/Gy", "/O2", "/MT", "/GX"]),
        (ANCHOR, ["/Gy", "/O2", "/MT", "/GX"]),
        (SYSTEM_KEY_OWNER, ["/Gy", "/O2", "/MT"]),
        (STARTUP_OWNER, ["/Gy", "/O2", "/MT"]),
        (GRAPHICS_OWNER, ["/Gy", "/O2", "/MT", "/GX"]),
    ]
    objects, compiles = [], []
    for source, flags in object_sources:
        obj = BUILD / (source.stem + ".obj")
        argv = [str(COMPILER), "/nologo", "/c", "/showIncludes", *flags,
                *["/I" + str(path) for path in include_dirs],
                "/Fo" + str(obj), str(source.resolve())]
        record = command(argv, BUILD, env, BUILD / (source.stem + ".compile.log"))
        record.update({"source": str(source.resolve()), "source_sha256": sha(source),
                       "object": str(obj.resolve()), "object_sha256": sha(obj)})
        compiles.append(record)
        objects.append(obj)

    dll = BUILD / "host_frame_ui_fixture.dll"
    map_path = BUILD / "host_frame_ui_fixture.map"
    link_argv = [str(LINKER), "/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
                 "/INCREMENTAL:NO", "/OPT:REF", "/MAP:" + str(map_path),
                 "/OUT:" + str(dll), *[str(path.resolve()) for path in objects],
                 *[str(path.resolve()) for path in LINK_ARCHIVES]]
    link = command(link_argv, BUILD, env, BUILD / "fixture_link.log")
    after = {str(path.resolve()): sha(path) for path in base_inputs}
    after[str(ANCHOR.resolve())] = sha(ANCHOR)
    if before != after:
        raise RuntimeError("one or more source/header/library/toolchain inputs changed during build")

    map_data = map_symbols(map_path)
    linked_pe = PeFile(str(dll), fast_load=False)
    image_base = linked_pe.OPTIONAL_HEADER.ImageBase
    image_size = linked_pe.OPTIONAL_HEADER.SizeOfImage
    if "_host_frame_ui_fixture_anchor" not in map_data:
        raise RuntimeError("fixture map lacks its generated anchor")
    global_addresses = {}
    for name, symbol in SOURCE_GLOBALS.items():
        if symbol not in map_data:
            raise RuntimeError(f"fixture map lacks actual owner cell {symbol}")
        address = map_data[symbol]
        if not image_base <= address < image_base + image_size:
            raise RuntimeError(f"owner symbol {symbol} falls outside the linked source image")
        global_addresses[name] = address

    candidate_obj = BUILD / "host_frame_command_ui.obj"
    candidate_coff = COFF(candidate_obj)
    native_pe = PeFile(str(ORACLE), fast_load=False)
    if native_pe.OPTIONAL_HEADER.ImageBase != BASE:
        raise RuntimeError("HERCULES.EXE does not have expected fixed 0x400000 image base")
    native_relocs = [
        {"rva": entry.rva, "type": entry.type}
        for block in getattr(native_pe, "DIRECTORY_ENTRY_BASERELOC", [])
        for entry in block.entries
    ]
    contributions, symbols = [], {}
    for label, spec in FUNCTIONS.items():
        symbol = source_symbol(candidate_coff, spec["symbol_prefix"])
        symbols[label] = symbol
        contribution = candidate_coff.function(symbol)
        target = native_pe.get_data(spec["rva"], spec["size"])
        if hashlib.sha256(target).hexdigest() != spec["native_sha256"]:
            raise RuntimeError(f"pinned 32-byte native span changed at {spec['rva']:#x}")
        candidate = contribution["data"]
        diffs = [i for i, (a, b) in enumerate(zip(target, candidate)) if a != b]
        diff_count = len(diffs) + abs(len(target) - len(candidate))
        native_span_relocs = [row for row in native_relocs
                              if spec["rva"] <= row["rva"] < spec["rva"] + spec["size"]]
        contributions.append({
            "callback": label, "symbol": symbol, "rva": hex(spec["rva"]),
            "native_extent_bytes": len(target), "candidate_coff_bytes": len(candidate),
            "native_sha256": hashlib.sha256(target).hexdigest(),
            "pinned_native_sha256": spec["native_sha256"],
            "candidate_sha256": hashlib.sha256(candidate).hexdigest(),
            "raw_byte_difference_count": diff_count,
            "first_raw_difference_offsets": diffs[:24],
            "candidate_relocations": contribution["relocations"],
            "native_pe_base_relocations_in_span": native_span_relocs,
            "raw_function_equal": target == candidate and not contribution["relocations"] and not native_span_relocs,
            "comparison": "Complete 32-byte target span versus complete fresh COFF contribution; no operand masking. Relocation-bearing or unequal output is diagnostic only.",
        })

    return {
        "compile_commands": compiles,
        "link": link,
        "generated_anchor": {"path": str(ANCHOR.resolve()),
                             "sha256": sha(ANCHOR), "source_text": anchor_text},
        "objects": [{"path": str(path.resolve()), "sha256": sha(path)} for path in objects],
        "dll": str(dll.resolve()), "dll_sha256": sha(dll),
        "map": str(map_path.resolve()), "map_sha256": sha(map_path),
        "archive_hashes": {str(path.resolve()): sha(path) for path in LINK_ARCHIVES},
        "image_base": hex(image_base), "image_size": image_size,
        "function_symbols": symbols,
        "source_global_addresses": {name: hex(address) for name, address in global_addresses.items()},
        "global_addresses_raw": global_addresses,
        "source_contributions": contributions,
        "input_hashes_before": before, "input_hashes_after": after,
        "inputs_unchanged": before == after,
        "native_image_base": hex(native_pe.OPTIONAL_HEADER.ImageBase),
        "native_image_size": native_pe.OPTIONAL_HEADER.SizeOfImage,
        "native_relocation_directory_present": bool(native_relocs),
    }


def pack32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def read32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", uc.mem_read(address, 4))[0]


def write_cell(uc: Uc, address: int, value: int, width: int) -> None:
    uc.mem_write(address, (value & ((1 << (width * 8)) - 1)).to_bytes(width, "little"))


def mapped_pe(path: Path) -> tuple[PeFile, bytes]:
    pe = PeFile(str(path), fast_load=False)
    image = pe.get_memory_mapped_image()
    size = pe.OPTIONAL_HEADER.SizeOfImage
    if len(image) > size:
        raise RuntimeError(f"mapped PE image {path} exceeds SizeOfImage")
    return pe, image.ljust(size, b"\0")


def run_direct(image_path: Path, function_addresses: dict[str, int],
               global_addresses: dict[str, int], inputs: dict[str, int]) -> dict:
    pe, image = mapped_pe(image_path)
    base = pe.OPTIONAL_HEADER.ImageBase
    image_size = pe.OPTIONAL_HEADER.SizeOfImage
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(base, align(image_size), UC_PROT_ALL)
    uc.mem_write(base, image)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_ALL)
    uc.mem_map(DATA_BASE, DATA_SIZE, UC_PROT_ALL)
    # Unicorn supplies the zero-filled endpoint page. No endpoint opcodes are written.
    uc.mem_write(VTABLE, VTABLE_PATTERN)
    uc.mem_write(VTABLE, pack32(ENABLE_ENDPOINT))
    uc.mem_write(CMDUI, CMDUI_PATTERN)
    uc.mem_write(CMDUI, pack32(VTABLE))
    uc.mem_write(FRAME, FRAME_PATTERN)

    for spec in FUNCTIONS.values():
        address = global_addresses[spec["cell"]] if spec["cell"] in global_addresses else global_addresses[
            next(label for label, row in FUNCTIONS.items() if row is spec)]
        write_cell(uc, address, inputs.get(spec["cell"], 0), spec["width"])

    cell_before = {}
    for label, spec in FUNCTIONS.items():
        address = global_addresses[spec["cell"]] if spec["cell"] in global_addresses else global_addresses[label]
        begin, end = address - 8, address + spec["width"] + 8
        cell_before[spec["cell"]] = bytes(uc.mem_read(begin, end - begin))

    events, unexpected_exec, unexpected_writes = [], [], []
    hook_register_checks = []
    instruction_count = 0
    current_callback = next(iter(function_addresses))
    allowed_ranges = {
        label: (address, address + FUNCTIONS[label]["size"])
        for label, address in function_addresses.items()
    }

    def on_code(machine, address, size, _user):
        nonlocal instruction_count
        if address == RETURN_SENTINEL:
            machine.emu_stop()
            return
        if address == ENABLE_ENDPOINT:
            instruction_count += 1
            if instruction_count > INSTRUCTION_CAP:
                unexpected_exec.append("instruction cap reached at synthetic Enable endpoint")
                machine.emu_stop()
                return
            esp = machine.reg_read(UC_X86_REG_ESP)
            return_address = read32(machine, esp)
            arg = read32(machine, esp + 4)
            low, high = allowed_ranges[current_callback]
            if not low <= return_address < high:
                unexpected_exec.append(f"Enable return address outside callback span: {return_address:#x}")
                machine.emu_stop()
                return
            preserved_before = {name: machine.reg_read(register)
                                for name, register in REGISTERS.items()
                                if name not in ("esp", "eip")}
            events.append({
                "cmd_ui_this": hex(machine.reg_read(UC_X86_REG_ECX)),
                "enable_value": hex(arg),
                "return_address_in_image": base <= return_address < base + image_size,
                "return_address": hex(return_address),
            })
            # Synthetic CCmdUI::Enable returns with ret 4, preserving all other
            # registers. This is control-state modeling, not endpoint machine code.
            machine.reg_write(UC_X86_REG_ESP, esp + 8)
            machine.reg_write(UC_X86_REG_EIP, return_address)
            preserved_after = {name: machine.reg_read(register)
                               for name, register in REGISTERS.items()
                               if name not in ("esp", "eip")}
            hook_register_checks.append(preserved_before == preserved_after)
            machine.emu_stop()
            return

        low, high = allowed_ranges[current_callback]
        if low <= address < high:
            instruction_count += 1
            if instruction_count > INSTRUCTION_CAP:
                unexpected_exec.append(f"instruction cap reached at {address:#x}")
                machine.emu_stop()
            return
        unexpected_exec.append(f"execution outside callback whitelist: {address:#x}")
        machine.emu_stop()

    def on_mem_write(machine, access, address, size, value, _user):
        if STACK_WRITE_LOW <= address and address + size <= STACK_WRITE_HIGH:
            return
        unexpected_writes.append({"address": hex(address), "size": size,
                                  "value": hex(value & 0xFFFFFFFF)})
        machine.emu_stop()

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_mem_write)
    call_results = []
    call_sequence = list(function_addresses)
    if not call_sequence:
        raise ValueError("direct callback sequence cannot be empty")

    for callback in call_sequence:
        current_callback = callback
        instruction_count = 0
        event_start = len(events)
        uc.mem_write(STACK_ENTRY, STACK_PATTERN)
        expected_stack = pack32(RETURN_SENTINEL) + pack32(CMDUI) + STACK_PATTERN[8:]
        uc.mem_write(STACK_ENTRY, expected_stack)
        uc.reg_write(UC_X86_REG_ESP, STACK_ENTRY)
        uc.reg_write(UC_X86_REG_ECX, FRAME)
        uc.reg_write(UC_X86_REG_EAX, 0x11111111)
        uc.reg_write(UC_X86_REG_EDX, 0x22222222)
        for name, value in NONVOLATILE_SEED.items():
            uc.reg_write(REGISTERS[name], value)
        uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        address = function_addresses[callback]
        uc.reg_write(UC_X86_REG_EIP, address)
        steps = 0
        while uc.reg_read(UC_X86_REG_EIP) != RETURN_SENTINEL and not unexpected_exec and not unexpected_writes:
            if steps >= INSTRUCTION_CAP:
                unexpected_exec.append("outer execution instruction cap reached")
                break
            start_eip = uc.reg_read(UC_X86_REG_EIP)
            remaining = max(1, INSTRUCTION_CAP - instruction_count)
            uc.emu_start(start_eip, RETURN_SENTINEL, timeout=1_000_000, count=remaining)
            steps += 1
            current_eip = uc.reg_read(UC_X86_REG_EIP)
            if current_eip == start_eip and not unexpected_exec and not unexpected_writes:
                unexpected_exec.append(f"emulator made no progress at {current_eip:#x}")
                break
        current_eip = uc.reg_read(UC_X86_REG_EIP)
        current_esp = uc.reg_read(UC_X86_REG_ESP)
        call_events = events[event_start:]
        stack_after = bytes(uc.mem_read(STACK_ENTRY, len(STACK_PATTERN)))
        call_results.append({
            "callback": callback,
            "entry_eip": hex(address), "return_eip": hex(current_eip),
            "entry_esp": hex(STACK_ENTRY), "return_esp": hex(current_esp),
            "instruction_count": instruction_count,
            "events": call_events,
            "synthetic_enable_preserved_other_registers": all(hook_register_checks) if call_events else False,
            "nonvolatile_registers": {name: hex(uc.reg_read(REGISTERS[name]))
                                       for name in NONVOLATILE_SEED},
            "stack_window_unchanged": stack_after == expected_stack,
            "frame_188_bytes_unchanged": bytes(uc.mem_read(FRAME, FRAME_SIZE)) == FRAME_PATTERN,
            "cmd_ui_tail_unchanged": bytes(uc.mem_read(CMDUI + 4, len(CMDUI_PATTERN) - 4)) == CMDUI_PATTERN[4:],
            "vtable_tail_unchanged": bytes(uc.mem_read(VTABLE + 4, len(VTABLE_PATTERN) - 4)) == VTABLE_PATTERN[4:],
        })
        uc.reg_write(UC_X86_REG_EIP, 0)

    cell_after = {}
    for label, spec in FUNCTIONS.items():
        address = global_addresses[spec["cell"]] if spec["cell"] in global_addresses else global_addresses[label]
        begin, end = address - 8, address + spec["width"] + 8
        cell_after[spec["cell"]] = bytes(uc.mem_read(begin, end - begin))
    return {
        "calls": call_results, "events": events,
        "unexpected_executions": unexpected_exec,
        "unexpected_memory_writes": unexpected_writes,
        "owner_cells_and_neighbor_bytes_unchanged": cell_before == cell_after,
        "owner_snapshot_sha256": {name: hashlib.sha256(value).hexdigest()
                                   for name, value in cell_after.items()},
        "instruction_cap": INSTRUCTION_CAP,
        "code_entry_whitelist": {name: [hex(lo), hex(hi)]
                                  for name, (lo, hi) in allowed_ranges.items()},
        "image": {"base": hex(base), "size": image_size},
    }


def expected_value(callback: str, value: int) -> int:
    if callback == "signed_byte":
        return (value if value < 0x80 else value - 0x100) & 0xFFFFFFFF
    if callback == "full_dword":
        return value & 0xFFFFFFFF
    return int(value == 0)


def validate_result(result: dict, expected: list[tuple[str, int]]) -> list[str]:
    failures = []
    if result["unexpected_executions"]:
        failures.append("unexpected execution: " + "; ".join(result["unexpected_executions"]))
    if result["unexpected_memory_writes"]:
        failures.append("writes outside callback stack frame: " + json.dumps(result["unexpected_memory_writes"]))
    if not result["owner_cells_and_neighbor_bytes_unchanged"]:
        failures.append("callback changed an owner cell or adjacent canary bytes")
    if len(result["calls"]) != len(expected):
        failures.append("direct-call count differs from fixture sequence")
    for i, ((callback, arg), call) in enumerate(zip(expected, result["calls"])):
        label = f"{callback}[{i}]"
        if call["return_eip"] != hex(RETURN_SENTINEL):
            failures.append(label + " did not return to caller sentinel")
        if call["return_esp"] != hex(STACK_ENTRY + 8):
            failures.append(label + " did not perform callback ret 4 cleanup")
        if call["events"] != [{
                "cmd_ui_this": hex(CMDUI), "enable_value": hex(arg),
                "return_address_in_image": True,
                "return_address": call["events"][0]["return_address"] if call["events"] else "missing",
        }]:
            failures.append(label + " did not make exactly one expected virtual Enable event")
        elif not call["events"][0]["return_address_in_image"]:
            failures.append(label + " synthetic Enable return address is outside the image")
        if not call["synthetic_enable_preserved_other_registers"]:
            failures.append(label + " synthetic Enable model changed another register")
        if not call["stack_window_unchanged"]:
            failures.append(label + " changed caller stack arguments/canaries")
        if not call["frame_188_bytes_unchanged"]:
            failures.append(label + " changed the complete 188-byte CMainFrame receiver")
        if not call["cmd_ui_tail_unchanged"] or not call["vtable_tail_unchanged"]:
            failures.append(label + " changed CCmdUI or vtable canaries")
        if call["nonvolatile_registers"] != {name: hex(value) for name, value in NONVOLATILE_SEED.items()}:
            failures.append(label + " did not preserve x86 nonvolatile registers")
    return failures


def semantic_signature(result: dict) -> dict:
    fields = ("callback", "return_eip", "return_esp", "events",
              "synthetic_enable_preserved_other_registers", "nonvolatile_registers",
              "stack_window_unchanged", "frame_188_bytes_unchanged",
              "cmd_ui_tail_unchanged", "vtable_tail_unchanged")
    calls = []
    for call in result["calls"]:
        row = {key: call[key] for key in fields}
        row["events"] = [
            {key: event[key] for key in ("cmd_ui_this", "enable_value", "return_address_in_image")}
            for event in call["events"]
        ]
        calls.append(row)
    events = [
        {key: event[key] for key in ("cmd_ui_this", "enable_value", "return_address_in_image")}
        for event in result["events"]
    ]
    return {
        "calls": calls,
        "events": events,
        "unexpected_executions": result["unexpected_executions"],
        "unexpected_memory_writes": result["unexpected_memory_writes"],
        "owner_cells_and_neighbor_bytes_unchanged": result["owner_cells_and_neighbor_bytes_unchanged"],
    }


def differential(build: dict) -> dict:
    failures, cases = [], []
    map_data = map_symbols(Path(build["map"]))
    source_functions = {}
    for label, symbol in build["function_symbols"].items():
        if symbol not in map_data:
            raise RuntimeError(f"fixture map lacks callback {symbol}")
        source_functions[label] = map_data[symbol]
    native_functions = {label: BASE + spec["rva"] for label, spec in FUNCTIONS.items()}
    native_globals = {label: spec["va"] for label, spec in FUNCTIONS.items()}

    for label, spec in FUNCTIONS.items():
        for value in spec["values"]:
            inputs = {spec["cell"]: value}
            expected_arg = expected_value(label, value)
            expected = [(label, expected_arg)]
            native = run_direct(ORACLE, {label: native_functions[label]}, native_globals, inputs)
            source = run_direct(Path(build["dll"]), {label: source_functions[label]},
                                build["global_addresses_raw"], inputs)
            native_bad, source_bad = validate_result(native, expected), validate_result(source, expected)
            equal = semantic_signature(native) == semantic_signature(source)
            failures.extend([f"{label} {value:#x} native: {item}" for item in native_bad])
            failures.extend([f"{label} {value:#x} source: {item}" for item in source_bad])
            if not equal:
                failures.append(f"{label} {value:#x} source/original results differ")
            cases.append({"callback": label, "owner_value": hex(value),
                          "expected_enable_value": hex(expected_arg),
                          "native": native, "source": source, "same_measured_result": equal})

    triple_values = {
        "g_exe_92c488": 0x80,
        "g_startup_apply_guard_rva_51c450": 0x89ABCDEF,
        "g_exe_91c44c": 0,
    }
    order = ["signed_byte", "full_dword", "inverted_dword"]
    expected_order = [("signed_byte", 0xFFFFFF80), ("full_dword", 0x89ABCDEF),
                      ("inverted_dword", 1)]
    native_order = run_direct(ORACLE, native_functions, native_globals, triple_values)
    source_order = run_direct(Path(build["dll"]), source_functions,
                              build["global_addresses_raw"], triple_values)
    native_order_bad = validate_result(native_order, expected_order)
    source_order_bad = validate_result(source_order, expected_order)
    failures.extend(["native order: " + item for item in native_order_bad])
    failures.extend(["source order: " + item for item in source_order_bad])
    order_equal = semantic_signature(native_order) == semantic_signature(source_order)
    if not order_equal:
        failures.append("ordered three-callback source/original trace differs")
    return {
        "status": "PASS" if not failures else "FAIL",
        "cases": cases,
        "ordered_three_callback_case": {
            "order": order, "input_cells": {name: hex(value) for name, value in triple_values.items()},
            "expected_enable_values": [hex(value) for _, value in expected_order],
            "native": native_order, "source": source_order,
            "same_measured_result": order_equal,
        },
        "case_count": len(cases) + 1,
        "virtual_enable_event_count": sum(len(row["native"]["events"]) for row in cases)
            + len(native_order["events"]),
        "failure_count": len(failures), "failures": failures,
        "fixture_execution": "Direct Unicorn calls into only the pinned callback spans. The synthetic virtual Enable endpoint is handled by the code hook, which records the event, advances ESP by eight, redirects EIP to the call continuation and stops the emulation slice. No endpoint bytes, executable entry point, DLL initializer, MFC implementation, real API, window or message loop runs.",
        "scope": "17 directed calls / 19 virtual events compare callback semantics, stack/ABI boundary, all 188 receiver bytes, measured owner cells and neighbors, fixture canaries, execution whitelist and bounded memory writes.",
    }


def main() -> None:
    if sha(ORACLE) != ORACLE_SHA256:
        raise RuntimeError("pinned HERCULES.EXE oracle hash changed before run")
    build = compile_and_link()
    diff = differential(build)
    if build['input_hashes_after'] != {path: sha(Path(path)) for path in build['input_hashes_after']}:
        raise RuntimeError('a pinned input changed during the bounded differential')
    report = {
        "status": "PASS_BOUNDED_DIRECT_UNICORN_DIFFERENTIAL" if diff["status"] == "PASS" else "FAIL_DIFFERENTIAL",
        "scope": "Self-contained published runner for three canonical HERCULES.EXE command-UI callbacks; no production entry or canonical/recovery mutation.",
        "repository_root_discovery": "Walk this runner's parent directories until installed scripts/match.py and src/pc/ are found.",
        "oracle": {"path": str(ORACLE.resolve()), "sha256": sha(ORACLE),
                   "image_base": build["native_image_base"], "image_size": build["native_image_size"]},
        "native_span_receipt": {
            "receipt_path_for_provenance_only": NATIVE_SPAN_RECEIPT,
            "receipt_sha256_for_provenance_only": NATIVE_SPAN_RECEIPT_SHA256,
            "runtime_receipt_dependency": False,
            "spans": [{"rva": hex(spec["rva"]), "extent": spec["size"],
                       "sha256": spec["native_sha256"]} for spec in FUNCTIONS.values()],
        },
        "source_selection": {
            "canonical_callback_source": str(SOURCE.resolve()), "sha256": sha(SOURCE),
            "canonical_class_header": str(CANONICAL_HEADER.resolve()), "sha256_header": sha(CANONICAL_HEADER),
            "canonical_startup_header": str(STARTUP_HEADER.resolve()), "sha256_startup_header": sha(STARTUP_HEADER),
            "storage_owner_sources": [
                {"path": str(path.resolve()), "sha256": sha(path)}
                for path in (SYSTEM_KEY_OWNER, STARTUP_OWNER, GRAPHICS_OWNER)
            ],
            "generated_fixture_anchor": build["generated_anchor"],
            "source_and_header_library_hashes_before": build["input_hashes_before"],
            "source_and_header_library_hashes_after": build["input_hashes_after"],
            "all_pinned_inputs_unchanged": build["inputs_unchanged"],
            "private_header_prediction_or_native_callbacks_prerequisites": False,
        },
        "toolchain": {
            "id": "msvc5_rtm", "root": str(TOOLCHAIN),
            "compiler": str(COMPILER), "compiler_sha256": sha(COMPILER),
            "linker": str(LINKER), "linker_sha256": sha(LINKER),
            "manifest": str(TOOLCHAIN_MANIFEST.resolve()), "manifest_sha256": sha(TOOLCHAIN_MANIFEST),
            "candidate_flags": ["/Gy", "/O2", "/MT", "/GX"],
            "archive_hashes": build["archive_hashes"],
        },
        "build": {key: value for key, value in build.items() if key != "global_addresses_raw"},
        "differential": diff,
        "claims_excluded": [
            "No original source identity or original translation-unit ownership claim.",
            "No MFC class-wide equivalence or real CCmdUI::Enable implementation claim.",
            "No whole EXE, linker layout, initialized game state or window behavior claim.",
            "No FUNCTION_MATCH claim unless a separate strict gate observes complete raw equality and no relocations.",
        ],
    }
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"status={report['status']} cases={diff['case_count']} checks={diff['virtual_enable_event_count']} failures={diff['failure_count']}")
    for row in build["source_contributions"]:
        print(f"{row['callback']} raw_equal={row['raw_function_equal']} native={row['native_extent_bytes']} candidate={row['candidate_coff_bytes']} raw_differences={row['raw_byte_difference_count']} relocations={len(row['candidate_relocations'])}")
    print(f"report={REPORT}")
    print(f"report_sha256={sha(REPORT)}")
    print(f"fixture_dll_sha256={build['dll_sha256']}")
    if diff["status"] != "PASS":
        raise RuntimeError("bounded source/original differential failed: " + json.dumps(diff["failures"]))


if __name__ == "__main__":
    main()
