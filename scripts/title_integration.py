"""Prepare and exercise a disposable original-PC TITLE integration copy.

The verified installer is the host/data baseline. --no-cd applies the separately
authorized one-byte change only to the disposable host copy at the final staging
step; all original and matching build inputs remain unchanged. The behavioral
runtime omits original TITLE; the separate baseline preserves it for comparison.
Attach requires the
fresh x86 DLL named and hashed by scripts/title_behavioral.py's report. Nothing
in assets/ or the original installation is modified.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
LANE = ROOT / "work/title_behavioral/integration"
INSTALL = ROOT / "work/discs/pc_install"
DEFAULT_RUNTIME = LANE / "runtime_nocd"
BASELINE_RUNTIME = LANE / "runtime_baseline_nocd"
BUILD_REPORT = ROOT / "work/title_behavioral/build/report.json"
FIXTURE_SOURCE = ROOT / "tests/title_loader_fixture.c"
DEBUGGER_SOURCE = ROOT / "tests/title_host_probe.c"
TOOLCHAIN = Path(r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953")

ORIGINAL_SHA256 = {
    "ENG1.DLL": "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757",
    "ENG3.DLL": "aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7",
    "HERCULES.EXE": "587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d",
    "HERCULES.FS": "affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4",
    "HERCULES.GID": "7d36ba8792c436b84dff13d217ca16793c04cd45b7beeac24b59b2f1b9b3ca41",
    "HERCULES.HLP": "643494f46dcb6ef06a397fea3967f76d355c42079c6a81df833ba6a52293a6ab",
    "TITLE.DLL": "b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b",
}
NOCD_EXE_SHA256 = 'b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551'
NOCD_OFFSET = 0x94b0


def runtime_host_hash(manifest):
    expected = manifest['host_sha256']
    if expected not in (ORIGINAL_SHA256['HERCULES.EXE'], NOCD_EXE_SHA256):
        raise ValueError('Unrecognized runtime host identity')
    return expected


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
    if not under(runtime, LANE):
        raise ValueError("Runtime output must stay inside work/title_behavioral/integration")
    return runtime


def load_report(path=BUILD_REPORT):
    path = Path(path).resolve()
    if not under(path, ROOT / "work") or not path.is_file():
        raise ValueError("Build report must be an existing file under work/")
    report = json.loads(path.read_text(encoding="utf-8"))
    if report.get("link_exit_code") != 0 or report.get("architecture") != "x86 PE32":
        raise ValueError("The behavioral build report does not describe a successful x86 PE32 DLL")
    export_identity = [{"name": e.get("name"), "ordinal": e.get("ordinal")}
                       for e in report.get("exports", [])]
    if export_identity != [{"name": "PC_DLLEngineMain", "ordinal": 1}]:
        raise ValueError("Behavioral build has the wrong export ABI")
    if "TITLE.DLL" in [x.get("dll", "").upper() for x in report.get("imports", [])]:
        raise ValueError("Behavioral build imports the original TITLE.DLL")
    dll = (ROOT / report["dll"]).resolve()
    if not under(dll, ROOT / "work") or not dll.is_file():
        raise ValueError("Reported behavioral DLL must exist under work/")
    digest = sha256(dll)
    if digest != report.get("dll_sha256"):
        raise ValueError("Behavioral DLL hash differs from its build report")
    recorded_inputs = dict(report.get("context_sha256", {}))
    recorded_inputs["recovery.json"] = report.get("recovery_sha256")
    recorded_inputs["evidence/title_behavioral_build.json"] = report.get("config_sha256")
    for relative, expected in recorded_inputs.items():
        source = (ROOT / relative).resolve()
        if not under(source, ROOT) or not source.is_file() or sha256(source) != expected:
            raise ValueError("Rebuild TITLE: recorded build input changed: " + relative)
    if digest == ORIGINAL_SHA256["TITLE.DLL"]:
        raise ValueError("Refusing to use the unmodified original TITLE.DLL as a behavioral build")
    entry = [row for row in report.get("functions", [])
             if row.get("module") == "TITLE.DLL" and row.get("rva", "").lower() == "0xc1f0"]
    if len(entry) != 1 or not entry[0].get("compiled_definition"):
        raise ValueError("Build report does not include a compiled PC_DLLEngineMain behavioral provider")
    return report, dll, digest


def parse_title_link_map(report=None):
    """Read the map belonging to the hash-verified source DLL, never native RVAs."""
    from pe import PE
    from title_link import read_map
    verified, dll, _ = load_report()
    if report is not None and report.get("dll_sha256") != verified["dll_sha256"]:
        raise ValueError("TITLE address map and supplied build report differ")
    path = (ROOT / verified.get("link_map", str(dll.with_suffix('.map').relative_to(ROOT)))).resolve()
    if not under(path, ROOT / 'work') or not path.is_file():
        raise ValueError("TITLE link map must exist under work/")
    expected = verified.get('link_map_sha256')
    if expected is not None and sha256(path) != expected:
        raise ValueError("TITLE link map changed after linking")
    pe = PE(dll)
    symbols = read_map(path)
    entry = symbols.get('_PC_DLLEngineMain')
    if entry is None or entry - pe.image_base != verified['exports'][0]['rva']:
        raise ValueError("TITLE link map disagrees with the DLL export")
    return pe.image_base, symbols


def prepare(runtime, no_cd=True, baseline=False):
    runtime = check_runtime(runtime)
    runtime.mkdir(parents=True, exist_ok=True)
    source_hashes = {}
    for name, expected in ORIGINAL_SHA256.items():
        source = INSTALL / name
        if not source.is_file() or sha256(source) != expected:
            raise ValueError("Original installation input changed: " + str(source))
        source_hashes[name] = expected
        if name == "TITLE.DLL":
            continue
        destination = runtime / name
        runtime_hash = NOCD_EXE_SHA256 if no_cd and name == 'HERCULES.EXE' else expected
        if destination.exists():
            if not destination.is_file() or sha256(destination) != runtime_hash:
                raise ValueError("Refusing to overwrite a nonmatching runtime file: " + str(destination))
        else:
            shutil.copy2(source, destination)
            if no_cd and name == 'HERCULES.EXE':
                data = bytearray(destination.read_bytes())
                if data[NOCD_OFFSET] != 0x53:
                    raise ValueError('Unexpected original byte at the reviewed no-CD site')
                data[NOCD_OFFSET] = 0xc3
                destination.write_bytes(data)
        if sha256(destination) != runtime_hash:
            raise ValueError("Runtime copy hash differs: " + str(destination))

    title = runtime / "TITLE.DLL"
    if title.exists():
        if not title.is_file():
            raise ValueError("Runtime TITLE.DLL path is not a file")
        if sha256(title) == ORIGINAL_SHA256["TITLE.DLL"] and not baseline:
            raise ValueError("Original TITLE.DLL appeared in the runtime copy")
        if baseline and sha256(title) != ORIGINAL_SHA256["TITLE.DLL"]:
            raise ValueError("Baseline directory already contains a different TITLE.DLL")
        # It may only be the same behavioral build already recorded below.
    manifest_path = runtime / "runtime_manifest.json"
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if manifest.get("original_install_sha256") != source_hashes:
            raise ValueError("Existing runtime manifest has different original inputs")
    else:
        manifest = {
            "scope": "Disposable integration copy from original PC install; no game launch implied.",
            "source_install": str(INSTALL.relative_to(ROOT)),
            "original_install_sha256": source_hashes,
            "host_sha256": NOCD_EXE_SHA256 if no_cd else source_hashes["HERCULES.EXE"],
            "host_variant": "no_cd_runtime_copy" if no_cd else "unmodified_installer",
            "post_build_patch": {"scope": "Disposable runtime EXE only; user-authorized no-CD staging",
                                 "offset": hex(NOCD_OFFSET), "before": "53", "after": "c3",
                                 "original_sha256": source_hashes["HERCULES.EXE"],
                                 "runtime_sha256": NOCD_EXE_SHA256} if no_cd else None,
            "original_title_sha256_excluded": source_hashes["TITLE.DLL"],
            "runtime_title_sha256": None,
            "prepared_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "game_launched": False,
        }
    if baseline and not title.exists():
        shutil.copy2(INSTALL / 'TITLE.DLL', title)
    if title.exists():
        manifest["runtime_title_sha256"] = sha256(title)
    manifest['title_kind'] = 'original_baseline' if baseline else 'behavioral'
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"runtime": str(runtime), "files_copied_or_verified": len(source_hashes) - 1,
                      "original_title_excluded": not baseline, "host_sha256": manifest['host_sha256'],
                      "host_variant": manifest.get('host_variant', 'unmodified_installer')}))


def attach(runtime, report_path=BUILD_REPORT):
    runtime = check_runtime(runtime)
    manifest_path = runtime / "runtime_manifest.json"
    if not manifest_path.is_file():
        raise ValueError("Prepare the runtime copy before attaching TITLE.DLL")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    report, dll, digest = load_report(report_path)
    destination = runtime / "TITLE.DLL"
    if destination.exists() and sha256(destination) != digest:
        previous = sha256(destination)
        if previous != manifest.get("runtime_title_sha256") or previous == ORIGINAL_SHA256["TITLE.DLL"]:
            raise ValueError("Refusing to overwrite an unrecorded TITLE.DLL in the runtime copy")
        history = LANE / "previous_builds"
        history.mkdir(exist_ok=True)
        shutil.copy2(destination, history / (previous + ".dll"))
        shutil.copy2(dll, destination)
    elif not destination.exists():
        shutil.copy2(dll, destination)
    if sha256(destination) != digest:
        raise ValueError("Copied behavioral TITLE.DLL hash mismatch")
    manifest.update({"scope": "Disposable host with the reported behavioral TITLE build installed.",
                     "runtime_title_sha256": digest,
                     "behavioral_build_report": str(Path(report_path).resolve().relative_to(ROOT)),
                     "behavioral_build_sha256": report.get("dll_sha256")})
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"runtime": str(runtime), "title_dll": str(destination),
                      "title_sha256": digest, "host_sha256": manifest["host_sha256"]}))


def build_fixture():
    if not FIXTURE_SOURCE.is_file():
        raise ValueError("Missing loader fixture source: " + str(FIXTURE_SOURCE))
    from match import verify_toolchain
    verify_toolchain(TOOLCHAIN)
    out = LANE / "fixture"
    out.mkdir(parents=True, exist_ok=True)
    obj, exe = out / "title_loader_fixture.obj", out / "title_loader_fixture.exe"
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(TOOLCHAIN / "include")
    env["LIB"] = str(TOOLCHAIN / "lib")
    compile_command = [str(TOOLCHAIN / "bin/cl.exe"), "/nologo", "/W3", "/Od", "/Oy-", "/MT", "/c",
                       "/Fo" + str(obj), str(FIXTURE_SOURCE)]
    compile_result = subprocess.run(compile_command, cwd=out, env=env, capture_output=True, text=True)
    (out / "compile.log").write_text(compile_result.stdout + compile_result.stderr, encoding="utf-8")
    if compile_result.returncode or not obj.is_file():
        raise RuntimeError("VC5 failed to build loader fixture; see " + str(out / "compile.log"))
    link_command = [str(TOOLCHAIN / "bin/link.exe"), "/nologo", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO",
                    "/OUT:" + str(exe), str(obj), "libcmt.lib", "kernel32.lib"]
    link_result = subprocess.run(link_command, cwd=out, env=env, capture_output=True, text=True)
    (out / "link.log").write_text(link_result.stdout + link_result.stderr, encoding="utf-8")
    if link_result.returncode or not exe.is_file():
        raise RuntimeError("VC5 failed to link loader fixture; see " + str(out / "link.log"))
    result = {"fixture_source_sha256": sha256(FIXTURE_SOURCE), "fixture_exe_sha256": sha256(exe),
              "fixture": str(exe.relative_to(ROOT)), "toolchain": verify_toolchain(TOOLCHAIN),
              "compile_command": compile_command, "link_command": link_command}
    (out / "build_report.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"fixture": result["fixture"], "sha256": result["fixture_exe_sha256"],
                      "toolchain": result["toolchain"]}))
    return exe


def run_fixture(runtime):
    runtime = check_runtime(runtime)
    report, dll, digest = load_report()
    manifest_path = runtime / "runtime_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if sha256(runtime / "HERCULES.EXE") != runtime_host_hash(manifest):
        raise ValueError("Runtime host executable is not the verified original")
    if sha256(runtime / "TITLE.DLL") != digest or manifest.get("runtime_title_sha256") != digest:
        raise ValueError("Runtime TITLE.DLL is not the reported behavioral build")
    exe = build_fixture()

    command = [str(exe), str((runtime / "TITLE.DLL").resolve())]
    result = subprocess.run(command, cwd=runtime, capture_output=True, text=True, timeout=30)
    measurement = {
        "scope": "Behavioral DLL load/export/entry smoke only; no game launch or equivalence claim.",
        "runtime": str(runtime), "host_sha256": sha256(runtime / "HERCULES.EXE"),
        "original_title_excluded_sha256": ORIGINAL_SHA256["TITLE.DLL"],
        "behavioral_title_sha256": digest, "fixture_source_sha256": sha256(FIXTURE_SOURCE),
        "fixture_exe_sha256": sha256(exe), "fixture_command": command,
        "fixture_exit_code": result.returncode, "fixture_stdout": result.stdout,
        "fixture_stderr": result.stderr,
        "result": "PASS" if result.returncode == 0 else "FAIL",
    }
    report_path = LANE / "loader_report.json"
    report_path.write_text(json.dumps(measurement, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": measurement["result"], "exit_code": result.returncode,
                      "stdout": result.stdout.strip(), "report": str(report_path)}))
    return result.returncode


def build_debugger():
    from match import verify_toolchain
    from pe import PE
    verify_toolchain(TOOLCHAIN)
    out = LANE / "debugger"
    out.mkdir(parents=True, exist_ok=True)
    obj, exe = out / "host_debugger_probe.obj", out / "host_debugger_probe.exe"
    env = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        env.pop(key, None)
    env["PATH"] = str(TOOLCHAIN / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(TOOLCHAIN / "include")
    env["LIB"] = str(TOOLCHAIN / "lib")
    compile_command = [str(TOOLCHAIN / "bin/cl.exe"), "/nologo", "/W3", "/Od", "/Oy-", "/MT", "/c",
                       "/Fo" + str(obj), str(DEBUGGER_SOURCE)]
    compile_result = subprocess.run(compile_command, cwd=out, env=env, capture_output=True, text=True)
    (out / "compile.log").write_text(compile_result.stdout + compile_result.stderr, encoding="utf-8")
    if compile_result.returncode or not obj.is_file():
        raise RuntimeError("VC5 failed to build debugger probe; see " + str(out / "compile.log"))
    link_command = [str(TOOLCHAIN / "bin/link.exe"), "/nologo", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO",
                    "/OUT:" + str(exe), str(obj), "libcmt.lib", "kernel32.lib", "user32.lib", "advapi32.lib", "gdi32.lib"]
    link_result = subprocess.run(link_command, cwd=out, env=env, capture_output=True, text=True)
    (out / "link.log").write_text(link_result.stdout + link_result.stderr, encoding="utf-8")
    if link_result.returncode or not exe.is_file():
        raise RuntimeError("VC5 failed to link debugger probe; see " + str(out / "link.log"))
    pe = PE(exe)
    metadata = pe.metadata()
    if metadata["machine"] != 0x14c:
        raise RuntimeError("Debugger probe did not build as x86 PE32")
    result = {"scope": "Source-built Win32 debug-event probe; not executed.",
              "source_sha256": sha256(DEBUGGER_SOURCE), "exe_sha256": sha256(exe),
              "exe": str(exe.relative_to(ROOT)), "toolchain": verify_toolchain(TOOLCHAIN),
              "architecture": "x86 PE32", "imports": metadata["imports"],
              "compile_command": compile_command, "link_command": link_command}
    (out / "build_report.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"probe": result["exe"], "sha256": result["exe_sha256"],
                      "toolchain": result["toolchain"], "architecture": result["architecture"]}))
    return exe


def debug_runtime(runtime, timeout_ms=45000, title_trace=False, baseline=False):
    runtime = check_runtime(runtime)
    if baseline:
        title_digest = ORIGINAL_SHA256['TITLE.DLL']
        title_trace = False
        pool_rva = 0x29e18
    else:
        report, dll, title_digest = load_report()
        from title_behavioral import read_map
        from pe import PE
        pool_rva = read_map(LANE.parent / 'build/TITLE.map')['_g_29e18'] - PE(dll).image_base
    manifest_path = runtime / "runtime_manifest.json"
    if not manifest_path.is_file():
        raise ValueError("Prepare the runtime copy before debug launch")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    host = runtime / "HERCULES.EXE"
    title = runtime / "TITLE.DLL"
    if sha256(host) != runtime_host_hash(manifest):
        raise ValueError("Refusing debug launch: host executable hash changed")
    if sha256(title) != title_digest or manifest.get("runtime_title_sha256") != title_digest:
        raise ValueError("Refusing debug launch: TITLE.DLL is not the reported behavioral build")
    for name in ("ENG1.DLL", "ENG3.DLL", "HERCULES.FS", "HERCULES.GID", "HERCULES.HLP"):
        if sha256(runtime / name) != ORIGINAL_SHA256[name]:
            raise ValueError("Refusing debug launch: original support hash changed: " + name)

    probe = LANE / "debugger/host_debugger_probe.exe"
    if not probe.is_file():
        raise ValueError("Build debugger probe first: " + str(probe))
    build_report = json.loads((LANE / "debugger/build_report.json").read_text(encoding="utf-8"))
    if sha256(probe) != build_report.get("exe_sha256"):
        raise ValueError("Debugger executable differs from its build report")
    if sha256(DEBUGGER_SOURCE) != build_report.get("source_sha256"):
        raise ValueError("Rebuild debugger: probe source changed after compilation")
    suffix = 'baseline_windowed' if baseline else 'behavioral_windowed'
    events_path = LANE / ('debugger/runtime_events_' + suffix + '.jsonl')
    trace_path = LANE / ('debugger/title_trace_' + suffix + '.jsonl')
    if title_trace and trace_path.exists():
        trace_path.unlink()
    command = [str(probe.resolve()), str(host.resolve()), str(runtime), str(events_path), str(timeout_ms), '--windowed',
               '--title-pool-rva', hex(pool_rva)]
    env = os.environ.copy()
    env.pop("HERCULES_TITLE_TRACE", None)
    if title_trace:
        env["HERCULES_TITLE_TRACE"] = str(trace_path.resolve())
    result = subprocess.run(command, cwd=runtime, env=env, capture_output=True, text=True,
                            timeout=timeout_ms / 1000.0 + 15)
    event_rows = []
    if events_path.is_file():
        for line in events_path.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.strip():
                event_rows.append(json.loads(line))
    for row in event_rows:
        if row.get("type") not in ("load_dll", "create_process"):
            continue
        image_name = row.get("image_name", "")
        leaf = image_name.replace("/", "\\").rstrip("\\").split("\\")[-1].upper()
        expected = ORIGINAL_SHA256.get(leaf)
        if leaf == 'HERCULES.EXE':
            expected = runtime_host_hash(manifest)
        if leaf == "TITLE.DLL":
            expected = title_digest
        row["expected_runtime_sha256"] = expected
        candidate = runtime / leaf
        row["runtime_file_sha256"] = sha256(candidate) if expected and candidate.is_file() else None

    summary = next((row for row in reversed(event_rows) if row.get("type") == "summary"), None)
    measurement = {
        "scope": "Windowed DEBUG_ONLY_THIS_PROCESS launch; native menu command, own-PID window observations and captures, no code or process-memory edits.",
        "host_sha256": sha256(host), "title_sha256": title_digest,
        "behavioral_title_sha256": None if baseline else title_digest,
        "title_kind": 'original_baseline' if baseline else 'behavioral',
        "windowed_requested": True,
        "behavioral_build_report": None if baseline else str(BUILD_REPORT.relative_to(ROOT)),
        "debugger_sha256": sha256(probe), "debugger_toolchain": build_report.get("toolchain"),
        "timeout_ms": timeout_ms, "title_trace_enabled": bool(title_trace),
        "title_trace_file": str(trace_path.relative_to(ROOT)) if title_trace else None,
        "probe_command": command, "probe_exit_code": result.returncode,
        "probe_stdout": result.stdout, "probe_stderr": result.stderr,
        "events": event_rows, "summary": summary,
        "loaded_title_event_count": sum(r.get("type") == "load_dll" and
                                         r.get("image_name", "").replace("/", "\\").rstrip("\\").split("\\")[-1].upper() == "TITLE.DLL"
                                         for r in event_rows),
    }
    output_path = LANE / ('debugger/host_debugger_report_' + suffix + '.json')
    output_path.write_text(json.dumps(measurement, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"probe_exit_code": result.returncode, "summary": summary,
                      "loaded_title_event_count": measurement["loaded_title_event_count"],
                      "report": str(output_path)}))
    return result.returncode


def launch_game(runtime, timeout):
    # Every game integration run uses the native windowed command and bounded
    # debugger lifecycle, so launch cannot silently fall back to fullscreen.
    return debug_runtime(runtime, timeout * 1000, False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("prepare", "prepare-baseline", "attach", "build-fixture", "build-debugger", "fixture", "debug-launch", "debug-baseline", "launch"))
    parser.add_argument("--runtime", type=Path, default=DEFAULT_RUNTIME)
    parser.add_argument("--no-cd", action="store_true", default=True,
                        help="Apply the reviewed EXE-only patch to the disposable copy during prepare (default)")
    parser.add_argument("--original-host", action="store_false", dest="no_cd",
                        help="Stage the CD-requiring original installer EXE; choose a separate --runtime directory")
    parser.add_argument("--timeout", type=int, default=45)
    parser.add_argument("--timeout-ms", type=int, default=45000)
    parser.add_argument("--title-trace", action="store_true")
    args = parser.parse_args()
    if args.command in ('prepare-baseline', 'debug-baseline') and args.runtime == DEFAULT_RUNTIME:
        args.runtime = BASELINE_RUNTIME
    if args.command == 'prepare-baseline':
        prepare(args.runtime, args.no_cd, True)
    elif args.command == "prepare":
        prepare(args.runtime, args.no_cd)
    elif args.command == "attach":
        attach(args.runtime)
    elif args.command == "build-fixture":
        build_fixture()
    elif args.command == "build-debugger":
        build_debugger()
    elif args.command == "fixture":
        return run_fixture(args.runtime)
    elif args.command == "debug-launch":
        return debug_runtime(args.runtime, args.timeout_ms, args.title_trace)
    elif args.command == 'debug-baseline':
        return debug_runtime(args.runtime, args.timeout_ms, False, True)
    else:
        return launch_game(args.runtime, args.timeout)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        print("integration error:", exc, file=sys.stderr)
        raise SystemExit(2)
