"""Reduce recorded host runs into compact provenance and first-failure evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def summarize(path):
    path = path.resolve()
    if not path.is_relative_to(ROOT / 'work/eng1_behavioral/integration/runs'):
        raise ValueError('Expected an actual isolated integration run report')
    run = json.loads(path.read_bytes())
    build_path = ROOT / run['build_report']
    build = json.loads(build_path.read_bytes())
    if sha(ROOT / build['dll']) != run['eng1_sha256'] or build['dll_sha256'] != run['eng1_sha256']:
        raise ValueError('Recorded runtime/build DLL identity changed')
    if sha(ROOT / build['link_map']) != run['build_map_sha256']:
        raise ValueError('Recorded link map identity changed')
    context = dict(build['context_sha256'])
    context['recovery.json'] = build['recovery_sha256']
    context['evidence/eng1_behavioral_build.json'] = build['config_sha256']
    current = all((ROOT / p).is_file() and sha(ROOT / p) == value for p, value in context.items())
    events = run['events']
    loads = [x for x in events if x['type'] == 'eng1_module_load']
    snapshots = []
    for event in events:
        if event['type'] != 'eng1_interface_snapshot' or event['phase'] not in ('first_post_load', 'setup_observed'):
            continue
        words = event['interface_words']
        snapshots.append(dict(phase=event['phase'], words_read=len(words),
            words_sha256=hashlib.sha256(b''.join(int(x, 0).to_bytes(4, 'little') for x in words)).hexdigest(),
            signature=event['signature'], first_use=event['first_use'],
            callback_slots_nonzero=event['callback_slots_nonzero'],
            selected_slots={str(i): words[i] for i in (1, 30, 81, 89, 129, 143, 174, 175)}))
    incomplete = next((x for x in events if x['type'] == 'explicit_incomplete_entry'), None)
    captures = []
    for event in events:
        if event['type'] != 'source_call_stack_candidate':
            continue
        prefix = bytes.fromhex(event.get('input_prefix_hex', ''))
        if len(prefix) != event['input_prefix_bytes_read']:
            raise ValueError('Recorded capture byte count differs from its actual prefix')
        captures.append({**{k: event[k] for k in ['original_rva', 'return_rva',
            'stack_word_offset', 'arguments_read', 'arguments',
            'input_prefix_bytes_requested', 'input_prefix_bytes_read']},
            'input_prefix_sha256': hashlib.sha256(prefix).hexdigest()})
    if incomplete is None:
        for event in events:
            match = re.search(r'ENG1 incomplete behavior at RVA (0x[0-9a-f]+): (.*)', event.get('text', ''))
            if match:
                incomplete = dict(original_rva=match[1], reason=match[2], basis='recorded debug string')
                break
    return dict(run=path.relative_to(ROOT).as_posix(), run_report_sha256=sha(path),
        build_report_sha256=sha(build_path), build_config_sha256=build['config_sha256'],
        recovery_sha256=build['recovery_sha256'], source_context_matches_current_checkout=current,
        eng1_sha256=run['eng1_sha256'], host_sha256=run['host_sha256'],
        eng3_sha256=run['eng3_sha256'], source_title_sha256=run['source_title_sha256'],
        probe_sha256=run['probe_sha256'], map_sha256=run['build_map_sha256'],
        architecture=build['architecture'], exports=build['exports'],
        imports=sorted({x['dll'] for x in build['imports']}),
        strict_linked_byte_matches=len([x for x in build['preserved_exact_functions'] if x.get('linked_rva') and x['exact']]),
        genuine_function_symbols=build['genuine_compiled_function_symbols'],
        explicit_incomplete_symbols=build['provisional_function_count'],
        loaded_runtime_path_matches=bool(loads) and all(x['loaded_image_path_matches_runtime'] for x in loads),
        interface_setup_observed=run['interface_setup_observed'], snapshots=snapshots,
        windowed_mode_seen=run.get('windowed_mode_seen', False),
        windowed_mode_ready_at_end=run['windowed_mode_ready'],
        first_failure=run['first_divergence_candidate'], incomplete=incomplete,
        source_call_capture_specifications=run.get('call_capture_specifications', []),
        source_call_stack_candidates=captures,
        exception_context=next((x for x in events if x['type'] == 'exception_context'), None),
        exception_source_state={x['name']: x['value'] for x in events if x['type'] == 'exception_source_state' and x['read']},
        summary=next((x for x in reversed(events) if x['type'] == 'summary'), None),
        normal_shutdown_verified=False, gameplay_verified=False)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runs', type=Path, nargs='+')
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_behavioral/runtime_receipt.json')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT):
        raise ValueError('Receipt output must remain in the repository')
    result = dict(scope='Historical bounded source-DLL host loading, interface setup and earliest observed startup failures. No gameplay or universal equivalence acceptance. Historic runs remain historical when source_context_matches_current_checkout is false.',
                  observations=[summarize(p) for p in args.runs])
    output.write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print(json.dumps(dict(observations=len(result['observations']), output=str(output))))
