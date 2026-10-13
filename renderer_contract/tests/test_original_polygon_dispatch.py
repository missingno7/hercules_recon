"""Portable finite polygon-dispatch test over pinned original x86 bytes."""
from __future__ import annotations
import hashlib
import json
import struct
import sys
from pathlib import Path

def find_repo_root(script_path: Path) -> Path:
    """Find the nearest checkout that owns the shared scripts and test helper."""
    for parent in script_path.resolve().parents:
        if (
            (parent / "scripts" / "pe.py").is_file()
            and (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file()
            and (parent / "work").is_dir()
        ):
            return parent
    raise RuntimeError(
        "cannot find repository ancestor with scripts/pe.py, "
        "renderer_contract/tests/oracle_common.py, and work/"
    )


ROOT = find_repo_root(Path(__file__))
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime, write_receipt

if not any(arg == "--receipt" or arg.startswith("--receipt=") for arg in sys.argv[1:]):
    raise SystemExit("--receipt is required and must name a file under <repo>/work/")

ARGS = parse_args(
    "Run finite original HERCULES.EXE polygon and rectangle-dispatch fixtures",
    "native_d420_rowzero.json",
)
if ARGS.original_root != ROOT:
    raise SystemExit(f"--original-root must name the detected repository root: {ROOT}")
try:
    ARGS.receipt.relative_to(ROOT / "work")
except ValueError as error:
    raise SystemExit("--receipt must stay inside the detected repository's work/ directory") from error
from pe import PE
import unicorn
from unicorn import (
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_PROT_EXEC,
    UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP

BASE = 0x400000
SCRATCH = 0x10000000
SCRATCH_SIZE = 0x4000
TEXTURE = SCRATCH + 0x100
SURFACE = SCRATCH + 0x1000
SURFACE_SIZE = 0x400
STACK = 0x20000000
STACK_SIZE = 0x10000
FRAME = STACK + 0x8000
SENTINEL = 0x30000000
MAX_INSNS = 100000
EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"

GATE = 0x0096E3E0
CLIP_SWEEP_MAX = 0x0043A48C
AVAILABLE = 0x00BA01F0
SURFACE_CELL = 0x00BA0210
STRIDE_CELL = 0x00BA0214
VERTEX_COUNT = 0x00A909F8
X = 0x00A909B8
Y = 0x00A909C8
U = 0x00A909D8
V = 0x00A909E8
TEX = 0x00A909FC
ROW_CODE = 0x00A90A04
CLIP_SWEEP_MIN = 0x00A909B4
SHADE_COLOR = 0x00A909AC
SHADE_LUT_BASE = 0x0091C488
SHADE_COLOR_INDEX = 0x23

WINDOW = (0xD420, 0xDD70)
ROWZERO_WINDOW = (0xD60C, 0xDD70)
CAB0_WINDOW = (0xCAB0, 0xCC45)
E680_WINDOW = (0xE680, 0xE7C7)
C8F0_WINDOW = (0xC8F0, 0xCAA8)
MODE_CELL = 0x00A90A00
TPAGE_CELL = 0x00A8E6F8
BLEND_BASE = 0x0092E3D0


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def align(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def perms(chars: int) -> int:
    out = 0
    if chars & 0x40000000:
        out |= UC_PROT_READ
    if chars & 0x80000000:
        out |= UC_PROT_WRITE
    if chars & 0x20000000:
        out |= UC_PROT_EXEC
    return out


def put32(uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def map_original(uc, pe) -> None:
    h = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE, align(h), UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:h])
    for section in pe.sections:
        size = align(max(section["virtual_size"], section["raw_size"]))
        va = BASE + section["virtual_address"]
        uc.mem_map(va, size, perms(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))


def find_data_region(pe):
    section = next(s for s in pe.sections if s["name"] == ".data")
    return BASE + section["virtual_address"], align(max(section["virtual_size"], section["raw_size"]))


class Fixture:
    def __init__(self, pe, name: str, gate: int, source: int,
                 quad_fastpath: bool = False, e680_fastpath: bool = False):
        self.name = name
        self.gate = gate
        self.source = source
        self.quad_fastpath = quad_fastpath or e680_fastpath
        self.e680_fastpath = e680_fastpath
        self.main_window = E680_WINDOW if e680_fastpath else WINDOW
        self.extra_window = (C8F0_WINDOW if e680_fastpath else
                             CAB0_WINDOW if quad_fastpath else None)
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        map_original(self.uc, pe)
        self.uc.mem_map(SCRATCH, SCRATCH_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(SENTINEL, 0x1000, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        self.uc.mem_write(SENTINEL, b"\x90")
        self.data_va, self.data_size = find_data_region(pe)
        self.original_data = bytes(self.uc.mem_read(self.data_va, self.data_size))
        self.code_before = bytes(self.uc.mem_read(BASE + self.main_window[0], self.main_window[1] - self.main_window[0]))
        self.extra_code_before = (bytes(self.uc.mem_read(BASE + self.extra_window[0], self.extra_window[1] - self.extra_window[0]))
                                  if self.extra_window else None)
        self.code_trace: list[int] = []
        self.code_edges: list[int] = []
        self.events: list[dict] = []
        self.data_write_count = 0
        self.shade_decoy_reads = 0
        self.blend_read_events: list[dict] = []
        self.error = None
        self.stop_seen = False
        self.uc.hook_add(UC_HOOK_CODE, self._code)
        self.uc.hook_add(UC_HOOK_MEM_READ, self._read)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self._write)
        self.uc.hook_add(UC_HOOK_MEM_INVALID, self._invalid)

        # Synthetic caller state: either the 3-vertex rowzero triangle or an
        # eligible 4-vertex unit-scale rectangle.  Both use an eight-byte row
        # stride and the original entry's global lane layout.
        put32(self.uc, AVAILABLE, 1)
        put32(self.uc, SURFACE_CELL, SURFACE)
        put32(self.uc, STRIDE_CELL, 8)
        put32(self.uc, VERTEX_COUNT, 4 if self.quad_fastpath else 3)
        xs = (1, 3, 3, 1) if self.quad_fastpath else (1, 3, 1, 0)
        ys = (1, 1, 3, 3) if self.quad_fastpath else (1, 1, 3, 0)
        us = (0, 0x00020000, 0x00020000, 0) if self.quad_fastpath else (0, 0, 0, 0)
        vs = (0, 0, 0x00020000, 0x00020000) if self.quad_fastpath else (0, 0, 0, 0)
        for i, value in enumerate(xs):
            put32(self.uc, X + 4 * i, value)
        for i, value in enumerate(ys):
            put32(self.uc, Y + 4 * i, value)
        for i in range(4):
            put32(self.uc, U + 4 * i, us[i])
            put32(self.uc, V + 4 * i, vs[i])
        put32(self.uc, TEX, TEXTURE)
        put32(self.uc, SHADE_COLOR, SHADE_COLOR_INDEX)
        put32(self.uc, ROW_CODE, 0x7800)  # SAR32(0x7800, 6) - 0x1e0 == 0
        put32(self.uc, CLIP_SWEEP_MIN, 0)
        put32(self.uc, CLIP_SWEEP_MAX, 240)
        put32(self.uc, GATE, gate)
        if e680_fastpath:
            put32(self.uc, MODE_CELL, 0x80)
            put32(self.uc, TPAGE_CELL, 2)
        if self.quad_fastpath:
            texels = bytearray(0x202)
            texels[0:2] = b"\x00\x5a"
            texels[0x100:0x102] = b"\x77\x6b"
            self.uc.mem_write(TEXTURE, bytes(texels))
        else:
            self.uc.mem_write(TEXTURE, bytes((source,)) + b"\x5A" * 15)
        self.shade_decoy = SHADE_LUT_BASE + SHADE_COLOR_INDEX * 256 + source
        self.shade_decoy_value = 0 if source else 0xC3
        self.uc.mem_write(self.shade_decoy, bytes((self.shade_decoy_value,)))
        self.blend_entries = {}
        if e680_fastpath:
            blend_table = BLEND_BASE + 2 * 0x10000
            for texel, result in ((0x5A, 0xE1), (0x77, 0xE2), (0x6B, 0xE3)):
                address = blend_table + (texel << 8) + 0xA5
                self.blend_entries[address] = result
                self.uc.mem_write(address, bytes((result,)))
        self.uc.mem_write(SURFACE, b"\xA5" * SURFACE_SIZE)
        self.initial_surface = bytes(self.uc.mem_read(SURFACE, SURFACE_SIZE))
        self.initial_data = bytes(self.uc.mem_read(self.data_va, self.data_size))

    def _code(self, uc, address, size, _):
        if address == SENTINEL:
            self.stop_seen = True
            uc.emu_stop()
            return
        rva = address - BASE
        in_main = self.main_window[0] <= rva < self.main_window[1]
        in_extra = self.extra_window is not None and self.extra_window[0] <= rva < self.extra_window[1]
        if not in_main and not in_extra:
            self.error = {"kind": "unexpected_code", "pc": hex(address), "rva": hex(rva),
                          "recent_rvas": [hex(x) for x in self.code_trace[-20:]],
                          "e680_count_at_branch": getattr(self, "e680_count_at_branch", None)}
            uc.emu_stop()
            return
        self.code_trace.append(rva)
        if self.e680_fastpath and rva == 0xE6CD:
            self.e680_count_at_branch = {"eax": hex(uc.reg_read(UC_X86_REG_EAX)),
                                         "count_cell": hex(int.from_bytes(uc.mem_read(VERTEX_COUNT, 4), "little")),
                                         "mode_cell": hex(int.from_bytes(uc.mem_read(MODE_CELL, 4), "little"))}
        if rva in (0xD549, 0xCAB0, 0xCBD6, 0xCBF4, 0xCBFB,
                   0xE7BB, 0xC8F0, 0xCA51, 0xCA58, 0xCA5B, 0xCA5E,
                   0xD555, 0xD583, 0xD589, 0xD595, 0xD597, 0xD600,
                   0xD60C, 0xDC8A, 0xDC92, 0xDC94, 0xDC96):
            self.code_edges.append(rva)

    def _read(self, uc, access, address, size, value, _):
        if address <= self.shade_decoy < address + size:
            self.shade_decoy_reads += 1
        for entry, result in self.blend_entries.items():
            if address <= entry < address + size:
                pc = uc.reg_read(UC_X86_REG_EIP) - BASE
                self.blend_read_events.append({"pc_rva": hex(pc), "address": hex(entry),
                                               "size": size, "value": hex(result)})
        if SURFACE <= address and address + size <= SURFACE + SURFACE_SIZE:
            pc = uc.reg_read(UC_X86_REG_EIP) - BASE
            raw = bytes(uc.mem_read(address, size))
            self.events.append({"kind": "surface_read", "pc_rva": hex(pc), "address": hex(address),
                                "size": size, "value": hex(int.from_bytes(raw, "little"))})
        if address < TEXTURE + 0x400 and address + size > TEXTURE:
            pc = uc.reg_read(UC_X86_REG_EIP) - BASE
            raw = bytes(uc.mem_read(address, size))
            self.events.append({"kind": "read", "pc_rva": hex(pc), "address": hex(address),
                                "size": size, "value": hex(int.from_bytes(raw, "little"))})

    def _write(self, uc, access, address, size, value, _):
        pc = uc.reg_read(UC_X86_REG_EIP) - BASE
        if self.data_va <= address and address + size <= self.data_va + self.data_size:
            self.data_write_count += 1
        elif SURFACE <= address and address + size <= SURFACE + SURFACE_SIZE:
            self.events.append({"kind": "surface_write", "pc_rva": hex(pc), "address": hex(address),
                                "size": size, "value": hex(value & ((1 << (size * 8)) - 1))})
        elif STACK <= address and address + size <= STACK + STACK_SIZE:
            pass
        else:
            self.error = {"kind": "unexpected_write", "pc_rva": hex(pc), "address": hex(address), "size": size}
            uc.emu_stop()

    def _invalid(self, uc, access, address, size, value, _):
        self.error = {"kind": "invalid_memory", "pc": hex(uc.reg_read(UC_X86_REG_EIP)),
                      "access": access, "address": hex(address), "size": size}
        uc.emu_stop()
        return False

    def run(self):
        put32(self.uc, FRAME, SENTINEL)
        self.uc.reg_write(UC_X86_REG_ESP, FRAME)
        self.uc.reg_write(UC_X86_REG_EBP, 0)
        self.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        try:
            self.uc.emu_start(BASE + self.main_window[0], 0, timeout=2_000_000, count=MAX_INSNS)
        except UcError as exc:
            if self.error is None:
                self.error = {"kind": "unicorn_error", "detail": str(exc),
                              "pc": hex(self.uc.reg_read(UC_X86_REG_EIP))}
        if self.error:
            raise AssertionError(f"{self.name}: {self.error}")
        if not self.stop_seen:
            raise AssertionError(f"{self.name}: execution did not return to sentinel; pc={self.uc.reg_read(UC_X86_REG_EIP):#x}")
        if self.e680_fastpath:
            if not all(rva in self.code_trace for rva in (0xE7BB, 0xC8F0, 0xCA51)):
                raise AssertionError(f"{self.name}: missed E680's C8F0 direct-blend branch: {self.code_edges!r}")
            if any(rva in self.code_trace for rva in (0xE7C7, 0xD555, 0xD60C, 0xD549, 0xCAB0)):
                raise AssertionError(f"{self.name}: eligible E680 quad unexpectedly entered fallback")
        elif self.quad_fastpath:
            if not all(rva in self.code_trace for rva in (0xD549, 0xCAB0, 0xCBD6, 0xCBF4)):
                raise AssertionError(f"{self.name}: missed D420's CAB0 direct-copy branch: {self.code_edges!r}")
            if any(rva in self.code_trace for rva in (0xD555, 0xD60C, 0xDC8A, 0xD589, 0xD600)):
                raise AssertionError(f"{self.name}: eligible quad unexpectedly entered general row/color path")
        else:
            if not all(rva in self.code_trace for rva in (0xD555, 0xD60C, 0xDC8A, 0xDC92)):
                raise AssertionError(f"{self.name}: missing rowzero path markers {self.code_edges!r}")
            if any(rva in self.code_trace for rva in (0xD589, 0xD600, 0xD549)):
                raise AssertionError(f"{self.name}: entered a non-rowzero service path")
        surface = bytes(self.uc.mem_read(SURFACE, SURFACE_SIZE))
        source_reads = [e for e in self.events if e["kind"] == "read"]
        surface_writes = [e for e in self.events if e["kind"] == "surface_write"]
        changed_offsets = [i for i, (a, b) in enumerate(zip(self.initial_surface, surface)) if a != b]
        if self.e680_fastpath:
            expected_read_values = [0, 0x5A, 0x77, 0x6B]
            expected_offsets = [10, 17, 18]
            if [int(e["value"], 16) for e in source_reads] != expected_read_values:
                raise AssertionError(f"{self.name}: C8F0 source samples {source_reads!r}")
            expected_blend = [
                {"pc_rva": "0xca5b", "address": hex(addr), "size": 1, "value": hex(value)}
                for addr, value in self.blend_entries.items()
            ]
            if self.blend_read_events != expected_blend:
                raise AssertionError(f"{self.name}: blend lookup stream {self.blend_read_events!r} != {expected_blend!r}")
            expected_surface_reads = [
                {"kind": "surface_read", "pc_rva": "0xca58", "address": hex(SURFACE + offset),
                 "size": 1, "value": "0xa5"}
                for offset in (10, 17, 18)
            ]
            got_surface_reads = [e for e in self.events if e["kind"] == "surface_read"]
            if got_surface_reads != expected_surface_reads:
                raise AssertionError(f"{self.name}: destination blend input reads {got_surface_reads!r}")
            expected_write_values = [0xE1, 0xE2, 0xE3]
            if [int(e["value"], 16) for e in surface_writes] != expected_write_values:
                raise AssertionError(f"{self.name}: blended surface writes {surface_writes!r}")
        elif self.quad_fastpath:
            expected_read_values = [0, 0x5A, 0x77, 0x6B]
            expected_offsets = [10, 17, 18]
            if [int(e["value"], 16) for e in source_reads] != expected_read_values:
                raise AssertionError(f"{self.name}: direct-copy source samples {source_reads!r}")
        else:
            if len(source_reads) != 4 or any(int(e["value"], 16) != self.source for e in source_reads):
                raise AssertionError(f"{self.name}: unexpected source sample stream {source_reads!r}")
            expected_offsets = [] if self.source == 0 else [9, 10, 17, 18]
        if changed_offsets != expected_offsets:
            raise AssertionError(f"{self.name}: destination offsets {changed_offsets!r} != {expected_offsets!r}")
        if self.shade_decoy_reads:
            raise AssertionError(f"{self.name}: source/colour decoy LUT was read")
        if not self.quad_fastpath:
            if int.from_bytes(self.uc.mem_read(ROW_CODE, 4), "little") != 0:
                raise AssertionError(f"{self.name}: row code did not normalize to zero")
            if (self.gate == 0 and not all(x in self.code_trace for x in (0xD595, 0xD597))) or (
                self.gate != 0 and 0xD583 not in self.code_trace
            ):
                raise AssertionError(f"{self.name}: gate-specific normalized-row-zero branch was not observed")
        elif int.from_bytes(self.uc.mem_read(ROW_CODE, 4), "little") != 0x7800:
            raise AssertionError(f"{self.name}: direct path unexpectedly changed row code")
        data_after = bytes(self.uc.mem_read(self.data_va, self.data_size))
        row_after = int.from_bytes(self.uc.mem_read(ROW_CODE, 4), "little")
        data_diff = []
        for i, (before, after) in enumerate(zip(self.initial_data, data_after)):
            if before != after:
                data_diff.append({"address": hex(self.data_va + i), "before": hex(before), "after": hex(after)})
        changed_ranges = []
        for item in data_diff:
            address = int(item["address"], 16)
            if changed_ranges and address == int(changed_ranges[-1]["end_exclusive"], 16):
                changed_ranges[-1]["end_exclusive"] = hex(address + 1)
            else:
                changed_ranges.append({"start": hex(address), "end_exclusive": hex(address + 1)})
        snapshots = [(self.main_window[0], self.main_window[1] - self.main_window[0], self.code_before)]
        if self.extra_window:
            snapshots.append((self.extra_window[0], self.extra_window[1] - self.extra_window[0], self.extra_code_before))
        for lo, size, before_code in snapshots:
            if bytes(self.uc.mem_read(BASE + lo, size)) != before_code:
                raise AssertionError(f"{self.name}: original code changed")
        self.uc.mem_write(self.data_va, self.original_data)
        if bytes(self.uc.mem_read(self.data_va, self.data_size)) != self.original_data:
            raise AssertionError(f"{self.name}: .data was not restored")
        return {
            "case": self.name,
            "entry_rva": hex(self.main_window[0]),
            "vertex_count": 4 if self.quad_fastpath else 3,
            "synthetic_vertices_xy": [[1, 1], [3, 1], [3, 3], [1, 3]] if self.quad_fastpath else [[1, 1], [3, 1], [1, 3]],
            "u_v_fixed16_per_vertex": [[0, 0], [0x20000, 0], [0x20000, 0x20000], [0, 0x20000]] if self.quad_fastpath else [[0, 0], [0, 0], [0, 0]],
            "normalized_row_input": "0x7800",
            "normalized_row_observed": row_after,
            "gate_value": hex(self.gate),
            "texture_sample_source_byte": hex(self.source),
            "shade_lut_decoy": {"address": hex(self.shade_decoy),
                                "value": hex(self.shade_decoy_value),
                                "read_count": self.shade_decoy_reads},
            "surface_before_sha256": sha(self.initial_surface),
            "surface_after_sha256": sha(surface),
            "surface_changed_bytes": [
                {"offset": i, "before": hex(a), "after": hex(b)}
                for i, (a, b) in enumerate(zip(self.initial_surface, surface)) if a != b
            ],
            "path_markers": [hex(x) for x in self.code_edges],
            "entered_d420_cab0": self.quad_fastpath and not self.e680_fastpath,
            "entered_e680_c8f0": self.e680_fastpath,
            "source_reads": source_reads,
            "surface_reads": [e for e in self.events if e["kind"] == "surface_read"],
            "surface_writes": surface_writes,
            "blend_table_reads": self.blend_read_events,
            "data_write_instruction_count": self.data_write_count,
            "data_mutation_byte_count": len(data_diff),
            "data_mutation_ranges": changed_ranges,
            "original_code_unchanged": True,
            "original_data_restored": True,
        }


def main():
    pe = PE(ARGS.exe)
    if sha(pe.data) != EXE_SHA or pe.image_base != BASE:
        raise AssertionError("pinned original EXE identity/base mismatch")
    if getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", None):
        raise AssertionError("unexpected base relocations")
    runtime = verify_runtime(unicorn, ARGS.deps)
    cases = [
        Fixture(pe, f"rowzero_gate_{gate}_{'zero' if source == 0 else 'nonzero'}_texel",
                gate=gate, source=source).run()
        for gate, source in ((0, 0), (0, 0x5A), (1, 0), (1, 0x5A))
    ]
    cases.append(Fixture(pe, "d420_eligible_quad_to_cab0", gate=0,
                         source=0x5A, quad_fastpath=True).run())
    cases.append(Fixture(pe, "e680_eligible_quad_to_c8f0", gate=1,
                         source=0x5A, e680_fastpath=True).run())
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": "Four finite original-byte CPU fixtures execute D420 through the normalized-row-zero polygon path using a synthetic 3-vertex right triangle; one eligible 4-vertex case executes D420→CAB0 and another executes E680→C8F0. The rowzero 2x2 matrix varies gate and source texel zero/nonzero. The direct-path fixtures seed conflicting shade-LUT entries; E680 additionally seeds three blend entries. No whole rasterizer, full renderer, GPU, DirectDraw, gameplay, source identity, FUNCTION_MATCH, or whole-module equality is claimed.",
        "oracle": {"path": str(pe.path), "sha256": sha(pe.data), "image_base": hex(pe.image_base),
                   "function_span": {"rva": "0xd420", "end_rva_exclusive": "0xdd70", "size": 0x950,
                                     "sha256": sha(pe.read_rva(0xD420, 0x950))},
                   "rowzero_span": {"rva": "0xd60c", "end_rva_exclusive": "0xdd70", "size": 0x764,
                                    "sha256": sha(pe.read_rva(0xD60C, 0x764))},
                   "rowzero_pixel_loop": {"rva": "0xdc8a", "end_rva_exclusive": "0xdc9e", "size": 0x14,
                                          "sha256": sha(pe.read_rva(0xDC8A, 0x14))},
                   "cab0_entry_to_return": {"rva": "0xcab0", "end_rva_exclusive": "0xcc45", "size": 0x195,
                                            "sha256": sha(pe.read_rva(0xCAB0, 0x195))},
                   "e680_dispatch": {"rva": "0xe680", "end_rva_exclusive": "0xe7c7", "size": 0x147,
                                     "sha256": sha(pe.read_rva(0xE680, 0x147))},
                   "c8f0_entry_to_return": {"rva": "0xc8f0", "end_rva_exclusive": "0xcaa8", "size": 0x1b8,
                                            "sha256": sha(pe.read_rva(0xC8F0, 0x1B8))}},
        "execution": {"engine": "Unicorn CPU emulator", "architecture": "x86 32-bit",
                      "python_executable": sys.executable, "python_version": sys.version.split()[0],
                      "script_sha256": sha(Path(__file__).read_bytes()),
                      "all_original_sections_mapped_at_preferred_base": True,
                      "instruction_budget": MAX_INSNS, "no_original_code_written": True,
                      "unexpected_pc_or_memory_access_fails": True,
                      "data_section_restored_each_case": True,
                      "all_synthetic_geometry_texture_and_surface_state_is_emulator_only": True},
        "runtime": runtime,
        "cases": cases,
        "limits": [
            "Both fixtures use constant U=V=0 and a 2x2 right triangle; they do not characterize arbitrary edge slopes, clipping boundaries, or UV gradients.",
            "The nonzero fixture proves direct sampling and write on this finite polygon; it does not assert general perspective/affine texture semantics.",
            "The task's row-zero fixtures bypass the separate CC50 palette and C2E0 fill paths. Their behaviors are independently covered by work/renderer_next/pixel_oracles/native_report.json.",
            "The eligible-quad case uses one small in-bounds 2x2 rectangle and verifies D420 reaches CAB0 while bypassing D555/D60C; it does not cover other dimensions, clipped rectangles, negative coordinates, or general quadrilateral contours.",
            "The E680 case uses mode 0x80, tpage selector 2, one in-bounds 2x2 rectangle, and three synthetic blend entries; it verifies the direct C8F0 caller route and selected lookups, not other modes, selectors, or rectangle boundaries.",
        ],
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
                       "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
                       "receipt": str(ARGS.receipt)},
    }
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({"status": receipt["status"],
                      "cases": [{"case": c["case"], "path_markers": c["path_markers"],
                                 "surface_changed_bytes": c["surface_changed_bytes"],
                                 "source_reads": c["source_reads"],
                                 "surface_writes": c["surface_writes"],
                                 "shade_lut_decoy": c["shade_lut_decoy"]} for c in cases],
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
