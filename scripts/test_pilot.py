"""Compile/run a historical-VC5 harness containing reconstructed source only."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain


SUITES = {
    "pilot": ["src/title/pilot.c"],
    "shared": ["src/shared/object_commands.c", "src/shared/motion.c", "src/shared/relative.c",
               "src/shared/counted_list.c", "src/shared/bounds.c"],
    "engine": ["src/shared/motion.c", "src/shared/relative.c", "src/shared/frame_motion.c",
               "src/shared/bounds.c"],
    "macro": ["calibration/macro_actor.c"],
    "frame_state": ["src/shared/frame_state.c"],
    "refinery_blind": ["calibration/refinery_blind.c"],
    "effect_state": ["src/shared/effect_state.c"],
    "linked_particle": ["src/shared/linked_particle.c"],
    "actor_continuation": ["calibration/actor_continuation.c"],
    "release_actor": ["calibration/release_actor_repaired.c"],
    "clear_actor": ["calibration/cleanup_helpers.c"],
    "state_helpers": ["calibration/cleanup_helpers.c"],
    "remove_actor": ["calibration/remove_actor.c"],
    "resource_cleanup": ["calibration/cleanup_helpers_writers.c"],
    "resource_callbacks": ["calibration/resource_callbacks.c", "calibration/resource_context.h"],
    "resource_metadata": ["calibration/resource_metadata.c"],
    "resource_destroy": ["calibration/resource_destroy.c", "calibration/resource_record.h"],
    "resource_readiness": ["calibration/resource_readiness.c", "calibration/resource_record.h", "calibration/resource_context.h"],
    "resource_provider": ["calibration/resource_provider.c", "calibration/resource_record.h", "calibration/resource_context.h"],
    "token_release": ["calibration/cleanup_helpers_writers.c"],
    "action_registry": ["calibration/action_registry.c"],
    "action_registry_writers": ["calibration/action_registry_writers.c"],
    "registry_allocation": ["calibration/registry_allocation.c"],
    "actor_factories": ["calibration/actor_factories.c"],
    "host_pool": ["calibration/host_pool_repaired.c"],
    "actor_pool_reset_basic": ["calibration/actor_pool_reset_repaired.c"],
    "actor_pool_reset": ["calibration/actor_pool_reset_repaired.c"],
    "actor_pools": ["calibration/actor_pools.c"],
    "host_pool_lifecycle": ["calibration/host_pool_lifecycle_repaired.c"],
    "host_arena_reset": ["calibration/host_arena_reset.c"],
    "host_pool_shrink": ["calibration/host_pool_shrink.c"],
    "host_pool_counts": ["calibration/host_pool_counts.c"],
    "host_reverse_boundary": ["tests/host_reverse_boundary_semantics.c"],
    "pool_reverse_copy": ["calibration/pool_reverse_copy.c"],
}


def run(toolchain, suite="pilot"):
    identity = verify_toolchain(toolchain)
    parent = ROOT / "build/tests"
    parent.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=suite+"-", dir=parent))
    source = ROOT / f"tests/{suite}_semantics.c"
    executable = output / "semantics.exe"
    environment = os.environ.copy()
    for key in ("CL", "_CL_", "LINK", "_LINK_"):
        environment.pop(key, None)
    environment["PATH"] = str(toolchain / "bin") + os.pathsep + environment.get("PATH", "")
    environment["INCLUDE"] = str(toolchain / "include")
    environment["LIB"] = str(toolchain / "lib")
    command = [str(toolchain / "bin/cl.exe"), "/nologo", "/O2", "/Gy", "/ML",
               "/Fo" + str(output / "semantics.obj"),
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
              "source_sha256": {s: digest((ROOT / s).read_bytes()) for s in SUITES[suite]},
              "executable_sha256": digest(executable.read_bytes()),
              "output_directory": str(output.relative_to(ROOT)),
              "scope": "semantic tests of reconstructed C source; no original executable or DLL is loaded or run"}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"{suite}: {report['stdout']}; report: {output.relative_to(ROOT) / 'report.json'}")
    if result.returncode:
        raise SystemExit(result.returncode)
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", type=Path, default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--suite", choices=["all", *SUITES], default="all")
    args = parser.parse_args()
    for suite in SUITES if args.suite == "all" else [args.suite]:
        run(args.toolchain, suite)
