from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from pe import PE

LANE = ROOT / "work/eng1_behavioral/differential/actor-list-success"
OUT = LANE
OUT.mkdir(parents=True, exist_ok=True)
ORACLE = ROOT / "assets/pc/HERCULES/ENG1.DLL"
ORACLE_SHA256 = "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757"
PREVIOUS_REPORT = ROOT / "evidence/eng1_actor_list_producer_2b130.json"
PREVIOUS_REPORT_SHA256 = hashlib.sha256(PREVIOUS_REPORT.read_bytes()).hexdigest()
FIXTURE = ROOT / "tests/eng1_actor_list_success_differential.c"
PRODUCER = ROOT / "src/eng1/actor_list_producer_2b130.c"
REGISTRY = ROOT / "src/shared/action_registry.c"
FACTORIES = ROOT / "src/shared/actor_factories.c"
POOLS = ROOT / "src/shared/actor_pool_reset_repaired.c"
SPAWN_SERVICES = ROOT / "src/eng1/spawn_services.c"
TABLE_GUARDS = ROOT / "src/eng1/worklist_read_guards.c"
INCOMPLETE = ROOT / "src/eng1/incomplete.c"
EXECUTABLE = OUT / "neutralfixture.exe"
PREDICTION = ROOT / "evidence/eng1_actor_list_success_2b130.json"
NATIVE_RVA = 0x2B130
NATIVE_SPAN = 0x1F0

REFERENCES = [
    ROOT / "src/eng1/interpreters.h",
    ROOT / "src/eng1/secondary_records.h",
    ROOT / "src/eng1/resource_runtime_data.h",
    ROOT / "src/eng1/worklist_factory_types.h",
    ROOT / "src/eng1/forest_actor_init.h",
    ROOT / "src/eng1/worklist_read_guards.h",
    ROOT / "src/eng1/incomplete.h",
    ROOT / "src/eng1/row0_layouts.h",
    ROOT / "evidence/actor_pool_reset_region.json",
    ROOT / "evidence/actor_pools_region.json",
    ROOT / "evidence/actor_factories_region.json",
    ROOT / "evidence/action_registry_region.json",
    ROOT / "evidence/eng1_actor_list_producer_2b130.json",
    ROOT / "toolchains/manifest.json",
]
INPUTS = [
    FIXTURE, Path(__file__).resolve(), PREDICTION, PRODUCER, REGISTRY, FACTORIES, POOLS,
    SPAWN_SERVICES, TABLE_GUARDS, INCOMPLETE, PREVIOUS_REPORT, ORACLE,
] + REFERENCES


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


require(digest(ORACLE) == ORACLE_SHA256, "ENG1 oracle hash changed")
require(verify_toolchain(DEFAULT_TOOLCHAIN) == "msvc5_rtm",
        "pinned MSVC 5.00 RTM toolchain is required")
require(DEFAULT_TOOLCHAIN == Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"),
    "toolchain path changed")
native_bytes = PE(ORACLE).read_rva(NATIVE_RVA, NATIVE_SPAN)
require(native_bytes[-6:] == b"\x90" * 6,
        "0x2b130 diagnostic span no longer ends in reviewed alignment NOPs")
before = {str(path.relative_to(ROOT)).replace("\\", "/"): digest(path)
          for path in INPUTS}

env = os.environ.copy()
for key in ("CL", "_CL_", "LINK", "_LINK_"):
    env.pop(key, None)
env.update(
    PATH=str(DEFAULT_TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", ""),
    INCLUDE=str(DEFAULT_TOOLCHAIN / "include"),
    LIB=str(DEFAULT_TOOLCHAIN / "lib"),
)
for path in (EXECUTABLE, OUT / "eng1_actor_list_success_differential.obj", OUT / "actor_list_producer_2b130.obj",
             OUT / "action_registry.obj", OUT / "actor_factories.obj",
             OUT / "actor_pool_reset_repaired.obj", OUT / "spawn_services.obj",
             OUT / "worklist_read_guards.obj", OUT / "incomplete.obj"):
    path.unlink(missing_ok=True)

command = [
    str(DEFAULT_TOOLCHAIN / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/MT",
    "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS", "/DENG1_WORKLIST_GUARD_FIXTURE",
    "/I" + str(ROOT / "src/eng1"),
    "/I" + str(ROOT / "src/shared"),
    "/Fe" + str(EXECUTABLE),
    str(FIXTURE), str(PRODUCER), str(REGISTRY), str(FACTORIES), str(POOLS),
    str(SPAWN_SERVICES), str(TABLE_GUARDS), str(INCOMPLETE),
    "/link", "/INCREMENTAL:NO", "/OPT:REF", "kernel32.lib",
]
build = subprocess.run(command, cwd=OUT, env=env, capture_output=True,
                       text=True, timeout=120)
(OUT / "compile.log").write_bytes(
    (build.stdout + build.stderr).encode("utf-8"))
require(build.returncode == 0,
        f"VC5 compile/link failed: {build.stdout}{build.stderr}")

def run_case(mode: str, kind: int) -> dict:
    args = [str(EXECUTABLE), str(ORACLE), mode, hex(kind)]
    process = subprocess.run(args, cwd=OUT, env=env, capture_output=True,
                             text=True, timeout=60)
    if process.returncode:
        raise RuntimeError(
            f"fixture failed ({process.returncode}) for {mode}/{kind:#06x}: "
            f"{process.stdout}{process.stderr}"
        )
    flattened = process.stdout.replace("\r", "").replace("\n", " ").strip()
    row = dict(re.findall(r"([a-z_]+)=([^ ]+)", flattened))
    names = {
        "case": "factory", "result": "result", "callbacks": "callbacks",
        "state": "state", "factory_step": "factory_step", "count": "count", "owner": "owner",
        "pool_cursor": "pool_cursor", "remove": "remove_calls",
        "reset": "reset_endpoint_calls", "chain": "chain_endpoint_calls",
        "coordinates": "coordinates", "child_state": "child_state",
        "child_sprite": "child_sprite", "child_kind": "child_kind",
        "records_ptr": "records_ptr", "actor": "actor_hex", "pool": "pool_hex",
        "group": "group_hex", "old_records": "old_records_hex",
        "new_records": "new_records_hex", "checks": "fixture_assertions_passed",
    }
    if row.get("mode") != mode or any(key not in row for key in names):
        missing = [key for key in names if key not in row]
        raise RuntimeError(f"unrecognized fixture output; mode={row.get('mode')!r}, missing={missing}: {process.stdout!r}")
    row = {target: row[source] for source, target in names.items()} | {"mode": mode}
    expected_lengths = {"actor_hex": 0x124 * 2, "group_hex": 0x40 * 2,
                        "old_records_hex": 24 * 2, "new_records_hex": 24 * 2}
    if any(len(row[key]) != length for key, length in expected_lengths.items()):
        raise RuntimeError(f"fixture output has wrong byte-span lengths: {process.stdout!r}")
    for name in ("result", "callbacks", "count", "pool_cursor", "remove_calls",
                 "reset_endpoint_calls", "chain_endpoint_calls", "fixture_assertions_passed"):
        row[name] = int(row[name])
    row["state"] = int(row["state"], 16)
    row["factory_step"] = int(row["factory_step"], 16)
    row["child_state"] = int(row["child_state"], 16)
    row["child_sprite"] = int(row["child_sprite"], 16)
    row["child_kind"] = int(row["child_kind"], 16)
    row["stdout"] = process.stdout
    row["command"] = args
    return row


cases = []
expected = {
    0x0001: ("normal", "80018001_ffff0003_0002fff0"),
    0x4001: ("middle", "80018001_ffff0003_0002fff0"),
    0x2001: ("alternate", "80018001_ffff0003_0002fff0"),
}
for kind, (factory, coordinates) in expected.items():
    native = run_case("native", kind)
    source = run_case("source", kind)
    require(native["factory"] == factory and source["factory"] == factory,
            f"factory category mismatch for {kind:#06x}")
    for mode, result in (("native", native), ("source", source)):
        require(result["result"] == 1, f"{mode} did not terminate on swapped sentinel")
        require(result["callbacks"] == 1, f"{mode} callback did not run exactly once")
        require(result["state"] == 0x5A3C and result["child_state"] == 0x5A3C,
                f"{mode} did not transfer the live state WORD")
        require(result["factory_step"] == 0,
                f"{mode} factory/init did not leave step WORD zero before callback")
        require(result["count"] == 3, f"{mode} did not reload callback-mutated count")
        require(int(result["owner"], 16) != 0,
                f"{mode} did not leave the acquired group installed")
        require(result["coordinates"] == coordinates,
                f"{mode} coordinate wrap mismatch: {result['coordinates']}")
        require(result["child_sprite"] == 0x2468,
                f"{mode} did not overwrite callback/factory step value with cached sprite")
        require(result["child_kind"] == kind,
                f"{mode} factory stored a different kind {result['child_kind']:#06x}")
        require(result["pool_cursor"] == 0,
                f"{mode} selected pool cursor changed unexpectedly")
        require(result["remove_calls"] == result["reset_endpoint_calls"] ==
                result["chain_endpoint_calls"] == 0,
                f"{mode} reached an unused cleanup/chain endpoint")
        require(result["fixture_assertions_passed"] == 1,
                f"{mode} fixture-side mutation invariants failed")

    compared = [
        "factory", "result", "callbacks", "state", "factory_step", "count", "owner",
        "pool_cursor", "coordinates", "child_state", "child_sprite",
        "child_kind", "records_ptr", "actor_hex", "pool_hex", "group_hex",
        "old_records_hex", "new_records_hex",
    ]
    differences = [key for key in compared if native[key] != source[key]]
    require(not differences,
            f"native/source differ for {kind:#06x} at {differences}")
    old_records = bytes.fromhex(native["old_records_hex"])
    require(old_records[8:10] == b"\x57\x13",
            "callback did not mutate original record sprite after producer cached it")
    require(bytes.fromhex(native["new_records_hex"])[18:20] == b"\x00\x00",
            "replacement list no longer has the terminating second record")
    cases.append({
        "kind": hex(kind),
        "factory": factory,
        "native_source_equal_fields": compared,
        "checks_per_native_source_pair": len(compared) + 15,
        "native": native,
        "source": source,
        "callback_mutation_discriminators": {
            "live_group_count": "callback overwrote count with 2; producer appended at 3",
            "next_kind_and_record_base": "callback replaced actor+0x118; stale old row1 is kind 0x2002, live replacement row1 is sentinel",
            "cached_sprite": "callback overwrote current record sprite with 0x1357 and child+0x34 with 0xfedc; post-callback child+0x34 remains cached 0x2468",
            "factory_initialized_step": "the actual factory/initializer left child+0x34 zero at callback entry; after callback wrote 0xfedc, the producer restored the pre-factory cached 0x2468 sprite",
            "state_reload": "state WORD 0x5a3c is observed at child+0x2a before callback",
        },
    })

after = {str(path.relative_to(ROOT)).replace("\\", "/"): digest(path)
         for path in INPUTS}
require(before == after, "a hashed input changed during the fixture")
require(digest(PREVIOUS_REPORT) == PREVIOUS_REPORT_SHA256,
        "the earlier accepted five-case receipt was overwritten")

manifest = json.loads((ROOT / "toolchains/manifest.json").read_text(encoding="utf-8"))
toolchain = next(row for row in manifest["toolchains"] if row["id"] == "msvc5_rtm")
compiler = next(row for row in toolchain["files"] if row["path"] == "bin/cl.exe")
report = {
    "scope": (
        "Fresh pinned VC5 x86 native/source comparison for successful ENG1 RVA "
        "0x2b130 factory/callback paths. The native side enters the read-only "
        "oracle and executes its real acquire, normal/middle/alternate factory, "
        "allocator and initializer bodies. The source side calls the canonical "
        "ordinary-C 0x2b130 producer plus the real action_registry, actor-pool "
        "initializer/allocator, actor_factories, spawn_services middle factory, "
        "and bounded worklist-read guard providers. Synthetic pools, asset/callback "
        "rows, registry group, and a test callback are controlled fixture state; "
        "no production capacity or callback-body claim is made."
    ),
    "oracle": {
        "path": "assets/pc/HERCULES/ENG1.DLL",
        "sha256": ORACLE_SHA256,
        "file_unchanged_before_after": digest(ORACLE) == ORACLE_SHA256,
        "entry_rva": hex(NATIVE_RVA),
        "diagnostic_span_bytes": NATIVE_SPAN,
        "diagnostic_span_sha256": hashlib.sha256(native_bytes).hexdigest(),
        "extent_caveat": "0x1f0-byte diagnostic inventory includes six trailing alignment NOPs and is not an ownership claim.",
        "process_local_data_overrides": "Only measured loader data cells required for the fixture were redirected to process-local actor pools, asset/callback rows and the single test group. Every written DLL cell was snapshotted and restored before FreeLibrary; code bytes were never written.",
    },
    "toolchain": {
        "id": "msvc5_rtm",
        "root": str(DEFAULT_TOOLCHAIN),
        "driver_version": compiler.get("file_version"),
        "driver_sha256": compiler.get("sha256"),
        "command": command,
        "compile_exit_code": build.returncode,
        "executable_sha256": digest(EXECUTABLE),
        "object_sha256": {
            name: digest(OUT / name) for name in (
                "eng1_actor_list_success_differential.obj", "actor_list_producer_2b130.obj",
                "action_registry.obj", "actor_factories.obj",
                "actor_pool_reset_repaired.obj", "spawn_services.obj",
                "worklist_read_guards.obj", "incomplete.obj",
            )
        },
    },
    "inputs_sha256_before": before,
    "inputs_sha256_after": after,
    "prior_receipt": {
        "path": "evidence/eng1_actor_list_producer_2b130.json",
        "sha256_before_after": PREVIOUS_REPORT_SHA256,
        "preserved": True,
    },
    "prediction": {
        "path": "evidence/eng1_actor_list_success_2b130.json",
        "sha256": digest(PREDICTION),
        "falsifier": json.loads(PREDICTION.read_text(encoding="utf-8"))["private_prediction"]["falsifier"],
    },
    "source_owners_exercised": {
        "producer": {"path": "src/eng1/actor_list_producer_2b130.c", "sha256": digest(PRODUCER)},
        "registry_helpers": {"path": "src/shared/action_registry.c", "sha256": digest(REGISTRY)},
        "pool_allocator_initializer": {"path": "src/shared/actor_pool_reset_repaired.c", "sha256": digest(POOLS)},
        "normal_and_alternate_factories": {"path": "src/shared/actor_factories.c", "sha256": digest(FACTORIES)},
        "middle_factory": {"path": "src/eng1/spawn_services.c", "sha256": digest(SPAWN_SERVICES), "function": "spawn_middle_277e0"},
        "bounded_table_guard": {"path": "src/eng1/worklist_read_guards.c", "sha256": digest(TABLE_GUARDS), "build_define": "ENG1_WORKLIST_GUARD_FIXTURE"},
    },
    "cases": cases,
    "checks": sum(row["checks_per_native_source_pair"] for row in cases),
    "callback_and_fixture_endpoints": {
        "test_callback": "Same host executable callback is installed in native and source test tables. It mutates group count to 2, original record sprite to 0x1357, child+0x34 to 0xfedc, and parent+0x118 to a replacement list with a zero sentinel at index 1.",
        "unused_link_stubs": ["remove_actor_variant", "remove_actor", "remove_middle_actor", "release_actor", "clear_actor", "append_chain"],
        "unused_link_stub_calls": 0,
    },
    "limits": [
        "Three controlled kinds exercise only one bounded actor slot and one callback row per family; no production pool or table capacity is inferred.",
        "Test callback behavior is a controlled live-mutation probe, not a claim about the semantics of any production callback body.",
        "Fixture group backing is 64 bytes to safely observe the native post-callback count=2 then entry[3] write. This does not enlarge or assert the production allocator request; the existing allocation-limit quirk remains unresolved.",
        "Asset image/frame/step pointers address shared marker bytes. Their pointee behavior is not tested.",
        "The source path uses the already integrated canonical provider and existing real helpers; it does not prove original source/TU identity, strict function equality, complete startup/gameplay equivalence, or dynamic group/pool capacities.",
        "Callback tables and assets are synthetic fixture data registered with the actual bounded source guard; this does not validate all table entries or production callback semantics.",
    ],
    "result": "PASS",
}
report_path = OUT / "report.json"
report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
print(json.dumps({"result": report["result"], "report": str(report_path),
                  "cases": len(cases), "checks": report["checks"],
                  "executable_sha256": report["toolchain"]["executable_sha256"]}, indent=2))
