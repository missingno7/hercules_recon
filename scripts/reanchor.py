"""Reprove accepted BSS and file-data-owner-v1 references with reviewed replacement native anchors.

No function acceptance, source/profile, cell identity/owner, relocation use or
instruction change is allowed. Original manifests and proofs remain provenance.
Both sides require fresh complete strict verification; publication is journaled.
"""
import argparse
import base64
import copy
import json
from pathlib import Path

from env import ROOT
from match import RELOCATED,accepted_rows,digest,reference_digest,verify_rows
from promote import JOURNAL,STATE,assert_verified_inputs,atomic_bytes,lock,recover


def anchor_container(record,name):
    """The only mutable native identity field for each supported storage form."""
    if record.get('kind') == 'bss':
        return record, 'identity'
    if record.get('kind') == 'data' and record.get('data_profile') == 'file-data-owner-v1':
        from file_data_owner import validate_record
        validate_record(record, name)
        return record['file_data'], 'anchor'
    raise ValueError('Only an existing BSS or file-data-owner-v1 anchor may change')


def native_anchor(record):
    # Inventory unsupported forms as absent; selected changes reject them below.
    if record.get('kind') == 'bss':
        return record.get('identity')
    if record.get('kind') == 'data' and record.get('data_profile') == 'file-data-owner-v1':
        return record.get('file_data', {}).get('anchor')
    return None


def derive(state,review,documents,review_path,review_sha):
    proposal=copy.deepcopy(state);byid={r['id']:r for r in state['functions']}
    if not review.get('reason') or review.get('schema')!=1 or not review.get('changes'):
        raise ValueError('Explicit anchor review required')
    derived={};touched=set()
    for change in review['changes']:
        if set(change)!={'bindings_path','bindings_sha256','symbol','function_ids','before','after'}:
            raise ValueError('Only native anchor fields may be reviewed')
        path=change['bindings_path'];content,document=documents[path]
        if digest(content)!=change['bindings_sha256']:raise ValueError('Original manifest baseline changed')
        if (not isinstance(change['before'],dict) or not isinstance(change['after'],dict) or
            set(change['before'])!={'rva','size','operand','sha256'} or set(change['after'])!=set(change['before']) or change['before']==change['after']):
            raise ValueError('One distinct complete native instruction anchor is required')
        name=change['symbol'];ids=change['function_ids']
        if not isinstance(ids,list) or not ids or len(set(ids))!=len(ids):raise ValueError('Exact unique accepted row inventory required')
        expected={r['id'] for r in state['functions'] if r['status']==RELOCATED and r['proof']['bindings_path']==path and
                  native_anchor(document['functions'][r['id']]['symbols'].get(name,{}))==change['before']}
        if set(ids)!=expected:raise ValueError('Review must cover every accepted use of that old anchor in the manifest')
        if path not in derived:derived[path]=copy.deepcopy(document)
        for fid in ids:
            if (fid,name) in touched:raise ValueError('Overlapping anchor changes')
            touched.add((fid,name));row=byid[fid]
            if row['status']!=RELOCATED or row['proof']['bindings_path']!=path:raise ValueError('Accepted relocated rows only')
            rec=derived[path]['functions'][fid]['symbols'][name]
            container,key=anchor_container(rec,name)
            if container.get(key)!=change['before']:raise ValueError('Existing native anchor differs from review')
            container[key]=copy.deepcopy(change['after'])
    manifests={}
    for path,document in derived.items():
        parent_sha=digest(documents[path][0])
        for name,sha in [(path,parent_sha),(review_path,review_sha)]:
            if name in document['evidence_files'] and document['evidence_files'][name]!=sha:raise ValueError('Conflicting immutable provenance')
            document['evidence_files'][name]=sha
        document['anchor_refresh_review']=review_path
        new_path='evidence/anchor_'+parent_sha[:12]+'_'+review_sha[:12]+'.json';manifests[path]=(new_path,document)
    changed_rows=[]
    for row in proposal['functions']:
        proof=row.get('proof',{})
        if row['status']!=RELOCATED or proof['bindings_path'] not in manifests:continue
        old_path=proof['bindings_path'];new_path,document=manifests[old_path];old_proof=copy.deepcopy(proof)
        old_binding=documents[old_path][1]['functions'][row['id']];new_binding=document['functions'][row['id']]
        # Independently enforce that every difference is exactly a reviewed identity.
        restored=copy.deepcopy(new_binding)
        for fid,name in touched:
            if fid==row['id']:
                container,key=anchor_container(restored['symbols'][name],name)
                container[key]=copy.deepcopy(native_anchor(old_binding['symbols'][name]))
        if restored!=old_binding:raise ValueError('Reference destination/owner/use changed')
        proof['bindings_path']=new_path;proof['bindings_sha256']=reference_digest(new_binding)
        identity={k:v for k,v in document.items() if k not in ('functions','evidence_files')}
        proof['reference_evidence_sha256']=reference_digest(dict(files=document['evidence_files'],identity_sha256=reference_digest(identity)))
        history=row.setdefault('reference_anchor_history',[])
        if not isinstance(history,list):raise ValueError('Invalid prior reference history')
        history.append(dict(review=review_path,parent_bindings_sha256=digest(documents[old_path][0]),proof=old_proof))
        changed_rows.append(row['id'])
    return proposal,manifests,sorted(touched),changed_rows


def reanchor(review_path,report,verify_only=False):
    review_path=Path(review_path).resolve();report=Path(report).resolve()
    if not review_path.is_relative_to(ROOT/'evidence') or not report.is_relative_to(ROOT/'work'):raise ValueError('Review in evidence/, report in work/')
    if JOURNAL.exists():raise ValueError('Recover the interrupted transaction first')
    content=review_path.read_bytes();review=json.loads(content);before=STATE.read_bytes();state=json.loads(before)
    if digest(before)!=review.get('baseline_recovery_sha256'):raise ValueError('Reviewed recovery baseline changed')
    observed={review_path:digest(content)};documents={}
    for change in review.get('changes',[]):
        path=(ROOT/change['bindings_path']).resolve()
        if not path.is_relative_to(ROOT/'evidence'):raise ValueError('Canonical evidence manifest required')
        raw=path.read_bytes();documents[change['bindings_path']]=(raw,json.loads(raw));observed[path]=digest(raw)
    for name,sha in review.get('evidence_files',{}).items():
        path=(ROOT/name).resolve()
        if not path.is_relative_to(ROOT/'evidence') or digest(path.read_bytes())!=sha:raise ValueError('Native review evidence changed')
        observed[path]=sha
    proposal,manifests,touched,changed_rows=derive(state,review,documents,review_path.relative_to(ROOT).as_posix(),digest(content))
    baseline,baseline_stats=verify_rows(accepted_rows(state['functions']))
    if not all(r['exact'] for r in baseline):raise ValueError('Baseline strict proof failed')
    print('Baseline verified:',len(baseline),'strict proofs',flush=True)
    for _,(name,document) in manifests.items():
        path=ROOT/name;payload=(json.dumps(document,indent=2)+'\n').encode()
        if path.exists() and path.read_bytes()!=payload:raise ValueError('Conflicting immutable descendant manifest')
        atomic_bytes(path,payload);observed[path]=digest(payload)
    rows=accepted_rows(proposal['functions']);results,stats=verify_rows(rows)
    if not all(r['exact'] for r in results):raise ValueError('Replacement anchor failed the complete strict verifier')
    prior={r['id']:r for r in baseline}
    for result in results:
        for key in ('compiled_sha256','target_sha256','proof_category','source_sha256','toolchain','context_sha256'):
            if result.get(key)!=prior[result['id']].get(key):raise ValueError('Accepted function identity/context changed')
    linked=None
    if any(r['status']=='LINKED_MATCH' for r in state['functions']):
        from linked import verify
        linked=verify()
    if STATE.read_bytes()!=before:raise ValueError('Recovery state changed during reproof')
    for path,sha in observed.items():
        if digest(path.read_bytes())!=sha:raise ValueError('Review or manifest changed during reproof')
    assert_verified_inputs(rows,results)
    after=(json.dumps(proposal,indent=2)+'\n').encode()
    receipt=dict(scope='Same accepted functions, source/profile, cell identity/owner and reference uses; only reviewed BSS/file-data-owner-v1 instruction anchors and immutable provenance changed.',strict_proofs=len(results),new_acceptances=0,changed_reference_anchors=touched,reproven_manifest_rows=changed_rows,baseline_statistics=baseline_stats,statistics=stats,before_recovery_sha256=digest(before),after_recovery_sha256=digest(after),review_sha256=digest(content),derived_manifests=[p for p,_ in manifests.values()],results=results,verify_only=verify_only)
    report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(receipt,indent=2)+'\n')
    if not verify_only:
        journal=[dict(path='recovery.json',before=base64.b64encode(before).decode(),before_sha256=digest(before),after_sha256=digest(after))]
        atomic_bytes(JOURNAL,json.dumps(journal).encode())
        try:atomic_bytes(STATE,after)
        except BaseException:recover();raise
        JOURNAL.unlink()
    print('Verified' if verify_only else 'Reanchored',len(touched),'references;',len(results),'strict proofs; no new acceptance')
    return receipt


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--review',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify-only',action='store_true');args=p.parse_args()
    with lock():reanchor(args.review,args.report,args.verify_only)


if __name__=='__main__':main()
