"""Historical source differential of the original Config readers/helpers and provider closure.

Builds each source as an independent, pinned VC5 COFF object, compares complete
target contributions, then runs a bounded 24-fixture original-vs-candidate
Unicorn matrix with synthetic IAT endpoints. This is diagnostic evidence only.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--native-exe", type=Path,
                    default=root / "work/discs/pc_install/HERCULES.EXE")
parser.add_argument("--source-root", type=Path,
                    default=root)
parser.add_argument("--original-root", type=Path,
                    default=Path(r"D:/Prog/hercules_recon"),
                    help="Read-only checkout containing the original function inventory")
parser.add_argument("--output", type=Path,
                    default=root / "work/host_config_differential")
args = parser.parse_args()
oracle = args.native_exe.resolve()
source_root = args.source_root.resolve()
if not source_root.is_dir():
    raise FileNotFoundError(f"Source worktree is not present: {source_root}")
output_dir = args.output.resolve()
if not output_dir.is_relative_to((root / "work").resolve()):
    raise ValueError("Generated outputs must remain under ignored work/")
output_dir.mkdir(parents=True, exist_ok=True)
build_dir = output_dir / "objects"
build_dir.mkdir(parents=True, exist_ok=True)
expected_oracle_hash = "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d"
oracle_hash = hashlib.sha256(oracle.read_bytes()).hexdigest()
if oracle_hash != expected_oracle_hash:
    raise ValueError(f"Pinned original EXE hash differs: {oracle_hash}")

manifest_path = source_root / "toolchains/manifest.json"
manifest_bytes = manifest_path.read_bytes()
manifest = json.loads(manifest_bytes.decode("utf-8-sig"))
toolchain = next(row for row in manifest["toolchains"]
                 if row["directory"] == "msvc500-8abf95ce980161ad87b0b02402269cce76988953")
tc = Path(r"C:/tools/hercules") / toolchain["directory"]
for row in toolchain["files"]:
    path = tc / row["path"]
    data = path.read_bytes()
    if len(data) != row["size"] or hashlib.sha256(data).hexdigest() != row["sha256"]:
        raise ValueError(f"Pinned toolchain identity changed: {path}")

source_units = [
    ("reader", "host_registry_readers", source_root / "src/pc/host_config_readers.c"),
    ("cluster", "host_config_helpers", source_root / "src/pc/host_config_helpers.c"),
    ("context", "host_config_context_setters", source_root / "src/pc/host_config_context_setters.c"),
    ("cleanup", "cleanup_registry", source_root / "src/pc/cleanup_registry.c"),
    ("audio", "audio_small_helpers", source_root / "src/pc/audio_small_helpers.c"),
    ("registry_storage", "registry_storage", source_root / "src/shared/registry_storage.c"),
    ("audio_storage", "audio_gain_storage", source_root / "src/pc/audio_gain_storage.c"),
    ("literals", "registry_literals", source_root / "src/shared/registry_literals.c"),
    ("startup_storage", "host_startup_storage", source_root / "src/pc/host_startup_storage.c"),
]
for _, _, source in source_units:
    if not source.is_file():
        raise FileNotFoundError(f"Required frozen/canonical source is not present: {source}")
source_hashes_before = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                        for _, _, path in source_units}
env = os.environ.copy()
env.pop("CL", None)
env.pop("_CL_", None)
env["PATH"] = str(tc / "bin") + os.pathsep + env.get("PATH", "")
env["INCLUDE"] = str(tc / "include")
env["LIB"] = str(tc / "lib")
include_hashes = {}
headers_per_provider = {}
preprocessor_receipts = []
line_marker = re.compile(r'^#line\s+\d+\s+"([^"]+)"$', re.MULTILINE)
source_file_set = {str(path.resolve()).casefold() for _, _, path in source_units}
for provider, _, source in source_units:
    command = [str(tc / "bin/cl.exe"), "/nologo", "/E", "/Gy", "/O2", str(source)]
    result = subprocess.run(command, cwd=source_root, env=env,
                            capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"VC5 preprocessing failed for {source}: {result.stderr}")
    provider_headers = set()
    missing_markers = []
    for raw_path in line_marker.findall(result.stdout):
        header = Path(raw_path.replace("\\\\", "\\")).resolve()
        if str(header).casefold() in source_file_set:
            continue
        if not header.is_file():
            missing_markers.append(str(header))
            continue
        header_key = str(header)
        include_hashes[header_key] = hashlib.sha256(header.read_bytes()).hexdigest()
        provider_headers.add(header_key)
    if missing_markers:
        raise RuntimeError(f"VC5 emitted unresolvable include paths for {source}: {missing_markers}")
    headers_per_provider[provider] = sorted(provider_headers)
    preprocessor_receipts.append({
        "provider": provider,
        "source": str(source),
        "command": command,
        "returncode": result.returncode,
        "preprocessed_sha256": hashlib.sha256(result.stdout.encode("utf-8")).hexdigest(),
        "transitive_header_count": len(provider_headers),
    })
object_paths = {}
compile_commands = []
for object_key, stem, source in source_units:
    obj = build_dir / f"{stem}.obj"
    command = [str(tc / "bin/cl.exe"), "/nologo", "/c", "/Gy", "/O2",
               "/Fo" + str(obj), str(source)]
    compiled = subprocess.run(command, cwd=output_dir, env=env,
                              capture_output=True, text=True)
    compiler_output = compiled.stdout + compiled.stderr
    if compiled.returncode != 0 or not obj.is_file():
        raise RuntimeError(f"VC5 compile failed for {source}:\n{compiler_output}")
    compile_commands.append({
        "provider": object_key,
        "source": str(source),
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "command": command,
        "returncode": compiled.returncode,
        "compiler_output": compiler_output,
        "object": str(obj),
        "object_sha256": hashlib.sha256(obj.read_bytes()).hexdigest(),
        "object_size": obj.stat().st_size,
    })
    object_paths[object_key] = obj
source_hashes_after = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                       for _, _, path in source_units}
if source_hashes_before != source_hashes_after:
    raise RuntimeError("A source changed while the independent objects were compiling")
include_hashes_after = {path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
                        for path in include_hashes}
if include_hashes != include_hashes_after:
    raise RuntimeError("A transitive header changed during preprocessing or compilation")
sys.path.insert(0, str(source_root / "scripts"))
sys.path.insert(0, r"C:/tools/hercules/python")
sys.path.insert(0, r"C:/tools/hercules/renderer-oracle-python")
from pe import PE
from coff import COFF
from match import compare
from masked import masked_compare
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.unicorn_const import UC_PROT_ALL
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

BASE = 0x400000
CAND_BASE = 0x20000000
CAND_SIZE = 0x300000
STUB_BASE = 0x30000000
IAT_BASE = 0x30010000
STACK_BASE = 0x70000000
STACK_SIZE = 0x10000
SENTINEL = STUB_BASE + 0x0ff0
AUDIO_TRAP = STUB_BASE + 0x1800
API_NAMES = [
    "RegOpenKeyExA", "RegQueryValueExA", "RegCloseKey",
    "SetCurrentDirectoryA", "RegCreateKeyExA", "RegSetValueExA",
]
API_STUBS = {name: STUB_BASE + 0x100 + i * 0x20 for i, name in enumerate(API_NAMES)}
API_CELLS = {name: IAT_BASE + i * 4 for i, name in enumerate(API_NAMES)}
API_IMPORT_SYMBOLS = {
    "__imp__RegOpenKeyExA@20": "RegOpenKeyExA",
    "__imp__RegQueryValueExA@24": "RegQueryValueExA",
    "__imp__RegCloseKey@4": "RegCloseKey",
    "__imp__SetCurrentDirectoryA@4": "SetCurrentDirectoryA",
    "__imp__RegCreateKeyExA@36": "RegCreateKeyExA",
    "__imp__RegSetValueExA@24": "RegSetValueExA",
}
EXTERNALS = {
    "_g_host_engine_interface": BASE + 0x361b8,
    "_audio_native_fa10": AUDIO_TRAP,
}

pe = PE(oracle)
objects = {name: COFF(path) for name, path in object_paths.items()}
startup_storage_prefix_specs = [
    ("_g_config_default_values_rva_36158", 0x36158, 36, "initialized", "ce045a7f01f8a40183ac5e22912bef9e2010e92f78b392300eb93123029a1ab3"),
    ("_g_startup_apply_guard_rva_51c450", 0x51c450, 4, "loader_zero", "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
    ("_g_config_table_a_rva_56e4f0", 0x56e4f0, 112, "loader_zero", "b5fdab78d8947eacc864bfeecb4d2100780e5afe1cd8efafb124887913ac49fa"),
    ("_g_config_table_b_rva_56e560", 0x56e560, 112, "loader_zero", "b5fdab78d8947eacc864bfeecb4d2100780e5afe1cd8efafb124887913ac49fa"),
    ("_g_exe_b9e458", 0x79e458, 1024, "loader_zero", "5f70bf18a086007016e948b04aed3b82103a36bea41755b6cddfaf10ace3c6ef"),
    ("_g_host_input_joy_available_rva_79e8a0", 0x79e8a0, 16, "loader_zero", "374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb"),
    ("_g_host_input_joy_caps_rva_79e8b0", 0x79e8b0, 6464, "loader_zero", "3f2c7d671d4a2c210124550cf00f8e21727a0ae1a43e1758982a25725dde2b70"),
    ("_g_host_input_joy_count_rva_7a0218", 0x7a0218, 4, "loader_zero", "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
    ("_g_host_input_last_state_first_rva_56e5d0", 0x56e5d0, 4, "loader_zero", "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
    ("_g_host_input_last_state_second_rva_56e5d4", 0x56e5d4, 4, "loader_zero", "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
    ("_g_host_input_pending_rva_7a0234", 0x7a0234, 4, "loader_zero", "df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119"),
    ("_g_host_input_gate_rva_03a594", 0x3a594, 1, "initialized", "4bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"),
]
if sum(size for _, _, size, _, _ in startup_storage_prefix_specs) != 7785:
    raise AssertionError("Startup storage prefix extent changed")

def pe_section_for(rva, size):
    for section in pe.sections:
        start = section["virtual_address"]
        end = start + section["virtual_size"]
        if start <= rva and rva + size <= end:
            file_backed = max(0, min(rva + size, start + section["raw_size"]) - rva)
            return {"name": section["name"], "file_backed_bytes": file_backed,
                    "loader_zero_fill_bytes": size - file_backed}
    raise AssertionError(f"Native field outside PE sections: {rva:#x}+{size:#x}")

startup_storage = objects["startup_storage"]
startup_storage_prefixes = []
for symbol_name, rva, size, origin, expected_hash in startup_storage_prefix_specs:
    definitions = [sym for sym in startup_storage.symbols.values()
                   if sym["name"] == symbol_name and sym["section"] > 0]
    if len(definitions) != 1:
        raise RuntimeError(f"Expected one startup storage definition for {symbol_name}, got {len(definitions)}")
    symbol = definitions[0]
    section = startup_storage.sections[symbol["section"] - 1]
    if symbol["value"] + size > section["size"]:
        raise RuntimeError(f"COFF extent too short for {symbol_name}")
    native_section = pe_section_for(rva, size)
    if origin == "initialized":
        if section["name"] != ".data" or len(section["data"]) < symbol["value"] + size:
            raise RuntimeError(f"Initialized source bytes not present in .data for {symbol_name}")
        candidate_initial = section["data"][symbol["value"]:symbol["value"] + size]
        native_initial = pe.read_rva(rva, size)
        if native_section["file_backed_bytes"] != size:
            raise RuntimeError(f"Expected file-backed original bytes for {symbol_name}")
    else:
        if section["name"] != ".bss" or section["data"]:
            raise RuntimeError(f"Expected loader-zero COFF .bss for {symbol_name}")
        candidate_initial = bytes(size)
        native_initial = bytes(size)
        if native_section["loader_zero_fill_bytes"] != size:
            raise RuntimeError(f"Expected loader-zero original bytes for {symbol_name}")
    candidate_hash = hashlib.sha256(candidate_initial).hexdigest()
    native_hash = hashlib.sha256(native_initial).hexdigest()
    exact = candidate_initial == native_initial and candidate_hash == expected_hash == native_hash
    if not exact:
        raise RuntimeError(f"Original initial-prefix mismatch for {symbol_name}: {candidate_hash} vs {native_hash}")
    startup_storage_prefixes.append({
        "symbol": symbol_name,
        "rva": hex(rva),
        "size": size,
        "origin": origin,
        "coff_section": section["name"],
        "coff_section_size": section["size"],
        "coff_section_raw_data_size": len(section["data"]),
        "coff_section_flags": hex(section["flags"]),
        "coff_offset": symbol["value"],
        "candidate_initial_sha256": candidate_hash,
        "native_initial_sha256": native_hash,
        "initial_hex": candidate_initial.hex() if size <= 128 else None,
        "native_section": native_section,
        "exact_initial_bytes": exact,
    })

object_symbol_owners = {}
for object_name, coff in objects.items():
    for symbol in coff.symbols.values():
        if symbol["section"] > 0 and not symbol["name"].startswith("."):
            object_symbol_owners.setdefault(symbol["name"], []).append(object_name)
for symbol_name, expected_owner in {
    "_g_registry_config": "registry_storage",
    "_g_exe_79e438": "audio_storage",
    "_g_exe_79e43c": "audio_storage",
    "_g_registry_key": "literals",
    "_g_registry_value_name": "literals",
}.items():
    if object_symbol_owners.get(symbol_name) != [expected_owner]:
        raise RuntimeError(f"Shared storage/literal provider identity changed for {symbol_name}: {object_symbol_owners.get(symbol_name)}")
for symbol_name in ("_g_registry_config", "_g_exe_79e438", "_g_exe_79e43c",
                    "_g_registry_key", "_g_registry_value_name"):
    if symbol_name in {sym["name"] for sym in startup_storage.symbols.values() if sym["section"] > 0}:
        raise RuntimeError(f"Duplicate Config/audio/literal storage in startup provider: {symbol_name}")
if len(startup_storage_prefixes) != 12 or not all(row["exact_initial_bytes"] for row in startup_storage_prefixes):
    raise AssertionError("Startup storage prefix proof is incomplete")
if any(symbol in EXTERNALS for symbol, *_ in startup_storage_prefix_specs):
    raise RuntimeError("Owned startup storage was still bound as a foreign image address")
imports = {}
for module in pe.metadata()["imports"]:
    for symbol in module["symbols"]:
        if symbol["name"] in API_NAMES:
            imports[symbol["name"]] = symbol["iat_rva"]
assert set(imports) == set(API_NAMES), imports
candidate_symbol_bindings = {}
candidate_loader_verification_passes = 0


def u32(uc, address):
    return struct.unpack("<I", uc.mem_read(address, 4))[0]


def w32(uc, address, value):
    uc.mem_write(address, struct.pack("<I", value & 0xffffffff))


def cstr(uc, address, limit=512):
    if not address:
        return None
    data = bytearray()
    for i in range(limit):
        value = uc.mem_read(address + i, 1)[0]
        if value == 0:
            break
        data.append(value)
    return data.decode("ascii", "replace")


def align(value, alignment=0x1000):
    return (value + alignment - 1) & ~(alignment - 1)


def map_original(uc):
    uc.mem_map(BASE, align(pe.pe.OPTIONAL_HEADER.SizeOfImage), UC_PROT_ALL)
    for section in pe.sections:
        if section["raw_size"]:
            raw = pe.data[section["raw_offset"]:section["raw_offset"] + section["raw_size"]]
            uc.mem_write(BASE + section["virtual_address"], raw)


def map_candidate_objects(uc):
    global candidate_symbol_bindings
    section_addresses = {}
    symbols = dict(EXTERNALS)
    object_items = list(objects.items())
    for obj_index, (object_name, coff) in enumerate(object_items):
        for section_index, section in enumerate(coff.sections, 1):
            if section["name"] in (".drectve",) or section["name"].startswith(".debug"):
                continue
            if section["size"]:
                address = CAND_BASE + obj_index * 0x40000 + section_index * 0x1000
                if address + section["size"] > CAND_BASE + CAND_SIZE:
                    raise RuntimeError(f"Candidate section exceeds mapped workspace: {object_name}:{section['name']}")
                section_addresses[(obj_index, section_index)] = address
                data = section["data"]
                if data:
                    uc.mem_write(address, data)
    definitions = {}
    for obj_index, (object_name, coff) in enumerate(object_items):
        for symbol in coff.symbols.values():
            if symbol["section"] > 0 and not symbol["name"].startswith("."):
                if symbol["name"] in definitions:
                    raise RuntimeError(
                        f"duplicate candidate definition {symbol['name']}: "
                        f"{definitions[symbol['name']]} and {object_name}"
                    )
                definitions[symbol["name"]] = object_name
                symbols[symbol["name"]] = (
                    section_addresses[(obj_index, symbol["section"])] + symbol["value"]
                )
    symbols.update({name: API_CELLS[api] for name, api in API_IMPORT_SYMBOLS.items()})
    for obj_index, (object_name, coff) in enumerate(object_items):
        for section_index, section in enumerate(coff.sections, 1):
            if section["name"] in (".drectve",) or section["name"].startswith(".debug"):
                continue
            key = (obj_index, section_index)
            if key not in section_addresses:
                continue
            code = bytearray(section["data"])
            for relocation in section["relocations"]:
                offset = relocation["offset"]
                name = relocation["symbol"]
                kind = relocation["type"]
                if name not in symbols:
                    raise RuntimeError(f"unresolved candidate symbol {name}")
                target = symbols[name]
                if kind == 0x06:  # IMAGE_REL_I386_DIR32
                    addend = struct.unpack_from("<I", code, offset)[0]
                    struct.pack_into("<I", code, offset, (target + addend) & 0xffffffff)
                elif kind == 0x14:  # IMAGE_REL_I386_REL32
                    addend = struct.unpack_from("<i", code, offset)[0]
                    place = section_addresses[key] + offset
                    displacement = (target + addend - (place + 4)) & 0xffffffff
                    if displacement >= 0x80000000:
                        displacement -= 0x100000000
                    struct.pack_into("<i", code, offset, displacement)
                else:
                    raise RuntimeError(f"unsupported relocation {kind:#x} to {name}")
            if code:
                uc.mem_write(section_addresses[key], bytes(code))

    funcs = {}
    for symbol_name in [
        "_host_registry_read_path", "_host_registry_read_config",
        "_cleanup_native_4ac0", "_cleanup_native_4be0",
        "_cleanup_native_7aa0", "_cleanup_native_7b70",
        "_cleanup_native_4cc0", "_cleanup_native_1fb0",
        "_cleanup_native_1fd0", "_audio_native_fdd0",
    ]:
        if symbol_name not in symbols or symbol_name not in definitions:
            raise RuntimeError(f"candidate function not defined: {symbol_name}")
        funcs[symbol_name] = symbols[symbol_name]
    bound_names = [
        "_g_registry_config", "_g_exe_79e438", "_g_exe_79e43c",
        "_g_registry_key", "_g_registry_value_name", "_g_exe_b9e458",
        "_g_config_default_values_rva_36158", "_g_config_table_a_rva_56e4f0",
        "_g_config_table_b_rva_56e560", "_g_startup_apply_guard_rva_51c450",
        "_g_host_input_joy_available_rva_79e8a0", "_g_host_input_joy_caps_rva_79e8b0",
        "_g_host_input_joy_count_rva_7a0218", "_g_host_input_last_state_first_rva_56e5d0",
        "_g_host_input_last_state_second_rva_56e5d4", "_g_host_input_pending_rva_7a0234",
        "_g_host_input_gate_rva_03a594", "_g_host_engine_interface",
    ]
    candidate_symbol_bindings = {
        name: {
            "address": hex(symbols[name]),
            "provider": definitions.get(name, "pinned-original-view"),
        }
        for name in bound_names
    }
    return funcs, symbols


def calc_sum(data):
    value = 0x94375494
    for offset in range(4, 0x44):
        value = (value + struct.unpack_from("<I", data, offset)[0]) & 0xffffffff
    return value


def config_blob(level=3, control=1, stored="sum", table_values=None):
    data = bytearray(((i * 37 + 11) & 0xff) for i in range(72))
    data[0x0c] = control
    struct.pack_into("<i", data, 0x18, level)
    if table_values is not None:
        for i, value in enumerate(table_values):
            struct.pack_into("<I", data, 0x30 + 4 * i, value & 0xffffffff)
    checksum = calc_sum(data)
    value = 0xffffffff if stored == "sentinel" else checksum
    if stored == "bad":
        value ^= 1
    struct.pack_into("<I", data, 0, value)
    return bytes(data)


def expected_defaults():
    data = bytearray(72)
    words = [0x26, 0x28, 0x25, 0x27, 0x58, 0x5a, 0x43, 0x44, 0x53]
    for i, value in enumerate(words):
        struct.pack_into("<H", data, 0x1c + 2 * i, value)
    data[4] = data[5] = data[6] = 1
    data[0x0c] = data[0x0d] = data[0x0e] = 1
    data[0x14] = 1
    struct.pack_into("<I", data, 0x08, 0x00010103)
    struct.pack_into("<I", data, 0x10, 0x00003fff)
    struct.pack_into("<H", data, 0x16, 0xffff)
    struct.pack_into("<I", data, 0x18, 3)
    return bytes(data)


def expected_table_bytes(values):
    masks = [0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0008]
    selected = []
    for level in (3, 2, 1):
        for index, value in enumerate(values):
            if (value & 0x0fffffff).bit_count() == level:
                selected.append((value, masks[index]))
    table_a = bytearray(28 * 4)
    table_b = bytearray(28 * 4)
    for index, (value, mask) in enumerate(selected):
        struct.pack_into("<I", table_a, 4 * index, value)
        struct.pack_into("<I", table_b, 4 * index, mask)
    return bytes(table_a), bytes(table_b)


ORIGINAL_HELPERS = {
    BASE + 0x4a30: "path_reader",
    BASE + 0x4ce0: "config_reader",
    BASE + 0x4cc0: "checksum",
    BASE + 0x4be0: "defaults",
    BASE + 0x7aa0: "table_build",
    BASE + 0x7b70: "mask_test",
    BASE + 0x4ac0: "apply",
    BASE + 0xfdd0: "audio_store",
    BASE + 0x1fb0: "context_mode",
    BASE + 0x1fd0: "context_flag",
}
CANDIDATE_HELPER_SYMBOLS = {
    "_host_registry_read_path": "path_reader",
    "_host_registry_read_config": "config_reader",
    "_cleanup_native_4cc0": "checksum",
    "_cleanup_native_4be0": "defaults",
    "_cleanup_native_7aa0": "table_build",
    "_cleanup_native_7b70": "mask_test",
    "_cleanup_native_4ac0": "apply",
    "_audio_native_fdd0": "audio_store",
    "_cleanup_native_1fb0": "context_mode",
    "_cleanup_native_1fd0": "context_flag",
}


def run(kind, entry, fixture):
    global candidate_loader_verification_passes
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    map_original(uc)
    uc.mem_map(CAND_BASE, CAND_SIZE, UC_PROT_ALL)
    uc.mem_map(STUB_BASE, 0x3000, UC_PROT_ALL)
    uc.mem_map(IAT_BASE, 0x1000, UC_PROT_ALL)
    uc.mem_map(STACK_BASE, STACK_SIZE, UC_PROT_ALL)
    if kind == "candidate":
        funcs, candidate_symbols = map_candidate_objects(uc)
        config_address = candidate_symbols["_g_registry_config"]
        audio_address = candidate_symbols["_g_exe_79e438"]
        path_address = candidate_symbols["_g_exe_b9e458"]
        table_a_address = candidate_symbols["_g_config_table_a_rva_56e4f0"]
        table_b_address = candidate_symbols["_g_config_table_b_rva_56e560"]
        guard_address = candidate_symbols["_g_startup_apply_guard_rva_51c450"]
        for row in startup_storage_prefixes:
            address = candidate_symbols[row["symbol"]]
            initial = (bytes.fromhex(row["initial_hex"])
                       if row["initial_hex"] is not None else bytes(row["size"]))
            loaded = bytes(uc.mem_read(address, row["size"]))
            if loaded != initial or hashlib.sha256(loaded).hexdigest() != row["candidate_initial_sha256"]:
                raise RuntimeError(f"COFF loader initial bytes mismatch for {row['symbol']}")
        candidate_loader_verification_passes += 1
    else:
        funcs, candidate_symbols = {}, {}
        config_address = BASE + 0x79e858
        audio_address = BASE + 0x79e438
        path_address = BASE + 0x79e458
        table_a_address = BASE + 0x56e4f0
        table_b_address = BASE + 0x56e560
        guard_address = BASE + 0x51c450
    for name, rva in imports.items():
        w32(uc, BASE + rva, API_STUBS[name])
    for name in API_NAMES:
        w32(uc, API_CELLS[name], API_STUBS[name])
    uc.mem_write(AUDIO_TRAP, b"\xcc")

    config = fixture.get("config_seed", bytes([0xa5]) * 72)
    uc.mem_write(config_address, config)
    uc.mem_write(table_a_address, bytes([0xa5]) * (28 * 4))
    uc.mem_write(table_b_address, bytes([0x5a]) * (28 * 4))
    uc.mem_write(audio_address, bytes([0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7]))
    w32(uc, guard_address, fixture.get("guard", 0))
    context = u32(uc, BASE + 0x361bc)
    uc.mem_write(context + 0x0b, bytes([0xb1, 0xb2, 0xb3, 0xb4, 0xb5]))

    events = []
    hooks = dict(ORIGINAL_HELPERS)
    if kind == "candidate":
        hooks = {
            funcs[symbol]: name for symbol, name in CANDIDATE_HELPER_SYMBOLS.items()
        }
    api_by_address = {address: name for name, address in API_STUBS.items()}

    def api_return(esp, argument_count, value):
        return_to = u32(uc, esp)
        uc.reg_write(UC_X86_REG_ESP, esp + 4 + 4 * argument_count)
        uc.reg_write(UC_X86_REG_EIP, return_to)
        # EAX is deliberately excluded from semantic equality for these void
        # helpers and this API-focused fixture.
        uc.reg_write(UC_X86_REG_EAX, value & 0xffffffff)

    def hook(uc, address, size, user):
        if address == SENTINEL:
            uc.emu_stop()
            return
        if address == AUDIO_TRAP:
            raise RuntimeError("Unexpected unresolved audio_native_fa10 endpoint entered")
        if address in hooks:
            events.append({"kind": "helper", "name": hooks[address]})
        api = api_by_address.get(address)
        if api is None:
            return
        esp = uc.reg_read(UC_X86_REG_ESP)
        if api == "RegOpenKeyExA":
            args = [u32(uc, esp + 4 + 4 * i) for i in range(5)]
            root_key, subkey, options, access, out_handle = args
            status = fixture.get("open_status", 0)
            handle = fixture.get("handle", 0x13572468)
            if fixture.get("write_handle", True):
                w32(uc, out_handle, handle)
            events.append({
                "kind": "api", "name": api, "root": root_key,
                "subkey": cstr(uc, subkey), "options": options,
                "access": access, "status": status,
                "handle_out_written": fixture.get("write_handle", True),
            })
            api_return(esp, 5, status)
            return
        if api == "RegQueryValueExA":
            args = [u32(uc, esp + 4 + 4 * i) for i in range(6)]
            key, value, reserved, type_pointer, data, size_pointer = args
            input_size = u32(uc, size_pointer)
            payload = fixture.get("query_data", b"")
            status = fixture.get("query_status", 0)
            output_size = fixture.get("query_length", len(payload))
            copied = 0
            if fixture.get("write_query_data", True) and payload:
                copied = min(len(payload), input_size, max(0, output_size))
                if copied:
                    uc.mem_write(data, payload[:copied])
            if fixture.get("write_query_length", True):
                w32(uc, size_pointer, output_size)
            events.append({
                "kind": "api", "name": api, "key": key,
                "value": cstr(uc, value), "reserved": reserved,
                "type_is_null": type_pointer == 0,
                "data_target": (
                    "g_registry_config" if data == config_address else
                    "g_exe_b9e458" if data == path_address else "other"
                ),
                "size_in": input_size, "size_out": output_size,
                "status": status, "bytes_copied": copied,
            })
            api_return(esp, 6, status)
            return
        if api == "RegCloseKey":
            key = u32(uc, esp + 4)
            events.append({"kind": "api", "name": api, "key": key,
                           "status": fixture.get("close_status", 0)})
            api_return(esp, 1, fixture.get("close_status", 0))
            return
        if api == "SetCurrentDirectoryA":
            path = u32(uc, esp + 4)
            result = fixture.get("setcwd_result", 1)
            events.append({"kind": "api", "name": api,
                           "path": cstr(uc, path), "result": result})
            api_return(esp, 1, result)
            return
        if api == "RegCreateKeyExA":
            raise RuntimeError("Unexpected RegCreateKeyExA endpoint entered in read-only matrix")
        if api == "RegSetValueExA":
            raise RuntimeError("Unexpected RegSetValueExA endpoint entered in read-only matrix")
        raise RuntimeError(f"Unexpected synthetic API endpoint entered: {api}")

    uc.hook_add(UC_HOOK_CODE, hook)
    stack_top = STACK_BASE + 0x8000
    w32(uc, stack_top - 4, SENTINEL)
    uc.reg_write(UC_X86_REG_ESP, stack_top - 4)
    if kind == "original":
        original_rvas = {
            "path": 0x4a30, "config": 0x4ce0, "defaults": 0x4be0,
            "table": 0x7aa0, "apply": 0x4ac0,
        }
        start = BASE + original_rvas[entry]
    else:
        candidate_symbols = {
            "path": "_host_registry_read_path",
            "config": "_host_registry_read_config",
            "defaults": "_cleanup_native_4be0",
            "table": "_cleanup_native_7aa0",
            "apply": "_cleanup_native_4ac0",
        }
        start = funcs[candidate_symbols[entry]]
    uc.emu_start(start, SENTINEL, timeout=5_000_000, count=500_000)
    completion_eip = uc.reg_read(UC_X86_REG_EIP)
    assert completion_eip == SENTINEL, hex(completion_eip)

    table_a = bytes(uc.mem_read(table_a_address, 28 * 4))
    table_b = bytes(uc.mem_read(table_b_address, 28 * 4))
    audio = bytes(uc.mem_read(audio_address, 8))
    context_out = bytes(uc.mem_read(context + 0x0b, 5))
    config_out = bytes(uc.mem_read(config_address, 72))
    path_buffer = bytes(uc.mem_read(path_address, 0x400))
    query_events = [event for event in events if event.get("name") == "RegQueryValueExA"]
    return {
        "events": events,
        "returned_to_sentinel": completion_eip == SENTINEL,
        "config": config_out.hex(),
        "config_sha256": hashlib.sha256(config_out).hexdigest(),
        "table_a": table_a.hex(),
        "table_b": table_b.hex(),
        "table_a_sha256": hashlib.sha256(table_a).hexdigest(),
        "table_b_sha256": hashlib.sha256(table_b).hexdigest(),
        "audio_settings": audio.hex(),
        "startup_guard": u32(uc, guard_address),
        "context_bytes_0b_to_0f": context_out.hex(),
        "path_buffer_prefix": path_buffer[:64].hex(),
        "path_buffer_sha256": hashlib.sha256(path_buffer).hexdigest(),
        "path_reader_return": (
            uc.reg_read(UC_X86_REG_EAX) if entry == "path" else None
        ),
        "all_query_types_null": all(
            event.get("type_is_null", True) for event in query_events
        ),
    }


table_values = [0x08000003, 0x00000005, 0x80000007,
                0x10000001, 0x00000003, 0x00000001]
valid_table_config = config_blob(table_values=table_values)
expected_table_a, expected_table_b = expected_table_bytes(table_values)
scenarios = {
    "path_open_error": (
        "path", {"open_status": 2, "handle": 0x13572468}),
    "path_query_error_leaks_open_key": (
        "path", {"open_status": 0, "handle": 0x13572468,
                 "query_status": 2}),
    "path_success_close_then_setcwd": (
        "path", {"open_status": 0, "handle": 0x13572468,
                 "query_data": b"C:\\Hercules\\" + bytes([0]), "setcwd_result": 1}),
    "path_setcwd_failure_after_close": (
        "path", {"open_status": 0, "handle": 0x13572468,
                 "query_data": b"C:\\Hercules\\" + bytes([0]), "setcwd_result": 0}),
    "config_missing_key_guard0": ("config", {"open_status": 2, "handle": 0, "guard": 0}),
    "config_missing_key_guard1": ("config", {"open_status": 2, "handle": 0, "guard": 1}),
    "config_open_error_with_handle_valid": (
        "config", {"open_status": 2, "handle": 0x13572468,
                    "query_data": config_blob(), "guard": 1}),
    "config_query_error": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_status": 2, "query_data": b"bad", "query_length": 3}),
    "config_length_0x47": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(), "query_length": 0x47}),
    "config_length_0x49": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(), "query_length": 0x49}),
    "config_success_length_not_written": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(), "write_query_length": False}),
    "config_bad_checksum": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(stored="bad"), "guard": 1}),
    "config_level_low": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(level=1)}),
    "config_level_high": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(level=5), "guard": 1}),
    "config_control_high": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(control=3)}),
    "config_valid_lower_bounds": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(level=2, control=2), "guard": 1}),
    "config_valid_upper_level": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(level=4, control=0)}),
    "config_valid_checksum_sentinel": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": config_blob(level=3, control=2, stored="sentinel"),
                    "guard": 1}),
    "config_valid_popcount_tables": (
        "config", {"open_status": 0, "handle": 0x13572468,
                    "query_data": valid_table_config, "guard": 1}),
    "direct_defaults_guard0": ("defaults", {"guard": 0}),
    "direct_defaults_guard1": ("defaults", {"guard": 1}),
    "direct_table_population_grouping": (
        "table", {"config_seed": valid_table_config, "guard": 0}),
    "direct_apply_guard0": (
        "apply", {"config_seed": config_blob(table_values=table_values), "guard": 0}),
    "direct_apply_guard1": (
        "apply", {"config_seed": config_blob(table_values=table_values), "guard": 1}),
}

results = {}
failures = []
for label, (entry, fixture) in scenarios.items():
    original = run("original", entry, fixture)
    candidate = run("candidate", entry, fixture)
    equivalent = original == candidate
    if entry == "config":
        assert original["all_query_types_null"] and candidate["all_query_types_null"], label
    if entry == "defaults" or (
        entry == "config" and any(
            event.get("name") == "defaults" for event in original["events"]
        )
    ):
        assert bytes.fromhex(original["config"]) == expected_defaults(), label
    if entry == "table" or label == "config_valid_popcount_tables":
        assert bytes.fromhex(original["table_a"]) == expected_table_a, label
        assert bytes.fromhex(original["table_b"]) == expected_table_b, label
    if label == "config_missing_key_guard1":
        names = [event.get("name") for event in original["events"]]
        assert names.count("apply") == 2, names
        assert names.count("audio_store") == 2, names
    if label.startswith("config_bad_checksum") or label == "config_level_high":
        names = [event.get("name") for event in original["events"]]
        assert names.count("RegCloseKey") == 2, names
    if entry == "path":
        names = [event["name"] for event in original["events"]
                 if event.get("kind") == "api"]
        if label == "path_open_error":
            assert original["path_reader_return"] == 0
            assert names == ["RegOpenKeyExA"]
        if label == "path_query_error_leaks_open_key":
            assert original["path_reader_return"] == 0
            assert names == ["RegOpenKeyExA", "RegQueryValueExA"]
        if label == "path_success_close_then_setcwd":
            assert original["path_reader_return"] == 1
            assert names == ["RegOpenKeyExA", "RegQueryValueExA",
                             "RegCloseKey", "SetCurrentDirectoryA"]
        if label == "path_setcwd_failure_after_close":
            assert original["path_reader_return"] == 0
            assert names == ["RegOpenKeyExA", "RegQueryValueExA",
                             "RegCloseKey", "SetCurrentDirectoryA"]
    if not equivalent:
        failures.append(label)
    results[label] = {
        "entry": entry,
        "equivalent": equivalent,
        "original": original,
        "candidate": candidate,
        "original_api_order": [
            event["name"] for event in original["events"]
            if event.get("kind") == "api"
        ],
        "original_helper_order": [
            event["name"] for event in original["events"]
            if event.get("kind") == "helper"
        ],
        "close_count": sum(
            event.get("name") == "RegCloseKey" for event in original["events"]
        ),
        "query_type_null": original["all_query_types_null"],
    }

if len(results) != 24:
    raise AssertionError(f"The bounded scenario inventory changed: {len(results)} != 24")
if len(compile_commands) != 9:
    raise AssertionError(f"Provider inventory changed: {len(compile_commands)} != 9")
for row in startup_storage_prefixes:
    binding = candidate_symbol_bindings[row["symbol"]]
    address = int(binding["address"], 16)
    if binding["provider"] != "startup_storage" or not (CAND_BASE <= address < CAND_BASE + CAND_SIZE):
        raise RuntimeError(f"Owned startup cell did not resolve to its candidate COFF section: {row['symbol']} => {binding}")
if candidate_symbol_bindings["_g_host_engine_interface"] != {
        "address": hex(BASE + 0x361b8), "provider": "pinned-original-view"}:
    raise RuntimeError("The bounded host interface/context view changed")
for symbol_name, owner in {
    "_g_registry_config": "registry_storage",
    "_g_exe_79e438": "audio_storage",
    "_g_exe_79e43c": "audio_storage",
    "_g_registry_key": "literals",
    "_g_registry_value_name": "literals",
}.items():
    if candidate_symbol_bindings[symbol_name]["provider"] != owner:
        raise RuntimeError(f"Candidate shared provider changed for {symbol_name}")
if candidate_loader_verification_passes != 24:
    raise AssertionError(f"Startup provider initial mapping checks: {candidate_loader_verification_passes} != 24")
startup_storage_sections = [
    {"name": section["name"], "size": section["size"],
     "raw_data_size": len(section["data"]), "flags": hex(section["flags"])}
    for section in objects["startup_storage"].sections
    if section["name"] in (".data", ".bss")
]

function_map_path = args.original_root.resolve() / "work/luna_fatal_cleanup_chain_11/function_map.json"
function_map = json.loads(function_map_path.read_text(encoding="utf-8"))["functions"]
comparison_targets = {
    "host_registry_read_path": ("reader", 0x4a30),
    "host_registry_read_config": ("reader", 0x4ce0),
    "cleanup_native_1fb0": ("context", 0x1fb0),
    "cleanup_native_1fd0": ("context", 0x1fd0),
    "cleanup_native_4ac0": ("cluster", 0x4ac0),
    "cleanup_native_4be0": ("cluster", 0x4be0),
    "cleanup_native_4cc0": ("cleanup", 0x4cc0),
    "cleanup_native_7aa0": ("cluster", 0x7aa0),
    "cleanup_native_7b70": ("cluster", 0x7b70),
    "audio_native_fdd0": ("audio", 0xfdd0),
}
function_comparisons = {}
for symbol, (provider, rva) in comparison_targets.items():
    row = next(item for item in function_map if int(item["rva"], 16) == rva)
    obj = object_paths[provider]
    function_comparisons[symbol] = {
        "provider": provider,
        "rva": hex(rva),
        "size": row["size"],
        "oracle_span_sha256": row.get("sha256"),
        "raw": compare(obj, "_" + symbol, oracle, rva, row["size"]),
        "masked_diagnostic": masked_compare(obj, "_" + symbol, oracle, rva, row["size"]),
    }

def section_for(rva, size):
    for section in pe.sections:
        start = section["virtual_address"]
        end = start + section["virtual_size"]
        if start <= rva and rva + size <= end:
            file_backed = max(0, min(rva + size, start + section["raw_size"]) - rva)
            return {"name": section["name"], "file_backed_bytes": file_backed,
                    "loader_zero_fill_bytes": size - file_backed}
    return {"name": None, "file_backed_bytes": None, "loader_zero_fill_bytes": None}

def native_field(name, rva, size, interpretation, zero_fill=False, values=None):
    section = section_for(rva, size)
    if zero_fill:
        if section["loader_zero_fill_bytes"] != size:
            raise AssertionError((name, section))
        data = bytes(size)
    else:
        data = pe.read_rva(rva, size)
    row = {
        "rva": hex(rva), "va": hex(pe.image_base + rva), "size": size,
        "sha256": hashlib.sha256(data).hexdigest(), "initial_hex": data.hex(),
        "section": section, "interpretation": interpretation,
    }
    if values is not None:
        row["values"] = values
    return name, row

def native_literal(name, rva, interpretation):
    value = bytearray()
    for offset in range(512):
        char = pe.read_rva(rva + offset, 1)[0]
        value.append(char)
        if char == 0:
            break
    return native_field(name, rva, len(value), interpretation,
                        values={"ascii": bytes(value[:-1]).decode("ascii")})

defaults_data = pe.read_rva(0x36158, 36)
default_dwords = struct.unpack("<9I", defaults_data)
native_fields = dict([
    native_literal("path_value_name", 0x3617c, "NUL-terminated Path value literal"),
    native_literal("registry_key", 0x36184, "NUL-terminated registry subkey literal"),
    native_literal("config_value_name", 0x361b0, "NUL-terminated Config value literal"),
    native_field("config_default_cells", 0x36158, 36,
                 "Nine DWORD cells; defaults helper reads low WORD at DWORD stride",
                 values={"dwords": [hex(value) for value in default_dwords],
                         "low_words": [hex(value & 0xffff) for value in default_dwords]}),
    native_field("host_interface_context_pointer_cell", 0x361bc, 4,
                 "Host interface context_004 pointer cell",
                 values={"pointer_va": hex(struct.unpack("<I", pe.read_rva(0x361bc, 4))[0])}),
    native_field("startup_apply_guard", 0x51c450, 4,
                 "Four-byte startup guard prefix", zero_fill=True),
    native_field("config_value_table_prefix", 0x56e4f0, 112,
                 "28-DWORD table-A prefix", zero_fill=True),
    native_field("config_mask_table_prefix", 0x56e560, 112,
                 "28-DWORD table-B prefix", zero_fill=True),
    native_field("config_protocol_prefix", 0x79e858, 72,
                 "72-byte accessed Config prefix", zero_fill=True),
    native_field("audio_word_prefix", 0x79e438, 8,
                 "Two-WORD audio settings prefix", zero_fill=True),
    native_field("path_query_output_prefix", 0x79e458, 0x400,
                 "Path query output buffer prefix", zero_fill=True),
])

report = {
    "scope": "Original EXE bounded Config differential; not source identity or FUNCTION_MATCH acceptance.",
    "source_root": str(source_root),
    "oracle": str(oracle),
    "oracle_sha256": oracle_hash,
    "toolchain": {
        "id": toolchain["id"],
        "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
        "directory": str(tc),
        "compiler": str(tc / "bin/cl.exe"),
        "compiler_sha256": next(row["sha256"] for row in toolchain["files"]
                                 if row["path"] == "bin/cl.exe"),
        "pinned_manifest_files": len(toolchain["files"]),
    },
    "providers": compile_commands,
    "headers": include_hashes,
    "headers_per_provider": headers_per_provider,
    "preprocessor_receipts": preprocessor_receipts,
    "startup_storage_prefixes": startup_storage_prefixes,
    "startup_storage_total_initial_bytes": sum(row["size"] for row in startup_storage_prefixes),
    "startup_storage_coff_sections": startup_storage_sections,
    "candidate_loader_initial_prefix_checks": {
        "fields_per_candidate_run": len(startup_storage_prefixes),
        "bytes_per_candidate_run": sum(row["size"] for row in startup_storage_prefixes),
        "candidate_runs_checked": candidate_loader_verification_passes,
        "all_exact": True,
    },
    "candidate_symbol_bindings": candidate_symbol_bindings,
    "function_map": {
        "path": str(function_map_path),
        "sha256": hashlib.sha256(function_map_path.read_bytes()).hexdigest(),
    },
    "native_fields": native_fields,
    "function_comparisons": function_comparisons,
    "execution_scope": (
        "Only the bounded original helper/reader spans ran under Unicorn. Registry and "
        "directory APIs were synthetic. No real API, game process, DLL initializer, or "
        "original post-entry process execution occurred. Unused service endpoints trap."
    ),
    "synthetic_endpoints": {
        "handled": ["RegOpenKeyExA", "RegQueryValueExA", "RegCloseKey", "SetCurrentDirectoryA"],
        "mapped_but_fatal_if_entered": ["RegCreateKeyExA", "RegSetValueExA", "audio_native_fa10"],
        "imports_present_in_pinned_PE": sorted(imports),
    },
    "scenario_comparison": {
        "scope": "Config/path observable 72-byte config, 28-DWORD tables, audio words, context bytes, path buffer prefix, path return, and synthetic API/helper order; volatile void-return EAX residue is excluded.",
        "default_config_sha256": hashlib.sha256(expected_defaults()).hexdigest(),
        "table_boundary_values": [hex(value) for value in table_values],
        "scenario_count": len(results),
        "all_equivalent": not failures,
        "failures": failures,
        "scenarios": results,
    },
}
out = output_dir / "config_differential.json"
out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
print("report", out)
print("scenarios", len(results), "all_equivalent", not failures, "failures", failures)
for symbol, item in function_comparisons.items():
    raw = item["raw"]
    masked = item["masked_diagnostic"]
    print(symbol, "raw_exact", raw["exact"], "raw_diff", raw["different_bytes"],
          "masked_equal", masked["masked_equal"],
          "masked_diff", masked["differing_bytes"])
if failures:
    raise SystemExit(1)
