#!/usr/bin/env python3
"""Finite original AC60 trace for safe 8-byte group-0x60 fallthrough tags."""
from __future__ import annotations
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT_HINT = next(parent for parent in Path(__file__).resolve().parents
                 if (parent / "scripts" / "pe.py").is_file()
                 and (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file())
sys.path.insert(0, str(ROOT_HINT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime, write_receipt  # noqa: E402
ARGS = parse_args(
    "Trace original AC60 group-0x60 preamble before command comparisons",
    "group60_fallthrough.json",
)
if (ARGS.original_root / "work").resolve() != ARGS.original_root / "work":
    raise ValueError("refusing a redirected work directory")
if ARGS.receipt == ARGS.exe or ARGS.exe.parent in ARGS.receipt.parents:
    raise ValueError("receipt must be outside the original image directory")
from pe import PE  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC,
    UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP,
)

PAGE = 0x1000
EXE_BASE = 0x00400000
AC60_START = EXE_BASE + 0xAC60
AC60_END = EXE_BASE + 0xB2F0
FALLTHROUGH_EXIT = EXE_BASE + 0xB119
TRAVERSAL_CONTINUATION = EXE_BASE + 0xACA2
INPUT_BASE = 0x10000000
STACK_BASE = 0x20000000
STACK_SIZE = 0x10000
RETURN_SENTINEL = 0x30000000
PACKET = INPUT_BASE + 0x1000
OT = INPUT_BASE + 0x100
GATE = 0x00BA01FC
PAGE_INDEX = 0x00A8E6F4
PAGE_MODE = 0x00A8E6F8
ALT_BANK = 0x00A8E6FC
PAGE_BASE = 0x00A909FC
UNKNOWN_CALLBACK = 0x31000000
MAX_INSTRUCTIONS = 100_000
HELPER_RANGES = [
    (EXE_BASE + 0x6FF0, EXE_BASE + 0x70B7),
    (EXE_BASE + 0x70C0, EXE_BASE + 0x7136),
    (EXE_BASE + 0x7140, EXE_BASE + 0x716E),
    (EXE_BASE + 0x6620, EXE_BASE + 0x66F5),
    (EXE_BASE + 0xBD80, EXE_BASE + 0xBD96),
]
RASTER = {EXE_BASE + rva for rva in (0xD420, 0xE680, 0xCAB0, 0xC8F0)}


def align_up(value: int) -> int:
    return (value + PAGE - 1) & ~(PAGE - 1)


def permissions(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000: result |= UC_PROT_READ
    if characteristics & 0x80000000: result |= UC_PROT_WRITE
    if characteristics & 0x20000000: result |= UC_PROT_EXEC
    return result


def map_image(uc: Uc, pe: PE):
    headers = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(EXE_BASE, align_up(headers), UC_PROT_READ)
    uc.mem_write(EXE_BASE, pe.data[:headers])
    rows = []
    for section in pe.sections:
        size = align_up(max(section["virtual_size"], section["raw_size"]))
        va = EXE_BASE + section["virtual_address"]
        uc.mem_map(va, size, permissions(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        rows.append((section, va, size))
    return rows


def read32(uc: Uc, va: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(va, 4)))[0]


def run_case(pe: PE, tag: int, gate_word: int):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = map_image(uc, pe)
    uc.mem_map(INPUT_BASE, 0x2000, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
    uc.mem_map(RETURN_SENTINEL, PAGE, UC_PROT_READ | UC_PROT_EXEC)

    packet = bytearray([0xA5] * 8)
    packet[0:4] = struct.pack("<I", OT + 4)
    packet[7] = tag
    uc.mem_write(PACKET, bytes(packet))
    uc.mem_write(OT, struct.pack("<II", PACKET, 0))

    data_section = next(row for row in pe.sections if row["name"] == ".data")
    data_va = EXE_BASE + data_section["virtual_address"]
    data_size = align_up(max(data_section["virtual_size"], data_section["raw_size"]))
    before_data = bytes(uc.mem_read(data_va, data_size))
    original_spans = [(AC60_START, AC60_END - AC60_START),
                      (EXE_BASE + 0xBD80, 0x16)]
    before_code = {va: bytes(uc.mem_read(va, size)) for va, size in original_spans}

    # Deliberate adjacent-byte canary: gate_word is written as two bytes at BA01FC.
    fixture = {
        GATE: struct.pack("<H", gate_word),
        PAGE_INDEX: struct.pack("<I", 0x0B),
        PAGE_MODE: struct.pack("<I", 0),
        ALT_BANK: struct.pack("<I", 0),
        PAGE_BASE: struct.pack("<I", 0x11223344),
        0x0096E698: b"\0",
        0x00BA020C: b"\0",
        0x0043A590: b"\0",
        0x0096E3D8: b"\0" * 4,
        0x00A8E780: struct.pack("<I", UNKNOWN_CALLBACK),
        0x00A8E788: b"\0" * 4,
    }
    old_fixture = {va: bytes(uc.mem_read(va, len(value))) for va, value in fixture.items()}
    for va, value in fixture.items(): uc.mem_write(va, value)

    sp = STACK_BASE + STACK_SIZE - 0x100
    uc.mem_write(sp, struct.pack("<II", RETURN_SENTINEL, OT))
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
    uc.reg_write(UC_X86_REG_EIP, AC60_START)

    state = {"kind": None, "detail": None, "instructions": 0,
             "stop_pc": None, "group60_seen": False}
    writes = []
    packet_reads = []
    pcs = []
    text = next(row for row in pe.sections if row["name"] == ".text")
    text_start = EXE_BASE + text["virtual_address"]
    text_end = text_start + align_up(max(text["virtual_size"], text["raw_size"]))
    scratch = [(INPUT_BASE, INPUT_BASE + 0x2000), (STACK_BASE, STACK_BASE + STACK_SIZE)]

    def on_code(machine, address, _size, _user):
        state["instructions"] += 1
        if state["instructions"] > MAX_INSTRUCTIONS:
            state.update(kind="budget", detail=state["instructions"]); machine.emu_stop(); return
        if EXE_BASE + 0xB04E <= address <= FALLTHROUGH_EXIT:
            state["group60_seen"] = True
        if address == FALLTHROUGH_EXIT:
            state.update(kind="group60_fallthrough_exit", stop_pc=hex(address - EXE_BASE))
            machine.emu_stop(); return
        if address == TRAVERSAL_CONTINUATION and state["group60_seen"]:
            state.update(kind="group60_gate_exit", stop_pc=hex(address - EXE_BASE))
            machine.emu_stop(); return
        if address in RASTER:
            state.update(kind="unexpected_raster", detail=hex(address - EXE_BASE)); machine.emu_stop(); return
        if address == RETURN_SENTINEL:
            state.update(kind="returned_before_group60_exit"); machine.emu_stop(); return
        if not (AC60_START <= address < AC60_END or any(a <= address < b for a,b in HELPER_RANGES)):
            state.update(kind="unexpected_code", detail=hex(address)); machine.emu_stop(); return
        pcs.append(address)

    def on_write(machine, _access, address, size, value, _user):
        pc = machine.reg_read(UC_X86_REG_EIP)
        if text_start <= address and address + size <= text_end:
            state.update(kind="write_code", detail={"pc":hex(pc),"address":hex(address)}); machine.emu_stop(); return
        if any(a <= address and address + size <= b for a,b in scratch): return
        writes.append({"pc_rva":hex(pc-EXE_BASE),"address":hex(address),"size":size,
                       "value":hex(value & ((1 << (size*8))-1))})

    def on_read(machine, _access, address, size, _value, _user):
        if address < PACKET + 0x100 and address + size > PACKET:
            if address < PACKET or address + size > PACKET + 8:
                state.update(kind="packet_read_out_of_bounds", detail={
                    "pc_rva":hex(machine.reg_read(UC_X86_REG_EIP)-EXE_BASE),
                    "address":hex(address),"size":size})
                machine.emu_stop()
                return
            packet_reads.append({
                "pc_rva":hex(machine.reg_read(UC_X86_REG_EIP)-EXE_BASE),
                "offset":hex(address-PACKET),"size":size})

    def on_invalid(machine, access, address, size, _value, _user):
        state.update(kind="invalid_memory", detail={"pc":hex(machine.reg_read(UC_X86_REG_EIP)-EXE_BASE),
                      "access":access,"address":hex(address),"size":size})
        machine.emu_stop(); return False

    uc.hook_add(UC_HOOK_CODE, on_code)
    uc.hook_add(UC_HOOK_MEM_READ, on_read)
    uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
    uc.hook_add(UC_HOOK_MEM_INVALID, on_invalid)
    try:
        uc.emu_start(AC60_START, 0, timeout=3_000_000)
    except UcError as error:
        if state["kind"] is None: state.update(kind="unicorn_error", detail=str(error))

    final_base = read32(uc, PAGE_BASE)
    page_write_rows = [w for w in writes if int(w["address"],16) == PAGE_BASE]
    tag_seen = bytes(uc.mem_read(PACKET+7,1))[0]
    status = state["kind"]
    if status not in ("group60_fallthrough_exit", "group60_gate_exit"):
        raise RuntimeError(f"tag={tag:#x} gate={gate_word:#x} stopped unexpectedly: {state}")
    if tag_seen != tag:
        raise RuntimeError("fixture tag changed")
    if bytes(uc.mem_read(GATE, 2)) != fixture[GATE]:
        raise RuntimeError("original changed the gate/canary bytes")
    for va, blob in fixture.items():
        if va not in (GATE, PAGE_BASE) and bytes(uc.mem_read(va,len(blob))) != blob:
            raise RuntimeError(f"original changed read-only fixture cell {va:#x}")
    if any(bytes(uc.mem_read(va,len(before))) != before for va,before in before_code.items()):
        raise RuntimeError("original code bytes changed")

    # Restore all changed .data bytes and setup cells inside the private emulator.
    after_data = bytes(uc.mem_read(data_va,data_size))
    changed = [i for i,(a,b) in enumerate(zip(before_data,after_data)) if a != b]
    for i in changed: uc.mem_write(data_va+i,before_data[i:i+1])
    for va, blob in old_fixture.items(): uc.mem_write(va,blob)
    if bytes(uc.mem_read(data_va,data_size)) != before_data:
        raise RuntimeError("emulator .data did not restore")

    return {
        "tag":hex(tag),"group":hex(tag&0xe0),"gate_word":hex(gate_word),
        "gate_bytes":struct.pack('<H',gate_word).hex(),"stop_rva":state["stop_pc"],
        "instructions":state["instructions"],"A909FC":hex(final_base),
        "A909FC_writes":page_write_rows,
        "packet_read_events":packet_reads,
        "max_packet_read_end":max((int(r["offset"],16)+r["size"] for r in packet_reads),default=0),
        "all_data_writes":writes,
        "raster_entry_seen":False,
        "unrelated_code_or_OS_endpoint_seen":False,
        "original_code_unchanged":True,"private_data_restored":True,
        "ac60_touched_packet_only_header":True
    }


def main():
    pe = PE(ARGS.exe)
    if pe.image_base != EXE_BASE or getattr(pe.pe,"DIRECTORY_ENTRY_BASERELOC",None):
        raise RuntimeError("pinned fixed-base x86 EXE assumptions failed")
    cases=[]
    for tag in (0x60,0x62,0x70,0x68):
        for gate in (0x0000,0x0001,0x0100):
            cases.append(run_case(pe,tag,gate))
    expected={
        (0x60,0):0x009BE6E0,(0x60,1):0x11223344,(0x60,0x100):0x009BE6E0,
        (0x62,0):0x009BE6E0,(0x62,1):0x11223344,(0x62,0x100):0x009BE6E0,
        (0x70,0):0x009BE6E0,(0x70,1):0x11223344,(0x70,0x100):0x009BE6E0,
        (0x68,0):0x009BE6E0,(0x68,1):0x11223344,(0x68,0x100):0x009BE6E0,
    }
    for row in cases:
        key=(int(row['tag'],16),int(row['gate_word'],16))
        if int(row['A909FC'],16)!=expected[key]:
            raise RuntimeError(f"prediction falsified at {key}: A909FC={row['A909FC']}")
    span=pe.read_rva(0xAC60,0xB2F0-0xAC60)
    receipt={
      'schema_version':1,'status':'BOUNDED_EQUIVALENT',
      'scope':'Finite original x86 AC60 group-0x60 preamble trace; stops at B119 after fallthrough compares or at ACA2 on the nonzero-byte early branch, before raster/tail. Tests only safe 8-byte heads for 0x60/0x62/0x70/0x68 and byte-gate vectors 0, 1, 0x100.',
      'prediction':json.loads((ROOT_HINT / 'renderer_contract/fixtures/group60/prediction.json').read_text(encoding='utf-8')),
      'prediction_sha256':hashlib.sha256((ROOT_HINT / 'renderer_contract/fixtures/group60/prediction.json').read_bytes()).hexdigest(),
      'runner_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
      'source_hashes':{name:hashlib.sha256((ROOT_HINT / name).read_bytes()).hexdigest() for name in
                       ('renderer_contract/tests/oracle_common.py','scripts/pe.py',
                        'renderer_contract/hercules_staging.c','renderer_contract/hercules_staging.h',
                        'renderer_contract/tests/test_unknown_group60.c')},
      'oracle':{'exe':str(ARGS.exe),'sha256':hashlib.sha256(ARGS.exe.read_bytes()).hexdigest(),
                'image_base':hex(pe.image_base),'ac60_rva':'0xac60','ac60_size':'0x690',
                'ac60_sha256':hashlib.sha256(span).hexdigest()},
      'execution':{**verify_runtime(unicorn,ARGS.deps),'architecture':'x86 32-bit',
                   'instructions_per_case_bounded':MAX_INSTRUCTIONS,
                   'original_code_modified':False,'fixture_data_only':True},
      'initial_page_state':{'A8E6F4':'0x0b','A8E6F8':'0','A8E6FC':'0','A909FC':'0x11223344'},
      'cases':cases,
      'conclusion':'All four tested group-0x60 tags refresh A909FC to 0x9be6e0 when BA01FC byte is zero, preserve it when that byte is one, and still refresh when only BA01FD is one. The observed preamble therefore precedes the 0x64/0x74/0x7c/0x68 comparisons and uses a byte gate for these finite cases.',
      'limitations':['Finite discriminator only; no producer or first-party occurrence claim for 0x60, 0x62, 0x70.','Only the 8-byte common header was allocated, and hooked reads are checked to end within that header; no draw or A8F0 body was executed.','No raster pixel, texture mutation, gameplay, or full AC60 exit behavior claimed.']
    }
    write_receipt(ARGS.receipt,receipt)
    print(f"BOUNDED_EQUIVALENT;cases={len(cases)};receipt={ARGS.receipt}")

if __name__=='__main__': main()
