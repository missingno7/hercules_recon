"""Anchor reproof changes no destination, source context, status or old evidence."""
import copy,json
from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from reanchor import derive
from match import digest,reference_digest,RELOCATED


class ReanchorTests(unittest.TestCase):
    def fixture(self):
        old=dict(rva=100,size=5,operand=0,sha256='a');new=dict(rva=200,size=6,operand=1,sha256='b')
        record=dict(kind='bss',rva=900,size=4,owner=dict(source='src/owner.c',flags=['/O2'],symbol='_cell'),identity=old,references=[dict(offset=1,type=6,addend=0)],evidence=['evidence/origin.json'])
        doc=dict(schema=1,scope='old',evidence_files={'src/owner.c':'owner'},functions={'test':dict(symbols={'_cell':record}),'peer':dict(symbols={})})
        raw=json.dumps(doc).encode();proof=dict(bindings_path='evidence/old.json',bindings_sha256=reference_digest(doc['functions']['test']),compiled_sha256='code',target_sha256='native',source_sha256='source',context_sha256={'src/a.c':'source'})
        state=dict(functions=[dict(id='test',status=RELOCATED,source='src/a.c',flags=['/O2'],rva='0x100',size=16,proof=proof),dict(id='peer',status=RELOCATED,source='src/b.c',proof={**proof,'bindings_sha256':reference_digest(doc['functions']['peer'])})])
        change=dict(bindings_path='evidence/old.json',bindings_sha256=digest(raw),symbol='_cell',function_ids=['test'],before=old,after=new)
        review=dict(schema=1,reason='independent replacement reader',changes=[change])
        return state,review,{'evidence/old.json':(raw,doc)}

    def run_case(self,state,review,docs):return derive(state,review,docs,'evidence/review.json','review-sha')

    def test_only_anchor_and_provenance_change(self):
        state,review,docs=self.fixture();original=copy.deepcopy((state,docs));proposal,manifests,touched,rows=self.run_case(state,review,docs)
        self.assertEqual((state,docs),original);self.assertEqual(touched,[('test','_cell')]);self.assertEqual(rows,['test','peer'])
        path,document=manifests['evidence/old.json'];self.assertEqual(document['functions']['test']['symbols']['_cell']['identity'],review['changes'][0]['after'])
        for old,new in zip(state['functions'],proposal['functions']):
            self.assertEqual({k:v for k,v in old.items() if k!='proof'},{k:v for k,v in new.items() if k not in ('proof','reference_anchor_history')})
            self.assertEqual(new['reference_anchor_history'][0]['proof'],old['proof'])
            for key in ('compiled_sha256','target_sha256','source_sha256','context_sha256'):self.assertEqual(old['proof'][key],new['proof'][key])

    def test_destination_owner_flags_fields_cannot_be_added(self):
        for field in ['rva','owner','source','flags','status','references']:
            state,review,docs=self.fixture();review['changes'][0][field]='changed'
            with self.subTest(field=field),self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_manifest_baseline_must_match(self):
        state,review,docs=self.fixture();review['changes'][0]['bindings_sha256']='stale'
        with self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_exact_complete_accepted_inventory(self):
        for ids in [[],['test','test'],['missing'],['test','peer']]:
            state,review,docs=self.fixture();review['changes'][0]['function_ids']=ids
            with self.subTest(ids=ids),self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_same_or_partial_anchor_rejects(self):
        for after in [dict(rva=200),None,'wrong']:
            state,review,docs=self.fixture();review['changes'][0]['after']=after
            with self.subTest(after=after),self.assertRaises(ValueError):self.run_case(state,review,docs)
        state,review,docs=self.fixture();review['changes'][0]['after']=review['changes'][0]['before']
        with self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_function_binding_cannot_be_reanchored(self):
        state,review,docs=self.fixture();docs['evidence/old.json'][1]['functions']['test']['symbols']['_cell']['kind']='function'
        with self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_unaccepted_rows_cannot_gain_acceptance(self):
        for status in ['CODEGEN_SIMILAR','FUNCTION_MATCH','LINKED_MATCH']:
            state,review,docs=self.fixture();state['functions'][0]['status']=status
            with self.subTest(status=status),self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_overlapping_changes_reject(self):
        state,review,docs=self.fixture();review['changes'].append(copy.deepcopy(review['changes'][0]))
        with self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_empty_reason_and_bad_history_reject(self):
        state,review,docs=self.fixture();review['reason']=''
        with self.assertRaises(ValueError):self.run_case(state,review,docs)
        state,review,docs=self.fixture();state['functions'][0]['reference_anchor_history']='bad'
        with self.assertRaises(ValueError):self.run_case(state,review,docs)


class FileDataReanchorTests(unittest.TestCase):
    def fixture(self, peer=False):
        state,review,docs=ReanchorTests.fixture(self)
        doc=docs['evidence/old.json'][1]
        rec=doc['functions']['test']['symbols']['_cell']
        old=rec.pop('identity')
        rec.update(kind='data',data_profile='file-data-owner-v1',sha256='1'*64,
                   file_data=dict(extent_claim='minimum',anchor=old))
        if peer:
            doc['functions']['peer']['symbols']['_cell']=copy.deepcopy(rec)
            review['changes'][0]['function_ids'].append('peer')
        self.repin(state,review,docs)
        return state,review,docs

    def repin(self,state,review,docs):
        doc=docs['evidence/old.json'][1];raw=json.dumps(doc).encode()
        docs['evidence/old.json']=(raw,doc)
        review['changes'][0]['bindings_sha256']=digest(raw)
        for row in state['functions']:
            row['proof']['bindings_sha256']=reference_digest(doc['functions'][row['id']])

    def run_case(self,*args):return derive(*args,'evidence/review.json','review-sha')

    def test_file_data_only_anchor_and_provenance_change(self):
        state,review,docs=self.fixture();original=copy.deepcopy((state,docs))
        proposal,manifests,touched,rows=self.run_case(state,review,docs)
        self.assertEqual((state,docs),original)
        self.assertEqual(touched,[('test','_cell')]);self.assertEqual(rows,['test','peer'])
        _,doc=manifests['evidence/old.json']
        rec=doc['functions']['test']['symbols']['_cell']
        self.assertEqual(rec['file_data']['anchor'],review['changes'][0]['after'])
        restored=copy.deepcopy(rec);restored['file_data']['anchor']=review['changes'][0]['before']
        self.assertEqual(restored,docs['evidence/old.json'][1]['functions']['test']['symbols']['_cell'])
        for old,new in zip(state['functions'],proposal['functions']):
            self.assertEqual(new['reference_anchor_history'][0]['proof'],old['proof'])
            for key in ('compiled_sha256','target_sha256','source_sha256','context_sha256'):
                self.assertEqual(old['proof'][key],new['proof'][key])
            self.assertEqual(old['status'],new['status'])

    def test_every_accepted_file_data_use_required(self):
        state,review,docs=self.fixture(peer=True)
        self.assertEqual(self.run_case(state,review,docs)[2],[('peer','_cell'),('test','_cell')])
        review['changes'][0]['function_ids']=['test']
        with self.assertRaisesRegex(ValueError,'every accepted use'):self.run_case(state,review,docs)

    def test_distinct_old_anchors_have_separate_inventories(self):
        state,review,docs=self.fixture(peer=True)
        doc=docs['evidence/old.json'][1]
        doc['functions']['peer']['symbols']['_cell']['file_data']['anchor']['rva']=300
        review['changes'][0]['function_ids']=['test'];self.repin(state,review,docs)
        _,manifests,touched,_=self.run_case(state,review,docs)
        self.assertEqual(touched,[('test','_cell')])
        self.assertEqual(manifests['evidence/old.json'][1]['functions']['peer'],doc['functions']['peer'])

    def test_legacy_or_other_data_profile_rejects(self):
        for profile in (None,'relocated-data-v1','unknown'):
            state,review,docs=self.fixture()
            rec=docs['evidence/old.json'][1]['functions']['test']['symbols']['_cell']
            rec.pop('data_profile')
            if profile:rec['data_profile']=profile
            self.repin(state,review,docs)
            with self.subTest(profile=profile),self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_file_data_cannot_refresh_complete_or_other_fields(self):
        for mutation in ('complete','end_bound','owner','uses','payload'):
            state,review,docs=self.fixture()
            rec=docs['evidence/old.json'][1]['functions']['test']['symbols']['_cell']
            if mutation=='complete':rec['file_data']['extent_claim']='complete'
            elif mutation=='end_bound':rec['file_data']['end_bound']={}
            elif mutation=='owner':review['changes'][0]['owner']={'source':'src/else.c'}
            elif mutation=='uses':review['changes'][0]['references']=[]
            else:review['changes'][0]['sha256']='2'*64
            self.repin(state,review,docs)
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_unaccepted_file_data_rows_cannot_gain_acceptance(self):
        for status in ('CODEGEN_SIMILAR','RECONSTRUCTED','FUNCTION_MATCH','LINKED_MATCH'):
            state,review,docs=self.fixture();state['functions'][0]['status']=status
            with self.subTest(status=status),self.assertRaises(ValueError):self.run_case(state,review,docs)

    def test_mixed_bss_and_file_data_refresh(self):
        state,review,docs=self.fixture(peer=True);doc=docs['evidence/old.json'][1]
        rec=doc['functions']['peer']['symbols']['_cell']
        rec['kind']='bss';rec['identity']=rec.pop('file_data')['anchor']
        rec.pop('data_profile');rec.pop('sha256');self.repin(state,review,docs)
        _,manifests,touched,_=self.run_case(state,review,docs)
        self.assertEqual(touched,[('peer','_cell'),('test','_cell')])
        rec=manifests['evidence/old.json'][1]['functions']['peer']['symbols']['_cell']
        self.assertEqual(rec['identity'],review['changes'][0]['after'])


if __name__=='__main__':unittest.main()
