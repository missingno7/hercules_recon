"""Focused controls for the unreviewed proposal. These grant no acceptance."""
import copy,hashlib,importlib.util,json,pathlib,sys,tempfile,unittest
from unittest.mock import patch
LANE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(LANE.parent/'scripts'))
import repin as proposal
sha=lambda b:hashlib.sha256(b).hexdigest()

class ProfileProposalTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.root=pathlib.Path(self.temp.name)
  self.addCleanup(self.temp.cleanup)
  self.source='src/title/u02b70.c';p=self.root/self.source;p.parent.mkdir(parents=True);p.write_bytes(b'int x;\n')
  self.pin=sha(p.read_bytes());self.target='assets/pc/HERCULES/ENG3.DLL'
  def row(id,target,flags,status):return dict(id=id,source=self.source,target=target,target_sha256='a'*64,flags=flags,status=status,proof=dict(source_sha256=self.pin))
  self.state=dict(functions=[row('eng3.a',self.target,['/O2'],'FUNCTION_MATCH_RELOCATED'),row('eng3.b',self.target,['/O2'],'FUNCTION_MATCH'),row('eng3.pending',self.target,['/O2'],'CODEGEN_SIMILAR'),row('title.a','assets/pc/HERCULES/TITLE.DLL',['/O2'],'FUNCTION_MATCH_RELOCATED')])
  self.item=dict(source=self.source,source_sha256=self.pin,target=self.target,target_sha256='a'*64,before=['/O2'],after=['/O2','/DHERCULES_ENGINE3'],rows=['eng3.a','eng3.b'])
  self.review=dict(schema=1,profile='reviewed-append-define-v1',reason='Focused test only',revisions=[self.item])
  self.active=patch.object(proposal,'ROOT',self.root);self.active.start();self.addCleanup(self.active.stop)
 def run_review(self):
  path=self.root/'evidence/profile.json';path.parent.mkdir(exist_ok=True);raw=json.dumps(self.review).encode();path.write_bytes(raw)
  return proposal.compiler_profile_revisions(self.state,dict(path='evidence/profile.json',sha256=sha(raw)),set())
 def test_exact_accepted_peer_inventory_only_and_no_mutation(self):
  before=copy.deepcopy(self.state);changes,review=self.run_review()
  self.assertEqual(set(changes),{'eng3.a','eng3.b'});self.assertEqual(self.state,before)
  self.assertEqual(changes['eng3.a']['after'],['/O2','/DHERCULES_ENGINE3'])
 def test_missing_or_duplicate_or_pending_or_other_module_rows_rejected(self):
  for rows in [['eng3.a'],['eng3.a','eng3.b','eng3.b'],['eng3.a','eng3.b','eng3.pending'],['eng3.a','eng3.b','title.a']]:
   with self.subTest(rows=rows):
    self.item['rows']=rows
    with self.assertRaises(ValueError):self.run_review()
 def test_flag_reordering_or_compiler_option_injection_rejected(self):
  for flags in [['/DHERCULES_ENGINE3','/O2'],['/O1','/DHERCULES_ENGINE3'],['/O2','/Foelsewhere'],['/O2','/DNAME=1'],['/O2','/DNAME','/DSECOND']]:
   with self.subTest(flags=flags):
    self.item['after']=flags
    with self.assertRaises(ValueError):self.run_review()
 def test_source_target_before_flags_and_proof_mismatch_rejected(self):
  for field,value in [('source_sha256','b'*64),('target_sha256','b'*64),('before',['/Od'])]:
   with self.subTest(field=field):
    saved=self.item[field];self.item[field]=value
    with self.assertRaises(ValueError):self.run_review()
    self.item[field]=saved
  self.state['functions'][0]['proof']['source_sha256']='b'*64
  with self.assertRaises(ValueError):self.run_review()
 def test_linked_retained_and_unit_inventory_proofs_refused(self):
  row=self.state['functions'][0]
  for field,value in [('status','LINKED_MATCH'),('prior_raw_proof',{}),('proof',dict(source_sha256=self.pin,unit_inventory_sha256='a'*64))]:
   with self.subTest(field=field):
    saved=copy.deepcopy(row);row[field]=value if field!='prior_raw_proof' else {'source_sha256':self.pin}
    with self.assertRaises(ValueError):self.run_review()
    row.clear();row.update(saved)
 def test_missing_review_preserves_legacy_path(self):
  self.assertEqual(proposal.compiler_profile_revisions(self.state,None,set()),({},None))
 def test_same_source_replacement_refused(self):
  self.run_review();raw=(self.root/'evidence/profile.json').read_bytes()
  with self.assertRaises(ValueError):proposal.compiler_profile_revisions(self.state,dict(path='evidence/profile.json',sha256=sha(raw)),{self.source})
 def test_review_hash_and_unknown_fields_rejected(self):
  self.review['unexpected']=1
  with self.assertRaises(ValueError):self.run_review()
  with self.assertRaises(ValueError):proposal.compiler_profile_revisions(self.state,dict(path='evidence/profile.json',sha256='b'*64),set())

if __name__=='__main__':unittest.main(verbosity=2)
