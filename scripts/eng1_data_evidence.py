"""Fresh MSVC 5 ENG1 data-provider/oracle diagnostic; not function acceptance."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from typing import Any


def find_root(start: Path) -> Path:
    for path in (start, *start.parents):
        if (path / "recovery.json").is_file() and (path / "scripts" / "coff.py").is_file():
            return path
    raise RuntimeError("could not locate repository root")


ROOT = find_root(Path(__file__).resolve())
sys.path.insert(0, str(ROOT / "scripts"))
from coff import COFF  # noqa: E402
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain  # noqa: E402
from pe import PE  # noqa: E402


DEFAULT_MANIFEST = ROOT / "evidence" / "eng1_data_evidence_manifest.json"
DEFAULT_OUTPUT = ROOT / "work" / "eng1_behavioral" / "data_verification" / "report.json"
WRITABLE = ROOT / "work"
TARGET_HIGHLOW = 3
I386_DIR32 = 6


class CheckError(RuntimeError):
    pass


def sha_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def repo_relative(path: Path) -> str:
    return path.resolve().relative_to(ROOT.resolve()).as_posix()


def scan_quoted_includes(sources: list[Path]) -> list[Path]:
    """Return the transitive repository headers read by the compiler.

    System/toolchain headers are covered by the pinned toolchain identity. Every
    quoted include inside the repository is read and tracked, including .inc
    fragments that are part of another provider's translation unit.
    """
    toolchain_include = (DEFAULT_TOOLCHAIN / "include").resolve()
    pending = [path.resolve() for path in sources]
    visited: set[Path] = set()
    repository_headers: set[Path] = set()
    include_line = re.compile(r"^\s*#\s*include\s*(.*?)(?:\s*(?://.*)?)$")
    quoted = re.compile(r'^"([^"\r\n]+)"')
    angled = re.compile(r"^<([^>\r\n]+)>")

    while pending:
        current = pending.pop()
        if current in visited:
            continue
        visited.add(current)
        try:
            current.relative_to(ROOT.resolve())
        except ValueError as exc:
            raise CheckError(f"provider include escapes repository: {current}") from exc
        for line_number, line in enumerate(current.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            directive = include_line.match(line)
            if not directive:
                if re.match(r"^\s*#\s*include\b", line):
                    raise CheckError(f"unsupported macro-form include at {repo_relative(current)}:{line_number}")
                continue
            argument = directive.group(1).strip()
            match = quoted.match(argument)
            if not match:
                if angled.match(argument):
                    continue
                raise CheckError(f"unsupported include syntax at {repo_relative(current)}:{line_number}: {argument}")
            name = match.group(1)
            candidates = [current.parent / name, ROOT / name, toolchain_include / name]
            found = next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)
            if found is None:
                raise CheckError(f"cannot resolve quoted include {name!r} from {repo_relative(current)}:{line_number}")
            try:
                found.relative_to(ROOT.resolve())
            except ValueError:
                try:
                    found.relative_to(toolchain_include)
                except ValueError as exc:
                    raise CheckError(f"quoted include resolves outside repository/toolchain: {found}") from exc
                continue
            if found not in visited:
                repository_headers.add(found)
                pending.append(found)
    return sorted(repository_headers, key=lambda path: path.as_posix().casefold())


def load_json_bytes(path: Path) -> tuple[bytes, Any]:
    data = path.read_bytes()
    return data, json.loads(data)


def parse_rva(value: str | int) -> int:
    return int(value, 0) if isinstance(value, str) else int(value)


def in_work(path: Path) -> bool:
    try:
        path.resolve().relative_to(WRITABLE.resolve())
        return True
    except ValueError:
        return False


def parse_native_relocations(pe: PE) -> dict[int, int]:
    found: dict[int, int] = {}
    for block in getattr(pe.pe, "DIRECTORY_ENTRY_BASERELOC", []):
        for entry in block.entries:
            kind = int(entry.type)
            if kind == 0:  # IMAGE_REL_BASED_ABSOLUTE page padding
                continue
            rva = int(entry.rva)
            previous = found.get(rva)
            if previous is not None:
                raise CheckError(f"duplicate native base relocation at {rva:#x}")
            found[rva] = kind
    return found


def native_section(pe: PE, rva: int, size: int, read_only_table: bool = False) -> dict[str, Any]:
    if size <= 0:
        raise CheckError("manifest field size must be positive")
    matches = [s for s in pe.sections
               if s["virtual_address"] <= rva and
               rva + size <= s["virtual_address"] + s["virtual_size"]]
    if len(matches) != 1:
        raise CheckError(f"native interval {rva:#x}+{size:#x} is not within one PE section")
    section = matches[0]
    if read_only_table:
        if section["characteristics"] & 0x80000000 or not section["characteristics"] & 0x40000000:
            raise CheckError(f"native read-only table {rva:#x}+{size:#x} has incompatible section permissions")
    elif not section["characteristics"] & 0x80000000:
        raise CheckError(f"native storage interval {rva:#x}+{size:#x} is not writable")
    return section


def native_bytes(pe: PE, field: dict[str, Any]) -> tuple[bytes, str, dict[str, Any]]:
    rva = parse_rva(field["rva"])
    size = int(field["size"])
    section = native_section(pe, rva, size, field.get("storage") == "read_only_table")
    delta = rva - section["virtual_address"]
    file_backed = delta + size <= section["raw_size"]
    loader_zero = delta >= section["raw_size"]
    if file_backed:
        return pe.read_rva(rva, size), "initialized", section
    if loader_zero:
        return bytes(size), "loader_zero", section
    if field.get("allow_compiled_zero_fill") is True:
        prefix = pe.read_rva(rva, section["raw_size"] - delta)
        if not any(prefix):
            return bytes(size), "mixed_zero", section
    raise CheckError(f"native interval {rva:#x}+{size:#x} crosses file-backed/zero-fill boundary")


def recover_function_symbols(state: dict[str, Any], oracle_path: str, oracle_sha256: str) -> dict[str, dict[str, Any]]:
    resolved: dict[str, dict[str, Any]] = {}
    wanted = oracle_path.replace("\\", "/").casefold()
    expected_module = Path(oracle_path).name.casefold()
    for row in state.get("functions", []):
        row_target = str(row.get("target", "")).replace("\\", "/").casefold()
        if row_target != wanted:
            continue
        if row.get("target_sha256") != oracle_sha256:
            raise CheckError(f"recovery row {row.get('id')} targets ENG1.DLL with a different oracle hash")
        row_module = row.get("module")
        if row_module is not None and str(row_module).casefold() != expected_module:
            raise CheckError(f"recovery row {row.get('id')} has conflicting module identity {row_module!r}")
        symbol = row.get("symbol")
        rva = row.get("rva")
        if not symbol or rva is None:
            continue
        item = {"rva": parse_rva(rva), "resolution": "recovery.json function row", "function_id": row.get("id"), "status": row.get("status")}
        if symbol in resolved and resolved[symbol]["rva"] != item["rva"]:
            raise CheckError(f"recovery.json gives conflicting addresses for {symbol}")
        resolved[symbol] = item
    return resolved


def resolve_target(symbol: str, data_symbols: dict[str, dict[str, Any]], function_symbols: dict[str, dict[str, Any]]) -> dict[str, Any] | None:
    if symbol in data_symbols:
        field = data_symbols[symbol]
        return {"rva": parse_rva(field["rva"]), "resolution": "measured provider field manifest", "field": field["name"]}
    if symbol in function_symbols:
        return function_symbols[symbol]
    bare = symbol[1:] if symbol.startswith("_") else symbol
    match = re.fullmatch(r"eng1_(?:frame_target|native_code_target)_([0-9a-fA-F]{4,8})", bare)
    if match:
        return {"rva": int(match.group(1), 16), "resolution": "explicit ENG1 RVA alias suffix"}
    return None


def read_word(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def write_word(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", data, offset, value & 0xFFFFFFFF)


def make_compile_env(toolchain: Path) -> dict[str, str]:
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    return env


def compile_provider(source: Path, out_dir: Path, flags: list[str], toolchain: Path) -> tuple[Path, list[str], str]:
    out_dir.mkdir(parents=True, exist_ok=True)
    obj = out_dir / (source.stem + ".obj")
    command = [str(toolchain / "bin" / "cl.exe"), "/nologo", "/c", "/Gy", *flags,
               "/Fo" + str(obj), str(source)]
    run = subprocess.run(command, cwd=out_dir, env=make_compile_env(toolchain), capture_output=True, text=True)
    log = run.stdout + run.stderr
    (out_dir / "compile.log").write_text(log, encoding="utf-8", newline="\n")
    if run.returncode != 0 or not obj.is_file():
        raise CheckError(f"MSVC 5 compilation failed for {source}; see {out_dir / 'compile.log'}")
    return obj, command, log


def defined_public_data(coff: COFF) -> dict[str, dict[str, Any]]:
    found: dict[str, dict[str, Any]] = {}
    for symbol in coff.symbols.values():
        if symbol["storage"] != 2:
            continue
        if symbol["type"] & 0x20:
            continue
        if symbol["section"] > 0 or (symbol["section"] == 0 and symbol["value"] > 0):
            if symbol["name"] in found:
                raise CheckError(f"duplicate public COFF definition {symbol['name']}")
            found[symbol["name"]] = symbol
    return found


def overlaps(rows: list[tuple[int, int, str]]) -> list[dict[str, Any]]:
    rows = sorted(rows)
    errors = []
    for left, right in zip(rows, rows[1:]):
        if left[1] > right[0]:
            errors.append({"left": left[2], "left_interval": [hex(left[0]), hex(left[1])],
                           "right": right[2], "right_interval": [hex(right[0]), hex(right[1])]})
    return errors


def check_manifest(manifest: dict[str, Any]) -> None:
    if manifest.get("schema") != 1:
        raise CheckError("unsupported ENG1 data manifest schema")
    if manifest.get("oracle", {}).get("sha256") != "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757":
        raise CheckError("manifest oracle hash is not the measured ENG1.DLL")
    if not manifest.get("providers"):
        raise CheckError("manifest has no provider entries")
    all_fields = []
    source_names = set()
    for provider in manifest["providers"]:
        source = provider.get("source")
        if not source or source in source_names:
            raise CheckError(f"provider source is absent or repeated: {source}")
        if Path(source).is_absolute() or ".." in Path(source).parts:
            raise CheckError(f"provider source must be a repository-relative path: {source}")
        source_names.add(source)
        if not provider.get("source_sha256"):
            raise CheckError(f"provider source must be bound to a current SHA-256: {source}")
        evidence = provider.get("evidence", [])
        if not evidence:
            raise CheckError(f"provider lacks public evidence references: {source}")
        for entry in evidence:
            if not entry.startswith("evidence/") or Path(entry).is_absolute() or ".." in Path(entry).parts:
                raise CheckError(f"provider evidence must be a public repository evidence path: {entry}")
        fields = provider.get("fields", [])
        if not fields:
            raise CheckError(f"provider has no measured fields: {source}")
        symbols = set()
        for field in fields:
            symbol = field.get("symbol")
            if not symbol or symbol in symbols:
                raise CheckError(f"provider field symbol is absent or repeated: {source}:{symbol}")
            symbols.add(symbol)
            if field.get("scope") != "prefix":
                raise CheckError(f"manifest field scope must remain prefix: {source}:{symbol}")
            if "storage" in field and field["storage"] != "read_only_table":
                raise CheckError(f"unsupported explicit storage class: {source}:{symbol}")
            required_keys = {"symbol", "rva", "size", "scope"}
            if not required_keys <= set(field) or set(field) - required_keys - {"allow_compiled_zero_fill", "storage"}:
                raise CheckError(f"field definition has missing or unknown keys: {source}:{symbol}")
            if "allow_compiled_zero_fill" in field and field["allow_compiled_zero_fill"] is not True:
                raise CheckError(f"loaded-zero equivalence must be an explicit true opt-in: {source}:{symbol}")
            if int(field.get("size", 0)) <= 0:
                raise CheckError(f"invalid field width: {source}:{symbol}")
            all_fields.append((parse_rva(field["rva"]), parse_rva(field["rva"]) + int(field["size"]), f"{source}:{symbol}"))
    collisions = overlaps(all_fields)
    if collisions:
        raise CheckError("manifest native intervals overlap: " + json.dumps(collisions[:8]))


def validate_public_contracts(manifest: dict[str, Any], tracked_before: dict[str, bytes],
                              oracle_sha256: str) -> dict[str, Any]:
    """Bind the measured prefix definitions to the adopted public contracts."""
    fields_by_source = {
        provider["source"]: {field["symbol"]: field for field in provider["fields"]}
        for provider in manifest["providers"]
    }
    loaded: dict[str, dict[str, Any]] = {}
    for provider in manifest["providers"]:
        for relative in provider["evidence"]:
            if relative in loaded:
                continue
            data = tracked_before[str((ROOT / relative).resolve())]
            contract = json.loads(data)
            for key in ("oracle_sha256", "target_sha256"):
                if key in contract and contract[key] != oracle_sha256:
                    raise CheckError(f"{relative} is bound to a different native oracle via {key}")
            loaded[relative] = contract

    def expect(source: str, symbol: str, rva: Any, size: Any, evidence: str) -> None:
        canonical = symbol if symbol.startswith("_") else "_" + symbol
        field = fields_by_source.get(source, {}).get(canonical)
        if field is None:
            raise CheckError(f"{evidence} field is absent from manifest: {canonical}")
        if parse_rva(field["rva"]) != parse_rva(rva) or int(field["size"]) != int(size):
            raise CheckError(f"{evidence} interval differs from manifest for {canonical}")

    actor_source = "src/eng1/actor_storage.c"
    actor = loaded["evidence/eng1_actor_storage.json"]
    actor_fields = fields_by_source[actor_source]
    expected_actor = sorted((parse_rva(f["rva"]), int(f["size"])) for f in actor_fields.values())
    observed_actor = sorted((parse_rva(cell["rva"]), int(cell["size"])) for cell in actor["cells"])
    if expected_actor != observed_actor:
        raise CheckError("actor storage manifest intervals differ from the adopted native backing contract")

    dispatch_source = "src/eng1/dispatch_storage.c"
    dispatch = loaded["evidence/eng1_dispatch_storage.json"]
    for field in dispatch["fields"]:
        expect(dispatch_source, field["symbol"], field["rva"], field["minimum_bytes"], "dispatch contract")

    camera_source = "src/eng1/camera_storage.c"
    camera = loaded["evidence/eng1_camera.json"]
    for field in camera["fields"]:
        expect(camera_source, field["symbol"], field["rva"], field["bytes"], "camera contract")

    static_source = "src/eng1/static_pointees.c"
    static = loaded["evidence/eng1_static_pointees.json"]
    for field in static["checks"]:
        expect(static_source, field["symbol"], field["rva"], field["size"], "static-pointee contract")

    row0_source = "src/eng1/row0_pointees.c"
    row0 = loaded["evidence/eng1_row0_pointees.json"]
    start = parse_rva(row0["target_span"]["start"])
    end = parse_rva(row0["target_span"]["end"])
    table_offsets = sorted((parse_rva(offset), symbol)
                           for symbol, offset in row0["candidate_table_symbol_offsets"].items())
    for index, (offset, symbol) in enumerate(table_offsets):
        next_offset = table_offsets[index + 1][0] if index + 1 < len(table_offsets) else end - start
        expect(row0_source, symbol, start + offset, next_offset - offset, "row-0 table contract")

    frame_source = "src/eng1/frame_data.c"
    descriptor = loaded["evidence/eng1_descriptor_pointees.json"]
    for field in descriptor["checks"]:
        expect(frame_source, field["symbol"], field["oracle_rva"], field["size"], "descriptor-pointee contract")

    return {"validated_documents": sorted(loaded), "validated_interval_groups": 6}


def run(manifest_path: Path, output: Path, original_root: Path | None = None) -> dict[str, Any]:
    manifest_path = manifest_path.resolve()
    output = output.resolve()
    verifier_path = Path(__file__).resolve()
    if not in_work(output):
        raise CheckError("diagnostic reports and generated objects must remain under work/")
    manifest_bytes, manifest = load_json_bytes(manifest_path)
    check_manifest(manifest)

    oracle_path_rel = manifest["oracle"]["path"]
    # Keep the manifest's target identity for recovery-row binding. A worktree
    # may read the pinned original from another checkout without copying it.
    oracle_path = ((original_root.resolve() if original_root else ROOT) / oracle_path_rel).resolve()
    oracle_before = oracle_path.read_bytes()
    oracle_sha256 = sha_bytes(oracle_before)
    if oracle_sha256 != manifest["oracle"]["sha256"]:
        raise CheckError("ENG1 oracle hash changed")
    pe = PE(oracle_path)
    reloc_map = parse_native_relocations(pe)

    recovery_path = ROOT / "recovery.json"
    recovery_before, recovery = load_json_bytes(recovery_path)
    if recovery.get("module") is not None and str(recovery.get("module")).casefold() != "eng1.dll":
        # Some project schemas are global and omit module; per-row target/hash is authoritative.
        raise CheckError("unexpected recovery.json module identity")
    function_symbols = recover_function_symbols(recovery, oracle_path_rel, oracle_sha256)

    toolchain_manifest = ROOT / "toolchains" / "manifest.json"
    toolchain_manifest_before = toolchain_manifest.read_bytes()
    verify_toolchain.cache_clear()
    toolchain_id_before = verify_toolchain(DEFAULT_TOOLCHAIN)
    expected_toolchain = manifest.get("toolchain_id")
    if expected_toolchain and toolchain_id_before != expected_toolchain:
        raise CheckError(f"toolchain id changed: expected {expected_toolchain}, got {toolchain_id_before}")

    evidence_paths = sorted({entry for provider in manifest["providers"] for entry in provider.get("evidence", [])})
    expected_inputs = manifest.get("inputs_sha256", {})
    if set(expected_inputs) != set(evidence_paths):
        raise CheckError("inputs_sha256 must bind exactly the public evidence paths referenced by providers")
    tracked_paths = {manifest_path, verifier_path, oracle_path, recovery_path, toolchain_manifest}
    provider_sources = []
    for provider in manifest["providers"]:
        source = (ROOT / provider["source"]).resolve()
        provider_sources.append(source)
        tracked_paths.add(source)
    tracked_paths.update((ROOT / item).resolve() for item in evidence_paths)
    context_paths = scan_quoted_includes(provider_sources)
    tracked_paths.update(context_paths)
    tracked_before = {str(path): path.read_bytes() for path in sorted(tracked_paths, key=str)}
    for relative, expected_sha in expected_inputs.items():
        path = str((ROOT / relative).resolve())
        if path not in tracked_before:
            raise CheckError(f"public evidence input is not tracked by any provider: {relative}")
        if sha_bytes(tracked_before[path]) != expected_sha:
            raise CheckError(f"public evidence identity changed: {relative}")
    evidence_validation = validate_public_contracts(manifest, tracked_before, oracle_sha256)

    all_data_fields: dict[str, dict[str, Any]] = {}
    fields_by_provider: dict[str, dict[str, dict[str, Any]]] = {}
    for provider in manifest["providers"]:
        by_symbol = {field["symbol"]: field for field in provider["fields"]}
        fields_by_provider[provider["source"]] = by_symbol
        for symbol, field in by_symbol.items():
            if symbol in all_data_fields:
                raise CheckError(f"COFF data symbol reused between providers: {symbol}")
            all_data_fields[symbol] = {**field, "name": symbol, "source": provider["source"]}

    output.parent.mkdir(parents=True, exist_ok=True)
    provider_reports = []
    unresolved: list[dict[str, Any]] = []
    mismatches: list[dict[str, Any]] = []
    native_intervals: list[tuple[int, int, str]] = []
    compile_root = output.parent / "objects"

    for provider in manifest["providers"]:
        source = (ROOT / provider["source"]).resolve()
        source_before = source.read_bytes()
        expected_source = provider.get("source_sha256")
        if expected_source and sha_bytes(source_before) != expected_source:
            raise CheckError(f"provider source differs from manifest identity: {provider['source']}")
        source_key = provider["source"].replace("/", "_").replace("\\", "_").replace(".", "_")
        obj, command, _ = compile_provider(source, compile_root / source_key, provider.get("flags", ["/O2"]), DEFAULT_TOOLCHAIN)
        coff = COFF(obj)
        fields = fields_by_provider[provider["source"]]
        defined = defined_public_data(coff)
        expected_names = set(fields)
        actual_names = set(defined)
        missing = sorted(expected_names - actual_names)
        extra = sorted(actual_names - expected_names)
        if missing or extra:
            mismatches.append({"provider": provider["source"], "kind": "public definition manifest mismatch", "missing": missing, "unmanifested": extra})

        section_relocs = []
        for section_index, section in enumerate(coff.sections, start=1):
            for relocation in section["relocations"]:
                section_relocs.append({"section_index": section_index, "section": section["name"], **relocation})
        field_reports = []
        for name, field in fields.items():
            field_mismatch_start = len(mismatches)
            field_unsupported_start = len(unresolved)
            rva = parse_rva(field["rva"])
            size = int(field["size"])
            native_intervals.append((rva, rva + size, f"{provider['source']}:{name}"))
            if name not in defined:
                field_reports.append({"symbol": name, "rva": hex(rva), "size": size, "scope": "prefix", "status": "mismatch", "reason": "manifest field has no public COFF definition"})
                continue
            symbol = defined[name]
            native, kind, native_sec = native_bytes(pe, field)
            zero_fill_allowed = bool(field.get("allow_compiled_zero_fill", False)) and not any(native)
            section: dict[str, Any] | None = None
            if symbol["section"] == 0:
                common_size = int(symbol["value"])
                if field.get("storage") == "read_only_table":
                    mismatches.append({"provider": provider["source"], "symbol": name, "kind": "read-only table cannot use writable COMMON storage"})
                if common_size != size or (kind != "loader_zero" and not zero_fill_allowed):
                    mismatches.append({"provider": provider["source"], "symbol": name, "kind": "COMMON width/backing mismatch", "common_size": common_size, "manifest_size": size, "native_kind": kind})
                candidate = bytearray(size)
                object_offset = None
            else:
                section = coff.sections[symbol["section"] - 1]
                object_offset = int(symbol["value"])
                if object_offset + size > section["size"]:
                    mismatches.append({"provider": provider["source"], "symbol": name, "kind": "field exceeds COFF section", "section_size": section["size"], "symbol_offset": object_offset, "size": size})
                    candidate = bytearray()
                elif (kind == "loader_zero" or zero_fill_allowed) and section["name"] == ".bss" and section["flags"] & 0x80 and not section["data"]:
                    # BSS has no raw COFF bytes; its effective field value is zero.
                    candidate = bytearray(size)
                else:
                    candidate = bytearray(section["data"][object_offset:object_offset + size])
                writable = bool(section["flags"] & 0x80000000)
                read_only_table = field.get("storage") == "read_only_table"
                if read_only_table:
                    if writable or not section["flags"] & 0x40000000 or section["flags"] & 0x20000000 or section["name"] != ".rdata":
                        mismatches.append({"provider": provider["source"], "symbol": name, "kind": "read-only table is not nonexecutable COFF .rdata", "section": section["name"], "flags": hex(section["flags"])})
                elif not writable:
                    mismatches.append({"provider": provider["source"], "symbol": name, "kind": "COFF data section is not writable", "section": section["name"], "flags": hex(section["flags"])})
                if kind == "loader_zero":
                    if section["name"] != ".bss" or not (section["flags"] & 0x80) or section["data"]:
                        mismatches.append({"provider": provider["source"], "symbol": name, "kind": "loader-zero field is not represented by rawless COFF BSS", "section": section["name"], "flags": hex(section["flags"]), "section_data_bytes": len(section["data"])})
                elif not read_only_table and section["name"] != ".data" and not (zero_fill_allowed and section["name"] == ".bss" and section["flags"] & 0x80 and not section["data"]):
                    mismatches.append({"provider": provider["source"], "symbol": name, "kind": "initialized field is not represented by COFF .data", "section": section["name"]})

            field_relocs = []
            if symbol["section"] > 0 and section is not None:
                for relocation in section["relocations"]:
                    if object_offset <= relocation["offset"] < object_offset + size:
                        absolute_end = relocation["offset"] + 4
                        if absolute_end > object_offset + size:
                            mismatches.append({"provider": provider["source"], "symbol": name, "kind": "relocation cell crosses measured field prefix", "offset": hex(relocation["offset"] - object_offset)})
                            continue
                        field_relocs.append(relocation)

            expected_reloc_cells: set[int] = set()
            actual_reloc_cells = {cell: typ for cell, typ in reloc_map.items() if rva <= cell < rva + size}
            field_unresolved = []
            pointer_reports = []
            if kind == "initialized":
                for relocation in field_relocs:
                    rel_offset = int(relocation["offset"]) - int(object_offset)
                    cell_rva = rva + rel_offset
                    expected_reloc_cells.add(cell_rva)
                    if relocation["type"] != I386_DIR32:
                        issue = {"provider": provider["source"], "symbol": name, "cell_rva": hex(cell_rva), "kind": "unsupported COFF relocation type", "coff_type": relocation["type"]}
                        unresolved.append(issue)
                        field_unresolved.append(issue)
                        continue
                    resolution = resolve_target(relocation["symbol"], all_data_fields, function_symbols)
                    if resolution is None:
                        issue = {"provider": provider["source"], "symbol": name, "cell_rva": hex(cell_rva), "target_symbol": relocation["symbol"], "kind": "unsupported: target identity has no native RVA mapping"}
                        unresolved.append(issue)
                        field_unresolved.append(issue)
                        continue
                    addend = read_word(section["data"], int(relocation["offset"]))
                    expected_target_rva = int(resolution["rva"]) + addend
                    expected_word = pe.image_base + expected_target_rva
                    actual_type = reloc_map.get(cell_rva)
                    actual_word = read_word(native, rel_offset)
                    actual_target_rva = actual_word - pe.image_base if actual_word >= pe.image_base else None
                    target_is_mapped = (0 <= expected_target_rva < pe.pe.OPTIONAL_HEADER.SizeOfImage and
                                        any(s["virtual_address"] <= expected_target_rva < s["virtual_address"] + s["virtual_size"]
                                            for s in pe.sections))
                    pointer_ok = (target_is_mapped and actual_type == TARGET_HIGHLOW and
                                  actual_word == expected_word and actual_target_rva == expected_target_rva)
                    if not pointer_ok:
                        mismatches.append({"provider": provider["source"], "symbol": name, "cell_rva": hex(cell_rva), "kind": "native pointer target mismatch", "target_symbol": relocation["symbol"], "resolution": resolution["resolution"], "addend": hex(addend), "expected_target_rva": hex(expected_target_rva), "expected_target_mapped": target_is_mapped, "actual_target_rva": hex(actual_target_rva) if actual_target_rva is not None else None, "native_relocation_type": actual_type, "expected_native_relocation_type": TARGET_HIGHLOW})
                    if len(candidate) == size:
                        write_word(candidate, rel_offset, expected_word)
                    pointer_reports.append({"cell_rva": hex(cell_rva), "target_symbol": relocation["symbol"], "resolution": resolution["resolution"], "addend": hex(addend), "expected_target_rva": hex(expected_target_rva), "actual_target_rva": hex(actual_target_rva) if actual_target_rva is not None else None, "native_relocation_type": actual_type, "target_exact": pointer_ok})
            elif field_relocs:
                mismatches.append({"provider": provider["source"], "symbol": name, "kind": "loader-zero field unexpectedly has COFF relocations", "count": len(field_relocs)})

            if expected_reloc_cells != set(actual_reloc_cells):
                mismatches.append({"provider": provider["source"], "symbol": name, "kind": "COFF/native relocation cell set differs", "compiled_cells": sorted(hex(x) for x in expected_reloc_cells), "native_cells": {hex(k): v for k, v in sorted(actual_reloc_cells.items())}})
            non_highlow = {hex(cell): typ for cell, typ in actual_reloc_cells.items() if typ != TARGET_HIGHLOW}
            if non_highlow:
                mismatches.append({"provider": provider["source"], "symbol": name, "kind": "unexpected native base-relocation type", "cells": non_highlow})

            byte_exact = len(candidate) == size and bytes(candidate) == native
            if kind == "initialized" and not field_unresolved and not byte_exact:
                mismatches.append({"provider": provider["source"], "symbol": name, "kind": "reconstructed COFF bytes differ from native field", "compiled_sha256": sha_bytes(bytes(candidate)), "native_sha256": sha_bytes(native), "difference_count": sum(a != b for a, b in zip(candidate, native))})
            if kind == "loader_zero" and any(native):
                mismatches.append({"provider": provider["source"], "symbol": name, "kind": "loader-zero native bytes are not zero"})
            field_status = ("unsupported" if len(unresolved) > field_unsupported_start else
                            "mismatch" if len(mismatches) > field_mismatch_start or not byte_exact else
                            "exact")
            field_reports.append({
                "symbol": name, "rva": hex(rva), "size": size, "scope": "prefix",
                "native_kind": kind,
                "compiled_zero_fill_equivalence": zero_fill_allowed and (section is None or section["name"] == ".bss"),
                "coff_section": section["name"] if section else "COMMON",
                "coff_value": object_offset if object_offset is not None else symbol["value"],
                "compiled_relocations": len(field_relocs), "native_relocations": len(actual_reloc_cells),
                "relocations": pointer_reports,
                "compiled_sha256": sha_bytes(bytes(candidate)) if len(candidate) == size else None,
                "native_sha256": sha_bytes(native), "bytes_exact": byte_exact,
                "status": field_status,
            })
        # No relocation may be silently left outside the measured symbols.
        for relocation in section_relocs:
            section = coff.sections[relocation["section_index"] - 1]
            owners = []
            for name, symbol in defined.items():
                if name in fields and symbol["section"] == relocation["section_index"] and symbol["section"] > 0:
                    field = fields[name]
                    start = int(symbol["value"])
                    end = start + int(field["size"])
                    if start <= relocation["offset"] and relocation["offset"] + 4 <= end:
                        owners.append(name)
            if len(owners) != 1:
                unresolved.append({"provider": provider["source"], "section": section["name"], "relocation_offset": hex(relocation["offset"]), "target_symbol": relocation["symbol"], "kind": "unsupported: COFF relocation lacks one measured field owner", "owners": owners})
        provider_reports.append({
            "source": provider["source"], "source_sha256": sha_bytes(source_before),
            "object": str(obj.resolve()), "object_sha256": sha_bytes(obj.read_bytes()),
            "command": command, "defined_public_data_count": len(defined),
            "manifest_field_count": len(fields), "coff_relocation_count": len(section_relocs),
            "fields": field_reports,
        })

    cross_overlap = overlaps(native_intervals)
    if cross_overlap:
        mismatches.append({"kind": "cross-provider/native field overlap", "intervals": cross_overlap})

    tracked_after = {str(path): path.read_bytes() for path in sorted(tracked_paths, key=str)}
    changed_inputs = [path for path in tracked_before if tracked_before[path] != tracked_after[path]]
    if changed_inputs:
        raise CheckError("source, header, receipt, manifest, target, toolchain manifest or recovery state changed during compilation: " + ", ".join(changed_inputs))
    verify_toolchain.cache_clear()
    toolchain_id_after = verify_toolchain(DEFAULT_TOOLCHAIN)
    if toolchain_id_before != toolchain_id_after:
        raise CheckError("pinned toolchain identity changed during compilation")

    blocked = bool(unresolved)
    result = "UNSUPPORTED" if blocked else ("MISMATCH" if mismatches else "PASS")
    input_hashes = {path: sha_bytes(data) for path, data in tracked_before.items()}
    report = {
        "schema": 1,
        "result": result,
        "scope": "Fresh MSVC 5 /O2 provider data bytes and each COFF-to-native relocation target; no function, TU, linker-layout or whole-module acceptance.",
        "manifest": {"path": str(manifest_path), "sha256": sha_bytes(manifest_bytes)},
        "verifier": {"path": str(verifier_path), "sha256": sha_bytes(tracked_before[str(verifier_path)])},
        "oracle": {"path": oracle_path_rel, "read_path": str(oracle_path), "sha256": oracle_sha256, "image_base": hex(pe.image_base), "native_relocation_cells": len(reloc_map)},
        "toolchain": {"id": toolchain_id_before, "path": str(DEFAULT_TOOLCHAIN), "manifest_sha256": sha_bytes(toolchain_manifest_before), "verified_before_and_after": True},
        "python": {"executable": sys.executable, "version": sys.version.split()[0]},
        "input_sha256_before": input_hashes,
        "input_sha256_after": {path: sha_bytes(data) for path, data in tracked_after.items()},
        "recovery": {
            "sha256_before": sha_bytes(recovery_before),
            "sha256_after": sha_bytes(tracked_after[str(recovery_path)]),
            "pinned_by_manifest": False,
            "ENG1_rows_checked": sum(1 for row in recovery.get("functions", []) if str(row.get("target", "")).replace("\\", "/").casefold() == oracle_path_rel.replace("\\", "/").casefold()),
        },
        "source_context": {
            "provider_sources": {provider["source"]: provider.get("source_sha256") for provider in manifest["providers"]},
            "transitive_quoted_includes": {repo_relative(path): sha_bytes(tracked_before[str(path)]) for path in context_paths},
        },
        "public_evidence": {relative: expected_inputs[relative] for relative in evidence_paths},
        "public_contract_validation": evidence_validation,
        "providers": provider_reports,
        "unsupported": unresolved,
        "mismatches": mismatches,
        "functions": 0,
        "promotion": "none; diagnostic data evidence only",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes((json.dumps(report, indent=2) + "\n").encode("utf-8"))
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--original-root", type=Path, help="Read only the pinned original from this checkout; retain local sources, evidence and recovery state")
    args = parser.parse_args()
    try:
        report = run(args.manifest, args.output, args.original_root)
    except Exception as exc:
        print(f"ENG1 data verification failed before a complete report: {exc}", file=sys.stderr)
        return 1
    print(json.dumps({"result": report["result"], "providers": len(report["providers"]), "fields": sum(len(p["fields"]) for p in report["providers"]), "relocations": sum(p["coff_relocation_count"] for p in report["providers"]), "unsupported": len(report["unsupported"]), "mismatches": len(report["mismatches"]), "report": str(args.output.resolve())}, indent=2))
    return 0 if report["result"] == "PASS" else (2 if report["result"] == "UNSUPPORTED" else 1)


if __name__ == "__main__":
    raise SystemExit(main())
