#!/usr/bin/env python3
"""Reconcile one native AC60/DIB event stream with the C89 trace validator."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

EXPECTED_EXE_SHA256 = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
EXPECTED_RUN_PREDICTION_SHA256 = "3892ff88452c5dd021184c79b85d5a9bbb2544f99951af588374cd0b2969465f"
EXPECTED_C_PREDICTION_SHA256 = "761156b5ce9a410a045cbc288f9d2e3a0687f7e2c6c236157f92cfbf594dd896"
EXPECTED_REMAP_SPAN = (0x70C0, 0x80, "3432496f55144a415399893358618b7f970f1a263eb4ea80df30083e6494d4e7")
EXPECTED_REMAP_TRACE = [
    0x70C0, 0x70C4, 0x70C5, 0x70C6, 0x70C8, 0x70C9,
    0x70D4, 0x70DB, 0x7132, 0x7133, 0x7134, 0x7135,
]
KIND = {
    "frame": 1, "geometry": 2, "screen_space": 3, "state": 4,
    "texture": 5, "palette_lut": 6, "clear": 7, "lock": 8,
    "move": 9, "callback_entry": 10, "callback_service": 11,
    "link_reload": 12, "packet": 13, "remap_request": 14, "unlock": 15,
    "dirty": 16, "present": 17, "movie": 18, "desktop_text": 19,
    "output": 20, "flush": 21, "unknown": 255,
}
ORIGIN = {"original": 1, "controlled": 2, "static": 3, "bookkeeping": 4, "unresolved": 255}
DOMAIN = {
    "none": 0, "order_table": 1, "geometry": 2, "screen_space": 3,
    "texture": 4, "palette_lut": 5, "cpu_surface": 6,
    "desktop_dc": 7, "display": 8, "external": 9,
}


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--deps", required=True, type=Path,
                        help="existing pinned Unicorn dependency directory")
    parser.add_argument("--analysis-deps", required=True, type=Path,
                        help="existing pefile/Capstone dependency directory")
    parser.add_argument("--cc", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path, help="new directory below repo/work")
    args = parser.parse_args()

    def normalized(path: Path, base: Path | None = None) -> Path:
        candidate = path if path.is_absolute() or base is None else base / path
        candidate = Path(os.path.abspath(candidate))
        if ".." in path.parts:
            parser.error("parent traversal is not accepted in paths")
        current = Path(candidate.anchor)
        for part in candidate.parts[1:]:
            current = current / part
            if current.is_symlink():
                parser.error("symlink/redirected paths are not accepted: " + str(current))
        if candidate.resolve(strict=False) != candidate:
            parser.error("resolved path differs from lexical path: " + str(candidate))
        return candidate

    args.repo_root = normalized(args.repo_root)
    args.exe = normalized(args.exe, args.repo_root)
    args.deps = normalized(args.deps)
    args.analysis_deps = normalized(args.analysis_deps)
    args.cc = normalized(args.cc)
    if not args.cc.is_file() or not args.exe.is_file():
        parser.error("compiler and original EXE must be files")
    if not args.deps.is_dir() or not args.analysis_deps.is_dir():
        parser.error("the existing Unicorn and pefile/Capstone dependency directories are required")
    if sha256_bytes(args.exe.read_bytes()) != EXPECTED_EXE_SHA256:
        parser.error("--exe is not the pinned read-only HERCULES.EXE")
    work = args.repo_root / "work"
    if work.resolve() != work:
        parser.error("refusing a redirected repo/work directory")
    args.output = normalized(args.output, args.repo_root)
    try:
        args.output.relative_to(work)
    except ValueError:
        parser.error("--output must resolve below repo/work")
    if args.output.exists() and any(args.output.iterdir()):
        parser.error("--output must be new or empty")
    assets = (args.repo_root / "assets").resolve()
    if args.output == assets or assets in args.output.parents:
        parser.error("refusing output under assets")
    original_dir = args.exe.parent
    if args.output == original_dir or original_dir in args.output.parents:
        parser.error("refusing output inside the original image directory")
    return args


def static_remap_guard(exe: Path, analysis_deps: Path) -> dict:
    sys.path.insert(0, str(analysis_deps))
    import pefile  # type: ignore[import-not-found]
    import capstone  # type: ignore[import-not-found]
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # type: ignore[import-not-found]

    if getattr(capstone, "__version__", None) != "5.0.7":
        raise ValueError("static guard decode requires the existing pinned Capstone 5.0.7")
    pe = pefile.PE(str(exe), fast_load=True)
    rva, size, expected_hash = EXPECTED_REMAP_SPAN
    span = bytes(pe.get_data(rva, size))
    if len(span) != size or sha256_bytes(span) != expected_hash:
        raise ValueError("original 0x70C0 span does not match the pinned 128-byte code span")
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    instructions = list(decoder.disasm(span[:0x1D], rva))
    proof = [(item.address, item.mnemonic, item.op_str) for item in instructions]
    expected = [
        (0x70C0, "mov", "al, byte ptr [esp + 4]"),
        (0x70C4, "push", "ebx"),
        (0x70C5, "push", "esi"),
        (0x70C6, "cmp", "al, 0xff"),
        (0x70C8, "push", "edi"),
        (0x70C9, "je", "0x70d4"),
        (0x70CB, "mov", "byte ptr [0x436a0c], al"),
        (0x70D0, "pop", "edi"),
        (0x70D1, "pop", "esi"),
        (0x70D2, "pop", "ebx"),
        (0x70D3, "ret", ""),
        (0x70D4, "cmp", "byte ptr [0x436a0c], 0x80"),
        (0x70DB, "je", "0x7132"),
    ]
    if proof != expected:
        raise ValueError("Capstone decode differs from pinned 0x70C0 guard sequence")
    return {
        "original_sha256": EXPECTED_EXE_SHA256,
        "span_rva": hex(rva), "span_size": size, "span_sha256": expected_hash,
        "guard_instruction_bytes": span[:0x22].hex(),
        "instructions": [
            {"rva": hex(address), "mnemonic": mnemonic, "operands": operands}
            for address, mnemonic, operands in proof
        ],
        "guard": "argument 0xFF branches at 0x70C9 to 0x70D4; byte [0x436A0C] is compared with 0x80; JE at 0x70DB skips to 0x7132, bypassing the pixel loop beginning 0x70DD",
        "decoder": "Capstone 5.0.7 x86-32; existing analysis dependency directory",
    }


def integer(value: str | int) -> int:
    return int(value, 0) if isinstance(value, str) else int(value)


def required_trace_step(case: dict, rva: int, occurrence: int = 0) -> int:
    matches = [
        integer(row["instruction_count"])
        for row in case["original_instruction_trace_events"]
        if integer(row["rva"]) == rva
    ]
    if len(matches) <= occurrence:
        raise ValueError("original trace lacks RVA %s occurrence %d" % (hex(rva), occurrence))
    return matches[occurrence]


def add_event(rows: list[dict], source_order: int, kind: str, origin: str,
              domain: str, source_rva: int, raw_token: int = 0,
              arguments: tuple[int, int, int, int] = (0, 0, 0, 0),
              flags: int = 0) -> None:
    rows.append({
        "source_order": source_order, "kind": KIND[kind], "origin": ORIGIN[origin],
        "domain": DOMAIN[domain], "semantic_owner": 0, "source_rva": source_rva,
        "raw_token": raw_token, "arguments": arguments, "flags": flags,
    })


def native_events(receipt: dict, repo_root: Path, package: Path) -> list[dict]:
    if receipt.get("status") != "BOUNDED_EQUIVALENT":
        raise ValueError("native fixture did not pass its own bounded checks")
    if receipt.get("oracle", {}).get("sha256") != EXPECTED_EXE_SHA256:
        raise ValueError("native receipt is not pinned to the expected original")
    provenance = receipt.get("source_provenance", {})
    source_pins = {
        "this_runner_sha256": package / "tests" / "test_original_composition_frame.py",
        "prediction_sha256": package / "tests" / "prediction.json",
        "reused_fixture_api_sha256": repo_root / "renderer_contract" / "tests" / "original_ac60_mixed_runner.py",
        "oracle_common_sha256": repo_root / "renderer_contract" / "tests" / "oracle_common.py",
    }
    for field, path in source_pins.items():
        if provenance.get(field) != sha256_bytes(path.read_bytes()):
            raise ValueError("native source provenance mismatch for " + str(path))
    closure = (repo_root / "renderer_contract" / "closure.md").read_text(
        encoding="utf-8", errors="replace")
    if "70C0(0xFF)" not in closure:
        raise ValueError("static AC60 remap argument pin is missing")
    cases = {case["scenario"]: case for case in receipt.get("cases", [])}
    case = cases.get("rewrite_link")
    if case is None:
        raise ValueError("native receipt has no callback-mutated-link scenario")
    if [node["name"] for node in case["native_consumed_packet_boundaries"]] != [
        "e1_current", "cmd01", "e1_alt", "ot_gate",
    ]:
        raise ValueError("original AC60 did not consume the callback-rewritten link")
    reloads = [
        item for item in case["live_ac60_link_reload_boundaries"]
        if item.get("current_packet") == "cmd01"
    ]
    if len(reloads) != 1 or reloads[0].get("loaded_packet_after_original_mov") != "e1_alt":
        raise ValueError("native ACA2 reload does not select e1_alt")
    callbacks = case["synthetic_host_service_calls"]
    if [integer(item["return_rva"]) for item in callbacks] != [0xABE9, 0xB2E7]:
        raise ValueError("native callback return PCs do not match the expected nested/tail sequence")
    services = case.get("controlled_external_services", [])
    if [item["service"] for item in services] != ["StretchDIBits", "GdiFlush"]:
        raise ValueError("unexpected or missing controlled display endpoints")
    if case.get("surface_route_fixture", {}).get("mode") != "windowed DIB":
        raise ValueError("native case is not the bounded windowed DIB route")
    if case.get("cpu_surface_write_byte_count") != 0:
        raise ValueError("this state-only fixture unexpectedly wrote DIB bytes")
    remap_span = receipt.get("oracle", {}).get("executed_spans", {}).get("REMAP_70C0", {})
    if (integer(remap_span.get("rva", "0x0")), remap_span.get("size"),
            remap_span.get("sha256")) != EXPECTED_REMAP_SPAN:
        raise ValueError("native receipt lacks the hash-pinned original 0x70C0 span")
    remap_trace = [
        integer(row["rva"]) for row in case["original_instruction_trace_events"]
        if 0x70C0 <= integer(row["rva"]) < 0x7140
    ]
    if remap_trace != EXPECTED_REMAP_TRACE:
        raise ValueError("native 0x70C0 path did not take the pinned 0x80 guard branch")
    if any(integer(write["address"]) == 0x436A0C
           for write in case["original_data_write_events"]):
        raise ValueError("native fixture unexpectedly changed the 0x70C0 color guard cell")
    if case.get("cpu_surface_write_events") or case.get("cpu_surface_write_byte_count") != 0:
        raise ValueError("the skipped-remap fixture unexpectedly wrote CPU surface pixels")
    trace_rvas = {integer(row["rva"]) for row in case["original_instruction_trace_events"]}
    if any(0x6EE0 <= rva < 0x6FF0 for rva in trace_rvas):
        raise ValueError("the supposedly absent clear route executed in the connected fixture")
    if any(rva in trace_rvas for rva in (0xD420, 0xE680, 0xCAB0, 0xC8F0)):
        raise ValueError("the state-only connected fixture entered a rasterizer")
    expected_stretch = [
        "0x36000000", "0x0", "0x0", "0x140", "0xf0", "0x0", "0x0",
        "0x140", "0xf0", "0x34000000", "0x35000000", "0x0", "0xcc0020",
    ]
    if services[0].get("stack_arguments") != expected_stretch:
        raise ValueError("original StretchDIBits endpoint arguments changed")
    dirty_consumes = [
        row for row in case["original_data_write_events"]
        if integer(row["pc_rva"]) == 0x6637 and integer(row["address"]) == 0x43A590
        and integer(row["value"]) == 0
    ]
    if len(dirty_consumes) != 1:
        raise ValueError("original 6620 did not clear the seeded dirty byte exactly once")

    rows: list[dict] = []
    add_event(rows, required_trace_step(case, 0xAC60), "frame", "original", "order_table", 0xAC60)
    lock_pitch = next(
        integer(write["value"])
        for write in case["original_data_write_events"]
        if integer(write["address"]) == 0x00BA0214 and
        integer(write["pc_rva"]) == 0x709B
    )
    add_event(rows, required_trace_step(case, 0x6FF0), "lock", "original", "cpu_surface", 0x6FF0,
              raw_token=integer(case["surface_route_fixture"]["pixel_token"]),
              arguments=(lock_pitch, 0, 0, 0))

    packets = {node["name"]: node for node in case["native_consumed_packet_boundaries"]}
    for name in ("e1_current", "cmd01", "e1_alt", "ot_gate"):
        node = packets[name]
        add_event(rows, integer(node["instruction_count"]), "packet", "original", "order_table",
                  0xACB9, integer(node["address"]),
                  (integer(node["tag"]), integer(node["next"]), 0, 0))

    for write in case["original_data_write_events"]:
        rva = integer(write["pc_rva"])
        if rva in (0xB1B0, 0xB1BC, 0xB1CA):
            add_event(rows, integer(write["instruction_count"]), "state", "original", "none",
                      rva, integer(write["address"]),
                      (integer(write["value"]), integer(write["size"]), 0, 0))

    move = case["a8f0_original_entry"][0]
    packet_token = integer(move["stack_dwords"][1])
    add_event(rows, integer(move["instruction_count"]), "move", "original", "texture",
              0xA8F0, packet_token,
              (integer(move["destination_s16"][0]), integer(move["destination_s16"][1]),
               integer(move["rect_s16"][2]), integer(move["rect_s16"][3])))

    bd_entries = case["bd80_original_call_boundaries"]
    for index, entry in enumerate(bd_entries):
        add_event(rows, integer(entry["instruction_count"]), "callback_entry", "original",
                  "external", 0xBD80, integer(entry["callback_pointer_at_entry"]),
                  (integer(entry["call_return_rva"]), 0, 0, 0))

    reload = reloads[0]
    add_event(rows, integer(reload["instruction_count"]), "link_reload", "original",
              "order_table", 0xACA2, integer(reload["current_packet_va"]),
              (integer(reload["loaded_esi_after_original_mov"]), 0, 0, 0))

    for service in callbacks:
        mutation = service.get("mutation") or {}
        add_event(rows, integer(service["instruction_count"]), "callback_service", "controlled",
                  "external", 0, integer(service["endpoint"]),
                  (integer(service["return_rva"]), integer(mutation.get("address", "0x0")),
                   integer(mutation.get("old", "0x0")), integer(mutation.get("new", "0x0"))))

    add_event(rows, required_trace_step(case, 0x70C0), "remap_request", "original", "cpu_surface", 0x70C0,
              raw_token=integer(case["surface_route_fixture"]["pixel_token"]),
              arguments=(0xFF, 0, 0, 0))
    add_event(rows, required_trace_step(case, 0x7140), "unlock", "original", "cpu_surface", 0x7140)
    add_event(rows, required_trace_step(case, 0x6620), "present", "original", "display", 0x6620,
              arguments=(1, 0, 0, 0))
    add_event(rows, integer(services[0]["instruction_count"]), "output", "controlled", "display",
              0, integer(services[0]["stack_arguments"][0]),
              (integer(services[0]["return_address_rva"]), 0, 0, 0))
    add_event(rows, integer(services[1]["instruction_count"]), "flush", "controlled", "display",
              0, 0, (integer(services[1]["return_address_rva"]), 0, 0, 0))

    rows.sort(key=lambda item: item["source_order"])
    if len({row["source_order"] for row in rows}) != len(rows):
        raise ValueError("two distinct modeled operations have the same source instruction order")
    for index, row in enumerate(rows):
        row["scope_id"] = 1
        row["local_order"] = index
    return rows


def write_tsv(path: Path, events: list[dict]) -> None:
    with path.open("w", encoding="ascii", newline="\n") as stream:
        for event in events:
            values = [
                event["scope_id"], event["local_order"], event["source_order"],
                event["kind"], event["origin"], event["domain"], event["semantic_owner"],
                event["source_rva"], event["raw_token"], *event["arguments"], event["flags"],
            ]
            stream.write(" ".join(str(int(value)) for value in values) + "\n")


def read_tsv(path: Path) -> list[dict]:
    rows = []
    for index, line in enumerate(path.read_text(encoding="ascii").splitlines()):
        values = [int(value, 10) for value in line.split()]
        if len(values) != 14:
            raise ValueError("C reconstruction row %d has %d fields" % (index, len(values)))
        rows.append({
            "scope_id": values[0], "local_order": values[1],
            "source_order": values[2], "kind": values[3],
            "origin": values[4], "domain": values[5],
            "semantic_owner": values[6], "source_rva": values[7],
            "raw_token": values[8], "arguments": values[9:13],
            "flags": values[13],
        })
    return rows


def event_signature(event: dict) -> tuple:
    return (event["kind"], event["domain"], event["semantic_owner"],
            event["source_rva"], event["raw_token"], tuple(event["arguments"]))


def compare_reconstructed_events(native: list[dict], reconstructed: list[dict]) -> None:
    if len(native) != 23 or len(reconstructed) != len(native):
        raise ValueError("native/C event count differs from the bounded 23-event subset")
    if [event_signature(row) for row in native] != [event_signature(row) for row in reconstructed]:
        for index, (left, right) in enumerate(zip(native, reconstructed)):
            if event_signature(left) != event_signature(right):
                raise ValueError("native/C event signature differs at index %d: %r != %r" %
                                 (index, event_signature(left), event_signature(right)))
        raise ValueError("native/C event count differs")
    for index, row in enumerate(reconstructed):
        if row["scope_id"] != 1 or row["local_order"] != index or row["flags"] != 1:
            raise ValueError("C event provenance/order marker mismatch at index %d" % index)
        should_be_controlled = row["kind"] in (11, 20, 21)
        if row["origin"] != (2 if should_be_controlled else 3):
            raise ValueError("C event origin is not marked reconstructed/controlled at index %d" % index)


def run(argv: list[str], cwd: Path) -> dict:
    result = subprocess.run(argv, cwd=str(cwd), shell=False, capture_output=True,
                            text=True, errors="replace", timeout=60)
    return {"argv": argv, "returncode": result.returncode,
            "stdout": result.stdout, "stderr": result.stderr}


def main() -> int:
    args = parse_args()
    package = Path(__file__).resolve().parents[1]
    runner_path = package / "tests" / "test_original_composition_frame.py"
    run_prediction_path = package / "tests" / "prediction.json"
    c_prediction_path = package / "evidence" / "c_orchestration_prediction.json"
    if sha256_bytes(run_prediction_path.read_bytes()) != EXPECTED_RUN_PREDICTION_SHA256:
        raise ValueError("immutable pre-experiment original-run prediction bytes changed")
    if sha256_bytes(c_prediction_path.read_bytes()) != EXPECTED_C_PREDICTION_SHA256:
        raise ValueError("immutable pre-experiment C-model prediction bytes changed")
    args.output.mkdir(parents=True, exist_ok=True)
    static_guard = static_remap_guard(args.exe, args.analysis_deps)
    pinned_guard_path = package / "evidence" / "remap_guard.json"
    pinned_guard = json.loads(pinned_guard_path.read_text(encoding="utf-8"))
    if pinned_guard != static_guard:
        raise ValueError("remap_guard.json does not match original bytes and current static decode")
    native_receipt_path = args.output / "native_original_receipt.json"
    native_command = [
        sys.executable, "-B", str(runner_path),
        "--exe", str(args.exe), "--original-root", str(args.repo_root),
        "--deps", str(args.deps), "--analysis-deps", str(args.analysis_deps),
        "--receipt", str(native_receipt_path),
    ]
    native_run = run(native_command, args.repo_root)
    if native_run["returncode"] != 0 or not native_receipt_path.is_file():
        raise RuntimeError("fresh unchanged-original fixture failed: " + native_run["stderr"])
    source_receipt_bytes = native_receipt_path.read_bytes()
    receipt = json.loads(source_receipt_bytes.decode("utf-8-sig"))
    events = native_events(receipt, args.repo_root, package)
    tsv = args.output / "connected_frame_events.tsv"
    write_tsv(tsv, events)
    event_json = args.output / "connected_frame_events.json"
    event_json.write_text(json.dumps({
        "scope_id": 1,
        "ordering": "source_order is native Unicorn instruction count; local_order is tool bookkeeping within this one fixture scope",
        "events": events,
        "cross_scope_order": "not asserted",
    }, indent=2) + "\n", encoding="utf-8")

    executable_test = args.output / "test_composition_events.exe"
    executable_stream = args.output / "validate_composition_stream.exe"
    source = package / "src" / "composition_events.c"
    tests = package / "tests"
    compile_test = run([str(args.cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                        str(source), str(tests / "test_composition_events.c"),
                        "-o", str(executable_test)], args.output)
    if compile_test["returncode"] != 0:
        raise RuntimeError("C89 API test compilation failed: " + compile_test["stderr"])
    test_run = run([str(executable_test)], args.output)
    if test_run["returncode"] != 0:
        raise RuntimeError("C89 API tests failed: " + test_run["stderr"])
    compile_stream = run([str(args.cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
                         str(source), str(tests / "validate_composition_stream.c"),
                         "-o", str(executable_stream)], args.output)
    if compile_stream["returncode"] != 0:
        raise RuntimeError("C89 stream validator compilation failed: " + compile_stream["stderr"])
    stream_run = run([str(executable_stream), str(tsv)], args.output)
    if stream_run["returncode"] != 0:
        raise RuntimeError("C89 native event stream validation failed: " + stream_run["stderr"])

    renderer_contract = args.repo_root / "renderer_contract"
    orchestration_source = package / "src" / "composition_orchestration.c"
    orchestration_test = tests / "test_composition_orchestration.c"
    executable_orchestration = args.output / "test_composition_orchestration.exe"
    reconstructed_tsv = args.output / "reconstructed_composition_events.tsv"
    compile_orchestration = run([
        str(args.cc), "-std=c89", "-pedantic-errors", "-Wall", "-Wextra",
        "-I", str(package / "src"), "-I", str(renderer_contract),
        str(source), str(orchestration_source),
        str(renderer_contract / "hercules_frame.c"),
        str(renderer_contract / "hercules_packets.c"),
        str(renderer_contract / "hercules_state.c"),
        str(renderer_contract / "hercules_staging.c"),
        str(orchestration_test), "-o", str(executable_orchestration),
    ], args.output)
    if compile_orchestration["returncode"] != 0:
        raise RuntimeError("C89 orchestration fixture compilation failed: " +
                           compile_orchestration["stderr"])
    orchestration_run = run([str(executable_orchestration), str(reconstructed_tsv)],
                            args.output)
    if orchestration_run["returncode"] != 0:
        raise RuntimeError("C89 orchestration fixture failed: " +
                           orchestration_run["stderr"])
    reconstructed_events = read_tsv(reconstructed_tsv)
    compare_reconstructed_events(events, reconstructed_events)
    reconstructed_json = args.output / "reconstructed_composition_events.json"
    reconstructed_json.write_text(json.dumps({
        "scope_id": 1,
        "ordering": "source_order is C reconstructed operation sequence; it is not a native instruction count",
        "provenance": "C orchestration over the reused bounded frame walker and packet staging; controlled providers named by service; flags mark reconstruction",
        "native_comparison": "same ordered event signatures as native rewrite_link projection after source-order and provenance-origin fields are excluded",
        "events": reconstructed_events,
        "cross_scope_order": "not asserted",
    }, indent=2) + "\n", encoding="utf-8")

    closure_path = renderer_contract / "closure.md"
    contract_report_path = package.parents[1] / "composition_ordering.md"
    source_file_paths = {
        str((package / "src" / "composition_events.h").relative_to(args.repo_root)): package / "src" / "composition_events.h",
        str((package / "src" / "composition_events.c").relative_to(args.repo_root)): source,
        str((package / "src" / "composition_orchestration.h").relative_to(args.repo_root)): package / "src" / "composition_orchestration.h",
        str(orchestration_source.relative_to(args.repo_root)): orchestration_source,
        str((tests / "test_composition_events.c").relative_to(args.repo_root)): tests / "test_composition_events.c",
        str((tests / "validate_composition_stream.c").relative_to(args.repo_root)): tests / "validate_composition_stream.c",
        str(orchestration_test.relative_to(args.repo_root)): orchestration_test,
        str(Path(__file__).resolve().relative_to(args.repo_root)): Path(__file__).resolve(),
        str((tests / "test_original_composition_frame.py").relative_to(args.repo_root)): tests / "test_original_composition_frame.py",
        str((tests / "prediction.json").relative_to(args.repo_root)): tests / "prediction.json",
        str((package / "evidence" / "c_orchestration_prediction.json").relative_to(args.repo_root)): package / "evidence" / "c_orchestration_prediction.json",
        str((package / "evidence" / "event_schema.json").relative_to(args.repo_root)): package / "evidence" / "event_schema.json",
        str(contract_report_path.relative_to(args.repo_root)): contract_report_path,
        "renderer_contract/hercules_frame.h": renderer_contract / "hercules_frame.h",
        "renderer_contract/hercules_frame.c": renderer_contract / "hercules_frame.c",
        "renderer_contract/hercules_types.h": renderer_contract / "hercules_types.h",
        "renderer_contract/hercules_packets.h": renderer_contract / "hercules_packets.h",
        "renderer_contract/hercules_packets.c": renderer_contract / "hercules_packets.c",
        "renderer_contract/hercules_state.h": renderer_contract / "hercules_state.h",
        "renderer_contract/hercules_state_wire.h": renderer_contract / "hercules_state_wire.h",
        "renderer_contract/hercules_state.c": renderer_contract / "hercules_state.c",
        "renderer_contract/hercules_staging.h": renderer_contract / "hercules_staging.h",
        "renderer_contract/hercules_staging.c": renderer_contract / "hercules_staging.c",
        "renderer_contract/closure.md": closure_path,
        "renderer_contract/tests/original_ac60_mixed_runner.py": renderer_contract / "tests" / "original_ac60_mixed_runner.py",
        "renderer_contract/tests/oracle_common.py": renderer_contract / "tests" / "oracle_common.py",
        str((package / "evidence" / "remap_guard.json").relative_to(args.repo_root)): pinned_guard_path,
        str((package / "evidence" / "integration_notes.md").relative_to(args.repo_root)): package / "evidence" / "integration_notes.md",
    }
    source_file_map = {
        name: sha256_bytes(path.read_bytes())
        for name, path in source_file_paths.items()
    }

    audit = {
        "schema_version": 1,
        "status": "CONNECTED_BOUNDED_SUBSET_PASS",
        "original_game_executed": False,
        "original_exe_sha256": EXPECTED_EXE_SHA256,
        "native_receipt": str(native_receipt_path.relative_to(args.repo_root)),
        "native_receipt_sha256": sha256_bytes(source_receipt_bytes),
        "native_receipt_origin": "fresh unchanged-original fixture run created by this audit; not a mutable pre-existing receipt",
        "fresh_native_fixture_run": native_run,
        "static_remap_guard": static_guard,
        "source_file_map": source_file_map,
        "native_scenario": "rewrite_link",
        "native_event_count": len(events),
        "native_control_flow_findings": {
            "command01_callback_mutates_live_next": True,
            "aca2_reload_selects_mutated_packet": True,
            "same_original_frame_reaches_6ff0_70c0_7140_6620": True,
            "windowed_stretchdibits_and_gdiflush_endpoints": "controlled synthetic providers; no OS/GDI call",
            "surface_writes": "zero for this state-only route; no pixel output equivalence",
            "70c0_0xff_result": "remap request only; original guard compares byte [0x436A0C] with 0x80 at 0x70D4 and the observed JE at 0x70DB takes 0x7132, skipping the pixel loop at 0x70DD; zero CPU-surface write events",
        },
        "c_reconstruction": {
            "status": "PASS_SAME_BOUNDED_EVENT_SIGNATURE",
            "event_count": len(reconstructed_events),
            "event_order": "independently emitted by C; same ordered signatures as native projection",
            "source_order_semantics": "C operation sequence, not native instruction counts",
            "provenance_flag": "COMPOSITION_EVENT_FLAG_C_RECONSTRUCTION",
            "unknown_raster_packet": "explicit incomplete-unknown failure asserted by C fixture",
            "controlled_provider_order": orchestration_run["stdout"].strip(),
            "provider_contract": "zero-height A8F0 request; lock/remap/unlock/present request; exact 13-argument synthetic StretchDIBits; synthetic GdiFlush; two synchronous synthetic callback services; no OS/GDI/COM call",
            "pixel_effect": "none; CPU-visible byte output is not implemented or claimed",
        },
        "c_api": {
            "pointer_free_event_record": True,
            "raw32_addresses_only": True,
            "semantic_owner": "unclassified for all events",
            "unknown_writer_or_unresolved_origin": "retained then non-success",
            "overflow": "non-success",
            "api_test": {"compile": compile_test, "run": test_run},
            "trace_validator": {"compile": compile_stream, "run": stream_run},
            "orchestration_fixture": {
                "compile": compile_orchestration,
                "run": orchestration_run,
                "event_sequence_match": True,
                "native_events": len(events),
                "c_events": len(reconstructed_events),
            },
            "sources": {
                "header": sha256_bytes((package / "src" / "composition_events.h").read_bytes()),
                "implementation": sha256_bytes(source.read_bytes()),
                "api_test": sha256_bytes((tests / "test_composition_events.c").read_bytes()),
                "stream_validator": sha256_bytes((tests / "validate_composition_stream.c").read_bytes()),
                "orchestration_header": sha256_bytes((package / "src" / "composition_orchestration.h").read_bytes()),
                "orchestration_implementation": sha256_bytes(orchestration_source.read_bytes()),
                "orchestration_test": sha256_bytes(orchestration_test.read_bytes()),
                "reused_frame_walker": sha256_bytes((renderer_contract / "hercules_frame.c").read_bytes()),
                "reused_packet_decoder": sha256_bytes((renderer_contract / "hercules_packets.c").read_bytes()),
                "reused_state_decoder": sha256_bytes((renderer_contract / "hercules_state.c").read_bytes()),
                "reused_packet_staging": sha256_bytes((renderer_contract / "hercules_staging.c").read_bytes()),
                "closure_static_argument_pin": sha256_bytes(closure_path.read_bytes()),
                "audit": sha256_bytes(Path(__file__).read_bytes()),
            },
        },
        "nonclaims": [
            "No geometry/rendering pixels or UI/world layer assignment is established.",
            "No palette, movie, GDI text, framework paint or DirectDraw route was observed in this same frame fixture.",
            "No ordering among independent callback routes, OS reentrancy or computed writers is inferred.",
            "The C event stream is a bounded source reconstruction with controlled providers, not original instruction execution, a live host ABI or compositor.",
        ],
    }
    out_receipt = args.output / "composition_audit.json"
    out_receipt.write_text(json.dumps(audit, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": audit["status"], "events": len(events),
                      "c_reconstruction": audit["c_reconstruction"]["status"],
                      "native_receipt_sha256": audit["native_receipt_sha256"],
                      "audit": str(out_receipt)}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        import traceback
        traceback.print_exc()
        print(type(exc).__name__ + ": " + str(exc), file=sys.stderr)
        raise SystemExit(2)
