"""Compile/run a historical-VC5 harness containing reconstructed source only."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain


def run(toolchain):
    identity = verify_toolchain(toolchain)
    parent = ROOT / "build/tests"
    parent.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="pilot-", dir=parent))
    source = ROOT / "tests/pilot_semantics.c"
    executable = output / "pilot_semantics.exe"
    environment = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        environment.pop(key, None)
    environment["PATH"] = str(toolchain / "bin") + os.pathsep + environment.get("PATH", "")
    environment["INCLUDE"] = str(toolchain / "include")
    environment["LIB"] = str(toolchain / "lib")
    command = [str(toolchain / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/ML",
               "/Fo" + str(output / "pilot_semantics.obj"),
               "/Fe" + str(executable), str(source), "/link", "/INCREMENTAL:NO"]
    compiled = subprocess.run(command, cwd=output, env=environment, capture_output=True,
                              text=True, timeout=60)
    (output / "compile.log").write_text(compiled.stdout + compiled.stderr)
    if compiled.returncode or not executable.is_file():
        raise RuntimeError(f"Harness compile failed ({compiled.returncode}); see {output / 'compile.log'}")
    result = subprocess.run([str(executable)], cwd=output, env=environment,
                            capture_output=True, text=True, timeout=30)
    (output / "run.log").write_text(result.stdout + result.stderr)
    report = {"toolchain": identity, "command": command, "exit_code": result.returncode,
              "stdout": result.stdout.strip(), "stderr": result.stderr.strip(),
              "harness_sha256": digest(source.read_bytes()),
              "source_sha256": digest((ROOT / "src/title/pilot.c").read_bytes()),
              "executable_sha256": digest(executable.read_bytes()),
              "output_directory": str(output.relative_to(ROOT)),
              "scope": "semantic tests of reconstructed C source; no original executable or DLL is loaded or run"}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if result.returncode:
        raise SystemExit(result.returncode)
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", type=Path, default=DEFAULT_TOOLCHAIN)
    run(parser.parse_args().toolchain)
