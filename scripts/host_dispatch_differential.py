"""Bounded public retail/source differential for the host dispatcher at RVA 0x5ec0.

Freshly compiles canonical dispatcher/loader/selector translation units as
separate stock-VC5 COFF objects. It raw-compares those same objects, links their
actual function contributions for bounded Unicorn emulation, maps unchanged
retail dispatcher/CRT bytes, and stops at synthetic OS/fatal/DLL boundaries.
No game process or real DLL/API is started. Synthetic cells/table inputs do not
establish retail storage ownership or capacities.
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import traceback

SCRIPT = Path(__file__).resolve()
if SCRIPT.parent.name == "scripts":
    REPO = SCRIPT.parent.parent
else:
    REPO = next((parent for parent in SCRIPT.parents
                 if (parent / "scripts").is_dir() and
                 (parent / "calibration").is_dir()), None)
    if REPO is None:
        raise RuntimeError(f"cannot derive repository root from {SCRIPT}")
WORK = REPO / "work/host_dispatch_differential"
SOURCE_DIR = REPO / "calibration"
LOADER_SOURCE = SOURCE_DIR / "host_engine_loader.c"
SELECT_SOURCE = SOURCE_DIR / "host_engine_select.c"
HEADER_SOURCE = SOURCE_DIR / "host_engine_cells.h"
DISPATCH_SOURCE = SOURCE_DIR / "host_engine_dispatch.c"
STORAGE_SOURCE = SOURCE_DIR / "host_engine_storage.c"
ORIGINAL_DEFAULT = Path(r"D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE")
DEFAULT_OUTPUT = WORK / "report.json"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
SCRIPT_TOOLS = REPO / "scripts"
PYTHON_TOOLS = Path(r"C:\tools\hercules\python")
UNICORN_TOOLS = Path(r"C:\tools\hercules\renderer-oracle-python")
sys.path[:0] = [str(SCRIPT_TOOLS), str(PYTHON_TOOLS), str(UNICORN_TOOLS)]
sys.dont_write_bytecode = True


def find_library_map():
    for parent in (REPO, *REPO.parents):
        path = parent / "work/library_map/library_map.json"
        if path.is_file():
            return path.resolve()
    raise FileNotFoundError("could not find an existing work/library_map/library_map.json in repository ancestors")


LIBRARY_MAP = find_library_map()

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
ORIGINAL_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
DISPATCH_VA = IMAGE_BASE + 0x5EC0
RESOLVE_VA = IMAGE_BASE + 0x5C20
SELECT_VA = IMAGE_BASE + 0x5CD0
DISPATCH_BODY_SIZE = 0x54
DISPATCH_PADDED_SIZE = 0x60
RESOLVE_BODY_SIZE = 0xAA
RESOLVE_PADDED_SIZE = 0xB0
SELECT_BODY_SIZE = 0x13C
SELECT_PADDED_SIZE = 0x140

PATH_VA = 0x91BBD8
ERROR_VA = 0x91C040
ENTRY_VA = 0x91C45C
GATE_VA = 0x91C460
SELECTOR_VA = 0x91C464
MODULE_VA = 0x91C468
JUMPBUF_VA = 0x91BFD8
SOURCE_TABLE_POINTER_VA = IMAGE_BASE + 0x365A0
SOURCE_TABLE_VA = IMAGE_BASE + 0x361B8
SOURCE_TABLE_BYTES = 250 * 4
CALL_FLAG_VA = IMAGE_BASE + 0x7A01FC

SPRINTF_VA = IMAGE_BASE + 0x10F40
SPLITPATH_VA = IMAGE_BASE + 0x11420
LONGJMP_VA = IMAGE_BASE + 0x115A0
SETJMP3_VA = IMAGE_BASE + 0x1161C
STRICMP_VA = IMAGE_BASE + 0x1C270
FATAL_VA = IMAGE_BASE + 0x10460
IAT_FREE = 0xBA58A4
IAT_LOAD = 0xBA589C
IAT_GET = 0xBA58AC
FREE_STUB = 0x6000100
LOAD_STUB = 0x6000110
GET_STUB = 0x6000120
STOP_VA = 0x2000FF0
STACK_BASE = 0x2000000
STACK_TOP = STACK_BASE + 0x8000
INITIAL_REGISTERS = {
    "eax": 0x11111111, "ebx": 0x22222222, "ecx": 0x33333333,
    "edx": 0x44444444, "esi": 0x55555555, "edi": 0x66666666,
    "ebp": 0x77777777, "eflags": 0x202,
}
CALLEE_SAVED_REGISTERS = ("ebx", "esi", "edi", "ebp")

NATIVE_DISPATCH_VA = DISPATCH_VA
NATIVE_RESOLVE_VA = RESOLVE_VA
NATIVE_SELECT_VA = SELECT_VA
SOURCE_DISPATCH_VA = 0x7002000
SOURCE_RESOLVE_VA = 0x7001000
SOURCE_SELECT_VA = 0x7000000
FIXTURE_CALLBACK_VA = 0x7003000
STATIC_BASE = 0x7200000
COMMON_BASE = 0x7300000

FIXTURE_SOURCE = r'''#include <string.h>
#include <windows.h>
#include "host_engine_cells.h"

char g_host_engine_path_91bbd8[512];
char g_host_engine_error_91c040[512];
FARPROC g_host_engine_entry_91c45c;
DWORD g_host_engine_gate_91c460;
DWORD g_host_engine_selector_91c464;
HMODULE g_host_engine_module_91c468;
jmp_buf g_host_engine_jump_buffer_91bfd8;

extern void *g_host_engine_source_table_cell_365a0;
extern BYTE g_host_engine_call_flag_7a01fc;
extern void __cdecl host_engine_select(const char *, const char *);

DWORD g_dispatch_mode;
DWORD g_dispatch_count;
void *g_dispatch_last_table;
int g_dispatch_last_flag;
DWORD g_dispatch_mutation_gate;
DWORD g_dispatch_mutation_selector;
void *g_dispatch_mutation_table_pointer;
DWORD g_dispatch_mutation_table_word0;

void __cdecl dispatch_test_engine_main(void *table, int flag)
{
    ++g_dispatch_count;
    g_dispatch_last_table = table;
    g_dispatch_last_flag = flag;

    if (g_dispatch_mode == 1 && g_dispatch_count == 1) {
        g_host_engine_gate_91c460 = 0x13579bdf;
        g_host_engine_selector_91c464 = 0x2468ace0;
        strcpy(g_host_engine_path_91bbd8, "C:\\callback\\updated.dll");
        g_host_engine_source_table_cell_365a0 = (char *)table + 4;
        ((DWORD *)table)[0] = 0xdec0ad01;
        g_dispatch_mutation_gate = g_host_engine_gate_91c460;
        g_dispatch_mutation_selector = g_host_engine_selector_91c464;
        g_dispatch_mutation_table_pointer = g_host_engine_source_table_cell_365a0;
        g_dispatch_mutation_table_word0 = ((DWORD *)table)[0];
        return;
    }

    if (g_dispatch_mode == 2 && g_dispatch_count == 1) {
        g_host_engine_selector_91c464 = 0;
        host_engine_select("D:\\Nested\\", "custom.dll");
        return;
    }

    if (g_dispatch_mode == 3 && g_dispatch_count == 1) {
        host_engine_select("D:\\Switch\\", "TITLE.DLL");
        return;
    }

    if (g_dispatch_mode == 4 && g_dispatch_count == 1) {
        g_host_engine_selector_91c464 = 0;
        host_engine_select(0, 0);
        return;
    }

    if (g_dispatch_mode == 5 && g_dispatch_count == 1) {
        host_engine_select(0, 0);
        return;
    }
}
'''

def u32(uc, address):
    return struct.unpack("<I", uc.mem_read(address, 4))[0]


def put_u32(uc, address, value):
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def read_c_string(uc, address, limit=2048):
    result = bytearray()
    for offset in range(limit):
        value = uc.mem_read(address + offset, 1)[0]
        if value == 0:
            return result.decode("ascii", "replace")
        result.append(value)
    raise AssertionError(f"unterminated string at {address:#x}")


def write_c_string(uc, address, value):
    encoded = value.encode("ascii") + b"\0"
    uc.mem_write(address, encoded)


def archive_members(data):
    if data[:8] != b"!<arch>\n":
        raise ValueError("pinned libcmt.lib is not a COFF archive")
    position = 8
    index = 0
    while position < len(data):
        header = data[position:position + 60]
        if len(header) != 60 or header[58:60] != b"`\n":
            raise ValueError(f"malformed COFF archive member {index}")
        size = int(header[48:58])
        start = position + 60
        payload = data[start:start + size]
        if len(payload) != size:
            raise ValueError(f"truncated COFF archive member {index}")
        yield index, payload
        position = start + size + (size & 1)
        index += 1


def crt_archive_contract(pe, build_dir):
    archive_path = TOOLCHAIN / "lib/libcmt.lib"
    archive_data = archive_path.read_bytes()
    archive_sha = digest(archive_data)
    expected_archive_sha = "7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b"
    if archive_sha != expected_archive_sha:
        raise ValueError("pinned VC5 libcmt.lib archive hash changed")

    library_map_data = LIBRARY_MAP.read_bytes()
    library_map = json.loads(library_map_data)
    contributions = library_map["contributions"]["HERCULES.EXE"]
    expected = {
        "__setjmp3": {"rva": "0x1161c", "size": 123, "member": 259,
                      "relocations": 1},
        "_longjmp": {"rva": "0x115a0", "size": 121, "member": 256,
                     "relocations": 5},
    }
    expected_rows = {}
    for symbol, spec in expected.items():
        matches = [row for row in contributions if row["rva"] == spec["rva"]]
        if len(matches) != 1:
            raise ValueError(f"library map has {len(matches)} rows for {symbol} at {spec['rva']}")
        row = matches[0]
        if (row.get("library") != "lib/libcmt.lib" or
                row.get("member") != spec["member"] or
                row.get("size") != spec["size"] or
                row.get("relocations") != spec["relocations"] or
                symbol not in row.get("names", [])):
            raise ValueError(f"unexpected existing library-map identity for {symbol}: {row}")
        expected_rows[symbol] = row

    member_payloads = dict(archive_members(archive_data))
    member_rows = {}
    for symbol, spec in expected.items():
        payload = member_payloads[spec["member"]]
        object_path = build_dir / f"libcmt_member_{spec['member']}.obj"
        object_path.write_bytes(payload)
        coff = COFF(object_path)
        definitions = [row for row in coff.symbols.values()
                       if row["name"] == symbol and row["section"] > 0 and
                       row["storage"] == 2 and row["type"] & 0x20]
        if len(definitions) != 1 or definitions[0]["value"] != 0:
            raise ValueError(f"archive member does not uniquely define {symbol} at section offset zero")
        section_number = definitions[0]["section"]
        section = coff.sections[section_number - 1]
        if (not section["flags"] & 0x20 or section["size"] != spec["size"] or
                len(section["relocations"]) != spec["relocations"]):
            raise ValueError(f"unexpected archive code contribution for {symbol}")
        member_rows[symbol] = {
            "library_map_row": expected_rows[symbol],
            "member_index": spec["member"],
            "member_sha256": digest(payload),
            "member_bytes": len(payload),
            "contribution_section": section["name"],
            "contribution_bytes": section["size"],
            "contribution_sha256": digest(section["data"]),
            "relocations": section["relocations"],
            "original_target_rva": spec["rva"],
            "original_target_sha256": digest(pe.read_rva(int(spec["rva"], 0), spec["size"])),
        }
    return {
        "library_map": {"path": str(LIBRARY_MAP), "sha256": digest(library_map_data)},
        "archive": {"path": str(archive_path), "sha256": archive_sha},
        "members": member_rows,
        "execution": "mapped unchanged original executable CRT bytes; archive members are identification evidence only",
        "unsupported_exception_domain": "only FS:[0]=0 was exercised; nonempty exception-registration chains, SEH unwind behavior, and any associated CRT notification path are outside this finite harness",
    }


def dos_splitpath(path):
    drive = path[:2] if len(path) >= 2 and path[1] == ":" else ""
    rest = path[len(drive):]
    slash = max(rest.rfind("\\"), rest.rfind("/"))
    directory = rest[:slash + 1] if slash >= 0 else ""
    base = rest[slash + 1:] if slash >= 0 else rest
    dot = base.rfind(".")
    stem = base[:dot] if dot >= 0 else base
    extension = base[dot:] if dot >= 0 else ""
    return drive, directory, stem, extension


def fixture_source(path, native=False):
    text = FIXTURE_SOURCE
    if not native:
        for declaration in ('FARPROC g_host_engine_entry_91c45c;',
                            'DWORD g_host_engine_gate_91c460;',
                            'DWORD g_host_engine_selector_91c464;',
                            'HMODULE g_host_engine_module_91c468;',
                            'jmp_buf g_host_engine_jump_buffer_91bfd8;'):
            if text.count(declaration) != 1:
                raise RuntimeError('fixture declaration shape changed')
            text = text.replace(declaration + '\n', '')
    path.write_text(text, encoding="ascii", newline="\n")
    return path


def compile_unit(source, obj_path, include_dir, cwd):
    command = [str(TOOLCHAIN / "bin/cl.exe"), "/nologo", "/c", "/O2", "/Gy",
               "/I" + str(include_dir), "/I" + str(TOOLCHAIN / "include"),
               "/Fo" + str(obj_path), str(source)]
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(TOOLCHAIN / "include")
    env["LIB"] = str(TOOLCHAIN / "lib")
    row = {"source": str(source), "command": command, "cwd": str(cwd)}
    try:
        result = subprocess.run(command, cwd=cwd, env=env, capture_output=True,
                                text=True, timeout=120, check=False)
    except OSError as exc:
        row.update({"status": "launch_error", "exception_type": type(exc).__name__,
                    "winerror": getattr(exc, "winerror", None), "errno": exc.errno,
                    "message": str(exc)})
        return row
    except subprocess.TimeoutExpired as exc:
        row.update({"status": "process_timeout", "message": str(exc),
                    "stdout": str(exc.stdout or ""), "stderr": str(exc.stderr or "")})
        return row
    row.update({"returncode": result.returncode, "stdout": result.stdout or "",
                "stderr": result.stderr or ""})
    if result.returncode != 0 or not obj_path.is_file():
        row["status"] = "process_failed"
        return row
    row.update({"status": "compiled", "object": str(obj_path),
                "object_sha256": digest(obj_path.read_bytes()),
                "object_bytes": obj_path.stat().st_size})
    return row


def link_objects(object_paths, variant):
    coffs = [COFF(path) for path in object_paths]
    function_va = {
        "_host_engine_resolve": SOURCE_RESOLVE_VA,
        "_host_engine_select": SOURCE_SELECT_VA,
        "_host_engine_dispatch_5ec0": SOURCE_DISPATCH_VA,
        "_dispatch_test_engine_main": FIXTURE_CALLBACK_VA,
    }
    if variant == "original_fixture":
        function_va["_host_engine_select"] = NATIVE_SELECT_VA
    definitions = {}
    section_bases = []
    symbol_addresses = {}
    code_rows = []
    data_rows = []
    next_data = STATIC_BASE

    for object_index, coff in enumerate(coffs):
        bases = {}
        for symbol in coff.symbols.values():
            if symbol["section"] <= 0 or not symbol["type"] & 0x20:
                continue
            name = symbol["name"]
            if name not in function_va:
                continue
            if name in definitions:
                raise AssertionError(f"duplicate source function definition: {name}")
            section_number = symbol["section"]
            section = coff.sections[section_number - 1]
            if not section["flags"] & 0x20 or symbol["value"] != 0:
                raise AssertionError(f"unexpected /Gy code contribution for {name}")
            address = function_va[name]
            bases[section_number] = address
            symbol_addresses[name] = address
            code_rows.append((object_index, section_number, address, section))
            definitions[name] = {"object_index": object_index,
                                 "section": section_number,
                                 "size": section["size"], "address": address}

        # Place every remaining initialized/uninitialized section independently.
        for number, section in enumerate(coff.sections, 1):
            if number in bases or not section["size"]:
                continue
            if section["name"] == ".drectve" or section["name"].startswith(".debug"):
                continue
            if section["flags"] & 0x20:
                raise AssertionError(f"unmapped executable COFF section {section['name']}")
            next_data = (next_data + 0xFFF) & ~0xFFF
            bases[number] = next_data
            next_data += max(0x1000, (section["size"] + 0xFFF) & ~0xFFF)
            data_rows.append((object_index, number, bases[number], section["data"],
                              section["size"], section["name"]))
        section_bases.append(bases)

    if variant != "original_fixture":
        expected = {"_host_engine_resolve", "_host_engine_select",
                    "_host_engine_dispatch_5ec0", "_dispatch_test_engine_main"}
    else:
        expected = {"_dispatch_test_engine_main"}
    if not expected.issubset(definitions):
        raise AssertionError(f"missing function contributions: {sorted(expected - set(definitions))}")

    # VC5 emits zero-initialized external fixture cells as COFF common symbols.
    next_common = COMMON_BASE
    common_sizes = {}
    common_names = set()
    if variant == "original_fixture":
        # The unchanged native instructions address these cells by fixed VA.
        # Their mapped contents/capacities remain synthetic test fixtures; this
        # binding does not assert retail C storage ownership.
        symbol_addresses.update({
            "_g_host_engine_path_91bbd8": PATH_VA,
            "_g_host_engine_error_91c040": ERROR_VA,
            "_g_host_engine_entry_91c45c": ENTRY_VA,
            "_g_host_engine_gate_91c460": GATE_VA,
            "_g_host_engine_selector_91c464": SELECTOR_VA,
            "_g_host_engine_module_91c468": MODULE_VA,
            "_g_host_engine_jump_buffer_91bfd8": JUMPBUF_VA,
        })
    for coff in coffs:
        for symbol in coff.symbols.values():
            if symbol["storage"] != 2 or symbol["section"] != 0 or symbol["value"] <= 0:
                continue
            name = symbol["name"]
            if name in symbol_addresses or name in common_names:
                continue
            next_common = (next_common + 3) & ~3
            symbol_addresses[name] = next_common
            common_sizes[name] = symbol["value"]
            common_names.add(name)
            next_common += symbol["value"]

    for object_index, coff in enumerate(coffs):
        bases = section_bases[object_index]
        for symbol in coff.symbols.values():
            if symbol["storage"] != 2 or symbol["section"] <= 0:
                continue
            if symbol["section"] not in bases:
                continue
            address = bases[symbol["section"]] + symbol["value"]
            old = symbol_addresses.get(symbol["name"])
            if old is not None and old != address:
                raise AssertionError(f"duplicate linked source symbol {symbol['name']}")
            symbol_addresses[symbol["name"]] = address

    original_bindings = {
        "__imp__FreeLibrary@4": IAT_FREE,
        "__imp__LoadLibraryA@4": IAT_LOAD,
        "__imp__GetProcAddress@8": IAT_GET,
        "_sprintf": SPRINTF_VA,
        "__splitpath": SPLITPATH_VA,
        "__stricmp": STRICMP_VA,
        "_longjmp": LONGJMP_VA,
        "__setjmp3": SETJMP3_VA,
        "_archive_fatal": FATAL_VA,
        "_g_host_engine_source_table_cell_365a0": SOURCE_TABLE_POINTER_VA,
        "_g_host_engine_call_flag_7a01fc": CALL_FLAG_VA,
        "_host_engine_select": NATIVE_SELECT_VA,
    }
    if variant != "original_fixture":
        original_bindings.pop("_host_engine_select")
    symbol_addresses = {**original_bindings, **symbol_addresses}

    linked_code = []
    for object_index, coff in enumerate(coffs):
        for oi, number, base, section in code_rows:
            if oi != object_index:
                continue
            patched = bytearray(section["data"])
            for reloc in section["relocations"]:
                name, offset, kind = reloc["symbol"], reloc["offset"], reloc["type"]
                if name not in symbol_addresses:
                    raise AssertionError(f"unbound source relocation: {reloc}")
                target = symbol_addresses[name]
                addend = struct.unpack_from("<i", patched, offset)[0]
                if kind == 0x0006:
                    value = target + addend
                elif kind == 0x0014:
                    value = target + addend - (base + offset + 4)
                else:
                    raise AssertionError(f"unsupported relocation: {reloc}")
                struct.pack_into("<I", patched, offset, value & 0xFFFFFFFF)
            linked_code.append((base, bytes(patched)))

    # Ordinary data-section relocations are needed for fixture literals/cells.
    # The fixture's C sections only contain addresses in initialized `.data`;
    # keep this path explicit and reject unknown relocation kinds.
    for object_index, section_number, base, content, size, name in data_rows:
        if not content:
            continue
        coff = coffs[object_index]
        section = coff.sections[section_number - 1]
        if not section["relocations"]:
            continue
        patched = bytearray(content)
        for reloc in section["relocations"]:
            symbol = reloc["symbol"]
            if symbol not in symbol_addresses:
                raise AssertionError(f"unbound data relocation: {reloc}")
            addend = struct.unpack_from("<i", patched, reloc["offset"])[0]
            if reloc["type"] != 0x0006:
                raise AssertionError(f"unsupported data relocation: {reloc}")
            struct.pack_into("<I", patched, reloc["offset"],
                             (symbol_addresses[symbol] + addend) & 0xFFFFFFFF)
        data_rows = [(oi, sn, b,
                      bytes(patched) if oi == object_index and sn == section_number else c,
                      s, n) for oi, sn, b, c, s, n in data_rows]

    cells = {
        "path": symbol_addresses["_g_host_engine_path_91bbd8"],
        "error": symbol_addresses["_g_host_engine_error_91c040"],
        "entry": symbol_addresses["_g_host_engine_entry_91c45c"],
        "gate": symbol_addresses["_g_host_engine_gate_91c460"],
        "selector": symbol_addresses["_g_host_engine_selector_91c464"],
        "module": symbol_addresses["_g_host_engine_module_91c468"],
        "jump_buffer": symbol_addresses["_g_host_engine_jump_buffer_91bfd8"],
        "mode": symbol_addresses["_g_dispatch_mode"],
        "count": symbol_addresses["_g_dispatch_count"],
        "last_table": symbol_addresses["_g_dispatch_last_table"],
        "last_flag": symbol_addresses["_g_dispatch_last_flag"],
        "mutation_gate": symbol_addresses["_g_dispatch_mutation_gate"],
        "mutation_selector": symbol_addresses["_g_dispatch_mutation_selector"],
        "mutation_table_pointer": symbol_addresses["_g_dispatch_mutation_table_pointer"],
        "mutation_table_word0": symbol_addresses["_g_dispatch_mutation_table_word0"],
    }
    return (coffs, common_sizes), linked_code, data_rows, cells, symbol_addresses, definitions


def source_diagnostics(obj, source, target, symbol, rva, body_size, padded_size):
    contribution = COFF(obj).function(symbol)
    relocations = []
    for reloc in contribution["relocations"]:
        addend = struct.unpack_from("<i", contribution["data"], reloc["offset"])[0]
        relocations.append({**reloc, "addend_signed": addend})
    noncode = []
    coff = COFF(obj)
    for number, section in enumerate(coff.sections, 1):
        if section["flags"] & 0x20:
            continue
        noncode.append({"index": number, "name": section["name"],
                        "bytes": section["size"], "flags": hex(section["flags"]),
                        "sha256": digest(section["data"]),
                        "relocations": section["relocations"]})
    return {
        "symbol": symbol, "rva": hex(rva),
        "source": str(source), "source_sha256": digest(source.read_bytes()),
        "object": str(obj), "object_sha256": digest(obj.read_bytes()),
        "compiled_function_bytes": len(contribution["data"]),
        "compiled_function_sha256": digest(contribution["data"]),
        "raw_body": raw_compare(obj, symbol, target, rva, body_size),
        "raw_padded": raw_compare(obj, symbol, target, rva, padded_size),
        "relocation_masked_body_diagnostic": masked_compare(
            obj, symbol, target, rva, body_size),
        "relocation_masked_padded_diagnostic": masked_compare(
            obj, symbol, target, rva, padded_size),
        "function_relocations": relocations,
        "noncode_contributions": noncode,
    }


def _state(uc, cells):
    return {
        "path": read_c_string(uc, cells["path"]),
        "error": read_c_string(uc, cells["error"]),
        "gate": u32(uc, cells["gate"]),
        "selector": u32(uc, cells["selector"]),
        "module": u32(uc, cells["module"]),
        "entry": u32(uc, cells["entry"]),
        "table_pointer_cell": u32(uc, SOURCE_TABLE_POINTER_VA),
        "table_word0": u32(uc, SOURCE_TABLE_VA),
    }


def _api_callback(uc, address, cells, events, scenario):
    esp = uc.reg_read(UC_X86_REG_ESP)
    return_pc = u32(uc, esp)
    if address == SPRINTF_VA:
        destination, fmt_ptr = u32(uc, esp + 4), u32(uc, esp + 8)
        fmt = read_c_string(uc, fmt_ptr)
        first = read_c_string(uc, u32(uc, esp + 12))
        if fmt == "%s%s":
            output = first + read_c_string(uc, u32(uc, esp + 16))
            args = [first, read_c_string(uc, u32(uc, esp + 16))]
        elif fmt == '"%s"':
            output, args = '"' + first + '"', [first]
        else:
            raise AssertionError(f"unexpected sprintf format {fmt!r}")
        write_c_string(uc, destination, output)
        events.append({"api": "sprintf", "format": fmt, "args": args,
                       "output": output, "state_before": _state(uc, cells)})
        uc.reg_write(UC_X86_REG_EAX, len(output))
    elif address == SPLITPATH_VA:
        path = read_c_string(uc, u32(uc, esp + 4))
        outputs = [u32(uc, esp + 8 + n * 4) for n in range(4)]
        _drive, _directory, stem, _extension = dos_splitpath(path)
        if outputs[2]:
            write_c_string(uc, outputs[2], stem)
        events.append({"api": "_splitpath", "path": path, "filename": stem,
                       "null_outputs": [not pointer for pointer in outputs]})
        uc.reg_write(UC_X86_REG_EAX, 0)
    elif address == STRICMP_VA:
        left, right = (read_c_string(uc, u32(uc, esp + 4)),
                       read_c_string(uc, u32(uc, esp + 8)))
        a, b = left.lower(), right.lower()
        result = (a > b) - (a < b)
        events.append({"api": "_stricmp", "left": left, "right": right,
                       "result": result})
        uc.reg_write(UC_X86_REG_EAX, result)
    elif address == FREE_STUB:
        module = u32(uc, esp + 4)
        events.append({"api": "FreeLibrary", "module": module})
        if scenario.get("free_mutates_path") is not None:
            write_c_string(uc, cells["path"], scenario["free_mutates_path"])
        if scenario.get("free_mutates_module") is not None:
            put_u32(uc, cells["module"], scenario["free_mutates_module"])
        uc.reg_write(UC_X86_REG_EAX, 1)
    elif address == LOAD_STUB:
        basename = read_c_string(uc, u32(uc, esp + 4))
        events.append({"api": "LoadLibraryA", "basename": basename,
                       "state_before": _state(uc, cells)})
        if scenario.get("load_mutates_path") is not None:
            write_c_string(uc, cells["path"], scenario["load_mutates_path"])
        uc.reg_write(UC_X86_REG_EAX, scenario.get("loaded_module", 0x77770000))
    elif address == GET_STUB:
        module = u32(uc, esp + 4)
        export_name = read_c_string(uc, u32(uc, esp + 8))
        events.append({"api": "GetProcAddress", "module": module,
                       "name": export_name, "state_before": _state(uc, cells)})
        uc.reg_write(UC_X86_REG_EAX,
                     scenario.get("resolved_entry", FIXTURE_CALLBACK_VA))
    else:
        raise AssertionError(f"unexpected ordinary API/CRT boundary {address:#x}")

    # The synthetic API boundary is cdecl/stdcall as appropriate. Pop the
    # return PC here; callee-cleanup arguments are removed for Win32 imports.
    pop_bytes = 4
    if address in (FREE_STUB, LOAD_STUB):
        pop_bytes += 4
    elif address == GET_STUB:
        pop_bytes += 8
    uc.reg_write(UC_X86_REG_ESP, esp + pop_bytes)
    uc.reg_write(UC_X86_REG_EIP, return_pc)


def scenario_matrix():
    callback = FIXTURE_CALLBACK_VA
    return [
        ("initial-setjmp-normal-export", dict(mode=0, gate=0, selector=0,
            module=0, entry=callback, path="C:\\Initial\\idle.dll",
            call_flag_byte=0xA5)),
        ("zero-export-skips-callback", dict(mode=0, gate=0, selector=0,
            module=0, entry=0, path="C:\\Initial\\idle.dll")),
        ("entry-gate-loads-before-export", dict(mode=0, gate=1, selector=0,
            module=0, entry=callback, path="C:\\Initial\\before.dll")),
        ("export-callback-mutates-live-cells", dict(mode=1, gate=0, selector=0,
            module=0, entry=callback, path="C:\\Initial\\before.dll",
            expected_gate=0x13579BDF, expected_selector=0,
            expected_path="C:\\callback\\updated.dll",
            expected_callback_selector=0x2468ACE0,
            expected_table_pointer=SOURCE_TABLE_VA + 4,
            expected_table_word0=0xDEC0AD01)),
        ("export-callback-selector-zero-normal-path", dict(mode=2, gate=0,
            selector=0, module=0, entry=callback, path="C:\\Initial\\before.dll",
            loaded_module=0x77770000, resolved_entry=callback)),
        ("export-callback-selector-transfer-and-reentry", dict(mode=3,
            gate=0, selector=0, module=0, entry=callback,
            path="C:\\Initial\\before.dll", loaded_module=0x77770000,
            resolved_entry=callback, expected_path="m:\\pc\\title\\release\\title.dll",
            expected_callback_count=2)),
        ("null-selector-zero-stops-at-resolver", dict(mode=4, gate=0,
            selector=0, module=0, entry=callback, path="C:\\Initial\\before.dll",
            stop_at_resolver=True, expected_path="", expected_resolver_gate=0)),
        ("null-selector-transfer-stops-at-resolver", dict(mode=5,
            gate=0, selector=0, module=0, entry=callback,
            path="C:\\Initial\\before.dll", stop_at_resolver=True,
            expected_path="", expected_resolver_gate=1)),
        ("fatal-load-failure-boundary", dict(mode=0, gate=1, selector=0,
            module=0, entry=callback, path="C:\\Missing\\missing.dll",
            loaded_module=0, stop_at_fatal=True)),
    ]


def make_emulator(kind, pe, layout, scenario):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    for base, size in (
        (0, 0x1000),
        (IMAGE_BASE + 0x5000, 0x2000),
        (IMAGE_BASE + 0x10000, 0xD000),
        (IMAGE_BASE + 0x36000, 0x1000),
        (IMAGE_BASE + 0x3A000, 0x1000),
        (0x6000000, 0x1000),
        (0x7000000, 0x10000),
        (STATIC_BASE, 0x100000),
        (COMMON_BASE, 0x10000),
        (0x91B000, 0x2000),
        (0xBA0000, 0x1000),
        (0xBA5000, 0x1000),
        (STACK_BASE, 0x10000),
    ):
        uc.mem_map(base, size)

    # Original-function execution uses file-backed retail bytes at their
    # preferred image VAs. The fixture table/call flag are initialized retail
    # data; the test-only path/error/jmp_buf cells remain separate allocations.
    uc.mem_write(IMAGE_BASE + 0x5000, pe.read_rva(0x5000, 0x2000))
    uc.mem_write(IMAGE_BASE + 0x10000, pe.read_rva(0x10000, 0xD000))
    uc.mem_write(IMAGE_BASE + 0x36000, pe.read_rva(0x36000, 0x1000))
    uc.mem_write(IMAGE_BASE + 0x3A000, pe.read_rva(0x3A000, 0x1000))
    try:
        uc.mem_write(0xBA5000, pe.read_rva(0x7A5000, 0x1000))
    except ValueError:
        pass

    if kind == "source":
        _, linked_code, data_rows, cells, symbols, _definitions = layout
        for base, content in linked_code:
            uc.mem_write(base, content)
        for _object_index, _section, base, content, size, _name in data_rows:
            uc.mem_write(base, content + b"\0" * max(0, size - len(content)))
        dispatch_pc = SOURCE_DISPATCH_VA
        resolver_pc = SOURCE_RESOLVE_VA
        selector_pc = SOURCE_SELECT_VA
    else:
        _, linked_code, data_rows, cells, symbols, _definitions = layout
        for base, content in linked_code:
            uc.mem_write(base, content)
        for _object_index, _section, base, content, size, _name in data_rows:
            uc.mem_write(base, content + b"\0" * max(0, size - len(content)))
        dispatch_pc = NATIVE_DISPATCH_VA
        resolver_pc = NATIVE_RESOLVE_VA
        selector_pc = NATIVE_SELECT_VA

    # Map imported API slots to explicit synthetic boundaries. All CRT
    # setjmp3/longjmp code remains the unchanged original archive contribution.
    put_u32(uc, IAT_FREE, FREE_STUB)
    put_u32(uc, IAT_LOAD, LOAD_STUB)
    put_u32(uc, IAT_GET, GET_STUB)

    cells = {**cells, "dispatch_pc": dispatch_pc,
             "resolver_pc": resolver_pc, "selector_pc": selector_pc,
             "dispatch_callback": symbols.get("_dispatch_test_engine_main", FIXTURE_CALLBACK_VA)}
    return uc, cells


def invoke(kind, scenario, pe, layout):
    uc, cells = make_emulator(kind, pe, layout, scenario)
    uc.mem_write(cells["path"], b"\0" * 512)
    uc.mem_write(cells["error"], b"\0" * 512)
    put_u32(uc, cells["gate"], scenario["gate"])
    put_u32(uc, cells["selector"], scenario["selector"])
    put_u32(uc, cells["module"], scenario["module"])
    put_u32(uc, cells["entry"], scenario["entry"])
    uc.mem_write(cells["jump_buffer"], b"\xA7" * 64)
    put_u32(uc, cells["mode"], scenario["mode"])
    put_u32(uc, cells["count"], 0)
    put_u32(uc, cells["last_table"], 0)
    put_u32(uc, cells["last_flag"], 0)
    for label in ("mutation_gate", "mutation_selector", "mutation_table_pointer",
                  "mutation_table_word0"):
        put_u32(uc, cells[label], 0)
    uc.mem_write(CALL_FLAG_VA, bytes((scenario.get("call_flag_byte", 0),)))
    write_c_string(uc, cells["path"], scenario["path"])
    initial_table_pointer = u32(uc, SOURCE_TABLE_POINTER_VA)
    initial_table_word = u32(uc, SOURCE_TABLE_VA)
    initial_call_flag = uc.mem_read(CALL_FLAG_VA, 1)[0]

    stack_entry = STACK_TOP - 0x40
    put_u32(uc, stack_entry, STOP_VA)
    registers = (
        (UC_X86_REG_EAX, INITIAL_REGISTERS["eax"]),
        (UC_X86_REG_EBX, INITIAL_REGISTERS["ebx"]),
        (UC_X86_REG_ECX, INITIAL_REGISTERS["ecx"]),
        (UC_X86_REG_EDX, INITIAL_REGISTERS["edx"]),
        (UC_X86_REG_ESI, INITIAL_REGISTERS["esi"]),
        (UC_X86_REG_EDI, INITIAL_REGISTERS["edi"]),
        (UC_X86_REG_EBP, INITIAL_REGISTERS["ebp"]),
        (UC_X86_REG_EFLAGS, INITIAL_REGISTERS["eflags"]),
        (UC_X86_REG_ESP, stack_entry), (UC_X86_REG_EIP, cells["dispatch_pc"]),
    )
    for register, value in registers:
        uc.reg_write(register, value)

    events = []
    pending = []
    watched_apis = {SPRINTF_VA, SPLITPATH_VA, STRICMP_VA,
                    FREE_STUB, LOAD_STUB, GET_STUB, FATAL_VA}
    stop_at_resolver = scenario.get("stop_at_resolver", False)
    stop_at_fatal = scenario.get("stop_at_fatal", False)
    resolver_boundary = []

    def on_code(engine, address, _size, _user):
        if address == SETJMP3_VA:
            esp = engine.reg_read(UC_X86_REG_ESP)
            env = u32(engine, esp + 4)
            events.append({"api": "__setjmp3_entry", "env_address": env,
                           "env_is_dispatcher_jump_buffer": env == cells["jump_buffer"],
                           "extra_count": u32(engine, esp + 8),
                           "body_sha256": digest(pe.read_rva(0x1161C, 123))})
        elif address == LONGJMP_VA:
            esp = engine.reg_read(UC_X86_REG_ESP)
            env = u32(engine, esp + 4)
            events.append({"api": "_longjmp_entry", "env_address": env,
                           "env_is_dispatcher_jump_buffer": env == cells["jump_buffer"],
                           "value": u32(engine, esp + 8),
                           "body_sha256": digest(pe.read_rva(0x115A0, 121)),
                           "continues_through_original_body": True})
        if address in watched_apis:
            pending.append(address)
            engine.emu_stop()
            return
        if address == cells["dispatch_callback"]:
            esp = engine.reg_read(UC_X86_REG_ESP)
            events.append({"api": "engine_export_entry",
                           "table_arg": u32(engine, esp + 4),
                           "flag_arg": u32(engine, esp + 8),
                           "callback_count_before": u32(engine, cells["count"])})
        if address == cells["selector_pc"]:
            esp = engine.reg_read(UC_X86_REG_ESP)
            prefix_ptr, suffix_ptr = u32(engine, esp + 4), u32(engine, esp + 8)
            events.append({"api": "selector_entry",
                           "prefix": None if prefix_ptr == 0 else read_c_string(engine, prefix_ptr),
                           "suffix": None if suffix_ptr == 0 else read_c_string(engine, suffix_ptr),
                           "selector": u32(engine, cells["selector"])})
        if stop_at_resolver and address == cells["resolver_pc"]:
            resolver_boundary.append(True)
            events.append({"api": "resolver_entry_boundary",
                           "path": read_c_string(engine, cells["path"]),
                           "gate": u32(engine, cells["gate"])})
            engine.emu_stop()
        if stop_at_fatal and address == FATAL_VA:
            pending.append(FATAL_VA)
            engine.emu_stop()

    uc.hook_add(UC_HOOK_CODE, on_code)
    boundary = None
    for _ in range(128):
        pending.clear()
        current = uc.reg_read(UC_X86_REG_EIP)
        uc.emu_start(current, STOP_VA, count=250000)
        current = uc.reg_read(UC_X86_REG_EIP)
        if current == STOP_VA:
            boundary = "return"
            break
        if resolver_boundary:
            boundary = "resolver_entry"
            break
        if current == FATAL_VA:
            esp = uc.reg_read(UC_X86_REG_ESP)
            message_pointer = u32(uc, esp + 4)
            events.append({"api": "fatal_boundary",
                           "message": read_c_string(uc, message_pointer),
                           "message_pointer_is_error": message_pointer == cells["error"]})
            boundary = "fatal_entry"
            break
        if current in pending:
            if current == FATAL_VA:
                esp = uc.reg_read(UC_X86_REG_ESP)
                pointer = u32(uc, esp + 4)
                events.append({"api": "fatal_boundary",
                               "message": read_c_string(uc, pointer),
                               "message_pointer_is_error": pointer == cells["error"]})
                boundary = "fatal_entry"
                break
            _api_callback(uc, current, cells, events, scenario)
            continue
        raise AssertionError(f"unexpected execution stop {current:#x} ({kind})")
    else:
        raise AssertionError("dispatcher/API loop exceeded bound")

    final_state = _state(uc, cells)
    esp = uc.reg_read(UC_X86_REG_ESP)
    saved_registers = {
        "ebx": uc.reg_read(UC_X86_REG_EBX),
        "esi": uc.reg_read(UC_X86_REG_ESI),
        "edi": uc.reg_read(UC_X86_REG_EDI),
        "ebp": uc.reg_read(UC_X86_REG_EBP),
    }
    final = {
        **final_state,
        "callback_count": u32(uc, cells["count"]),
        "callback_last_table": u32(uc, cells["last_table"]),
        "callback_last_flag": u32(uc, cells["last_flag"]),
        "callback_mutation_gate": u32(uc, cells["mutation_gate"]),
        "callback_mutation_selector": u32(uc, cells["mutation_selector"]),
        "callback_mutation_table_pointer": u32(uc, cells["mutation_table_pointer"]),
        "callback_mutation_table_word0": u32(uc, cells["mutation_table_word0"]),
        "initial_table_pointer": initial_table_pointer,
        "initial_table_word0": initial_table_word,
        "call_flag_byte": initial_call_flag,
        "return_stack_delta": (esp - stack_entry) if boundary == "return" else None,
        "callee_saved_registers": saved_registers if boundary == "return" else None,
    }
    return {"boundary": boundary, "events": events, "final": final,
            "boundary_registers": saved_registers,
            "sdk_unwind_data_tail_unchanged": bytes(uc.mem_read(cells["jump_buffer"] + 40, 24)) == bytes([0xA7]) * 24}


def validate_case(name, scenario, observed):
    if not observed["sdk_unwind_data_tail_unchanged"]:
        raise AssertionError("extra-count0 modified SDK optional unwind-data slots")
    if observed["boundary"] == "return":
        if observed["final"]["return_stack_delta"] != 4:
            raise AssertionError("returning dispatcher path did not preserve cdecl stack balance")
        for register in CALLEE_SAVED_REGISTERS:
            if observed["final"]["callee_saved_registers"][register] != INITIAL_REGISTERS[register]:
                raise AssertionError(f"returning dispatcher path did not preserve {register.upper()}")
    if name == "initial-setjmp-normal-export":
        entries = [e for e in observed["events"] if e["api"] == "engine_export_entry"]
        if len(entries) != 1 or entries[0]["table_arg"] != observed["final"]["initial_table_pointer"]:
            raise AssertionError("normal export did not receive the live retail table pointer")
        if entries[0]["flag_arg"] != observed["final"]["call_flag_byte"]:
            raise AssertionError("normal export did not receive the retail zero-extended status byte")
        if observed["final"]["selector"] != 0:
            raise AssertionError("normal dispatch did not clear selector on return")
        setjmps = [e for e in observed["events"] if e["api"] == "__setjmp3_entry"]
        if (len(setjmps) != 1 or setjmps[0]["extra_count"] != 0 or
                not setjmps[0]["env_is_dispatcher_jump_buffer"]):
            raise AssertionError("initial setjmp did not use the mapped VC5 __setjmp3(env, 0) path")
    elif name == "zero-export-skips-callback":
        if any(e["api"] == "engine_export_entry" for e in observed["events"]):
            raise AssertionError("zero export unexpectedly invoked the synthetic engine")
        if observed["final"]["selector"] != 0:
            raise AssertionError("zero-export dispatch did not clear selector")
    elif name == "entry-gate-loads-before-export":
        apis = [e["api"] for e in observed["events"]]
        if not (apis.index("LoadLibraryA") < apis.index("GetProcAddress") <
                apis.index("engine_export_entry")):
            raise AssertionError("entry gate/load/export ordering changed")
    elif name == "export-callback-mutates-live-cells":
        final = observed["final"]
        expected = {"gate": scenario["expected_gate"],
                    "selector": scenario["expected_selector"],
                    "path": scenario["expected_path"],
                    "table_pointer_cell": scenario["expected_table_pointer"],
                    "table_word0": scenario["expected_table_word0"],
                    "callback_mutation_gate": scenario["expected_gate"],
                    "callback_mutation_selector": scenario["expected_callback_selector"],
                    "callback_mutation_table_pointer": scenario["expected_table_pointer"],
                    "callback_mutation_table_word0": scenario["expected_table_word0"]}
        for key, value in expected.items():
            if final[key] != value:
                raise AssertionError(f"callback mutation mismatch {key}: {final[key]!r} != {value!r}")
    elif name == "export-callback-selector-zero-normal-path":
        apis = [e["api"] for e in observed["events"]]
        if "selector_entry" not in apis or "LoadLibraryA" not in apis or "GetProcAddress" not in apis:
            raise AssertionError("selector-zero branch did not resolve through the loader")
        if observed["final"]["path"] != "D:\\Nested\\custom.dll":
            raise AssertionError("unknown selector filename did not retain its joined path")
    elif name == "export-callback-selector-transfer-and-reentry":
        apis = [e["api"] for e in observed["events"]]
        if apis.count("engine_export_entry") != 2 or "selector_entry" not in apis:
            raise AssertionError("nonlocal transfer did not re-enter at the saved dispatch continuation")
        if observed["final"]["callback_count"] != 2:
            raise AssertionError("callback count does not show first call plus post-longjmp call")
        if observed["final"]["path"] != scenario["expected_path"]:
            raise AssertionError("selector replacement path was not live after longjmp")
        jumps = [e for e in observed["events"] if e["api"] == "_longjmp_entry"]
        if (len(jumps) != 1 or jumps[0]["value"] != 0 or
                not jumps[0]["env_is_dispatcher_jump_buffer"] or
                not jumps[0]["continues_through_original_body"]):
            raise AssertionError("selector did not execute the mapped original CRT longjmp(env, 0)")
    elif name.startswith("null-selector-"):
        if observed["boundary"] != "resolver_entry" or observed["final"]["path"] != "":
            raise AssertionError("null selector input did not stop at the resolver precondition")
        expected_gate = scenario["expected_resolver_gate"]
        if observed["final"]["gate"] != expected_gate:
            raise AssertionError("null selector branch gate did not match observed transfer")
        if name.startswith("null-selector-transfer") and not any(
                e["api"] == "selector_entry" for e in observed["events"]):
            raise AssertionError("nonzero selector null-input test did not enter selector")
        if name.startswith("null-selector-transfer") and not any(
                e["api"] == "_longjmp_entry" and e["value"] == 0 for e in observed["events"]):
            raise AssertionError("nonzero selector null-input test did not execute the CRT longjmp path")
    elif name == "fatal-load-failure-boundary":
        if observed["boundary"] != "fatal_entry" or not observed["events"][-1]["message_pointer_is_error"]:
            raise AssertionError("load failure did not stop at the existing fatal/error-cell boundary")


def provenance():
    manifest_path = REPO / "toolchains/manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest_row = next(row for row in manifest["toolchains"]
                        if row["directory"] == TOOLCHAIN.name)
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
            raise RuntimeError(f"helper imported outside scripts/: {path}")
        helper_hashes[name] = {"path": str(path), "sha256": digest(path.read_bytes())}
    return {
        "python": {"path": sys.executable, "version": sys.version.split()[0],
                   "sha256": digest(Path(sys.executable).read_bytes())},
        "tracked_helpers": helper_hashes,
        "pefile": {"version": pefile.__version__, "path": str(Path(pefile.__file__).resolve())},
        "unicorn": {"version": unicorn.__version__,
                    "path": str(Path(unicorn.__file__).resolve())},
        "toolchain": {"id": verify_toolchain(TOOLCHAIN), "root": str(TOOLCHAIN),
            "compiler": compiler, "manifest_sha256": digest(manifest_path.read_bytes()),
            "environment": {"PATH": "pinned VC5 bin prepended", "INCLUDE": str(TOOLCHAIN / "include"),
                            "LIB": str(TOOLCHAIN / "lib"), "CL": "removed", "_CL_": "removed"},
            "headers": headers}}


def write_report(path, report):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-exe", type=Path, default=ORIGINAL_DEFAULT,
                        help="read-only pinned retail HERCULES.EXE")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help="receipt path under ignored work/host_dispatch_differential/")
    args = parser.parse_args(argv)
    native = args.native_exe.resolve()
    output = (args.output if args.output.is_absolute() else REPO / args.output).resolve()
    if not output.is_relative_to(WORK.resolve()):
        parser.error("--output must be under ignored work/host_dispatch_differential/")

    report = {"schema": "public-host-dispatch-5ec0-differential-v1",
              "status": "starting", "runner": {"path": str(SCRIPT),
              "sha256": digest(SCRIPT.read_bytes())},
              "scope": "unchanged retail dispatcher versus fresh separate stock-VC5 canonical loader/select/dispatcher objects; each exact object is raw-compared then linked/emulated; synthetic callback only; mapped retail CRT setjmp3/longjmp; no game process or real DLL/API calls",
              "arguments": {"native_exe": str(native), "output": str(output)}}
    try:
        report["analysis_provenance"] = provenance()
        required = (LOADER_SOURCE, SELECT_SOURCE, HEADER_SOURCE, DISPATCH_SOURCE, STORAGE_SOURCE)
        missing = [str(p) for p in required if not p.is_file()]
        if missing:
            raise FileNotFoundError("missing source/header: " + ", ".join(missing))
        before = {p.name: {"path": str(p), "sha256": digest(p.read_bytes()),
                           "bytes": p.stat().st_size} for p in required}
        report["source_header_inputs"] = before
        if digest(native.read_bytes()) != ORIGINAL_SHA256:
            raise ValueError("native exe SHA-256 does not match the pinned retail image")
        pe = PE(native)
        report["oracle"] = {"path": str(native), "sha256": digest(native.read_bytes()),
                            "expected_sha256": ORIGINAL_SHA256,
                            "image_base": hex(pe.image_base)}
        report["original_fixture_data"] = {
            "table_rva": "0x361b8", "table_bytes": SOURCE_TABLE_BYTES,
            "table_sha256": digest(pe.read_rva(0x361B8, SOURCE_TABLE_BYTES)),
            "self_pointer_cell_rva": "0x365a0",
            "self_pointer_value": hex(struct.unpack("<I", pe.read_rva(0x365A0, 4))[0]),
            "callback_byte_rva": "0x7a01fc",
            "callback_byte_value": 0,
            "callback_byte_storage": "RVA 0x7a01fc lies in the virtual, non-file-backed tail of .data; this bounded fixture uses a zero-filled initial byte and does not claim its runtime value before dispatcher entry",
            "first_case_synthetic_status_byte_override": 165,
            "ownership": "the host table/self-pointer use exact original initialized bytes; the status byte is virtual zero-fill in this fresh emulator; source/TU owner unresolved; callback writes in mutation scenarios are synthetic fixture changes only"}
        report["target_functions"] = {
            "dispatcher": {"rva": "0x5ec0", "body_bytes": DISPATCH_BODY_SIZE,
                "padded_bytes": DISPATCH_PADDED_SIZE,
                "body_sha256": digest(pe.read_rva(0x5EC0, DISPATCH_BODY_SIZE)),
                "padded_sha256": digest(pe.read_rva(0x5EC0, DISPATCH_PADDED_SIZE))},
            "loader": {"rva": "0x5c20", "body_bytes": RESOLVE_BODY_SIZE,
                "padded_bytes": RESOLVE_PADDED_SIZE,
                "body_sha256": digest(pe.read_rva(0x5C20, RESOLVE_BODY_SIZE)),
                "padded_sha256": digest(pe.read_rva(0x5C20, RESOLVE_PADDED_SIZE))},
            "selector": {"rva": "0x5cd0", "body_bytes": SELECT_BODY_SIZE,
                "padded_bytes": SELECT_PADDED_SIZE,
                "body_sha256": digest(pe.read_rva(0x5CD0, SELECT_BODY_SIZE)),
                "padded_sha256": digest(pe.read_rva(0x5CD0, SELECT_PADDED_SIZE))}}

        build_root = WORK / "builds"
        build_root.mkdir(parents=True, exist_ok=True)
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S_")
        run_dir = Path(tempfile.mkdtemp(prefix=stamp, dir=build_root))
        report["build_directory"] = str(run_dir)
        report["crt_archive_identity"] = crt_archive_contract(pe, run_dir)
        fixture = fixture_source(run_dir / "dispatch_fixture.c")
        native_fixture = fixture_source(run_dir / "native_dispatch_fixture.c", native=True)
        report["fixture_source"] = {"path": str(fixture), "sha256": digest(fixture.read_bytes()),
            "purpose": "Source fixture supplies only512-byte path/error test buffers and a compiled synthetic export; actual loader scalar/jmp_buf source supplies its storage. Native fixture keeps test-only fixed-address views. No path/error capacity, callback behavior or original allocation ownership follows."}
        units = [
            {"name": "host_engine_loader", "source": LOADER_SOURCE,
             "symbol": "_host_engine_resolve", "rva": 0x5C20,
             "body": RESOLVE_BODY_SIZE, "padded": RESOLVE_PADDED_SIZE},
            {"name": "host_engine_select", "source": SELECT_SOURCE,
             "symbol": "_host_engine_select", "rva": 0x5CD0,
             "body": SELECT_BODY_SIZE, "padded": SELECT_PADDED_SIZE},
            {"name": "host_engine_dispatch_5ec0", "source": DISPATCH_SOURCE,
             "symbol": "_host_engine_dispatch_5ec0", "rva": 0x5EC0,
             "body": DISPATCH_BODY_SIZE, "padded": DISPATCH_PADDED_SIZE},
            {"name": "dispatch_fixture", "source": fixture,
             "symbol": "_dispatch_test_engine_main"},
            {"name": "host_engine_storage", "source": STORAGE_SOURCE},
            {"name": "native_dispatch_fixture", "source": native_fixture,
             "symbol": "_dispatch_test_engine_main"},
        ]
        compile_rows = []
        for unit in units:
            unit["object"] = run_dir / (unit["name"] + ".obj")
            unit["compile"] = compile_unit(unit["source"], unit["object"], SOURCE_DIR, REPO)
            compile_rows.append(unit["compile"])
            if unit["compile"]["status"] != "compiled":
                report["compile"] = {"status": "incomplete", "units": compile_rows,
                                     "not_attempted": [u["name"] for u in units[len(compile_rows):]]}
                report["status"] = ("compiler_launch_error" if unit["compile"]["status"] == "launch_error"
                                    else unit["compile"]["status"])
                write_report(output, report)
                print(json.dumps({"status": report["status"], "output": str(output),
                                  "compile": unit["compile"]}, indent=2))
                return 2
        report["compile"] = {"status": "compiled", "units": compile_rows}
        after = {p.name: digest(p.read_bytes()) for p in required}
        if any(after[name] != value["sha256"] for name, value in before.items()):
            raise RuntimeError("a source/header changed while fresh objects were compiled")

        report["object_comparisons"] = [source_diagnostics(
            unit["object"], unit["source"], native, unit["symbol"], unit["rva"],
            unit["body"], unit["padded"]) for unit in units[:3]]
        source_paths = [unit["object"] for unit in units[:5]]
        source_layout = link_objects(source_paths, "source")
        fixture_native_layout = link_objects([units[5]["object"]], "original_fixture")
        report["synthetic_link"] = {
            "separate_source_objects": {unit["name"]: {
                "object_sha256": digest(unit["object"].read_bytes()),
                    "symbol": unit.get("symbol"),
                    "data_only_provider": unit["name"] == "host_engine_storage"} for unit in units[:5]},
            "source_function_addresses": {name: hex(row["address"])
                for name, row in source_layout[5].items()},
            "fixture_common_sizes": source_layout[0][1],
            "fixture_state_cells": {name: hex(address)
                for name, address in source_layout[3].items()},
            "external_bindings": {name: hex(address) for name, address in sorted(source_layout[4].items())
                if name.startswith(("_", "__imp_"))},
            "crt_contract": {
                "archive_identity_receipt_key": "crt_archive_identity",
                "__setjmp3": {"va": hex(SETJMP3_VA), "archive": "lib/libcmt.lib", "member": 259, "size": 123},
                "_longjmp": {"va": hex(LONGJMP_VA), "archive": "lib/libcmt.lib", "member": 256, "size": 121},
                "execution": "unchanged original executable CRT bytes execute; synthetic FS:[0]=0 with zero-backed address0 precondition; actual Windows thread exception chains are not modeled; no helper call beyond mapped/identified CRT paths",
                "setjmp_header": "pinned VC5 setjmp.h: jmp_buf is 16 ints/64 bytes; setjmp and longjmp are __cdecl; longjmp takes env then int value",
                "native_body_sha256": {"__setjmp3": digest(pe.read_rva(0x1161C, 123)),
                                       "_longjmp": digest(pe.read_rva(0x115A0, 121))}},
            "abi_contract": {
                "dispatcher": "void __cdecl host_engine_dispatch_5ec0(void)",
                "export_fixture": "void __cdecl dispatch_test_engine_main(void *, int); target caller removes two 32-bit arguments after callback",
                "win32_imports": ["FreeLibrary BOOL WINAPI(HMODULE)",
                                  "LoadLibraryA HMODULE WINAPI(LPCSTR)",
                                  "GetProcAddress FARPROC WINAPI(HMODULE,LPCSTR)"],
                "fixture_callback_is_not_retail_DLL_code": True},
            "table": "retail file-backed table bytes and self-pointer are mapped as fixture input at preferred addresses; source/TU ownership unresolved",
            "path_error_jump_storage": "Source loader scalars and64-byte SDK jmp_buf prefix come from freshly compiled canonical host_engine_storage.c. Path/error512-byte arrays, native-address fixture cells, table inputs, export callback and SEH-head precondition remain synthetic; original owner/capacities and placement are not proved.",
            "same_object_gate": "the source COFF objects named by compile.units are the exact objects raw-compared in object_comparisons and linked/emulated below; object SHA-256 values bind all three steps"}

        scenarios = scenario_matrix()
        report["scenario_count"] = len(scenarios)
        case_rows = []
        for name, scenario in scenarios:
            native_result = invoke("original", scenario, pe, fixture_native_layout)
            candidate_result = invoke("source", scenario, pe, source_layout)
            def normalized(value):
                if not value["sdk_unwind_data_tail_unchanged"]:
                    raise AssertionError("SDK unwind-data tail changed")
                # Native fixed-address cells and candidate fixture allocations
                # intentionally use different env VAs. Compare the proven cell
                # role; preserve both actual addresses in the receipt.
                return [{key: item for key, item in event.items() if key != "env_address"}
                        if event.get("api") in ("__setjmp3_entry", "_longjmp_entry")
                        else event for event in value["events"]], value["boundary"], value["final"]
            equal = normalized(native_result) == normalized(candidate_result)
            case_rows.append({"case": name, "differential_equal": equal,
                              "original": native_result, "source": candidate_result})
            if not equal:
                report["cases"] = case_rows
                report["status"] = "differential_mismatch"
                report["failure"] = {"case": name}
                write_report(output, report)
                raise AssertionError(json.dumps({"case": name, "original": native_result,
                                                 "source": candidate_result}, indent=2))
            validate_case(name, scenario, native_result)
            validate_case(name, scenario, candidate_result)
        report["cases"] = case_rows
        if any(digest(p.read_bytes()) != before[p.name]["sha256"] for p in required):
            raise RuntimeError("a source/header changed during the bounded comparison")
        if digest(native.read_bytes()) != ORIGINAL_SHA256:
            raise RuntimeError("the pinned original changed during the bounded comparison")
        report["source_header_oracle_inputs_unchanged"] = True
        report["differential_equal"] = True
        report["status"] = "pass_behavioral_differential_raw_mismatch"
        report["claim_limit"] = "behavioral agreement for nine finite fixtures is not FUNCTION_MATCH, source identity, storage ownership, DLL behavior, SEH-unwind behavior, or full startup proof"
        report["runtime"] = {"python": report["analysis_provenance"]["python"],
                             "unicorn": report["analysis_provenance"]["unicorn"],
                             "pefile": report["analysis_provenance"]["pefile"]}
        write_report(output, report)
        print(json.dumps({"status": report["status"], "cases": len(case_rows),
                          "differential_equal": True,
                          "raw_exact": {row["symbol"]: row["raw_body"]["exact"]
                                        for row in report["object_comparisons"]},
                          "objects": {name: row["object_sha256"]
                                      for name, row in report["synthetic_link"]["separate_source_objects"].items()},
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
