"""Strict canonical-source differential for HERCULES.EXE RVA 0x5f20.

Requires canonical timing, loader, selector, dispatcher, table-pointer and
callback-byte sources. There is no private-source fallback. It freshly builds
each source as an independent VC5 object, compares function/data contributions,
and runs the same finite Unicorn cases. The engine table in the source graph is
test-only fixture backing; no table initializer or production owner is inferred.
Helper 0x9cd0, WinMM and the engine export remain bounded test boundaries. No
original process, real DLL/API or asset is run.
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
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
LANE = REPO / "work/host_engine_loader_recovery"
CANONICAL_SOURCE = REPO / "calibration/host_timing_dispatch_5f20.c"
POINTER_SOURCE = REPO / "calibration/host_source_table_pointer_cell_365a0.c"
FLAG_SOURCE = REPO / "calibration/host_dispatch_flag_cell_7a01fc.c"
RESET_CELL_SOURCE = REPO / "calibration/graphics_storage.cpp"
ACCUMULATOR_CELL_SOURCE = REPO / "calibration/host_timing_cell_91c470.c"
PREVIOUS_TICK_CELL_SOURCE = REPO / "calibration/host_timing_cell_91c474.c"
RESET_CELL_MEASUREMENT = REPO / "evidence/host_timing_dispatch_recovery.json"
PUBLISHED_SOURCE_PINS = REPO / "evidence/host_timing_dispatch_recovery.json"
PUBLIC_RUNNER = REPO / "scripts/host_dispatch_differential.py"
ORIGINAL = Path(r"D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE")
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")
SOURCE_STEP_VA = 0x7004000
NATIVE_STEP_VA = 0x405F20
HELPER_VA = 0x409CD0
TIME_IAT_VA = 0xBA5B24
TIME_STUB_VA = 0x6000130
REENTRY_RETURN_VA = 0x6000140
CELL_91C46C = 0x91C46C
CELL_91C470 = 0x91C470
CELL_91C474 = 0x91C474
FLAG_VA = 0xBA01FC
TABLE_CELL_VA = 0x4365A0
TABLE_VA = 0x4361B8
ORIGINAL_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_public_runner():
    spec = importlib.util.spec_from_file_location("host_dispatch_differential", PUBLIC_RUNNER)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load bounded canonical provider runner")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_and_link(r, run_dir, original, source):
    from coff import COFF

    fixture = r.fixture_source(run_dir / "dispatch_fixture.c")
    fixture_text = fixture.read_text(encoding="ascii")
    fixture_marker = '#include "host_engine_cells.h"\n'
    if fixture_text.count(fixture_marker) != 1:
        raise ValueError("public dispatcher fixture include marker changed")
    fixture_text = fixture_text.replace(
        fixture_marker,
        fixture_marker + '#include "engine_interface.h"\n\n'
        '/* TEST-ONLY backing for the external table symbol in the source graph.\n'
        ' * This is not a recovered production initializer or table owner. */\n'
        'EngineInterface g_host_engine_interface;\n', 1)
    fixture.write_text(
        "/* TEST-ONLY generated differential fixture. Every storage object in this\n"
        " * file is synthetic; matching target addresses and capacities do not\n"
        " * assert a retail declaration, owner, aggregate extent or translation unit.\n"
        " * The callback is a test endpoint, not original DLL code. */\n" +
        fixture_text, encoding="ascii", newline="\n")
    units = [
        ("host_engine_loader", r.LOADER_SOURCE),
        ("host_engine_select", r.SELECT_SOURCE),
        ("host_engine_dispatch", r.DISPATCH_SOURCE),
        ("host_source_table_pointer_cell_365a0", POINTER_SOURCE),
        ("host_dispatch_flag_cell_7a01fc", FLAG_SOURCE),
        ("graphics_storage", RESET_CELL_SOURCE),
        ("host_timing_cell_91c470", ACCUMULATOR_CELL_SOURCE),
        ("host_timing_cell_91c474", PREVIOUS_TICK_CELL_SOURCE),
        ("dispatch_fixture", fixture),
        ("host_timing_dispatch_5f20", source),
    ]
    objects = {}
    compiles = []
    for name, source in units:
        obj = run_dir / (name + ".obj")
        row = r.compile_unit(source, obj, r.SOURCE_DIR, REPO)
        compiles.append(row)
        if row.get("status") != "compiled":
            return {"compile_rows": compiles, "objects": objects, "layout": None,
                    "native_layout": None, "fixture": fixture}
        objects[name] = obj

    provider_ownership = {}
    for symbol, expected_owner in (
            ("_g_exe_91c46c", "graphics_storage"),
            ("_g_exe_cell_91c470", "host_timing_cell_91c470"),
            ("_g_exe_cell_91c474", "host_timing_cell_91c474")):
        definitions = []
        for object_name, obj in objects.items():
            coff = COFF(obj)
            for row in coff.symbols.values():
                if (row["name"] == symbol and row["storage"] == 2 and
                        (row["section"] > 0 or row["value"] > 0)):
                    definitions.append(object_name)
        if definitions != [expected_owner]:
            raise ValueError(f"{symbol}: expected sole owner {expected_owner}, found {definitions}")
        provider_ownership[symbol] = expected_owner

    source_layout = r.link_objects(
        [objects[name] for name in ("host_engine_loader", "host_engine_select",
                                    "host_engine_dispatch",
                                    "host_source_table_pointer_cell_365a0",
                                    "host_dispatch_flag_cell_7a01fc",
                                    "graphics_storage",
                                    "host_timing_cell_91c470",
                                    "host_timing_cell_91c474",
                                    "dispatch_fixture")], "source")
    native_layout = r.link_objects([objects["dispatch_fixture"]], "original_fixture")

    step_coff = COFF(objects["host_timing_dispatch_5f20"])
    step_section = step_coff.function("_host_timing_dispatch_5f20")
    step_symbol = next(s for s in step_coff.symbols.values()
                       if s["name"] == "_host_timing_dispatch_5f20" and s["section"] > 0)
    if step_symbol["value"] != 0 or step_section["size"] != 176:
        raise ValueError("candidate does not occupy one complete /Gy COMDAT with 176 bytes")

    bound = dict(source_layout[4])
    bound.update({
        "_host_timing_service_409cd0": HELPER_VA,
        "__imp__timeGetTime@0": TIME_IAT_VA,
        "_g_host_engine_call_flag_7a01fc": source_layout[4]["_g_host_engine_call_flag_7a01fc"],
    })
    code = bytearray(step_section["data"])
    relocs = []
    for reloc in step_section["relocations"]:
        name, offset, kind = reloc["symbol"], reloc["offset"], reloc["type"]
        if name not in bound:
            raise ValueError(f"unbound candidate call/cell {name}")
        target = bound[name]
        addend = struct.unpack_from("<i", code, offset)[0]
        if kind == 0x0006:
            value = target + addend
        elif kind == 0x0014:
            value = target + addend - (SOURCE_STEP_VA + offset + 4)
        else:
            raise ValueError(f"unsupported candidate relocation {reloc}")
        struct.pack_into("<I", code, offset, value & 0xFFFFFFFF)
        relocs.append({**reloc, "target": hex(target), "addend": addend})

    linked_code = list(source_layout[1]) + [(SOURCE_STEP_VA, bytes(code))]
    symbols = dict(source_layout[4])
    symbols.update(bound)
    symbols["_host_timing_dispatch_5f20"] = SOURCE_STEP_VA
    definitions = dict(source_layout[5])
    definitions["_host_timing_dispatch_5f20"] = {
        "object_index": len(source_layout[0][0]), "section": step_symbol["section"],
        "size": step_section["size"], "address": SOURCE_STEP_VA}
    source_layout = (source_layout[0], linked_code, list(source_layout[2]),
                     dict(source_layout[3]), symbols, definitions)
    return {"compile_rows": compiles, "objects": objects, "layout": source_layout,
            "native_layout": native_layout, "fixture": fixture,
            "provider_ownership": provider_ownership,
            "step_section": step_section, "step_relocations": relocs}


def state(r, uc, flag_address, clock_addresses):
    return {
        "cell_91c46c": r.u32(uc, clock_addresses["cell_91c46c"]),
        "cell_91c470": r.u32(uc, clock_addresses["cell_91c470"]),
        "cell_91c474": r.u32(uc, clock_addresses["cell_91c474"]),
        "callback_byte": uc.mem_read(flag_address, 1)[0],
    }


def apply_mutation(r, uc, mutation, flag_address, clock_addresses):
    if not mutation:
        return
    fields = clock_addresses
    for name, value in mutation.items():
        if name == "callback_byte":
            uc.mem_write(flag_address, bytes((value & 0xFF,)))
        elif name in fields:
            r.put_u32(uc, fields[name], value)
        else:
            raise ValueError(f"unknown synthetic mutation field {name}")


def execute(r, pe, kind, layout, scenario):
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import (
        UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ECX,
        UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_EFLAGS, UC_X86_REG_EIP,
        UC_X86_REG_ESI, UC_X86_REG_ESP,
    )

    runner_kind = "source" if kind == "source" else "native"
    uc, cells = r.make_emulator(runner_kind, pe, layout, {})
    symbols = layout[4]
    clock_addresses = {
        "cell_91c46c": symbols["_g_exe_91c46c"] if kind == "source" else CELL_91C46C,
        "cell_91c470": symbols["_g_exe_cell_91c470"] if kind == "source" else CELL_91C470,
        "cell_91c474": symbols["_g_exe_cell_91c474"] if kind == "source" else CELL_91C474,
    }
    flag_address = symbols.get("_g_host_engine_call_flag_7a01fc", FLAG_VA)
    table_cell_address = symbols.get("_g_host_engine_source_table_cell_365a0", TABLE_CELL_VA)
    expected_table_address = (symbols.get("_g_host_engine_interface")
                              if kind == "source" else TABLE_VA)
    actual_table_address = r.u32(uc, table_cell_address)
    if actual_table_address != expected_table_address:
        raise AssertionError(
            f"{kind} table pointer cell {actual_table_address:#x} does not target its fixture table {expected_table_address:#x}")
    # The original 0x9cd0 body is deliberately outside this caller-family test;
    # a mapped page lets the hook stop exactly at that unchanged entry.
    uc.mem_map(0x409000, 0x1000)
    uc.mem_write(0x409000, pe.read_rva(0x9000, 0x1000))
    r.put_u32(uc, TIME_IAT_VA, TIME_STUB_VA)

    for cell, value in ((cells["gate"], 0), (cells["selector"], 0),
                        (cells["module"], 0), (cells["entry"], r.FIXTURE_CALLBACK_VA),
                        (cells["mode"], 0), (cells["count"], 0),
                        (cells["last_table"], 0), (cells["last_flag"], 0)):
        r.put_u32(uc, cell, value)
    # TEST-ONLY backing: the public fixture allocates a synthetic 512-byte path,
    # 512-byte error buffer and 64-byte jmp_buf; none is adopted game storage.
    uc.mem_write(cells["jump_buffer"], b"\xA7" * 64)
    uc.mem_write(cells["path"], b"\0" * 512)
    uc.mem_write(cells["error"], b"\0" * 512)
    for name, address in clock_addresses.items():
        r.put_u32(uc, address, scenario.get(name, 0))
    uc.mem_write(flag_address, bytes((scenario.get("initial_flag", 0xA5),)))

    step_pc = SOURCE_STEP_VA if kind == "source" else NATIVE_STEP_VA
    stack_entry = r.STACK_TOP - 0x40
    r.put_u32(uc, stack_entry, r.STOP_VA)
    initial_registers = (
        (UC_X86_REG_EAX, 0x11111111), (UC_X86_REG_EBX, 0x22222222),
        (UC_X86_REG_ECX, 0x33333333), (UC_X86_REG_EDX, 0x44444444),
        (UC_X86_REG_ESI, 0x55555555), (UC_X86_REG_EDI, 0x66666666),
        (UC_X86_REG_EBP, 0x77777777), (UC_X86_REG_EFLAGS, 0x202),
        (UC_X86_REG_ESP, stack_entry), (UC_X86_REG_EIP, step_pc),
    )
    for reg, value in initial_registers:
        uc.reg_write(reg, value)

    events = []
    instruction_trace = []
    time_index = 0
    dispatch_index = 0
    reentered = False
    pending_reentry = []
    entry_callback = cells["dispatch_callback"]
    times = list(scenario["time_samples"])

    def return_cdecl(engine, esp, return_pc):
        engine.reg_write(UC_X86_REG_ESP, esp + 4)
        engine.reg_write(UC_X86_REG_EIP, return_pc)

    def on_code(engine, address, _size, _user):
        nonlocal time_index, dispatch_index, reentered
        if (kind == "original" and scenario.get("name") ==
                "first-sample-exact-step" and NATIVE_STEP_VA <= address < NATIVE_STEP_VA + 173):
            instruction_trace.append({
                "pc": hex(address),
                "eax": hex(engine.reg_read(UC_X86_REG_EAX)),
                "ecx": hex(engine.reg_read(UC_X86_REG_ECX)),
                "edx": hex(engine.reg_read(UC_X86_REG_EDX)),
                "eflags": hex(engine.reg_read(UC_X86_REG_EFLAGS)),
                "state": state(r, engine, flag_address, clock_addresses),
            })
        if address == HELPER_VA:
            events.append({"event": "helper_409cd0_entry", "state": state(r, engine, flag_address, clock_addresses)})
            apply_mutation(r, engine, scenario.get("helper_mutation"), flag_address,
                           clock_addresses)
            esp = engine.reg_read(UC_X86_REG_ESP)
            return_cdecl(engine, esp, r.u32(engine, esp))
            return
        if address == TIME_STUB_VA:
            esp = engine.reg_read(UC_X86_REG_ESP)
            ret = r.u32(engine, esp)
            value = times[min(time_index, len(times) - 1)]
            time_index += 1
            apply_mutation(r, engine, scenario.get("timer_mutation"), flag_address,
                           clock_addresses)
            events.append({"event": "timeGetTime_boundary", "value": value,
                           "state_after_mutation": state(r, engine, flag_address,
                                                          clock_addresses)})
            engine.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
            return_cdecl(engine, esp, ret)
            return
        if address == entry_callback:
            esp = engine.reg_read(UC_X86_REG_ESP)
            ret = r.u32(engine, esp)
            table_arg = r.u32(engine, esp + 4)
            flag_arg = r.u32(engine, esp + 8)
            dispatch_index += 1
            events.append({"event": "engine_export_boundary", "call": dispatch_index,
                           "table_arg": "test_table" if table_arg == expected_table_address else hex(table_arg),
                           "flag_arg": flag_arg,
                           "state": state(r, engine, flag_address, clock_addresses)})
            callback_mutations = scenario.get("callback_mutations", {})
            apply_mutation(r, engine, callback_mutations.get(str(dispatch_index)), flag_address,
                           clock_addresses)
            if scenario.get("reenter_once") and not reentered:
                reentered = True
                pending_reentry.append((esp, ret))
                new_esp = esp - 4
                r.put_u32(engine, new_esp, REENTRY_RETURN_VA)
                engine.reg_write(UC_X86_REG_ESP, new_esp)
                engine.reg_write(UC_X86_REG_EIP, step_pc)
                return
            return_cdecl(engine, esp, ret)
            return
        if address == REENTRY_RETURN_VA:
            if not pending_reentry:
                raise AssertionError("unexpected synthetic caller re-entry return")
            callback_esp, callback_return = pending_reentry.pop()
            events.append({"event": "reentry_return_to_export",
                           "state": state(r, engine, flag_address, clock_addresses)})
            engine.reg_write(UC_X86_REG_ESP, callback_esp + 4)
            engine.reg_write(UC_X86_REG_EIP, callback_return)

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.emu_start(step_pc, r.STOP_VA, count=1_000_000)
    final_esp = uc.reg_read(UC_X86_REG_ESP)
    if uc.reg_read(UC_X86_REG_EIP) != r.STOP_VA:
        raise AssertionError(f"unexpected stop at {uc.reg_read(UC_X86_REG_EIP):#x}")
    output = {
        "events": events,
        "instruction_trace": instruction_trace,
        "final_cells": state(r, uc, flag_address, clock_addresses),
        "selector": r.u32(uc, cells["selector"]),
        "entry": r.u32(uc, cells["entry"]),
        "export_callback_count": dispatch_index,
        "time_calls": time_index,
        "return_stack_delta": final_esp - stack_entry,
        "table_pointer_cell": "test_table" if r.u32(uc, table_cell_address) == expected_table_address else hex(r.u32(uc, table_cell_address)),
    }
    return output


def case_matrix():
    return [
        {"name": "below-fixed-step", "cell_91c474": 100, "time_samples": [132]},
        {"name": "exact-fixed-step", "cell_91c474": 100, "time_samples": [133]},
        {"name": "negative-underflow-clamp", "cell_91c474": 1000, "time_samples": [0]},
        {"name": "first-sample-zero", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [0]},
        {"name": "first-sample-exact-step", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [33]},
        {"name": "positive-clamp-three-calls", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [1000]},
        {"name": "initial-negative-no-callback", "cell_91c470": -66, "cell_91c474": 100, "time_samples": [100]},
        {"name": "reset-cell-cleared", "cell_91c46c": 1, "cell_91c470": 64, "cell_91c474": 100, "time_samples": [333]},
        {"name": "first-sample-clears-shared-reset", "cell_91c46c": 1, "cell_91c470": 0, "cell_91c474": 0, "time_samples": [33]},
        {"name": "callback-live-accumulator-to-65", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [33], "callback_mutations": {"1": {"cell_91c470": 65}}},
        {"name": "callback-live-accumulator-negative", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [33], "callback_mutations": {"1": {"cell_91c470": -1}}},
        {"name": "callback-live-accumulator-to-zero", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [66], "callback_mutations": {"1": {"cell_91c470": 0}}},
        {"name": "helper-boundary-mutation-read-after-helper", "cell_91c474": 900, "cell_91c470": 0, "time_samples": [33], "helper_mutation": {"cell_91c474": 10, "cell_91c470": 20}},
        {"name": "timer-boundary-mutation-read-after-import", "cell_91c474": 100, "cell_91c470": 0, "time_samples": [53], "timer_mutation": {"cell_91c474": 20, "cell_91c470": 30}},
        {"name": "one-level-reentry-through-dispatch-callback", "cell_91c470": 0, "cell_91c474": 0, "time_samples": [66, 99], "reenter_once": True},
    ]


def storage_diagnostics(obj, symbol_name):
    from coff import COFF
    coff = COFF(obj)
    matches = [s for s in coff.symbols.values()
               if s["name"] == symbol_name and s["storage"] == 2]
    if len(matches) != 1:
        raise ValueError(f"{symbol_name}: expected one external COFF definition, got {matches}")
    sym = matches[0]
    rows = []
    for index, section in enumerate(coff.sections, 1):
        if section["size"]:
            rows.append({"index": index, "name": section["name"],
                         "bytes": section["size"], "flags": hex(section["flags"]),
                         "raw_hex": section["data"].hex(),
                         "raw_sha256": digest(section["data"]),
                         "relocations": section["relocations"]})
    common = sym["section"] == 0 and sym["value"] > 0
    section_bytes = 0
    relocation = None
    if sym["section"] > 0:
        section = coff.sections[sym["section"] - 1]
        section_bytes = section["size"]
        relocation = next((r for r in section["relocations"]
                           if r["offset"] == sym["value"]), None)
    return {"object": str(obj), "object_sha256": digest(obj.read_bytes()),
            "object_bytes": obj.stat().st_size, "symbol": sym,
            "storage_class": "common/BSS" if common else "section-defined",
            "minimum_bytes": sym["value"] if common else None,
            "defined_section_bytes": section_bytes,
            "symbol_start_relocation": relocation,
            "noncode_sections": rows}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-exe", type=Path, default=ORIGINAL)
    parser.add_argument("--output", type=Path,
                        default=LANE / "public_host_timing_dispatch_strict_differential.json")
    args = parser.parse_args(argv)
    native = args.native_exe.resolve()
    output = args.output.resolve()
    source = CANONICAL_SOURCE.resolve()
    required_canonical = (source, POINTER_SOURCE, FLAG_SOURCE, RESET_CELL_SOURCE,
                          ACCUMULATOR_CELL_SOURCE, PREVIOUS_TICK_CELL_SOURCE,
                          REPO / "calibration/host_engine_loader.c",
                          REPO / "calibration/host_engine_select.c",
                          REPO / "calibration/host_engine_dispatch.c")
    missing = [str(path) for path in required_canonical if not path.is_file()]
    if missing:
        parser.error("required canonical source missing (private fallback disabled): " +
                     ", ".join(missing))
    if not output.is_relative_to((REPO / "work").resolve()):
        parser.error("--output must be under this checkout's ignored work directory")
    report = {"schema": "host-timing-dispatch-5f20-strict-canonical-differential-v1", "status": "starting",
              "scope": "unchanged retail 0x5f20 versus freshly compiled mandatory canonical caller, loader, selector, dispatcher, pointer-cell, flag, existing graphics_storage reset-cell owner, and separate accumulator/previous-tick cell providers; each is an independent object; helper/time import/export are test boundaries; no game process, DLL or real API",
              "timing_cell_prediction": "With the existing graphics_storage.cpp definition reused for RVA 0x51c46c, the caller source should preserve its full 176-byte contribution under the three corresponding reset-cell symbol renames; the other clock cells remain independent 4-byte views. Falsifier: any raw contribution, relocation identity/count/addend, ABI, or finite behavior mismatch beyond those names.",
              "runner": {"path": str(SCRIPT), "sha256": digest(SCRIPT.read_bytes())},
              "caller_source": {"path": str(source), "origin": "canonical_required",
                                "sha256": digest(source.read_bytes())},
              "arguments": {"native_exe": str(native), "source": str(source),
                            "output": str(output)}}
    try:
        r = load_public_runner()
        from coff import COFF
        from pe import PE
        if digest(native.read_bytes()) != ORIGINAL_SHA256:
            raise ValueError("original EXE hash differs from the pinned retail image")
        required = (r.LOADER_SOURCE, r.SELECT_SOURCE, r.HEADER_SOURCE,
                    r.DISPATCH_SOURCE, source, POINTER_SOURCE, FLAG_SOURCE,
                    RESET_CELL_SOURCE, ACCUMULATOR_CELL_SOURCE,
                    PREVIOUS_TICK_CELL_SOURCE,
                    RESET_CELL_MEASUREMENT, PUBLISHED_SOURCE_PINS,
                    REPO / "calibration/engine_interface.h",
                    REPO / "calibration/resource_context.h",
                    r.TOOLCHAIN / "include/windows.h",
                    r.TOOLCHAIN / "include/mmsystem.h",
                    PUBLIC_RUNNER)
        before = {str(path): digest(path.read_bytes()) for path in required}
        pins = json.loads(PUBLISHED_SOURCE_PINS.read_text(encoding="utf-8"))
        pinned = {row["source"]: row["sha256"] for row in pins["sources"]}
        for path in (source, POINTER_SOURCE, FLAG_SOURCE,
                     ACCUMULATOR_CELL_SOURCE, PREVIOUS_TICK_CELL_SOURCE):
            relative = path.relative_to(REPO).as_posix()
            if pinned.get(relative) != digest(path.read_bytes()):
                raise ValueError(f"published source pin mismatch for {relative}")
        reset_pin = pins["existing_reset_owner"]
        if (reset_pin["path"] != RESET_CELL_SOURCE.relative_to(REPO).as_posix() or
                reset_pin["sha256"] != digest(RESET_CELL_SOURCE.read_bytes()) or
                pins.get("no_reset_provider_added") is not True):
            raise ValueError("published reset-cell ownership pin changed")
        pe = PE(native)
        report["original"] = {"path": str(native), "sha256": digest(native.read_bytes()),
                              "rva": "0x5f20", "body_bytes": 173, "padded_bytes": 176,
                              "body_sha256": digest(pe.read_rva(0x5F20, 173)),
                              "padded_sha256": digest(pe.read_rva(0x5F20, 176)),
                              "table_rva": "0x361b8", "table_sha256": digest(pe.read_rva(0x361B8, 1000)),
                              "table_pointer_cell_rva": "0x365a0",
                              "table_pointer_cell_sha256": digest(pe.read_rva(0x365A0, 4))}
        report["source_inputs"] = before
        report["related_evidence"] = {
            "shared_reset_measurement": {
                "path": str(RESET_CELL_MEASUREMENT),
                "sha256": digest(RESET_CELL_MEASUREMENT.read_bytes())},
            "published_source_pins": {
                "path": str(PUBLISHED_SOURCE_PINS),
                "sha256": digest(PUBLISHED_SOURCE_PINS.read_bytes())},
            "clock_cell_audit_pin": pins.get("clock_evidence")}
        run_root = LANE / "public_timing_differential_builds"
        run_root.mkdir(parents=True, exist_ok=True)
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S_")
        run_dir = Path(tempfile.mkdtemp(prefix=stamp, dir=run_root))
        report["build_directory"] = str(run_dir)
        build = compile_and_link(r, run_dir, native, source)
        report["compile"] = build["compile_rows"]
        if build["layout"] is None:
            report["status"] = "compiler_launch_error" if build["compile_rows"][-1].get("status") == "launch_error" else "compile_failed"
            report["failed_compile"] = build["compile_rows"][-1]
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
            print(json.dumps({"status": report["status"], "output": str(output), "compile": report["failed_compile"]}, indent=2))
            return 2
        report["fresh_objects"] = {name: {"path": str(path), "sha256": digest(path.read_bytes()),
                                                "bytes": path.stat().st_size}
                                   for name, path in build["objects"].items()}
        report["clock_cell_provider_ownership"] = build["provider_ownership"]
        fixture_path = build["fixture"]
        report["test_only_fixture"] = {
            "path": str(fixture_path),
            "sha256": digest(fixture_path.read_bytes()),
            "synthetic_storage": {
                "host_path": {"bytes": 512, "status": "test-only; original capacity unknown"},
                "host_error": {"bytes": 512, "status": "test-only; original capacity unknown"},
                "dispatcher_jmp_buf": {"bytes": 64, "status": "test-only fixture backing; not an original ownership claim"},
                "loader_dispatch_cells": "test-only fixture definitions bound at their observed addresses for this isolated differential",
                "source_host_table": {"bytes": build["layout"][0][1].get("_g_host_engine_interface"),
                                      "status": "synthetic fixture-only EngineInterface backing for the source external symbol; never a production provider or recovered original table initializer"},
                "callback_state": "compiled synthetic test endpoint; not an original DLL body"},
            "banner": "TEST-ONLY generated differential fixture; no declaration, owner, aggregate extent or translation unit is inferred"}
        after = {str(path): digest(path.read_bytes()) for path in required}
        if after != before:
            raise RuntimeError("a source/header/tool input changed during fresh compilation")
        report["inputs_unchanged_during_compile"] = True
        report["provider_object_comparisons"] = {}
        for name, source_path, symbol, rva, body, padded in (
                ("host_engine_loader", r.LOADER_SOURCE, "_host_engine_resolve",
                 0x5C20, r.RESOLVE_BODY_SIZE, r.RESOLVE_PADDED_SIZE),
                ("host_engine_select", r.SELECT_SOURCE, "_host_engine_select",
                 0x5CD0, r.SELECT_BODY_SIZE, r.SELECT_PADDED_SIZE),
                ("host_engine_dispatch", r.DISPATCH_SOURCE,
                 "_host_engine_dispatch_5ec0", 0x5EC0,
                 r.DISPATCH_BODY_SIZE, r.DISPATCH_PADDED_SIZE)):
            report["provider_object_comparisons"][name] = r.source_diagnostics(
                build["objects"][name], source_path, native, symbol, rva, body, padded)
        report["storage_object_comparisons"] = {
            "source_table_pointer_cell": storage_diagnostics(
                build["objects"]["host_source_table_pointer_cell_365a0"],
                "_g_host_engine_source_table_cell_365a0"),
            "callback_flag": storage_diagnostics(
                build["objects"]["host_dispatch_flag_cell_7a01fc"],
                "_g_host_engine_call_flag_7a01fc"),
            "existing_reset_cell_owner_0x91c46c": storage_diagnostics(
                build["objects"]["graphics_storage"], "_g_exe_91c46c"),
            "accumulator_cell_0x91c470": storage_diagnostics(
                build["objects"]["host_timing_cell_91c470"],
                "_g_exe_cell_91c470"),
            "previous_tick_cell_0x91c474": storage_diagnostics(
                build["objects"]["host_timing_cell_91c474"],
                "_g_exe_cell_91c474"),
            "native_prefixes": {
                "clock_cells": {
                    "rva_0x51c46c": {"native_va": hex(CELL_91C46C), "bytes": 4,
                                      "origin": "loader-zero tail of .data"},
                    "rva_0x51c470": {"native_va": hex(CELL_91C470), "bytes": 4,
                                      "origin": "loader-zero tail of .data"},
                    "rva_0x51c474": {"native_va": hex(CELL_91C474), "bytes": 4,
                                      "origin": "loader-zero tail of .data"}},
                "table_pointer_cell_rva_0x365a0": {
                    "raw_hex": pe.read_rva(0x365A0, 4).hex(),
                    "value": hex(struct.unpack("<I", pe.read_rva(0x365A0, 4))[0])},
                "callback_flag_rva_0x7a01fc": {
                    "origin": "PE .data virtual zero-fill tail",
                    "value_in_fresh_emulator": 0}},
            "linked_fixture_addresses": {
                "source_table_cell": hex(build["layout"][4]["_g_host_engine_source_table_cell_365a0"]),
                "source_flag": hex(build["layout"][4]["_g_host_engine_call_flag_7a01fc"]),
                "source_table_backing": hex(build["layout"][4]["_g_host_engine_interface"]),
                "source_clock_cells": {
                    "0x51c46c": hex(build["layout"][4]["_g_exe_91c46c"]),
                    "0x51c470": hex(build["layout"][4]["_g_exe_cell_91c470"]),
                    "0x51c474": hex(build["layout"][4]["_g_exe_cell_91c474"])},
                "source_table_bytes": build["layout"][0][1].get("_g_host_engine_interface"),
                "native_table_cell": hex(TABLE_CELL_VA),
                "native_table_backing": hex(TABLE_VA)},
            "binding_contract": "source pointer initializer resolves to the fixture-only table symbol; original run uses unchanged retail pointer/table bytes. Callback observations normalize only the per-run table address identity. No table initializer/provider is adopted."}
        report["caller_object"] = {
            "symbol": "_host_timing_dispatch_5f20", "comdat_bytes": build["step_section"]["size"],
            "comdat_sha256": digest(build["step_section"]["data"]),
            "relocations": build["step_relocations"],
            "raw_body_compare": r.source_diagnostics(
                build["objects"]["host_timing_dispatch_5f20"], source, native,
                "_host_timing_dispatch_5f20", 0x5F20, 173, 176)["raw_body"],
            "raw_padded_compare": r.source_diagnostics(
                build["objects"]["host_timing_dispatch_5f20"], source, native,
                "_host_timing_dispatch_5f20", 0x5F20, 173, 176)["raw_padded"],
            "relocation_diagnostic": r.source_diagnostics(
                build["objects"]["host_timing_dispatch_5f20"], source, native,
                "_host_timing_dispatch_5f20", 0x5F20, 173, 176)["relocation_masked_padded_diagnostic"],
        }
        cases = []
        all_equal = True
        for scenario in case_matrix():
            original_result = execute(r, pe, "original", build["native_layout"], scenario)
            source_result = execute(r, pe, "source", build["layout"], scenario)
            # Instruction tracing is a native-only diagnostic. Exclude it from
            # the semantic result comparison while retaining it in the receipt.
            original_semantics = {k: v for k, v in original_result.items()
                                  if k != "instruction_trace"}
            source_semantics = {k: v for k, v in source_result.items()
                                if k != "instruction_trace"}
            equal = original_semantics == source_semantics
            all_equal &= equal
            cases.append({"name": scenario["name"], "inputs": scenario,
                          "equal": equal, "original": original_result, "source": source_result})
        report["cases"] = cases
        report["status"] = "behavioral_match" if all_equal else "behavioral_mismatch"
        report["acceptance_boundary"] = "Finite boundary differential only; source remains a reconstructed candidate. Raw caller equality is reported separately and is required for FUNCTION_MATCH. The source graph reuses graphics_storage.cpp for RVA 0x51c46c and the two independent 32-bit prefix providers for 0x51c470/474; no shared clock aggregate or TU is inferred. No placement claim or full helper/API behavior claim."
        report["boundary_contract"] = {
            "RVA_0x9cd0": "hooked before entry; the helper body is not executed in this caller differential",
            "timeGetTime": "synthetic value sequence through IAT cell RVA 0x7a5b24; actual WinMM is never called",
            "PC_DLLEngineMain": "synthetic export stop boundary; callback records two arguments and optionally mutates address-labelled cells or re-enters caller once",
            "fixture_storage": "generated fixture path/error/jmp_buf and loader/dispatch state cells are TEST-ONLY allocations: path=512 bytes, error=512 bytes, jmp_buf=64 bytes; no game storage capacity/owner is inferred",
            "retail_table": "native run maps the unchanged initialized table and self-pointer; source run uses an explicit synthetic EngineInterface fixture object. No source table owner or production initializer is defined or inferred",
            "callback_flag": "source run uses the freshly compiled canonical BYTE provider as a test-only common cell; native run uses its PE zero-fill location. The fixture compares the observed byte behavior without inferring the original declared type or larger extent",
            "CRT": "actual source dispatcher uses the canonical provider; original path uses unchanged retail dispatcher and mapped VC5 CRT setjmp bytes under the prior runner's FS:[0]=0 bounded contract",
        }
    except Exception as exc:
        report["status"] = "error"
        report["error"] = {"type": type(exc).__name__, "message": str(exc),
                            "traceback": traceback.format_exc()}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"status": report["status"], "output": str(output),
                      "cases": len(report.get("cases", []))}, indent=2))
    return 0 if report["status"] == "behavioral_match" else 1


if __name__ == "__main__":
    raise SystemExit(main())
