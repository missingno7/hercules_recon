#!/usr/bin/env python3
"""Run public-helper original-code fixtures and the shared VC5 test adapter."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "renderer_contract" / "tests"))
from oracle_common import EXPECTED_EXE_SHA256, parse_args, write_receipt  # noqa: E402

ARGS = parse_args(
    "Compare the private AC60 staging adapter against bounded original-code fixtures",
    "packet_staging_report.json",
)
CANDIDATE = Path(__file__).resolve().parent
PACKAGE = ARGS.original_root / "renderer_contract"
WORK = ARGS.original_root / "work" / "renderer_next" / "packet_staging"
EXPECTED_AC60_SHA256 = "f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")

WORKSPACE_TO_COLUMN = {
    "0xb0": 6, "0xb4": 7, "0xb8": 8, "0xbc": 9, "0xc0": 10, "0xc4": 11,
    "0xc8": 12, "0xcc": 13, "0xd0": 14, "0xd4": 15, "0xd8": 16, "0xdc": 17,
    "0xe0": 18, "0xe4": 19, "0xe8": 20, "0xec": 21, "0xf0": 22, "0xf4": 23,
    "0xf8": 24, "0xfc": 25, "0x100": 26, "0x104": 27,
    "0x10c_u16": 28, "0x110_u16": 29,
}
PAGE_TO_COLUMN = {"clut": 30, "page": 31, "semi": 32}
EVENT_KIND = {"ft3": 1, "ft4": 2, "gt4": 3, "e1": 7, "ignored68": 11,
              "f3": 8, "move": 9, "sprite": 4, "ot_gate": 10}
EXPECTED_COMMAND = {"ft3": 0x26, "ft4": 0x2C, "gt4": 0x3E, "e1": 0xE1,
                    "ignored68": 0x68, "f3": 0xF3, "move": 0x01,
                    "sprite": 0x64, "ot_gate": 0x40}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], cwd: Path) -> subprocess.CompletedProcess:
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True,
                            errors="replace", timeout=180)
    require(result.returncode == 0,
            f"command failed ({result.returncode}): {subprocess.list2cmdline(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result


def main() -> int:
    root = ARGS.original_root
    exe = ARGS.exe
    candidate = root / "renderer_contract" / "tests"
    work = root / "work" / "renderer_next" / "packet_staging"
    work.mkdir(parents=True, exist_ok=True)
    started = time.time()
    mixed_path = work / "native_mixed_report.json"
    gate_path = work / "native_gate_report.json"
    flags_path = work / "native_flags_report.json"
    native_common = [
        sys.executable, str(candidate / "original_ac60_mixed_runner.py"),
        "--exe", str(exe), "--original-root", str(root), "--deps", str(ARGS.deps),
        "--analysis-deps", str(ARGS.analysis_deps), "--receipt",
        str(mixed_path.relative_to(root)),
    ]
    flag_command = [
        sys.executable, str(candidate / "test_original_page_masks.py"),
        "--exe", str(exe), "--original-root", str(root), "--deps", str(ARGS.deps),
        "--analysis-deps", str(ARGS.analysis_deps), "--receipt",
        str(flags_path.relative_to(root)),
    ]
    gate_command = [
        sys.executable, str(candidate / "test_original_ac60_gate.py"),
        "--exe", str(exe), "--original-root", str(root), "--deps", str(ARGS.deps),
        "--analysis-deps", str(ARGS.analysis_deps), "--receipt",
        str(gate_path.relative_to(root)),
    ]
    for command in (native_common, flag_command, gate_command):
        run(command, root)
    native = json.loads(mixed_path.read_text(encoding="utf-8"))
    native_flags = json.loads(flags_path.read_text(encoding="utf-8"))
    native_gate = json.loads(gate_path.read_text(encoding="utf-8"))
    require(native.get("status") == "BOUNDED_EQUIVALENT" and
            native.get("oracle", {}).get("sha256") == EXPECTED_EXE_SHA256 and
            native.get("oracle", {}).get("ac60_span_sha256") == EXPECTED_AC60_SHA256,
            "portable original AC60 mixed fixture receipt identity/status failed")
    require(native_flags.get("status") == "BOUNDED_EQUIVALENT" and
            len(native_flags.get("probes", [])) == 9,
            "portable original page-mask probe receipt is incomplete")
    gate_result = native_gate.get("result", {})
    require(native_gate.get("status") == "BOUNDED_EQUIVALENT" and
            len(gate_result.get("native_gate_branch_vectors", [])) == 7 and
            not gate_result.get("gated_packets_with_data_writes"),
            "original nonzero byte-gate fixture did not cover all seven raster groups")
    require(gate_result["native_gate_load_slices"] == {
        "top_AC60_mov_al_byte": "a0fc01ba00",
        "polygon_ACCF_mov_cl_byte": "8a0dfc01ba00",
        "sprite_B057_mov_cl_byte": "8a0dfc01ba00",
    }, "original gate load width/bytes differ from the recorded byte reads")
    require(all(row["gate_byte"] == 1 and row["jne_taken_from_test_flags"] and
                row["state_unchanged_across_branch"] for row in
                gate_result["native_gate_branch_boundaries"]),
            "a native nonzero-gate branch did not preserve staging/page state")
    require(gate_result["command01_boundary"]["entry_rva"] == "0xa8f0" and
            gate_result["ot_gate_reached"][-1]["tag"] == "0x40",
            "non-raster commands failed to continue through the native gate")

    sys.path.insert(0, str(root / "scripts"))
    from pe import PE  # noqa: E402
    import match  # noqa: E402

    pe = PE(exe)
    require(pe.image_base == 0x00400000, "unexpected original image base")
    data = next(row for row in pe.sections if row["name"] == ".data")
    relevant_cells = [0x00A909B0 + offset for offset in range(0, 0x108, 4)] + [
        0x00A90A0C, 0x00A90A10, 0x00A8E6F4, 0x00A8E6F8, 0x00A8E6FC,
    ]
    initial_bss_checks = []
    for address in relevant_cells:
        rva = address - pe.image_base
        delta = rva - data["virtual_address"]
        require(0 <= delta < data["virtual_size"] and delta >= data["raw_size"],
                f"state cell {address:#x} is not inside the PE .data BSS tail")
        initial_bss_checks.append({"va": hex(address), "bss_offset": hex(delta), "zero_filled": True})

    # The repository's shared test adapter supplies the pinned VC5 flags,
    # compiler verification, temporary directories, source hashing, and report.
    common_test_report = root / "work" / "renderer_contract_tests" / "verification.json"
    package_runner = root / "renderer_contract" / "tests" / "run_tests.py"
    test_command = [sys.executable, str(package_runner), str(PACKAGE), "--timeout", "60"]
    test_process = run(test_command, root)
    common = json.loads(common_test_report.read_text(encoding="utf-8"))
    require(common.get("summary", {}).get("failed") == 0,
            f"public VC5 fixtures failed: {common.get('summary')}")
    test_row = next(row for row in common["tests"] if Path(row["test"]).name == "test_staging.c")
    c_stdout = test_row["run"]["stdout"]
    rows = {}
    flags = {}
    check_line = None
    for line in c_stdout.splitlines():
        fields = line.strip().split(";")
        if fields[0] == "ROW":
            require(len(fields) == 45, f"unexpected C row shape ({len(fields)}): {line}")
            rows[fields[1]] = [int(value, 10) for value in fields[2:]]
        elif fields[0] == "FLAG":
            require(len(fields) == 8, f"unexpected C flag shape ({len(fields)}): {line}")
            key = (fields[1], int(fields[2], 10))
            require(key not in flags, f"duplicate C flag vector {key}")
            flags[key] = [int(value, 10) for value in fields[3:]]
        elif fields[0] == "CHECKS":
            check_line = (int(fields[1]), int(fields[2]))
    require(check_line is not None and check_line[1] == 0 and check_line[0] >= 70,
            "C fixture did not report all gate, continuation, and staging checks")
    require(list(rows) == ["ft3", "ft4", "gt4", "e1", "ignored68", "f3", "move", "sprite", "ot_gate"],
            f"candidate event sequence differs: {list(rows)}")
    native_result = native["result"]
    native_order = [name for name in native_result["packet_chain_order"] if name != "null"]
    require(list(rows) == native_order, "candidate dispatch sequence differs from native chain order")
    require(set(flags) == {(family, bits) for family in ("ft4", "gt4", "e1")
                           for bits in (0x80, 0x100, 0x180)},
            "candidate page-mask vector set differs")
    require([row["packet"] for row in native_result["raster_boundaries"]] ==
            [name for name in rows if name in {"ft3", "ft4", "gt4", "sprite"}],
            "native raster-call boundary order differs")

    comparisons = []
    flag_comparisons = []
    snapshot_checks = 0
    for probe in native_flags["probes"]:
        family = probe["family"] if probe["mode"] == "polygon" else "e1"
        input_bits = int(probe["input_bits"], 16)
        key = (family, input_bits)
        selector, mode, alternate, page_address, semitransparent = flags[key]
        page = probe["native_page_cells_at_boundary"]
        native_address = int(probe["native_texture_page_address"])
        expected = probe["native_expected_from_discriminator"]
        require(selector == int(page["clut"]) == expected["page_selector"],
                f"{key}: A8E6F4 discriminator differs")
        require(mode == int(page["page"]) == expected["page_mode"],
                f"{key}: A8E6F8 discriminator differs")
        require(alternate == int(page["semi"]) == expected["alternate_bank"],
                f"{key}: A8E6FC discriminator differs")
        require(page_address == native_address == int(expected["texture_page_address"], 16),
                f"{key}: A909FC discriminator differs")
        require(semitransparent == (1 if family == "gt4" else 0),
                f"{key}: packet semitransparency got mixed with page flags")
        flag_comparisons.append({"family": family, "input_bits": hex(input_bits),
                                 "page_selector": selector, "page_mode": mode,
                                 "alternate_bank": alternate, "texture_page_address": hex(page_address),
                                 "semitransparent": semitransparent, "equal": True})
        snapshot_checks += 4

    for boundary in native_result["raster_boundaries"]:
        name = boundary["packet"]
        row = rows[name]
        require(row[0] == 0 and row[1] == EVENT_KIND[name] and
                row[3] == EXPECTED_COMMAND[name] and
                row[4] == (EXPECTED_COMMAND[name] & 2) // 2,
                f"{name}: candidate event descriptor differs")
        require(row[5] == native_order.index(name) + 1, f"{name}: sequence differs")
        for key, column in WORKSPACE_TO_COLUMN.items():
            wanted = int(boundary["workspace"][key])
            require(row[column] == wanted, f"{name}: workspace {key} differs")
            snapshot_checks += 1
        for key, column in PAGE_TO_COLUMN.items():
            require(row[column] == int(boundary["texture_state"][key]),
                    f"{name}: page cell {key} differs")
            snapshot_checks += 1
        comparisons.append({"boundary": name, "native_rva": boundary["raster_entry_rva"],
                            "fields_equal": len(WORKSPACE_TO_COLUMN) + len(PAGE_TO_COLUMN)})

    ignored_row = rows["ignored68"]
    ignored = native_result["ignored_raster_boundaries"][0]
    for key, column in WORKSPACE_TO_COLUMN.items():
        require(ignored_row[column] == int(ignored["workspace"][key]),
                f"0x68 workspace {key} differs")
        snapshot_checks += 1
    for key, column in PAGE_TO_COLUMN.items():
        require(ignored_row[column] == int(ignored["texture_state"][key]),
                f"0x68 page cell {key} differs")
        snapshot_checks += 1

    toolchain_id = match.verify_toolchain(TOOLCHAIN)
    compiler = TOOLCHAIN / "bin" / "cl.exe"
    changed_sources = []
    for test in common["tests"]:
        for source, digest in test["source_hashes_after"].items():
            if sha256_file(Path(source)) != digest:
                changed_sources.append(source)
    require(not changed_sources, f"source changed after common fixture run: {changed_sources}")
    report = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": (
            "Private C89 AC60 staging adapter compared at the original FT3/FT4/GT4/sprite "
            "raster call entries, the native 0x68 page-refresh branch, E1/F3/command-01 state "
            "observations, nine original page-mask probes, and seven nonzero-gate branches. "
            "Raster and A8F0 callees are skipped; no pixels or gameplay are claimed."
        ),
        "prediction_and_falsifier": str((root / "work" / "renderer_next" / "packet_staging" /
                                          "prediction.json").relative_to(root)),
        "oracle": {
            "exe_sha256": sha256_file(exe), "ac60_span_sha256": native["oracle"]["ac60_span_sha256"],
            "native_mixed_report": str(mixed_path.relative_to(root)),
            "native_mixed_report_sha256": sha256_file(mixed_path),
            "native_gate_report": str(gate_path.relative_to(root)),
            "native_gate_report_sha256": sha256_file(gate_path),
            "native_flags_report": str(flags_path.relative_to(root)),
            "native_flags_report_sha256": sha256_file(flags_path),
            "native_fixture_driver": str((candidate / "original_ac60_mixed_runner.py").relative_to(root)),
            "native_gate_driver": str((candidate / "test_original_ac60_gate.py").relative_to(root)),
            "native_flag_driver": str((candidate / "test_original_page_masks.py").relative_to(root)),
        },
        "initial_state": {
            "source": "The captured A909/A8E6 cells lie in the mapped .data BSS tail and began zero-filled.",
            "pe_data_virtual_size": hex(data["virtual_size"]),
            "pe_data_raw_size": hex(data["raw_size"]), "checked_cells": initial_bss_checks,
        },
        "candidate": {
            "files": [{"path": str(Path(row["path"]).relative_to(root)), "sha256": row["sha256"]}
                      for row in common["package_sources"] + common["package_headers"] +
                      common["tests_discovered"]],
            "adapter_runner": str(Path(__file__).resolve().relative_to(root)),
            "toolchain": str(TOOLCHAIN), "toolchain_id": toolchain_id,
            "compiler_sha256": sha256_file(compiler), "flags": common["fixed_flags"],
            "common_test_runner": str(package_runner.relative_to(root)),
            "test_command": test_command, "common_test_summary": common["summary"],
            "fixture_exe_sha256": test_row["exe_sha256"],
            "checks": {"count": check_line[0], "failures": check_line[1]},
            "common_runner_stdout": test_process.stdout.strip(),
        },
        "comparison": {
            "native_packet_chain_order": native_order,
            "candidate_packet_chain_order": list(rows),
            "raster_boundaries": comparisons,
            "ignored_raster_boundary": {
                "packet": "0x68", "original_rva": ignored["rva"],
                "fields_equal": len(WORKSPACE_TO_COLUMN) + len(PAGE_TO_COLUMN),
                "page_base": hex(int(ignored["workspace"]["0xfc"])),
                "native_no_raster_call": True,
            },
            "alternate_bank_discriminator_vectors": flag_comparisons,
            "native_gate": {
                "byte_load_instruction_slices": gate_result["native_gate_load_slices"],
                "native_gate_branches": gate_result["native_gate_branch_vectors"],
                "gated_data_writes": gate_result["gated_packets_with_data_writes"],
                "state_preserved_across_each_branch": True,
                "e1_f3_command01_ot_continue": True,
            },
            "native_workspace_and_page_field_comparisons": snapshot_checks,
            "native_gate_branch_comparisons": len(gate_result["native_gate_branch_vectors"]),
            "common_c_checks": check_line,
        },
        "semantics": {
            "gate": "BA01FC is read as a byte. Native AC60 suppresses all polygon and group-0x60 staging/page-refresh/raster paths when nonzero; E1, F3, command 01, and OT traversal continue.",
            "alternate_bank": "FT4, GT4, and E1 all observed alternate-bank values 0,0,1 for inputs 0x80, 0x100, 0x180; packet blend bit remains separate.",
            "command01": "The adapter reports a deferred descriptor; this fixture does not copy bytes.",
            "scope": "No raster pixels, full device execution, or gameplay behavior are covered.",
        },
        "limitations": [
            "Finite synthetic mixed chains and discriminator vectors only.",
            "Actual raster and A8F0 callees are stopped at their original call boundaries; no pixel or copied-byte result is asserted.",
            "No canonical source integration, FUNCTION_MATCH, source identity, or whole-module equality is claimed.",
        ],
        "started_unix": started, "finished_unix": time.time(),
    }
    write_receipt(ARGS.receipt, report)
    print(json.dumps({"status": report["status"], "checks": check_line,
                      "native_field_comparisons": snapshot_checks,
                      "native_gate_branches": len(gate_result["native_gate_branch_vectors"]),
                      "page_mask_vectors": len(flag_comparisons), "receipt": str(ARGS.receipt)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
