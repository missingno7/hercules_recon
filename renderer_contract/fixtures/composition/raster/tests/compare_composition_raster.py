"""Compare a bounded C FT4 source model to one original-x86 frame receipt."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


KIND_TO_ID = {
    "frame_entry": 1,
    "surface_lock": 8,
    "packet_consumed": 13,
    "geometry_command": 2,
    "cpu_pixel_write": 3,
    "remap_request": 14,
    "surface_unlock": 15,
    "present_request": 17,
    "final_output": 20,
    "output_flush": 21,
}
EXPECTED_DOMAIN = {1: 1, 8: 6, 13: 1, 2: 2, 3: 6, 14: 6, 15: 6, 17: 8, 20: 8, 21: 8}
EXPECTED_ORIGIN = {1: 3, 8: 3, 13: 3, 2: 3, 3: 3, 14: 3, 15: 3, 17: 3, 20: 2, 21: 2}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_events(path: Path) -> list[dict]:
    lines = path.read_text(encoding="ascii").splitlines()
    if not lines or lines[0] != "status 0":
        raise ValueError("C event stream did not report status 0")
    events = []
    for expected_order, line in enumerate(lines[1:]):
        fields = [int(item, 10) for item in line.split()]
        if len(fields) != 11:
            raise ValueError(f"unexpected C event row width at {expected_order}: {line!r}")
        order, kind, source_rva, token, a0, a1, a2, a3, origin, domain, owner = fields
        if order != expected_order or owner != 0:
            raise ValueError(f"nonmonotonic event or non-unclassified owner at {expected_order}")
        if origin != EXPECTED_ORIGIN.get(kind) or domain != EXPECTED_DOMAIN.get(kind):
            raise ValueError(f"unexpected event origin/domain at {expected_order}: {fields!r}")
        events.append({"kind": kind, "source_rva": source_rva, "raw_token": token,
                       "arguments": [a0, a1, a2, a3]})
    return events


def parse_pixels(path: Path) -> list[dict]:
    result = []
    for line in path.read_text(encoding="ascii").splitlines():
        fields = [int(item, 10) for item in line.split()]
        if len(fields) != 3:
            raise ValueError(f"unexpected pixel row: {line!r}")
        result.append({"offset": fields[0], "before": fields[1], "after": fields[2]})
    return result


def native_signature(native: dict) -> list[dict]:
    projection = native["execution"]["native_event_projection"]
    result = []
    prior_order = 0
    for index, event in enumerate(projection):
        order = event["instruction_order"]
        if order <= prior_order:
            raise ValueError(f"native instruction event order is not increasing at {index}")
        prior_order = order
        result.append({
            "kind": KIND_TO_ID[event["kind"]],
            "source_rva": int(event["source_rva"], 16),
            "raw_token": int(event["raw_token"], 16),
            "arguments": event["arguments"],
        })
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", required=True, type=Path)
    parser.add_argument("--events", required=True, type=Path)
    parser.add_argument("--pixels", required=True, type=Path)
    parser.add_argument("--raw-inputs", required=True, type=Path)
    parser.add_argument("--receipt", required=True, type=Path)
    args = parser.parse_args()
    native_bytes = args.native.read_bytes()
    event_bytes = args.events.read_bytes()
    pixel_bytes = args.pixels.read_bytes()
    raw_input_bytes = args.raw_inputs.read_bytes()
    native = json.loads(native_bytes.decode("utf-8"))
    original_events = native_signature(native)
    reconstructed_events = parse_events(args.events)
    original_pixels = native["execution"]["surface_changed"]
    reconstructed_pixels = parse_pixels(args.pixels)
    expected_raw_inputs = bytes.fromhex(native["fixture"]["raw_input_bytes_hex"])
    event_match = original_events == reconstructed_events
    pixel_match = original_pixels == reconstructed_pixels
    input_match = expected_raw_inputs == raw_input_bytes
    if not event_match or not pixel_match or not input_match:
        event_diffs = []
        for index in range(max(len(original_events), len(reconstructed_events))):
            left = original_events[index] if index < len(original_events) else None
            right = reconstructed_events[index] if index < len(reconstructed_events) else None
            if left != right:
                event_diffs.append({"index": index, "native": left, "c": right})
                if len(event_diffs) >= 4:
                    break
        expected_hex = native["fixture"]["raw_input_bytes_hex"]
        actual_hex = raw_input_bytes.hex()
        raw_diffs = [{"offset": i, "native": expected_hex[i:i + 2], "c": actual_hex[i:i + 2]}
                     for i in range(0, min(len(expected_hex), len(actual_hex)), 2)
                     if expected_hex[i:i + 2] != actual_hex[i:i + 2]][:16]
        raise SystemExit(json.dumps({"status": "MISMATCH", "event_match": event_match,
                                     "pixel_match": pixel_match, "input_match": input_match,
                                     "native_event_count": len(original_events),
                                     "c_event_count": len(reconstructed_events),
                                     "event_diffs": event_diffs,
                                     "native_raw_input_length": len(expected_raw_inputs),
                                     "c_raw_input_length": len(raw_input_bytes),
                                     "raw_diffs": raw_diffs,
                                     "native_pixels": original_pixels,
                                     "c_pixels": reconstructed_pixels}, indent=2))
    receipt = {
        "schema_version": 1,
        "status": "CONNECTED_BOUNDED_RASTER_SIGNATURE_PASS",
        "claim": "The bounded source-built C model matches one unchanged-original x86 FT4 packet-to-CPU-byte-write and windowed-present event signature, using the same raw FT4 and terminator bytes and synthetic texture/surface/service buffers.",
        "comparison": {
            "native_exe_sha256": native["oracle"]["sha256"],
            "native_receipt_sha256": sha(native_bytes),
            "native_event_count": len(original_events),
            "c_event_count": len(reconstructed_events),
            "event_signature_match": True,
            "native_pixel_changes": original_pixels,
            "c_pixel_changes": reconstructed_pixels,
            "pixel_result_match": True,
            "raw_order_table_and_packet_bytes_match": True,
            "raw_input_sha256": sha(raw_input_bytes),
            "native_event_source_order_is_instruction_count": True,
            "c_event_source_order_is_per_operation": True,
            "native_and_c_origin_fields_are_not_compared": True,
        },
        "inputs": {
            "native_receipt": {"path": str(args.native), "sha256": sha(native_bytes)},
            "c_event_stream": {"path": str(args.events), "sha256": sha(event_bytes)},
            "c_pixel_diff": {"path": str(args.pixels), "sha256": sha(pixel_bytes)},
            "c_raw_inputs": {"path": str(args.raw_inputs), "sha256": sha(raw_input_bytes)},
        },
        "limitations": [
            "The C model supports only unblended FT4 command 0x2c, tpage/clut zero, the D420 axis-aligned rectangle plus 1:1 UV predicate, fully in-bounds rectangles up to 8x8, and the fixed observed texture/surface pitches.",
            "Base and shifted/resized UV-offset cases are separate original-x86 invocations; their matching signatures do not establish gameplay frame chronology.",
            "An asymmetric-UV native discriminator takes D420's generic D60C path and performs no CPU pixel write for that input; the C model rejects that packet before emitting frame events or mutating the surface. The generic route is not reconstructed.",
            "The original remap call requests 0xff, but guard 0x80 takes the equality skip branch; this fixture performs no remap byte writes.",
            "The C reconstruction has controlled providers and does not call Windows GDI, launch the game, or claim full-frame, gameplay, palette, or writer-closure equivalence.",
            "The packet primitive is not assigned a world/UI layer or semantic owner.",
        ],
    }
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "events": len(original_events),
                      "pixels": len(original_pixels), "receipt": str(args.receipt),
                      "receipt_sha256": sha(args.receipt.read_bytes())}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
