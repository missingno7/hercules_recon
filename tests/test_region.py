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
            write('evidence/spec.json',spec);(root/'base.c').write_text('source')
            write('evidence/receipt.json',dict(context_id=context_identity(spec,'source'),receipt_id='one',output_id='one',
                source_sha256='source',summary={},functions=[dict(symbol='_f',strict_equal=True,target_size=4,normalized_different_bytes=0)]))
            write('evidence/region_index.json',dict(regions=[dict(id='r',spec='evidence/spec.json',baseline='base.c',receipts=['evidence/receipt.json'])]))
            write('recovery.json',dict(functions=[dict(id='accepted',status='FUNCTION_MATCH',target_sha256='oracle',rva='0x1000')]))
            with redirect_stdout(io.StringIO()):p=packet('r')
            result=json.loads(p.read_text())
            self.assertEqual(result['unresolved'],[])
            self.assertEqual(result['frontier'],[])
            self.assertEqual(result['canonical_in_region'],1)


if __name__=='__main__':unittest.main()
