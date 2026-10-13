"""Build and run the fixture-only early/deferred table-publication surrogate."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess

PINNED_TOOLCHAIN = Path(
    r"C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953"
)
PINNED_DDRAW_IMPORT_LIB = Path(r"C:\tools\dx6sdk\lib\ddraw.lib")
EXPECTED_DDRAW_IMPORT_LIB_SHA256 = (
    "09632af2299d3d7ea44c86cea9ca267671488d87abeaecd7c8d99ac0ac71c7d1"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def verify_toolchain(root: Path, toolchain: Path) -> tuple[str, dict[str, dict]]:
    manifest_path = root / "toolchains" / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    matches = [
        row for row in manifest["toolchains"]
        if row["directory"] == toolchain.name
    ]
    if len(matches) != 1:
        raise ValueError("Pinned VC5 directory is not uniquely listed in manifest")
    row = matches[0]
    verified = {}
    for record in row["files"]:
        path = toolchain / record["path"]
        if (not path.is_file() or path.stat().st_size != record["size"]
                or sha256(path) != record["sha256"]):
            raise ValueError("Pinned toolchain file mismatch: " + str(path))
        verified[record["path"]] = {
            "size": record["size"],
            "sha256": record["sha256"],
        }
    return row["id"], verified


def run(command: list[str], cwd: Path, env: dict[str, str],
        commands: list[dict]) -> subprocess.CompletedProcess:
    result = subprocess.run(
        command, cwd=str(cwd), env=env, capture_output=True,
        text=True, errors="replace", check=False
    )
    commands.append({
        "argv": command,
        "cwd": str(cwd),
        "exit_code": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    })
    if result.returncode:
        raise RuntimeError(
            "Command failed with exit " + str(result.returncode) + ": "
            + subprocess.list2cmdline(command) + "\n"
            + result.stdout + result.stderr
        )
    return result


def sha_sources(paths: dict[str, Path]) -> dict[str, str]:
    return {name: sha256(path) for name, path in paths.items()}


def parse_output(stdout: str) -> dict[str, str]:
    fields = {}
    for line in stdout.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value
    return fields


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True,
                        help="checkout containing toolchains/manifest.json and renderer_contract/")
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="new or empty build/run directory under repo-root/work")
    parser.add_argument("--toolchain-dir", type=Path, default=PINNED_TOOLCHAIN)
    parser.add_argument("--ddraw-import-lib", type=Path,
                        default=PINNED_DDRAW_IMPORT_LIB)
    args = parser.parse_args()

    root = args.repo_root.resolve()
    output = args.output_dir.resolve()
    toolchain = args.toolchain_dir.resolve()
    ddraw_import_lib = args.ddraw_import_lib.resolve()
    if not (root / "toolchains" / "manifest.json").is_file():
        parser.error("--repo-root must contain toolchains/manifest.json")
    if not (root / "renderer_contract" / "hercules_host_abi.c").is_file():
        parser.error("--repo-root must contain the existing table-clone helper")
    work_path = root / "work"
    work = work_path.resolve()
    if work_path.is_symlink():
        parser.error("refusing a redirected work directory")
    if output == work or not output.is_relative_to(work):
        parser.error("--output-dir must be a child of repo-root/work")
    if output.exists() and any(output.iterdir()):
        parser.error("--output-dir must be new or empty")

    toolchain_id, verified_toolchain_files = verify_toolchain(root, toolchain)
    if toolchain_id != "msvc5_rtm":
        raise ValueError("Expected msvc5_rtm, found " + toolchain_id)
    if (not ddraw_import_lib.is_file()
            or sha256(ddraw_import_lib) != EXPECTED_DDRAW_IMPORT_LIB_SHA256):
        raise ValueError("DX6 DirectDraw import library hash mismatch")
    output.mkdir(parents=True, exist_ok=True)

    fixture = Path(__file__).resolve().parent
    src = fixture / "src"
    sources = {
        "probe.cpp": src / "probe.cpp",
        "probe.def": src / "probe.def",
        "ddraw_stub.c": src / "ddraw_stub.c",
        "ddraw.def": src / "ddraw.def",
        "hercules_host_abi.c": root / "renderer_contract" / "hercules_host_abi.c",
        "hercules_host_abi.h": root / "renderer_contract" / "hercules_host_abi.h",
        "hercules_types.h": root / "renderer_contract" / "hercules_types.h",
        "builder": Path(__file__).resolve(),
    }
    for name, path in sources.items():
        if not path.is_file():
            raise FileNotFoundError(name + ": " + str(path))

    cl = toolchain / "bin" / "cl.exe"
    link = toolchain / "bin" / "link.exe"
    if not cl.is_file() or not link.is_file():
        raise FileNotFoundError("Pinned VC5 cl.exe or link.exe is missing")

    env = os.environ.copy()
    env.pop("CL", None)
    env.pop("_CL_", None)
    env["PATH"] = str(toolchain / "bin") + os.pathsep + env.get("PATH", "")
    env["INCLUDE"] = str(toolchain / "include")
    env["LIB"] = str(toolchain / "lib")

    early = output / "early_attach"
    deferred = output / "deferred_api"
    early.mkdir()
    deferred.mkdir()
    commands: list[dict] = []

    helper_obj = output / "table_clone.obj"
    run([str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
         "/I" + str(root / "renderer_contract"),
         "/Fo" + str(helper_obj), str(sources["hercules_host_abi.c"])],
        output, env, commands)

    def build_dll(directory: Path, early_attach: bool) -> Path:
        tag = "early" if early_attach else "deferred"
        stub_obj = output / ("ddraw_stub_" + tag + ".obj")
        dll_path = directory / "ddraw.dll"
        implib = directory / "ddraw.lib"
        compile_command = [
            str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
            "/I" + str(root / "renderer_contract"),
        ]
        if early_attach:
            compile_command.append("/DEARLY_ATTACH_PUBLISH")
        compile_command += ["/Fo" + str(stub_obj), str(sources["ddraw_stub.c"])]
        run(compile_command, output, env, commands)
        run([
            str(link), "/NOLOGO", "/DLL", "/MACHINE:I386",
            "/DEF:" + str(sources["ddraw.def"]),
            "/OUT:" + str(dll_path), "/IMPLIB:" + str(implib),
            str(stub_obj), str(helper_obj), "kernel32.lib",
        ], output, env, commands)
        return implib

    build_dll(early, True)
    build_dll(deferred, False)

    def build_probe(directory: Path, expected_early: bool,
                    import_lib: Path) -> Path:
        tag = "early" if expected_early else "deferred"
        obj = output / ("probe_" + tag + ".obj")
        exe = directory / "probe.exe"
        compile_command = [
            str(cl), "/nologo", "/c", "/W3", "/O2", "/MT",
            "/I" + str(root / "renderer_contract"),
        ]
        if expected_early:
            compile_command.append("/DEXPECT_EARLY_ATTACH")
        compile_command += ["/Fo" + str(obj), str(sources["probe.cpp"])]
        run(compile_command, output, env, commands)
        run([
            str(link), "/NOLOGO", "/MACHINE:I386",
            "/SUBSYSTEM:CONSOLE", "/DEF:" + str(sources["probe.def"]),
            "/OUT:" + str(exe), str(obj), str(import_lib), "kernel32.lib",
        ], output, env, commands)
        return exe

    early_exe = build_probe(early, True, ddraw_import_lib)
    deferred_exe = build_probe(deferred, False, ddraw_import_lib)

    run_results = {}
    expected_by_case = {
        "early_attach": {
            "MODE": "EARLY_ATTACH",
            "FIRST_COPY_SLOT89": "0xf17e0089",
            "DLL_STATE_BEFORE_API": "2",
            "PTR_IS_ORIGINAL_AT_CTOR": "0",
        },
        "deferred_api": {
            "MODE": "DEFERRED_API",
            "FIRST_COPY_SLOT89": "0xa5000059",
            "DLL_STATE_BEFORE_API": "1",
            "PTR_IS_ORIGINAL_AT_CTOR": "1",
        },
    }
    for name, directory, exe in (
        ("early_attach", early, early_exe),
        ("deferred_api", deferred, deferred_exe),
    ):
        result = run([str(exe)], directory, env, commands)
        fields = parse_output(result.stdout)
        expected = expected_by_case[name]
        for key, value in expected.items():
            if fields.get(key) != value:
                raise RuntimeError(
                    name + " expected " + key + "=" + value
                    + ", got " + repr(fields.get(key))
                )
        for key in ("FIXTURE_GUARD", "CTOR_RAN", "FIRST_COPY_VALID",
                    "POST_COPY_VALID", "ORIGINAL_UNCHANGED",
                    "OUT_POINTER_NULL"):
            if fields.get(key) != "1":
                raise RuntimeError(name + " failed gate " + key)
        if fields.get("DLL_STATE_AFTER_API") != "2":
            raise RuntimeError(name + " did not reach cloned table state")
        if fields.get("POST_COPY_SLOT89") != "0xf17e0089":
            raise RuntimeError(name + " did not publish the selected clone value")
        if fields.get("MOCK_HRESULT") != "0x80004001":
            raise RuntimeError(name + " mock API did not return E_NOTIMPL")
        run_results[name] = {
            "exit_code": result.returncode,
            "stdout": result.stdout,
            "fields": fields,
        }

    built = {
        "early_attach/ddraw.dll": early / "ddraw.dll",
        "early_attach/probe.exe": early_exe,
        "deferred_api/ddraw.dll": deferred / "ddraw.dll",
        "deferred_api/probe.exe": deferred_exe,
    }
    receipt = {
        "schema": "early-loader-publication-surrogate-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "scope": (
            "SOURCE_BUILT_FIXTURE_ONLY; no Hercules executable, original pointers, "
            "genuine DirectDraw, COM, window, GPU, or product proxy."
        ),
        "prediction": (
            "A selected clone published in fixture DLL_PROCESS_ATTACH is visible "
            "to the EXE C++ global constructor's first table copy; publication "
            "deferred to mock DirectDrawCreate occurs after that first copy."
        ),
        "falsifier": (
            "Reject this fixture result if early_attach first-copy slot 89 is not "
            "the selected replacement, if deferred_api first-copy slot 89 is "
            "already replaced, if any unselected word or source word changes, "
            "or if the mock API does not return E_NOTIMPL."
        ),
        "fixture_guard": (
            "The DLL resolves the V7 accessor during PROCESS_ATTACH; early "
            "publication occurs only after source signature validation. The "
            "probe checks sibling path, V7 identity, and expected DLL state "
            "before calling. Invalid state leaves the source pointer untouched "
            "and the mock API returns E_NOTIMPL with a debug diagnostic."
        ),
        "publication": {
            "source_table_words": 250,
            "signature": "0x5ac00cac",
            "selected_slot": 89,
            "fixture_replacement": "0xf17e0089",
            "unselected_words_preserved": True,
            "source_table_preserved": True,
            "clone_lifetime": "static DLL storage for the process lifetime",
            "helper": "renderer_contract/hercules_host_abi.c:engine_table_clone_selected",
        },
        "api_safety": (
            "Fixture DirectDrawCreate never calls a system API and always returns "
            "HRESULT E_NOTIMPL with a null output object."
        ),
        "toolchain": {
            "id": toolchain_id,
            "path": str(toolchain),
            "manifest_files_verified": verified_toolchain_files,
        },
        "dx6_import_library": {
            "path": str(ddraw_import_lib),
            "sha256": sha256(ddraw_import_lib),
            "role": (
                "link-time x86 DirectDraw import symbols only; each fixture EXE "
                "is runtime-guarded to call its sibling source-built ddraw.dll."
            ),
        },
        "source_sha256": sha_sources(sources),
        "build_commands": commands,
        "build_sha256": {name: sha256(path) for name, path in built.items()},
        "runs": run_results,
        "result": "PASS",
        "limits": [
            "This fixture proves only its own Windows loader ordering and data publication.",
            "It does not prove app-local resolution or constructor ordering for Hercules.",
            "It does not establish that data copying in a product DllMain is safe.",
            "Real DirectDraw loading, forwarding, HWND ownership, and presentation are untested.",
        ],
    }
    receipt_path = output / "receipt.json"
    receipt_path.write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print("RESULT=PASS")
    print("TOOLCHAIN=" + toolchain_id)
    print("RECEIPT=" + str(receipt_path))
    print("EARLY_EXE_SHA256=" + sha256(early_exe))
    print("EARLY_DLL_SHA256=" + sha256(early / "ddraw.dll"))
    print("DEFERRED_EXE_SHA256=" + sha256(deferred_exe))
    print("DEFERRED_DLL_SHA256=" + sha256(deferred / "ddraw.dll"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
