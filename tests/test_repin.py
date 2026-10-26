import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import repin
from original_evidence import original_evidence_path, inventory_with_original_paths
from reviewed_evidence import snapshot_path


class ImpactTests(unittest.TestCase):
    def test_pins_and_consumers_are_not_hidden(self):
        state={'functions':[{'id':'a','status':'FUNCTION_MATCH_RELOCATED','source':'s.c','proof':{'bindings_path':'e/a.json'}},
                            {'id':'b','status':'LINKED_MATCH','source':'s.c','prior_raw_proof':{'scope':'raw'}}]}
        docs={'e/a.json':{'evidence_files':{'s.c':'123'}},'e/unused.json':{'evidence_files':{'s.c':'456'}}}
        result=repin.impact(state,docs,{'s.c'})
        self.assertEqual(result['accepted_source_peers'],['a','b'])
        self.assertEqual(result['direct_pins'][0]['accepted_consumers'],['a'])
        self.assertEqual(result['direct_pins'][1]['accepted_consumers'],[])
        self.assertFalse(result['publication_allowed'])

    def test_unrelated_pin_is_not_reported(self):
        self.assertEqual(repin.impact({'functions':[]},{'e/x':{'evidence_files':{'other':'a'}}},{'x'})['direct_pins'],[])


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'work')
        self.root=Path(self.tmp.name).resolve()
        (self.root/'src').mkdir();(self.root/'evidence/repin_snapshots').mkdir(parents=True)
        (self.root/'toolchains').mkdir();(self.root/'toolchains/manifest.json').write_text('{}')
        self.name='src/unit.c';self.old=b'int g;\n';self.new=b'int g; /* changed */\n'
        self.hash=match.digest(self.old)
        self.snap='evidence/repin_snapshots/'+self.hash+'.txt'
        (self.root/self.name).write_bytes(self.new);(self.root/self.snap).write_bytes(self.old)
        self.doc={'schema':1,'evidence_files':{self.name:self.hash},'functions':{'x':{'symbols':{}}}}
        (self.root/'evidence/b.json').write_text(json.dumps(self.doc))
        self.row={'id':'x','proof':{'reference_snapshots':{self.name:self.snap}}}
        self.scope=patch.object(match,'ROOT',self.root);self.scope.start()

    def tearDown(self):
        self.scope.stop();self.tmp.cleanup()

    def test_old_default_still_rejects_changed_provenance(self):
        with self.assertRaisesRegex(ValueError,'evidence changed'):
            match.ReferenceSnapshot().read('evidence/b.json',{'id':'x','proof':{}})

    def test_snapshot_preserves_digest_and_observes_real_snapshot_only(self):
        binding,context,observed=match.ReferenceSnapshot().read('evidence/b.json',self.row)
        self.assertEqual(context['files'],self.doc['evidence_files'])
        self.assertIn(self.root/self.snap,observed)
        self.assertNotIn(self.root/self.name,observed)
        self.assertEqual(binding,{'symbols':{}})

    def test_wrong_address_rejected(self):
        with self.assertRaisesRegex(ValueError,'content address'):
            snapshot_path(self.root,self.name,self.hash,{self.name:'evidence/other.txt'})

    def test_corrupt_snapshot_rejected(self):
        (self.root/self.snap).write_bytes(self.new)
        with self.assertRaisesRegex(ValueError,'evidence changed'):
            match.ReferenceSnapshot().read('evidence/b.json',self.row)

    def test_mutation_after_read_rejected(self):
        snapshot=match.ReferenceSnapshot();snapshot.read('evidence/b.json',self.row)
        (self.root/self.snap).write_bytes(self.new)
        with self.assertRaisesRegex(ValueError,'changed during verification'):
            snapshot.assert_unchanged()

    def test_scripts_and_state_cannot_be_historical_inputs(self):
        for name in ('scripts/match.py','recovery.json','../src/x'):
            with self.assertRaises(ValueError):
                snapshot_path(self.root,name,self.hash,{name:self.snap})

    def test_snapshot_cannot_satisfy_live_owner_context(self):
        symbol='_g'
        self.doc['functions']['x']['symbols'][symbol]={'kind':'bss','rva':32,'size':4,
          'owner':{'source':self.name,'flags':['/O2'],'symbol':symbol},'identity':{}}
        (self.root/'evidence/b.json').write_text(json.dumps(self.doc))
        (self.root/'target.bin').write_bytes(b'oracle')
        self.row.update(status=match.RELOCATED,source=self.name,target='target.bin',target_sha256=match.digest(b'oracle'),flags=['/O2'],rva='0x100',size=16)
        proof=self.row['proof'];proof.update(source_sha256=match.digest(self.new),bindings_path='evidence/b.json')
        b,e,_=match.ReferenceSnapshot().read('evidence/b.json',self.row)
        proof.update(bindings_sha256=match.reference_digest(b),reference_evidence_sha256=match.reference_digest(e),context_sha256={self.name:match.digest(self.new)})
        with patch.object(match,'verify_toolchain',return_value='fixture'),patch.object(match,'compile_source') as compile_mock:
            with self.assertRaisesRegex(ValueError,'owner source/include context must be independently pinned'):
                match.verify_rows([self.row])
            compile_mock.assert_not_called()

    def test_relocation_and_reference_changes_fail_preservation(self):
        for key in ('relocations','references','data_checks','source_alias_checks','indirect_call_checks'):
            with self.assertRaises(ValueError):
                repin.require_equal(repin.invariant({key:[1]}),repin.invariant({key:[2]}),'fixture')

    def test_journal_failure_restores_exact_source_and_state(self):
        source=self.root/'src/unit.c';state=self.root/'recovery.json';state.write_bytes(b'old state')
        journal=self.root/'journal.json'
        def fail():raise RuntimeError('comparison failed')
        with patch.object(repin,'ROOT',self.root):
            with self.assertRaises(RuntimeError):
                repin.transaction({source:b'candidate',state:b'new state'},journal,fail)
        self.assertEqual(source.read_bytes(),self.new);self.assertEqual(state.read_bytes(),b'old state')
        self.assertFalse(journal.exists())

    def test_original_checkout_source_is_rebased_to_local_source(self):
        primary=self.root/'primary'
        path,external=original_evidence_path(self.root,str(primary/'src/x.c'),primary)
        self.assertEqual(path,self.root/'src/x.c');self.assertFalse(external)
        with self.assertRaises(ValueError):original_evidence_path(self.root,'assets/../escape',primary)

    def test_journal_recovers_intermediate_and_refuses_concurrent_edits(self):
        import base64
        state=self.root/'recovery.json';journal=self.root/'journal.json'
        record=dict(path='recovery.json',before=base64.b64encode(b'old').decode(),
                    before_sha256=repin.sha(b'old'),intermediate_sha256=repin.sha(b'staged'),
                    after_sha256=repin.sha(b'final'))
        journal.write_text(json.dumps([record]));state.write_bytes(b'concurrent')
        with patch.object(repin,'ROOT',self.root):
            with self.assertRaisesRegex(ValueError,'Concurrent edit'):
                repin.recover_transaction(journal)
            state.write_bytes(b'staged');repin.recover_transaction(journal)
        self.assertEqual(state.read_bytes(),b'old');self.assertFalse(journal.exists())

    def test_inventory_rebase_preserves_sections_and_other_aux(self):
        source=self.root/'src/x.c';raw=str(source).encode()+b'\0';raw=raw.ljust((len(raw)+17)//18*18,b'\0')
        class Coff:pass
        c=Coff();c.sections=[{'data':b'code','relocations':[{'offset':1}]}]
        c.symbols={0:{'storage':103,'aux':[raw[i:i+18] for i in range(0,len(raw),18)]},4:{'storage':2,'aux':[b'other']}}
        before=copy.deepcopy(c.symbols)
        result=inventory_with_original_paths(c,self.root,self.root.with_name(self.root.name[:-1]+'X'))
        self.assertEqual(result['sections'],c.sections);self.assertEqual(result['symbols'][4],c.symbols[4])
        self.assertEqual(c.symbols,before)


class SecondRepinTests(unittest.TestCase):
    def test_before_check_keeps_redirects_recorded_by_an_earlier_repin(self):
        earlier = {'src/eng3/a.cpp': 'evidence/repin_snapshots/' + 'a' * 64 + '.txt'}
        added = {'evidence/b.json': 'evidence/repin_snapshots/' + 'b' * 64 + '.txt'}
        row = dict(id='m.r', status='FUNCTION_MATCH_RELOCATED', symbol='_f',
                   proof=dict(reference_snapshots=dict(earlier), compiled_sha256='c'))
        result = dict(id='m.r', exact=True, compiled_sha256='c', object='obj')
        seen = {}
        def capture(rows, results, original_root):
            seen.update(rows[0]['proof']['reference_snapshots'])
            raise StopIteration
        with patch('promote.assert_verified_inputs', capture), \
                patch.object(repin, 'accepted_rows', lambda rows: rows):
            with self.assertRaises(StopIteration):
                repin.frozen_before(dict(functions=[row]), [result], None, added)
        self.assertEqual(seen, {**earlier, **added})
        self.assertEqual(row['proof']['reference_snapshots'], earlier)  # the ledger copy is untouched


if __name__=='__main__':unittest.main()
