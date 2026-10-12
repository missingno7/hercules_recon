"""Fresh VC5 bounded comparisons against the unpatched ENG1 x86 oracle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from match import DEFAULT_TOOLCHAIN, verify_toolchain
from pe import PE
from title_behavioral import context_files
from refinery_forms import function_text, replace_function

PROFILES = {
    'alternate-bootstrap': dict(
        sources=['tests/eng1_alternate_bootstrap_differential.c',
                 'src/eng1/behavioral/alternate_bootstrap_12960.c',
                 'src/eng1/behavioral/alternate_bootstrap_data.c',
                 'src/eng1/behavioral/simulation_data_owners.c',
                 'src/eng1/behavioral/actor_storage.c',
                 'calibration/actor_pools.c'],
        spans=[(0x12960,117)], cases=3,
        scope='Three finite native alternate-pool bootstrap comparisons with the actual production initializer: original lookup data, reversible mutable DWORD/WORD changes, and zero lookup values. Full ten-record pool, leading records, trailing canaries and all eight initialized actors checked. One source null-pool guard; no arbitrary allocation capacity or gameplay claim.'),
    'actor-bootstrap': dict(
        sources=['tests/eng1_actor_bootstrap_differential.c',
                 'src/eng1/behavioral/actor_bootstrap_1e50.c',
                 'calibration/actor_pools.c',
                 'calibration/engine_interface_storage.c',
                 'src/eng1/behavioral/actor_storage.c',
                 'src/eng1/behavioral/actor_pipeline_storage.c',
                 'src/eng1/behavioral/camera_storage.c',
                 'src/eng1/behavioral/worklist_read_guards.c'],
        spans=[(0x1e50,240)], cases=3,
        flags={'src/eng1/behavioral/worklist_read_guards.c':[
            '/DENG1_WORKLIST_GUARD_FIXTURE']},
        scope='Three finite native bootstrap comparisons: zero kind, two live callback reloads, nonzero descriptor subrow and 03ff mask with high coordinate WORDs. Full actor bytes, camera values and callback actors compared. Actual normal initializer; controlled descriptor/action rows are test data. No complete capacities, full return ABI or gameplay claim.'),
    'spawn-services': dict(
        sources=['tests/eng1_spawn_services_differential.c',
                 'src/eng1/behavioral/spawn_services.c',
                 'calibration/actor_pools.c',
                 'calibration/actor_pool_reset_repaired.c',
                 'src/eng1/behavioral/worklist_read_guards.c',
                 'calibration/actor_factories.c',
                 'calibration/eng1/actor_middle_init.c',
                 'src/shared/frame_state.c'],
        spans=[(0x300c0,192),(0x30180,272),(0x277e0,128)], cases=6,
        production_omissions=True,
        flags={'src/eng1/behavioral/worklist_read_guards.c':['/DENG1_WORKLIST_GUARD_FIXTURE']},
        scope='Six finite native spawn selector-family invocations plus normal/alternate exhaustion controls. Actual production pool/init composition, existing factories, source append_chain and controlled callback/asset tables.30180 treatedvoid for known direct callers; native callbackEAX marker distinguishes superseded actor-return claim. Indirect return consumers and full table/pool capacities remain unverified.'),
    'action-spawn': dict(
        sources=['tests/eng1_action_spawn_differential.c',
                 'src/eng1/behavioral/action_spawn_27d40.c',
                 'calibration/actor_factories.c',
                 'calibration/actor_pools.c',
                 'calibration/actor_detach_helpers.c',
                 'src/eng1/behavioral/actor_storage.c'],
        spans=[(0x27d40,100)], cases=5,
        flags={p:[
            '/Dg_normal_cursor_primary=g_configuration_word_71308',
            '/Dg_normal_limit_primary=g_configuration_word_713b4',
            '/Dg_normal_limit_wrap=g_configuration_word_713a8',
            '/Dg_normal_cursor_alternate=g_configuration_word_713c4',
            '/Dg_normal_limit_wrap_start=g_configuration_word_71378',
            '/Dg_normal_factory_assets=g_configuration_dword_71388',
            '/Dg_alternate_cursor_primary=g_configuration_word_71374',
            '/Dg_alternate_limit_primary=g_configuration_word_713ba',
            '/Dg_alternate_limit_wrap=g_configuration_word_713b6',
            '/Dg_alternate_cursor_alternate=g_configuration_word_71430',
            '/Dg_alternate_limit_wrap_start=g_configuration_word_7138c',
            '/Dg_alternate_factory_assets=g_configuration_dword_713a4']
            for p in ['calibration/actor_factories.c','calibration/actor_pools.c']},
        scope='Five finite native child-link comparisons with actual existing factory/pool/init/detach sources. Controlled config mirrors and synthetic asset rows; source release spy restricted to zero-kind no-op. Blocked/success/exhausted/parent-pool-alias paths tested. No production table/pool capacity, nonzero cleanup or universal equivalence claim.'),
    'grid-scan': dict(
        sources=['tests/eng1_grid_query_20b50_differential.c',
                 'src/eng1/behavioral/grid_query_20b50.c',
                 'src/eng1/behavioral/query_20990.c',
                 'src/eng1/behavioral/resource_data.c',
                 'src/eng1/behavioral/grid_query_data.c',
                 'src/eng1/behavioral/payload_dispatch_data.c'],
        spans=[(0x20b50,480),(0x20990,448)], cases=4,
        scope='Four finite native scan comparisons with the actual source decoder: sentinel, exclusive extent, y-based reverse seed and one-past-width read. One separately counted source guard. Row/header/stream allocation capacities remain unproven; no source identity, full grid domain or gameplay claim.'),
    'query-decoder': dict(
        sources=['tests/eng1_query_20990_differential.c',
                 'src/eng1/behavioral/query_20990.c',
                 'src/eng1/behavioral/grid_query_data.c',
                 'src/eng1/behavioral/payload_dispatch_data.c'],
        spans=[(0x20990,448)], cases=15,
        scope='Fifteen finite native compressed-query comparisons, high sample64/65/127 controls in each encoding, scalar/row/bilinear interpolation and live shared coefficient mutations restored afterward. One source-only arithmetic control and two unsupported-owner guards separately counted. Program/table capacities and wider input domains remain unproven.'),
    'actor-bounds': dict(
        sources=['tests/eng1_actor_bounds_differential.c',
                 'src/eng1/behavioral/actor_bounds_2a880.c',
                 'src/eng1/behavioral/extent_producer_30e40.c',
                 'calibration/resource_data.c',
                 'calibration/resource_metadata.c',
                 'src/eng1/behavioral/resource_data.c'],
        spans=[(0x2a880,208),(0x30e40,448),(0x31ec0,400)], cases=3,
        flags={'src/eng1/behavioral/actor_bounds_2a880.c':[
            '/Dextent_producer_30e40=eng1_fixture_0030e40']},
        scope='Three finite native bounds comparisons: real extent failure and two positive output/actor aliases through resource and profile paths. Eight separately counted source spies. Canonical extent/metadata/data providers used; state3 resource path, no readiness callback or universal geometry proof.'),
    'extent-producer': dict(
        sources=['tests/eng1_extent_producer_differential.c',
                 'src/eng1/behavioral/extent_producer_30e40.c',
                 'calibration/resource_data.c',
                 'calibration/resource_metadata.c',
                 'src/eng1/behavioral/resource_data.c'],
        spans=[(0x30e40,448),(0x31ec0,400)], cases=20,
        scope='Twenty direct native extent comparisons with controlled dynamic profile and resource data restored afterward. Actual source metadata reader and data owners. Modes0/1/2/out-of-count, signed count, packed sums and scale variants tested; negative modes, resource readiness and full dynamic allocation capacities remain unverified.'),
    'grid-query': dict(
        sources=['tests/eng1_grid_query_differential.c',
                 'src/eng1/behavioral/grid_query_20d30.c',
                 'src/eng1/behavioral/grid_query_data.c',
                 'src/eng1/behavioral/resource_data.c',
                 'calibration/resource_rebind.c',
                 'calibration/engine_interface_storage.c',
                 'src/eng1/behavioral/payload_dispatch_data.c'],
        spans=[(0x20d30, 368), (0x208f0, 160)], cases=14,
        flags={'calibration/resource_rebind.c':[
            '/Dg_group_cumulative_70e4c=g_resource_group_state_70e4c']},
        scope='Fourteen finite low-WORD grid-query comparisons with real native/source row producers, measured104B coefficient prefix, and reversible live sequence-alias mutations. Five source-only unsupported-read guards. First coefficient lanes0..13/factors0..12 are measured views, not full caller capacity; row/header allocation extents and fullEAX return remain unverified.'),
    'index-query': dict(
        sources=['tests/eng1_index_query_differential.c',
                 'src/eng1/behavioral/query_2a170.c',
                 'src/eng1/behavioral/query_match_state.c',
                 'src/eng1/behavioral/actor_pipeline_storage.c',
                 'src/eng1/behavioral/point_in_box_2a810.c'],
        spans=[(0x2a170, 176), (0x2a810, 112)], cases=2,
        flags={'src/eng1/behavioral/query_2a170.c':['/Dpoint_in_box_2a810=fixture_point_in_box']},
        scope='Two original-x86 root comparisons cover only empty/negative worklists with sentinel grid fallback. Seven separate native box comparisons use actual reused C. Four source spies cover live traversal and early hits, plus two actual-read prefix guards. Bounds/grid services are controlled endpoints here and have independently tested source providers; no active native traversal or gameplay claim.'),
    'query': dict(
        sources=['tests/eng1_query_30010_differential.c',
                 'src/eng1/behavioral/query_30010.c',
                 'src/eng1/behavioral/initial_spatial_roots.c',
                 'src/eng1/behavioral/spatial_read_guard.c'],
        spans=[(0x30010, 64)], cases=4,
        scope='Four native zero-index query comparisons with independently controlled writable table WORD/mask values. Five separately counted source spies, two prefix/owner guards and64 initial data WORD comparisons. Index-query2a170 has a separate source provider; this fixture controls that endpoint and uses isolated713bc test storage. No nonzero native index, full collision or gameplay claim.'),
    'actor-update': dict(
        sources=['tests/eng1_actor_update_2a290_differential.c',
                 'src/eng1/behavioral/actor_update_2a290.c',
                 'calibration/engine_interface_storage.c'],
        spans=[(0x2a290, 160)], cases=6,
        scope='Six native/source full296B actor comparisons using test-only original20b50 service. Its7fff7fff result covers the upper-return path. A separate source-only signed-coordinate/parameter argument spy. Other arithmetic branches remain instruction-supported; no source identity, complete service or gameplay claim.'),
    'player-setup': dict(
        sources=['tests/eng1_player_setup_continuation_differential.c',
                 'src/eng1/behavioral/player_setup_continuation.c',
                 'src/eng1/behavioral/setup_shared_state.c',
                 'src/eng1/behavioral/script_54558_owner.c',
                 'src/eng1/behavioral/simulation_data_owners.c',
                 'src/eng1/behavioral/simulation_storage.c',
                 'src/eng1/behavioral/actor_storage.c',
                 'src/eng1/behavioral/runtime_storage.c',
                 'src/eng1/behavioral/actor_pipeline_storage.c',
                 'src/eng1/behavioral/dispatch_storage.c',
                 'calibration/engine_interface_storage.c'],
        spans=[(0x15d00, 2704)], cases=8,
        scope='Eight finite original-x86 player dispatcher cases: default53820 to53830, bounded54558 record21, timer return, child orientation aliasing and four collision-script operand discriminators. Separate source-only query/RNG mutation spy. Missing services are fixture endpoints; no full opcode, downstream service, gameplay or universal equivalence claim.'),
    'setup-leaves': dict(
        sources=['tests/eng1_setup_leaf_differential.c',
                 'src/eng1/behavioral/setup_leaf_dependencies.c',
                 'src/eng1/behavioral/setup_shared_state.c',
                 'src/eng1/behavioral/viewport_state_data.c',
                 'src/eng1/behavioral/actor_storage.c',
                 'src/eng1/behavioral/runtime_storage.c',
                 'calibration/engine_interface_storage.c'],
        spans=[(0x11fc0, 128), (0x15670, 48)], cases=3,
        scope='Three finite direct original-x86 memory comparisons for actual source11fc0/15670 and measured source providers. Exact-width context/global writes and overlapping pool/collision windows; no source identity, full setup, gameplay or universal acceptance.'),
    'simulation-helpers': dict(
        sources=['tests/eng1_simulation_helpers_differential.c',
                 'src/shared/frame_state.c',
                 'src/eng1/behavioral/simulation_data_owners.c',
                 'src/eng1/behavioral/simulation_transition_storage.c',
                 'src/eng1/behavioral/interpreters.c',
                 'src/eng1/behavioral/actor_storage.c',
                 'src/eng1/behavioral/resource_data.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/engine_file_bridge.c',
                 'calibration/resource_data.c',
                 'calibration/resource_callbacks.c'],
        spans=[(0x12620, 345), (0x12790, 269), (0x29260, 32)], cases=15,
        scope='Fifteen original-x86 simulation mode/row cases, actual source lookup/program owners, reset, cancel, file bridge and WORD interpreter. Actor program pointers normalized by nativeRVA only for comparison; source pointer cells are relocatable. Only caller indices0..7 and positive-ending interpreter path tested. Audio expiry and wider capacities remain unverified.'),
    'payload-dispatch': dict(
        sources=['tests/eng1_payload_dispatch_differential.c',
                 'src/eng1/behavioral/payload_dispatch.c',
                 'src/eng1/behavioral/payload_dispatch_data.c',
                 'calibration/resource_data.c',
                 'calibration/dispatcher_bridges.c',
                 'calibration/engine_interface_storage.c',
                 'src/eng1/behavioral/dispatch_storage.c',
                 'src/eng1/behavioral/frame_commands.c',
                 'src/eng1/behavioral/host_wrappers.c',
                 'src/eng1/behavioral/level_storage.c'],
        spans=[(0x4ca0, 1072)], cases=6,
        scope='Four payload-family controls and two queue-flush cases versus unpatched x86, with actual source queue consumer and controlled host endpoints. Optional pinned captured startup prefix adds one case; its bytes remain private and its seed state is not a complete live snapshot.'),
    'dialog-frame': dict(
        sources=['tests/eng1_dialog_frame_differential.c',
                 'tests/eng1_draw_fixture_state.c',
                 'src/eng1/behavioral/frame_runtime_callees.c',
                 'src/eng1/behavioral/frame_program_data.c',
                 'src/eng1/behavioral/draw_commands.c',
                 'src/eng1/behavioral/cold_start_frame_data.c',
                 'src/eng1/behavioral/viewport_state_data.c',
                 'src/eng1/behavioral/color_storage.c',
                 'src/eng1/behavioral/runtime_storage.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/frame_lifecycle_bridges.c'],
        spans=[(0x1f40, 0x1a1), (0x2450, 0x73), (0x24d0, 0x10),
               (0x2c60, 0x40), (0x2c9b0, 0x10)], cases=147,
        success_marker='2c60, 132 state-value and 6 branch-context 1f40, and 6 direct 2450 native differentials passed; missing-NUL guard trapped at the measured prefix',
        scope='147 finite native comparisons plus a source-only missing-NUL guard discriminator. Actual source dialog/data/command writers; host endpoints are controlled spies. Mutable strings, pointer/operation changes, width/context/frame/cursor callback mutation and DWORD wrap covered. No pixel or gameplay claim.'),
    'simulation-root': dict(
        sources=['tests/eng1_simulation_iteration_differential.c',
                 'src/eng1/behavioral/simulation_storage.c'],
        spans=[(0x156a0, 209)], cases=6,
        scope='Six cases/36 checks for the actual source156a0 body and scalar storage. Standalone test-only wrappers execute original15780 and12620, with bounded zero-row preconditions; production contains separate C dependencies. No fully source simulation or gameplay claim.'),
    'resource-links': dict(
        sources=['tests/eng1_resource_link_differential.c',
                 'src/eng1/behavioral/resource_link_initialization.c',
                 'calibration/resource_data.c'],
        spans=[(0x39b0, 0xa0)], cases=2,
        scope='Two seeded full256-row resource-link prefixes versus unpatched x86; masks/descriptors and preserved successor keys. No wider capacity, gameplay or universal equivalence claim.'),
    'command-writers': dict(
        sources=['tests/eng1_draw_commands_differential.c',
                 'tests/eng1_draw_fixture_state.c',
                 'src/eng1/behavioral/draw_commands.c',
                 'src/eng1/behavioral/cold_start_frame_data.c',
                 'src/eng1/behavioral/viewport_state_data.c',
                 'src/eng1/behavioral/color_storage.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/frame_lifecycle_bridges.c'],
        spans=[(0x33420, 0x200), (0x33620, 0xe0)], cases=14,
        scope='Fourteen unpatched-x86 command-writer cases: complete128B output/canaries, callback order/args, chain/head/cursor; truncation, blend gates, unsigned edges and callback cursor mutation. Host endpoints are controlled spies; no pixel or gameplay claim.'),
    'descriptor-reload': dict(
        sources=['tests/eng1_descriptor_reload_differential.c',
                 'src/eng1/behavioral/descriptor_resource_reload.c',
                 'calibration/resource_data.c',
                 'src/eng1/behavioral/resource_data.c',
                 'calibration/engine_interface_storage.c',
                 'calibration/engine_file_bridge.c',
                 'calibration/resource_provider.c',
                 'calibration/resource_callbacks.c',
                 'calibration/resource_destroy.c',
                 'calibration/frame_lifecycle_bridges.c'],
        spans=[(0x33e0, 0x30), (0x3410, 0x60)], cases=3,
        flags={'calibration/resource_provider.c': [
            '/Dresource_allocate=engine_pool_allocate',
            '/Dresource_file_length=engine_file_length',
            '/Dresource_load_async=engine_file_load_async',
            '/Dresource_load_sync=engine_file_load_sync']},
        scope='Three finite synchronous reload scenarios versus unpatched33e0/3410: all81 rows, provider globals, context, buffer contents and host callback order/arguments. Real source resource_provider and resource_load_complete; only host endpoints and valid-list guard counting are spies. Failure/reclaim/async paths and gameplay unverified.'),
}

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def actor_update_cases(exe, oracle, out, env):
    """Keep each native and source observation in a fresh fixture process."""
    def case(kind, mode, flags):
        args = [str(exe), str(oracle), kind, str(mode), hex(flags), '0']
        result = subprocess.run(args, cwd=out, env=env, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise ValueError('Actor fixture failed: '+result.stdout+result.stderr)
        match = re.fullmatch(r'mode=(\S+) helper_calls=(\d+) args=([0-9a-f]{8}),([0-9a-f]{8}),([0-9a-f]{8}),([0-9a-f]{8}) result=([0-9a-f]{8}) actor=([0-9a-f]{592})\n?', result.stdout)
        if not match:
            raise ValueError('Unrecognized actor fixture output: '+result.stdout)
        return dict(args=[int(match[i],16) for i in range(3,7)],calls=int(match[2]),result=int(match[7],16),actor=bytes.fromhex(match[8]))
    probe = case('probe',0,0)
    rows=[]
    for mode,flags in [(0,0),(2,0x10000000),(8,0x10000000),(10,0x10000000),(2,0),(4,0x10000000)]:
        native=case('native',mode,flags);source=case('source',mode,flags)
        if native['actor']!=source['actor'] or source['calls']!=1 or source['args']!=[160,128,320,0x100] or source['result']!=probe['result']:
            raise ValueError('Native actor/argument mismatch for mode '+str(mode))
        rows.append(dict(mode=mode,flags=hex(flags),actor_sha256=hashlib.sha256(source['actor']).hexdigest(),helper_result=hex(source['result'])))
    negative=case('source-fixed-negative',2,0)
    if negative['calls']!=1 or negative['args']!=[0xffffffff,0xffff8000,0x7fff,0xfffe]:
        raise ValueError('Actor signed-coordinate/WORD argument mismatch')
    return SimpleNamespace(returncode=0,stdout='actor_update_native_cases=6 checks=24 failures=0\nactor_update_source_spy_cases=1 checks=2 failures=0\n',stderr=''),dict(cases=6,checks=24,failures=0,source_spy_cases=1,source_spy_checks=2,source_spy_failures=0,native_observations=rows)

def bootstrap_cases(exe, oracle, out, env):
    tested = subprocess.run([str(exe),str(oracle)],cwd=out,env=env,
        capture_output=True,text=True,timeout=60)
    if tested.returncode:
        return tested, {}
    for name, rva in [('pool','72860'),('level','5a69c'),
                      ('subrow','5a69c'),('base','71370'),('callback','71370')]:
        guard = subprocess.run([str(exe),str(oracle),name],cwd=out,env=env,
            capture_output=True,text=True,timeout=60)
        if guard.returncode != 0xe1 or 'unsupported RVA 0x'+rva not in guard.stderr:
            raise ValueError('Bootstrap guard failed: '+name)
    return tested, dict(source_guard_cases=5,source_guard_failures=0)


def run(profile, oracle, captured_payload=None):
    cfg = dict(PROFILES[profile])
    oracle = oracle.resolve()
    if sha(oracle) != '01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757':
        raise ValueError('Unexpected ENG1 oracle identity')
    out = ROOT / 'work/eng1_behavioral/differential' / profile
    out.mkdir(parents=True, exist_ok=True)
    sources = [ROOT / p for p in cfg['sources']]
    paths = {p for source in sources for p in context_files(source)} | {Path(__file__).resolve()}
    graph_config = None
    if cfg.get('production_omissions'):
        graph_path = ROOT / 'evidence/eng1_behavioral_build.json'
        paths.add(graph_path)
        graph_config = json.loads(graph_path.read_bytes())
    if captured_payload is not None:
        if profile != 'payload-dispatch':
            raise ValueError('Captured payload is supported only by payload-dispatch')
        captured_payload = captured_payload.resolve()
        if sha(captured_payload) != '8d3fb673b1806e5c5f502df64eb75edc26a3b380eb472e5be30a469d6889f21d':
            raise ValueError('Captured payload differs from the reviewed startup prefix')
        paths.add(captured_payload)
        cfg['cases'] = 7
    before = {p.relative_to(ROOT).as_posix(): sha(p) for p in paths}
    tool = verify_toolchain(DEFAULT_TOOLCHAIN)
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               INCLUDE=str(DEFAULT_TOOLCHAIN / 'include'), LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    exe = out / 'fixture.exe'
    exe.unlink(missing_ok=True)
    commands, objects, logs, derivations = [], [], [], []
    for index, source in enumerate(sources):
        obj = out / ('unit_' + str(index) + '.obj')
        obj.unlink(missing_ok=True)
        name = source.relative_to(ROOT).as_posix()
        flags = list(cfg.get('flags', {}).get(name, []))
        compiler_input = source
        omitted = graph_config.get('omit_functions', {}).get(name, []) if graph_config else []
        if omitted:
            source_text = source.read_text(encoding='utf-8')
            for row in omitted:
                body = function_text(source_text, row['function'])
                source_text = replace_function(source_text, row['function'], body[:body.index('{')].rstrip() + ';')
            compiler_input = out / ('derived_' + str(index) + '.c')
            compiler_input.write_bytes(source_text.encode())
            flags.append('/I' + str(source.parent))
            derivations.append(dict(source=name, source_sha256=sha(source), omitted_functions=omitted,
                compiler_input=compiler_input.relative_to(ROOT).as_posix(), compiler_input_sha256=sha(compiler_input)))
        command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/c', '/O2', '/Gy', '/MT', *flags, '/Fo' + str(obj), str(compiler_input)]
        commands.append(command)
        built = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
        logs.append(built.stdout + built.stderr)
        if built.returncode:
            break
        objects.append(obj)
    if built.returncode == 0:
        command = [str(DEFAULT_TOOLCHAIN / 'bin/cl.exe'), '/nologo', '/Fe' + str(exe), *map(str, objects), '/link', '/INCREMENTAL:NO', '/OPT:REF', 'kernel32.lib']
        commands.append(command)
        built = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, timeout=60)
        logs.append(built.stdout + built.stderr)
    (out / 'compile.log').write_bytes(''.join(logs).encode())
    pe = PE(oracle)
    report = dict(scope=cfg['scope'], toolchain=tool, input_sha256=before,
                  oracle_sha256=sha(oracle), native_spans={hex(rva):dict(size=size,sha256=hashlib.sha256(pe.read_rva(rva,size)).hexdigest()) for rva,size in cfg['spans']},
                  commands=commands, compile_exit_code=built.returncode, production_derivations=derivations)
    if built.returncode == 0:
        arguments = [str(exe), str(oracle)]
        if captured_payload is not None:
            arguments.append(str(captured_payload))
        if profile == 'actor-bootstrap':
            tested, bootstrap_proof = bootstrap_cases(exe, oracle, out, env)
            report.update(bootstrap_proof)
        elif profile == 'actor-update':
            tested, actor_proof = actor_update_cases(exe, oracle, out, env)
            report.update(actor_proof)
        else:
            tested = subprocess.run(arguments, cwd=out, env=env, capture_output=True, text=True, timeout=60)
            if profile == 'alternate-bootstrap' and tested.returncode == 0:
                guard = subprocess.run(arguments + ['pool'],cwd=out,env=env,
                    capture_output=True,text=True,timeout=60)
                if guard.returncode != 0xe1 or 'unsupported RVA 0x734e0' not in guard.stderr:
                    raise ValueError('Alternate bootstrap null-pool guard failed')
                report.update(source_guard_cases=1,source_guard_failures=0)
        report.update(run_exit_code=tested.returncode, stdout=tested.stdout, stderr=tested.stderr, executable_sha256=sha(exe))
        if profile in ('grid-scan','query-decoder'):
            prefix='grid_query_20b50' if profile=='grid-scan' else 'query_20990'
            match=re.search(prefix+r'_native_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            guard=re.search(prefix+r'_source_guard_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            control=re.search(prefix+r'_source_control_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            if match:report.update(cases=int(match[1]),checks=int(match[2]),failures=int(match[3]))
            if guard:report.update(source_guard_cases=int(guard[1]),source_guard_checks=int(guard[2]),source_guard_failures=int(guard[3]))
            if control:report.update(source_control_cases=int(control[1]),source_control_checks=int(control[2]),source_control_failures=int(control[3]))
        elif profile == 'actor-bounds':
            match=re.search(r'actor_bounds_native_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            spy=re.search(r'actor_bounds_source_spy_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            if match:report.update(cases=int(match[1]),checks=int(match[2]),failures=int(match[3]))
            if spy:report.update(source_spy_cases=int(spy[1]),source_spy_checks=int(spy[2]),source_spy_failures=int(spy[3]))
        elif profile == 'grid-query':
            match=re.search(r'grid_query_native_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            guard=re.search(r'grid_query_source_guard_cases=(\d+) failures=(\d+)',tested.stdout)
            if match:report.update(cases=int(match[1]),checks=int(match[2]),failures=int(match[3]))
            if guard:report.update(source_guard_cases=int(guard[1]),source_guard_failures=int(guard[2]))
        elif profile == 'index-query':
            match=re.search(r'index_query_native_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            box=re.search(r'box_native_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            spy=re.search(r'index_query_source_spy_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            guard=re.search(r'index_query_source_guard_cases=(\d+) checks=(\d+) failures=(\d+)',tested.stdout)
            if match:report.update(cases=int(match[1]),checks=int(match[2]),failures=int(match[3]))
            if box:report.update(box_native_cases=int(box[1]),box_native_checks=int(box[2]),box_native_failures=int(box[3]))
            if spy:report.update(source_spy_cases=int(spy[1]),source_spy_checks=int(spy[2]),source_spy_failures=int(spy[3]))
            if guard:report.update(source_guard_cases=int(guard[1]),source_guard_checks=int(guard[2]),source_guard_failures=int(guard[3]))
        elif profile == 'query':
            match = re.search(r'query_native_cases=(\d+) checks=(\d+) failures=(\d+) data_word_checks=(\d+)', tested.stdout)
            spy = re.search(r'query_source_spy_cases=(\d+) checks=(\d+) failures=(\d+)', tested.stdout)
            guards = re.search(r'query_source_guard_cases=(\d+) failures=(\d+)', tested.stdout)
            if match:
                report.update(cases=int(match[1]),checks=int(match[2]),failures=int(match[3]),data_word_checks=int(match[4]))
            if spy:
                report.update(source_spy_cases=int(spy[1]),source_spy_checks=int(spy[2]),source_spy_failures=int(spy[3]))
            if guards:
                report.update(source_guard_cases=int(guards[1]),source_guard_failures=int(guards[2]))
        elif profile == 'player-setup':
            match = re.search(r'player_setup_native_cases=(\d+) checks=(\d+) failures=(\d+)', tested.stdout)
            spy = re.search(r'player_setup_source_spy_cases=(\d+) checks=(\d+) failures=(\d+)', tested.stdout)
            if match:
                report.update(cases=int(match[1]), checks=int(match[2]), failures=int(match[3]))
            if spy:
                report.update(source_spy_cases=int(spy[1]), source_spy_checks=int(spy[2]), source_spy_failures=int(spy[3]))
        elif profile == 'payload-dispatch':
            match = re.search(r'native/source 0x4ca0 cases=(\d+)/(\d+)', tested.stdout)
            if match:
                report.update(cases=int(match[1]), failures=int(match[2])-int(match[1]), checks=76 if captured_payload is not None else 67)
        elif 'success_marker' in cfg:
            if cfg['success_marker'] in tested.stdout:
                report.update(cases=cfg['cases'], failures=0, source_guard_cases=1)
        elif profile not in ('command-writers','actor-update'):
            match = re.search(r'checks=(\d+) failures=(\d+)', tested.stdout)
            if match:
                report.update(checks=int(match[1]), failures=int(match[2]), cases=cfg['cases'])
            if profile == 'simulation-helpers':
                guards = re.search(r'simulation_source_guard_cases=(\d+) failures=(\d+)', tested.stdout)
                if guards:
                    report.update(source_guard_cases=int(guards[1]), source_guard_failures=int(guards[2]))
        elif profile == 'command-writers':
            report.update(cases=len(re.findall(r'^differential .*: pass ', tested.stdout, re.M)), failures=0 if tested.returncode==0 else None)
    report['inputs_unchanged'] = all(sha(ROOT / p)==value for p,value in before.items()) and sha(oracle)==report['oracle_sha256']
    coverage = report.get('cases') == cfg['cases'] and (profile=='command-writers' or 'success_marker' in cfg or report.get('checks',0)>0)
    if profile == 'simulation-helpers':
        coverage = coverage and report.get('source_guard_cases') == 2 and report.get('source_guard_failures') == 0
    if profile == 'player-setup':
        coverage = coverage and report.get('source_spy_cases') == 1 and report.get('source_spy_failures') == 0
    if profile == 'query':
        coverage = coverage and report.get('source_spy_cases') == 5 and report.get('source_spy_failures') == 0 and report.get('source_guard_cases') == 2 and report.get('source_guard_failures') == 0 and report.get('data_word_checks') == 64
    if profile == 'actor-update':
        coverage = coverage and report.get('source_spy_cases') == 1 and report.get('source_spy_failures') == 0
    if profile == 'index-query':
        coverage = coverage and report.get('box_native_cases') == 7 and report.get('box_native_failures') == 0 and report.get('source_spy_cases') == 4 and report.get('source_spy_failures') == 0 and report.get('source_guard_cases') == 2 and report.get('source_guard_failures') == 0
    if profile == 'grid-query':
        coverage = coverage and report.get('source_guard_cases') == 5 and report.get('source_guard_failures') == 0
    if profile == 'actor-bounds':
        coverage = coverage and report.get('source_spy_cases') == 8 and report.get('source_spy_failures') == 0
    if profile == 'grid-scan':
        coverage = coverage and report.get('source_guard_cases') == 1 and report.get('source_guard_failures') == 0
    if profile == 'query-decoder':
        coverage = coverage and report.get('source_guard_cases') == 2 and report.get('source_guard_failures') == 0 and report.get('source_control_cases') == 1 and report.get('source_control_failures') == 0
    if profile == 'actor-bootstrap':
        coverage = coverage and report.get('source_guard_cases') == 5 and report.get('source_guard_failures') == 0
    if profile == 'alternate-bootstrap':
        coverage = coverage and report.get('source_guard_cases') == 1 and report.get('source_guard_failures') == 0
    report['result'] = 'PASS' if report.get('run_exit_code')==0 and report.get('failures')==0 and report['inputs_unchanged'] and coverage else 'FAIL'
    (out / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(report.get('stdout', ''.join(logs)))
    return 0 if report['result']=='PASS' else 1

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', choices=PROFILES)
    parser.add_argument('--native-eng1', type=Path, default=ROOT/'assets/pc/HERCULES/ENG1.DLL')
    parser.add_argument('--captured-payload', type=Path, help='Optional reviewed private startup input; bytes are never saved in public source')
    args = parser.parse_args()
    raise SystemExit(run(args.profile, args.native_eng1, args.captured_payload))
