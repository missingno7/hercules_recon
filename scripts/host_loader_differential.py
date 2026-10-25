"""Independent 15-case source/original differential for the two host loader units.

Uses canonical src/ sources, freshly compiled as separate stock-VC5 objects.
Only retail function bytes are interpreted; OS APIs, CRT services, fatal and
longjmp are synthetic callbacks/boundaries. No game process or real DLL is run.
"""
from __future__ import annotations

import argparse
import datetime
import json
import os
import struct
import subprocess
import sys
import tempfile
import traceback
from pathlib import Path

SCRIPT = Path(__file__).resolve()
REPO = SCRIPT.parents[1]
WORK = REPO / "work/host_loader_differential"
SOURCE_DIR = REPO / "src/pc"
LOADER_SOURCE = SOURCE_DIR / "host_engine_loader.c"
SELECT_SOURCE = SOURCE_DIR / "host_engine_select.c"
HEADER_SOURCE = SOURCE_DIR / "host_engine_cells.h"
FIXTURE_NAME = "loader_cells_fixture.c"
PRIMARY = Path(r"D:\Prog\hercules_recon")
ORIGINAL = PRIMARY / "work/discs/pc_install/HERCULES.EXE"
DEFAULT_OUTPUT = WORK / "canonical_source_differential.json"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
PYTHON_TOOLS = Path(r"C:\tools\hercules\python")
UNICORN_TOOLS = Path(r"C:\tools\hercules\renderer-oracle-python")
SCRIPT_TOOLS = REPO / "scripts"
sys.path[:0] = [str(SCRIPT_TOOLS), str(PYTHON_TOOLS), str(UNICORN_TOOLS)]

from coff import COFF
from match import compare as raw_compare, digest, verify_toolchain
from masked import masked_compare
from pe import PE
import pefile
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP,
    UC_X86_REG_ESI, UC_X86_REG_ESP,
)

IMAGE_BASE = 0x400000
SELECT_VA = IMAGE_BASE + 0x5CD0
SELECT_SIZE = 0x140
RESOLVE_VA = IMAGE_BASE + 0x5C20
RESOLVE_SIZE = 0xB0
PATH_VA = 0x91BBD8
ERROR_VA = 0x91C040
ENTRY_VA = 0x91C45C
GATE_VA = 0x91C460
SELECTOR_VA = 0x91C464
MODULE_VA = 0x91C468
JUMPBUF_VA = 0x91BFD8
SPRINTF_VA = IMAGE_BASE + 0x10F40
SPLITPATH_VA = IMAGE_BASE + 0x11420
LONGJMP_VA = IMAGE_BASE + 0x115A0
STRICMP_VA = IMAGE_BASE + 0x1C270
FATAL_VA = IMAGE_BASE + 0x10460
IAT_FREE = 0xBA58A4
IAT_LOAD = 0xBA589C
IAT_GET = 0xBA58AC
FREE_STUB = 0x6000010
LOAD_STUB = 0x6000110
GET_STUB = 0x6000210
STOP_VA = 0x2000FF0
STACK_BASE = 0x2000000
STACK_TOP = STACK_BASE + 0x8000
SELECT_CAND_VA = 0x7000000
RESOLVE_CAND_VA = 0x7001000
STATIC_BASE = 0x7200000
COMMON_BASE = 0x7300000
ORIGINAL_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"

COMMON_SYMBOLS = {
    "_g_host_engine_path_91bbd8": "path",
    "_g_host_engine_error_91c040": "error",
    "_g_host_engine_entry_91c45c": "entry",
    "_g_host_engine_gate_91c460": "gate",
    "_g_host_engine_selector_91c464": "selector",
    "_g_host_engine_module_91c468": "module",
    "_g_host_engine_jump_buffer_91bfd8": "jump_buffer",
}
EXTERNALS = {
    "__imp__FreeLibrary@4": IAT_FREE,
    "__imp__LoadLibraryA@4": IAT_LOAD,
    "__imp__GetProcAddress@8": IAT_GET,
    "_sprintf": SPRINTF_VA,
    "__splitpath": SPLITPATH_VA,
    "__stricmp": STRICMP_VA,
    "_longjmp": LONGJMP_VA,
    "_archive_fatal": FATAL_VA,
}

# Inserted from the prior measured harness as plain source, not imported at run time.
def u32(uc, address):
    return struct.unpack("<I", uc.mem_read(address, 4))[0]


def put_u32(uc, address, value):
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def read_c_string(uc, address, limit=4096):
    result = bytearray()
    for offset in range(limit):
        value = uc.mem_read(address + offset, 1)[0]
        if value == 0:
            return result.decode("ascii", "replace")
        result.append(value)
    raise AssertionError(f"unterminated string at {address:#x}")


def write_c_string(uc, address, value):
    data = value.encode("ascii") + b"\0"
    uc.mem_write(address, data)


def dos_splitpath(path):
    drive = path[:2] if len(path) >= 2 and path[1] == ":" else ""
    rest = path[len(drive):]
    split = max(rest.rfind("\\"), rest.rfind("/"))
    directory = rest[:split + 1] if split >= 0 else ""
    base = rest[split + 1:] if split >= 0 else rest
    dot = base.rfind(".")
    stem = base[:dot] if dot >= 0 else base
    extension = base[dot:] if dot >= 0 else ""
    return drive, directory, stem, extension




def make_emulator(kind, original_code, layout):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    for base, size in (
        (IMAGE_BASE + 0x5000, 0x1000),
        (IMAGE_BASE + 0x10000, 0x2000),
        (IMAGE_BASE + 0x1C000, 0x1000),
        (IMAGE_BASE + 0x36000, 0x1000),
        (0x6000000, 0x1000),
        (0x7000000, 0x3000),
        (0x7200000, 0x20000),
        (0x7300000, 0x10000),
        (0x91B000, 0x2000),
        (0xBA5000, 0x1000),
        (STACK_BASE, 0x10000),
    ):
        uc.mem_map(base, size)

    if kind == "original":
        uc.mem_write(RESOLVE_VA, original_code["resolve"])
        uc.mem_write(SELECT_VA, original_code["select"])
        cells = {"path": PATH_VA, "error": ERROR_VA, "entry": ENTRY_VA,
                 "gate": GATE_VA, "selector": SELECTOR_VA,
                 "module": MODULE_VA, "jump_buffer": JUMPBUF_VA}
    else:
        _, code, data, cells, _, *extra = layout
        for base, content in code:
            uc.mem_write(base, content)
        for base, content, size, name in data:
            if content:
                uc.mem_write(base, content)
            elif size:
                uc.mem_write(base, b"\0" * size)

    pe = PE(ORIGINAL)
    uc.mem_write(IMAGE_BASE + 0x36000, pe.read_rva(0x36000, 0x1000))
    for address in (SPRINTF_VA, SPLITPATH_VA, LONGJMP_VA, STRICMP_VA,
                    FATAL_VA, FREE_STUB, LOAD_STUB, GET_STUB):
        uc.mem_write(address, b"\xc3")
    put_u32(uc, IAT_FREE, FREE_STUB)
    put_u32(uc, IAT_LOAD, LOAD_STUB)
    put_u32(uc, IAT_GET, GET_STUB)
    return uc, cells


def invoke(kind, entry_name, scenario, original_code, layout):
    uc, cells = make_emulator(kind, original_code, layout)
    path, error = cells["path"], cells["error"]
    gate, selector = cells["gate"], cells["selector"]
    module, export, jump_buffer = cells["module"], cells["entry"], cells["jump_buffer"]
    uc.mem_write(path, b"\0" + b"\xa5" * 0x1ff)
    uc.mem_write(error, b"\0" + b"\xa5" * 0x1ff)
    put_u32(uc, gate, scenario.get("initial_gate", 0x55555555))
    put_u32(uc, selector, scenario.get("selector", 0))
    put_u32(uc, module, scenario.get("old_module", 0))
    put_u32(uc, export, scenario.get("initial_entry", 0x33334444))
    uc.mem_write(jump_buffer, b"\xA7" * 0x40)
    if entry_name == "resolve":
        write_c_string(uc, path, scenario["path"])

    if scenario.get("prefix") is None:
        prefix_pointer = 0
    else:
        prefix_pointer = 0x2002000
        write_c_string(uc, prefix_pointer, scenario["prefix"])
    if scenario.get("suffix") is None:
        suffix_pointer = 0
    else:
        suffix_pointer = 0x2002100
        write_c_string(uc, suffix_pointer, scenario["suffix"])

    entry_esp = STACK_TOP - 0x20
    put_u32(uc, entry_esp, STOP_VA)
    if entry_name == "select":
        put_u32(uc, entry_esp + 4, prefix_pointer)
        put_u32(uc, entry_esp + 8, suffix_pointer)
        pc = SELECT_VA if kind == "original" else SELECT_CAND_VA
    else:
        pc = RESOLVE_VA if kind == "original" else RESOLVE_CAND_VA
    for reg, value in (
        (UC_X86_REG_EAX, 0x11111111), (UC_X86_REG_EBX, 0x22222222),
        (UC_X86_REG_ECX, 0x33333333), (UC_X86_REG_EDX, 0x44444444),
        (UC_X86_REG_ESI, 0x55555555), (UC_X86_REG_EDI, 0x66666666),
        (UC_X86_REG_EBP, 0x77777777), (UC_X86_REG_EFLAGS, 0x202),
        (UC_X86_REG_ESP, entry_esp), (UC_X86_REG_EIP, pc),
    ):
        uc.reg_write(reg, value)

    events = []
    callback_addresses = {SPRINTF_VA, SPLITPATH_VA, STRICMP_VA,
                          FREE_STUB, LOAD_STUB, GET_STUB}
    stop_resolver = scenario.get("stop_at_resolver", False)
    resolve_pc = RESOLVE_VA if kind == "original" else RESOLVE_CAND_VA
    hit = []

    def on_code(engine, address, _size, _user):
        if address in callback_addresses or address in (FATAL_VA, LONGJMP_VA) or (
                stop_resolver and address == resolve_pc):
            hit.append(address)
            engine.emu_stop()

    uc.hook_add(UC_HOOK_CODE, on_code)
    boundary = None
    fatal_argument = None
    for _ in range(100):
        hit.clear()
        current = uc.reg_read(UC_X86_REG_EIP)
        if stop_resolver and current == resolve_pc:
            boundary = "resolver_entry"
            events.append({"api": "resolver_boundary", "path": read_c_string(uc, path),
                           "gate": u32(uc, gate)})
            break
        uc.emu_start(current, STOP_VA, count=100000)
        current = uc.reg_read(UC_X86_REG_EIP)
        if current == STOP_VA:
            boundary = "return"
            break
        if current == FATAL_VA:
            boundary = "fatal_entry"
            esp = uc.reg_read(UC_X86_REG_ESP)
            fatal_argument = u32(uc, esp + 4)
            events.append({"api": "archive_fatal_boundary",
                           "message": read_c_string(uc, fatal_argument),
                           "argument_is_error_cell": fatal_argument == error,
                           "state": _state(uc, cells)})
            break
        if current == LONGJMP_VA:
            boundary = "longjmp_entry"
            esp = uc.reg_read(UC_X86_REG_ESP)
            env, value = u32(uc, esp + 4), u32(uc, esp + 8)
            events.append({"api": "longjmp_boundary", "env_is_jump_buffer": env == jump_buffer,
                           "value": value, "state": _state(uc, cells)})
            break
        if stop_resolver and current == resolve_pc:
            boundary = "resolver_entry"
            events.append({"api": "resolver_boundary", "path": read_c_string(uc, path),
                           "gate": u32(uc, gate)})
            break
        if current not in callback_addresses:
            raise AssertionError(f"unexpected stop at {current:#x} ({kind}/{entry_name})")
        _callback(uc, current, cells, events, scenario)
    else:
        raise AssertionError("callback loop exhausted")

    esp = uc.reg_read(UC_X86_REG_ESP)
    if boundary == "return" and esp != entry_esp + (4 if entry_name in ("select", "resolve") else 0):
        # Both routines are cdecl; caller arguments remain on the stack. Direct
        # resolve has no arguments and both return to the same sentinel.
        expected = entry_esp + 4 + (8 if entry_name == "select" else 0)
        if esp != expected:
            raise AssertionError(f"unexpected caller stack at return: {esp:#x} != {expected:#x}")
    return {
        "boundary": boundary,
        "events": events,
        "final": {**_state(uc, cells), "esp_delta": esp - entry_esp},
        "fatal_argument_is_error_cell": fatal_argument == error if fatal_argument is not None else None,
    }


def _state(uc, cells):
    return {
        "path": read_c_string(uc, cells["path"]),
        "error": read_c_string(uc, cells["error"]),
        "gate": u32(uc, cells["gate"]),
        "selector": u32(uc, cells["selector"]),
        "module": u32(uc, cells["module"]),
        "entry": u32(uc, cells["entry"]),
    }


def _callback(uc, address, cells, events, scenario):
    esp = uc.reg_read(UC_X86_REG_ESP)
    return_pc = u32(uc, esp)
    path, error = cells["path"], cells["error"]
    state_before = _state(uc, cells)
    if address == SPRINTF_VA:
        destination, fmt_ptr = u32(uc, esp + 4), u32(uc, esp + 8)
        fmt = read_c_string(uc, fmt_ptr)
        first = read_c_string(uc, u32(uc, esp + 12))
        if fmt == "%s%s":
            second = read_c_string(uc, u32(uc, esp + 16))
            args, output = [first, second], first + second
        elif fmt == '"%s"':
            args, output = [first], '"' + first + '"'
        else:
            raise AssertionError(f"unexpected sprintf format: {fmt!r}")
        write_c_string(uc, destination, output)
        events.append({"api": "sprintf", "format": fmt, "args": args,
                       "output": output, "state_before": state_before})
        uc.reg_write(UC_X86_REG_EAX, len(output))
        pop = 4
    elif address == SPLITPATH_VA:
        source = read_c_string(uc, u32(uc, esp + 4))
        outputs = [u32(uc, esp + 8 + i * 4) for i in range(4)]
        drive, directory, stem, extension = dos_splitpath(source)
        for pointer, value in zip(outputs, (drive, directory, stem, extension)):
            if pointer:
                write_c_string(uc, pointer, value)
        events.append({"api": "_splitpath", "path": source, "stem": stem,
                       "null_outputs": [pointer == 0 for pointer in outputs],
                       "state_before": state_before})
        uc.reg_write(UC_X86_REG_EAX, 0)
        pop = 4
    elif address == STRICMP_VA:
        left, right = read_c_string(uc, u32(uc, esp + 4)), read_c_string(uc, u32(uc, esp + 8))
        a, b = left.lower(), right.lower()
        result = (a > b) - (a < b)
        events.append({"api": "_stricmp", "left": left, "right": right,
                       "result": result, "state_before": state_before})
        uc.reg_write(UC_X86_REG_EAX, result)
        pop = 4
    elif address == FREE_STUB:
        module_argument = u32(uc, esp + 4)
        events.append({"api": "FreeLibrary", "module_arg": module_argument,
                       "state_before": state_before})
        if scenario.get("free_mutates_path") is not None:
            write_c_string(uc, path, scenario["free_mutates_path"])
        if scenario.get("free_mutates_module") is not None:
            put_u32(uc, cells["module"], scenario["free_mutates_module"])
        uc.reg_write(UC_X86_REG_EAX, 1)
        pop = 8
    elif address == LOAD_STUB:
        argument = u32(uc, esp + 4)
        basename = read_c_string(uc, argument)
        events.append({"api": "LoadLibraryA", "basename": basename,
                       "state_before": state_before})
        if scenario.get("load_mutates_path") is not None:
            write_c_string(uc, path, scenario["load_mutates_path"])
        uc.reg_write(UC_X86_REG_EAX, scenario.get("loaded_module", 0x77770000))
        pop = 8
    elif address == GET_STUB:
        module_argument = u32(uc, esp + 4)
        name = read_c_string(uc, u32(uc, esp + 8))
        events.append({"api": "GetProcAddress", "module_arg": module_argument,
                       "name": name, "state_before": state_before})
        if scenario.get("get_mutates_path") is not None:
            write_c_string(uc, path, scenario["get_mutates_path"])
        if scenario.get("get_mutates_module") is not None:
            put_u32(uc, cells["module"], scenario["get_mutates_module"])
        uc.reg_write(UC_X86_REG_EAX, scenario.get("resolved_entry", 0x88880000))
        pop = 12
    else:
        raise AssertionError(f"unexpected callback {address:#x}")
    uc.reg_write(UC_X86_REG_ESP, esp + pop)
    uc.reg_write(UC_X86_REG_EIP, return_pc)


def scenario_matrix():
    return [
        ("title-fallback-success", "select", dict(prefix="C:\\Games\\TI", suffix="TLE.DLL",
            selector=0, old_module=0x11112222, loaded_module=0x77770000, resolved_entry=0x88880000,
            expected_path="m:\\pc\\title\\release\\title.dll", expected_boundary="return")),
        ("engine1-fallback-success", "select", dict(prefix="C:\\Games\\", suffix="eNgInE1.DLL",
            selector=0, old_module=0, loaded_module=0x77770000, resolved_entry=0x88880000,
            expected_path="m:\\pc\\eng1\\release\\eng1.dll", expected_boundary="return")),
        ("engine3-fallback-success", "select", dict(prefix="C:\\Games\\", suffix="ENGINE3.dll",
            selector=0, old_module=0, loaded_module=0x77770000, resolved_entry=0x88880000,
            expected_path="m:\\pc\\eng3\\release\\eng3.dll", expected_boundary="return")),
        ("unknown-filename-success", "select", dict(prefix="C:\\Mods\\", suffix="custom.dll",
            selector=0, old_module=0, loaded_module=0x77770000, resolved_entry=0x88880000,
            expected_path="C:\\Mods\\custom.dll", expected_boundary="return")),
        ("free-callback-live-path-reload", "select", dict(prefix="C:\\Mods\\", suffix="old.dll",
            selector=0, old_module=0x12345678, free_mutates_path="D:\\Late\\current.dll",
            free_mutates_module=0xDEADBEEF, loaded_module=0x33330000, resolved_entry=0x44440000,
            expected_path="D:\\Late\\current.dll", expected_basename="current.dll",
            expected_boundary="return")),
        ("load-failure-preserves-old-export", "resolve", dict(path="C:\\Missing\\missing.dll",
            old_module=0x11110000, initial_entry=0xAABBCCDD, loaded_module=0,
            expected_path="C:\\Missing\\missing.dll", expected_error='"C:\\Missing\\missing.dll"',
            expected_boundary="fatal_entry")),
        ("null-export-overwrites-old-export", "resolve", dict(path="C:\\Bad\\bad.dll",
            old_module=0x11110000, initial_entry=0xAABBCCDD, loaded_module=0x22220000, resolved_entry=0,
            expected_path="C:\\Bad\\bad.dll", expected_entry=0,
            expected_error='"C:\\Bad\\bad.dll"', expected_boundary="fatal_entry")),
        ("getproc-callback-live-path-reload", "resolve", dict(path="C:\\Initial\\good.dll",
            old_module=0, initial_entry=0xAABBCCDD, loaded_module=0x22220000, resolved_entry=0,
            get_mutates_path="E:\\Changed\\after-get.dll", expected_path="E:\\Changed\\after-get.dll",
            expected_error='"E:\\Changed\\after-get.dll"', expected_boundary="fatal_entry")),
        ("selector-transfer-after-engine-selection", "select", dict(prefix="C:\\Games\\",
            suffix="ENGINE3.dll", selector=0x80000000, expected_path="m:\\pc\\eng3\\release\\eng3.dll",
            expected_boundary="longjmp_entry")),
        ("null-prefix-transfer", "select", dict(prefix=None, suffix="title.dll", selector=1,
            expected_path="", expected_boundary="longjmp_entry")),
        ("null-suffix-transfer", "select", dict(prefix="C:\\Games\\", suffix=None, selector=1,
            expected_path="", expected_boundary="longjmp_entry")),
        ("both-null-transfer", "select", dict(prefix=None, suffix=None, selector=1,
            expected_path="", expected_boundary="longjmp_entry")),
        ("both-null-resolver-precondition-boundary", "select", dict(prefix=None, suffix=None,
            selector=0, expected_path="", expected_boundary="resolver_entry",
            stop_at_resolver=True)),
        ("no-separator-selector-boundary", "select", dict(prefix="plain", suffix=".dll",
            selector=1, expected_path="plain.dll", expected_boundary="longjmp_entry",
            no_separator=True)),
        ("no-separator-resolver-precondition-boundary", "select", dict(prefix="plain", suffix=".dll",
            selector=0, expected_path="plain.dll", expected_boundary="resolver_entry",
            stop_at_resolver=True, no_separator=True)),
    ]





def link_separate_objects(object_paths):
    coffs = [COFF(path) for path in object_paths]
    code_symbols = {"_host_engine_resolve": RESOLVE_CAND_VA,
                    "_host_engine_select": SELECT_CAND_VA}
    definitions = {}
    code_rows = []
    section_bases = []
    addresses = {}
    next_static = STATIC_BASE
    for object_index, (coff, expected_symbol) in enumerate(zip(coffs[:2],
            ("_host_engine_resolve", "_host_engine_select"))):
        functions = [symbol for symbol in coff.symbols.values()
                     if symbol["name"] == expected_symbol and symbol["section"] > 0]
        if len(functions) != 1:
            raise AssertionError(f"expected one {expected_symbol} definition")
        function = functions[0]
        section = coff.sections[function["section"] - 1]
        if not section["flags"] & 0x20 or function["value"] != 0:
            raise AssertionError(f"unexpected independent /Gy contribution: {expected_symbol}")
        definitions[expected_symbol] = {"object_index": object_index,
                                        "section": function["section"],
                                        "size": section["size"]}

    for object_index, coff in enumerate(coffs):
        bases = {}
        for symbol in coff.symbols.values():
            name = symbol["name"]
            if name in code_symbols and symbol["section"] > 0:
                if name in addresses:
                    raise AssertionError(f"duplicate function definition: {name}")
                addresses[name] = code_symbols[name] + symbol["value"]
                bases[symbol["section"]] = code_symbols[name]
                code_rows.append((object_index, symbol["section"],
                                  code_symbols[name], coff.sections[symbol["section"] - 1]))
        for number, section in enumerate(coff.sections, 1):
            if number in bases or not section["size"]:
                continue
            if section["name"] == ".drectve" or section["name"].startswith(".debug"):
                continue
            if section["flags"] & 0x20:
                raise AssertionError(f"unexpected executable section {object_index}/{section['name']}")
            bases[number] = next_static
            next_static += max(0x1000, (section["size"] + 0xFFF) & ~0xFFF)
        section_bases.append(bases)

    next_common = COMMON_BASE
    common_addresses = {}
    common_sizes = {}
    for symbol in coffs[2].symbols.values():
        if symbol["storage"] != 2 or symbol["section"] != 0 or symbol["value"] <= 0:
            continue
        if symbol["name"] not in COMMON_SYMBOLS:
            raise AssertionError(f"unrecognized test fixture common: {symbol['name']}")
        next_common = (next_common + 3) & ~3
        common_addresses[symbol["name"]] = next_common
        common_sizes[symbol["name"]] = symbol["value"]
        next_common += symbol["value"]
    if set(common_addresses) != set(COMMON_SYMBOLS):
        raise AssertionError(f"fixture cell provider is incomplete: {sorted(common_addresses)}")
    addresses.update(common_addresses)

    for object_index, coff in enumerate(coffs):
        bases = section_bases[object_index]
        for symbol in coff.symbols.values():
            if symbol["storage"] != 2 or symbol["section"] <= 0:
                continue
            if symbol["section"] not in bases:
                continue
            address = bases[symbol["section"]] + symbol["value"]
            if symbol["name"] in addresses and addresses[symbol["name"]] != address:
                raise AssertionError(f"duplicate source definition: {symbol['name']}")
            addresses[symbol["name"]] = address

    symbol_addresses = dict(EXTERNALS)
    symbol_addresses.update(addresses)
    linked_code, data_sections = [], []
    for object_index, coff in enumerate(coffs):
        code_section_numbers = {number for oi, number, _, _ in code_rows if oi == object_index}
        for oi, number, base, section in code_rows:
            if oi != object_index:
                continue
            patched = bytearray(section["data"])
            for reloc in section["relocations"]:
                offset, kind, name = reloc["offset"], reloc["type"], reloc["symbol"]
                if name not in symbol_addresses:
                    raise AssertionError(f"unbound source relocation: {reloc}")
                target = symbol_addresses[name]
                addend = struct.unpack_from("<i", patched, offset)[0]
                if kind == 0x0006:
                    value = target + addend
                elif kind == 0x0014:
                    value = target + addend - (base + offset + 4)
                else:
                    raise AssertionError(f"unsupported source relocation: {reloc}")
                struct.pack_into("<I", patched, offset, value & 0xFFFFFFFF)
            linked_code.append((base, bytes(patched)))
        for number, section in enumerate(coff.sections, 1):
            if number in section_bases[object_index] and number not in code_section_numbers:
                data_sections.append((section_bases[object_index][number], section["data"],
                                      section["size"], section["name"]))

    cells = {label: common_addresses[symbol] for symbol, label in COMMON_SYMBOLS.items()}
    return (coffs, common_sizes), linked_code, data_sections, cells, symbol_addresses, definitions


def validate_case(name, scenario, observed):
    if observed["boundary"] != scenario["expected_boundary"]:
        raise AssertionError(f"unexpected boundary in {name}: {observed['boundary']}")
    if observed["final"]["path"] != scenario["expected_path"]:
        raise AssertionError(f"unexpected path in {name}: {observed['final']['path']!r}")
    if "expected_error" in scenario and observed["final"]["error"] != scenario["expected_error"]:
        raise AssertionError(f"unexpected error text in {name}")
    api_order = [event["api"] for event in observed["events"]]
    if scenario["expected_boundary"] == "return":
        loader_order = [api for api in api_order if api in
                        ("FreeLibrary", "LoadLibraryA", "GetProcAddress")]
        expected = (["FreeLibrary"] if scenario.get("old_module", 0) else []) + [
            "LoadLibraryA", "GetProcAddress"]
        if loader_order != expected:
            raise AssertionError(f"loader callback order changed in {name}: {loader_order}")
    if name == "load-failure-preserves-old-export":
        expected = ["FreeLibrary", "LoadLibraryA", "sprintf", "archive_fatal_boundary"]
        if api_order != expected or observed["final"]["entry"] != 0xAABBCCDD or observed["final"]["module"] != 0:
            raise AssertionError(f"failed-load state/order changed in {name}: {api_order}")
    if name == "null-export-overwrites-old-export":
        expected = ["FreeLibrary", "LoadLibraryA", "GetProcAddress", "sprintf",
                    "archive_fatal_boundary"]
        if api_order != expected or observed["final"]["entry"] != 0:
            raise AssertionError(f"null-export state/order changed in {name}: {api_order}")
    if observed["boundary"] == "fatal_entry" and not observed["fatal_argument_is_error_cell"]:
        raise AssertionError(f"fatal argument changed in {name}")
    if name == "free-callback-live-path-reload":
        load = next(event for event in observed["events"] if event["api"] == "LoadLibraryA")
        if load["basename"] != scenario["expected_basename"]:
            raise AssertionError("FreeLibrary mutation was not observed by basename scan")
    if name.startswith(("selector-transfer", "null-prefix", "null-suffix",
                        "both-null-transfer", "no-separator-selector")):
        jump = observed["events"][-1]
        if jump["api"] != "longjmp_boundary" or not jump["env_is_jump_buffer"] or jump["value"] != 0:
            raise AssertionError(f"longjmp boundary changed in {name}")
        if jump["state"]["gate"] != 1:
            raise AssertionError(f"gate write ordering changed in {name}")
    if scenario.get("no_separator") and "LoadLibraryA" in api_order:
        raise AssertionError(f"no-separator input crossed resolver boundary in {name}")
    if name.startswith(("null-prefix", "null-suffix", "both-null")) and any(
            api in api_order for api in ("sprintf", "_splitpath", "_stricmp")):
        raise AssertionError(f"null-input guard changed in {name}")


def dependency_contract(primary, source_dir):
    archive_fatal = source_dir / "archive_fatal.c"
    library_map = primary / "work/library_map/library_map.json"
    archive = TOOLCHAIN / "lib/libcmt.lib"
    result = {
        "functions": {
            "host_engine_loader.c": "void __cdecl host_engine_resolve(void)",
            "host_engine_select.c": "void __cdecl host_engine_select(const char *, const char *)",
            "cross_unit": "selector retains external _host_engine_resolve REL32 reference; separately compiled translation units",
        },
        "win32_imports": ["FreeLibrary BOOL WINAPI(HMODULE)",
                           "LoadLibraryA HMODULE WINAPI(LPCSTR)",
                           "GetProcAddress FARPROC WINAPI(HMODULE,LPCSTR)"],
        "synthetic_stop_boundaries": {
            "archive_fatal": "loaded VA 0x4010460; stop at entry, do not call provider body",
            "longjmp": "loaded VA 0x4115a0; stop at entry and record cdecl env/value"},
        "crt_calls": ["sprintf RVA 0x10f40", "_splitpath RVA 0x11420",
                      "_stricmp RVA 0x1c270", "longjmp RVA 0x115a0"],
        "state_cells": {
            "path": "VA 0x91bbd8, mutable NUL string, capacity unknown",
            "error": "VA 0x91c040, formatted string, capacity unknown",
            "export": "VA 0x91c45c, 4-byte FARPROC view",
            "gate": "VA 0x91c460, 4-byte DWORD",
            "selector": "VA 0x91c464, 4-byte DWORD",
            "module": "VA 0x91c468, 4-byte HMODULE view",
            "jump_buffer": "VA 0x91bfd8; pinned x86 jmp_buf is 64 bytes, original owner/runtime contents unknown"},
        "fixture_only_storage": "generated fixture has 512-byte path/error arrays and a 64-byte jmp_buf; sizes are not original storage evidence",
    }
    if archive_fatal.is_file():
        result["archive_fatal_provider"] = {
            "path": str(archive_fatal), "sha256": digest(archive_fatal.read_bytes()),
            "symbol": "_archive_fatal", "prototype": "void __cdecl archive_fatal(const char *)"}
    if library_map.is_file():
        rows = json.loads(library_map.read_text(encoding="utf-8"))["contributions"]["HERCULES.EXE"]
        result["library_map"] = {
            "path": str(library_map), "sha256": digest(library_map.read_bytes()),
            "archive": str(archive), "archive_sha256": digest(archive.read_bytes()),
            "crt_rows": {rva: next(row for row in rows if row["rva"] == rva)
                         for rva in ("0x10f40", "0x11420", "0x115a0", "0x1c270")}}
    return result


def provenance():
    manifest_path = REPO / "toolchains/manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest_row = next(row for row in manifest["toolchains"] if row["directory"] == TOOLCHAIN.name)
    compiler = next(row for row in manifest_row["files"] if row["path"] == "bin/cl.exe")
    headers = {}
    for name in ("windows.h", "winbase.h", "windef.h", "setjmp.h", "stdio.h", "string.h"):
        path = TOOLCHAIN / "include" / name
        headers[name] = {"path": str(path), "sha256": digest(path.read_bytes())}
    helper_hashes = {}
    for name in ("coff", "env", "match", "masked", "pe"):
        module = sys.modules[name]
        path = Path(module.__file__).resolve()
        if not path.is_relative_to(SCRIPT_TOOLS.resolve()):
            raise RuntimeError(f"helper imported outside tracked scripts/: {path}")
        helper_hashes[name] = {"path": str(path), "sha256": digest(path.read_bytes())}
    py = Path(sys.executable)
    pefile_path = Path(pefile.__file__).resolve()
    unicorn_dll = Path(unicorn.__file__).resolve().parent / "lib/unicorn.dll"
    return {
        "python": {"path": str(py), "version": sys.version.split()[0], "sha256": digest(py.read_bytes())},
        "tracked_helpers": helper_hashes,
        "pefile": {"version": pefile.__version__, "path": str(pefile_path), "sha256": digest(pefile_path.read_bytes())},
        "unicorn": {"version": unicorn.__version__, "path": str(unicorn_dll), "sha256": digest(unicorn_dll.read_bytes())},
        "toolchain": {"id": verify_toolchain(TOOLCHAIN), "root": str(TOOLCHAIN),
            "compiler": compiler, "manifest_sha256": digest(manifest_path.read_bytes()),
            "environment": {"PATH": "pinned VC5 bin prepended", "INCLUDE": str(TOOLCHAIN / "include"),
                            "LIB": str(TOOLCHAIN / "lib"), "CL": "removed", "_CL_": "removed"},
            "headers": headers}}


def compile_unit(source, obj_path, toolchain, include_dir, cwd):
    command = [str(toolchain / "bin/cl.exe"), "/nologo", "/c", "/O2", "/Gy",
               "/I" + str(include_dir), "/Fo" + str(obj_path), str(source)]
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    record = {"source": str(source), "command": command, "cwd": str(cwd)}
    try:
        result = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                                text=True, timeout=120, check=False)
    except OSError as exc:
        record.update({"status": "launch_error", "exception_type": type(exc).__name__,
                       "winerror": getattr(exc, "winerror", None), "errno": exc.errno,
                       "message": str(exc)})
        return record
    except subprocess.TimeoutExpired as exc:
        record.update({"status": "process_timeout", "message": str(exc),
                       "stdout": str(exc.stdout or ""), "stderr": str(exc.stderr or "")})
        return record
    record.update({"returncode": result.returncode,
                   "stdout": result.stdout or "", "stderr": result.stderr or ""})
    if result.returncode != 0:
        record["status"] = "process_failed"
        return record
    if not obj_path.is_file():
        record.update({"status": "process_failed", "reason": "compiler exited zero without object"})
        return record
    record.update({"status": "compiled", "object": str(obj_path),
                   "object_sha256": digest(obj_path.read_bytes()),
                   "object_bytes": obj_path.stat().st_size})
    return record


def unit_diagnostics(unit, target):
    coff = COFF(unit["object"])
    contribution = coff.function(unit["symbol"])
    raw_body = raw_compare(unit["object"], unit["symbol"], target,
                           unit["rva"], unit["body_size"])
    raw_padded = raw_compare(unit["object"], unit["symbol"], target,
                             unit["rva"], unit["padded_size"])
    masked = masked_compare(unit["object"], unit["symbol"], target,
                            unit["rva"], unit["body_size"])
    relocations = []
    for reloc in contribution["relocations"]:
        addend = struct.unpack_from("<i", contribution["data"], reloc["offset"])[0]
        relocations.append({**reloc, "addend_signed": addend,
                            "target_rva_inferred": masked["symbols"].get(reloc["symbol"])})
    noncode = []
    for number, section in enumerate(coff.sections, 1):
        if section["flags"] & 0x20:
            continue
        noncode.append({"index": number, "name": section["name"], "size": section["size"],
                        "flags": hex(section["flags"]), "sha256": digest(section["data"]),
                        "bytes_hex": section["data"].hex(), "relocations": section["relocations"]})
    return {"symbol": unit["symbol"], "rva": hex(unit["rva"]),
            "source": str(unit["source"]), "source_sha256": digest(unit["source"].read_bytes()),
            "object": str(unit["object"]), "object_sha256": digest(unit["object"].read_bytes()),
            "function_size": len(contribution["data"]),
            "function_sha256": digest(contribution["data"]),
            "compiler_command": unit["compile"]["command"],
            "raw_body": raw_body, "raw_padded": raw_padded,
            "relocation_masked_body_diagnostic": masked,
            "function_relocations_with_addends": relocations,
            "noncode_contributions": noncode}


def write_report(path, report):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")


def main(argv=None):
    global ORIGINAL
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-exe", type=Path, default=ORIGINAL,
                        help="read-only pinned retail HERCULES.EXE")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help="JSON receipt under this checkout's ignored work/")
    args = parser.parse_args(argv)
    ORIGINAL = args.native_exe.resolve()
    output = args.output.resolve()
    work_root = (REPO / "work").resolve()
    if not output.is_relative_to(work_root):
        parser.error("--output must be under this checkout's work directory")
    report = {"schema": "host-engine-separate-source-differential-v1",
              "status": "starting", "runner": {"path": str(SCRIPT),
              "sha256": digest(SCRIPT.read_bytes())},
              "arguments": {"native_exe": str(ORIGINAL), "output": str(output)},
              "scope": "two independently compiled canonical host functions plus generated test-only cell provider; unchanged retail bytes interpreted only; synthetic APIs/CRT/fatal/longjmp; no game process or real DLL/service execution"}
    try:
        analysis_provenance = provenance()
        report["analysis_provenance"] = analysis_provenance
        required_sources = (LOADER_SOURCE, SELECT_SOURCE, HEADER_SOURCE)
        missing = [str(path) for path in required_sources if not path.is_file()]
        if missing:
            raise FileNotFoundError("canonical source/header missing: " + ", ".join(missing))
        source_before = {path.name: {"path": str(path), "sha256": digest(path.read_bytes()),
                                    "bytes": path.stat().st_size} for path in required_sources}
        report["sources"] = source_before
        native_hash = digest(ORIGINAL.read_bytes())
        report["oracle"] = {"path": str(ORIGINAL), "sha256": native_hash,
                            "expected_sha256": ORIGINAL_SHA256}
        if native_hash != ORIGINAL_SHA256:
            raise ValueError("--native-exe does not match the pinned retail executable hash")
        pe = PE(ORIGINAL)
        report["target_functions"] = {
            "host_engine_resolve": {"rva": "0x5c20", "body_size": 0xAA, "padded_size": RESOLVE_SIZE,
                "body_sha256": digest(pe.read_rva(0x5C20, 0xAA)),
                "padded_sha256": digest(pe.read_rva(0x5C20, RESOLVE_SIZE))},
            "host_engine_select": {"rva": "0x5cd0", "body_size": 0x13C, "padded_size": SELECT_SIZE,
                "body_sha256": digest(pe.read_rva(0x5CD0, 0x13C)),
                "padded_sha256": digest(pe.read_rva(0x5CD0, SELECT_SIZE))}}
        report["dependency_contract"] = dependency_contract(PRIMARY, SOURCE_DIR)

        build_root = WORK / "canonical_differential_builds"
        build_root.mkdir(parents=True, exist_ok=True)
        prefix = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S_")
        run_dir = Path(tempfile.mkdtemp(prefix=prefix, dir=build_root))
        report["build_directory"] = str(run_dir)
        fixture = run_dir / FIXTURE_NAME
        fixture.write_text(
            '#include "host_engine_cells.h"\n'
            'char g_host_engine_path_91bbd8[512];\n'
            'char g_host_engine_error_91c040[512];\n'
            'FARPROC g_host_engine_entry_91c45c;\n'
            'DWORD g_host_engine_gate_91c460;\n'
            'DWORD g_host_engine_selector_91c464;\n'
            'HMODULE g_host_engine_module_91c468;\n'
            'jmp_buf g_host_engine_jump_buffer_91bfd8;\n',
            encoding="ascii", newline="\n")
        report["fixture_source"] = {"path": str(fixture), "sha256": digest(fixture.read_bytes()),
            "purpose": "synthetic test-only cell provider; arrays are 512 bytes; jmp_buf is 64 bytes; not original storage evidence"}
        units = [
            {"name": "host_engine_loader", "source": LOADER_SOURCE,
             "object": run_dir / "host_engine_loader.obj", "symbol": "_host_engine_resolve",
             "rva": 0x5C20, "body_size": 0xAA, "padded_size": RESOLVE_SIZE},
            {"name": "host_engine_select", "source": SELECT_SOURCE,
             "object": run_dir / "host_engine_select.obj", "symbol": "_host_engine_select",
             "rva": 0x5CD0, "body_size": 0x13C, "padded_size": SELECT_SIZE},
        ]
        compile_records = []
        for unit in units:
            unit["compile"] = compile_unit(unit["source"], unit["object"], TOOLCHAIN,
                                            SOURCE_DIR, REPO)
            compile_records.append(unit["compile"])
            if unit["compile"]["status"] != "compiled":
                not_started = [item["name"] for item in units[len(compile_records):]] + ["fixture"]
                report["compile"] = {"status": "incomplete", "units": compile_records,
                                     "not_attempted": not_started}
                report["status"] = ("compiler_launch_error" if unit["compile"]["status"] == "launch_error"
                                    else unit["compile"]["status"])
                write_report(output, report)
                print(json.dumps({"status": report["status"], "output": str(output),
                                  "failed_unit": unit["name"], "compile": unit["compile"]}, indent=2))
                return 2
        fixture_obj = run_dir / "loader_cells_fixture.obj"
        fixture_compile = compile_unit(fixture, fixture_obj, TOOLCHAIN, SOURCE_DIR, REPO)
        compile_records.append(fixture_compile)
        report["compile"] = {"status": "compiled", "units": compile_records}
        if fixture_compile["status"] != "compiled":
            report["compile"]["status"] = "incomplete"
            report["status"] = ("compiler_launch_error" if fixture_compile["status"] == "launch_error"
                                else fixture_compile["status"])
            write_report(output, report)
            print(json.dumps({"status": report["status"], "output": str(output),
                              "failed_unit": "fixture", "compile": fixture_compile}, indent=2))
            return 2
        source_after = {path.name: digest(path.read_bytes()) for path in required_sources}
        if any(source_after[name] != row["sha256"] for name, row in source_before.items()):
            raise RuntimeError("canonical sources/header changed during compile; discard this run")

        fixture_coff = COFF(fixture_obj)
        fixture_cells = [{"symbol": row["name"], "test_only_bytes": row["value"]}
                         for row in fixture_coff.symbols.values()
                         if row["storage"] == 2 and row["section"] == 0 and row["value"] > 0]
        report["fixture_object"] = {"path": str(fixture_obj),
                                    "sha256": digest(fixture_obj.read_bytes()),
                                    "cell_definitions": fixture_cells}
        reports = [unit_diagnostics(unit, ORIGINAL) for unit in units]
        report["unit_comparisons"] = reports
        original_code = {"resolve": pe.read_rva(0x5C20, RESOLVE_SIZE),
                         "select": pe.read_rva(0x5CD0, SELECT_SIZE)}
        layout = link_separate_objects([units[0]["object"], units[1]["object"], fixture_obj])
        (_, common_sizes), linked_code, data_sections, cells, symbol_addresses, definitions = layout
        report["synthetic_link"] = {
            "source_function_definitions": {name: {
                "address": hex(SELECT_CAND_VA if name == "_host_engine_select" else RESOLVE_CAND_VA), **row}
                for name, row in definitions.items()},
            "cells": {name: hex(address) for name, address in cells.items()},
            "fixture_common_sizes_only": common_sizes,
            "external_and_defined_bindings": {name: hex(value) for name, value in sorted(symbol_addresses.items())
                                               if name.startswith("_") or name.startswith("__imp_")},
            "linked_code_contributions": [{"base": hex(base), "size": len(content)}
                                           for base, content in linked_code],
            "mapped_noncode_sections": [{"base": hex(base), "name": name, "size": size}
                                         for base, _, size, name in data_sections]}
        rows = []
        scenarios = scenario_matrix()
        report["scenario_count"] = len(scenarios)
        for name, entry_name, scenario in scenarios:
            scenario["name"] = name
            original = invoke("original", entry_name, scenario, original_code, layout)
            candidate = invoke("candidate", entry_name, scenario, original_code, layout)
            equal = original == candidate
            rows.append({"case": name, "entry": entry_name, "differential_equal": equal,
                         "expected_boundary": scenario["expected_boundary"],
                         "original": original, "candidate": candidate})
            if not equal:
                report["cases"] = rows
                report["status"] = "differential_mismatch"
                report["failure"] = {"case": name}
                write_report(output, report)
                raise AssertionError(json.dumps({"case": name, "original": original,
                                                 "candidate": candidate}, indent=2))
            validate_case(name, scenario, original)
        report["cases"] = rows
        report["differential_equal"] = all(row["differential_equal"] for row in rows)
        report["status"] = "pass" if report["differential_equal"] else "differential_mismatch"
        report["analysis_runtime"] = {key: analysis_provenance[key]
            for key in ("python", "tracked_helpers", "pefile", "unicorn")}
        report["raw_equal"] = {row["symbol"]: row["raw_body"]["exact"] for row in reports}
        report["relocation_masked_equal_diagnostic"] = {
            row["symbol"]: row["relocation_masked_body_diagnostic"]["masked_equal"] for row in reports}
        write_report(output, report)
        print(json.dumps({"status": report["status"], "cases": len(rows),
                          "raw_equal": report["raw_equal"],
                          "relocation_masked_equal_diagnostic": report["relocation_masked_equal_diagnostic"],
                          "objects": {unit["name"]: unit["compile"].get("object_sha256") for unit in units},
                          "output": str(output)}, indent=2))
        return 0
    except Exception as exc:
        if report.get("status") != "differential_mismatch":
            report["status"] = "error"
        report["error"] = {"type": type(exc).__name__, "message": str(exc),
                           "traceback": traceback.format_exc()}
        write_report(output, report)
        print(json.dumps({"status": report["status"], "output": str(output),
                          "error": report["error"]}, indent=2))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
