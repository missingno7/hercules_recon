"""Hash-bound ENG1 host integration preparation and one-shot probe."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid

ROOT = next(parent for parent in Path(__file__).resolve().parents
            if (parent / ".git").exists() and (parent / "AGENTS.md").is_file())
sys.path.insert(0, str(ROOT / "scripts"))

import title_integration as title  # Reuse its reviewed installer/no-CD identities.
from match import verify_toolchain
from pe import PE
from coff import COFF
from title_link import read_map

PROBE_SOURCE = ROOT / "tests/eng1_host_probe.c"
PROBE_BUILD = ROOT / "work/eng1_behavioral/integration/probe_build"
PROBE_EXE = PROBE_BUILD / "eng1_host_probe.exe"
PROBE_REPORT = PROBE_BUILD / "build_report.json"
LANE = ROOT / "work/eng1_behavioral/integration"
DEFAULT_RUNTIME = LANE / "runtime"
DEFAULT_BUILD_REPORT = ROOT / "work/eng1_behavioral/build/report.json"

SOURCE_TITLE_SHA256 = "0dde33225d04557aa81e3870674ea9b08edb7faa38ef3f6db978e08fe54ab775"
FORBIDDEN_IMPORTS = {"ENG1.DLL", "ENG3.DLL", "TITLE.DLL"}
NOCD_OFFSET = title.NOCD_OFFSET
NOCD_BEFORE = 0x53
NOCD_AFTER = 0xC3


def digest(data):
    return hashlib.sha256(data).hexdigest()


def sha256(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def under(path, parent):
    return Path(path).resolve().is_relative_to(Path(parent).resolve())


def check_runtime(runtime):
    runtime = Path(runtime).resolve()
    if not under(runtime, LANE) or under(runtime, title.INSTALL):
        raise ValueError("Runtime must stay inside work/eng1_behavioral/integration")
    return runtime


def verified_source_title():
    report, dll, actual = title.load_report()
    if actual != SOURCE_TITLE_SHA256:
        raise ValueError("The stable source TITLE.DLL identity changed")
    return report, dll, actual


def load_eng1_report(report_path=DEFAULT_BUILD_REPORT):
    report_path = Path(report_path).resolve()
    if not under(report_path, ROOT / "work") or not report_path.is_file():
        raise ValueError("ENG1 build report must be an existing file under work/")
    report = json.loads(report_path.read_text(encoding="utf-8"))
    if report.get("link_exit_code") != 0 or report.get("architecture") != "x86 PE32":
        raise ValueError("ENG1 report does not describe a successful x86 PE32 link")
    exports = [{"name": row.get("name"), "ordinal": row.get("ordinal")}
               for row in report.get("exports", [])]
    if exports != [{"name": "PC_DLLEngineMain", "ordinal": 1}]:
        raise ValueError("ENG1 must export only PC_DLLEngineMain at ordinal 1")
    imports = [row.get("dll", "").upper() for row in report.get("imports", [])]
    forbidden = sorted(FORBIDDEN_IMPORTS.intersection(imports))
    if forbidden:
        raise ValueError("ENG1 imports original engine modules: " + ", ".join(forbidden))
    dll = Path(report["dll"])
    dll = (ROOT / dll).resolve() if not dll.is_absolute() else dll.resolve()
    if not under(dll, ROOT / "work") or not dll.is_file():
        raise ValueError("Reported ENG1.DLL must exist under work/")
    if sha256(dll) != report.get("dll_sha256"):
        raise ValueError("ENG1.DLL differs from its report hash")
    if report.get("dll_sha256") == title.ORIGINAL_SHA256["ENG1.DLL"]:
        raise ValueError("Refusing the unmodified original ENG1.DLL")
    image = PE(dll)
    metadata = image.metadata()
    if metadata.get("machine") != 0x14C:
        raise ValueError("Reported ENG1.DLL is not x86 PE32")
    if [{"name": e.get("name"), "ordinal": e.get("ordinal")}
        for e in metadata.get("exports", [])] != exports:
        raise ValueError("ENG1 PE exports differ from its report")
    if sorted(row.get("dll", "").upper() for row in metadata.get("imports", [])) != sorted(imports):
        raise ValueError("ENG1 PE imports differ from its report")

    recorded = dict(report.get("context_sha256", {}))
    recorded["recovery.json"] = report.get("recovery_sha256")
    recorded["evidence/eng1_behavioral_build.json"] = report.get("config_sha256")
    if not recorded.get("recovery.json") or not recorded.get("evidence/eng1_behavioral_build.json"):
        raise ValueError("ENG1 report lacks recovery/configuration input hashes")
    for relative, expected in recorded.items():
        source = (ROOT / relative).resolve()
        if not under(source, ROOT) or not source.is_file() or sha256(source) != expected:
            raise ValueError("ENG1 build context changed: " + relative)

    map_path = Path(report["link_map"])
    map_path = (ROOT / map_path).resolve() if not map_path.is_absolute() else map_path.resolve()
    if not under(map_path, ROOT / "work") or not map_path.is_file():
        raise ValueError("ENG1 map must exist under work/")
    if sha256(map_path) != report.get("link_map_sha256"):
        raise ValueError("ENG1 map differs from its report hash")
    symbols = read_map(map_path)

    def symbol_address(*names):
        for name in names:
            if name in symbols:
                return symbols[name]
        raise ValueError("ENG1 map lacks symbol: " + " / ".join(names))

    entry_address = symbol_address("_PC_DLLEngineMain", "PC_DLLEngineMain")
    entry_rva = entry_address - image.image_base
    export_rva = report["exports"][0].get("rva")
    if isinstance(export_rva, str):
        export_rva = int(export_rva, 0)
    if entry_rva != export_rva:
        raise ValueError("ENG1 link map entry address disagrees with the sole export")
    interface_address = symbol_address("_g_engine_interface", "g_engine_interface")
    first_use_address = symbol_address("_g_eng1_entry_first_use_5b648",
                                       "g_eng1_entry_first_use_5b648")
    interface_rva = interface_address - image.image_base
    first_use_rva = first_use_address - image.image_base
    image_size = image.metadata().get("image_size", 0)
    if not (0 <= interface_rva <= image_size - 1000 and
            0 <= first_use_rva <= image_size - 4):
        raise ValueError("ENG1 entry/interface data symbols fall outside the linked image")
    observed_cells = {
        'g_resource_root': 4, 'g_root_count_70ed0': 1,
        'g_active_category_70cfe': 1, 'g_active_row_70d01': 1,
        'g_group_count_70ece': 1, 'g_group_flag_70ecf': 1,
        'g_normal_actor_pool': 4, 'g_middle_actor_pool': 4,
        'g_alternate_actor_pool': 4, 'g_configuration_word_713b4': 2,
        'g_configuration_word_71432': 2, 'g_configuration_word_713ba': 2,
        'g_configuration_word_71380': 2, 'g_frame_transition_flag_5ce48': 1,
        'g_header_pointer_70d48': 4, 'g_frame_global_734f4': 4,
    }
    state_cells = []
    for name, width in observed_cells.items():
        rva = symbol_address('_' + name, name) - image.image_base
        if not 0 <= rva <= image_size - width:
            raise ValueError('Observed source state lies outside the linked image: ' + name)
        state_cells.append(dict(name=name, rva=rva, width=width))
    rows_rva = symbol_address('_g_root_group_rows_70d5c') - image.image_base
    for index in range(4):
        state_cells.append(dict(name='root_group_row_' + str(index),
                                rva=rows_rva + index * 4, width=4))
    return {"report": report, "report_path": report_path, "dll": dll,
            "dll_sha256": report["dll_sha256"], "map_path": map_path,
            "map_sha256": report["link_map_sha256"], "image": image,
            "entry_rva": entry_rva, "interface_rva": interface_rva,
            "first_use_rva": first_use_rva, 'state_cells': state_cells}


def original_install_inputs():
    names = ("ENG1.DLL", "ENG3.DLL", "HERCULES.EXE", "HERCULES.FS",
             "HERCULES.GID", "HERCULES.HLP", "TITLE.DLL")
    result = {}
    for name in names:
        source = title.INSTALL / name
        expected = title.ORIGINAL_SHA256[name]
        if not source.is_file() or sha256(source) != expected:
            raise ValueError("Original installer identity changed: " + str(source))
        result[name] = expected
    return result


def expected_runtime_hashes():
    return {
        "HERCULES.EXE": title.NOCD_EXE_SHA256,
        "ENG3.DLL": title.ORIGINAL_SHA256["ENG3.DLL"],
        "HERCULES.FS": title.ORIGINAL_SHA256["HERCULES.FS"],
        "HERCULES.GID": title.ORIGINAL_SHA256["HERCULES.GID"],
        "HERCULES.HLP": title.ORIGINAL_SHA256["HERCULES.HLP"],
        "TITLE.DLL": SOURCE_TITLE_SHA256,
    }


def read_manifest(runtime):
    path = runtime / "runtime_manifest.json"
    if not path.is_file():
        raise ValueError("Prepare the ENG1 runtime first")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("original_install_sha256") != original_install_inputs():
        raise ValueError("Runtime manifest is bound to different installer files")
    return manifest


def prepare(runtime=DEFAULT_RUNTIME):
    runtime = check_runtime(runtime)
    source_hashes = original_install_inputs()
    _, source_title, source_title_hash = verified_source_title()
    runtime.mkdir(parents=True, exist_ok=True)
    expected = expected_runtime_hashes()
    allowed = {name.upper() for name in expected} | {"ENG1.DLL", "RUNTIME_MANIFEST.JSON"}
    unknown = sorted(p.name for p in runtime.iterdir() if p.name.upper() not in allowed)
    if unknown:
        raise ValueError("Refusing runtime with unknown files: " + ", ".join(unknown))

    manifest_path = runtime / "runtime_manifest.json"
    manifest = None
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if manifest.get("original_install_sha256") != source_hashes:
            raise ValueError("Existing runtime manifest has different installer inputs")
        if manifest.get("host_sha256") != title.NOCD_EXE_SHA256:
            raise ValueError("Existing runtime manifest has a different host variant")
    existing_eng1 = runtime / "ENG1.DLL"
    if existing_eng1.exists():
        current = sha256(existing_eng1)
        if not manifest or manifest.get("runtime_eng1_sha256") != current:
            raise ValueError("Refusing an unrecorded ENG1.DLL in the runtime")
    for name, wanted in expected.items():
        destination = runtime / name
        if destination.exists():
            if not destination.is_file() or sha256(destination) != wanted:
                raise ValueError("Refusing to overwrite a nonmatching runtime file: " + str(destination))
            continue
        if name == "HERCULES.EXE":
            source = title.INSTALL / name
            shutil.copy2(source, destination)
            data = bytearray(destination.read_bytes())
            if len(data) <= NOCD_OFFSET or data[NOCD_OFFSET] != NOCD_BEFORE:
                raise ValueError("Unexpected byte at the reviewed no-CD site")
            data[NOCD_OFFSET] = NOCD_AFTER
            destination.write_bytes(data)
        elif name == "TITLE.DLL":
            shutil.copy2(source_title, destination)
        else:
            shutil.copy2(title.INSTALL / name, destination)
        if sha256(destination) != wanted:
            raise ValueError("Staged runtime hash mismatch: " + name)

    if manifest is None:
        manifest = {
            "scope": "Disposable integration runtime; staged only, no game launch implied.",
            "source_install": str(title.INSTALL.relative_to(ROOT)),
            "original_install_sha256": source_hashes,
            "host_sha256": title.NOCD_EXE_SHA256,
            "host_variant": "no_cd_runtime_copy",
            "post_build_patch": {"scope": "Runtime EXE copy only", "offset": hex(NOCD_OFFSET),
                                 "before": "53", "after": "c3",
                                 "original_sha256": source_hashes["HERCULES.EXE"],
                                 "runtime_sha256": title.NOCD_EXE_SHA256},
            "original_eng1_excluded_sha256": source_hashes["ENG1.DLL"],
            "original_title_excluded_sha256": source_hashes["TITLE.DLL"],
            "runtime_title_sha256": source_title_hash,
            "source_title_build_report": "work/title_behavioral/build/report.json",
            "runtime_eng1_sha256": None,
            "eng1_build_report": None,
            "game_launched": False,
        }
    manifest["runtime_title_sha256"] = source_title_hash
    manifest["runtime_title_kind"] = "verified_source_build"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    staged_hashes = {name: sha256(runtime / name) for name in expected}
    probe_build = json.loads(PROBE_REPORT.read_text(encoding="utf-8")) if PROBE_REPORT.is_file() else None
    preparation = {
        "scope": "Private ENG1 integration preparation; not a successful ENG1 link or a game run.",
        "integration_script": str(Path(__file__).resolve().relative_to(ROOT)),
        "integration_script_sha256": sha256(Path(__file__)),
        "probe_source": str(PROBE_SOURCE.resolve().relative_to(ROOT)),
        "probe_source_sha256": sha256(PROBE_SOURCE),
        "probe_build_report": str(PROBE_REPORT.resolve().relative_to(ROOT)) if probe_build else None,
        "probe_build_report_sha256": sha256(PROBE_REPORT) if probe_build else None,
        "probe_exe_sha256": probe_build.get("exe_sha256") if probe_build else None,
        "probe_architecture": probe_build.get("architecture") if probe_build else None,
        "probe_was_executed": False,
        "source_title_build_report": "work/title_behavioral/build/report.json",
        "source_title_build_report_sha256": sha256(ROOT / "work/title_behavioral/build/report.json"),
        "source_title_sha256": source_title_hash,
        "original_installer_sha256": source_hashes,
        "runtime_file_sha256": staged_hashes,
        "runtime_host_sha256": title.NOCD_EXE_SHA256,
        "runtime_no_cd_patch": {"offset": hex(NOCD_OFFSET), "before": "53", "after": "c3"},
        "original_eng1_attached": False,
        "behavioral_eng1_attached": bool(manifest.get("runtime_eng1_sha256")),
        "window_contract": {"requested_fullscreen_va": hex(REQUESTED_MODE_ADDRESS),
                            "current_mode_va": hex(CURRENT_MODE_ADDRESS),
                            "alt_enter_down_lparam": "0x201c0001",
                            "alt_enter_up_lparam": "0xe01c0001",
                            "resize_menu_command": 32801,
                            "windowed_client_size": [320, 240]},
        "game_launched": False,
    }
    (LANE / "preparation_report.json").write_text(
        json.dumps(preparation, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"runtime": str(runtime), "host_sha256": title.NOCD_EXE_SHA256,
                      "eng3_sha256": expected["ENG3.DLL"],
                      "title_sha256": source_title_hash, "eng1_attached": (runtime / "ENG1.DLL").exists(),
                      "game_launched": False}, indent=2))


def attach(runtime=DEFAULT_RUNTIME, report_path=DEFAULT_BUILD_REPORT):
    runtime = check_runtime(runtime)
    inspect(runtime)
    manifest = read_manifest(runtime)
    build = load_eng1_report(report_path)
    destination = runtime / "ENG1.DLL"
    if destination.exists():
        current = sha256(destination)
        if current != build["dll_sha256"]:
            if current != manifest.get("runtime_eng1_sha256"):
                raise ValueError("Refusing to overwrite an unrecorded ENG1.DLL")
            raise ValueError("A different recorded ENG1 build is attached; use a fresh runtime directory")
    else:
        shutil.copy2(build["dll"], destination)
    if sha256(destination) != build["dll_sha256"]:
        raise ValueError("Copied behavioral ENG1.DLL hash mismatch")
    manifest.update({"runtime_eng1_sha256": build["dll_sha256"],
                     "eng1_build_report": str(Path(report_path).resolve().relative_to(ROOT)),
                     "eng1_map_sha256": build["map_sha256"]})
    (runtime / "runtime_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"runtime": str(runtime), "eng1_sha256": build["dll_sha256"],
                      "entry_rva": hex(build["entry_rva"]), "game_launched": False}, indent=2))


def inspect(runtime=DEFAULT_RUNTIME):
    runtime = check_runtime(runtime)
    manifest = read_manifest(runtime)
    expected = expected_runtime_hashes()
    file_rows = []
    for name, wanted in expected.items():
        path = runtime / name
        if not path.is_file() or sha256(path) != wanted:
            raise ValueError("Runtime input is absent or changed: " + name)
        file_rows.append({"name": name, "sha256": wanted})
    eng1 = runtime / "ENG1.DLL"
    if eng1.exists():
        current = sha256(eng1)
        if current != manifest.get("runtime_eng1_sha256"):
            raise ValueError("Runtime ENG1.DLL differs from its manifest")
        report_path = ROOT / manifest["eng1_build_report"]
        build = load_eng1_report(report_path)
        if current != build["dll_sha256"]:
            raise ValueError("Attached ENG1 differs from its current build report")
        file_rows.append({"name": "ENG1.DLL", "sha256": current})
    else:
        if manifest.get("runtime_eng1_sha256"):
            raise ValueError("Manifest records an ENG1 DLL absent from the runtime")
    allowed = {row["name"].upper() for row in file_rows} | {"RUNTIME_MANIFEST.JSON"}
    unknown = sorted(p.name for p in runtime.iterdir() if p.name.upper() not in allowed)
    if unknown:
        raise ValueError("Runtime contains unrecorded files: " + ", ".join(unknown))
    result = {"runtime": str(runtime), "files": file_rows,
              "eng1_attached": eng1.is_file(),
              "source_title_sha256": manifest["runtime_title_sha256"],
              "host_sha256": manifest["host_sha256"],
              "window_mode_contract": {"requested_fullscreen_va": hex(REQUESTED_MODE_ADDRESS),
                                        "current_mode_va": hex(CURRENT_MODE_ADDRESS),
                                        "alt_enter_down_lparam": "0x201c0001",
                                        "alt_enter_up_lparam": "0xe01c0001",
                                        "menu_command": 32801,
                                        "required_client_size": [320, 240]},
              "game_launched": bool(manifest.get("game_launched", False))}
    print(json.dumps(result, indent=2))
    return result


REQUESTED_MODE_ADDRESS = 0x0096E3D0
CURRENT_MODE_ADDRESS = 0x00BA020C


def build_probe():
    if not PROBE_SOURCE.is_file():
        raise ValueError("Probe source is missing: " + str(PROBE_SOURCE))
    toolchain = verify_toolchain(title.TOOLCHAIN)
    PROBE_BUILD.mkdir(parents=True, exist_ok=True)
    obj = PROBE_BUILD / "eng1_host_probe.obj"
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env.update(PATH=str(title.TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", ""),
               INCLUDE=str(title.TOOLCHAIN / "include"),
               LIB=str(title.TOOLCHAIN / "lib"))
    compile_command = [str(title.TOOLCHAIN / "bin/cl.exe"), "/nologo", "/W3", "/Od", "/Oy-", "/MT",
                       "/c", "/Fo" + str(obj), str(PROBE_SOURCE)]
    compiled = subprocess.run(compile_command, cwd=PROBE_BUILD, env=env,
                              capture_output=True, text=True, timeout=60)
    (PROBE_BUILD / "compile.log").write_text(compiled.stdout + compiled.stderr,
                                             encoding="utf-8")
    if compiled.returncode or not obj.is_file():
        raise RuntimeError("VC5 failed to compile ENG1 probe; see " + str(PROBE_BUILD / "compile.log"))
    link_command = [str(title.TOOLCHAIN / "bin/link.exe"), "/nologo", "/SUBSYSTEM:CONSOLE",
                    "/INCREMENTAL:NO", "/OUT:" + str(PROBE_EXE), str(obj),
                    "libcmt.lib", "kernel32.lib", "user32.lib"]
    linked = subprocess.run(link_command, cwd=PROBE_BUILD, env=env,
                             capture_output=True, text=True, timeout=60)
    (PROBE_BUILD / "link.log").write_text(linked.stdout + linked.stderr,
                                          encoding="utf-8")
    if linked.returncode or not PROBE_EXE.is_file():
        raise RuntimeError("VC5 failed to link ENG1 probe; see " + str(PROBE_BUILD / "link.log"))
    metadata = PE(PROBE_EXE).metadata()
    if metadata.get("machine") != 0x14C:
        raise RuntimeError("ENG1 probe did not build as x86 PE32")
    report = {"scope": "Static source-built host probe; not executed.",
              "source": str(PROBE_SOURCE.relative_to(ROOT)),
              "source_sha256": sha256(PROBE_SOURCE),
              "exe": str(PROBE_EXE.relative_to(ROOT)),
              "exe_sha256": sha256(PROBE_EXE),
              "architecture": "x86 PE32", "imports": metadata.get("imports", []),
              "toolchain": toolchain, "compile_command": compile_command,
              "link_command": link_command}
    PROBE_REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"probe": report["exe"], "sha256": report["exe_sha256"],
                      "architecture": report["architecture"], "executed": False}, indent=2))
    return report


def source_call_capture_specifications(build, original_rva):
    """Bind a known cdecl call to its fresh COFF relocation and linked E8."""
    if original_rva != 0x4ca0:
        raise ValueError('No reviewed caller capture ABI for this RVA')
    caller = '_dispatch_local_03270'
    target = '_eng1_target_04ca0'
    source = 'src/eng1/behavioral/level_loading.c'
    unit = next(x for x in build['report']['compiled_sources'] if x['source']==source)
    obj = Path(next(x[3:] for x in unit['command'] if x.startswith('/Fo')))
    if sha256(obj) != unit['object_sha256']:
        raise ValueError('Caller COFF contribution identity changed')
    section = COFF(obj).function(caller)
    symbols = read_map(build['map_path'])
    caller_rva = symbols[caller] - build['image'].image_base
    target_rva = symbols[target] - build['image'].image_base
    linked = build['image'].read_rva(caller_rva,len(section['data']))
    result = []
    for relocation in section['relocations']:
        if relocation['symbol'] != target:
            continue
        offset = relocation['offset']
        if relocation['type']!=0x14 or offset<1 or linked[offset-1]!=0xe8:
            raise ValueError('Capture target lacks the reviewed direct cdecl CALL')
        return_rva = caller_rva + offset + 4
        destination = return_rva + int.from_bytes(linked[offset:offset+4],'little',signed=True)
        if destination != target_rva:
            raise ValueError('Linked caller CALL disagrees with its target symbol')
        result.append(dict(original_rva=original_rva,caller=caller,target=target,
            caller_rva=caller_rva,target_rva=target_rva,return_rva=return_rva,
            argument_count=3,prefix_bytes=128,caller_object_sha256=unit['object_sha256'],
            scope='Bounded stack return-address candidate matched to a verified source CALL; not an entry breakpoint or universal stack-unwind proof.'))
    if not result:
        raise ValueError('No reviewed source CALL to capture')
    return result


def launch_once(runtime=DEFAULT_RUNTIME, report_path=DEFAULT_BUILD_REPORT,
                timeout_ms=45000, enter_game=False, capture_call=None):
    runtime = check_runtime(runtime)
    runtime_info = inspect(runtime)
    if not runtime_info["eng1_attached"]:
        raise ValueError("Attach a successful ENG1 build before launch-once")
    if timeout_ms < 5000 or timeout_ms > 120000:
        raise ValueError("Observed launch timeout must be between 5000 and 120000 ms")
    build = load_eng1_report(report_path)
    manifest = read_manifest(runtime)
    if manifest.get("runtime_eng1_sha256") != build["dll_sha256"]:
        raise ValueError("Runtime ENG1 does not match the requested fresh build report")
    probe_report = json.loads(PROBE_REPORT.read_text(encoding="utf-8"))
    if not PROBE_EXE.is_file() or sha256(PROBE_EXE) != probe_report.get("exe_sha256"):
        raise ValueError("Build or restore the hash-verified ENG1 host probe first")
    if sha256(PROBE_SOURCE) != probe_report.get("source_sha256"):
        raise ValueError("ENG1 probe source changed after compilation")
    run_dir = LANE / "runs" / (time.strftime("%Y%m%dT%H%M%SZ", time.gmtime()) + "-" + uuid.uuid4().hex[:8])
    run_dir.mkdir(parents=True, exist_ok=False)
    states_path = run_dir / 'source_state_cells.txt'
    states_path.write_bytes((''.join(f"{x['name']} {x['rva']:x} {x['width']}\n"
                                    for x in build['state_cells'])).encode())
    claim = run_dir / "claim.json"
    claim.write_text(json.dumps({"scope": "One observed launch attempt; never reused.",
                                 "runtime": str(runtime),
                                 "runtime_eng1_sha256": build["dll_sha256"],
                                 "probe_sha256": probe_report["exe_sha256"],
                                 "claimed_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())},
                                indent=2) + "\n", encoding="utf-8")

    events_path = run_dir / "events.jsonl"
    host = runtime / "HERCULES.EXE"
    command = [str(PROBE_EXE.resolve()), str(host.resolve()), str(runtime),
               str(events_path.resolve()), str(timeout_ms), hex(build["entry_rva"]),
               hex(build["interface_rva"]), hex(build["first_use_rva"])]
    command += ['enter-game' if enter_game else 'no-game-input', str(states_path)]
    captures = source_call_capture_specifications(build,capture_call) if capture_call is not None else []
    if captures:
        captures_path=run_dir/'call_captures.txt'
        captures_path.write_bytes((''.join(f"{x['original_rva']:x} {x['return_rva']:x} {x['argument_count']} {x['prefix_bytes']}\n" for x in captures)).encode())
        command.append(str(captures_path))
    result = subprocess.run(command, cwd=runtime, capture_output=True, text=True,
                            timeout=timeout_ms / 1000.0 + 15)
    events = []
    if events_path.is_file():
        events = [json.loads(line) for line in events_path.read_text(
            encoding="utf-8", errors="replace").splitlines() if line.strip()]
    for event in events:
        if event.get("type") == "eng1_module_load":
            event["runtime_eng1_sha256"] = sha256(runtime / "ENG1.DLL")
            event["mapped_entry_rva"] = hex(build["entry_rva"])
            event["map_sha256"] = build["map_sha256"]
            event["source_build_report"] = str(Path(report_path).resolve().relative_to(ROOT))
        image_name = event.get("image_name", "")
        if image_name and Path(image_name).name.upper() == "ENG1.DLL":
            event["expected_eng1_path"] = str((runtime / "ENG1.DLL").resolve())
            event["loaded_image_path_matches_runtime"] = Path(image_name).resolve() == (
                runtime / "ENG1.DLL").resolve()
    exceptions = [row for row in events if row.get("type") == "exception" and
                  not row.get("loader_breakpoint")]
    divergence = next((row for row in exceptions if not row.get("first_chance")),
                      exceptions[0] if exceptions else None)
    report = {"scope": "One finite DEBUG_ONLY_THIS_PROCESS ENG1 runtime observation; not function or gameplay acceptance.",
              "host_sha256": sha256(host), "eng1_sha256": build["dll_sha256"],
              "eng3_sha256": sha256(runtime / "ENG3.DLL"),
              "source_title_sha256": sha256(runtime / "TITLE.DLL"),
              "build_report": str(Path(report_path).resolve().relative_to(ROOT)),
              "build_map_sha256": build["map_sha256"],
              "entry_rva": hex(build["entry_rva"]),
              "entry_address_observation": "module base + linked RVA; execution is not instrumented",
              "interface_rva": hex(build["interface_rva"]),
              "first_use_rva": hex(build["first_use_rva"]),
              "probe_sha256": probe_report["exe_sha256"],
              "timeout_ms": timeout_ms, "command": command,
              "new_game_input_requested": enter_game,
              "call_capture_specifications": captures,
              "probe_exit_code": result.returncode, "probe_stdout": result.stdout,
              "probe_stderr": result.stderr, "events": events,
              "first_divergence_candidate": divergence,
              "eng1_module_load_count": sum(row.get("type") == "eng1_module_load" for row in events),
              "interface_setup_observed": any(row.get("type") == "eng1_interface_snapshot" and
                                               row.get("phase") == "setup_observed" for row in events),
              "windowed_mode_seen": any(row.get("type") == "windowed_mode_ready" for row in events),
              "windowed_mode_ready": next((row.get("windowed_mode_ready",False) for row in reversed(events) if row.get("type") == "summary"),False),
              "shutdown": next((row for row in reversed(events)
                                if row.get("type") == "exit_process"), None)}
    (run_dir / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    manifest["game_launched"] = any(event.get("type") == "create_process" for event in events)
    manifest["last_observed_run_report"] = str((run_dir / "report.json").resolve().relative_to(ROOT))
    (runtime / "runtime_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"probe_exit_code": result.returncode,
                      "eng1_module_load_count": report["eng1_module_load_count"],
                      "interface_setup_observed": report["interface_setup_observed"],
                      "windowed_mode_ready": report["windowed_mode_ready"],
                      "first_divergence_candidate": divergence,
                      "shutdown": report["shutdown"],
                      "report": str((run_dir / "report.json").relative_to(ROOT))}, indent=2))
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("build-probe", help="Freshly compile/link only the x86 observation probe")
    p = sub.add_parser("prepare", help="Stage a hash-checked no-CD runtime with source TITLE and original ENG3")
    p.add_argument("--runtime", type=Path, default=DEFAULT_RUNTIME)
    p = sub.add_parser("attach", help="Attach only a successful report-bound ENG1 build")
    p.add_argument("--runtime", type=Path, default=DEFAULT_RUNTIME)
    p.add_argument("--report", type=Path, default=DEFAULT_BUILD_REPORT)
    p = sub.add_parser("inspect", help="Verify staged hashes and report runtime readiness")
    p.add_argument("--runtime", type=Path, default=DEFAULT_RUNTIME)
    p = sub.add_parser("launch-once", help="Run one finite observed host session; never invoked by preparation")
    p.add_argument("--runtime", type=Path, default=DEFAULT_RUNTIME)
    p.add_argument("--report", type=Path, default=DEFAULT_BUILD_REPORT)
    p.add_argument("--timeout-ms", type=int, default=45000)
    p.add_argument("--enter-game",action="store_true",help="At most six real Enter pulses, only when own PID is foreground and verified windowed; stop on ENG1 load")
    p.add_argument('--capture-call',type=lambda value:int(value,0),choices=[0x4ca0],help='Capture a bounded stack-return candidate and128B input prefix for reviewed source4ca0 caller; reads only')
    args = parser.parse_args()
    if args.command == "build-probe":
        build_probe()
    elif args.command == "prepare":
        prepare(args.runtime)
    elif args.command == "attach":
        attach(args.runtime, args.report)
    elif args.command == "inspect":
        inspect(args.runtime)
    elif args.command == "launch-once":
        raise SystemExit(launch_once(args.runtime, args.report, args.timeout_ms, args.enter_game,args.capture_call))


if __name__ == "__main__":
    main()
