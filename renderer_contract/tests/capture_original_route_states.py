"""Capture full loop-boundary states from the pinned 17-case native oracle."""
import argparse
import importlib.util
import json
import sys
from pathlib import Path


def find_root(start: Path) -> Path:
    for parent in (start.resolve(), *start.resolve().parents):
        if ((parent / "scripts" / "pe.py").is_file() and
                (parent / "renderer_contract" / "tests" / "oracle_common.py").is_file() and
                (parent / "work").is_dir()):
            return parent
    raise RuntimeError("repository root not found")


ROOT = find_root(Path(__file__))
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--package", type=Path, default=ROOT / "renderer_contract",
                    help="package directory containing the C model sources")
parser.add_argument("--fixture-helper", type=Path,
                    default=ROOT / "renderer_contract" / "tests" / "test_original_route_uv_feedback.py",
                    help="canonical public original-byte route fixture/helper")
parser.add_argument("--exe", required=True, type=Path)
parser.add_argument("--original-root", required=True, type=Path)
parser.add_argument("--receipt", required=True, type=Path)
parser.add_argument("--deps", type=Path, default=Path(r"C:\tools\hercules\renderer-oracle-python"))
parser.add_argument("--analysis-deps", type=Path, default=Path(r"C:\tools\hercules\python"))
ARGS = parser.parse_args()
for key in ("package", "fixture_helper", "exe", "original_root", "receipt", "deps", "analysis_deps"):
    setattr(ARGS, key, getattr(ARGS, key).resolve())
if ARGS.original_root != ROOT.resolve():
    parser.error("--original-root must name the discovered repository root")
for key in ("receipt",):
    try:
        getattr(ARGS, key).relative_to((ROOT / "work").resolve())
    except ValueError as error:
        raise SystemExit(f"--receipt must stay below repository work/ ({ARGS.receipt} vs {ROOT / 'work'})") from error

if (not ARGS.package.is_dir() or not ARGS.fixture_helper.is_file() or
        not ARGS.exe.is_file() or not ARGS.deps.is_dir() or not ARGS.analysis_deps.is_dir()):
    parser.error("package, original executable, fixture helper, or pinned dependency directory is missing")

PACKAGE_RUNNER = ARGS.fixture_helper
saved_argv = sys.argv[:]
sys.argv = [str(PACKAGE_RUNNER), "--exe", str(ARGS.exe), "--original-root", str(ROOT),
            "--receipt", str(ARGS.receipt), "--deps", str(ARGS.deps),
            "--analysis-deps", str(ARGS.analysis_deps)]
spec = importlib.util.spec_from_file_location("phase8_native_route_runner", PACKAGE_RUNNER)
if spec is None or spec.loader is None:
    raise RuntimeError("could not load the prior pinned native route runner")
native_route = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = native_route
spec.loader.exec_module(native_route)
sys.argv = saved_argv

ROUTE_CASES = [
    ("cc50_palette", "raw_1", dict(source=1, output=0x44)),
    ("cc50_palette", "raw_255", dict(source=255, output=0x44)),
    ("cc50_palette", "clut_44", dict(source=1, output=0x44)),
    ("cc50_palette", "clut_e1", dict(source=1, output=0xE1)),
    ("cc50_palette", "all_zero", dict(source=0, output=0xE1, all_zero=True)),
    ("e680_mode80", "raw_1", dict(source=1, source_values=(1,), output=0x66)),
    ("e680_mode80", "raw_255", dict(source=255, source_values=(1, 255), output=0x66)),
    ("e680_mode80", "blend_66", dict(source_values=(1,), output=0x66)),
    ("e680_mode80", "blend_e2", dict(source_values=(1,), output=0xE2)),
    ("e680_mode80", "all_zero", dict(source_values=(1,), output=0x66, all_zero=True)),
    ("e680_shaded_7f", "raw_1", dict(source=1, source_values=(1,), output=0x66,
                                       shade_value=0x22, shade_values=(0x22,))),
    ("e680_shaded_7f", "raw_255", dict(source=255, source_values=(1, 255), output=0x66,
                                          shade_value=0x22, shade_values=(0x22,))),
    ("e680_shaded_7f", "shade_22", dict(source_values=(1,), output=0x66,
                                          shade_value=0x22, shade_values=(0x22,))),
    ("e680_shaded_7f", "shade_d0", dict(source_values=(1,), output=0x66,
                                          shade_value=0xD0, shade_values=(0x22, 0xD0))),
    ("e680_shaded_7f", "blend_66", dict(source_values=(1,), output=0x66,
                                          shade_value=0x22, shade_values=(0x22,))),
    ("e680_shaded_7f", "blend_e4", dict(source_values=(1,), output=0xE4,
                                          shade_value=0x22, shade_values=(0x22,))),
    ("e680_shaded_7f", "all_zero", dict(source_values=(1,), output=0x66,
                                          shade_value=0x22, shade_values=(0x22,), all_zero=True)),
]
EXIT_RVA = {"cc50_palette": 0xD348, "e680_mode80": 0xEE93, "e680_shaded_7f": 0xE5A8}
CAPTURED = []


_base_state = native_route.state


def full_state(uc):
    state = _base_state(uc)
    state["esp"] = uc.reg_read(native_route.UC_X86_REG_ESP) & 0xFFFFFFFF
    return state


native_route.state = full_state


def add_exit_hook(native, route_name, capture):
    exit_address = native_route.BASE + EXIT_RVA[route_name]

    def on_exit(uc, address, _size, _user):
        if address == exit_address:
            capture["exit_state"] = full_state(uc)

    native.uc.hook_add(native_route.UC_HOOK_CODE, on_exit)


_instrumentation = native_route.instrumentation


def instrument_with_exit(native, route_name):
    capture = _instrumentation(native, route_name)
    add_exit_hook(native, route_name, capture)
    return capture


native_route.instrumentation = instrument_with_exit
_summarize_case = native_route.summarize_case


def summarize_and_keep(route_name, label, native, result, capture, all_zero):
    summary = _summarize_case(route_name, label, native, result, capture, all_zero)
    globals_read = {}
    for label_address, address, size in (
            ("step32", 0xA90998, 4), ("step8", 0xA90978, 1),
            ("pointer_correction", 0xA9099C, 4), ("shade_base", 0xA909A0, 4)):
        globals_read[label_address] = int.from_bytes(native.uc.mem_read(address, size), "little")
    CAPTURED.append({
        "route": route_name,
        "label": label,
        "all_zero": all_zero,
        "summary": summary,
        "globals": globals_read,
        "loop_states": [full_state_from_capture(row["states"]["loop_start"])
                        for row in capture["iterations"]],
        "post_uv_states": [full_state_from_capture(row["states"]["after_uv_step"])
                           for row in capture["iterations"]],
        "exit_state": capture.get("exit_state"),
        "events": capture["events"],
    })
    return summary


def full_state_from_capture(state):
    return dict(state)


native_route.summarize_case = summarize_and_keep


def main():
    native_route.fixture_geometry()
    pe = native_route.PE(ARGS.exe)
    if native_route.sha(pe.data) != native_route.EXPECTED_EXE_SHA256 or pe.image_base != native_route.BASE:
        raise RuntimeError("pinned original EXE identity mismatch")
    original_spans = native_route.verify_spans(pe)
    native_route.quad.verify_runtime(native_route.unicorn, ARGS.deps)
    data_section = next(section for section in pe.sections if section["name"] == ".data")
    data_region = (native_route.BASE + data_section["virtual_address"],
                   max(data_section["virtual_size"], data_section["raw_size"]))
    native_route.MAX_INSTRUCTIONS = 250_000
    for route_name, label, kwargs in ROUTE_CASES:
        native_route.run_case_with_capture(pe, data_region, route_name, label, **kwargs)
    if len(CAPTURED) != 17:
        raise RuntimeError("native trace count differs from the established 17-case family")
    receipt = {
        "schema_version": 1,
        "status": "NATIVE_TRACE_CAPTURED",
        "scope": "The established 17 cases, pinned original route loops, and full 32-bit register snapshots at loop-start and post-UV landmarks. No source, LUT, or fixture values are generated by this capture helper.",
        "oracle": {"path": str(pe.path), "sha256": native_route.sha(pe.data), "image_base": hex(pe.image_base)},
        "original_spans": original_spans,
        "runtime": native_route.quad.verify_runtime(native_route.unicorn, ARGS.deps),
        "runner": {"path": str(PACKAGE_RUNNER), "sha256": native_route.sha(PACKAGE_RUNNER.read_bytes())},
        "runs": CAPTURED,
        "invocation": {"receipt": str(ARGS.receipt), "original_root": str(ROOT),
                       "deps": str(ARGS.deps), "analysis_deps": str(ARGS.analysis_deps),
                       "package": str(ARGS.package), "fixture_helper": str(ARGS.fixture_helper)},
    }
    ARGS.receipt.parent.mkdir(parents=True, exist_ok=True)
    ARGS.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"],
                      "runs": len(CAPTURED),
                      "iterations": sum(len(row["loop_states"]) for row in CAPTURED),
                      "receipt": str(ARGS.receipt)}, indent=2))


if __name__ == "__main__":
    main()
