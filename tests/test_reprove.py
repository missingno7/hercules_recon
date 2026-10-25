"""Context refresh preserves byte identities, bindings and source ownership."""
import copy
import json
from contextlib import ExitStack
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import reprove
import promote
from match import reference_digest,context_proof
from test_matching import workspace_temporary_directory


class FakeObject:
    def __init__(self):
        self.symbols={0:dict(name='_accepted',section=1,type=0x20),
                      1:dict(name='_consumer',section=2,type=0x20),
                      2:dict(name='_cell',section=0,type=0,storage=2,value=4)}
        self.sections=[dict(name='.text',flags=0x60001020,size=1,data=b'\xc3',relocations=[]),
                       dict(name='.text',flags=0x60001020,size=1,data=b'\xc3',relocations=[]),
                       dict(name='.data',flags=0xc0000040,size=1,data=b'\0',relocations=[])]
    def function(self,name):return self.sections[next(s['section']-1 for s in self.symbols.values() if s['name']==name)]


class ReproveTests(unittest.TestCase):
    def pair(self):
        a=FakeObject();b=copy.deepcopy(a);b.sections[1]['data']=b'\x90'
        return a,b
    def check_pair(self,a,b,allowed=('_consumer',)):
        with patch.object(reprove,'COFF',side_effect=[a,b]):
            return reprove.changed_functions('before','after',allowed)
    def test_exact_changed_function_inventory(self):
        self.assertEqual(self.check_pair(*self.pair()),['_consumer'])
    def test_unlisted_function_change_rejected(self):
        a,b=self.pair();b.sections[0]['data']=b'\x90'
        with self.assertRaisesRegex(ValueError,'exact reviewed'):self.check_pair(a,b)
    def test_relocation_destination_change_is_function_change(self):
        a,b=self.pair();b.sections[0]['relocations']=[dict(offset=0,type=6,symbol='_other')]
        with self.assertRaisesRegex(ValueError,'exact reviewed'):self.check_pair(a,b)
    def test_extra_function_rejected(self):
        a,b=self.pair();b.symbols[3]=dict(name='_new',section=2,type=0x20)
        with self.assertRaisesRegex(ValueError,'add/remove'):self.check_pair(a,b)
    def test_data_payload_change_rejected(self):
        a,b=self.pair();b.sections[2]['data']=b'\1'
        with self.assertRaisesRegex(ValueError,'non-code'):self.check_pair(a,b)
    def test_common_extent_change_rejected(self):
        a,b=self.pair();b.symbols[2]['value']=2
        with self.assertRaisesRegex(ValueError,'non-code'):self.check_pair(a,b)
    def test_import_inventory_change_rejected(self):
        a,b=self.pair();b.symbols[2]['name']='_other'
        with self.assertRaisesRegex(ValueError,'non-code'):self.check_pair(a,b)
    def proposal(self):
        proof=dict(source_sha256='before',compiled_sha256='old-exact-code',target_sha256='native',
                   bindings_path='evidence/old.json',bindings_sha256=reference_digest(dict(symbols={})),
                   context_sha256={'src/unit.c':'before'})
        state=dict(functions=[dict(id='test',status=reprove.RELOCATED,source='src/unit.c',proof=proof)])
        document=dict(schema=1,evidence_files={'src/unit.c':'after'},functions={'test':dict(symbols={})})
        return state,document
    def test_context_only_fields_refresh_and_old_state_unchanged(self):
        state,document=self.proposal();before=copy.deepcopy(state)
        result=reprove.proposed_state(state,'src/unit.c','before','after',{'evidence/old.json':('evidence/new.json',document)}, {reprove.ROOT/'src/unit.c':'after'})
        self.assertEqual(state,before)
        row=result['functions'][0];proof=row['proof']
        self.assertEqual(row['status'],reprove.RELOCATED)
        self.assertEqual(proof['compiled_sha256'],'old-exact-code');self.assertEqual(proof['target_sha256'],'native')
        self.assertEqual(proof['bindings_sha256'],before['functions'][0]['proof']['bindings_sha256'])
        self.assertEqual(proof['source_sha256'],'after');self.assertEqual(proof['bindings_path'],'evidence/new.json')
    def test_binding_change_rejected(self):
        state,document=self.proposal();document['functions']['test']['symbols']={'_callee':{'rva':1}}
        with self.assertRaisesRegex(ValueError,'reference bindings'):
            reprove.proposed_state(state,'src/unit.c','before','after',{'evidence/old.json':('evidence/new.json',document)}, {})
    def test_stale_baseline_rejected(self):
        state,_=self.proposal()
        with self.assertRaisesRegex(ValueError,'stale baseline'):
            reprove.proposed_state(state,'src/unit.c','wrong','after',{}, {})
    def test_raw_category_not_rewritten(self):
        state,_=self.proposal();state['functions'][0]['status']='FUNCTION_MATCH'
        with self.assertRaisesRegex(ValueError,'relocated source peers'):
            reprove.proposed_state(state,'src/unit.c','before','after',{}, {})


class ReproveTransactionTests(unittest.TestCase):
    def exercise(self, failure=None):
        with workspace_temporary_directory('reprove-') as temporary:
            root=Path(temporary).resolve()
            for folder in ('src','candidates','evidence','work'):(root/folder).mkdir()
            source=root/'src/unit.c';source.write_bytes(b'before')
            candidate=root/'candidates/unit.c';candidate.write_bytes(b'after')
            state,document=ReproveTests().proposal()
            state['functions'][0]['flags']=['/O2']
            state['functions'][0]['proof']['source_sha256']=reprove.digest(b'before')
            document['evidence_files']={'src/unit.c':reprove.digest(b'before')}
            (root/'evidence/old.json').write_text(json.dumps(document))
            state_path=root/'recovery.json';state_path.write_text(json.dumps(state));original=state_path.read_bytes()
            review=dict(schema=1,reason='Synthetic reviewed consumer correction',source='src/unit.c',
                        before_sha256=reprove.digest(b'before'),after_sha256=reprove.digest(b'after'),changed_symbols=['_consumer'])
            review_path=root/'evidence/review.json';review_path.write_text(json.dumps(review))
            baseline=dict(id='test',exact=True,object='baseline.obj',compiled_sha256='same-code',target_sha256='same-target',proof_category=reprove.RELOCATED)
            after=dict(baseline,object='after.obj')
            if failure=='gate':after['exact']=False
            if failure=='bytes':after['compiled_sha256']='different-code'
            with ExitStack() as stack:
                for module in (reprove,promote):
                    stack.enter_context(patch.object(module,'ROOT',root))
                    stack.enter_context(patch.object(module,'STATE',state_path))
                    stack.enter_context(patch.object(module,'JOURNAL',root/'work/promotion-journal.json'))
                verify=stack.enter_context(patch.object(reprove,'verify_rows',side_effect=[([baseline],{'compiler_calls':1}),([after],{'compiler_calls':1})]))
                stack.enter_context(patch.object(reprove,'compile_inputs',return_value={source:reprove.digest(b'after')}))
                stack.enter_context(patch.object(reprove,'changed_functions',return_value=['_consumer']))
                stack.enter_context(patch.object(reprove,'assert_verified_inputs'))
                if failure:
                    with self.assertRaisesRegex(ValueError,'strict proof failed|byte identity changed'):
                        reprove.refresh(review_path,candidate,root/'work/report.json')
                    self.assertEqual(source.read_bytes(),b'before');self.assertEqual(state_path.read_bytes(),original)
                else:
                    receipt=reprove.refresh(review_path,candidate,root/'work/report.json')
                    self.assertEqual(source.read_bytes(),b'after')
                    refreshed=json.loads(state_path.read_text())
                    self.assertEqual(len(refreshed['functions']),1)
                    self.assertEqual(refreshed['functions'][0]['status'],reprove.RELOCATED)
                    self.assertEqual(refreshed['functions'][0]['proof']['compiled_sha256'],'old-exact-code')
                    self.assertEqual(receipt['new_acceptances'],0)
                self.assertEqual(verify.call_count,2)
                self.assertFalse((root/'work/promotion-journal.json').exists())
                self.assertEqual(json.loads((root/'evidence/old.json').read_text()),document)
    def test_gate_failure_restores_source_and_state(self):self.exercise('gate')
    def test_even_exact_new_bytes_must_equal_previous_compiler_bytes(self):self.exercise('bytes')
    def test_fresh_baseline_and_final_proofs_publish_context_only(self):self.exercise()


if __name__=='__main__':unittest.main()
