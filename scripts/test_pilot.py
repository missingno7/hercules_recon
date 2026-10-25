"""Compile/run a historical-VC5 harness containing reconstructed source only."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from env import ROOT
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from coff import COFF
from refinery_forms import function_text, replace_function


SUITES = {
    "host_frame_adapters": ["src/pc/host_frame_adapters.c", "src/pc/host_mode_packet.c", "src/pc/host_zero_callback.c"],
    "host_mode_packet": ["src/pc/host_mode_packet.c"],
    "host_frame_protocol": ["src/pc/host_frame_protocol.c", "src/pc/host_setup.c", "src/pc/virtual_file.c", "src/pc/host_callback_protocol.c", "src/pc/host_callback_protocol_externs.h", "src/shared/resource_context.h", "src/shared/engine_interface.h"],
    "frame_configuration": ["src/shared/frame_configuration.c", "src/shared/frame_configuration_row.h", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "frame_lifecycle_bridges": ["src/eng1/frame_lifecycle_bridges.c", "src/shared/frame_lifecycle_bridges.h", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "frame_bootstrap_family": ["src/shared/frame_configuration.c", "src/eng1/engine_file_bridge.c", "src/eng1/frame_bootstrap_family.c", "src/eng1/frame_lifecycle_bridges.c", "src/shared/frame_lifecycle_bridges.h", "src/shared/frame_configuration_row.h", "src/eng1/dispatcher_bridges.c", "src/eng1/frame_chain_bridge.c", "src/pc/frame_chain_write.c", "src/shared/engine_dispatch_externs.h", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "frame_chain_pipeline": ["src/pc/frame_chain_write.c", "src/eng1/frame_chain_bridge.c", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "frame_chain_writer": ["src/pc/frame_chain_write.c"],
    "engine_control": ["src/shared/engine_control.c", "src/shared/engine_interface.h", "src/shared/resource_context.h", "src/shared/resource_record.h"],
    "callback_pipeline": ["src/eng1/dispatcher_bridges.c", "src/pc/host_callback_protocol.c", "src/pc/host_callback_protocol_externs.h", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "host_callback_protocol": ["src/pc/host_callback_protocol.c", "src/pc/host_callback_protocol_externs.h", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "dispatcher_bridges": ["src/eng1/dispatcher_bridges.c", "src/eng1/engine_file_bridge.c", "src/eng1/engine_dependency_bridges.c", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "engine_dispatch": ["src/shared/engine_dispatch.c", "src/shared/engine_dispatch_externs.h", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "resource_pool_pipeline": ['src/eng1/engine_file_bridge.c', 'src/eng1/engine_dependency_bridges.c', 'src/shared/resource_rebind.c', 'src/pc/host_pool_repaired.c', 'src/pc/host_pool_lifecycle_repaired.c', 'src/pc/host_pool_counts.c', 'src/pc/pool_reverse_copy.c', 'src/pc/host_diagnostic.c', 'src/shared/engine_interface.h', 'src/shared/resource_context.h'],
    "resource_rebind": ["src/shared/resource_rebind.c", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "engine_dependency_bridges": ["src/eng1/engine_dependency_bridges.c", "src/eng1/engine_file_bridge.c", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "host_archive_pipeline": ["src/pc/host_file_service.c", "src/pc/virtual_file.c", "src/pc/archive_positions.c", "src/pc/archive_backend.c", "src/pc/archive_read.c", "src/pc/archive_lifecycle.c", "src/pc/archive_shutdown.c", "src/pc/archive_names.c", "src/pc/exe_helpers.c", "src/pc/interface_callbacks.c", "src/pc/host_diagnostic.c", "src/shared/resource_context.h"],
    "host_file_service": ["src/pc/host_file_service.c", "src/shared/resource_context.h"],
    "engine_file_bridge": ["src/eng1/engine_file_bridge.c", "src/shared/engine_interface.h", "src/shared/resource_context.h"],
    "pilot": ["src/title/pilot.c"],
    "shared": ["src/shared/object_commands.c", "src/shared/motion.c", "src/shared/relative.c",
               "src/shared/counted_list.c", "src/shared/bounds.c"],
    "engine": ["src/shared/motion.c", "src/shared/relative.c", "src/shared/frame_motion.c",
               "src/shared/bounds.c"],
    "macro": ["src/shared/macro_actor.c"],
    "frame_state": ["src/shared/frame_state.c"],
    "refinery_blind": ["prototypes/reconstruction/refinery_blind.c"],
    "effect_state": ["src/shared/effect_state.c"],
    "linked_particle": ["src/shared/linked_particle.c"],
    "actor_continuation": ["src/eng1/actor_continuation.c"],
    "release_actor": ["src/shared/release_actor_repaired.c"],
    "clear_actor": ["src/shared/cleanup_helpers.c"],
    "state_helpers": ["src/shared/cleanup_helpers.c"],
    "remove_actor": ["src/shared/remove_actor.c"],
    "resource_cleanup": ["src/eng1/cleanup_helpers_writers.c"],
    "resource_callbacks": ["src/eng1/resource_callbacks.c", "src/shared/resource_context.h"],
    "resource_metadata": ["src/eng1/resource_metadata.c", "src/shared/resource_record.h"],
    "resource_destroy": ["src/shared/resource_destroy.c", "src/shared/resource_record.h"],
    "resource_readiness": ["src/shared/resource_readiness.c", "src/shared/resource_record.h", "src/shared/resource_context.h"],
    "resource_provider": ["src/shared/resource_provider.c", "src/shared/resource_record.h", "src/shared/resource_context.h"],
    "virtual_file": ["src/pc/virtual_file.c"],
    "archive_positions": ["src/pc/archive_positions.c"],
    "archive_backend": ["src/pc/archive_backend.c", "src/pc/archive_read.c"],
    "archive_lifecycle": ["src/pc/archive_lifecycle.c", "src/pc/archive_shutdown.c"],
    "archive_pipeline": ["src/pc/virtual_file.c", "src/pc/archive_positions.c", "src/pc/archive_backend.c", "src/pc/archive_read.c", "src/pc/archive_lifecycle.c", "src/pc/archive_shutdown.c", "src/pc/archive_names.c", "src/pc/exe_helpers.c"],
    "archive_names": ["src/pc/archive_names.c", "src/pc/exe_helpers.c"],
    "token_release": ["src/eng1/cleanup_helpers_writers.c"],
    "action_registry": ["src/shared/action_registry.c"],
    "action_registry_writers": ["src/shared/action_registry_writers.c"],
    "registry_allocation": ["src/shared/registry_allocation.c"],
    "actor_factories": ["src/shared/actor_factories.c"],
    "host_pool": ["src/pc/host_pool_repaired.c"],
    "actor_pool_reset_basic": ["src/shared/actor_pool_reset_repaired.c"],
    "actor_pool_reset": ["src/shared/actor_pool_reset_repaired.c"],
    "actor_pools": ["src/eng3/actor_pools.c"],
    "host_pool_lifecycle": ["src/pc/host_pool_lifecycle_repaired.c"],
    "host_arena_reset": ["src/pc/host_arena_reset.c"],
    "host_pool_shrink": ["src/pc/host_pool_shrink.c"],
    "host_pool_counts": ["src/pc/host_pool_counts.c"],
    "host_reverse_boundary": ["tests/host_reverse_boundary_semantics.c"],
    "pool_reverse_copy": ["src/pc/pool_reverse_copy.c"],
}

# Most fixtures include one reconstructed source directly. Integration fixtures
# link actual separate translation units instead of replacing callees.
EXTRA_UNITS = {
    "archive_backend": ["src/pc/archive_read.c"],
    "archive_lifecycle": ["src/pc/archive_shutdown.c"],
    "archive_names": ["src/pc/exe_helpers.c"],
    "host_frame_adapters": ["src/pc/host_frame_adapters.c", "src/pc/host_mode_packet.c", "src/pc/host_zero_callback.c"],
    "host_mode_packet": ["src/pc/host_mode_packet.c"],
    "host_frame_protocol": ["src/pc/host_frame_protocol.c", "src/pc/host_setup.c", "src/pc/virtual_file.c", "src/pc/host_callback_protocol.c"],
    "frame_configuration": ["src/shared/frame_configuration.c"],
    "frame_lifecycle_bridges": ["src/eng1/frame_lifecycle_bridges.c"],
    "frame_bootstrap_family": ["src/shared/frame_configuration.c", "src/eng1/engine_file_bridge.c", "src/eng1/frame_bootstrap_family.c", "src/eng1/frame_lifecycle_bridges.c", "src/eng1/dispatcher_bridges.c", "src/eng1/frame_chain_bridge.c", "src/pc/frame_chain_write.c"],
    "frame_chain_pipeline": ["src/pc/frame_chain_write.c", "src/eng1/frame_chain_bridge.c"],
    "frame_chain_writer": ["src/pc/frame_chain_write.c"],
    "engine_control": ["src/shared/engine_control.c"],
    "callback_pipeline": ["src/eng1/dispatcher_bridges.c", "src/pc/host_callback_protocol.c"],
    "host_callback_protocol": ["src/pc/host_callback_protocol.c"],
    "engine_dispatch": ["src/shared/engine_dispatch.c"],
    "resource_pool_pipeline": [s for s in SUITES["resource_pool_pipeline"] if s.endswith(".c")],
    "host_archive_pipeline": [s for s in SUITES["host_archive_pipeline"] if s.endswith(".c")],
    "archive_pipeline": SUITES["archive_pipeline"],
}


def notification_fixture(source, output, toolchain):
    """Keep unrelated host setup dependencies out of this bounded fixture.

    Omit other definitions in an ignored projection, then require the complete
    notification contribution to equal its fresh canonical source contribution.
    The game unit and its four-argument interface remain unchanged.
    """
    flags = ['/O2', '/ML']
    original, _ = compile_source(source, flags, toolchain)
    original_coff = COFF(original)
    symbol = '_host_frame_notify_zero'
    text = source.read_text()
    for row in original_coff.symbols.values():
        if row['section'] > 0 and row['type'] & 0x20 and row['name'] != symbol:
            body = function_text(text, row['name'][1:])
            text = replace_function(text, row['name'][1:], body[:body.index('{')].rstrip() + ';')
    projected = output / 'host_setup_notification.c'
    # Resolve the original literal headers from the projection's ignored folder.
    text = text.replace('#include "host_setup_storage.h"',
                        '#include "' + (source.parent / 'host_setup_storage.h').as_posix() + '"')
    projected.write_text(text, newline='\n')
    candidate, _ = compile_source(projected, flags, toolchain)
    left, right = original_coff.function(symbol), COFF(candidate).function(symbol)
    if any(left[k] != right[k] for k in ('data', 'size', 'flags', 'relocations')):
        raise ValueError('Notification fixture changed its complete canonical contribution')
    return projected, dict(source=source.relative_to(ROOT).as_posix(),
                           source_sha256=digest(source.read_bytes()),
                           projection_sha256=digest(projected.read_bytes()),
                           complete_contribution_equal=True, function=symbol,
                           bytes=left['size'])


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
    units = [ROOT / s for s in EXTRA_UNITS.get(suite, [])]
    projections = []
    if suite == 'host_frame_protocol':
        owner = ROOT / 'src/pc/host_setup.c'
        projected, proof = notification_fixture(owner, output, toolchain)
        units[units.index(owner)] = projected
        projections.append(proof)
    object_output = str(output) + os.sep if units else str(output / "semantics.obj")
    # This shared-text control experiment was diagnosed under /Od without /Gy.
    flags = ["/Od"] if suite == "engine_control" else ["/O2", "/Gy"]
    command = [str(toolchain / "bin/cl.exe"), "/nologo", *flags, "/ML",
               "/Fo" + object_output,
               "/Fe" + str(executable), str(source), *map(str, units), "/link", "/INCREMENTAL:NO"]
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
              "fixture_projections": projections,
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
