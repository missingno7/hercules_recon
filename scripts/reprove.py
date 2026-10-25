"""Refresh one reviewed source context without changing any accepted function proof.

No acceptance, binding, compiler profile, oracle or extent changes are allowed.
The existing strict verifier recompiles every accepted row before and after the
edit; the existing promotion journal restores source/state on any failure.
"""
import argparse
import base64
import copy
import json
from pathlib import Path

from env import ROOT
from coff import COFF
from match import (ACCEPTED, RELOCATED, DEFAULT_TOOLCHAIN, accepted_rows,
                   compile_inputs, context_proof, digest, reference_digest,
                   verify_rows)
from promote import (JOURNAL, STATE, assert_verified_inputs, atomic_bytes, lock,
                     recover)


def contribution(section):
    return dict(size=section['size'], data=section['data'].hex(),
                relocations=section['relocations'], flags=section['flags'])


def changed_functions(before, after, allowed):
    """Protect every other function and every non-code contribution."""
    a, b = COFF(before), COFF(after)
    def functions(obj):
        return {s['name']: contribution(obj.function(s['name']))
                for s in obj.symbols.values() if s['section'] > 0 and s['type'] & 0x20}
    left, right = functions(a), functions(b)
    if left.keys() != right.keys() or not set(allowed) <= left.keys():
        raise ValueError('Reviewed refresh cannot add/remove functions or name unknown functions')
    changed = {name for name in left if left[name] != right[name]}
    if not changed or changed != set(allowed):
        raise ValueError('Changed functions differ from the exact reviewed inventory')
    def noncode(obj):
        # Preserve data, linkage directives and COMMON definitions as well.
        sections = [dict(name=s['name'], **contribution(s)) for s in obj.sections
                    if not s['flags'] & 0x20]
        commons = sorted((s['name'], s['value'], s['type'], s['storage'])
                         for s in obj.symbols.values() if s['section'] == 0)
        return sections, commons
    if noncode(a) != noncode(b):
        raise ValueError('Reviewed source-only refresh cannot change non-code storage or imports')
    return sorted(changed)


def proposed_state(state, source, before_hash, after_hash, manifest_paths, context):
    """Change context metadata only; retain all actual byte/reference hashes."""
    result = copy.deepcopy(state)
    for row in result['functions']:
        proof = row.get('proof', {})
        if row.get('source') == source and row['status'] in ACCEPTED | {'CODEGEN_SIMILAR'}:
            if proof.get('source_sha256') != before_hash:
                raise ValueError('Source row has a stale baseline: ' + row['id'])
            proof['source_sha256'] = after_hash
            if row['status'] == RELOCATED:
                proof['context_sha256'] = context_proof(context)
            elif row['status'] in ACCEPTED:
                raise ValueError('First context-refresh implementation supports relocated source peers only')
        if row['status'] == RELOCATED and proof['bindings_path'] in manifest_paths:
            new_path, document = manifest_paths[proof['bindings_path']]
            binding = document['functions'][row['id']]
            if reference_digest(binding) != proof['bindings_sha256']:
                raise ValueError('Context refresh cannot change accepted reference bindings')
            identity = {k:v for k,v in document.items() if k not in ('functions','evidence_files')}
            proof['bindings_path'] = new_path
            proof['reference_evidence_sha256'] = reference_digest(dict(
                files=document['evidence_files'], identity_sha256=reference_digest(identity)))
    return result


def refresh(review_path, candidate, report, verify_only=False):
    if JOURNAL.exists():
        raise ValueError('Interrupted promotion: recover it before refreshing source context')
    review_path, candidate, report = map(lambda p:Path(p).resolve(), (review_path,candidate,report))
    if not review_path.is_relative_to(ROOT/'evidence') or not report.is_relative_to(ROOT/'work'):
        raise ValueError('Reviewed evidence belongs in evidence/ and generated reports in work/')
    review_bytes = review_path.read_bytes(); review = json.loads(review_bytes)
    if review.get('schema') != 1 or not review.get('reason') or not review.get('changed_symbols'):
        raise ValueError('Explicit source review and exact changed-symbol inventory are required')
    destination = (ROOT/review['source']).resolve()
    if not destination.is_relative_to(ROOT/'src') or destination.suffix not in ('.c','.cpp'):
        raise ValueError('Refresh one existing source unit under src/')
    if not candidate.is_relative_to(ROOT/'candidates') or candidate.suffix != destination.suffix:
        raise ValueError('Candidate must use the same language under candidates/')
    before_source, after_source, before_state = destination.read_bytes(), candidate.read_bytes(), STATE.read_bytes()
    if (digest(before_source) != review['before_sha256'] or
            digest(after_source) != review['after_sha256'] or before_source == after_source):
        raise ValueError('Reviewed source baseline/candidate mismatch')
    evidence_inputs = {review_path:digest(review_bytes), candidate:digest(after_source)}
    for name, expected in review.get('evidence_files',{}).items():
        path=(ROOT/name).resolve()
        if not path.is_relative_to(ROOT/'evidence') or digest(path.read_bytes()) != expected:
            raise ValueError('Reviewed original evidence changed')
        evidence_inputs[path]=expected
    state=json.loads(before_state); rows=accepted_rows(state['functions'])
    members=[r for r in rows if r['source']==review['source']]
    if not members or any(r['status'] != RELOCATED for r in members):
        raise ValueError('Refresh requires existing relocated peers, preserving their category')
    flags={tuple(r['flags']) for r in members}
    if len(flags)!=1:raise ValueError('One ordered compile profile per source refresh')
    # Verify pristine historical evidence before proposing new context metadata.
    baseline, baseline_stats=verify_rows(rows)
    if not all(r['exact'] for r in baseline):raise ValueError('Baseline strict proof failed')
    print('Baseline verified:',len(rows),'strict proofs',flush=True)
    baseline_by_id={r['id']:r for r in baseline}
    baseline_object=baseline_by_id[members[0]['id']]['object']
    manifests={};manifest_frozen={}
    for path in sorted({r['proof']['bindings_path'] for r in rows if r['status']==RELOCATED}):
        absolute=ROOT/path; content=absolute.read_bytes(); document=json.loads(content)
        if review['source'] not in document['evidence_files']:continue
        if document['evidence_files'][review['source']]!=review['before_sha256']:
            raise ValueError('Reference manifest has another source baseline')
        updated=copy.deepcopy(document)
        updated['evidence_files'][review['source']]=review['after_sha256']
        updated['evidence_files'][path]=digest(content)
        updated['evidence_files'][review_path.relative_to(ROOT).as_posix()]=digest(review_bytes)
        # Original manifests remain immutable; descendants pin their exact bytes.
        derived=ROOT/'evidence'/('context_'+digest(content)[:12]+'_'+review['after_sha256'][:12]+'.json')
        payload=(json.dumps(updated,indent=2)+'\n').encode()
        if derived.exists() and derived.read_bytes()!=payload:
            raise ValueError('Conflicting immutable derived manifest')
        manifests[path]=(derived.relative_to(ROOT).as_posix(),updated)
        manifest_frozen[absolute]=digest(content)
        atomic_bytes(derived,payload)
    observed={**evidence_inputs,**manifest_frozen}
    for new_path,_ in manifests.values():observed[ROOT/new_path]=digest((ROOT/new_path).read_bytes())
    journal=[dict(path=review['source'],before=base64.b64encode(before_source).decode(),before_sha256=digest(before_source),after_sha256=digest(after_source)),
             dict(path='recovery.json',before=base64.b64encode(before_state).decode(),before_sha256=digest(before_state),after_sha256=digest(before_state))]
    if STATE.read_bytes()!=before_state or destination.read_bytes()!=before_source:
        raise ValueError('Canonical inputs changed before refresh')
    atomic_bytes(JOURNAL,json.dumps(journal).encode())
    try:
        atomic_bytes(destination,after_source)
        context=compile_inputs(destination,list(next(iter(flags))),DEFAULT_TOOLCHAIN)
        proposal=proposed_state(state,review['source'],review['before_sha256'],review['after_sha256'],manifests,context)
        proposed_rows=accepted_rows(proposal['functions'])
        results,statistics=verify_rows(proposed_rows)
        if not all(r['exact'] for r in results):raise ValueError('Fresh strict proof failed after source edit')
        print('Updated context verified:',len(results),'strict proofs',flush=True)
        after_by_id={r['id']:r for r in results}
        for row in rows:
            old,new=baseline_by_id[row['id']],after_by_id[row['id']]
            for key in ('compiled_sha256','target_sha256','proof_category'):
                if old[key]!=new[key]:raise ValueError('Accepted function byte identity changed: '+row['id'])
        changed=changed_functions(baseline_object,after_by_id[members[0]['id']]['object'],review['changed_symbols'])
        # Recheck diagnostic peers in the edited source; only hash context may change.
        from masked import masked_compare
        from similar import masked_identity
        for row in proposal['functions']:
            if row['source']!=review['source'] or row['status']!='CODEGEN_SIMILAR':continue
            result=masked_compare(after_by_id[members[0]['id']]['object'],row['symbol'],ROOT/row['target'],int(row['rva'],0),row['size'])
            if (not result['masked_equal'] or masked_identity(after_by_id[members[0]['id']]['object'],row['symbol'])!=row['proof']['masked_sha256'] or
                    result['symbols']!=row['link_symbols']):
                raise ValueError('Diagnostic peer identity changed: '+row['id'])
        linked=None
        if any(r['status']=='LINKED_MATCH' for r in state['functions']):
            from linked import verify
            linked=verify()
        for path,expected in observed.items():
            if digest(path.read_bytes())!=expected:raise ValueError('Review or manifest changed during refresh')
        if destination.read_bytes()!=after_source or STATE.read_bytes()!=before_state:
            raise ValueError('Concurrent source/state edit during refresh')
        assert_verified_inputs(proposed_rows,results)
        after_state=(json.dumps(proposal,indent=2)+'\n').encode()
        journal[1]['after_sha256']=digest(after_state)
        atomic_bytes(JOURNAL,json.dumps(journal).encode())
        receipt=dict(scope='Same accepted functions, exact bytes and bindings; changed unaccepted consumer code only.',source=review['source'],changed_functions=changed,
                     baseline_statistics=baseline_stats,statistics=statistics,strict_proofs=len(results),accepted_rows_preserved=len(rows),
                     status_changes=0,new_acceptances=0,reference_binding_changes=0,derived_manifests=[p for p,_ in manifests.values()],
                     before_recovery_sha256=digest(before_state),after_recovery_sha256=digest(after_state),
                     review_sha256=digest(review_bytes),results=results,verify_only=verify_only)
        report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(receipt,indent=2)+'\n')
        if verify_only:recover()
        else:
            atomic_bytes(STATE,after_state)
            JOURNAL.unlink()
    except BaseException:
        recover()
        raise
    print('Verified' if verify_only else 'Refreshed',review['source'],len(rows),'strict proofs; no acceptance, byte or binding changes')
    return receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--review',type=Path,required=True)
    parser.add_argument('--candidate',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify-only',action='store_true')
    args=parser.parse_args()
    with lock():refresh(args.review,args.candidate,args.report,args.verify_only)


if __name__=='__main__':main()
