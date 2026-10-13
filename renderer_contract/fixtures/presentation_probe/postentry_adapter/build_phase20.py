"""Build and exercise the phase-20 adapter against synthetic PE modules only."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

PACKAGE = Path(__file__).resolve().parent
REPO = PACKAGE.parents[3]
FIXTURE = REPO / "renderer_contract" / "fixtures" / "presentation_probe"
PUBLIC_SRC = FIXTURE / "src"
CONTRACT = REPO / "renderer_contract"
sys.path.insert(0, str(FIXTURE))
from build_postentry_synthetic import (  # noqa: E402
    EXPECTED_MACHINE, EXPECTED_PE32, PE32, SECTION_EXECUTE,
    sha256, verify_toolchain,
)
from path_guard import (UnsafePathError, guard_work_output,  # noqa: E402
                        require_no_redirects)

DEFAULT_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)


def run(command: list[str], cwd: Path, env: dict[str, str],
        commands: list[dict], timeout: int = 60,
        expected_exit: int = 0) -> subprocess.CompletedProcess:
    try:
        result = subprocess.run(command, cwd=str(cwd), env=env,
                                capture_output=True, text=True,
                                errors="replace", timeout=timeout,
                                check=False)
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout.decode("utf-8", "replace") if isinstance(
            exc.stdout, bytes) else (exc.stdout or "")
        stderr = exc.stderr.decode("utf-8", "replace") if isinstance(
            exc.stderr, bytes) else (exc.stderr or "")
        commands.append({"argv": command, "cwd": str(cwd),
                         "exit_code": None, "timed_out": True,
                         "stdout": stdout, "stderr": stderr})
        raise RuntimeError("timeout: " + subprocess.list2cmdline(command) +
                           "\n" + stdout + stderr)
    commands.append({"argv": command, "cwd": str(cwd),
                     "exit_code": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    if result.returncode != expected_exit:
        raise RuntimeError("unexpected exit %d (wanted %d): %s\n%s%s" % (
            result.returncode, expected_exit,
            subprocess.list2cmdline(command),
            result.stdout, result.stderr))
    return result


def require_tokens(name: str, result: subprocess.CompletedProcess,
                   tokens: tuple[str, ...]) -> None:
    missing = [token for token in tokens if token not in result.stdout]
    if missing:
        raise RuntimeError(name + " missing output tokens " + repr(missing) +
                           "\n" + result.stdout + result.stderr)


def image_info(path: Path) -> dict:
    pe = PE32(path)
    if pe.machine != EXPECTED_MACHINE:
        raise ValueError("not an x86 PE: " + str(path))
    magic = int.from_bytes(pe.data[pe.optional:pe.optional + 2], "little")
    if magic != EXPECTED_PE32:
        raise ValueError("not PE32: " + str(path))
    image_size = int.from_bytes(pe.data[pe.optional + 56:
                                         pe.optional + 60], "little")
    if image_size <= 0:
        raise ValueError("invalid SizeOfImage: " + str(path))
    return {"pe": pe, "size": image_size}


def export_rva(info: dict, name: str, executable: bool = False) -> int:
    pe: PE32 = info["pe"]
    exports = pe.exports()
    symbol = name if name in exports else "_" + name
    if symbol not in exports:
        raise ValueError("missing export %s in %s" % (name, pe.path))
    rva = exports[symbol]
    if rva <= 0 or rva >= info["size"]:
        raise ValueError("export outside image: " + name)
    section = pe.section_for_rva(rva)
    if executable and not section["characteristics"] & SECTION_EXECUTE:
        raise ValueError("marker export not executable: " + name)
    return rva


def write_layout(host: Path, engine: Path, proxy: Path,
                 destination: Path) -> dict:
    host_info = image_info(host)
    engine_info = image_info(engine)
    proxy_info = image_info(proxy)
    rvas = {
        "POSTENTRY_HOST_STATE_RVA": export_rva(
            host_info, "SyntheticHostState"),
        "POSTENTRY_HOST_BEFORE_PROXY_RVA": export_rva(
            host_info, "SyntheticHostBeforeProxyLoad", True),
        "POSTENTRY_ENGINE_STATE_RVA": export_rva(
            engine_info, "SyntheticEngineState"),
        "POSTENTRY_ENGINE_AFTER_COPY_RVA": export_rva(
            engine_info, "SyntheticEngineAfterCopy", True),
        "POSTENTRY_ENGINE_AFTER_WARM_RVA": export_rva(
            engine_info, "SyntheticEngineAfterWarmCopy", True),
        "POSTENTRY_ENGINE_AFTER_RENDER_RVA": export_rva(
            engine_info, "SyntheticEngineAfterRender", True),
        "POSTENTRY_ENGINE_AFTER_CHAIN_RETURN_RVA": export_rva(
            engine_info, "SyntheticEngineAfterChainReturn", True),
        "POSTENTRY_ENGINE_AFTER_SUBMIT_RETURN_RVA": export_rva(
            engine_info, "SyntheticEngineAfterSubmitReturn", True),
    }
    definitions = ["#ifndef HERCULES_PHASE20_LAYOUT_H",
                   "#define HERCULES_PHASE20_LAYOUT_H", ""]
    for key, value in rvas.items():
        definitions.append("#define %s 0x%08xUL" % (key, value))
    definitions.extend((
        "#define POSTENTRY_HOST_IMAGE_SIZE 0x%08xUL" % host_info["size"],
        "#define POSTENTRY_PROXY_IMAGE_SIZE 0x%08xUL" % proxy_info["size"],
        "#define POSTENTRY_ENGINE_IMAGE_SIZE 0x%08xUL" % engine_info["size"],
        '#define PHASE20_HOST_SHA256 "%s"' % sha256(host),
        '#define PHASE20_PROXY_SHA256 "%s"' % sha256(proxy),
        '#define PHASE20_ENGINE_SHA256 "%s"' % sha256(engine),
        "", "#endif", ""))
    destination.write_text("\n".join(definitions), encoding="ascii")
    return {
        "host": {"path": str(host), "sha256": sha256(host),
                 "size_of_image": host_info["size"],
                 "state_rva": rvas["POSTENTRY_HOST_STATE_RVA"],
                 "pre_proxy_marker_rva": rvas[
                     "POSTENTRY_HOST_BEFORE_PROXY_RVA"]},
        "proxy": {"path": str(proxy), "sha256": sha256(proxy),
                  "size_of_image": proxy_info["size"]},
        "engine": {"path": str(engine), "sha256": sha256(engine),
                   "size_of_image": engine_info["size"],
                   "state_rva": rvas["POSTENTRY_ENGINE_STATE_RVA"],
                   "marker_rvas": {
                       key: value for key, value in rvas.items()
                       if "AFTER_" in key}},
        "layout_header_sha256": sha256(destination),
    }


def source_hashes() -> dict[str, str]:
    paths = {path.resolve(strict=True)
             for base in (PACKAGE, PUBLIC_SRC)
             for path in base.rglob("*") if path.is_file() and
             path.suffix.lower() in (".c", ".h", ".def", ".py")}
    paths.update((CONTRACT / name).resolve(strict=True) for name in (
        "hercules_host_abi.c", "hercules_host_abi.h", "hercules_types.h"))
    paths.add((REPO / "toolchains" / "manifest.json").resolve(strict=True))
    result = {}
    for path in sorted(paths):
        resolved = path.resolve(strict=True)
        if not resolved.is_relative_to(REPO):
            raise ValueError("source escaped repository root: " + str(path))
        identity = str(resolved.relative_to(REPO)).replace("\\", "/")
        result[identity] = sha256(resolved)
    return dict(sorted(result.items()))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--toolchain-dir", type=Path,
                        default=DEFAULT_TOOLCHAIN)
    args = parser.parse_args()
    try:
        root, work, output = guard_work_output(args.repo_root,
                                               args.output_dir)
        package = require_no_redirects(PACKAGE)
        fixture = require_no_redirects(FIXTURE)
        toolchain = require_no_redirects(args.toolchain_dir)
    except (UnsafePathError, OSError) as exc:
        parser.error(str(exc))
    expected_package = root / "renderer_contract" / "fixtures" / \
        "presentation_probe" / "postentry_adapter"
    if package != expected_package.resolve(strict=True) or \
            fixture != (root / "renderer_contract" / "fixtures" /
                       "presentation_probe").resolve(strict=True) or \
            root != REPO.resolve(strict=True):
        parser.error("package must be the exact presentation_probe/postentry_adapter subtree under --repo-root")
    assets = (root / "assets").resolve(strict=False)
    if output == assets or output.is_relative_to(assets):
        parser.error("output may not be inside the repository assets tree")
    if output.exists():
        parser.error("choose a fresh output directory; refusing reuse")
    output.mkdir(parents=True)
    build = output / "build"
    build.mkdir()
    tool_record, tool_files = verify_toolchain(root, toolchain)
    cl = toolchain / "bin" / "cl.exe"
    link = toolchain / "bin" / "link.exe"
    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")
    commands: list[dict] = []

    # Rebuild the prior source-built modules and proxy fixture in a fresh child
    # directory; this nested smoke never enters original-target mode.
    base_output = output / "base"
    base_command = [sys.executable, "-B",
                    str(FIXTURE / "build_postentry_synthetic.py"),
                    "--repo-root", str(root),
                    "--output-dir", str(base_output),
                    "--toolchain-dir", str(toolchain)]
    base_run = run(base_command, root, env, commands, timeout=240)
    base_receipt_path = base_output / "postentry_synthetic_receipt.json"
    base_receipt = json.loads(base_receipt_path.read_text(encoding="utf-8"))
    if base_receipt.get("final_status") != "PASS" or \
            base_receipt.get("original_target") != "NOTRUN":
        raise RuntimeError("nested source-built fixture did not pass safely")
    proxy = base_output / "base" / "build" / "ddraw.dll"
    if not proxy.is_file():
        raise FileNotFoundError("nested synthetic proxy missing: " + str(proxy))

    src = PACKAGE / "src"
    host_source = src / "postentry_host.c"
    engine_source = src / "postentry_engine.c"
    engine_def = src / "postentry_engine.def"
    controller_source = src / "postentry_controller.c"
    adapter_source = src / "original_module_adapter.c"
    adapter_header = src / "original_module_adapter.h"
    test_source = src / "test_original_module_adapter.c"
    shared_header = src / "postentry_shared.h"
    for path in (host_source, engine_source, engine_def, controller_source,
                 adapter_source, adapter_header, test_source, shared_header):
        if not path.is_file() or not path.resolve(strict=True).is_relative_to(
                package):
            raise ValueError("private source missing or escaped: " + str(path))
    abi_source = CONTRACT / "hercules_host_abi.c"
    abi_header = CONTRACT / "hercules_host_abi.h"
    types_header = CONTRACT / "hercules_types.h"
    for path in (abi_source, abi_header, types_header,
                 PUBLIC_SRC / "presentation_probe.h"):
        if not path.is_file():
            raise FileNotFoundError(str(path))

    host_obj = build / "phase20_host.obj"
    engine_obj = build / "phase20_engine.obj"
    abi_obj = build / "phase20_abi.obj"
    controller_obj = build / "phase20_controller.obj"
    adapter_obj = build / "phase20_adapter.obj"
    test_obj = build / "phase20_adapter_test.obj"
    host = build / "synthetic_host.exe"
    engine = build / "synthetic_engine.dll"
    engine_lib = build / "synthetic_engine.lib"
    controller = build / "phase20_controller.exe"
    adapter_tests = build / "phase20_adapter_tests.exe"
    layout = build / "postentry_layout.h"
    common = ["/nologo", "/W3", "/Od", "/MT"]
    includes = ["/I" + str(CONTRACT), "/I" + str(PUBLIC_SRC),
                "/I" + str(src)]

    for source, obj, inc in (
            (engine_source, engine_obj, includes),
            (host_source, host_obj, includes),
            (abi_source, abi_obj, ["/I" + str(CONTRACT)])):
        run([str(cl), *common, "/c", *inc, "/Fo" + str(obj), str(source)],
            build, env, commands)
    run([str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
         "/BASE:0x10000000", "/FIXED:NO", "/DEF:" + str(engine_def),
         "/OUT:" + str(engine), "/IMPLIB:" + str(engine_lib),
         str(engine_obj), "kernel32.lib"], build, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(host),
         str(host_obj), str(abi_obj), "kernel32.lib"],
        build, env, commands)
    layout_record = write_layout(host, engine, proxy, layout)

    run([str(cl), *common, "/c", *includes, "/I" + str(build),
         "/Fo" + str(controller_obj), str(controller_source)],
        build, env, commands)
    run([str(cl), *common, "/c", *includes,
         "/Fo" + str(adapter_obj), str(adapter_source)],
        build, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(controller),
         str(controller_obj), str(adapter_obj), "kernel32.lib"],
        build, env, commands)

    run([str(cl), *common, "/c", *includes,
         "/Fo" + str(test_obj), str(test_source)],
        build, env, commands)
    run([str(link), "/NOLOGO", "/MACHINE:I386", "/SUBSYSTEM:CONSOLE",
         "/BASE:0x400000", "/OUT:" + str(adapter_tests),
         str(test_obj), str(adapter_obj), "kernel32.lib"],
        build, env, commands)
    unit = run([str(adapter_tests)], build, env, commands, timeout=10)
    if "RESULT=PASS" not in unit.stdout or "failures=0" not in unit.stdout:
        raise RuntimeError("109 assertion adapter suite did not pass")

    alt_engine = build / "synthetic_engine_alt.dll"
    shutil.copyfile(engine, alt_engine)
    executed = {"controller": controller, "host": host, "engine": engine,
                "engine_alt": alt_engine, "proxy": proxy}
    before = {name: sha256(path) for name, path in executed.items()}
    warning_lines = []
    for item in commands:
        diagnostics = item.get("stdout", "") + item.get("stderr", "")
        warning_lines.extend(line.strip() for line in diagnostics.splitlines()
                             if "warning C" in line or "warning LNK" in line)
    if warning_lines:
        raise RuntimeError("VC5 warnings:\n" + "\n".join(warning_lines))

    cases = {
        "positive": (engine, ("RESULT=PASS", "HOST_SHA256=",
                              "PROXY_SHA256=", "ENGINE_SHA256=",
                              "ORIGINAL_SOURCE_SNAPSHOT_WORDS=250",
                              "ORIGINAL_SOURCE_UNCHANGED=1",
                              "COLD_COPY_MATCHES_PUBLISHED_CLONE=1",
                              "SLOT81_CALLEE_RETURN_OBSERVED=1",
                              "SLOT81_WRAPPER_COMPLETED_CALLER_RETURN=1",
                              "SLOT89_CALLEE_RETURN_OBSERVED=1",
                              "SLOT89_WRAPPER_COMPLETED_CALLER_RETURN=1",
                              "SLOT89_RAW_EAX=0x55667788",
                              "STOP_AFTER_ONE_SYNTHETIC_CALLBACK_PAIR",
                              "EXIT_PROCESS_EVENT_DRAINED=1")),
        "coldwarm": (engine, ("RESULT=PASS", "COLD_COPY_WORDS=250",
                              "WARM_COPY_WORDS=250",
                              "SYNTHETIC_CALLBACK_PAIR_COMPLETED=1",
                              "EXIT_PROCESS_EVENT_DRAINED=1")),
        "timeout": (engine, ("RESULT=PASS", "CONTROLLER_TIMEOUT",
                             "TIMEOUT_AFTER_COLD_COPY=1",
                             "TIMEOUT_CLEANUP_VERIFIED",
                             "EXIT_PROCESS_EVENT_DRAINED=1")),
        "invalidsig": (engine, ("RESULT=PASS", "INVALID_SIGNATURE_REJECTED",
                                "EXIT_PROCESS_EVENT_DRAINED=1")),
        "source_mutation": (engine, ("RESULT=PASS",
                                     "ORIGINAL_SOURCE_UNCHANGED=0",
                                     "REJECTION=ORIGINAL_SOURCE_BASELINE_CHANGED",
                                     "EXPECTED_REJECTION_CHILD_REAPED=1",
                                     "EXIT_PROCESS_EVENT_DRAINED=1")),
        "wrongidentity": (alt_engine, ("RESULT=PASS",
                                       "REJECTION=UNEXPECTED_SYNTHETIC_MODULE_PATH",
                                       "EXPECTED_REJECTION_CHILD_REAPED=1",
                                       "EXIT_PROCESS_EVENT_DRAINED=1")),
        "hash_mismatch": (engine, ("RESULT=PASS",
                                   "REJECTION=ENGINE_HASH_MISMATCH",
                                   "ENGINE_OBSERVED_SHA256=",
                                   "EXPECTED_REJECTION_CHILD_REAPED=1",
                                   "EXIT_PROCESS_EVENT_DRAINED=1")),
    }
    tests = {}
    for mode, (child_engine, tokens) in cases.items():
        result = run([str(controller), "--synthetic", str(host), str(proxy),
                      str(child_engine), mode], build, env, commands,
                     timeout=35)
        require_tokens(mode, result, tokens)
        tests[mode] = {"exit_code": result.returncode,
                       "stdout": result.stdout, "stderr": result.stderr,
                       "status": "PASS", "required_tokens": list(tokens)}

    blocked = run([str(controller), "--original"], build, env, commands,
                  timeout=5, expected_exit=3)
    require_tokens("original_block", blocked,
                   ("ORIGINAL_TARGET=NOTRUN",
                    "REGISTRY_PROFILE_ISOLATION=UNPROVEN",
                    "WINDOWED_STARTUP_AND_MODE_RESTORE=UNPROVEN",
                    "ORIGINAL_LAUNCH_CODE=DISABLED", "RESULT=BLOCKED"))
    tests["original_block"] = {"exit_code": blocked.returncode,
                               "stdout": blocked.stdout,
                               "stderr": blocked.stderr,
                               "status": "BLOCKED_AS_DESIGNED"}
    after = {name: sha256(path) for name, path in executed.items()}
    if before != after:
        raise RuntimeError("an executed source-built image changed during tests")

    receipt = {
        "schema": "hercules-phase20-postentry-adapter-synthetic-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "baseline": "8c2b4c9cff266394f9917a3ac4854f3c8a40349e",
        "scope": "Source-built Windows host/proxy/engine and controller only; no original EXE/DLL/process, DirectDraw API, surface, HWND, display mode, assets, or gameplay.",
        "original_target": "NOTRUN",
        "original_gate": "--original exits before process creation; registry/profile isolation, writable-path confinement, and windowed startup/mode restoration remain unproven.",
        "toolchain": {"id": tool_record["id"],
                      "repository": tool_record["repository"],
                      "commit": tool_record["commit"],
                      "archive_sha256": tool_record["archive_sha256"],
                      "verified_files": tool_files},
        "runtime": {"python_version": sys.version,
                    "python_executable": sys.executable,
                    "python_executable_sha256": sha256(Path(sys.executable)),
                    "platform": platform.platform()},
        "candidate_install_path": "renderer_contract/fixtures/presentation_probe/postentry_adapter",
        "output_policy": "new output below --repo-root/work only; lexical and resolved paths guarded; assets tree excluded",
        "source_identities_sha256": source_hashes(),
        "layout_and_module_identities": layout_record,
        "nested_source_built_baseline": {
            "receipt": str(base_receipt_path),
            "sha256": sha256(base_receipt_path),
            "result": base_receipt.get("final_status"),
            "stdout": base_run.stdout},
        "adapter_unit_test": {"stdout": unit.stdout,
                              "exit_code": unit.returncode,
                              "checks": 109, "failures": 0},
        "executed_image_sha256_before_and_after": {
            "before": before, "after": after, "unchanged": True},
        "compiler_flags": ["/W3", "/Od", "/MT"],
        "compiler_warnings": warning_lines,
        "commands": commands,
        "synthetic_tests": tests,
        "observation_order": [
            "At the source host's exported pre-proxy marker, snapshot all 250 original source words before clone publication.",
            "At engine load, verify the observed event path, full SHA-256, PE32 image size, and required exports; arm DR0 at the source-built post-copy marker.",
            "At the cold-copy marker, compare original-source snapshot, published clone, and local table across all 250 words. Only then install slot-89 and trace-commit write watches plus source-defined caller-return markers.",
            "Trace committed-sequence writes distinguish callback entry from callback-return records. DR2 callback-return is reported before wrapper return; the engine's immediate post-call marker validates wrapper completion.",
            "Sticky DR6 is cleared before each continuation; raw slot89 EAX is checked against both the wrapper trace and host callback result. The positive target stops after one callback pair and is reaped.",
        ],
        "limits": [
            "Controller hashes files using the observed debug-event file handles and checks their final paths; these identities are synthetic source-built artifacts only.",
            "The caller-return points are native source-built marker entries immediately after each indirect callback invocation; they validate fixture control-flow completion but are not original-module callsite EIPs.",
            "The source-built host source table is compared with its own prior marker snapshot; this says nothing about original host state.",
            "The existing passive adapter profiles for original DLLs are not promoted to host integration; no original module was loaded or run.",
            "The `--original` command is still blocked before process creation. No display ownership, GPU, fullscreen, or gameplay behavior is tested.",
        ],
        "result": "PASS",
    }
    receipt_path = output / "phase20_receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) +
                            "\n", encoding="utf-8")
    print("RESULT=PASS")
    print("ORIGINAL_TARGET=NOTRUN")
    print("SYNTHETIC_CASES=%d" % len(cases))
    print("ADAPTER_ASSERTIONS=109")
    print("RECEIPT=" + str(receipt_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
