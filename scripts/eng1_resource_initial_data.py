"""Fresh pinned-compiler audit of the default 81-row ENG1 resource table.

This reuses the repository's strict resource-data byte/content checker while
directing its one COFF build into the ignored work directory. It adds
explicit per-row native scalar and actual path-content identities.
"""

from __future__ import annotations

import hashlib
import uuid
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

import resource_data_evidence as base  # noqa: E402
from coff import COFF  # noqa: E402
from match import DEFAULT_TOOLCHAIN, verify_toolchain  # noqa: E402
from pe import PE  # noqa: E402
from title_behavioral import context_files  # noqa: E402


LANE = ROOT / "work/eng1_behavioral/resource_initial_data"
BUILD = LANE / ("fresh_coff_" + uuid.uuid4().hex[:12])
REPORT = LANE / "report.json"
TABLE_RVA = 0x5CEE0
ROW_COUNT = 81
ROW_SIZE = 20
HIGHLOW = 3
COFF_DIR32 = 6


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fresh_compile(source: Path, flags: list[str], toolchain: Path = DEFAULT_TOOLCHAIN):
    verify_toolchain(toolchain)
    BUILD.mkdir(parents=True, exist_ok=False)
    obj = BUILD / "resource_data.obj"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    command = [str(toolchain / "bin/cl.exe"), "/nologo", "/c", "/Gy", *flags,
               "/Fo" + str(obj), str(source.resolve())]
    run = subprocess.run(command, cwd=BUILD, env=env, capture_output=True, text=True)
    (BUILD / "compile.log").write_bytes((run.stdout + run.stderr).encode("utf-8"))
    if run.returncode != 0 or not obj.is_file():
        raise RuntimeError(f"Fresh pinned MSVC compile failed ({run.returncode}); see {BUILD / 'compile.log'}")
    return obj, command


def symbol(coff: COFF, name: str) -> dict:
    rows = [row for row in coff.symbols.values() if row["name"] == name]
    if len(rows) != 1:
        raise ValueError(f"Expected one COFF symbol {name}, found {len(rows)}")
    return rows[0]


def read_native_byte(image: PE, rva: int) -> bytes:
    for section in image.sections:
        start = section["virtual_address"]
        if start <= rva < start + section["virtual_size"]:
            delta = rva - start
            if delta >= section["raw_size"]:
                return b"\0"
            return image.read_rva(rva, 1)
    raise ValueError(f"Native pointer target is outside mapped sections: {rva:#x}")


def native_string(image: PE, rva: int) -> bytes:
    value = bytearray()
    for offset in range(1024):
        byte = read_native_byte(image, rva + offset)
        value += byte
        if byte == b"\0":
            return bytes(value)
    raise ValueError(f"Unterminated native string at {rva:#x}")


def coff_bytes(coff: COFF, section: dict, offset: int, count: int) -> bytes:
    if offset < 0 or offset + count > section["size"]:
        raise ValueError("Candidate string points outside its COFF section")
    data = section["data"][offset:offset + count]
    if len(data) < count:
        if not section["flags"] & 0x80:
            raise ValueError("Candidate string bytes are missing")
        data += bytes(count - len(data))
    return data


def native_relocation_map(image: PE) -> dict[int, int]:
    result: dict[int, int] = {}
    for block in getattr(image.pe, "DIRECTORY_ENTRY_BASERELOC", []):
        for item in block.entries:
            kind = int(item.type)
            if kind == 0:
                continue
            if item.rva in result:
                raise ValueError(f"Duplicate native base relocation at {item.rva:#x}")
            result[int(item.rva)] = kind
    return result


def run() -> dict:
    # The base checker performs its own source/oracle pin checks and exact
    # scalar-byte, local-string-content, map, and zero-view checks.
    tracked = set(context_files(ROOT / "calibration/resource_data.c")) | {Path(__file__).resolve(), ROOT / "scripts/resource_data_evidence.py", ROOT / "evidence/resource_writer_layout.json", ROOT / "toolchains/manifest.json"}
    before = {p.relative_to(ROOT).as_posix(): sha(p.read_bytes()) for p in tracked}
    base.compile_source = fresh_compile
    result = base.audit()
    obj = BUILD / "resource_data.obj"
    evidence = json.loads((ROOT / "evidence/resource_writer_layout.json").read_text(encoding="utf-8"))
    target = ROOT / evidence["target"]
    image = PE(target)
    coff = COFF(obj)
    records = symbol(coff, "_g_actor_resources")
    if records["storage"] != 2 or records["section"] <= 0:
        raise ValueError("The table is not one initialized public COFF data definition")
    section = coff.sections[records["section"] - 1]
    start = records["value"]
    count = ROW_COUNT * ROW_SIZE
    compiled = section["data"][start:start + count]
    if len(compiled) != count:
        raise ValueError(f"Compiled row contribution has {len(compiled)} bytes, expected {count}")
    native = image.read_rva(TABLE_RVA, count)

    # Source-level static assertions plus the complete 1,620-byte COFF symbol
    # slice establish the proposed field offsets/widths for this C definition.
    source_text = (ROOT / "calibration/resource_data.c").read_text(encoding="utf-8")
    required_assertions = [
        "sizeof(ActorReference) == 4",
        "sizeof(ActorResource) == 20",
        "offsetof(ActorResource, path) == 0",
        "offsetof(ActorResource, entry_count) == 4",
        "offsetof(ActorResource, unknown_006) == 6",
        "offsetof(ActorResource, file_length) == 8",
        "offsetof(ActorResource, references) == 12",
        "offsetof(ActorResource, residency_state) == 16",
        "offsetof(ActorResource, unknown_012) == 18",
    ]
    absent = [item for item in required_assertions if item not in source_text]
    if absent:
        raise ValueError(f"Source layout guard(s) missing: {absent}")

    native_relocs = native_relocation_map(image)
    candidate_relocs = [r for r in section["relocations"]
                        if start <= r["offset"] < start + count]
    expected_cells = {start + index * ROW_SIZE for index in range(ROW_COUNT)}
    actual_cells = {r["offset"] for r in candidate_relocs}
    if len(candidate_relocs) != ROW_COUNT or actual_cells != expected_cells:
        raise ValueError("COFF path relocation cells do not exactly cover all 81 rows")
    native_path_cells = {TABLE_RVA + index * ROW_SIZE for index in range(ROW_COUNT)}
    native_found = {rva: native_relocs.get(rva) for rva in native_path_cells}
    if set(native_found.values()) != {HIGHLOW}:
        raise ValueError("Native path cell relocation types are not all IMAGE_REL_BASED_HIGHLOW")
    table_relocs = {rva: typ for rva, typ in native_relocs.items()
                    if TABLE_RVA <= rva < TABLE_RVA + count}
    if table_relocs != {rva: HIGHLOW for rva in native_path_cells}:
        raise ValueError("Native table has relocations beyond its 81 path fields")

    literal_symbols = {row["name"]: row for row in coff.symbols.values()}
    rows = []
    scalar_specs = [
        ("entry_count", 4, 2, "<H"),
        ("unknown_006", 6, 2, "<H"),
        ("file_length", 8, 4, "<I"),
        ("references_pointer_initial", 12, 4, "<I"),
        ("residency_state", 16, 2, "<H"),
        ("unknown_012", 18, 2, "<H"),
    ]
    for index in range(ROW_COUNT):
        row_start = index * ROW_SIZE
        native_path_va = struct.unpack_from("<I", native, row_start)[0]
        native_path_rva = native_path_va - image.image_base
        native_content = native_string(image, native_path_rva)
        relocation = next(r for r in candidate_relocs if r["offset"] == start + row_start)
        if relocation["type"] != COFF_DIR32:
            raise ValueError(f"Unexpected COFF path relocation at row {index}: {relocation['type']}")
        referent = literal_symbols.get(relocation["symbol"])
        if referent is None or referent["section"] <= 0:
            raise ValueError(f"Row {index} path relocation has no local string definition")
        string_section = coff.sections[referent["section"] - 1]
        addend = struct.unpack_from("<I", compiled, row_start)[0]
        local_content = coff_bytes(coff, string_section, referent["value"] + addend, len(native_content))
        if local_content != native_content:
            raise ValueError(f"Native/local path content mismatch at row {index}, RVA {TABLE_RVA + row_start:#x}")
        if compiled[row_start + 4:row_start + ROW_SIZE] != native[row_start + 4:row_start + ROW_SIZE]:
            raise ValueError(f"Scalar initializer mismatch at row {index}, RVA {TABLE_RVA + row_start + 4:#x}")
        scalar_values = {}
        for field, offset, width, fmt in scalar_specs:
            raw = native[row_start + offset:row_start + offset + width]
            scalar_values[field] = {
                "rva": hex(TABLE_RVA + row_start + offset),
                "value": struct.unpack(fmt, raw)[0],
                "bytes_hex": raw.hex(),
            }
        rows.append({
            "index": index,
            "row_rva": hex(TABLE_RVA + row_start),
            "native_path_target_rva": hex(native_path_rva),
            "path_content_ascii": native_content[:-1].decode("ascii"),
            "path_content_bytes_including_nul": len(native_content),
            "path_content_sha256": sha(native_content),
            "candidate_literal_symbol": referent["name"],
            "candidate_literal_addend": hex(addend),
            "path_content_equal": True,
            "scalar_fields": scalar_values,
        })

    if any(row["path_content_ascii"] != "" for row in rows[:1]):
        raise ValueError("Row zero no longer resolves to the empty path literal")
    if sum(row["scalar_fields"]["entry_count"]["value"] != 0 for row in rows) != 80:
        raise ValueError("Unexpected default entry-count residency pattern")
    zero_fields = ["unknown_006", "file_length", "references_pointer_initial",
                   "residency_state", "unknown_012"]
    nonzero = {name: [row["index"] for row in rows if row["scalar_fields"][name]["value"] != 0]
               for name in zero_fields}
    if any(nonzero.values()):
        raise ValueError(f"Expected zero initial fields contain nonzero rows: {nonzero}")

    result["scope"] = ("Fresh pinned MSVC 5 data-only comparison of the default 81-row table; "
                       "no FUNCTION_MATCH, source TU identity, placement, or whole-module claim.")
    result["table"] = {
        "source": "calibration/resource_data.c",
        "source_sha256": sha((ROOT / "calibration/resource_data.c").read_bytes()),
        "native_rva": hex(TABLE_RVA),
        "native_end_rva_exclusive": hex(TABLE_RVA + count),
        "rows": ROW_COUNT,
        "stride_bytes": ROW_SIZE,
        "compiled_data_symbol": "_g_actor_resources",
        "compiled_coff_section": section["name"],
        "compiled_contribution_bytes": len(compiled),
        "native_table_sha256": sha(native),
        "non_pointer_field_bytes_equal": True,
        "non_pointer_field_bytes": ROW_COUNT * (ROW_SIZE - 4),
        "native_path_relocations": {"count": len(native_path_cells), "type": "IMAGE_REL_BASED_HIGHLOW"},
        "candidate_path_relocations": {"count": len(candidate_relocs), "type": "IMAGE_REL_I386_DIR32"},
        "source_layout_assertions_present": required_assertions,
        "initialization_summary": {
            "entry_count_nonzero_rows": [row["index"] for row in rows
                                         if row["scalar_fields"]["entry_count"]["value"] != 0],
            "other_scalar_fields_nonzero_rows": nonzero,
            "row0_state_rva": hex(TABLE_RVA + 16),
            "row0_state_meaning": "Physical +0x10 field of row 0 within this same 81-row table.",
        },
        "path_content_identity_method": (
            "Resolve each actual native table VA to its mapped NUL-terminated bytes; resolve each COFF "
            "path relocation to its actual local literal bytes. Compare bytes and SHA-256. No native RVA "
            "is assigned to a candidate literal or used as its identity."
        ),
        "rows_detail": rows,
    }
    result["fresh_build"] = {
        "object": obj.relative_to(ROOT).as_posix(),
        "object_sha256": sha(obj.read_bytes()),
        "object_size": obj.stat().st_size,
        "command": result["command"],
        "toolchain_id": "msvc5_rtm",
        "toolchain_verification": "scripts/match.py::verify_toolchain checked pinned toolchain/manifest.json file hashes before launch",
        "compile_log": (BUILD / "compile.log").relative_to(ROOT).as_posix(),
    }
    after = {p: sha((ROOT / p).read_bytes()) for p in before}
    if before != after:
        raise ValueError("Resource data inputs changed during the fresh audit")
    verify_toolchain(DEFAULT_TOOLCHAIN)
    result.update(input_sha256_before=before, input_sha256_after=after, inputs_unchanged=True, result="PASS")
    REPORT.write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
    return result


if __name__ == "__main__":
    report = run()
    print(json.dumps({
        "rows": report["table"]["rows"],
        "non_pointer_field_bytes_equal": report["table"]["non_pointer_field_bytes_equal"],
        "native_path_relocations": report["table"]["native_path_relocations"],
        "candidate_path_relocations": report["table"]["candidate_path_relocations"],
        "entry_count_nonzero_rows": len(report["table"]["initialization_summary"]["entry_count_nonzero_rows"]),
        "other_scalar_fields_nonzero_rows": report["table"]["initialization_summary"]["other_scalar_fields_nonzero_rows"],
        "object_sha256": report["fresh_build"]["object_sha256"],
        "report": REPORT.relative_to(ROOT).as_posix(),
    }, indent=2))
