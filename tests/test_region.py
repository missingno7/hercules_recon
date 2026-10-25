"""Context is disposable; stale recipes and known failed families must fail closed."""
from copy import deepcopy
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
import tempfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from region import contribution_identity,make_patch,apply_patch_text,check_reentry,frontier,context_identity,packet


class RegionTests(unittest.TestCase):
    def test_header_change_invalidates_context_without_source_change(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp);(root/'toolchains').mkdir();(root/'toolchains/manifest.json').write_text('{}')
            header=root/'view.h';header.write_text('typedef int T;')
            spec=dict(target_sha256='oracle',functions=[],flags=['/O2'],context_files=['view.h'])
            before=context_identity(spec,'same source')
            header.write_text('typedef int T; typedef void (*Callback)(void);')
            self.assertNotEqual(before,context_identity(spec,'same source'))
            with self.assertRaises(ValueError):context_identity(dict(spec,context_files=['../outside.h']),'same source')
            with self.assertRaises(FileNotFoundError):context_identity(dict(spec,context_files=['missing.h']),'same source')

    def test_output_identity_preserves_bytes_and_relocations(self):
        base=dict(data=b'\x90\xc3',size=2,relocations=[dict(offset=0,type=6,symbol='_real')])
        same=dict(base,path='other.obj',timestamp=123)
        self.assertEqual(contribution_identity(base),contribution_identity(same))
        for field,value in [('data',b'\xc3\x90'),('size',3),
                            ('relocations',[dict(offset=0,type=6,symbol='_wrong')])]:
            changed=dict(base);changed[field]=value
            self.assertNotEqual(contribution_identity(base),contribution_identity(changed))

    def test_recipe_roundtrip_and_stale_baseline(self):
        base='line one\nline two\nline three\n'
        for after in ('',base,'insert\n'+base,base.replace('two','second'),'last without newline'):
            patch=make_patch(base,after)
            self.assertEqual(apply_patch_text(base,patch),after)
            with self.assertRaises(ValueError):apply_patch_text(base+'changed',patch)

    def test_failed_family_needs_a_real_reentry(self):
        meta=dict(region='r',lane='worker',family='order',prediction='equal',falsifier='different',
                  capability='bounded matching',model='gpt-6-luna',effort='xhigh')
        old=dict(region='r',family='order',context_id='ctx',verdict='exhausted',reentry_condition='new evidence')
        with self.assertRaises(ValueError):check_reentry(meta,[old],'ctx')
        check_reentry(dict(meta,new_discriminator='new offset observation'),[old],'ctx')
        check_reentry(meta,[old],'changed context')
        with self.assertRaises(ValueError):check_reentry(dict(meta,effort='low'),[],'ctx')

    def test_frontier_excludes_duplicates_regression_and_dominated(self):
        good=dict(output_id='a',functions=[dict(strict_equal=True,normalized_different_bytes=0,target_size=4,ordered_cfg_equal=True)])
        worse=dict(output_id='b',functions=[dict(strict_equal=False,normalized_different_bytes=2,target_size=4,ordered_cfg_equal=True)])
        unsafe=dict(good,output_id='c',regressions=['accepted_peer'])
        self.assertEqual(frontier([good,deepcopy(good),worse,unsafe]),[good])

    def test_packet_overlays_current_canonical_closure(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write('toolchains/manifest.json',{})
            spec=dict(region=dict(id='r',ownership='inferred_region'),target_sha256='oracle',flags=['/O2'],
                      functions=[dict(symbol='_f',rva='0x1000',size=4,target_sha256='span')])
            (root/'src').mkdir();(root/'src/base.c').write_text('source')
            write('evidence/spec.json',spec);write('evidence/receipt.json',dict(context_id=context_identity(spec,'source'),receipt_id='one',output_id='one',
                source_sha256='source',summary={},functions=[dict(symbol='_f',strict_equal=True,target_size=4,normalized_different_bytes=0)]))
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',baseline='src/base.c',receipts=['evidence/receipt.json'])]))
            write('recovery.json',dict(functions=[dict(id='accepted',status='FUNCTION_MATCH',target_sha256='oracle',rva='0x1000')]))
            with redirect_stdout(io.StringIO()):p=packet('r')
            result=json.loads(p.read_text())
            self.assertEqual(result['unresolved'],[])
            self.assertEqual(result['frontier'],[])
            self.assertEqual(result['canonical_in_region'],1)

    def test_packet_reports_non_strict_source_without_closing_strict_frontier(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write('toolchains/manifest.json',{})
            spec=dict(region=dict(id='r',ownership='inferred_region'),target_sha256='oracle',flags=['/O2'],
                      functions=[dict(symbol='_f',rva='0x1000',size=16),dict(symbol='_g',rva='0x1010',size=16)])
            write('evidence/spec.json',spec);(root/'src').mkdir();(root/'src/f.c').write_text('source')
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',baseline='src/f.c')]))
            write('recovery.json',dict(functions=[dict(id='known',status='CODEGEN_SIMILAR',target_sha256='oracle',
                rva='0x1000',size=16,source='src/f.c',proof=dict(scope='masked diagnostic'))]))
            with redirect_stdout(io.StringIO()):p=packet('r')
            result=json.loads(p.read_text())
            self.assertEqual(result['unresolved'],['_f','_g'])
            self.assertEqual(result['canonical_in_region'],0)
            self.assertEqual(result['unrecorded_functions'],['_g'])
            self.assertEqual(result['recorded_non_strict_functions'],[dict(region_symbol='_f',recovery_id='known',
                status='CODEGEN_SIMILAR',source='src/f.c',bytes=16,extent_agrees=True,proof_scope='masked diagnostic')])

    def test_packet_resolves_reviewed_retired_baseline_and_context_files(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write_text(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(value)
            def write_json(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write_json('toolchains/manifest.json',{})
            write_text('calibration/old.c','historical source')
            write_text('calibration/view.h','historical header')
            write_text('src/current.c','current complete TU')
            write_text('src/view.h','current header')
            spec=dict(source='calibration/old.c',region=dict(id='r',ownership='inferred_region'),
                target_sha256='oracle',flags=['/O2'],context_files=['calibration/view.h'],
                functions=[dict(symbol='_f',rva='0x1000',size=4)])
            old_context=context_identity(spec,'historical source')
            write_json('evidence/spec.json',spec)
            write_json('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',
                baseline='calibration/old.c',receipts=['evidence/old_receipt.json'])]))
            write_json('evidence/source_layout_2026_10_25/review.json',dict(moves={
                'calibration/old.c':'src/current.c','calibration/view.h':'src/view.h'}))
            write_json('evidence/old_receipt.json',dict(receipt_id='old',context_id=old_context,
                functions=[],output_id='old-output'))
            write_json('recovery.json',dict(functions=[dict(id='f',symbol='_f',status='CODEGEN_SIMILAR',
                target_sha256='oracle',rva='0x1000',size=4,source='src/current.c')]))
            with redirect_stdout(io.StringIO()):packet_path=packet('r')
            result=json.loads(packet_path.read_text())
            self.assertEqual(result['baseline_source'],'src/current.c')
            self.assertEqual(result['baseline_resolution']['resolution_basis'],'reviewed source-layout move')
            self.assertEqual(result['baseline_resolution']['context_files'],{'calibration/view.h':'src/view.h'})
            self.assertNotEqual(result['context_id'],old_context)
            self.assertEqual(result['stale_receipts'],['old'])
            self.assertEqual(result['frontier'],[])
            self.assertEqual(result['tried'],[])

    def test_packet_uses_unambiguous_current_baseline_context(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write('toolchains/manifest.json',{})
            (root/'src').mkdir();(root/'src/current.c').write_text('current TU')
            spec=dict(source='src/current.c',region=dict(id='r',ownership='inferred_region'),
                target_sha256='oracle',flags=['/O2'],functions=[dict(symbol='_f',rva='0x1000',size=4)])
            write('evidence/spec.json',spec)
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',baseline='src/current.c')]))
            write('recovery.json',dict(functions=[dict(id='f',symbol='_f',status='CODEGEN_SIMILAR',
                target_sha256='oracle',rva='0x1000',size=4,source='src/current.c')]))
            with redirect_stdout(io.StringIO()):packet_path=packet('r')
            result=json.loads(packet_path.read_text())
            self.assertEqual(result['baseline_source'],'src/current.c')
            self.assertEqual(result['baseline_resolution']['status'],'resolved')
            self.assertEqual(result['context_id'],context_identity(spec,'current TU'))

    def test_packet_rejects_retired_baseline_without_owner(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write('toolchains/manifest.json',{})
            spec=dict(region=dict(id='r',ownership='inferred_region'),target_sha256='oracle',flags=['/O2'],
                functions=[dict(symbol='_f',rva='0x1000',size=4)])
            write('evidence/spec.json',spec)
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',baseline='calibration/old.c')]))
            write('recovery.json',dict(functions=[]))
            with self.assertRaisesRegex(ValueError,'no reviewed move or complete unambiguous current ledger ownership'):
                packet('r')

    def test_packet_rejects_ambiguous_ledger_owner(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write('toolchains/manifest.json',{})
            spec=dict(region=dict(id='r',ownership='inferred_region'),target_sha256='oracle',flags=['/O2'],
                functions=[dict(symbol='_f',rva='0x1000',size=4)])
            write('evidence/spec.json',spec)
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',baseline='calibration/old.c')]))
            write('recovery.json',dict(functions=[
                dict(id='f1',symbol='_f',status='CODEGEN_SIMILAR',target_sha256='oracle',rva='0x1000',size=4,source='src/a.c'),
                dict(id='f2',symbol='_f_alias',status='CODEGEN_SIMILAR',target_sha256='oracle',rva='0x1000',size=4,source='src/b.c')]))
            with self.assertRaisesRegex(ValueError,'Ambiguous recovery source ownership'):
                packet('r')

    def test_packet_reports_split_current_units_without_reusing_receipts(self):
        scratch=Path(__file__).resolve().parents[1]/'work'
        with tempfile.TemporaryDirectory(dir=scratch) as temp,patch('region.ROOT',Path(temp)):
            root=Path(temp)
            def write(name,value):
                p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value))
            write('toolchains/manifest.json',{})
            (root/'src').mkdir();(root/'src/a.c').write_text('unit a');(root/'src/b.c').write_text('unit b')
            spec=dict(source='calibration/a.c',region=dict(id='r',ownership='inferred_region'),
                target_sha256='oracle',flags=['/O2'],functions=[
                    dict(symbol='_f',rva='0x1000',size=4),dict(symbol='_g',rva='0x1010',size=4)])
            write('evidence/spec.json',spec)
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',
                baseline='calibration/a.c',receipts=['evidence/old_receipt.json'])]))
            write('evidence/source_layout_2026_10_25/review.json',dict(moves={'calibration/a.c':'src/a.c'}))
            write('evidence/old_receipt.json',dict(receipt_id='old',context_id='old-context',functions=[],output_id='old-output'))
            write('recovery.json',dict(functions=[
                dict(id='f',symbol='_f',status='CODEGEN_SIMILAR',target_sha256='oracle',rva='0x1000',size=4,source='src/a.c'),
                dict(id='g',symbol='_g',status='CODEGEN_SIMILAR',target_sha256='oracle',rva='0x1010',size=4,source='src/b.c')]))
            with redirect_stdout(io.StringIO()):packet_path=packet('r')
            result=json.loads(packet_path.read_text())
            self.assertIsNone(result['context_id'])
            self.assertIsNone(result['baseline_source'])
            self.assertEqual(result['baseline_resolution']['status'],'split_current_ownership')
            self.assertEqual([s['path'] for s in result['baseline_resolution']['baseline_sources']],['src/a.c','src/b.c'])
            self.assertEqual(result['stale_receipts'],['old'])
            self.assertEqual(result['frontier'],[])
            self.assertEqual(result['tried'],[])


if __name__=='__main__':unittest.main()
