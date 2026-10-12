"""Fresh pinned VC5 source fixtures for ENG1 entry and host forwarding."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain

def context(source, seen=None):
    seen = set() if seen is None else seen
    source = source.resolve()
    if source in seen:
        return seen
    seen.add(source)
    for include in re.findall(r'^\s*#\s*include\s*"([^\"]+)"', source.read_text(), re.M):
        context(source.parent / include, seen)
    return seen

def run(output, wrapper_fixture):
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    output = output.resolve()
    if not any(output.is_relative_to(ROOT / folder) for folder in ('work', 'build')):
        raise ValueError('Fixture output must be in ignored work/ or build/')
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    fixtures = {
        'entry': ['tests/eng1_entry_semantics.c', 'src/eng1/behavioral/entry.c',
                  'calibration/engine_interface_storage.c'],
        'host_wrappers': [str(wrapper_fixture), 'src/eng1/behavioral/host_wrappers.c'],
        'camera': ['tests/eng1_camera_semantics.c'],
        'host_wrappers_extra': ['tests/eng1_host_wrappers_extra.c', 'src/eng1/behavioral/host_wrappers_extra.c'],
        'resource_host_forwarders': ['tests/eng1_resource_host_forwarders.c', 'src/eng1/behavioral/resource_host_forwarders.c'],
        'startup_host_forwarders': ['tests/eng1_startup_host_forwarders.c', 'src/eng1/behavioral/startup_host_forwarders.c'],
        'audio_host': ['tests/eng1_audio_host.c', 'src/eng1/behavioral/audio_host.c'],
        'host_control_wrappers': ['tests/eng1_host_control_wrappers.c', 'src/eng1/behavioral/host_control_wrappers.c'],
        'host_color_callback': ['tests/eng1_host_color_callback.c', 'src/eng1/behavioral/host_color_callback.c'],
        'geometry': ['tests/eng1_geometry_semantics.c'],
        'mesh_traversal': ['tests/eng1_mesh_traversal.c'],
        'color': ['tests/eng1_color_semantics.c'],
        'program_interpreters': ['tests/eng1_program_interpreters.c',
                                 'src/eng1/behavioral/interpreters.c', 'src/shared/frame_state.c'],
        'raw_rng_callback': ['tests/eng1_raw_rng_callback.c', 'src/eng1/behavioral/host_control_wrappers.c'],
        'default_callbacks': ['tests/eng1_default_callback_semantics.c',
                              'src/eng1/behavioral/eng1_default_callbacks.c',
                              'src/eng1/behavioral/default_storage.c'],
        'spatial_effects': ['tests/eng1_spatial_effects.c', 'src/eng1/behavioral/spatial_effects.c'],
        'frame_token_setup': ['tests/eng1_frame_token_setup.c',
                              'src/eng1/behavioral/frame_token_setup.c', 'src/eng1/behavioral/frame_token_storage.c'],
        'frame_data_prefixes': ['tests/eng1_frame_data_prefixes.c',
                                'src/eng1/behavioral/frame_data_prefixes.c', 'src/eng1/behavioral/incomplete.c'],
        'resource_completion': ['tests/eng1_resource_completion.c', 'src/eng1/behavioral/resource_completion.c', 'src/eng1/behavioral/resource_completion_data.c', 'calibration/engine_dependency_bridges.c'],
        'cold_start_32b10': ['tests/eng1_cold_start_32b10.c', 'src/eng1/behavioral/cold_start_32b10.c', 'src/eng1/behavioral/viewport_state_data.c', 'calibration/engine_file_bridge.c', 'calibration/dispatcher_bridges.c', 'calibration/frame_lifecycle_bridges.c', 'src/eng1/behavioral/host_wrappers.c', 'calibration/engine_interface_storage.c', 'src/eng1/behavioral/runtime_storage.c'],
        'cold_start_32bd0': ['tests/eng1_cold_start_32bd0.c', 'tests/eng1_cold_frame_fixture_state.c', 'src/eng1/behavioral/cold_start_32bd0.c', 'src/eng1/behavioral/cold_start_frame_data.c', 'src/eng1/behavioral/viewport_state_data.c', 'calibration/engine_interface_storage.c', 'calibration/frame_lifecycle_bridges.c'],
        'setup_source_spies': ['tests/eng1_setup_source_spies.c', 'src/eng1/behavioral/setup_source.c', 'src/eng1/behavioral/setup_shared_state.c', 'src/eng1/behavioral/setup_leaf_dependencies.c', 'src/eng1/behavioral/script_54558_owner.c', 'src/eng1/behavioral/simulation_data_owners.c'],
        'pool_startup_spies': ['tests/eng1_pool_startup_spies.c', 'src/eng1/behavioral/pool_startup.c'],
        'pool_startup_initializers': ['tests/eng1_pool_startup_initializers.c', 'src/eng1/behavioral/pool_startup.c', 'calibration/actor_pool_reset_repaired.c'],
        'resource_rebind': ['tests/resource_rebind_semantics.c'],
        'worklist_factories': ['tests/eng1_worklist_factory_spies.c', 'src/eng1/behavioral/worklist_factories.c', 'src/eng1/behavioral/worklist_read_guards.c', 'calibration/actor_pool_reset_repaired.c'],
        'command_table': ['tests/eng1_command_table.c', 'src/eng1/behavioral/command_table.c', 'src/eng1/behavioral/command_table_storage.c', 'calibration/engine_file_bridge.c', 'calibration/engine_interface_storage.c'],
        'forest_entry': ['tests/eng1_forest_entry.c', 'src/eng1/behavioral/forest_entry.c', 'src/eng1/behavioral/forest_entry_state_data.c', 'src/eng1/behavioral/forest_entry_words_data.c', 'src/eng1/behavioral/actor_pipeline_storage.c'],
        'context_dispatch': ['tests/eng1_context_dispatch.c', 'src/eng1/behavioral/context_dispatch.c'],
        'cold_runtime_setup': ['tests/eng1_cold_runtime_setup.c', 'src/eng1/behavioral/cold_sequence_setup.c', 'src/eng1/behavioral/cold_sequence_storage.c', 'src/eng1/behavioral/engine_fx_reset.c', 'src/eng1/behavioral/audio_stop_wrapper.c', 'src/eng1/behavioral/runtime_storage.c', 'calibration/engine_fx_storage.c', 'src/eng1/behavioral/host_wrappers.c'],
        'startup_frame_helpers': ['tests/eng1_startup_frame_helpers.c',
                                  'src/eng1/behavioral/startup_frame_helpers.c',
                                  'src/eng1/behavioral/startup_frame_storage.c'],
    }
    spatial_sources = ['src/eng1/behavioral/spatial_read_guard.c', 'src/eng1/behavioral/initial_spatial_roots.c', 'src/eng1/behavioral/incomplete.c']
    fixtures['default_callbacks'] += spatial_sources
    fixtures['spatial_effects'] += spatial_sources
    fixtures['spatial_read_guard'] = ['tests/eng1_spatial_read_guard.c', *spatial_sources[:-1]]
    initial_sources = ['tests/eng1_initial_guard_owner_spies.c', 'src/eng1/behavioral/initial_read_guards.c', 'src/eng1/behavioral/initial_custom_roots.c', 'src/eng1/behavioral/initial_motion_roots.c', 'src/eng1/behavioral/initial_unreadable_identities.c', 'src/eng1/behavioral/forest_entry_words_data.c']
    fixtures['default_callbacks'] += [*initial_sources, 'tests/eng1_custom_data_callback_spies.c']
    fixtures['spatial_effects'] += [*initial_sources, 'tests/eng1_custom_data_callback_spies.c']
    fixtures['frame_token_setup'] += [*initial_sources, 'tests/eng1_custom_data_callback_spies.c', 'src/shared/frame_motion.c', 'src/eng1/behavioral/incomplete.c']
    fixtures['initial_read_guards'] = ['tests/eng1_initial_read_guards.c', *initial_sources, 'src/eng1/behavioral/frame_token_setup.c', 'src/shared/frame_motion.c']
    guard_sources = ['src/eng1/behavioral/read_guard.c',
                     'src/eng1/behavioral/flags_word_programs.c',
                     'src/eng1/behavioral/provisional_frame_pairs.c',
                     'src/eng1/behavioral/shared_selector_assets.c',
                     'src/eng1/behavioral/finite_frame_prefixes.c']
    guard_sources += ['src/eng1/behavioral/initial_zero_word_program.c']
    fixtures['program_interpreters'] += guard_sources
    fixtures['program_interpreters'] += ['src/eng1/behavioral/incomplete.c']
    fixtures['runtime_read_guard'] = ['tests/eng1_runtime_read_guard.c',
                                    'src/eng1/behavioral/interpreters.c',
                                    'src/shared/frame_state.c', *guard_sources]
    fixtures['initial_zero_word_runtime'] = ['tests/eng1_initial_zero_word_runtime.c', 'src/eng1/behavioral/interpreters.c', 'src/shared/frame_state.c', *guard_sources]
    fixture_flags = {'worklist_factories': ['/DENG1_WORKLIST_GUARD_FIXTURE']}
    reports = []
    for name, names in fixtures.items():
        directory = output / name
        directory.mkdir(parents=True, exist_ok=True)
        units = [ROOT / p for p in names]
        if len({p.stem.lower() for p in units}) != len(units):
            raise ValueError('Fixture object basenames collide: ' + name)
        inputs = {str(p.relative_to(ROOT)):digest(p.read_bytes())
                  for source in units for p in context(source)}
        executable = directory / 'fixture.exe'
        executable.unlink(missing_ok=True)
        command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/O2', '/Gy', '/MT',
                   *fixture_flags.get(name, []), '/Fo' + str(directory) + os.sep, '/Fe' + str(executable),
                   *map(str, units), '/link', '/INCREMENTAL:NO']
        built = subprocess.run(command, cwd=directory, env=env, capture_output=True, text=True, timeout=60)
        (directory / 'compile.log').write_text(built.stdout + built.stderr)
        report = dict(name=name, toolchain=toolchain, command=command,
                      input_sha256=inputs, compile_exit_code=built.returncode)
        if built.returncode == 0:
            tested = subprocess.run([str(executable)], cwd=directory, env=env,
                                    capture_output=True, text=True, timeout=30)
            report.update(exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr,
                          executable_sha256=digest(executable.read_bytes()))
        else:
            report.update(exit_code=None, stdout='', stderr=built.stderr)
        report['inputs_unchanged'] = all(digest((ROOT / p).read_bytes()) == sha
                                         for p, sha in inputs.items())
        (directory / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
        reports.append(report)
        print(name + ': ' + ('PASS' if report['exit_code'] == 0 and report['inputs_unchanged'] else 'FAIL'))
    result = dict(scope='Adopted reconstructed source with test-only spies; no original code executed, no game or full DLL validation.', fixtures=reports)
    (output / 'report.json').write_bytes((json.dumps(result, indent=2) + '\n').encode())
    return 0 if all(r['exit_code'] == 0 and r['inputs_unchanged'] for r in reports) else 1

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_behavioral/entry_and_wrappers')
    parser.add_argument('--wrapper-fixture', type=Path, default=Path('tests/eng1_host_wrappers.c'))
    args = parser.parse_args()
    raise SystemExit(run(args.output, args.wrapper_fixture))
