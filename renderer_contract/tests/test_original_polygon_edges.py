"""Finite original-byte polygon coverage and Q16 interpolation probes."""
from __future__ import annotations

import hashlib
import argparse
import inspect
import json
import os
import sys
from fractions import Fraction
from pathlib import Path


def find_root(path: Path) -> Path:
    for parent in (path.resolve(), *path.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("cannot find repository root with scripts/pe.py and public oracle_common.py")


ROOT = find_root(Path(__file__))
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import (  # noqa: E402
    DEFAULT_ANALYSIS_DEPS, DEFAULT_DEPS, EXPECTED_EXE_SHA256,
    verify_runtime, write_receipt,
)


def parse_public_args():
    parser = argparse.ArgumentParser(
        description="Run finite original-byte polygon edge and UV contract cases")
    parser.add_argument("--exe", required=True, type=Path,
                        help="pinned original HERCULES.EXE")
    parser.add_argument("--receipt", required=True, type=Path,
                        help="compact JSON receipt path inside this repository's work directory")
    parser.add_argument("--original-root", type=Path,
                        help="repository root; inferred from this script when omitted")
    parser.add_argument("--deps", type=Path, default=DEFAULT_DEPS)
    parser.add_argument("--analysis-deps", type=Path, default=DEFAULT_ANALYSIS_DEPS)
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    args.receipt = args.receipt.resolve()
    args.original_root = args.original_root.resolve() if args.original_root else ROOT
    args.deps = args.deps.resolve()
    args.analysis_deps = args.analysis_deps.resolve()
    if args.original_root != ROOT:
        parser.error("--original-root must name the repository detected from this script")
    try:
        args.receipt.relative_to((ROOT / "work").resolve())
    except ValueError:
        parser.error("--receipt must stay inside the repository's ignored work directory")
    if hashlib.sha256(args.exe.read_bytes()).hexdigest() != EXPECTED_EXE_SHA256:
        parser.error("--exe does not match the pinned original HERCULES.EXE SHA-256")
    if not args.deps.is_dir():
        parser.error(f"Unicorn dependency directory does not exist: {args.deps}")
    if not args.analysis_deps.is_dir():
        parser.error(f"analysis dependency directory does not exist: {args.analysis_deps}")
    os.environ["HERCULES_PYTHON_DEPS"] = str(args.analysis_deps)
    sys.path[:0] = [str(args.deps), str(ROOT / "scripts")]
    return args


ARGS = parse_public_args()

from pe import PE  # noqa: E402
import capstone  # noqa: E402
import unicorn  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32,
    UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EBP, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP,
)


BASE = 0x400000
EXE_SHA = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
STACK = 0x20000000
STACK_SIZE = 0x10000
FRAME = STACK + 0x8000
SENTINEL = 0x30000000
SCRATCH = 0x10000000
SCRATCH_SIZE = 0x40000
TEXTURE = SCRATCH + 0x1000
SURFACE = SCRATCH + 0x20000
SURFACE_STRIDE = 32
SURFACE_HEIGHT = 24
SURFACE_SIZE = SURFACE_STRIDE * SURFACE_HEIGHT
CLUT_BASE = 0x92C390
F3_LEFT = 0xA909B0
F3_TOP = 0xA909B4
F3_RIGHT = 0x43A488
F3_BOTTOM = 0x43A48C
ROW_CODE = 0xA90A04
ROWZERO_ENCODED = 0x7800
CLUT3_ENCODED = (0x1E0 + 3) << 6
GATE = 0x96E3E0
DRAW_GATE = 0xBA01F0
SURFACE_CELL = 0xBA0210
STRIDE_CELL = 0xBA0214
VERTEX_COUNT = 0xA909F8
X_BASE = 0xA909B8
Y_BASE = 0xA909C8
U_BASE = 0xA909D8
V_BASE = 0xA909E8
TEXTURE_CELL = 0xA909FC
MAX_INSNS = 250000
RECIP_INIT_START = 0x72A0
RECIP_INIT_STOP = 0x72CC  # stop before the following host callback setup
RECIP_TABLE = 0xA8E918
RECIP_COUNT = (0xA90918 - RECIP_TABLE) // 4
CODE_WINDOWS = ((0x72A0, 0x72CC), (0xCC50, 0xD415), (0xD420, 0xDD70))

EXPECTED_SPANS = {
    "reciprocal_table_initializer": (0x72A0, 0x72CC,
        "3d59be43f3756224228dfdcc372a3bc59b7a7dbba4bbab8c79dd821cc958014b"),
    "cc50_active_palette_function": (0xCC50, 0xD415,
        "7504054c2fe867eef6602dbd439f1972b55a73540b98099498772db2f34b7fd0"),
    "cc50_edge_and_scan_setup": (0xCC50, 0xD2C6,
        "4f4bf72270e5dedecc4edeed63348276d22b32a648668de0d9e5f4dff82762f1"),
    "cc50_pixel_loop": (0xD2C6, 0xD349,
        "86688a0be90e8b5b3c931c4f789d6dd6fc53575085f9ee700362dbf3617b6d1f"),
    "d420_dispatch_and_rowzero_function": (0xD420, 0xDD70,
        "4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e"),
    "d60c_rowzero_function": (0xD60C, 0xDD70,
        "2ed9dc4341703ee82f789528b427f6333e6d916bec11148fe2cfecff14119b4a"),
    "d60c_bounds_clip": (0xD60C, 0xD776,
        "08d6139e8e4c3466d4261d9f32990718bfba8d43420a43869937e7ee02cc57b1"),
    "d60c_edge_and_attribute_setup": (0xD776, 0xDA7D,
        "9ac21057935e83d6d3b681286cabfa30ebb7b32f6d3169b84cfc183cc1e53d17"),
    "d60c_span_clipping": (0xDA7D, 0xDC28,
        "1368d5cf4feaa06e50f286e7add5aaafc137cc9d85413061361129f7c645fdaf"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def u32(value: int) -> int:
    return value & 0xFFFFFFFF


def align(value: int) -> int:
    return (value + 0xFFF) & ~0xFFF


def put32(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, u32(value).to_bytes(4, "little"))


def section_perms(characteristics: int) -> int:
    result = 0
    if characteristics & 0x40000000:
        result |= UC_PROT_READ
    if characteristics & 0x80000000:
        result |= UC_PROT_WRITE
    if characteristics & 0x20000000:
        result |= UC_PROT_EXEC
    return result


def map_original(uc: Uc, pe: PE) -> tuple[int, int]:
    header_size = pe.pe.OPTIONAL_HEADER.SizeOfHeaders
    uc.mem_map(BASE, align(header_size), UC_PROT_READ)
    uc.mem_write(BASE, pe.data[:header_size])
    data_region = None
    for section in pe.sections:
        virtual_size = max(section["virtual_size"], section["raw_size"])
        size = align(virtual_size)
        va = BASE + section["virtual_address"]
        uc.mem_map(va, size, section_perms(section["characteristics"]))
        if section["raw_size"]:
            uc.mem_write(va, pe.read_rva(section["virtual_address"], section["raw_size"]))
        if size > section["raw_size"]:
            uc.mem_write(va + section["raw_size"], b"\0" * (size - section["raw_size"]))
        if section["name"] == ".data":
            data_region = (va, virtual_size)
    if data_region is None:
        raise AssertionError("original image has no .data section")
    return data_region


def cross(a, b, p) -> Fraction:
    return Fraction((b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]))


def native_edge_model_expected(vertices, clip) -> list[tuple[int, int]]:
    """Independent integer-scan model: y in [top,bottom), floor(xL) <= x < floor(xR)."""
    _left_clip, top_clip, _right_clip, bottom_clip = clip
    min_y = max(min(v[1] for v in vertices), top_clip)
    max_y = min(max(v[1] for v in vertices), bottom_clip)
    result = []
    for y in range(min_y, max_y):
        intersections = []
        for i, a in enumerate(vertices):
            b = vertices[(i + 1) % len(vertices)]
            if a[1] == b[1]:
                continue
            low, high = (a, b) if a[1] < b[1] else (b, a)
            if low[1] <= y < high[1]:
                intersections.append(Fraction(low[0]) +
                    (y - low[1]) * Fraction(high[0] - low[0], high[1] - low[1]))
        if len(intersections) < 2:
            continue
        x_left, x_right = min(intersections), max(intersections)
        # The native SAR x,16 conversion floors both 16.16 span endpoints.
        x_start = max(x_left.numerator // x_left.denominator, 0)
        x_end = min(x_right.numerator // x_right.denominator, 320)
        result.extend((x, y) for x in range(x_start, x_end))
    return result


def expected_uv_integer(vertices, attributes, pixel) -> tuple[int, int]:
    """Exact rational barycentric Q16 values evaluated at integer (x,y)."""
    u_fixed, v_fixed = exact_barycentric_q16(vertices, attributes, pixel)
    return ((u_fixed // 65536) & 0xFF, (v_fixed // 65536) & 0xFF)


def exact_barycentric_q16(vertices, attributes, pixel) -> tuple[Fraction, Fraction]:
    """Return unrounded signed Q16 values at an exact integer-coordinate sample."""
    points = [(x, y) for x, y in vertices]
    p = (Fraction(pixel[0]), Fraction(pixel[1]))
    area = cross(points[0], points[1], points[2])
    weights = (cross(points[1], points[2], p) / area,
               cross(points[2], points[0], p) / area,
               cross(points[0], points[1], p) / area)
    u_fixed = sum(weights[i] * attributes[i][0] for i in range(3))
    v_fixed = sum(weights[i] * attributes[i][1] for i in range(3))
    return u_fixed, v_fixed


def expected_uv_edge_anchored(vertices, attributes, pixel) -> tuple[int, int]:
    """Interpolate edge UV at integer y, then index span steps from floor(left_x)."""
    y = pixel[1]
    intersections = []
    for i, a in enumerate(vertices):
        j = (i + 1) % len(vertices)
        b = vertices[j]
        if a[1] == b[1]:
            continue
        low_i, high_i = (i, j)
        low, high = a, b
        if low[1] > high[1]:
            low_i, high_i = high_i, low_i
            low, high = high, low
        if low[1] <= y < high[1]:
            t = Fraction(y - low[1], high[1] - low[1])
            x_at_y = Fraction(low[0]) + t * (high[0] - low[0])
            uv_at_y = tuple(Fraction(attributes[low_i][k]) +
                            t * (attributes[high_i][k] - attributes[low_i][k])
                            for k in range(2))
            intersections.append((x_at_y, uv_at_y))
    if len(intersections) < 2:
        raise AssertionError(f"pixel {pixel} has no active triangle span")
    (x_left, uv_left), (x_right, uv_right) = sorted(intersections, key=lambda row: row[0])
    x_start = x_left.numerator // x_left.denominator
    span_width = x_right - x_left
    if span_width == 0:
        raise AssertionError(f"pixel {pixel} lies on a zero-width span")
    values = []
    for component in range(2):
        step = (uv_right[component] - uv_left[component]) / span_width
        sample = uv_left[component] + (pixel[0] - x_start) * step
        values.append((sample // 65536) & 0xFF)
    return tuple(values)


def expected_uv_integer_reciprocal(vertices, attributes, pixel) -> tuple[int, int]:
    """Reproduce observed Q16 edge/span stepping with the original table formula."""
    y = pixel[1]
    intersections = []
    for i, a in enumerate(vertices):
        j = (i + 1) % len(vertices)
        b = vertices[j]
        if a[1] == b[1]:
            continue
        low_i, high_i = (i, j)
        low, high = a, b
        if low[1] > high[1]:
            low_i, high_i = high_i, low_i
            low, high = high, low
        if low[1] <= y < high[1]:
            dy = high[1] - low[1]
            reciprocal = (1 << 30) // dy
            delta_x = (high[0] - low[0]) << 16
            x_step = (delta_x * reciprocal) // (1 << 30)
            uv_steps = tuple(((attributes[high_i][k] - attributes[low_i][k]) * reciprocal) // (1 << 30)
                             for k in range(2))
            distance = y - low[1]
            x_q16 = (low[0] << 16) + x_step * distance
            uv_q16 = tuple(attributes[low_i][k] + uv_steps[k] * distance for k in range(2))
            intersections.append((x_q16, uv_q16))
    if len(intersections) < 2:
        raise AssertionError(f"pixel {pixel} has no active triangle span")
    (x_left_q16, uv_left), (x_right_q16, uv_right) = sorted(intersections, key=lambda row: row[0])
    x_start = x_left_q16 // 65536
    x_end = x_right_q16 // 65536
    width = x_end - x_start
    if width <= 0:
        raise AssertionError(f"pixel {pixel} lies on a zero-width integer span")
    reciprocal = (1 << 30) // width
    steps = tuple(((uv_right[k] - uv_left[k]) * reciprocal) // (1 << 30) for k in range(2))
    clipped_start = max(x_start, 0)
    initial_uv = tuple(uv_left[k] + steps[k] * (clipped_start - x_start) for k in range(2))
    values = tuple(((initial_uv[k] + steps[k] * (pixel[0] - clipped_start)) // 65536) & 0xFF
                   for k in range(2))
    return values


class NativeCase:
    def __init__(self, pe: PE, data_region: tuple[int, int], spec: dict, route: str):
        self.spec = spec
        self.route = route
        self.rowzero = route == "d60c_rowzero"
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.data_va, self.data_size = map_original(self.uc, pe)
        if (self.data_va, self.data_size) != data_region:
            raise AssertionError("unexpected PE .data region")
        self.uc.mem_map(SCRATCH, SCRATCH_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(STACK, STACK_SIZE, UC_PROT_READ | UC_PROT_WRITE)
        self.uc.mem_map(SENTINEL, 0x1000, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC)
        self.uc.mem_write(SENTINEL, b"\x90")
        self.error = None
        self.stopped = False
        self.reciprocal_init_stopped = False
        self.trace = []
        self.source_reads = []
        self.palette_reads = []
        self.surface_reads = []
        self.surface_writes = []
        self.reciprocal_table_writes = []
        self.reciprocal_stack_writes = []
        self.row = 0 if self.rowzero else 3
        self.texture = bytearray(0x10000)
        if spec.get("uv_mode") == "fractional_gradient":
            for v in range(256):
                for u in range(256):
                    self.texture[v * 256 + u] = 1 + ((u + 7 * v) % 250)
        else:
            self.texture[:] = bytes([0x5A]) * len(self.texture)
        self.uc.mem_write(TEXTURE, bytes(self.texture))
        self.initial_surface = bytes([0xA5]) * SURFACE_SIZE
        self.uc.mem_write(SURFACE, self.initial_surface)
        self.palette_base = CLUT_BASE + self.row * 256
        if not self.rowzero:
            self.uc.mem_write(self.palette_base, bytes(range(256)))

        put32(self.uc, DRAW_GATE, 1)
        put32(self.uc, SURFACE_CELL, SURFACE)
        put32(self.uc, STRIDE_CELL, SURFACE_STRIDE)
        put32(self.uc, GATE, 1)
        put32(self.uc, VERTEX_COUNT, len(spec["vertices"]))
        put32(self.uc, TEXTURE_CELL, TEXTURE)
        put32(self.uc, ROW_CODE, ROWZERO_ENCODED if self.rowzero else CLUT3_ENCODED)
        left, top, right, bottom = spec["clip"]
        put32(self.uc, F3_LEFT, left)
        put32(self.uc, F3_TOP, top)
        put32(self.uc, F3_RIGHT, right)
        put32(self.uc, F3_BOTTOM, bottom)
        attrs = spec.get("attributes") or [(0, 0)] * len(spec["vertices"])
        for i in range(4):
            if i < len(spec["vertices"]):
                x, y = spec["vertices"][i]
                u, v = attrs[i]
            else:
                x = y = u = v = 0
            put32(self.uc, X_BASE + 4 * i, x)
            put32(self.uc, Y_BASE + 4 * i, y)
            put32(self.uc, U_BASE + 4 * i, u)
            put32(self.uc, V_BASE + 4 * i, v)

        self.code_snapshot = {
            (start, end): bytes(self.uc.mem_read(BASE + start, end - start))
            for start, end in ((0x72A0, 0x72CC), (0xCC50, 0xD415), (0xD420, 0xDD70))
        }
        self.uc.hook_add(UC_HOOK_CODE, self._code)
        self.uc.hook_add(UC_HOOK_MEM_READ, self._read)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self._write)
        self.uc.hook_add(UC_HOOK_MEM_INVALID, self._invalid)

    def _code(self, uc, address, size, _):
        if address == BASE + RECIP_INIT_STOP:
            self.reciprocal_init_stopped = True
            uc.emu_stop()
            return
        if address == SENTINEL:
            self.stopped = True
            uc.emu_stop()
            return
        rva = address - BASE
        if not any(lo <= rva < hi for lo, hi in CODE_WINDOWS):
            self.error = {"kind": "unexpected_code", "rva": hex(rva),
                          "recent": [hex(x) for x in self.trace[-12:]]}
            uc.emu_stop()
            return
        self.trace.append(rva)

    def _read(self, uc, access, address, size, value, _):
        end = address + size
        in_data = self.data_va <= address and end <= self.data_va + self.data_size
        in_scratch = SCRATCH <= address and end <= SCRATCH + SCRATCH_SIZE
        in_stack = STACK <= address and end <= STACK + STACK_SIZE
        if not (in_data or in_scratch or in_stack):
            self.error = {"kind": "unexpected_read", "pc": hex(uc.reg_read(UC_X86_REG_EIP) - BASE),
                          "address": hex(address), "size": size}
            uc.emu_stop()
            return
        pc = uc.reg_read(UC_X86_REG_EIP) - BASE
        if TEXTURE <= address and end <= TEXTURE + 0x10000:
            self.source_reads.append({"pc": hex(pc), "offset": address - TEXTURE,
                                      "u": (address - TEXTURE) & 255,
                                      "v": ((address - TEXTURE) >> 8) & 255,
                                      "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little")})
        if self.palette_base <= address and end <= self.palette_base + 256:
            self.palette_reads.append({"pc": hex(pc), "index": address - self.palette_base,
                                      "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little")})
        if SURFACE <= address and end <= SURFACE + SURFACE_SIZE:
            self.surface_reads.append({"pc": hex(pc), "offset": address - SURFACE,
                                       "value": int.from_bytes(bytes(uc.mem_read(address, size)), "little")})

    def _write(self, uc, access, address, size, value, _):
        end = address + size
        in_data = self.data_va <= address and end <= self.data_va + self.data_size
        in_scratch = SCRATCH <= address and end <= SCRATCH + SCRATCH_SIZE
        in_stack = STACK <= address and end <= STACK + STACK_SIZE
        if not (in_data or in_scratch or in_stack):
            self.error = {"kind": "unexpected_write", "pc": hex(uc.reg_read(UC_X86_REG_EIP) - BASE),
                          "address": hex(address), "size": size}
            uc.emu_stop()
            return
        if SURFACE <= address and end <= SURFACE + SURFACE_SIZE:
            self.surface_writes.append({"pc": hex(uc.reg_read(UC_X86_REG_EIP) - BASE),
                                        "offset": address - SURFACE,
                                        "value": value & ((1 << (size * 8)) - 1)})
        pc = uc.reg_read(UC_X86_REG_EIP) - BASE
        if RECIP_INIT_START <= pc < RECIP_INIT_STOP:
            write = {"pc": hex(pc), "address": address, "size": size}
            if RECIP_TABLE <= address < RECIP_TABLE + RECIP_COUNT * 4:
                self.reciprocal_table_writes.append(write)
            else:
                self.reciprocal_stack_writes.append(write)

    def _invalid(self, uc, access, address, size, value, _):
        self.error = {"kind": "invalid_memory", "pc": hex(uc.reg_read(UC_X86_REG_EIP) - BASE),
                      "access": access, "address": hex(address), "size": size}
        uc.emu_stop()
        return False

    def run(self) -> dict:
        # The original image stores this lookup table in zero-filled .data;
        # run only its bounded original initializer before entering D420.
        self.uc.reg_write(UC_X86_REG_ESP, FRAME)
        self.uc.reg_write(UC_X86_REG_EBP, 0)
        self.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        try:
            self.uc.emu_start(BASE + RECIP_INIT_START, 0,
                              timeout=2_000_000, count=32768)
        except UcError as error:
            if self.error is None:
                self.error = {"kind": "reciprocal_initializer_error", "detail": str(error),
                              "pc": hex(self.uc.reg_read(UC_X86_REG_EIP) - BASE)}
        if self.error:
            raise AssertionError(f"{self.spec['name']} {self.route}: {self.error}")
        if not self.reciprocal_init_stopped:
            raise AssertionError(f"{self.spec['name']} {self.route}: reciprocal initializer did not stop at 0x72cc; "
                                 f"eip={self.uc.reg_read(UC_X86_REG_EIP) - BASE:#x}; "
                                 f"trace={self.trace[-8:]}")
        table = bytes(self.uc.mem_read(RECIP_TABLE, RECIP_COUNT * 4))
        if int.from_bytes(table[0:4], "little") != 0:
            raise AssertionError("reciprocal table entry 0 was not initialized to zero")
        if len(self.reciprocal_table_writes) != RECIP_COUNT:
            raise AssertionError(f"reciprocal initializer wrote {len(self.reciprocal_table_writes)} cells, "
                                 f"expected {RECIP_COUNT}")
        if any(w["size"] != 4 or not RECIP_TABLE <= w["address"] < RECIP_TABLE + RECIP_COUNT * 4
               for w in self.reciprocal_table_writes):
            raise AssertionError("reciprocal initializer wrote outside the pinned table")
        if any(w["size"] != 4 or not STACK <= w["address"] < STACK + STACK_SIZE
               for w in self.reciprocal_stack_writes):
            raise AssertionError("reciprocal initializer wrote outside its table and synthetic stack")
        for index in (1, 2, 3, RECIP_COUNT - 1):
            expected = 0x40000000 // index
            actual = int.from_bytes(table[index * 4:index * 4 + 4], "little")
            if actual != expected:
                raise AssertionError(f"reciprocal table[{index}]={actual:#x}, expected {expected:#x}")
        self.reciprocal_init_trace = self.trace[:]
        self.trace.clear()
        put32(self.uc, FRAME, SENTINEL)
        self.uc.reg_write(UC_X86_REG_ESP, FRAME)
        self.uc.reg_write(UC_X86_REG_EBP, 0)
        self.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        try:
            self.uc.emu_start(BASE + 0xD420, 0, timeout=2_000_000, count=MAX_INSNS)
        except UcError as error:
            if self.error is None:
                self.error = {"kind": "unicorn_error", "detail": str(error),
                              "pc": hex(self.uc.reg_read(UC_X86_REG_EIP) - BASE)}
        if self.error:
            raise AssertionError(f"{self.spec['name']} {self.route}: {self.error}")
        if not self.stopped:
            raise AssertionError(f"{self.spec['name']} {self.route}: did not reach sentinel")
        for (start, end), before in self.code_snapshot.items():
            if bytes(self.uc.mem_read(BASE + start, end - start)) != before:
                raise AssertionError("original code bytes changed in emulator")
        actual_pixels = sorted((event["offset"] % SURFACE_STRIDE,
                                event["offset"] // SURFACE_STRIDE)
                               for event in self.surface_writes)
        actual_pixels = sorted(set(actual_pixels), key=lambda p: (p[1], p[0]))
        surface = bytes(self.uc.mem_read(SURFACE, SURFACE_SIZE))
        pixel_values = [{"x": x, "y": y, "value": surface[y * SURFACE_STRIDE + x]}
                        for x, y in actual_pixels]
        expected_screen = native_edge_model_expected(self.spec["vertices"], self.spec["clip"])
        actual_set = set(actual_pixels)
        screen_set = set(expected_screen)
        missing = sorted(screen_set - actual_set, key=lambda p: (p[1], p[0]))
        extra = sorted(actual_set - screen_set, key=lambda p: (p[1], p[0]))
        attributes = self.spec.get("attributes")
        uv_samples = []
        if attributes is not None:
            for write, source in zip(sorted(self.surface_writes, key=lambda e: e["offset"]),
                                     self.source_reads):
                pixel = (write["offset"] % SURFACE_STRIDE, write["offset"] // SURFACE_STRIDE)
                barycentric_uv = expected_uv_integer(self.spec["vertices"], attributes, pixel)
                span_model_uv = expected_uv_edge_anchored(self.spec["vertices"], attributes, pixel)
                fixed_step_uv = expected_uv_integer_reciprocal(self.spec["vertices"], attributes, pixel)
                uv_samples.append({"pixel": pixel, "native_uv": [source["u"], source["v"]],
                                   "integer_barycentric_uv": list(barycentric_uv),
                                   "matches_integer_barycentric": [source["u"], source["v"]] == list(barycentric_uv),
                                   "edge_anchored_span_uv": list(span_model_uv),
                                   "integer_reciprocal_q16_uv": list(fixed_step_uv),
                                   "matches_integer_reciprocal_q16_model": [source["u"], source["v"]] == list(fixed_step_uv),
                                   "matches_edge_anchored_span_model": [source["u"], source["v"]] == list(span_model_uv)})
        return {
            "case": self.spec["name"],
            "route": self.route,
            "entry_rva": "0xd420",
            "runtime_precondition": {
                "reciprocal_initializer_span": [hex(RECIP_INIT_START), hex(RECIP_INIT_STOP)],
                "initializer_instruction_count": len(self.reciprocal_init_trace),
                "table_va": hex(RECIP_TABLE), "entry_count": RECIP_COUNT,
                "checked_entries": {str(i): hex(int.from_bytes(table[i * 4:i * 4 + 4], "little"))
                                    for i in (0, 1, 2, 3, RECIP_COUNT - 1)},
            },
            "path_markers": sorted({hex(rva) for rva in self.trace
                                     if rva in (0xCC50, 0xD555, 0xD583, 0xD589, 0xD60C, 0xDC8A, 0xDC96)}),
            "vertices_xy": self.spec["vertices"],
            "attributes_uv_q16": attributes or [[0, 0]] * len(self.spec["vertices"]),
            "attribute_domain": self.spec.get("attribute_domain", "wire_unsigned_u8_q16" if attributes else "none"),
            "f3_clip_values": dict(zip(("left", "top", "right", "bottom"), self.spec["clip"])),
            "expected_pixel_set_integer_scan_floor_span_model": [list(p) for p in expected_screen],
            "native_pixel_set": [list(p) for p in actual_pixels],
            "coverage_match": not missing and not extra,
            "missing_from_native": [list(p) for p in missing],
            "extra_in_native": [list(p) for p in extra],
            "pixel_values": pixel_values,
            "source_uv_reads": self.source_reads,
            "palette_reads": self.palette_reads,
            "surface_read_count": len(self.surface_reads),
            "surface_write_count": len(self.surface_writes),
            "uv_samples_against_independent_edge_models": uv_samples,
            "original_code_unchanged": True,
        }


def static_refs(pe: PE, start: int, end: int) -> list[dict]:
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    rows = []
    for insn in md.disasm(pe.read_rva(start, end - start), BASE + start):
        for operand in insn.operands:
            if operand.type == capstone.x86.X86_OP_MEM and operand.mem.base == 0 and operand.mem.index == 0:
                address = operand.mem.disp & 0xFFFFFFFF
                if address in (F3_LEFT, F3_TOP, F3_RIGHT, F3_BOTTOM):
                    rows.append({"rva": hex(insn.address - BASE),
                                 "address": hex(address), "width": operand.size})
    return rows


def main() -> None:
    pe = PE(ARGS.exe)
    if sha(pe.data) != EXE_SHA or pe.image_base != BASE:
        raise AssertionError("pinned original EXE identity/base mismatch")
    if capstone.__version__ != "5.0.7":
        raise AssertionError(f"expected Capstone 5.0.7; got {capstone.__version__}")
    data_region = next((BASE + s["virtual_address"], max(s["virtual_size"], s["raw_size"]))
                       for s in pe.sections if s["name"] == ".data")
    spans = {}
    for name, (start, end, digest) in EXPECTED_SPANS.items():
        actual = sha(pe.read_rva(start, end - start))
        if actual != digest:
            raise AssertionError(f"{name} SHA-256 {actual} != {digest}")
        spans[name] = {"start_rva": hex(start), "end_rva_exclusive": hex(end),
                       "size": end - start, "sha256": actual}

    specs = [
        {"name": "flat_top", "vertices": [(2, 2), (6, 2), (4, 6)], "clip": (0, 0, 320, 240)},
        {"name": "flat_bottom", "vertices": [(4, 2), (2, 6), (6, 6)], "clip": (0, 0, 320, 240)},
        {"name": "shared_edge_upper", "vertices": [(2, 2), (6, 2), (6, 6)], "clip": (0, 0, 320, 240)},
        {"name": "shared_edge_lower", "vertices": [(2, 2), (6, 6), (2, 6)], "clip": (0, 0, 320, 240)},
        {"name": "winding_reversed_flat_top", "vertices": [(4, 6), (6, 2), (2, 2)], "clip": (0, 0, 320, 240)},
        {"name": "negative_xy_f3_clipped", "vertices": [(-2, -2), (5, 1), (1, 6)], "clip": (2, 1, 4, 5)},
        {"name": "negative_xy_q16_wire_valid", "vertices": [(-2, -2), (5, 1), (1, 6)],
         "clip": (2, 1, 4, 5), "uv_mode": "fractional_gradient",
         "attributes": [(0, 2 << 16), (2 << 16, 2 << 16), (1 << 16, 0)],
         "attribute_domain": "wire_unsigned_u8_q16"},
        {"name": "negative_xy_q16_clipped", "vertices": [(-2, -2), (5, 1), (1, 6)],
         "clip": (2, 1, 4, 5), "uv_mode": "fractional_gradient",
         "attributes": [(0, 0), (2 << 16, 0), (1 << 16, -(2 << 16))],
         "attribute_domain": "internal_signed_state"},
        {"name": "negative_xy_q16_clip_top0", "vertices": [(-2, -2), (5, 1), (1, 6)],
         "clip": (2, 0, 4, 5), "uv_mode": "fractional_gradient",
         "attributes": [(0, 0), (2 << 16, 0), (1 << 16, -(2 << 16))],
         "attribute_domain": "internal_signed_state"},
        {"name": "f3_horizontal_bounds_probe", "vertices": [(1, 1), (8, 1), (1, 8)], "clip": (4, 3, 6, 6)},
        {"name": "q16_fractional_uv", "vertices": [(2, 2), (7, 2), (2, 7)],
         "clip": (0, 0, 320, 240), "uv_mode": "fractional_gradient",
         "attributes": [(0, 0), (2 << 16, 0), (0, -(2 << 16))],
         "attribute_domain": "internal_signed_state"},
    ]
    cases = [NativeCase(pe, data_region, spec, route).run()
             for spec in specs for route in ("d60c_rowzero", "cc50_palette")]

    by_name = {}
    for case in cases:
        by_name.setdefault(case["case"], {})[case["route"]] = set(tuple(p) for p in case["native_pixel_set"])
    shared_union = by_name["shared_edge_upper"]["d60c_rowzero"] | by_name["shared_edge_lower"]["d60c_rowzero"]
    shared_overlap = by_name["shared_edge_upper"]["d60c_rowzero"] & by_name["shared_edge_lower"]["d60c_rowzero"]
    expected_shared_rect = {(x, y) for y in range(2, 6) for x in range(2, 6)}
    clip_start_comparison = {}
    for route in ("d60c_rowzero", "cc50_palette"):
        named = {case["case"]: case for case in cases if case["route"] == route}
        uv_top1 = {tuple(s["pixel"]): tuple(s["native_uv"])
                   for s in named["negative_xy_q16_clipped"]["uv_samples_against_independent_edge_models"]}
        uv_top0 = {tuple(s["pixel"]): tuple(s["native_uv"])
                   for s in named["negative_xy_q16_clip_top0"]["uv_samples_against_independent_edge_models"]}
        overlap = sorted(set(uv_top1) & set(uv_top0), key=lambda p: (p[1], p[0]))
        clip_start_comparison[route] = {
            "overlap_pixel_count": len(overlap),
            "different_uv_on_overlap": [
                {"pixel": list(p), "f3_top_1": list(uv_top1[p]), "f3_top_0": list(uv_top0[p])}
                for p in overlap if uv_top1[p] != uv_top0[p]
            ],
        }
    first_coverage_counterexample = next(({
        "case": item["case"], "route": item["route"],
        "expected": item["expected_pixel_set_integer_scan_floor_span_model"],
        "native": item["native_pixel_set"],
        "missing": item["missing_from_native"], "extra": item["extra_in_native"]
    } for item in cases if not item["coverage_match"]), None)
    first_uv_counterexample = next(({
        "case": item["case"], "route": item["route"], "pixel": sample["pixel"],
        "native_uv": sample["native_uv"], "integer_reciprocal_q16_uv": sample["integer_reciprocal_q16_uv"]
    } for item in cases for sample in item["uv_samples_against_independent_edge_models"]
      if not sample["matches_integer_reciprocal_q16_model"]), None)
    first_barycentric_deviation = next(({
        "case": item["case"], "route": item["route"], "pixel": sample["pixel"],
        "native_uv": sample["native_uv"], "exact_integer_barycentric_uv": sample["integer_barycentric_uv"]
    } for item in cases for sample in item["uv_samples_against_independent_edge_models"]
      if not sample["matches_integer_barycentric"]), None)
    barycentric_trace = None
    if first_barycentric_deviation is not None:
        trace_case = next(item for item in cases
                          if item["case"] == first_barycentric_deviation["case"] and
                          item["route"] == first_barycentric_deviation["route"])
        point = tuple(first_barycentric_deviation["pixel"])
        sorted_pixels = [tuple(p) for p in trace_case["native_pixel_set"]]
        read = trace_case["source_uv_reads"][sorted_pixels.index(point)]
        other_route_case = next(item for item in cases
                                if item["case"] == trace_case["case"] and
                                item["route"] != trace_case["route"])
        other_route_pixels = [tuple(p) for p in other_route_case["native_pixel_set"]]
        other_read = other_route_case["source_uv_reads"][other_route_pixels.index(point)]
        vertex_case = next(spec for spec in specs if spec["name"] == trace_case["case"])
        fixed_u, fixed_v = exact_barycentric_q16(vertex_case["vertices"],
                                                  vertex_case["attributes"], point)
        barycentric_trace = {
            "case": trace_case["case"], "route": trace_case["route"],
            "pixel_xy": list(point), "native_read_rva": read["pc"],
            "native_uv_bytes": [read["u"], read["v"]],
            "native_texture_relative_offset": hex(read["offset"]),
            "native_texture_byte_value": read["value"],
            "same_pixel_other_route": {
                "route": other_route_case["route"], "read_rva": other_read["pc"],
                "uv_bytes": [other_read["u"], other_read["v"]],
                "texture_relative_offset": hex(other_read["offset"]),
                "texture_byte_value": other_read["value"],
            },
            "exact_unquantized_integer_barycentric_q16": {
                "u": str(fixed_u), "v": str(fixed_v),
            },
            "exact_barycentric_floor_bytes": [
                (fixed_u // 65536) & 0xFF, (fixed_v // 65536) & 0xFF,
            ],
            "integer_reciprocal_q16_model_bytes": next(
                sample["integer_reciprocal_q16_uv"]
                for sample in trace_case["uv_samples_against_independent_edge_models"]
                if tuple(sample["pixel"]) == point),
            "interpretation": "Exact rational barycentric sampling differs by one U byte; the original finite reciprocal/Q16 edge-and-span model matches the native read. This is a model-contract counterexample, not evidence of a native defect.",
        }
    uv_route_equality = {}
    for name in sorted({case["case"] for case in cases if case["uv_samples_against_independent_edge_models"]}):
        by_route = {case["route"]: {tuple(sample["pixel"]): tuple(sample["native_uv"])
                                     for sample in case["uv_samples_against_independent_edge_models"]}
                    for case in cases if case["case"] == name}
        uv_route_equality[name] = {
            "equal": by_route["d60c_rowzero"] == by_route["cc50_palette"],
            "rowzero_count": len(by_route["d60c_rowzero"]),
            "cc50_count": len(by_route["cc50_palette"]),
        }

    compact_cases = []
    for item in cases:
        samples = item["uv_samples_against_independent_edge_models"]
        uv_digest_payload = [[sample["pixel"], sample["native_uv"]] for sample in samples]
        expected_pixels = item["expected_pixel_set_integer_scan_floor_span_model"]
        native_pixels = item["native_pixel_set"]
        compact_cases.append({
            "case": item["case"], "route": item["route"],
            "vertices_xy": item["vertices_xy"],
            "attributes_uv_q16": item["attributes_uv_q16"],
            "attribute_domain": item["attribute_domain"],
            "f3_clip_values": item["f3_clip_values"],
            "coverage_match": item["coverage_match"],
            "pixel_sets": {
                "expected_count": len(expected_pixels),
                "expected_sha256": sha(json.dumps(expected_pixels, separators=(",", ":")).encode("ascii")),
                "native_count": len(native_pixels),
                "native_sha256": sha(json.dumps(native_pixels, separators=(",", ":")).encode("ascii")),
            },
            "uv_summary": {
                "sample_count": len(samples),
                "ordered_pixel_uv_sha256": sha(json.dumps(uv_digest_payload, separators=(",", ":")).encode("ascii")),
                "reciprocal_q16_model_match_count": sum(s["matches_integer_reciprocal_q16_model"] for s in samples),
                "edge_anchored_model_match_count": sum(s["matches_edge_anchored_span_model"] for s in samples),
                "exact_barycentric_match_count": sum(s["matches_integer_barycentric"] for s in samples),
            },
        })
    model_names = ("cross", "native_edge_model_expected", "expected_uv_integer",
                   "exact_barycentric_q16", "expected_uv_edge_anchored",
                   "expected_uv_integer_reciprocal")
    model_source_hashes = {
        name: sha(inspect.getsource(globals()[name]).encode("utf-8"))
        for name in model_names
    }
    model_source_hashes["combined_sha256"] = sha(json.dumps(
        model_source_hashes, sort_keys=True, separators=(",", ":")).encode("ascii"))
    wire_valid_trace = None
    if barycentric_trace is not None:
        trace_case_spec = next(spec for spec in specs
                               if spec["name"] == barycentric_trace["case"])
        if trace_case_spec.get("attribute_domain") == "wire_unsigned_u8_q16":
            wire_valid_trace = dict(barycentric_trace)
            wire_valid_trace["vertices_xy"] = [list(v) for v in trace_case_spec["vertices"]]
            wire_valid_trace["attributes_uv_q16"] = [list(v) for v in trace_case_spec["attributes"]]
            wire_valid_trace["attribute_domain"] = trace_case_spec["attribute_domain"]
    compact_clip_comparison = {
        route: {"common_pixel_count": data["overlap_pixel_count"],
                "different_uv_count": len(data["different_uv_on_overlap"])}
        for route, data in clip_start_comparison.items()
    }
    runtime = verify_runtime(unicorn, ARGS.deps)
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "model_comparison": {
            "coverage": "BOUNDED_EQUIVALENT",
            "finite_reciprocal_q16": "MATCHES_ALL_CAPTURED_NATIVE_UV_READS",
            "exact_integer_barycentric": "CONFIRMED_MISMATCH_ON_WIRE_VALID_VECTOR",
        },
        "scope": "11 synthetic triangles through original D420 -> D60C row-zero and CC50 palette routes; finite coverage and UV observations only.",
        "oracle": {"sha256": sha(pe.data), "image_base": hex(pe.image_base)},
        "runtime": {"engine": runtime["engine"], "unicorn_version": runtime["unicorn_version"],
                    "native_dll_sha256": runtime["native_dll_sha256"]},
        "analysis": {"capstone_version": capstone.__version__, "static_parser": "scripts/pe.py"},
        "spans": spans,
        "f3_absolute_references": {
            "cc50": static_refs(pe, 0xCC50, 0xD415),
            "d60c": static_refs(pe, 0xD60C, 0xDD70),
        },
        "static_interpolation": {
            "reciprocal_table_initializer": "A8E918; R[0]=0, R[i]=floor(2^30/i) for 1..2047",
            "edge_and_span_reduction": "signed deltas * R[index], then SHRD 30; integer-y scan; SAR 16 span endpoints; horizontal UV starts at left-edge value at floor(left_x)",
            "rowzero_product_rvas": ["0xd810", "0xd824", "0xd838", "0xd995", "0xd9a9", "0xd9bd"],
            "rowzero_shrd30_rvas": ["0xd816", "0xd82a", "0xd83e", "0xd99b", "0xd9af", "0xd9c3"],
            "cc50_product_rvas": ["0xce89", "0xce9d", "0xceb1", "0xd00e", "0xd022", "0xd036"],
            "cc50_shrd30_rvas": ["0xce8f", "0xcea3", "0xceb7", "0xd014", "0xd028", "0xd03c"],
        },
        "cases": compact_cases,
        "comparisons": {
            "coverage_equal_by_shape": {name: by_name[name]["d60c_rowzero"] == by_name[name]["cc50_palette"]
                                        for name in by_name},
            "uv_equal_by_textured_shape": uv_route_equality,
            "shared_edge": {"native_union": [list(p) for p in sorted(shared_union, key=lambda p: (p[1], p[0]))],
                            "expected_half_open_rectangle": [list(p) for p in sorted(expected_shared_rect, key=lambda p: (p[1], p[0]))],
                            "native_overlap": [list(p) for p in sorted(shared_overlap, key=lambda p: (p[1], p[0]))],
                            "union_matches_rectangle": shared_union == expected_shared_rect},
            "f3_top_1_vs_0_common_uv": compact_clip_comparison,
            "first_coverage_counterexample": first_coverage_counterexample,
            "first_finite_uv_counterexample": first_uv_counterexample,
            "wire_valid_exact_barycentric_counterexample": wire_valid_trace,
        },
        "model_source_sha256": model_source_hashes,
        "test_source_sha256": sha(Path(__file__).read_bytes()),
        "limits": [
            "Finite synthetic cases only; no full frame, game, or DirectDraw path ran.",
            "Polygon wire U/V inputs are unsigned bytes expanded to Q16. Signed attribute cases are internal-state diagnostics, not AC60-reachable packet values.",
            "F3 left/right cells are statically unreferenced in these bodies; this does not prove all other renderer paths ignore them.",
            "Coverage and fixed reciprocal/Q16 match only the listed finite fixtures; arbitrary overflow and coordinate domains remain unproved.",
        ],
        "invocation": {"receipt": str(ARGS.receipt), "instruction_budget_per_case": MAX_INSNS},
    }
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({"status": receipt["status"], "cases": len(cases),
                      "first_coverage_counterexample": first_coverage_counterexample,
                      "first_uv_counterexample": first_uv_counterexample,
                      "shared_edge": receipt["comparisons"]["shared_edge"],
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
