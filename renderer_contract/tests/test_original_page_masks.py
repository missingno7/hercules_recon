#!/usr/bin/env python3
"""Probe native alternate-page masks through original AC60 mixed-chain calls."""
from __future__ import annotations

import copy
import hashlib
import json
import struct

import original_ac60_mixed_runner as native

ARGS = native.ARGS
VALUES = (0x80, 0x100, 0x180)
TEXTURE_BASE = native.TEXTURE_BASE
STATE_CLUT = native.STATE_CLUT
STATE_PAGE = native.STATE_PAGE
STATE_SEMI = native.STATE_SEMI


def put_u32(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", data, offset, value & 0xFFFFFFFF)


def page_values(value: int):
    selector = value & 0x1F
    mode = (value >> 5) & 3
    alternate = 1 if (value & 0x180) == 0x180 else 0
    address = 0x00A6E6E0 if alternate else TEXTURE_BASE + (selector >> 1) * 0x10000
    return selector, mode, alternate, address


def main() -> int:
    base_fixture = native.fixture_packets
    base_expected = native.expected_for_packet
    pe = native.PE(ARGS.exe)
    probes = []

    for mode, family, value in [
        ("polygon", family, value)
        for family in ("ft4", "gt4") for value in VALUES
    ] + [("e1", None, value) for value in VALUES]:
        def fixture_packets(mode=mode, family=family, value=value):
            addresses, records = base_fixture()
            if mode == "polygon":
                record = bytearray(records[family])
                packet_offset = 0x16 if family == "ft4" else 0x1A
                struct.pack_into("<H", record, packet_offset, value)
                records[family] = bytes(record)
            else:
                record = bytearray(records["e1"])
                put_u32(record, 4, 0xE1000000 | value)
                records["e1"] = bytes(record)
            return addresses, records

        def expected_for_packet(name, mode=mode, family=family, value=value):
            result = copy.deepcopy(base_expected(name))
            selector, page_mode, alternate, address = page_values(value)
            target_family = family if mode == "polygon" else "sprite"
            if name == target_family:
                result["fields"][0xFC] = address
                result["globals"][STATE_CLUT] = selector
                result["globals"][STATE_PAGE] = page_mode
                result["globals"][STATE_SEMI] = alternate
            return result

        native.fixture_packets = fixture_packets
        native.expected_for_packet = expected_for_packet
        try:
            result = native.run_fixture(pe)
        finally:
            native.fixture_packets = base_fixture
            native.expected_for_packet = base_expected
        target = family if mode == "polygon" else "sprite"
        boundary = next(row for row in result["raster_boundaries"]
                        if row["packet"] == target)
        page = boundary["texture_state"]
        selector, page_mode, alternate, address = page_values(value)
        probes.append({
            "mode": mode,
            "family": family,
            "input_bits": hex(value),
            "target_boundary": target,
            "native_page_cells_at_boundary": page,
            "native_texture_page_address": boundary["workspace"]["0xfc"],
            "native_expected_from_discriminator": {
                "page_selector": selector, "page_mode": page_mode,
                "alternate_bank": alternate, "texture_page_address": hex(address),
            },
            "native_branch_stores": [
                row for row in result["all_original_data_write_events"]
                if row["address"] == "0xa8e6fc" and row["ac60_current_packet_va"] ==
                (result["packet_addresses"]["e1"] if mode == "e1" else
                 result["packet_addresses"][family])
            ],
            "original_code_bytes_unchanged": result["original_code_bytes_unchanged"],
            "unknown_or_os_endpoint_executed": result["unknown_or_os_endpoint_executed"],
        })
        if (page["clut"] != selector or page["page"] != page_mode or
                page["semi"] != alternate or
                int(boundary["workspace"]["0xfc"]) != address):
            raise AssertionError(f"native AC60 discriminator vector differs: {probes[-1]}")

    summary = {}
    for row in probes:
        name = row["family"] if row["mode"] == "polygon" else "e1"
        summary.setdefault(name, {})[row["input_bits"]] = row["native_page_cells_at_boundary"]["semi"]
    receipt = {
        "schema_version": 1,
        "status": "BOUNDED_EQUIVALENT",
        "scope": (
            "Nine original AC60 discriminator vectors at FT4, GT4, and sprite call "
            "boundaries for page values 0x80, 0x100, and 0x180; raster callees are skipped."
        ),
        "oracle": {
            "path": str(ARGS.exe), "sha256": hashlib.sha256(ARGS.exe.read_bytes()).hexdigest(),
            "ac60_span_sha256": hashlib.sha256(
                pe.read_rva(native.AC60_RVA, native.AC60_END_RVA-native.AC60_RVA)).hexdigest(),
        },
        "execution": {
            **native.verify_runtime(native.unicorn, ARGS.deps),
            "architecture": "x86 32-bit",
        },
        "alternate_bank_by_family": summary,
        "probes": probes,
        "limitations": [
            "These are finite page-mask discriminator inputs; they do not establish a broader device or renderer model.",
            "Original raster callees were skipped at their real call entries; no pixels are compared.",
        ],
    }
    native.write_receipt(ARGS.receipt, receipt)
    print(json.dumps({"status": receipt["status"], "probes": len(probes),
                      "alternate_bank_by_family": summary, "receipt": str(ARGS.receipt)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
