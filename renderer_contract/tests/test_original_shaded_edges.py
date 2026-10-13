"""Finite shaded-polygon coverage and Q16 probes over original x86 bytes."""
from __future__ import annotations

import hashlib
import json
import sys
from fractions import Fraction
from pathlib import Path


def find_root(path: Path) -> Path:
    for parent in path.resolve().parents:
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("cannot find repository root with scripts/pe.py and public oracle_common.py")


ROOT = find_root(Path(__file__))
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import parse_args, verify_runtime, write_receipt  # noqa: E402

if not any(arg == "--receipt" or arg.startswith("--receipt=") for arg in sys.argv[1:]):
    raise SystemExit("--receipt is required and must be inside the repository work directory")

ARGS = parse_args("Probe bounded original shaded polygon paths and interpolation",
                  "shaded_edges_native.json")
if ARGS.original_root.resolve() != ROOT.resolve():
    raise SystemExit(f"--original-root must name the detected repository root: {ROOT}")
from pe import PE  # noqa: E402
import capstone  # noqa: E402
import unicorn  # noqa: E402
import importlib.util  # noqa: E402
from unicorn import (  # noqa: E402
    Uc, UcError, UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID,
    UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MODE_32,
    UC_PROT_EXEC, UC_PROT_READ, UC_PROT_WRITE,
)
from unicorn.x86_const import (  # noqa: E402
    UC_X86_REG_EBP, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_ESP,
)

PUBLIC_EDGE_MODEL_PATH = ROOT / "renderer_contract" / "tests" / "test_original_polygon_edges.py"
if not PUBLIC_EDGE_MODEL_PATH.is_file():
    raise SystemExit(f"missing public polygon-edge model: {PUBLIC_EDGE_MODEL_PATH}")
_saved_argv = sys.argv[:]
try:
    sys.argv = [
        str(PUBLIC_EDGE_MODEL_PATH), "--exe", str(ARGS.exe),
        "--receipt", str(ARGS.receipt), "--original-root", str(ROOT),
        "--deps", str(ARGS.deps), "--analysis-deps", str(ARGS.analysis_deps),
    ]
    _model_spec = importlib.util.spec_from_file_location(
        "public_polygon_edge_model", PUBLIC_EDGE_MODEL_PATH)
    if _model_spec is None or _model_spec.loader is None:
        raise RuntimeError("cannot load public polygon-edge model module")
    PUBLIC_EDGE_MODEL = importlib.util.module_from_spec(_model_spec)
    _model_spec.loader.exec_module(PUBLIC_EDGE_MODEL)
finally:
    sys.argv = _saved_argv


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
SHADE_BASE = 0x91C488
BLEND_BASE = 0x92E3D0
MODE_CELL = 0xA90A00
TPAGE_CELL = 0xA8E6F8
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
CODE_WINDOWS = (
    (0x72A0, 0x72CC),
    (0xC700, 0xC8E3),
    (0xC8F0, 0xCAA8),
    (0xDD70, 0xE675),
    (0xE680, 0xEF60),
)

EXPECTED_SPANS = {
    "reciprocal_table_initializer": (0x72A0, 0x72CC,
        "3d59be43f3756224228dfdcc372a3bc59b7a7dbba4bbab8c79dd821cc958014b"),
    "dd70_entire_original_function": (0xDD70, 0xE675,
        "2d13dbe76315a0e9e0b2fbec9fc1d84beffc377ee8df2988a6f1f354329cf9be"),
    "dd70_rectangle_dispatch": (0xDD70, 0xDE9F,
        "41ee5cb23e08171eac2bb676b4139290f36d3c8226b043d3b0af80dd48b52ca7"),
    "dd70_generic_body": (0xDE9F, 0xE675,
        "645a1a416be1477bd9000e017bc4f733161c70769cc34c6f13f214ec2271f236"),
    "e680_entire_original_function": (0xE680, 0xEF60,
        "f42bfcb06d042a8cd9ced6a8df5375b734c1e5641811a062059bbd02cb8dcb58"),
    "e680_mode_shape_dispatch": (0xE680, 0xE7C7,
        "1ac531aa802148e641444cb62451c9903fe6e59b046329d0ea0a80f463ccc75a"),
    "e680_generic_body": (0xE7C7, 0xEF60,
        "2aad976b7e2ee3947a06fb8f89c31ef39dcdbfbf487bb1f9b07faed05d7adeae"),
    "c700_rectangle_kernel": (0xC700, 0xC8E3,
        "399af24f03efd83d72c62c74a808f0f5e4bceb87f48205524d7b53d88063316f"),
    "c8f0_rectangle_kernel": (0xC8F0, 0xCAA8,
        "74bfb760505e5695cf5f1668e390d07046a32f7014c506706a85f9488870d52c"),
    "dd70_zero_test_pixel_loop": (0xE56C, 0xE5A8,
        "1d396bce03e35e84d2f2ae6d5f5dc7a392836b581ece877599de5b9ef0677bdc"),
    "e680_mode80_zero_test_pixel_loop": (0xEE6D, 0xEE93,
        "c4c067791e0ebc638acd6707ef4a2f27c8c76095cce726bf1aa4b9666e466247"),
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


# Keep the current case runner's independent calculations in the public helper namespace.
# These aliases intentionally supersede the local historical copies above.
native_edge_model_expected = PUBLIC_EDGE_MODEL.native_edge_model_expected
expected_uv_integer_reciprocal = PUBLIC_EDGE_MODEL.expected_uv_integer_reciprocal


class NativeCase:
    def __init__(self, pe: PE, data_region: tuple[int, int], spec: dict, route: str,
                 source_mode: str = "nonzero_gradient"):
        self.spec = spec
        self.route = route
        self.source_mode = source_mode
        self.mode = {"dd70_internal_180": 0x180,
                     "dd70_wire_7f": 0x7F,
                     "mode80_direct": 0x80}[route]
        self.selector = 2
        self.shade_row_index = self.mode & 0xFF
        self.shade_row_address = SHADE_BASE + self.shade_row_index * 256
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
        self.shade_reads = []
        self.blend_reads = []
        self.surface_reads = []
        self.surface_writes = []
        self.f3_reads = []
        self.reciprocal_table_writes = []
        self.reciprocal_stack_writes = []

        self.texture = (bytes(0x10000) if source_mode == "all_zero" else
                        bytes(1 + ((u + 7 * v) % 250)
                              for v in range(256) for u in range(256)))
        self.uc.mem_write(TEXTURE, self.texture)
        self.initial_surface = bytes(0x31 + ((i * 7) % 191) for i in range(SURFACE_SIZE))
        self.uc.mem_write(SURFACE, self.initial_surface)

        self.shade_values = bytes(i ^ 0xA5 for i in range(256))
        self.uc.mem_write(self.shade_row_address, self.shade_values)
        self.blend_values = {}
        for selector in range(4):
            table = bytes((src + 13 * dest + 17 * selector) & 0xFF
                          for src in range(256) for dest in range(256))
            base = BLEND_BASE + selector * 0x10000
            self.uc.mem_write(base, table)
            self.blend_values[selector] = table

        put32(self.uc, DRAW_GATE, 1)
        put32(self.uc, SURFACE_CELL, SURFACE)
        put32(self.uc, STRIDE_CELL, SURFACE_STRIDE)
        put32(self.uc, VERTEX_COUNT, len(spec["vertices"]))
        put32(self.uc, TEXTURE_CELL, TEXTURE)
        put32(self.uc, MODE_CELL, self.mode)
        put32(self.uc, TPAGE_CELL, self.selector)
        left, top, right, bottom = spec["clip"]
        put32(self.uc, F3_LEFT, left)
        put32(self.uc, F3_TOP, top)
        put32(self.uc, F3_RIGHT, right)
        put32(self.uc, F3_BOTTOM, bottom)
        attrs = spec["attributes"]
        if any(not 0 <= value <= (255 << 16) for pair in attrs for value in pair):
            raise AssertionError(f"{spec['name']} contains UV outside wire-valid unsigned-byte Q16")
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
            for start, end, _digest in EXPECTED_SPANS.values()
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
        sampled = int.from_bytes(bytes(uc.mem_read(address, size)), "little")
        if address in (F3_LEFT, F3_TOP, F3_RIGHT, F3_BOTTOM):
            self.f3_reads.append({"pc": hex(pc), "address": hex(address),
                                  "value": sampled, "size": size})
        if TEXTURE <= address and end <= TEXTURE + 0x10000:
            offset = address - TEXTURE
            self.source_reads.append({"pc": hex(pc), "offset": offset,
                                      "u": offset & 255, "v": (offset >> 8) & 255,
                                      "value": sampled})
        if self.shade_row_address <= address and end <= self.shade_row_address + 256:
            self.shade_reads.append({"pc": hex(pc), "index": address - self.shade_row_address,
                                     "value": sampled, "size": size})
        for selector in range(4):
            blend_base = BLEND_BASE + selector * 0x10000
            if blend_base <= address and end <= blend_base + 0x10000:
                self.blend_reads.append({"pc": hex(pc), "selector": selector,
                                         "index": address - blend_base,
                                         "value": sampled, "size": size})
                break
        if SURFACE <= address and end <= SURFACE + SURFACE_SIZE:
            self.surface_reads.append({"pc": hex(pc), "offset": address - SURFACE,
                                       "value": sampled, "size": size})

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
                                        "value": value & ((1 << (size * 8)) - 1),
                                        "size": size})
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
            raise AssertionError(f"{self.spec['name']} {self.route}: reciprocal initializer did not stop at 0x72cc")
        table = bytes(self.uc.mem_read(RECIP_TABLE, RECIP_COUNT * 4))
        if int.from_bytes(table[:4], "little") != 0 or len(self.reciprocal_table_writes) != RECIP_COUNT:
            raise AssertionError("bounded original reciprocal initializer did not fill the expected table")
        for index in (1, 2, 3, RECIP_COUNT - 1):
            if int.from_bytes(table[index * 4:index * 4 + 4], "little") != 0x40000000 // index:
                raise AssertionError(f"reciprocal table[{index}] has an unexpected value")
        self.reciprocal_init_trace = self.trace[:]
        self.trace.clear()
        put32(self.uc, FRAME, SENTINEL)
        self.uc.reg_write(UC_X86_REG_ESP, FRAME)
        self.uc.reg_write(UC_X86_REG_EBP, 0)
        self.uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
        try:
            self.uc.emu_start(BASE + 0xE680, 0, timeout=2_000_000, count=MAX_INSNS)
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

        actual_pixels = sorted({(event["offset"] % SURFACE_STRIDE,
                                 event["offset"] // SURFACE_STRIDE)
                                for event in self.surface_writes}, key=lambda p: (p[1], p[0]))
        actual_set = set(actual_pixels)
        expected_screen = native_edge_model_expected(self.spec["vertices"], self.spec["clip"])
        screen_set = set(expected_screen)
        missing = sorted(screen_set - actual_set, key=lambda p: (p[1], p[0]))
        extra = sorted(actual_set - screen_set, key=lambda p: (p[1], p[0]))
        surface = bytes(self.uc.mem_read(SURFACE, SURFACE_SIZE))
        attributes = self.spec["attributes"]
        uv_samples = []
        value_checks = []
        reads_aligned = len(self.source_reads) == len(expected_screen)
        writes_are_bytes = all(event["size"] == 1 for event in self.surface_writes)
        for index, pixel in enumerate(expected_screen):
            native_read = self.source_reads[index] if index < len(self.source_reads) else None
            expected_uv = expected_uv_integer_reciprocal(self.spec["vertices"], attributes, pixel)
            observed_uv = None if native_read is None else [native_read["u"], native_read["v"]]
            uv_samples.append({"pixel": list(pixel), "native_uv": observed_uv,
                               "expected_integer_reciprocal_q16_uv": list(expected_uv),
                               "matches": observed_uv == list(expected_uv)})
            if self.source_mode != "all_zero":
                u, v = expected_uv
                texel = self.texture[v * 256 + u]
                dest = self.initial_surface[pixel[1] * SURFACE_STRIDE + pixel[0]]
                shade = texel if self.mode == 0x80 else self.shade_values[texel]
                blend_index = (shade << 8) | dest
                expected_value = (shade + 13 * dest + 17 * self.selector) & 0xFF
                expected_address = BLEND_BASE + self.selector * 0x10000 + blend_index
                actual_value = surface[pixel[1] * SURFACE_STRIDE + pixel[0]]
                opposite_value = self.shade_values[texel] if self.mode == 0x80 else texel
                opposite_blend = (opposite_value + 13 * dest + 17 * self.selector) & 0xFF
                value_checks.append({"pixel": list(pixel), "texel": texel,
                                     "expected_shade_input": shade,
                                     "destination_before": dest,
                                     "expected_blend_entry": hex(expected_address),
                                     "expected_output": expected_value,
                                     "native_output": actual_value,
                                     "matches": actual_value == expected_value,
                                     "opposite_path_alternative": opposite_blend,
                                     "opposite_path_differs": opposite_blend != expected_value})

        expected_route = (
            self.mode == 0x80 and len(self.spec["vertices"]) == 4,
            self.mode != 0x80 and len(self.spec["vertices"]) == 4,
        )
        entered = {"dd70": 0xDD70 in self.trace,
                   "dd70_generic": 0xDE9F in self.trace,
                   "e680_generic": 0xE7C7 in self.trace,
                   "c700": 0xC700 in self.trace,
                   "c8f0": 0xC8F0 in self.trace}
        wanted = ({"dd70": True, "dd70_generic": not expected_route[1], "e680_generic": False,
                   "c700": expected_route[1], "c8f0": False}
                  if self.mode != 0x80 else
                  {"dd70": False, "dd70_generic": False, "e680_generic": not expected_route[0],
                   "c700": False, "c8f0": expected_route[0]})
        route_match = entered == wanted
        if (not reads_aligned or not writes_are_bytes or
                len(self.blend_reads) != len(actual_pixels) or
                len(self.surface_reads) != len(actual_pixels) or
                (self.mode != 0x80 and len(self.shade_reads) != len(actual_pixels)) or
                (self.mode == 0x80 and self.shade_reads)):
            route_match = False
        zero_branch_target = 0xE5A3 if self.mode != 0x80 else 0xEE8E
        zero_source_skip_match = (
            self.source_mode == "all_zero" and
            len(self.source_reads) == len(expected_screen) and
            all(sample["value"] == 0 for sample in self.source_reads) and
            zero_branch_target in self.trace and
            not self.shade_reads and not self.blend_reads and
            not self.surface_reads and not self.surface_writes
        )
        pixel_values = [{"x": x, "y": y,
                         "value": surface[y * SURFACE_STRIDE + x]}
                        for x, y in actual_pixels]
        blend_selectors = sorted({row["selector"] for row in self.blend_reads})
        return {
            "case": self.spec["name"],
            "route": self.route,
            "source_mode": self.source_mode,
            "entry_rva": "0xe680",
            "mode": hex(self.mode),
            "vertices_xy": self.spec["vertices"],
            "attributes_uv_q16": attributes,
            "attribute_domain": "wire_unsigned_u8_q16",
            "f3_clip_values": dict(zip(("left", "top", "right", "bottom"), self.spec["clip"])),
            "f3_reads": self.f3_reads,
            "expected_pixel_set_integer_scan_floor_span_model": [list(p) for p in expected_screen],
            "native_pixel_set": [list(p) for p in actual_pixels],
            "coverage_match": not missing and not extra,
            "missing_from_native": [list(p) for p in missing],
            "extra_in_native": [list(p) for p in extra],
            "pixel_values": pixel_values,
            "source_uv_reads": self.source_reads,
            "uv_samples_against_integer_reciprocal_q16_model": uv_samples,
            "uv_model_match": reads_aligned and all(sample["matches"] for sample in uv_samples),
            "sample_coverage_match": reads_aligned and len(self.source_reads) == len(expected_screen),
            "zero_source_sample_count": len(self.source_reads) if self.source_mode == "all_zero" else 0,
            "zero_source_sample_values_are_zero": (self.source_mode == "all_zero" and
                                                     all(sample["value"] == 0 for sample in self.source_reads)),
            "zero_branch_target_rva": hex(zero_branch_target),
            "zero_branch_taken": self.source_mode == "all_zero" and zero_branch_target in self.trace,
            "zero_source_skip_match": zero_source_skip_match,
            "mode_source_domain": "internal_full_dword_test_only" if self.mode == 0x180 else "wire_unsigned_u8",
            "shade_row_index_low_byte": self.shade_row_index,
            "shade_row_seed": "xor_0xa5_nonidentity",
            "shade_row_reads": self.shade_reads,
            "shade_row_read_count": len(self.shade_reads),
            "blend_selector_set": blend_selectors,
            "blend_reads": self.blend_reads,
            "surface_read_count": len(self.surface_reads),
            "surface_write_count": len(self.surface_writes),
            "expected_shaded_blend_values": value_checks,
            "all_expected_values_match": all(check["matches"] for check in value_checks),
            "opposite_path_discriminates_every_pixel": all(check["opposite_path_differs"] for check in value_checks),
            "entered_paths": entered,
            "expected_paths": wanted,
            "path_match": route_match,
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
                    rows.append({"rva": hex(insn.address - BASE), "bytes": insn.bytes.hex(),
                                 "mnemonic": insn.mnemonic, "operands": insn.op_str,
                                 "address": hex(address), "width": operand.size})
    return rows


def digest_json(value) -> str:
    return sha(json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8"))


def main() -> None:
    pe = PE(ARGS.exe)
    if sha(pe.data) != EXE_SHA or pe.image_base != BASE:
        raise AssertionError("pinned original EXE identity/base mismatch")
    if capstone.__version__ != "5.0.7":
        raise AssertionError(f"expected Capstone 5.0.7; got {capstone.__version__}")
    data_region = next((BASE + section["virtual_address"],
                        max(section["virtual_size"], section["raw_size"]))
                       for section in pe.sections if section["name"] == ".data")
    spans = {}
    for name, (start, end, expected_digest) in EXPECTED_SPANS.items():
        actual_digest = sha(pe.read_rva(start, end - start))
        if actual_digest != expected_digest:
            raise AssertionError(f"{name} SHA-256 {actual_digest} != {expected_digest}")
        spans[name] = {"start_rva": hex(start), "end_rva_exclusive": hex(end),
                       "size": end - start, "sha256": actual_digest}

    affine_rect = [(0, 0), (4 << 16, 0), (4 << 16, 4 << 16), (0, 4 << 16)]
    specs = [
        {"name": "shared_edge_upper", "vertices": [(2, 2), (6, 2), (6, 6)],
         "attributes": affine_rect[:3], "clip": (0, 0, 320, 240)},
        {"name": "shared_edge_lower", "vertices": [(2, 2), (6, 6), (2, 6)],
         "attributes": [affine_rect[0], affine_rect[2], affine_rect[3]],
         "clip": (0, 0, 320, 240)},
        {"name": "negative_xy_wire_valid", "vertices": [(-2, -2), (5, 1), (1, 6)],
         "attributes": [(0, 2 << 16), (2 << 16, 2 << 16), (1 << 16, 0)],
         "clip": (4, 1, 6, 5)},
        {"name": "f3_horizontal_bounds_probe", "vertices": [(1, 1), (8, 1), (1, 8)],
         "attributes": [(0, 0), (4 << 16, 0), (0, 4 << 16)],
         "clip": (4, 3, 5, 6)},
        {"name": "eligible_rectangle", "vertices": [(2, 2), (6, 2), (6, 6), (2, 6)],
         "attributes": affine_rect, "clip": (0, 0, 320, 240)},
    ]
    routes = ("dd70_internal_180", "dd70_wire_7f", "mode80_direct")
    cases = [NativeCase(pe, data_region, spec, route).run()
             for spec in specs for route in routes]
    zero_spec = dict(specs[0], name="zero_source_generic_triangle")
    zero_cases = [NativeCase(pe, data_region, zero_spec, route,
                             source_mode="all_zero").run()
                  for route in ("dd70_wire_7f", "mode80_direct")]

    def case_output(case):
        uv_pairs = [{"pixel": sample["pixel"], "uv": sample["native_uv"]}
                    for sample in case["uv_samples_against_integer_reciprocal_q16_model"]]
        coordinates = case["native_pixel_set"]
        return {
            "case": case["case"], "route": case["route"], "mode": case["mode"],
            "source_mode": case["source_mode"],
            "mode_source_domain": case["mode_source_domain"],
            "shade_row_index_low_byte": case["shade_row_index_low_byte"],
            "pixel_count": len(coordinates),
            "expected_scan_pixel_count": len(case["expected_pixel_set_integer_scan_floor_span_model"]),
            "coverage_sha256": digest_json(coordinates),
            "uv_sample_count": len(uv_pairs), "uv_pairs_sha256": digest_json(uv_pairs),
            "source_sample_pc_set": sorted({sample["pc"] for sample in case["source_uv_reads"]}),
            "surface_pixel_values_sha256": digest_json(case["pixel_values"]),
            "coverage_match": case["coverage_match"],
            "uv_model_match": case["uv_model_match"],
            "sample_coverage_match": case["sample_coverage_match"],
            "zero_source_sample_count": case["zero_source_sample_count"],
            "zero_source_sample_values_are_zero": case["zero_source_sample_values_are_zero"],
            "zero_branch_target_rva": case["zero_branch_target_rva"],
            "zero_branch_taken": case["zero_branch_taken"],
            "zero_source_skip_match": case["zero_source_skip_match"],
            "shade_row_read_count": case["shade_row_read_count"],
            "blend_selector_set": case["blend_selector_set"],
            "blend_read_count": len(case["blend_reads"]),
            "surface_read_count": case["surface_read_count"],
            "surface_write_count": case["surface_write_count"],
            "all_expected_values_match": (None if case["source_mode"] == "all_zero"
                                           else case["all_expected_values_match"]),
            "opposite_path_discriminates_every_pixel": (
                None if case["source_mode"] == "all_zero"
                else case["opposite_path_discriminates_every_pixel"]),
            "entered_paths": case["entered_paths"], "expected_paths": case["expected_paths"],
            "path_match": case["path_match"],
            "f3_reads": [{"address": item["address"], "value": item["value"]}
                         for item in case["f3_reads"]],
        }

    positive_outputs = [case_output(case) for case in cases]
    zero_outputs = [case_output(case) for case in zero_cases]
    failures = [
        {"case": row["case"], "route": row["route"], "field": field}
        for row in positive_outputs
        for field in ("coverage_match", "uv_model_match", "all_expected_values_match",
                      "opposite_path_discriminates_every_pixel", "path_match")
        if not row[field]
    ]
    failures.extend(
        {"case": row["case"], "route": row["route"], "field": field}
        for row in zero_outputs
        for field in ("sample_coverage_match", "uv_model_match", "zero_source_sample_values_are_zero",
                      "zero_branch_taken", "zero_source_skip_match", "path_match")
        if not row[field]
    )
    failures.extend(
        {"case": row["case"], "route": row["route"], "field": field}
        for row in zero_outputs
        for field in ("pixel_count", "shade_row_read_count", "blend_read_count",
                      "surface_read_count", "surface_write_count")
        if row[field] != 0
    )

    shared_edge = {}
    for route in routes:
        upper = set(map(tuple, next(row for row in cases
                                    if row["case"] == "shared_edge_upper" and row["route"] == route)["native_pixel_set"]))
        lower = set(map(tuple, next(row for row in cases
                                    if row["case"] == "shared_edge_lower" and row["route"] == route)["native_pixel_set"]))
        union = sorted(upper | lower, key=lambda point: (point[1], point[0]))
        expected = sorted({(x, y) for y in range(2, 6) for x in range(2, 6)},
                          key=lambda point: (point[1], point[0]))
        shared_edge[route] = {"upper_count": len(upper), "lower_count": len(lower),
                              "overlap_count": len(upper & lower),
                              "union_count": len(union), "union_matches_4x4": union == expected,
                              "union_sha256": digest_json([list(point) for point in union])}

    reference_rows = {
        name: [{"rva": row["rva"], "address": row["address"]}
               for row in static_refs(pe, start, end)]
        for name, (start, end) in {"dd70": (0xDD70, 0xE675),
                                   "e680": (0xE680, 0xEF60)}.items()
    }
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_OBSERVED" if not failures else "COUNTEREXAMPLE",
        "oracle": {"path": str(ARGS.exe), "sha256": sha(pe.data), "image_base": hex(pe.image_base)},
        "source_provenance": {
            "runner_namespace": "renderer_next.shaded_edges.public_test_original_shaded_edges",
            "runner_path": str(Path(__file__).resolve()),
            "runner_sha256": sha(Path(__file__).read_bytes()),
            "model_namespace": "renderer_contract.tests.test_original_polygon_edges",
            "model_path": str(PUBLIC_EDGE_MODEL_PATH.resolve()),
            "model_sha256": sha(PUBLIC_EDGE_MODEL_PATH.read_bytes()),
            "imported_symbols": ["native_edge_model_expected", "expected_uv_integer_reciprocal"],
            "model_use": "NativeCase coverage and UV expected values resolve through these imported public functions.",
        },
        "runtime": verify_runtime(unicorn, ARGS.deps),
        "analysis": {"capstone_version": capstone.__version__, "static_parser": "scripts/pe.py"},
        "original_spans": spans,
        "static_dispatch_rvas": {
            "full_dword_mode_compare": "0xe6a1", "mode80_equal_branch": "0xe6ab",
            "not_mode80_calls_dd70": "0xe6ad", "mode80_rectangle_mismatch_to_e7c7": "0xe6cd",
            "dd70_rectangle_calls_c700": "0xde93", "mode80_rectangle_calls_c8f0": "0xe7bb",
            "shade_row_setup": ["0xdea4", "0xdead", "0xdeb6", "0xdebe"],
            "generic_body_entries": {"dd70": "0xde9f", "mode80": "0xe7c7"},
        },
        "static_zero_source_tests": {
            "dd70_wire_7f": {"sample_load": "0xe578", "test": "0xe581",
                              "jz": "0xe583", "taken_target": "0xe5a3",
                              "skipped_operations": ["shade lookup", "blend lookup",
                                                     "destination read", "destination write"]},
            "mode80_wire": {"sample_load": "0xee79", "test": "0xee82",
                            "jz": "0xee84", "taken_target": "0xee8e",
                            "skipped_operations": ["blend lookup", "destination read",
                                                   "destination write"]},
        },
        "f3_absolute_references": reference_rows,
        "fixture_inputs": [
            {"name": spec["name"], "vertices_xy": spec["vertices"],
             "attributes_uv_q16": spec["attributes"], "clip_left_top_right_bottom": spec["clip"]}
            for spec in specs
        ],
        "zero_source_fixture": {
            "name": zero_spec["name"], "routes": ["dd70_wire_7f", "mode80_direct"],
            "vertices_xy": zero_spec["vertices"],
            "attributes_uv_q16": zero_spec["attributes"],
            "clip_left_top_right_bottom": zero_spec["clip"],
            "texture": "all 65536 bytes are zero; UV samples still traced",
        },
        "mode_domain": {
            "internal_0x180": "Synthetic full-DWORD compare witness only. The ordinary AC60 polygon boundary zero-extends a u8, so this value is not an ordinary packet mode; low byte 0x80 selects row 128 after the full compare takes DD70.",
            "wire_0x7f": "Reachable zero-extended byte mode; DD70 selects seeded row 127.",
            "wire_0x80": "Reachable zero-extended byte mode; the mode-equality path uses raw texture indices and performs no shade-row read.",
        },
        "fixture_seed": {
            "positive_texture": "byte(u,v)=1+((u+7*v)%250), so all positive-case samples are nonzero",
            "zero_source_texture": "all 65536 bytes are zero",
            "surface": "byte(offset)=0x31+((offset*7)%191)",
            "shade_row": "byte(index)=index XOR 0xA5",
            "blend": "byte(source,destination,selector)=(source+13*destination+17*selector)&0xFF; selected selector=2; all four selector banks seeded distinctly",
        },
        "positive_case_outputs": positive_outputs,
        "zero_source_case_outputs": zero_outputs,
        "positive_case_count": len(positive_outputs),
        "zero_source_case_count": len(zero_outputs),
        "case_count": len(positive_outputs) + len(zero_outputs),
        "shared_edge_union": shared_edge,
        "limits": [
            "Fifteen positive finite synthetic executions plus two all-zero-source discriminators; not full-frame rendering or whole-rasterizer equivalence.",
            "The 0x180 case is internal-only; row 128 is not claimed reachable from ordinary AC60 polygon packets. The reachable shaded case is 0x7F -> row 127.",
            "The two zero-source runs establish the tested per-sample zero branch only for the selected wire-valid triangle and selector 2; they do not establish every geometry or LUT selector case.",
            "No original bytes or full memory/access traces are included in this receipt.",
        ],
        "failures": failures,
        "invocation": {"exe": str(ARGS.exe), "original_root": str(ARGS.original_root),
                       "receipt": str(ARGS.receipt), "instruction_budget_per_case": MAX_INSNS},
    }
    # Runner status is separate from the project's finite proof vocabulary.
    receipt["proof_level"] = "BOUNDED_EQUIVALENT" if not failures else "UNRESOLVED"
    write_receipt(ARGS.receipt, receipt)
    print(json.dumps({"status": receipt["status"], "case_count": receipt["case_count"],
                      "positive_case_count": receipt["positive_case_count"],
                      "zero_source_case_count": receipt["zero_source_case_count"],
                      "failure_count": len(failures), "receipt": str(ARGS.receipt),
                      "model_source": receipt["source_provenance"]["model_path"]}, indent=2))


if __name__ == "__main__":
    main()
