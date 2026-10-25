"""Small derived context/output index. No compiler cache, proof writer, scheduler or GC.

Workers keep append-only receipts in private ignored lanes. The integrator reviews
selected receipts/recipes for WARM publication. recovery.json is the only authority.
"""
import argparse
from datetime import datetime,timezone
from difflib import SequenceMatcher
import json
from pathlib import Path
import re

from env import ROOT
from coff import COFF
from match import digest,compare,verify_toolchain,DEFAULT_TOOLCHAIN,ACCEPTED
from macro_compare import declared_context_files


def identity(value):
    return digest(json.dumps(value,sort_keys=True,separators=(',',':')).encode())


def read(path):return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def contribution_identity(contribution):
    return identity(dict(bytes=contribution['data'].hex(),size=contribution['size'],
                         relocations=sorted(contribution['relocations'],key=lambda r:(r['offset'],r['type'],r['symbol']))))


def output_identities(obj,symbols):
    coff=COFF(obj)
    functions={name:contribution_identity(coff.function(name)) for name in sorted(symbols)}
    # Include other data/directive contributions; never call a function-only
    # equivalence a whole-object identity or reuse a comparison from it.
    extra=[dict(name=s['name'],flags=s['flags'],size=s['size'],bytes=s['data'].hex(),relocations=s['relocations'])
           for s in coff.sections if not s['flags']&0x20 and s['size']]
    return functions,identity(dict(functions=functions,noncode=extra))


def make_patch(before,after):
    a=before.splitlines(keepends=True);b=after.splitlines(keepends=True)
    edits=[dict(line=i,old=''.join(a[i:j]),new=''.join(b[k:l]))
           for op,i,j,k,l in SequenceMatcher(None,a,b,autojunk=False).get_opcodes() if op!='equal']
    return dict(base_sha256=digest(before.encode()),result_sha256=digest(after.encode()),edits=edits)


def apply_patch_text(before,patch):
    if digest(before.encode())!=patch['base_sha256']:raise ValueError('Stale source baseline')
    lines=before.splitlines(keepends=True)
    for edit in reversed(patch['edits']):
        old=edit['old'].splitlines(keepends=True);pos=edit['line']
        if ''.join(lines[pos:pos+len(old)])!=edit['old']:raise ValueError('Patch context mismatch')
        lines[pos:pos+len(old)]=edit['new'].splitlines(keepends=True)
    result=''.join(lines)
    if digest(result.encode())!=patch['result_sha256']:raise ValueError('Patch result identity mismatch')
    return result


def validate_meta(meta):
    for key in ('region','lane','family','prediction','falsifier','capability'):
        if not meta.get(key):raise ValueError('Missing experiment metadata: '+key)
    model=meta.get('model');effort=meta.get('effort')
    if model not in ('gpt-6-luna','gpt-6.1-sol'):raise ValueError('Only requested Sol/Luna workers are allowed')
    if model=='gpt-6-luna' and effort not in ('high','xhigh','max'):raise ValueError('Luna requires high or above')
    if model=='gpt-6.1-sol' and effort not in ('low','medium','high','xhigh','max'):raise ValueError('Unknown Sol effort')
    if (model,effort) not in (('gpt-6-luna','xhigh'),('gpt-6.1-sol','high')) and not meta.get('exception_reason'):
        raise ValueError('Other configurations require an explicit exception reason')


def check_reentry(meta,observations,context_id):
    validate_meta(meta)
    new=meta.get('new_evidence') or meta.get('new_discriminator')
    for old in observations:
        if old.get('region')!=meta['region'] or old.get('family')!=meta['family']:continue
        if old.get('context_id')!=context_id:continue
        if old.get('verdict') in ('exhausted','falsified','blocked') and not new:
            raise ValueError('Family requires new evidence/discriminator: '+str(old.get('reentry_condition')))


def known_observations(parents):
    # Compact published evidence is always consulted; omitted parent arguments
    # cannot silently reopen a known exhausted family.
    path=ROOT/'evidence/region_knowledge.json'
    return [read(p) for p in parents]+(read(path)['observations'] if path.exists() else [])


def ownership(spec):
    region=spec.get('region',{})
    if region.get('ownership') not in ('inferred_region','proven_tu','function_task'):
        raise ValueError('Specify inferred_region, proven_tu or function_task ownership')
    if region['ownership']=='proven_tu' and not region.get('ownership_evidence'):
        raise ValueError('Historical TU ownership requires evidence')
    return region


def context_identity(spec,baseline):
    value=dict(target=spec['target_sha256'],functions=spec['functions'],symbols=spec.get('symbol_rvas',{}),
        flags=spec['flags'],baseline_source=digest(baseline.encode()),toolchain_manifest=digest((ROOT/'toolchains/manifest.json').read_bytes()))
    if 'context_files' in spec:value['context_files']=declared_context_files(spec,ROOT)
    return identity(value)


def record(spec_path,source,baseline,report_path,meta_path,parents=()):
    spec=read(spec_path);scope=ownership(spec);meta=read(meta_path);validate_meta(meta)
    if meta['region']!=scope['id']:raise ValueError('Region metadata disagreement')
    if not re.fullmatch(r'[a-z0-9_-]+',meta['lane']):raise ValueError('Unsafe lane name')
    lane=ROOT/'work'/meta['lane'];lane.mkdir(parents=True,exist_ok=True)
    report=read(report_path);text=source.read_text();base=baseline.read_text()
    if report['source_sha256']!=digest(source.read_bytes()):raise ValueError('Source changed since compile')
    if report['target_sha256']!=spec['target_sha256']:raise ValueError('Wrong oracle report')
    if report.get('flags')!=spec['flags']:raise ValueError('Compile flags disagree')
    if 'context_files' in spec and report.get('context_files_sha256')!=declared_context_files(spec,ROOT):
        raise ValueError('Missing or changed declared compile dependency provenance')
    verify_toolchain(DEFAULT_TOOLCHAIN)
    if digest((ROOT/spec['target']).read_bytes())!=spec['target_sha256']:raise ValueError('Oracle changed')
    obj=Path(report['object'])
    if digest(obj.read_bytes())!=report['object_sha256']:raise ValueError('Object changed')
    context_id=context_identity(spec,base)
    previous=[read(p) for p in parents];check_reentry(meta,known_observations(parents),context_id)
    if any(p['region']!=scope['id'] for p in previous):raise ValueError('Parent belongs to another region')
    names=[f['symbol'] for f in spec['functions']]
    fingerprints,region_output=output_identities(obj,names)
    canonical=read(ROOT/'recovery.json')['functions'];rows=[];regressions=[]
    observations={r['symbol']:r for r in report['functions']}
    for f in spec['functions']:
        strict=compare(obj,f['symbol'],ROOT/spec['target'],int(f['rva'],0),f['size'])
        if strict['target_sha256']!=f['target_sha256']:raise ValueError('Reviewed extent changed')
        observed=observations[f['symbol']]
        row={k:observed.get(k) for k in ('symbol','target_size','candidate_size','raw_different_bytes',
            'normalized_different_bytes','instruction_percent','ordered_cfg_equal','category')}
        row['strict_equal']=strict['exact']
        row['target_size']=strict['target_size']
        row['raw_different_bytes']=strict['different_bytes']
        row['candidate_size']=strict['compiled_size']
        row['relocation_count']=len(strict['relocations'])
        row['target_relocation_count']=len(strict['target_relocations'])
        row['output_id']=fingerprints[f['symbol']]
        row['scope']='experimental contribution; not acceptance'
        if not strict['exact']:
            claims=[c for c in canonical if c['status']=='FUNCTION_MATCH' and c['target_sha256']==spec['target_sha256'] and int(c['rva'],0)==int(f['rva'],0)]
            if claims:regressions.append(f['symbol'])
        for p in previous:
            if p['context_id']!=context_id:continue
            old=next((x for x in p['functions'] if x['symbol']==f['symbol']),None)
            if old and old['strict_equal'] and old['output_id']!=row['output_id']:regressions.append(f['symbol'])
        rows.append(row)
    equivalent=[p['receipt_id'] for p in previous if p['context_id']==context_id and p['output_id']==region_output]
    payload=dict(schema_version=1,scope='Exploration/knowledge only; never accepted proof.',
        created_utc=datetime.now(timezone.utc).isoformat(),region=scope['id'],ownership=scope,
        context_id=context_id,source_sha256=digest(text.encode()),baseline_sha256=digest(base.encode()),
        source_identity_convention='UTF-8/LF recipe text; compiled_source_sha256 records actual compiler input bytes',
        compiled_source_sha256=report['source_sha256'],
        source_patch=make_patch(base,text),output_id=region_output,equivalent_receipts=equivalent,
        safe_frontier_eligible=not regressions and not equivalent,regressions=sorted(set(regressions)),functions=rows,
        summary=report.get('summary'),family=meta['family'],prediction=meta['prediction'],falsifier=meta['falsifier'],
        verdict=meta.get('verdict','observed'),blocker=meta.get('blocker'),reentry_condition=meta.get('reentry_condition'),
        next_discriminator=meta.get('next_discriminator'),ungated_work=meta.get('ungated_work',[]),
        new_evidence=meta.get('new_evidence'),new_discriminator=meta.get('new_discriminator'),
        metrics=dict(model=meta['model'],effort=meta['effort'],capability=meta['capability'],
            wall_seconds=meta.get('wall_seconds'),tokens=meta.get('tokens'),cost_usd=meta.get('cost_usd'),
            compile_cycles=meta.get('compile_cycles'),semantic_result=meta.get('semantic_result'),
            actual_model_identity=meta.get('actual_model_identity')),
        provenance=dict(spec_sha256=digest(Path(spec_path).read_bytes()),report_sha256=digest(Path(report_path).read_bytes()),
            context_files=report.get('context_files_sha256'),
            object_sha256=report['object_sha256'],command=report.get('command'),
            toolchain_manifest=digest((ROOT/'toolchains/manifest.json').read_bytes())))
    payload['receipt_id']=identity(payload)
    dest=lane/('receipt-'+payload['receipt_id']+'.json')
    with dest.open('x',encoding='utf-8',newline='\n') as out:out.write(json.dumps(payload,indent=2)+'\n')
    print(dest)
    if regressions:print('REGRESSION: excluded from frontier: '+', '.join(sorted(set(regressions))))
    if equivalent:print('Equivalent compiler outcome; keep existing frontier representative.')
    return dest


def frontier(receipts):
    unique={}
    for r in receipts:
        if r.get('regressions'):continue
        unique.setdefault(r['output_id'],r)
    def score(r):
        rows=r['functions']
        return (sum(f['strict_equal'] for f in rows),sum(f.get('normalized_different_bytes')==0 for f in rows),
                -sum(f.get('normalized_different_bytes') if f.get('normalized_different_bytes') is not None else f['target_size'] for f in rows),
                sum(bool(f.get('ordered_cfg_equal')) for f in rows))
    choices=list(unique.values())
    return [r for r in choices if not any(all(a>=b for a,b in zip(score(q),score(r))) and score(q)!=score(r) for q in choices)]


def _normalized_repo_name(name):
    path=Path(name)
    if path.is_absolute() or '..' in path.parts:
        raise ValueError('Packet source path must be repository-relative: '+str(name))
    return path.as_posix()


def _repo_file(name):
    relative=_normalized_repo_name(name)
    path=(ROOT/relative).resolve()
    if not path.is_relative_to(ROOT.resolve()):
        raise ValueError('Packet source path escapes repository: '+str(name))
    return relative,path


def _source_layout_moves():
    review=ROOT/'evidence/source_layout_2026_10_25/review.json'
    if not review.exists():return {}
    moves=read(review).get('moves')
    if not isinstance(moves,dict):raise ValueError('Invalid reviewed source-layout move map')
    return {_normalized_repo_name(old):_normalized_repo_name(new) for old,new in moves.items()}


def _ledger_region_owners(spec,state,moves):
    owners={}
    for f in spec['functions']:
        rva=int(f['rva'],0)
        matches=[row for row in state['functions']
                 if row.get('target_sha256')==spec['target_sha256'] and int(row.get('rva','0'),0)==rva]
        if not matches:continue
        extent_matches=[]
        for row in matches:
            if row.get('size') is None:continue
            if row['size']!=f['size']:raise ValueError('Recovery extent disagrees with region function: '+f['symbol'])
            extent_matches.append(row)
        records=set()
        for row in extent_matches:
            source=row.get('source')
            if not source:continue
            relative=_normalized_repo_name(source)
            relative=moves.get(relative,relative)
            records.add((relative,row.get('id'),row.get('symbol')))
        if records:owners[f['symbol']]=[dict(source=path,recovery_id=rid,recovery_symbol=symbol)
                                        for path,rid,symbol in sorted(records)]
    return owners


def _resolve_packet_context_files(spec,moves):
    resolved=[];mapping={}
    for name in spec.get('context_files',[]):
        old=_normalized_repo_name(name)
        new=moves.get(old,old)
        if old.startswith('calibration/') and old not in moves:
            raise ValueError('Retired context file has no reviewed source-layout move: '+old)
        new,path=_repo_file(new)
        if not path.is_file():raise FileNotFoundError('Resolved packet context file does not exist: '+new)
        resolved.append(new);mapping[old]=new
    if len(set(resolved))!=len(resolved):
        raise ValueError('Reviewed context-file moves collapse distinct inputs: '+', '.join(resolved))
    return resolved,mapping


def _resolve_packet_context(entry,spec,state):
    """Resolve a retired index baseline only through reviewed moves or exact ledger ownership."""
    moves=_source_layout_moves()
    indexed=_normalized_repo_name(entry['baseline'])
    owners=_ledger_region_owners(spec,state,moves)
    by_function={name:rows for name,rows in owners.items()}
    paths_by_function={name:sorted({row['source'] for row in rows}) for name,rows in by_function.items()}
    ambiguous=[name for name,paths in paths_by_function.items() if len(paths)>1]
    if ambiguous:
        detail='; '.join(name+': '+', '.join(paths_by_function[name]) for name in ambiguous)
        raise ValueError('Ambiguous recovery source ownership for region functions: '+detail)
    current_paths=sorted({path for paths in paths_by_function.values() for path in paths})
    mapped=moves.get(indexed)
    all_functions_owned=len(by_function)==len(spec['functions'])
    if len(current_paths)>1:
        if not mapped and (not indexed.startswith('calibration/') or not all_functions_owned):
            raise ValueError('Split current ownership cannot be tied to a reviewed baseline move: '+', '.join(current_paths))
        if mapped and mapped not in current_paths:
            raise ValueError('Reviewed baseline move disagrees with split current ledger owners: '+mapped)
        declared=spec.get('source')
        if declared:
            declared=_normalized_repo_name(declared)
            declared=moves.get(declared,declared)
            if declared not in current_paths:
                raise ValueError('Region source is not among split current ledger owners: '+declared)
        sources=[]
        for relative in current_paths:
            relative,path=_repo_file(relative)
            if not path.is_file():raise FileNotFoundError('Current ledger owner does not exist: '+relative)
            if not relative.startswith('src/'):
                raise ValueError('Split region includes a noncanonical source owner: '+relative)
            functions=[]
            for name,rows in by_function.items():
                matching=[row for row in rows if row['source']==relative]
                if matching:
                    functions.append(dict(region_symbol=name,ledger_rows=[dict(id=row['recovery_id'],
                        symbol=row['recovery_symbol']) for row in matching]))
            sources.append(dict(path=relative,sha256=digest(path.read_bytes()),functions=functions))
        context_files,context_mapping=_resolve_packet_context_files(spec,moves)
        resolution=dict(status='split_current_ownership',indexed_baseline=indexed,
            resolved_baseline=None,resolution_basis='current recovery ledger identifies multiple canonical source units',
            baseline_sources=sources,ledger_owners=by_function,context_files=context_mapping,
            unowned_functions=[f['symbol'] for f in spec['functions'] if f['symbol'] not in by_function],
            limitation='No single full-TU context exists for this region; receipts are excluded from the frontier.')
        return None,None,resolution
    owner=current_paths[0] if current_paths else None
    if mapped:
        relative=mapped
        basis='reviewed source-layout move'
        if owner is not None and owner!=relative:
            raise ValueError('Reviewed move disagrees with current recovery owner: '+relative+' vs '+owner)
    elif indexed.startswith('calibration/'):
        if owner is None or not all_functions_owned:
            raise ValueError('Retired calibration baseline has no reviewed move or complete unambiguous current ledger ownership: '+indexed)
        relative=owner
        basis='unambiguous current recovery ownership'
    else:
        relative=indexed
        basis='indexed current source path'
        if owner is not None and owner!=relative:
            raise ValueError('Indexed source path disagrees with current recovery owner: '+relative+' vs '+owner)
        _,indexed_path=_repo_file(relative)
        if not indexed_path.is_file() and owner is not None:
            relative=owner
            basis='unambiguous current recovery ownership'
    relative,path=_repo_file(relative)
    if not path.is_file():raise FileNotFoundError('Resolved packet source does not exist: '+relative)
    if not relative.startswith('src/'):
        raise ValueError('Packet baseline did not resolve to current game source: '+relative)
    if owner is not None and owner!=relative:
        raise ValueError('Reviewed move disagrees with current recovery owner: '+relative+' vs '+owner)
    declared=spec.get('source')
    if declared:
        declared=_normalized_repo_name(declared)
        declared=moves.get(declared,declared)
        if declared!=relative:
            raise ValueError('Region source and baseline resolve to different current units: '+declared+' vs '+relative)
    baseline=path.read_text(encoding='utf-8-sig')

    resolved_spec=dict(spec)
    resolved_context,context_mapping=_resolve_packet_context_files(spec,moves)
    if 'context_files' in spec:resolved_spec['context_files']=resolved_context
    provenance=dict(indexed_baseline=indexed,resolved_baseline=relative,resolution_basis=basis,
                    ledger_owners=by_function,context_files=context_mapping,status='resolved')
    return resolved_spec,baseline,provenance


def packet(region_id,more=False):
    if not re.fullmatch(r'[a-z0-9_-]+',region_id):raise ValueError('Unsafe region name')
    catalog=read(ROOT/'evidence/region_index.json');entry=next(r for r in catalog['regions'] if r['id']==region_id)
    state=read(ROOT/'recovery.json');spec=read(ROOT/entry['spec'])
    scope=ownership(spec);packet_spec,base,baseline_resolution=_resolve_packet_context(entry,spec,state)
    ctx=context_identity(packet_spec,base) if baseline_resolution['status']=='resolved' else None
    receipts=[read(ROOT/p) for p in entry.get('receipts',[])]
    current=[r for r in receipts if ctx is not None and r['context_id']==ctx]
    accepted={(r['target_sha256'],int(r['rva'],0)):r for r in state['functions'] if r['status'] in ACCEPTED | {'LINKED_MATCH'}}
    unresolved=[f['symbol'] for f in spec['functions'] if (spec['target_sha256'],int(f['rva'],0)) not in accepted]
    recorded={(r['target_sha256'],int(r['rva'],0)):r for r in state['functions']}
    non_strict=[];unrecorded=[]
    for f in spec['functions']:
        row=recorded.get((spec['target_sha256'],int(f['rva'],0)))
        if row is None:unrecorded.append(f['symbol'])
        elif row['status'] not in ACCEPTED | {'LINKED_MATCH'}:
            non_strict.append(dict(region_symbol=f['symbol'],recovery_id=row['id'],status=row['status'],
                source=row.get('source'),bytes=row.get('size'),extent_agrees=row.get('size')==f['size'],
                proof_scope=row.get('proof',{}).get('scope')))
    closed={f['symbol'] for f in spec['functions'] if f['symbol'] not in unresolved}
    safe=[r for r in current if all(f['strict_equal'] for f in r['functions'] if f['symbol'] in closed)]
    candidates=frontier(safe) if unresolved else []
    notes=read(ROOT/'evidence/region_knowledge.json') if (ROOT/'evidence/region_knowledge.json').exists() else {'observations':[]}
    result=dict(authority='recovery.json only; this packet is disposable',region=scope,context_id=ctx,
        accepted_total=len(accepted),unresolved=unresolved,canonical_in_region=len(spec['functions'])-len(unresolved),
        closure_scope='unresolved and canonical_in_region track strict closure only; recorded non-strict source does not close the frontier.',
        recorded_non_strict_functions=non_strict,unrecorded_functions=unrecorded,
        frontier=[dict(receipt_id=r['receipt_id'],source_sha256=r['source_sha256'],output_id=r['output_id'],summary=r['summary'],
                       next_discriminator=r.get('next_discriminator'),ungated_work=r.get('ungated_work')) for r in candidates[:3]],
        additional_pareto_receipts=max(0,len(candidates)-3),stale_receipts=[r['receipt_id'] for r in receipts if r not in current],
        tried=[{k:r.get(k) for k in ('family','verdict','prediction','falsifier','blocker','reentry_condition','output_id')} for r in current],
        negative_knowledge=[n for n in notes['observations'] if n['region']==region_id and unresolved],
        progressive_references=entry.get('references',[]),model_policy='Luna xhigh normally; Sol high for unresolved explanation. See AGENTS.md.',
        baseline_source=baseline_resolution['resolved_baseline'],baseline_resolution=baseline_resolution,
        baseline_sha256=digest(base.encode()) if base is not None else None)
    if more:result['functions']=[dict(f,accepted_id=accepted.get((spec['target_sha256'],int(f['rva'],0)),{}).get('id')) for f in spec['functions']]
    out=ROOT/'work/packets';out.mkdir(parents=True,exist_ok=True)
    path=out/(region_id+'-'+identity(result)[:16]+'.json')
    path.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(json.dumps(result,indent=2));return path


def _frontier_path(name):
    path=Path(name)
    path=path.resolve() if path.is_absolute() else (ROOT/path).resolve()
    if not path.is_relative_to(ROOT.resolve()):raise ValueError('Frontier input must stay inside the repository')
    return path.relative_to(ROOT.resolve()).as_posix(),path


def _frontier_pin(raw):
    if not isinstance(raw,dict) or not raw.get('path') or not re.fullmatch(r'[0-9a-f]{64}',str(raw.get('sha256',''))):
        raise ValueError('Evidence pins require repository path and SHA-256')
    relative,path=_frontier_path(raw['path'])
    actual=digest(path.read_bytes()) if path.is_file() else None
    return dict(path=relative,sha256=raw['sha256'],current=actual==raw['sha256'])


def _frontier_targets(group):
    values=group.get('target_sha256') or {}
    if isinstance(values,str):return {'':values}
    return {_frontier_path(name)[0]:sha for name,sha in values.items()}


def _frontier_context(group):
    values=group.get('context_files_sha256') or group.get('include_context_sha256')
    if values is None:values=group.get('context_files')
    if values is None:return True
    if isinstance(values,dict):pins=[dict(path=name,sha256=sha) for name,sha in values.items()]
    elif isinstance(values,list):pins=values
    else:return False
    try:return all(_frontier_pin(pin)['current'] for pin in pins)
    except (ValueError,TypeError):return False


def _typed_requirement(requirement):
    return isinstance(requirement,dict) and bool(
        requirement.get('owner') or requirement.get('typed_owner') or
        requirement.get('abi') or requirement.get('typed_profile'))


def _frontier_handoffs(path):
    value=read(path)
    tasks=value if isinstance(value,list) else value.get('tasks',[value])
    if not isinstance(tasks,list):raise ValueError('Handoff must be a task object or tasks array')
    return tasks


def _frontier_region_context(region_id,ledger):
    """Return a live region context only when the indexed inputs resolve now."""
    index=ROOT/'evidence/region_index.json'
    if not region_id or not index.exists():return None
    try:
        entry=next(r for r in read(index)['regions'] if r['id']==region_id)
        spec=read(ROOT/entry['spec'])
        resolved,baseline,_=_resolve_packet_context(entry,spec,ledger)
        return context_identity(resolved,baseline) if resolved is not None else None
    except (KeyError,StopIteration,ValueError,FileNotFoundError):return None


def matching_frontier(census_path,handoff_paths=(),ids=None,limit=8):
    """Build a read-only, hash-guarded work frontier; never change recovery state."""
    if not isinstance(limit,int) or limit<1:raise ValueError('limit must be a positive integer')
    _,census_file=_frontier_path(census_path)
    census=read(census_file);ledger_path=ROOT/'recovery.json';ledger=read(ledger_path)
    ledger_sha=digest(ledger_path.read_bytes());rows=ledger.get('functions',[])
    by_id={row['id']:row for row in rows}
    accepted_statuses=ACCEPTED|{'LINKED_MATCH'}
    selected=list(dict.fromkeys(ids or []));selected_set=set(selected)
    handoffs=[]
    for path in handoff_paths:
        _,handoff_file=_frontier_path(path);handoffs.extend(_frontier_handoffs(handoff_file))
    accepted_filtered=[];unknown_ids=[];tasks=[];covered_ids=set()
    census_rows={row.get('id'):row for row in census.get('rows',[]) if row.get('id')}
    groups=census.get('compile_group_results',[])

    def add_task(raw):
        if not isinstance(raw,dict):raise ValueError('Handoff task must be an object')
        required=('function_ids','source','source_sha256','flags','target','target_sha256','outcome','blockers')
        if any(key not in raw for key in required):raise ValueError('Handoff missing required planning fields')
        function_ids=raw['function_ids']
        if not isinstance(function_ids,list) or not function_ids or any(not isinstance(i,str) for i in function_ids):
            raise ValueError('function_ids must be a nonempty string array')
        all_function_ids=list(dict.fromkeys(function_ids))
        function_ids=list(dict.fromkeys(i for i in function_ids if not selected_set or i in selected_set))
        if not function_ids:return
        unresolved=[]
        for fid in function_ids:
            row=by_id.get(fid)
            if row is None:unknown_ids.append(fid);continue
            if row.get('status') in accepted_statuses:accepted_filtered.append(fid);continue
            unresolved.append(fid)
        if not unresolved:return
        source,source_path=_frontier_path(raw['source']);target,target_path=_frontier_path(raw['target'])
        stale=[];source_hash=raw['source_sha256'];target_hash=raw['target_sha256'];flags=raw['flags']
        if not isinstance(flags,list):raise ValueError('flags must be an ordered array')
        if not re.fullmatch(r'[0-9a-f]{64}',str(source_hash)) or not re.fullmatch(r'[0-9a-f]{64}',str(target_hash)):
            raise ValueError('Handoff source/target hashes must be SHA-256')
        if not source_path.is_file() or digest(source_path.read_bytes())!=source_hash:stale.append('source_bytes')
        if not target_path.is_file() or digest(target_path.read_bytes())!=target_hash:stale.append('target_bytes')
        member_rows=[];profile_ok=True
        for fid in unresolved:
            row=by_id[fid]
            if row.get('source')!=source or row.get('target_sha256')!=target_hash or row.get('target')!=target:
                stale.append('ledger_binding:'+fid)
            if row.get('flags')!=flags:stale.append('ledger_flags:'+fid)
            proof_source=row.get('proof',{}).get('source_sha256')
            if proof_source and proof_source!=source_hash:stale.append('ledger_source_hash:'+fid)
            c=census_rows.get(fid)
            if not c:profile_ok=False
            else:
                if c.get('source')!=source or c.get('target_sha256')!=target_hash or c.get('flags')!=flags:
                    stale.append('census_profile:'+fid)
                if c.get('source_sha256_matches_current') is False:stale.append('census_source_hash:'+fid)
            if c and (c.get('size')!=row.get('size') or c.get('target_size')!=row.get('size')):
                stale.append('census_extent:'+fid)
            member_rows.append(row)
        profile_groups=[g for g in groups if g.get('source')==source and g.get('flags')==flags]
        for group in profile_groups:
            targets=_frontier_targets(group)
            if target in targets and (targets[target]!=target_hash or group.get('source_sha256')!=source_hash or
                    group.get('source_sha256_after',source_hash)!=source_hash or
                    group.get('matches_recorded_source_sha256') is False):
                stale.append('recorded_profile_hash_mismatch')
        compile_group=next((g for g in groups if g.get('source')==source and g.get('flags')==flags and
            g.get('source_sha256')==source_hash and g.get('matches_recorded_source_sha256') is not False and
            g.get('source_sha256_after',source_hash)==source_hash and g.get('status')=='compiled' and
            _frontier_targets(g).get(target)==target_hash),None)
        if compile_group and not _frontier_context(compile_group):stale.append('recorded_include_context')
        if compile_group and compile_group.get('target_sha256_after'):
            after=_frontier_targets(dict(target_sha256=compile_group['target_sha256_after']))
            if after.get(target)!=target_hash:stale.append('recorded_target_after')
        current_compile=(not stale and census.get('recovery_sha256')==ledger_sha and profile_ok and
            bool(compile_group) and bool(member_rows) and all(
                census_rows[fid].get('category') in ('raw_equal','reference_only') and
                census_rows[fid].get('source_sha256_matches_current',True) is not False
                for fid in unresolved))
        freshness='stale_inputs' if stale else ('current_profile_compile' if current_compile else 'recorded_only')
        context=dict(function_ids=unresolved,source=source,source_sha256=source_hash,flags=flags,
                     target=target,target_sha256=target_hash)
        task_id=identity(context)
        blockers=[]
        if not isinstance(raw['blockers'],list):raise ValueError('blockers must be an array')
        for b in raw['blockers']:
            if not isinstance(b,dict) or not b.get('kind') or not isinstance(b.get('requirement'),dict):
                raise ValueError('Each blocker requires kind and requirement object')
            affected=b.get('function_ids')
            if not isinstance(affected,list) or not affected or not set(affected)<=set(all_function_ids):
                raise ValueError('Each blocker must name its affected function_ids explicitly')
            affected=[fid for fid in affected if fid in unresolved]
            if not affected:continue
            pins=b.get('evidence_files')
            if not isinstance(pins,list) or not pins:raise ValueError('Blockers require at least one evidence pin')
            pin_map={(p['path'],p['sha256']):p for p in (_frontier_pin(pin) for pin in pins)}
            checked=sorted(pin_map.values(),key=lambda p:(p['path'],p['sha256']))
            pins_current=all(pin['current'] for pin in checked)
            typed=_typed_requirement(b['requirement'])
            shared_key=identity(dict(target_module_sha256=target_hash,requirement=b['requirement'],
                                     evidence_pins=[dict(path=p['path'],sha256=p['sha256']) for p in checked])) if typed and pins_current else None
            blockers.append(dict(kind=b['kind'],requirement=b['requirement'],function_ids=affected,
                evidence_files=checked,scope='shared_owner' if shared_key else 'function_local',
                blocker_key=shared_key,stale_evidence=not pins_current))
        task=dict(task_context_id=task_id,function_ids=unresolved,source=source,source_sha256=source_hash,
            flags=flags,target=target,target_sha256=target_hash,outcome=raw['outcome'],
            freshness=freshness,stale_reasons=sorted(set(stale)),blockers=blockers,
            dispatch_status='stale_inputs' if stale else 'preflight_required',
            region=raw.get('region'),family=raw.get('family'),region_context_id=raw.get('context_id'),
            ledger_status={fid:by_id[fid]['status'] for fid in unresolved},
            census_category={fid:census_rows[fid].get('category') for fid in unresolved if fid in census_rows},
            recorded_relocations={fid:census_rows[fid].get('relocation_count') for fid in unresolved if fid in census_rows},
            compiler_call_count=raw.get('compiler_call_count',0))
        if any(b['stale_evidence'] for b in blockers):
            task['freshness']='stale_inputs';task['dispatch_status']='stale_inputs'
            task['stale_reasons']=sorted(set(task['stale_reasons']+['blocker_evidence']))
        task['first_blocker']=raw.get('first_blocker') or (blockers[0]['kind'] if blockers else
            'No pinned handoff blocker; current per-caller preflight is required')
        existing=next((t for t in tasks if t['task_context_id']==task_id),None)
        if existing is None:tasks.append(task)
        else:
            known={(b['kind'],identity(b['requirement']),tuple(b['function_ids']),b['blocker_key']) for b in existing['blockers']}
            for blocker in task['blockers']:
                key=(blocker['kind'],identity(blocker['requirement']),tuple(blocker['function_ids']),blocker['blocker_key'])
                if key not in known:existing['blockers'].append(blocker);known.add(key)
            if task['freshness']=='stale_inputs' or any(b['stale_evidence'] for b in task['blockers']):
                existing['freshness']='stale_inputs';existing['dispatch_status']='stale_inputs'
                existing['stale_reasons']=sorted(set(existing['stale_reasons']+task['stale_reasons']+
                    (['blocker_evidence'] if any(b['stale_evidence'] for b in task['blockers']) else [])))
        covered_ids.update(unresolved)

    for handoff in handoffs:add_task(handoff)
    # Explicit ID-only requests produce function-local packets, never inferred owner groups.
    for fid in selected:
        if fid in covered_ids:continue
        row=by_id.get(fid)
        if row is None:unknown_ids.append(fid);continue
        if row.get('status') in accepted_statuses:accepted_filtered.append(fid);continue
        source,source_path=_frontier_path(row['source']);target,target_path=_frontier_path(row['target'])
        source_hash=digest(source_path.read_bytes()) if source_path.is_file() else ''
        add_task(dict(function_ids=[fid],source=source,source_sha256=source_hash,flags=row.get('flags',[]),
            target=target,target_sha256=row['target_sha256'],outcome='no_handoff',blockers=[]))

    omitted_tasks=tasks[limit:]
    tasks=tasks[:limit]
    knowledge_path=ROOT/'evidence/region_knowledge.json'
    knowledge=read(knowledge_path) if knowledge_path.exists() else {'observations':[]}
    knowledge_hash=digest(knowledge_path.read_bytes()) if knowledge_path.exists() else None
    groups_by_key={};local=[];context_cache={}
    for task in tasks:
        region=task.get('region');context=task.get('region_context_id')
        if region and region not in context_cache:context_cache[region]=_frontier_region_context(region,ledger)
        current_context=context_cache.get(region) if region else None
        matching=[n for n in knowledge.get('observations',[]) if region and task.get('family') and context and
            current_context==context and n.get('region')==region and n.get('family')==task['family'] and
            n.get('context_id')==task['region_context_id'] and n.get('verdict') in ('exhausted','falsified','blocked')]
        task['negative_knowledge']=matching
        task['negative_knowledge_state']='applicable_reentry_required' if matching else 'unknown'
        if matching and task['dispatch_status']!='stale_inputs':task['dispatch_status']='negative_reentry_required'
        for blocker in task['blockers']:
            if not blocker['blocker_key']:
                local.append(dict(task_context_id=task['task_context_id'],function_ids=blocker['function_ids'],
                                  kind=blocker['kind'],requirement=blocker['requirement']))
                continue
            prior=groups_by_key.get(blocker['blocker_key'])
            if prior and prior['target_module']!=task['target']:
                # Version one never shares a blocker group across distinct modules.
                local.append(dict(task_context_id=task['task_context_id'],function_ids=blocker['function_ids'],
                    kind=blocker['kind'],requirement=blocker['requirement'],reason='cross_module_no_shared_proof_profile'))
                continue
            group=groups_by_key.setdefault(blocker['blocker_key'],dict(blocker_key=blocker['blocker_key'],
                target_module_sha256=task['target_sha256'],requirement=blocker['requirement'],
                target_module=task['target'],evidence_files=blocker['evidence_files'],kind=blocker['kind'],members=[]))
            member=next((m for m in group['members'] if m['task_context_id']==task['task_context_id']),None)
            if member is None:group['members'].append(dict(task_context_id=task['task_context_id'],function_ids=blocker['function_ids']))
            else:member['function_ids']=sorted(set(member['function_ids'])|set(blocker['function_ids']))
    result=dict(scope='on-demand planning only; recovery.json remains the sole acceptance authority',
        ledger_sha256=ledger_sha,census_path=_frontier_path(census_path)[0],census_sha256=digest(census_file.read_bytes()),
        census_freshness='current_ledger_header' if census.get('recovery_sha256')==ledger_sha else 'stale_recovery_provenance',
        region_knowledge_sha256=knowledge_hash,candidates=tasks,shared_blockers=list(groups_by_key.values()),
        function_local_blockers=local,omitted_candidate_count=len(omitted_tasks),
        omitted_task_context_ids=[task['task_context_id'] for task in omitted_tasks],
        accepted_filtered=sorted(set(accepted_filtered)),unknown_ids=sorted(set(unknown_ids)))
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    q=sub.add_parser('packet');q.add_argument('region');q.add_argument('--more',action='store_true')
    f=sub.add_parser('matching-frontier',help='Emit current-hash-guarded matching candidates from explicit IDs/handoffs')
    f.add_argument('--census',type=Path,required=True);f.add_argument('--handoff',type=Path,action='append',default=[])
    f.add_argument('--ids',nargs='+');f.add_argument('--limit',type=int,default=8)
    r=sub.add_parser('record')
    for name in ('spec','source','baseline','report','meta'):r.add_argument('--'+name,type=Path,required=True)
    r.add_argument('--parent',type=Path,action='append',default=[])
    c=sub.add_parser('check');c.add_argument('--meta',type=Path,required=True);c.add_argument('--spec',type=Path,required=True);c.add_argument('--baseline',type=Path,required=True);c.add_argument('--prior',type=Path,action='append',default=[])
    m=sub.add_parser('materialize');m.add_argument('--receipt',type=Path,required=True);m.add_argument('--baseline',type=Path,required=True);m.add_argument('--output',type=Path,required=True)
    a=parser.parse_args()
    if a.command=='packet':packet(a.region,a.more)
    elif a.command=='matching-frontier':print(json.dumps(matching_frontier(a.census,a.handoff,a.ids,a.limit),indent=2))
    elif a.command=='record':record(a.spec,a.source,a.baseline,a.report,a.meta,a.parent)
    elif a.command=='check':check_reentry(read(a.meta),known_observations(a.prior),context_identity(read(a.spec),a.baseline.read_text()));print('New discriminator/context allowed; no proof or compilation cached.')
    else:
        if not any(a.output.resolve().is_relative_to(ROOT/f) for f in ('work','candidates')):raise ValueError('Materialize only in private scratch')
        content=apply_patch_text(a.baseline.read_text(),read(a.receipt)['source_patch'])
        a.output.parent.mkdir(parents=True,exist_ok=True)
        with a.output.open('xb') as out:out.write(content.encode())


if __name__=='__main__':main()
