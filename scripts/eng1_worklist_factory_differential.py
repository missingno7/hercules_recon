import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "scripts"))
from eng1_data_evidence import scan_quoted_includes
from match import DEFAULT_TOOLCHAIN, verify_toolchain

candidate = root / "src/eng1/behavioral"
lane = root / "work/eng1_behavioral/differential/worklist_factories"
lane.mkdir(parents=True, exist_ok=True)
oracle = root / "assets/pc/HERCULES/ENG1.DLL"
sources = [
    root / "tests/eng1_worklist_factory_differential.c",
    candidate / "worklist_factories.c",
    candidate / "worklist_read_guards.c",
    root / "calibration/actor_pool_reset_repaired.c",
]
inputs = sources + scan_quoted_includes(sources)
sha = lambda data: hashlib.sha256(data).hexdigest()
oracle_hash = sha(oracle.read_bytes())
expected_oracle = "01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757"
if oracle_hash != expected_oracle:
    raise RuntimeError("ENG1 oracle identity changed")
before = {path.resolve().relative_to(root.resolve()).as_posix(): sha(path.read_bytes())
          for path in inputs}
before["assets/pc/HERCULES/ENG1.DLL"] = oracle_hash

toolchain_id = verify_toolchain(DEFAULT_TOOLCHAIN)
environment = os.environ.copy()
for key in ("CL", "_CL_", "LINK", "_LINK_"):
    environment.pop(key, None)
environment.update(PATH=str(DEFAULT_TOOLCHAIN / "bin") + os.pathsep + environment.get("PATH", ""),
                   INCLUDE=str(DEFAULT_TOOLCHAIN / "include"),
                   LIB=str(DEFAULT_TOOLCHAIN / "lib"))
executable = lane / "fixture.exe"
executable.unlink(missing_ok=True)
command = [str(DEFAULT_TOOLCHAIN / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/MT",
           "/DENG1_WORKLIST_GUARD_FIXTURE",
           "/Fo" + str(lane) + os.sep, "/Fe" + str(executable),
           *map(str, sources), "/link", "/INCREMENTAL:NO", "/OPT:REF", "kernel32.lib"]
build = subprocess.run(command, cwd=lane, env=environment,
                       capture_output=True, text=True, timeout=120)
(lane / "compile.log").write_text(build.stdout + build.stderr,
                                  encoding="utf-8", newline="\n")
report = {
    "scope": "Fresh historical VC5 x86 semantic differential: original unpatched ENG1 functions 0x270c0, 0x27720, 0x27c40 versus canonical source, with source allocator/initializer provider linked unchanged.",
    "oracle_sha256": oracle_hash,
    "toolchain_id": toolchain_id,
    "command": command,
    "compile_exit_code": build.returncode,
    "input_sha256_before": before,
    "native_execution": "LoadLibraryExA with DONT_RESOLVE_DLL_REFERENCES in a separate x86 process; calls are loaded_base + native RVA. No target code bytes are patched or called by source.",
    "coverage": {
        "successes": ["0x270c0 normal kind 2", "0x27720 middle kind 0x4003", "0x27c40 alternate kind 0x2005"],
        "failures": ["allocator-exhausted null return for each of the three families"],
        "compared": ["complete 20-byte worklist record", "full actor pool region", "entire selected actor stride including untouched gaps", "actor bytes observed by callback", "callback count/order/argument identity relative to selected slot", "allocator cursor WORD"],
        "pointer_normalization": "Actor return/callback pointers are compared only as pool-relative slot offsets. Actor+0x58 is raw record-owner address difference and compared as bytes; the controlled owner and record both use matching +20 layout. Asset DWORDs and callback code pointers use shared fixture allocations in one x86 process and retain exact values."
    },
    "limitations": [
        "Finite group-index-zero records and selected asset/action rows only; invalid source-prefix cases remain source-guard fixtures.",
        "This is semantic differential evidence, not FUNCTION_MATCH, whole-DLL equivalence, gameplay validation, or historical capacity proof.",
        "Only allocator-exhaustion failure is covered; other host/runtime failures are outside this fixture."
    ],
}
if build.returncode == 0 and executable.is_file():
    run = subprocess.run([str(executable), str(oracle)], cwd=lane, env=environment,
                         capture_output=True, text=True, timeout=60)
    (lane / "run.log").write_text(run.stdout + run.stderr,
                                  encoding="utf-8", newline="\n")
    report.update(run_exit_code=run.returncode, stdout=run.stdout, stderr=run.stderr,
                  executable_sha256=sha(executable.read_bytes()))
    report["result"] = "PASS" if run.returncode == 0 and "failures=0" in run.stdout else "DIFFERENTIAL_FAILURE"
else:
    report.update(run_exit_code=None, stdout="", stderr=build.stderr,
                  result="COMPILE_FAILURE")
after = {name: sha((root / name).read_bytes()) for name in before}
report["input_sha256_after"] = after
report["inputs_unchanged"] = before == after
(lane / "report.json").write_text(json.dumps(report, indent=2) + "\n",
                                  encoding="utf-8", newline="\n")
print(build.stdout + build.stderr if build.returncode else report.get("stdout", ""))
if report["result"] != "PASS" or not report["inputs_unchanged"]:
    sys.exit(1)
