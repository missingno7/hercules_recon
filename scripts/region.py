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
from match import digest,compare,verify_toolchain,DEFAULT_TOOLCHAIN


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
    return identity(dict(target=spec['target_sha256'],functions=spec['functions'],symbols=spec.get('symbol_rvas',{}),
        flags=spec['flags'],baseline_source=digest(baseline.encode()),toolchain_manifest=digest((ROOT/'toolchains/manifest.json').read_bytes())))


def record(spec_path,source,baseline,report_path,meta_path,parents=()):
    spec=read(spec_path);scope=ownership(spec);meta=read(meta_path);validate_meta(meta)
    if meta['region']!=scope['id']:raise ValueError('Region metadata disagreement')
    if not re.fullmatch(r'[a-z0-9_-]+',meta['lane']):raise ValueError('Unsafe lane name')
    lane=ROOT/'work'/meta['lane'];lane.mkdir(parents=True,exist_ok=True)
    report=read(report_path);text=source.read_text();base=baseline.read_text()
    if report['source_sha256']!=digest(source.read_bytes()):raise ValueError('Source changed since compile')
    if report['target_sha256']!=spec['target_sha256']:raise ValueError('Wrong oracle report')
    if report.get('flags')!=spec['flags']:raise ValueError('Compile flags disagree')
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


def packet(region_id,more=False):
    if not re.fullmatch(r'[a-z0-9_-]+',region_id):raise ValueError('Unsafe region name')
    catalog=read(ROOT/'evidence/region_index.json');entry=next(r for r in catalog['regions'] if r['id']==region_id)
    state=read(ROOT/'recovery.json');spec=read(ROOT/entry['spec'])
    scope=ownership(spec);base=(ROOT/entry['baseline']).read_text();ctx=context_identity(spec,base)
    receipts=[read(ROOT/p) for p in entry.get('receipts',[])]
    current=[r for r in receipts if r['context_id']==ctx]
    accepted={(r['target_sha256'],int(r['rva'],0)):r for r in state['functions'] if r['status']=='FUNCTION_MATCH'}
    unresolved=[f['symbol'] for f in spec['functions'] if (spec['target_sha256'],int(f['rva'],0)) not in accepted]
    closed={f['symbol'] for f in spec['functions'] if f['symbol'] not in unresolved}
    safe=[r for r in current if all(f['strict_equal'] for f in r['functions'] if f['symbol'] in closed)]
    candidates=frontier(safe) if unresolved else []
    notes=read(ROOT/'evidence/region_knowledge.json') if (ROOT/'evidence/region_knowledge.json').exists() else {'observations':[]}
    result=dict(authority='recovery.json only; this packet is disposable',region=scope,context_id=ctx,
        accepted_total=len(accepted),unresolved=unresolved,canonical_in_region=len(spec['functions'])-len(unresolved),
        frontier=[dict(receipt_id=r['receipt_id'],source_sha256=r['source_sha256'],output_id=r['output_id'],summary=r['summary'],
                       next_discriminator=r.get('next_discriminator'),ungated_work=r.get('ungated_work')) for r in candidates[:3]],
        additional_pareto_receipts=max(0,len(candidates)-3),stale_receipts=[r['receipt_id'] for r in receipts if r not in current],
        tried=[{k:r.get(k) for k in ('family','verdict','prediction','falsifier','blocker','reentry_condition','output_id')} for r in current],
        negative_knowledge=[n for n in notes['observations'] if n['region']==region_id and unresolved],
        progressive_references=entry.get('references',[]),model_policy='Luna xhigh normally; Sol high for unresolved explanation. See AGENTS.md.',
        baseline_source=entry['baseline'],baseline_sha256=digest(base.encode()))
    if more:result['functions']=[dict(f,accepted_id=accepted.get((spec['target_sha256'],int(f['rva'],0)),{}).get('id')) for f in spec['functions']]
    out=ROOT/'work/packets';out.mkdir(parents=True,exist_ok=True)
    path=out/(region_id+'-'+identity(result)[:16]+'.json')
    path.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(json.dumps(result,indent=2));return path


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    q=sub.add_parser('packet');q.add_argument('region');q.add_argument('--more',action='store_true')
    r=sub.add_parser('record')
    for name in ('spec','source','baseline','report','meta'):r.add_argument('--'+name,type=Path,required=True)
    r.add_argument('--parent',type=Path,action='append',default=[])
    c=sub.add_parser('check');c.add_argument('--meta',type=Path,required=True);c.add_argument('--spec',type=Path,required=True);c.add_argument('--baseline',type=Path,required=True);c.add_argument('--prior',type=Path,action='append',default=[])
    m=sub.add_parser('materialize');m.add_argument('--receipt',type=Path,required=True);m.add_argument('--baseline',type=Path,required=True);m.add_argument('--output',type=Path,required=True)
    a=parser.parse_args()
    if a.command=='packet':packet(a.region,a.more)
    elif a.command=='record':record(a.spec,a.source,a.baseline,a.report,a.meta,a.parent)
    elif a.command=='check':check_reentry(read(a.meta),known_observations(a.prior),context_identity(read(a.spec),a.baseline.read_text()));print('New discriminator/context allowed; no proof or compilation cached.')
    else:
        if not any(a.output.resolve().is_relative_to(ROOT/f) for f in ('work','candidates')):raise ValueError('Materialize only in private scratch')
        content=apply_patch_text(a.baseline.read_text(),read(a.receipt)['source_patch'])
        a.output.parent.mkdir(parents=True,exist_ok=True)
        with a.output.open('xb') as out:out.write(content.encode())


if __name__=='__main__':main()
