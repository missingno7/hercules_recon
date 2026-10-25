"""Read-only matching-frontier grouping and stale-input guards."""
import json
from contextlib import redirect_stdout
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import region


class MatchingFrontierTests(unittest.TestCase):
    def setUp(self):
        work=Path(__file__).resolve().parents[1]/'work'
        self.temp=tempfile.TemporaryDirectory(dir=work)
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        patch.object(region,'ROOT',self.root).start()
        self.addCleanup(patch.stopall)
        for path in ('src','originals','evidence','work','toolchains'):(self.root/path).mkdir(parents=True,exist_ok=True)
        (self.root/'toolchains/manifest.json').write_text('{}\n',encoding='utf-8')
        (self.root/'originals/a.dll').write_bytes(b'original-a')
        (self.root/'originals/b.dll').write_bytes(b'original-b')
        (self.root/'originals/c.dll').write_bytes(b'original-a')
        self.targets={'a':('originals/a.dll',region.digest(b'original-a')),
                      'b':('originals/b.dll',region.digest(b'original-b')),
                      'c':('originals/c.dll',region.digest(b'original-a'))}
        self.files={};self.add_function('f1','src/a.c','a')
        self.owner_evidence=self.write_file('evidence/owner.json',b'shared owner proof')
        self.write_state_and_census()

    def write_file(self,name,data):
        path=self.root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        return dict(path=name,sha256=region.digest(data))

    def add_function(self,fid,source,target,status='CODEGEN_SIMILAR'):
        path=self.root/source;path.parent.mkdir(parents=True,exist_ok=True)
        if not path.exists():path.write_text('source '+fid+'\n',encoding='utf-8')
        self.files[fid]=dict(id=fid,status=status,target=self.targets[target][0],target_sha256=self.targets[target][1],
            rva='0x1000',size=8,source=source,flags=['/O2'],symbol='_'+fid,
            proof={'source_sha256':region.digest(path.read_bytes())},link_symbols={'_lookalike':'0x1000'})

    def write_state_and_census(self,recovery_sha=None):
        state={'functions':list(self.files.values())}
        path=self.root/'recovery.json';path.write_text(json.dumps(state),encoding='utf-8')
        rows=[];compile_groups=[];profiles=set()
        for row in self.files.values():
            source_path=self.root/row['source'];source_hash=region.digest(source_path.read_bytes())
            rows.append(dict(id=row['id'],module=row['target'].split('/')[-1],source=row['source'],flags=row['flags'],
                target_sha256=row['target_sha256'],rva=row['rva'],size=row['size'],compiled_size=row['size'],
                target_size=row['size'],category='reference_only',relocation_count=1,
                source_sha256_matches_current=True))
            key=(row['source'],tuple(row['flags']),row['target_sha256'])
            if key not in profiles:
                profiles.add(key)
                compile_groups.append(dict(source=row['source'],flags=row['flags'],source_sha256=source_hash,
                    matches_recorded_source_sha256=True,status='compiled',
                    target_sha256={row['target']:row['target_sha256']}))
        census={'recovery_sha256':recovery_sha or region.digest(path.read_bytes()),'rows':rows,
                'compile_group_results':compile_groups}
        (self.root/'work/census.json').write_text(json.dumps(census),encoding='utf-8')
        self.census_path=Path('work/census.json')

    def task(self,fid,requirement=None,pins=None,kind='typed_owner'):
        row=self.files[fid]
        blocker=[]
        if requirement is not None:
            blocker=[dict(kind=kind,requirement=requirement,function_ids=[fid],
                          evidence_files=pins or [self.owner_evidence])]
        return dict(function_ids=[fid],source=row['source'],source_sha256=region.digest((self.root/row['source']).read_bytes()),
            flags=row['flags'],target=row['target'],target_sha256=row['target_sha256'],outcome='needs_reference_review',
            blockers=blocker)

    def write_handoffs(self,tasks):
        paths=[]
        for i,task in enumerate(tasks):
            path=self.root/f'work/handoff{i}.json';path.write_text(json.dumps(task),encoding='utf-8');paths.append(path.relative_to(self.root))
        return paths

    def run_frontier(self,tasks=None,ids=None,limit=8):
        return region.matching_frontier(self.census_path,self.write_handoffs(tasks or []),ids,limit)

    def test_same_pinned_typed_owner_collapses_across_callers(self):
        self.add_function('f2','src/b.c','a')
        self.write_state_and_census()
        req={'owner':{'kind':'actor_resource_table','type':'ActorResource[20-byte stride]'},
             'abi':{'calling_convention':'cdecl','argument':'Actor*'}}
        result=self.run_frontier([self.task('f1',req),self.task('f2',req)])
        self.assertEqual(len(result['shared_blockers']),1)
        self.assertEqual(len(result['shared_blockers'][0]['members']),2)
        self.assertEqual(len(result['candidates']),2)
        self.assertEqual({t['source'] for t in result['candidates']},{'src/a.c','src/b.c'})
        self.assertTrue(all(t['freshness']=='current_profile_compile' for t in result['candidates']))
        self.assertTrue(all(t['dispatch_status']=='preflight_required' for t in result['candidates']))

    def test_selected_ids_covered_by_multifunction_handoff_do_not_duplicate(self):
        self.add_function('f2','src/a.c','a');self.write_state_and_census()
        req={'owner':{'kind':'table','type':'DWORD[2]'},'abi':{'profile':'cdecl'}}
        task=self.task('f1',req);task['function_ids']=['f1','f2']
        task['blockers'][0]['function_ids']=['f1','f2']
        result=self.run_frontier([task],ids=['f1','f2'])
        self.assertEqual(len(result['candidates']),1)
        self.assertEqual(result['candidates'][0]['function_ids'],['f1','f2'])
        self.assertEqual(result['shared_blockers'][0]['members'][0]['function_ids'],['f1','f2'])
        self.assertEqual(result['function_local_blockers'],[])

    def test_limit_reports_omitted_task_contexts(self):
        self.add_function('f2','src/b.c','a');self.write_state_and_census()
        tasks=[self.task('f1'),self.task('f2')]
        result=self.run_frontier(tasks,limit=1)
        self.assertEqual(len(result['candidates']),1)
        self.assertEqual(result['omitted_candidate_count'],1)
        self.assertEqual(len(result['omitted_task_context_ids']),1)

    def test_different_module_owner_or_evidence_does_not_merge(self):
        self.add_function('f2','src/b.c','a');self.add_function('f3','src/c.c','b')
        self.add_function('f4','src/d.c','a');self.add_function('f5','src/e.c','a');self.add_function('f6','src/f.c','c')
        other=self.write_file('evidence/other.json',b'different pin')
        self.write_state_and_census()
        req={'owner':{'kind':'callback','type':'Actor*'},'abi':{'profile':'cdecl Actor*'}}
        tasks=[self.task('f1',req),self.task('f2',req),self.task('f3',req),
               self.task('f4',req,[other]),self.task('f5',{'owner':{'kind':'other','type':'Actor*'},'abi':req['abi']}),
               self.task('f6',req)]
        result=self.run_frontier(tasks)
        self.assertEqual(len(result['shared_blockers']),4)
        self.assertEqual(sorted(len(g['members']) for g in result['shared_blockers']),[1,1,1,2])
        cross_module=[b for b in result['function_local_blockers'] if b.get('reason')=='cross_module_no_shared_proof_profile']
        self.assertEqual([b['function_ids'] for b in cross_module],[['f6']])

    def test_stale_header_is_recorded_only_and_changed_source_target_are_stale(self):
        task=self.task('f1',{'owner':{'kind':'one','type':'word'},'abi':{'profile':'cdecl'}})
        self.write_state_and_census(recovery_sha='0'*64)
        result=self.run_frontier([task])
        self.assertEqual(result['candidates'][0]['freshness'],'recorded_only')
        self.assertEqual(result['candidates'][0]['dispatch_status'],'preflight_required')
        source=self.root/'src/a.c';source.write_text('changed source\n',encoding='utf-8')
        result=self.run_frontier([task])
        self.assertEqual(result['candidates'][0]['dispatch_status'],'stale_inputs')
        source.write_text('source f1\n',encoding='utf-8')
        target=self.root/'originals/a.dll';target.write_bytes(b'changed target')
        result=self.run_frontier([task])
        self.assertEqual(result['candidates'][0]['dispatch_status'],'stale_inputs')
        target.write_bytes(b'original-a')
        evidence=self.root/self.owner_evidence['path'];evidence.write_bytes(b'changed owner proof')
        result=self.run_frontier([task])
        self.assertEqual(result['candidates'][0]['freshness'],'stale_inputs')
        self.assertEqual(result['shared_blockers'],[])

    def test_accepted_rows_are_filtered_before_frontier(self):
        self.files['f1']['status']='FUNCTION_MATCH';self.write_state_and_census()
        result=self.run_frontier([self.task('f1')])
        self.assertEqual(result['candidates'],[])
        self.assertEqual(result['accepted_filtered'],['f1'])

    def test_unclassified_reference_stays_function_local_without_symbol_inference(self):
        task=self.task('f1',{'classification':'unknown'},kind='unclassified_reference')
        result=self.run_frontier([task])
        self.assertEqual(result['shared_blockers'],[])
        self.assertEqual(result['function_local_blockers'][0]['function_ids'],['f1'])
        self.assertNotIn('link_symbols',json.dumps(result))
        self.assertEqual(result['candidates'][0]['blockers'][0]['scope'],'function_local')

    def test_negative_family_requires_exact_live_context_and_carries_reentry(self):
        target, target_hash=self.targets['a']
        baseline=(self.root/'src/a.c').read_text(encoding='utf-8')
        spec=dict(source='src/a.c',region=dict(id='demo',ownership='function_task'),target=target,
                  target_sha256=target_hash,flags=['/O2'],functions=[dict(symbol='_f1',rva='0x1000',size=8)])
        (self.root/'evidence/spec.json').write_text(json.dumps(spec),encoding='utf-8')
        (self.root/'evidence/region_index.json').write_text(json.dumps({'regions':[dict(id='demo',
            spec='evidence/spec.json',baseline='src/a.c')]}),encoding='utf-8')
        context=region.context_identity(spec,baseline)
        observation=dict(region='demo',family='layout',context_id=context,verdict='exhausted',
                         reentry_condition='new native field-offset evidence')
        (self.root/'evidence/region_knowledge.json').write_text(json.dumps({'observations':[observation]}),encoding='utf-8')
        exact=self.task('f1');exact.update(region='demo',family='layout',context_id=context)
        result=self.run_frontier([exact])
        self.assertEqual(result['candidates'][0]['negative_knowledge_state'],'applicable_reentry_required')
        self.assertEqual(result['candidates'][0]['dispatch_status'],'negative_reentry_required')
        self.assertEqual(result['candidates'][0]['negative_knowledge'][0]['reentry_condition'],observation['reentry_condition'])
        changed=dict(exact,context_id='different-context')
        result=self.run_frontier([changed])
        self.assertEqual(result['candidates'][0]['negative_knowledge_state'],'unknown')
        self.assertEqual(result['candidates'][0]['dispatch_status'],'preflight_required')

    def test_cli_emits_json_without_writing_a_packet_or_state(self):
        before=set(self.root.rglob('*'))
        with patch('sys.argv',['region.py','matching-frontier','--census',str(self.root/'work/census.json'),
                               '--ids','f1']),redirect_stdout(io.StringIO()) as output:
            region.main()
        result=json.loads(output.getvalue())
        self.assertEqual(result['candidates'][0]['function_ids'],['f1'])
        self.assertEqual(set(self.root.rglob('*')),before)


if __name__=='__main__':unittest.main()
