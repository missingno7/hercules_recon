from __future__ import annotations

import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
import os
import struct
import sys
from pathlib import Path

PINNED_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
PINNED_UNICORN_DLL_SHA256 = "010d85dce84b8f6ffaec3d5420f1b5f7768f859eb831be574c7da23e656ee8f1"
PINNED_UNICORN_VERSION = "2.1.4"
PINNED_STATE_OWNERSHIP_SHA256 = "ecd1e0eaddcf35ceb5f8beeb5c798ffd9b54d29ae1fd0ae2760a3de344ee4508"
PINNED_PRESENTATION_SHA256 = "c98085c9f39b8c532817d90f5617067517a1157a8ede56042d778fde6c2d829a"
PINNED_POLYGON_HELPER_SHA256 = "9f48076ec02e47308ec4db760cdbd2b11832b6f41e8798867580cfdb41e85c32"
PINNED_ORACLE_COMMON_SHA256 = "a024048d9acc6e18197e10a82427cd84351a90666e1626e610bf60c6105f1357"
PINNED_PE_HELPER_SHA256 = "8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca"
BASE = 0x400000
DIRTY = 0x43A590
SYNTH_SURFACE = 0x31000000
SYNTH_SURFACE_SIZE = 0x13000
COLOR_CELL = 0x436A0C
LUT_BASE = 0x91C488

SPAN_PINS = {
    "present_frame_06620": (0x6620, 224, "4e350af06a5c660a75dabf57cd95c42eb06d6f0b1b4b8a2c65b709fefa3d30cd"),
    "lock_failure_transition_06bb0": (0x6BB0, 784, "369922176b91588536370342557e34de4e6e7dac3c6baeebfa3cf7f915f9ffe0"),
    "directdraw_color_setup_06ee0": (0x6EE0, 272, "6437bcd2fc991f42423b9602d567acd5f76f1d58f8d23270546dba9c480b7723"),
    "color_translate_070c0": (0x70C0, 128, "3432496f55144a415399893358618b7f970f1a263eb4ea80df30083e6494d4e7"),
    "polygon_span_helper_0c2e0": (0xC2E0, 1056, "8908bd9d0b920d8020f304feda864b1a25f42415cca421510b48362e2f091d9a"),
    "polygon_fill_helper_0c700": (0xC700, 496, "b81741c75b775d15bc06622ee9e11a4095385c92e8b6c0a3b2c641e80924a567"),
    "sprite_path_0c8f0": (0xC8F0, 448, "95ac42d7f4447015eb0760dfccbf2c40fddf600baa76f8f885cf3be7ad9926bf"),
    "sprite_path_0cab0": (0xCAB0, 416, "fe6c00a7e9109250e2e5aa4c3c3419c7381cf9758422d42d319b3e4648d12609"),
    "polygon_draw_helper_0cc50": (0xCC50, 2000, "26ac072b8de4b59f4a929abb5fd0d6cbcbe826fa02d3ee9f78d07245b39f8776"),
    "polygon_path_0d420": (0xD420, 2384, "4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e"),
    "polygon_pixel_helper_0dd70": (0xDD70, 2320, "44dcd2638f14fbcf644f76aa622cba9e4dff971776662c6d2317b725eafcad60"),
    "polygon_path_0e680": (0xE680, 2272, "f42bfcb06d042a8cd9ced6a8df5375b734c1e5641811a062059bbd02cb8dcb58"),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha_file(path: Path) -> str:
    return sha(path.read_bytes())


def checked_output(path: Path, root: Path, exe: Path) -> Path:
    resolved = path.resolve()
    work_root = root / "work"
    if work_root.resolve() != work_root:
        raise ValueError("redirected work directory is not an output boundary")
    try:
        resolved.relative_to(work_root)
    except ValueError as error:
        raise ValueError(f"output must remain under {root / 'work'}: {resolved}") from error
    if resolved == exe or resolved == exe.parent or exe.parent in resolved.parents:
        raise ValueError(f"output must be outside the original image directory: {resolved}")
    return resolved


def load_harness(root: Path, exe: Path, output_dir: Path, deps: Path, analysis_deps: Path):
    helper = root / "renderer_contract" / "tests" / "test_original_polygon_dispatch.py"
    helper_hash = sha_file(helper)
    if helper_hash != PINNED_POLYGON_HELPER_SHA256:
        raise RuntimeError(f"polygon fixture helper hash mismatch: {helper_hash}")
    oracle_common = root / "renderer_contract" / "tests" / "oracle_common.py"
    oracle_common_hash = sha_file(oracle_common)
    if oracle_common_hash != PINNED_ORACLE_COMMON_SHA256:
        raise RuntimeError(f"oracle_common.py hash mismatch: {oracle_common_hash}")
    receipt = checked_output(output_dir / "harness_import_receipt.json", root, exe)
    old_argv = sys.argv[:]
    sys.argv = [str(helper), "--exe", str(exe), "--original-root", str(root),
                "--deps", str(deps), "--analysis-deps", str(analysis_deps), "--receipt", str(receipt)]
    try:
        spec = importlib.util.spec_from_file_location("pinned_polygon_fixture", helper)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load pinned polygon fixture helper")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    finally:
        sys.argv = old_argv
    return module, {"test_original_polygon_dispatch.py": helper_hash,
                    "oracle_common.py": oracle_common_hash,
                    "scripts/pe.py": sha_file(root / "scripts" / "pe.py")}


def load_static_audit(root: Path, exe: Path, output_dir: Path):
    helper = Path(__file__).with_name("audit_dirty_refs.py")
    output = checked_output(output_dir / "dirty_refs.json", root, exe)
    old_argv = sys.argv[:]
    captured = io.StringIO()
    sys.argv = [str(helper), "--exe", str(exe), "--repo-root", str(root), "--output", str(output)]
    try:
        spec = importlib.util.spec_from_file_location("dirty_refs_audit", helper)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load dirty-reference audit")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with contextlib.redirect_stdout(captured):
            module.main()
    finally:
        sys.argv = old_argv
    receipt = json.loads(output.read_text(encoding="utf-8"))
    return receipt, output, sha_file(helper), captured.getvalue().strip()


def set_dirty_hook(module):
    old_write = module.Fixture._write

    def tracked_write(self, uc, access, address, size, value, user_data):
        if SYNTH_SURFACE <= address < SYNTH_SURFACE + SYNTH_SURFACE_SIZE:
            self.remap_write_count = getattr(self, "remap_write_count", 0) + 1
            return
        if address <= DIRTY < address + size:
            self.dirty_gate_writes.append({
                "pc_rva": hex(uc.reg_read(module.UC_X86_REG_EIP) - BASE),
                "size": size,
                "value": hex(value & ((1 << (size * 8)) - 1)),
            })
        old_write(self, uc, access, address, size, value, user_data)

    module.Fixture._write = tracked_write


def run_raster_case(module, exe: Path, name: str, kind: str, *, zero_texels=False,
                    xshift=0, zero_width=False, draw_gate=1, clut_gate=0,
                    row_code=0x7800, collapse_triangle_x=False, seed_dirty=0,
                    expected_site=None):
    quad = kind in ("d420_cab0", "e680_c8f0")
    e680 = kind == "e680_c8f0"
    fixture = module.Fixture(module.PE(exe), name, gate=clut_gate, source=0,
                             quad_fastpath=quad and not e680, e680_fastpath=e680)
    if not quad and row_code != 0x7800:
        fixture.extra_window = (0xCC50, 0xD420) if clut_gate else (0xC2E0, 0xC700)
        fixture.extra_code_before = bytes(fixture.uc.mem_read(
            BASE + fixture.extra_window[0], fixture.extra_window[1] - fixture.extra_window[0]))
    fixture.dirty_gate_writes = []
    fixture.uc.mem_write(DIRTY, bytes([seed_dirty & 0xFF]))
    module.put32(fixture.uc, module.AVAILABLE, draw_gate)
    module.put32(fixture.uc, module.ROW_CODE, row_code)
    if quad:
        if xshift:
            for i, x in enumerate((1, 3, 3, 1)):
                module.put32(fixture.uc, module.X + 4 * i, x + xshift)
        if zero_width:
            for i in range(4):
                module.put32(fixture.uc, module.X + 4 * i, 1)
                module.put32(fixture.uc, module.U + 4 * i, 0)
        if zero_texels:
            for offset in (0, 1, 0x100, 0x101):
                fixture.uc.mem_write(module.TEXTURE + offset, b"\x00")
    elif xshift:
        for i, x in enumerate((1, 3, 1)):
            module.put32(fixture.uc, module.X + 4 * i, x + xshift)
    if collapse_triangle_x and not quad:
        for i in range(3):
            module.put32(fixture.uc, module.X + 4 * i, 1)
    frame = module.FRAME
    fixture.uc.mem_write(frame, struct.pack("<I", module.SENTINEL))
    fixture.uc.reg_write(module.UC_X86_REG_ESP, frame)
    fixture.uc.reg_write(module.UC_X86_REG_EBP, 0)
    fixture.uc.reg_write(module.UC_X86_REG_EFLAGS, 0x202)
    try:
        fixture.uc.emu_start(BASE + fixture.main_window[0], 0, timeout=2_000_000, count=module.MAX_INSNS)
    except module.UcError as error:
        if fixture.error is None:
            fixture.error = {"kind": "unicorn_error", "detail": str(error),
                             "pc": hex(fixture.uc.reg_read(module.UC_X86_REG_EIP))}
    if fixture.error or not fixture.stop_seen:
        raise RuntimeError(f"{name}: execution failed: {fixture.error}; stop={fixture.stop_seen}")
    dirty_after = fixture.uc.mem_read(DIRTY, 1)[0]
    observed_sites = [int(row["pc_rva"], 16) for row in fixture.dirty_gate_writes]
    if expected_site is not None and observed_sites != [expected_site]:
        raise RuntimeError(f"{name}: expected dirty site {expected_site:#x}, observed {observed_sites!r}")
    if expected_site is None and observed_sites:
        raise RuntimeError(f"{name}: expected no dirty write, observed {observed_sites!r}")
    surface_after = bytes(fixture.uc.mem_read(module.SURFACE, module.SURFACE_SIZE))
    changed = sum(before != after for before, after in zip(fixture.initial_surface, surface_after))
    source_reads = sum(1 for event in fixture.events if event["kind"] == "read")
    surface_writes = sum(1 for event in fixture.events if event["kind"] == "surface_write")
    unchanged = bytes(fixture.uc.mem_read(BASE + fixture.main_window[0],
                   fixture.main_window[1] - fixture.main_window[0])) == fixture.code_before
    if fixture.extra_window is not None and fixture.extra_code_before is not None:
        unchanged = unchanged and bytes(fixture.uc.mem_read(BASE + fixture.extra_window[0],
                      fixture.extra_window[1] - fixture.extra_window[0])) == fixture.extra_code_before
    if not unchanged:
        raise RuntimeError(f"{name}: original code changed in the emulator mapping")
    return {"case": name, "route": kind, "entry_rva": hex(fixture.main_window[0]),
            "draw_gate_BA01F0": draw_gate, "clut_gate_96E3E0": clut_gate,
            "row_code": hex(row_code), "x_translation": xshift,
            "zero_width_input": zero_width, "all_source_texels_zero": zero_texels,
            "all_triangle_x_equal": collapse_triangle_x,
            "dirty_before": seed_dirty, "dirty_after": dirty_after,
            "dirty_writes": fixture.dirty_gate_writes,
            "texture_read_count": source_reads, "surface_write_count": surface_writes,
            "surface_changed_byte_count": changed,
            "terminal_trace_rvas": [hex(x) for x in fixture.code_trace[-8:]],
            "original_code_unchanged": True}


def run_e680_generic_case(module, exe: Path, name: str, mode: int, *, xshift=0,
                          zero_width=False, gate=1, expected_site=None):
    fixture = module.Fixture(module.PE(exe), name, gate=0, source=0, e680_fastpath=True)
    fixture.main_window = (0xE680, 0xEF60)
    fixture.code_before = bytes(fixture.uc.mem_read(BASE + 0xE680, 0x8E0))
    fixture.extra_window = (0xDD70, 0xE680) if mode != 0x80 else (0xC8F0, 0xCAA8)
    fixture.extra_code_before = bytes(fixture.uc.mem_read(BASE + fixture.extra_window[0],
                                   fixture.extra_window[1] - fixture.extra_window[0]))
    fixture.dirty_gate_writes = []
    module.put32(fixture.uc, module.MODE_CELL, mode)
    module.put32(fixture.uc, module.AVAILABLE, gate)
    module.put32(fixture.uc, module.VERTEX_COUNT, 3)
    xs, ys = (1, 3, 1), (1, 1, 3)
    for i in range(3):
        module.put32(fixture.uc, module.X + 4 * i, xs[i] + xshift)
        module.put32(fixture.uc, module.Y + 4 * i, ys[i])
        module.put32(fixture.uc, module.U + 4 * i, 0)
        module.put32(fixture.uc, module.V + 4 * i, 0)
    if zero_width:
        for i in range(3):
            module.put32(fixture.uc, module.X + 4 * i, 1)
    frame = module.FRAME
    fixture.uc.mem_write(frame, struct.pack("<I", module.SENTINEL))
    fixture.uc.reg_write(module.UC_X86_REG_ESP, frame)
    fixture.uc.reg_write(module.UC_X86_REG_EBP, 0)
    fixture.uc.reg_write(module.UC_X86_REG_EFLAGS, 0x202)
    try:
        fixture.uc.emu_start(BASE + 0xE680, 0, timeout=2_000_000, count=module.MAX_INSNS)
    except module.UcError as error:
        if fixture.error is None:
            fixture.error = {"kind": "unicorn_error", "detail": str(error),
                             "pc": hex(fixture.uc.reg_read(module.UC_X86_REG_EIP))}
    if fixture.error or not fixture.stop_seen:
        raise RuntimeError(f"{name}: execution failed: {fixture.error}; stop={fixture.stop_seen}")
    dirty_after = fixture.uc.mem_read(DIRTY, 1)[0]
    observed_sites = [int(row["pc_rva"], 16) for row in fixture.dirty_gate_writes]
    if expected_site is not None and observed_sites != [expected_site]:
        raise RuntimeError(f"{name}: expected dirty site {expected_site:#x}, observed {observed_sites!r}")
    if expected_site is None and observed_sites:
        raise RuntimeError(f"{name}: expected no dirty write, observed {observed_sites!r}")
    surface_after = bytes(fixture.uc.mem_read(module.SURFACE, module.SURFACE_SIZE))
    unchanged = bytes(fixture.uc.mem_read(BASE + 0xE680, 0x8E0)) == fixture.code_before
    unchanged = unchanged and bytes(fixture.uc.mem_read(BASE + fixture.extra_window[0],
                fixture.extra_window[1] - fixture.extra_window[0])) == fixture.extra_code_before
    if not unchanged:
        raise RuntimeError(f"{name}: original code changed in the emulator mapping")
    return {"case": name, "route": "e680_generic_or_dd70", "entry_rva": "0xe680",
            "mode": hex(mode), "draw_gate_BA01F0": gate, "x_translation": xshift,
            "zero_width_input": zero_width, "dirty_before": 0, "dirty_after": dirty_after,
            "dirty_writes": fixture.dirty_gate_writes,
            "texture_read_count": sum(1 for e in fixture.events if e["kind"] == "read"),
            "surface_write_count": sum(1 for e in fixture.events if e["kind"] == "surface_write"),
            "surface_changed_byte_count": sum(a != b for a, b in zip(fixture.initial_surface, surface_after)),
            "terminal_trace_rvas": [hex(x) for x in fixture.code_trace[-8:]],
            "original_code_unchanged": True}


def run_70c0_case(module, exe: Path, name: str, argument: int, *, stored_color=1,
                  lock=1, dirty=0):
    fixture = module.Fixture(module.PE(exe), name, gate=0, source=0)
    fixture.main_window = (0x70C0, 0x7136)
    fixture.extra_window = None
    fixture.dirty_gate_writes = []
    fixture.remap_write_count = 0
    original_span_before = bytes(fixture.uc.mem_read(BASE + 0x70C0, 0x80))
    fixture.uc.mem_map(SYNTH_SURFACE, SYNTH_SURFACE_SIZE,
                       module.UC_PROT_READ | module.UC_PROT_WRITE)
    pixels_before = bytes([0x10]) * (320 * 240)
    fixture.uc.mem_write(SYNTH_SURFACE, pixels_before)
    module.put32(fixture.uc, module.SURFACE_CELL, SYNTH_SURFACE)
    module.put32(fixture.uc, module.STRIDE_CELL, 320)
    module.put32(fixture.uc, module.AVAILABLE, lock)
    fixture.uc.mem_write(DIRTY, bytes([dirty & 0xFF]))
    fixture.uc.mem_write(COLOR_CELL, bytes([stored_color & 0xFF]))
    fixture.uc.mem_write(LUT_BASE + (stored_color & 0xFF) * 256 + 0x10, b"\xaa")
    frame = module.FRAME
    fixture.uc.mem_write(frame, struct.pack("<II", module.SENTINEL, argument & 0xFFFFFFFF))
    fixture.uc.reg_write(module.UC_X86_REG_ESP, frame)
    fixture.uc.reg_write(module.UC_X86_REG_EBP, 0)
    fixture.uc.reg_write(module.UC_X86_REG_EFLAGS, 0x202)
    try:
        fixture.uc.emu_start(BASE + 0x70C0, 0, timeout=3_000_000, count=1_000_000)
    except module.UcError as error:
        if fixture.error is None:
            fixture.error = {"kind": "unicorn_error", "detail": str(error),
                             "pc": hex(fixture.uc.reg_read(module.UC_X86_REG_EIP))}
    if fixture.error or not fixture.stop_seen:
        raise RuntimeError(f"{name}: execution failed: {fixture.error}; stop={fixture.stop_seen}")
    pixels_after = bytes(fixture.uc.mem_read(SYNTH_SURFACE, len(pixels_before)))
    code_unchanged = bytes(fixture.uc.mem_read(BASE + 0x70C0, 0x80)) == original_span_before
    if not code_unchanged:
        raise RuntimeError(f"{name}: original 70C0 span changed in emulator mapping")
    return {"case": name, "entry_rva": "0x70c0", "argument": hex(argument),
            "color_cell_before": stored_color & 0xFF, "color_cell_after": fixture.uc.mem_read(COLOR_CELL, 1)[0],
            "lock_BA01F0": lock, "dirty_before": dirty & 0xFF,
            "dirty_writes": fixture.dirty_gate_writes,
            "dirty_after": fixture.uc.mem_read(DIRTY, 1)[0],
            "surface_write_count": fixture.remap_write_count,
            "surface_changed_byte_count": sum(a != b for a, b in zip(pixels_before, pixels_after)),
            "first_pixel_after": hex(pixels_after[0]), "original_code_unchanged": True}


def main() -> None:
    parser = argparse.ArgumentParser(description="Run pinned bounded dirty-latch static and native CPU evidence")
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
    parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
    args = parser.parse_args()
    exe, root = args.exe.resolve(), args.repo_root.resolve()
    output_dir = checked_output(args.output_dir, root, exe)
    if sha_file(exe) != PINNED_EXE_SHA256:
        parser.error("--exe does not match the pinned original HERCULES.EXE SHA-256")
    if sha_file(root / "renderer_contract" / "evidence" / "state_ownership.json") != PINNED_STATE_OWNERSHIP_SHA256:
        parser.error("state_ownership.json hash does not match the evidence pin")
    if sha_file(root / "renderer_contract" / "evidence" / "presentation.json") != PINNED_PRESENTATION_SHA256:
        parser.error("presentation.json hash does not match the evidence pin")
    if sha_file(root / "scripts" / "pe.py") != PINNED_PE_HELPER_SHA256:
        parser.error("scripts/pe.py hash does not match the helper pin")
    if output_dir == exe.parent or exe.parent in output_dir.parents:
        parser.error("--output-dir must be outside the original image directory")
    output_dir.mkdir(parents=True, exist_ok=True)
    module, helper_hashes = load_harness(root, exe, output_dir, args.deps.resolve(), args.analysis_deps.resolve())
    runtime = module.verify_runtime(module.unicorn, args.deps.resolve())
    if runtime["unicorn_version"] != PINNED_UNICORN_VERSION or runtime["native_dll_sha256"] != PINNED_UNICORN_DLL_SHA256:
        parser.error("Unicorn runtime pin mismatch")

    # Validate every code interval whose behavior is classified below before execution.
    pe = module.PE(exe)
    spans = []
    for name, (rva, size, expected) in SPAN_PINS.items():
        observed = sha(pe.read_rva(rva, size))
        if observed != expected:
            raise RuntimeError(f"original span hash mismatch for {name}: {observed}")
        spans.append({"name": name, "module": "HERCULES.EXE", "rva": hex(rva),
                      "size": size, "sha256": observed})

    static, static_path, audit_hash, audit_stdout = load_static_audit(root, exe, output_dir)
    set_dirty_hook(module)
    cases = []
    cases.extend([
        run_raster_case(module, exe, "d420_rowzero_transparent_sets", "d420_triangle", expected_site=0xDD5F),
        run_raster_case(module, exe, "d420_cc50_row3_transparent_sets", "d420_triangle", clut_gate=1, row_code=0x78C0, expected_site=0xD407),
        run_raster_case(module, exe, "d420_c2e0_row3_zero_texel_writes_zero_sets", "d420_triangle", row_code=0x78C0, expected_site=0xC6EA),
        run_raster_case(module, exe, "d420_cab0_transparent_rectangle_sets", "d420_cab0", zero_texels=True, expected_site=0xCC37),
        run_raster_case(module, exe, "e680_c8f0_transparent_rectangle_sets", "e680_c8f0", zero_texels=True, expected_site=0xCA9A),
        run_raster_case(module, exe, "d420_cab0_fully_clipped_no_set", "d420_cab0", xshift=320),
        run_raster_case(module, exe, "e680_c8f0_fully_clipped_no_set", "e680_c8f0", xshift=320),
        run_raster_case(module, exe, "d420_cab0_zero_width_no_set", "d420_cab0", zero_width=True),
        run_raster_case(module, exe, "e680_c8f0_zero_width_no_set", "e680_c8f0", zero_width=True),
        run_raster_case(module, exe, "d420_lock_gate_off_no_set", "d420_triangle", draw_gate=0),
        run_raster_case(module, exe, "d420_lock_gate_off_retains_dirty", "d420_triangle", draw_gate=0, seed_dirty=1),
        run_raster_case(module, exe, "d420_general_fully_clipped_no_set", "d420_triangle", xshift=320),
        run_raster_case(module, exe, "d420_cc50_fully_clipped_no_set", "d420_triangle", clut_gate=1, row_code=0x78C0, xshift=320),
        run_raster_case(module, exe, "d420_c2e0_fully_clipped_no_set", "d420_triangle", row_code=0x78C0, xshift=320),
        run_raster_case(module, exe, "d420_equal_x_scan_route_sets_without_reads_or_stores", "d420_triangle", collapse_triangle_x=True, expected_site=0xDD5F),
        run_e680_generic_case(module, exe, "e680_mode80_generic_triangle_transparent_sets", 0x80, expected_site=0xEF52),
        run_e680_generic_case(module, exe, "e680_mode80_generic_fully_clipped_no_set", 0x80, xshift=320),
        run_e680_generic_case(module, exe, "e680_mode80_generic_zero_width_sets", 0x80, zero_width=True, expected_site=0xEF52),
        run_e680_generic_case(module, exe, "e680_mode7f_dd70_transparent_sets", 0x7F, expected_site=0xE667),
        run_e680_generic_case(module, exe, "e680_mode7f_dd70_fully_clipped_no_set", 0x7F, xshift=320),
        run_e680_generic_case(module, exe, "e680_mode80_lock_gate_off_no_set", 0x80, gate=0),
        run_70c0_case(module, exe, "70c0_non_ff_color_setter_does_not_dirty", 0x55, stored_color=0),
        run_70c0_case(module, exe, "70c0_ff_full_surface_remap_does_not_dirty", 0xFF, stored_color=1),
        run_70c0_case(module, exe, "70c0_ff_color80_does_not_remap_or_dirty", 0xFF, stored_color=0x80),
        run_70c0_case(module, exe, "70c0_ff_unlocked_view_does_not_remap_or_dirty", 0xFF, stored_color=1, lock=0),
        run_70c0_case(module, exe, "70c0_ff_remap_retains_existing_dirty", 0xFF, stored_color=1, dirty=1),
    ])

    route_rules = [
        {"route": "D420 row-zero triangle", "observed_eligibility": "with BA01F0=1 and the tested in-bounds row-zero scan path, the route reaches dirty store RVA 0xDD5F even when the sampled texels are zero; an all-X-equal triangle also reaches it with no reads/stores. The tested fully clipped form and BA01F0=0 form do not store.", "not_generalized": "This is not a generic polygon-area or byte-difference rule."},
        {"route": "D420 CC50/C2E0 row-code branches", "observed_eligibility": "with input row code 0x78C0 and BA01F0=1, CC50 gate 1 reaches 0xD407 for the tested transparent samples; gate 0 reaches C2E0 store 0xC6EA and can write zero values. Tested fully clipped inputs do not store.", "not_generalized": "No claim for untested row codes or boundary geometry."},
        {"route": "D420 CAB0 and E680 C8F0 rectangle shortcuts", "observed_eligibility": "the tested 2x2 in-bounds rectangles set at 0xCC37/0xCA9A with all source texels zero; the tested all-X-equal and fully right-clipped rectangles return without setting.", "not_generalized": "The tested zero-width conclusion applies to these rectangle shortcuts only."},
        {"route": "E680 generic mode 0x80", "observed_eligibility": "the tested in-bounds triangle reaches 0xEF52, including all-X-equal input with no pixel work; the fully right-clipped and BA01F0=0 cases do not set.", "not_generalized": "The generic triangle path has different zero-width behavior from the rectangle shortcuts."},
        {"route": "E680 mode 0x7f delegated DD70", "observed_eligibility": "the tested in-bounds transparent triangle sets at 0xE667; the fully right-clipped case does not.", "not_generalized": "No claim for other selector/mode values."},
        {"route": "70C0 color translation/remap", "observed_eligibility": "the bounded routine does not write 0x43A590: a non-0xFF argument updates the color cell; 0xFF with a non-0x80 color and BA01F0=1 remaps the 320x240 synthetic surface; all five tested cases leave the latch unchanged.", "not_generalized": "Do not infer whether another caller requests presentation after the remap."},
    ]
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_DIRTY_GATE_EVIDENCE",
        "scope": "Pinned original x86 routines in finite Unicorn CPU fixtures and a complete .text linear sweep. All surfaces/textures and external state are synthetic; no game, OS, DirectDraw, GDI, or GPU is invoked.",
        "oracle": {"path": str(exe), "sha256": sha_file(exe), "image_base": hex(BASE)},
        "runtime": runtime,
        "helpers": {**helper_hashes,
                    "state_ownership.json": PINNED_STATE_OWNERSHIP_SHA256,
                    "presentation.json": PINNED_PRESENTATION_SHA256,
                    "audit_dirty_refs.py": audit_hash,
                    "audit_dirty_refs_receipt": str(static_path)},
        "original_spans": spans,
        "static_inventory": {"candidate_count": static["candidate_count"],
                              "inside_hash_verified_renderer_spans": static["inside_hash_verified_renderer_spans"],
                              "outside_reviewed_spans": static["outside_reviewed_spans"],
                              "tool": static["tool"],
                              "coverage": static["coverage"],
                              "service_call_candidates": static["surface_service_call_candidates"],
                              "scope": static["scope"]},
        "route_eligibility_findings": route_rules,
        "case_count": len(cases),
        "cases": cases,
        "conclusion": "43A590 behaves as a sticky presentation request in reviewed paths. Preserve the existing byte and let 6620 consume it; do not derive it from changed-pixel count or a universal nonempty-geometry predicate. Match the native route-specific gates/completion behavior or retain that native callback. 70C0 itself does not request presentation.",
        "limits": ["Static callers/references are linear-sweep candidates, not proof of reachability; computed/indirect references and aliases are outside this inventory.",
                  "Raster fixtures cover selected modes and inputs, not all polygons, clipping boundaries, caller sequences, or frame serialization.",
                  "The dirty latch's complete owner/serialization contract remains unresolved."],
        "invocation": {"exe": str(exe), "repo_root": str(root), "output_dir": str(output_dir),
                       "deps": str(args.deps.resolve()), "analysis_deps": str(args.analysis_deps.resolve()),
                       "runner": str(Path(__file__).resolve()), "runner_sha256": sha_file(Path(__file__).resolve())},
    }
    receipt_path = checked_output(output_dir / "dirty_gate.json", root, exe)
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    summary = {"status": receipt["status"], "receipt": str(receipt_path),
               "case_count": len(cases), "static_candidates": static["candidate_count"],
               "inside": static["inside_hash_verified_renderer_spans"], "outside": static["outside_reviewed_spans"],
               "audit": audit_stdout}
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
