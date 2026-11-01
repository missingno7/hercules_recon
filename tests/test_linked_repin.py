import copy,json,os,struct,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import linked,linked_repin,repin,match
import test_binding_rename as base


class LinkedRepinTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'work')
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name).resolve()
        for name in ('src','toolchains','work','assets'):
            (self.root/name).mkdir()
        (self.root/'toolchains/manifest.json').write_bytes(b'{}')
        (self.root/'assets/one.dll').write_bytes(b'oracle')
        self.toolchain=self.root/'pinned_toolchain'
        (self.toolchain/'include').mkdir(parents=True)
        (self.toolchain/'include/used.h').write_bytes(b'/* pinned fixture header */\n')
        tool_patch=patch.object(linked,'DEFAULT_TOOLCHAIN',self.toolchain);tool_patch.start();self.addCleanup(tool_patch.stop)
        dep_patch=patch.object(linked_repin,'compiler_dependencies',self.dependencies);dep_patch.start();self.addCleanup(dep_patch.stop)
        self.source='src/unit.c';self.other='src/other.c'
        (self.root/self.source).write_bytes(b'int unit;\n')
        (self.root/self.other).write_bytes(b'int other;\n')
        self.obj=base.write_coff(self.root/'work/compiler.obj',[('_unit',b'\xc3'+b'\x90'*15,[])],[])
        self.object_bytes=self.obj.read_bytes()
        self.state={'functions':[]};self.plans={};self.flags=[]
        self.delta=None;self.delta_source=self.source;self.bad_equal=False;self.bad_hash=False
        for mod in (linked,linked_repin):
            p=patch.object(mod,'ROOT',self.root);p.start();self.addCleanup(p.stop)
        for method,fn in [('validate_plan',lambda p: None),('libraries',lambda p: []),
                          ('compile_source',self.compile),('build',self.build)]:
            p=patch.object(linked,method,fn);p.start();self.addCleanup(p.stop)
        self.add_module('ONE.DLL')
        self.plan={'sources':{self.source:{}},'linked_object_unchanged':{self.source:['ONE.DLL']}}

    def add_module(self,module):
        plan=dict(module=module,target='assets/one.dll',target_sha256=linked.digest(b'oracle'),toolchain='fixture',
                  objects=[dict(source=self.source,flags=['/O2','/DPROFILE']),dict(source=self.other,flags=['/O1'])])
        path=linked.plan_path(module);path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(json.dumps(plan))
        sources=linked.source_hashes(linked.snapshot(plan,self.root),self.root)
        for source in (self.source,self.other):
            rid=module+':'+source
            proof=dict(scope=linked.SCOPE,link_plan=path.relative_to(self.root).as_posix(),
                       link_plan_sha256=linked.digest(path.read_bytes()),source_sha256=sources[source],
                       sources_sha256='historical source inventory',toolchain='fixture',libraries={},
                       function_bytes_sha256='native bytes',image_sha256='historical exact image',
                       image_normalized_sha256='historical normalized image')
            self.state['functions'].append(dict(id=rid,status=linked.STATUS,source=source,target='assets/'+module,proof=proof))
        self.plans[module]=plan

    def dependencies(self,source,flags):
        files,_=linked_repin.include_inputs({'objects':[{'source':str(source.relative_to(self.root))}]})
        return set(files),dict(kind='synthetic dependency scan',inputs=list(map(str,files)),flags=flags)

    def compile(self,source,flags):
        self.flags.append((str(source),flags))
        return self.obj,['fixture']

    def build(self,module,out,**kw):
        plan=self.plans[module];out=Path(out);(out/'objects').mkdir(parents=True,exist_ok=True)
        listing=[]
        for i,o in enumerate(plan['objects']):
            name=str(i)+'.obj';data=self.object_bytes
            if self.delta and o['source']==self.delta_source:
                index,value=self.delta;b=bytearray(data);b[index]=value;data=bytes(b)
            (out/'objects'/name).write_bytes(data)
            listing.append(dict(source=o['source'],object=name,flags=o['flags'],object_sha256=linked.digest(data)))
        sources=linked.source_hashes(linked.snapshot(plan,self.root),self.root)
        return dict(link_plan_sha256=linked.digest(linked.plan_path(module).read_bytes()),toolchain='fixture',libraries=[],
                    sources=sources,sources_sha256=linked.jdigest(sources),objects=listing,
                    rows=[dict(id=r['id'],equal=not self.bad_equal,function_bytes_sha256='bad' if self.bad_hash else 'native bytes')
                          for r in linked.module_rows(self.state,module)])

    def prepare(self):
        return linked_repin.prepare(self.state,self.plan,self.root/'work/receipt',self.root)

    def finish(self,gate):
        candidate=b'int unit; /* excluded by the profile in a real build */\n'
        (self.root/self.source).write_bytes(candidate)
        return gate.finish(self.state,{self.root/self.source:candidate},self.root)

    def test_identical_complete_object_accepts_and_repins_entire_module_inventory(self):
        old=copy.deepcopy(self.state);gate=self.prepare();receipt=self.finish(gate)
        self.assertEqual(self.flags,[(str(self.root/self.source),['/O2','/DPROFILE']),(str(self.root/self.other),['/O1'])])
        self.assertEqual([o['source'] for o in receipt[0]['objects']],[self.source,self.other])
        self.assertEqual(receipt[0]['objects'][0]['before_sha256'],receipt[0]['objects'][0]['after_sha256'])
        for a,b in zip(old['functions'],self.state['functions']):
            expected=copy.deepcopy(a['proof'])
            expected['source_sha256']=linked.digest((self.root/b['source']).read_bytes())
            expected['sources_sha256']=receipt[0]['report']['sources_sha256']
            self.assertEqual(b['proof'],expected)
        self.assertEqual(receipt[0]['proofs_before'],{r['id']:r['proof'] for r in old['functions']})

    def test_any_object_difference_including_metadata_and_trailing_bytes_refuses(self):
        gate=self.prepare()
        for index in (3,8,70,len(self.object_bytes)-1):
            self.delta=(index,self.object_bytes[index]^1)
            with self.subTest(index=index),self.assertRaisesRegex(ValueError,'complete object bytes'):
                self.finish(gate)

    def test_timestamp_only_difference_passes_for_each_timestamp_byte(self):
        for index in range(4,8):
            with self.subTest(index=index):
                gate=self.prepare();self.delta=(index,self.object_bytes[index]^0xff)
                receipt=self.finish(gate)[0]
                checked=receipt['objects'][0]
                self.assertNotEqual(checked['before_sha256'],checked['after_sha256'])
                self.assertEqual(checked['before_normalized_sha256'],checked['after_normalized_sha256'])
                self.assertEqual(receipt['identity'],linked_repin.OBJECT_IDENTITY)
                (self.root/self.source).write_bytes(b'int unit;\n')
                for row in self.state['functions']:
                    row['proof']['source_sha256']=linked.digest((self.root/row['source']).read_bytes())

    def test_section_data_relocation_and_symbol_differences_refuse(self):
        self.obj=base.write_coff(self.root/'work/relocations.obj',
            [('_unit',b'\xe8'+b'\0'*4+b'\xc3'+b'\x90'*10,[(1,'_other',20)])],[('_other',0x20)])
        self.object_bytes=self.obj.read_bytes();gate=self.prepare()
        data_ptr=struct.unpack_from('<I',self.object_bytes,40)[0]
        relocation_ptr=struct.unpack_from('<I',self.object_bytes,44)[0]
        symbol_ptr=struct.unpack_from('<I',self.object_bytes,8)[0]
        for label,index in [('section data',data_ptr+2),('relocation',relocation_ptr+4),('symbol',symbol_ptr+8)]:
            self.delta=(index,self.object_bytes[index]^1)
            with self.subTest(part=label),self.assertRaisesRegex(ValueError,'complete object bytes'):
                self.finish(gate)

    def test_normalization_changes_exactly_four_bytes_and_preserves_all_others(self):
        before=self.object_bytes
        with_timestamp=before[:4]+b'\x12\x34\x56\x78'+before[8:]
        normalized=linked_repin.normalized_object(with_timestamp)
        self.assertEqual(normalized,with_timestamp[:4]+b'\0'*4+with_timestamp[8:])
        self.assertEqual(len(normalized),len(with_timestamp))
        for index in range(len(before)):
            if 4<=index<8:continue
            changed=bytearray(before);changed[index]^=1
            # Header bytes indicating another object format must refuse outright.
            try:result=linked_repin.normalized_object(changed)
            except ValueError:continue
            with self.subTest(offset=index):self.assertNotEqual(result,linked_repin.normalized_object(before))
        self.assertNotEqual(linked_repin.normalized_object(before+b'X'),linked_repin.normalized_object(before))

    def test_invalid_or_truncated_object_header_refuses(self):
        for data in (b'',b'X'*19,b'X'*20,b'\0'*20):
            with self.subTest(size=len(data)),self.assertRaises(ValueError):linked_repin.normalized_object(data)

    def test_missing_opt_in_refuses_without_compile(self):
        del self.plan['linked_object_unchanged']
        with self.assertRaisesRegex(ValueError,'opt-in required'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_linked_module_not_named_refuses_even_for_a_non_row_plan_source(self):
        self.add_module('TWO.DLL')
        # TWO's row uses other.c; unit.c still influences its natural link.
        self.state['functions']=[r for r in self.state['functions'] if r['id']!='TWO.DLL:'+self.source]
        with self.assertRaisesRegex(ValueError,'all affected modules'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_both_modules_named_accepts(self):
        self.add_module('TWO.DLL');self.plan['linked_object_unchanged'][self.source].append('TWO.DLL')
        self.assertEqual(len(self.finish(self.prepare())),2)

    def test_stale_source_plan_library_or_scope_pin_refuses(self):
        proof=self.state['functions'][0]['proof']
        for key in ('source_sha256','link_plan_sha256','libraries','toolchain','scope'):
            old=proof[key];proof[key]='wrong'
            with self.subTest(key=key),self.assertRaises(ValueError):self.prepare()
            proof[key]=old

    def test_literal_header_change_opt_in_checks_all_objects(self):
        (self.root/'src/unit.h').write_bytes(b'/* header */\n')
        (self.root/self.source).write_bytes(b'#include "unit.h"\nint unit;\n')
        self.plan={'sources':{'src/unit.h':{}},'linked_object_unchanged':{'src/unit.h':['ONE.DLL']}}
        for row in self.state['functions']:
            row['proof']['source_sha256']=linked.digest((self.root/row['source']).read_bytes())
        gate=self.prepare();header=self.root/'src/unit.h';header.write_bytes(b'/* changed header */\n')
        receipts=gate.finish(self.state,{header:header.read_bytes()},self.root)
        self.assertEqual(len(receipts[0]['objects']),2)

    def test_changed_frozen_input_and_new_include_dependency_refuse(self):
        gate=self.prepare();(self.root/self.other).write_bytes(b'int changed;\n')
        with self.assertRaisesRegex(ValueError,'frozen input'):self.finish(gate)
        (self.root/self.other).write_bytes(b'int other;\n')
        (self.root/'src/new.h').write_bytes(b'/* new */\n')
        candidate=b'#include "new.h"\nint unit;\n';(self.root/self.source).write_bytes(candidate)
        with self.assertRaisesRegex(ValueError,'repository input file inventory'):
            gate.finish(self.state,{self.root/self.source:candidate},self.root)

    def refresh_source_pins(self):
        for row in self.state['functions']:
            row['proof']['source_sha256']=linked.digest((self.root/row['source']).read_bytes())

    def test_review_macro_include_cannot_omit_module_or_opt_in(self):
        header=self.root/'src/shared.h';header.write_bytes(b'#define VALUE 1\n')
        (self.root/self.other).write_bytes(b'#define H "shared.h"\n#include H\nint other=VALUE;\n')
        self.refresh_source_pins();self.plan={'sources':{'src/shared.h':{}}}
        with self.assertRaisesRegex(ValueError,'opt-in required'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_opaque_include_opt_in_rebuilds_all_objects_and_rejects_other_tu_change(self):
        header=self.root/'src/shared.h';header.write_bytes(b'#define VALUE 1\n')
        (self.root/self.other).write_bytes(b'#define H "shared.h"\n#include H\nint other=VALUE;\n')
        self.refresh_source_pins()
        self.plan={'sources':{'src/shared.h':{}},'linked_object_unchanged':{'src/shared.h':['ONE.DLL']}}
        gate=self.prepare();self.assertEqual(len(self.flags),2)
        header.write_bytes(b'#define VALUE 2\n');self.delta_source=self.other;self.delta=(len(self.object_bytes)-1,self.object_bytes[-1]^1)
        with self.assertRaisesRegex(ValueError,'complete object bytes.*other.c'):
            gate.finish(self.state,{header:header.read_bytes()},self.root)

    def test_opaque_header_opt_in_accepts_only_after_every_object_is_identical(self):
        header=self.root/'src/shared.h';header.write_bytes(b'/* unused profile header */\n')
        (self.root/self.other).write_bytes(b'#define H "shared.h"\n#include H\nint other;\n')
        self.refresh_source_pins()
        self.plan={'sources':{'src/shared.h':{}},'linked_object_unchanged':{'src/shared.h':['ONE.DLL']}}
        gate=self.prepare();header.write_bytes(b'/* changed but identical compiler output */\n')
        receipts=gate.finish(self.state,{header:header.read_bytes()},self.root)
        self.assertEqual([o['source'] for o in receipts[0]['objects']],[self.source,self.other])
        self.assertTrue(receipts[0]['opaque_includes'])

    def test_review_included_c_must_preserve_every_consuming_object(self):
        (self.root/self.other).write_bytes(b'#define OTHER_PROFILE\n#include "unit.c"\n')
        self.refresh_source_pins();gate=self.prepare();self.delta_source=self.other
        self.delta=(len(self.object_bytes)-1,self.object_bytes[-1]^1)
        with self.assertRaisesRegex(ValueError,'complete object bytes.*other.c'):self.finish(gate)
        self.assertEqual(len(gate.modules['ONE.DLL']['objects']),2)

    def test_duplicate_canonical_link_plan_paths_refuse_before_compile(self):
        path=linked.plan_path('ONE.DLL');original=path.read_bytes()
        for alias in ('src/./unit.c','src/nested/../unit.c','SRC/UNIT.C'):
            plan=json.loads(original);plan['objects'].append(dict(source=alias,flags=['/O2','/DOTHER']))
            path.write_text(json.dumps(plan))
            with self.subTest(alias=alias),self.assertRaisesRegex(ValueError,'Duplicate canonical paths'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_duplicate_canonical_named_sources_refuse(self):
        for alias in ('src/./unit.c','src/nested/../unit.c','SRC/UNIT.C'):
            self.plan['sources']={self.source:{},alias:{}}
            with self.subTest(alias=alias),self.assertRaisesRegex(ValueError,'Duplicate canonical paths'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_opaque_forms_fail_closed_for_changes_not_in_literal_inventory(self):
        header=self.root/'src/shared.h';header.write_bytes(b'int value;\n')
        forms=[b'#include H\n',b'#include <shared.h>\n',b'#inc\\\nlude "shared.h"\n',
               b'??=include "shared.h"\n',b'%:include "shared.h"\n',b'#include_next "shared.h"\n']
        self.plan={'sources':{'src/shared.h':{}}}
        for form in forms:
            (self.root/self.other).write_bytes(form)
            with self.subTest(form=form),self.assertRaisesRegex(ValueError,'opt-in required'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_review2_cr_splice_and_other_newline_forms_require_opt_in(self):
        header=self.root/'src/shared.h';header.write_bytes(b'#define VALUE 1\n')
        forms=[b'#inc\\\r\r\nlude "shared.h"\n',b'#inc\\\r\r\r\nlude "shared.h"\n',
               b'#inc\\\rlude "shared.h"\n',b'#inc\\\r\nlude "shared.h"\n',b'#inc\\\nlude "shared.h"\n',
               b'#include "shared.h"\r',b'// ignored comment\r#include "shared.h"\n',
               b'/* a CR anywhere is opaque \r */\nint other;\n',b'#inc??/\nlude "shared.h"\n',
               b'#inc\\\vlude "shared.h"\n',b'#inc\\\flude "shared.h"\n',b'#inc\\\x85lude "shared.h"\n',
               b'#inc\\\xe2\x80\xa8lude "shared.h"\n']
        self.plan={'sources':{'src/shared.h':{}}}
        for form in forms:
            (self.root/self.other).write_bytes(form)
            with self.subTest(form=form),self.assertRaisesRegex(ValueError,'opt-in required'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_review2_cr_in_transitive_literal_header_requires_opt_in(self):
        (self.root/'src/shared.h').write_bytes(b'int value;\n')
        (self.root/'src/outer.h').write_bytes(b'#inc\\\r\r\nlude "shared.h"\n')
        (self.root/self.other).write_bytes(b'#include "outer.h"\n')
        self.plan={'sources':{'src/shared.h':{}}}
        with self.assertRaisesRegex(ValueError,'opt-in required'):self.prepare()

    def test_unparsed_directives_are_opaque_even_without_include_keyword(self):
        forms=[b'#inc "shared.h"\n',b'#INCLUDE "shared.h"\n',b'#unknown anything\n',
               b'\x7f#include "shared.h"\n',b'\xa0#include "shared.h"\n',
               b'#define F(a) a\n',b'#define HEADER "shared.h"\n',b'#if defined(HEADER)\n',
               b'#ifdef NAME extra\n',b'#endif extra\n',b'#pragma include_alias("a.h","shared.h")\n',
               b'#line 1 "shared.h"\n',b'# 1 "shared.h"\n',b'# /* comment\n */ include "shared.h"\n']
        for form in forms:
            with self.subTest(form=form):self.assertTrue(linked_repin.classify_source(form)[1])
        (self.root/'src/shared.h').write_bytes(b'int value;\n')
        self.plan={'sources':{'src/shared.h':{}}}
        for form in forms:
            (self.root/self.other).write_bytes(form)
            with self.subTest(discovery=form),self.assertRaisesRegex(ValueError,
                    'not allowed in a natural link' if form.startswith(b'#pragma include_alias') else 'opt-in required'):self.prepare()

    def test_plain_supported_directives_have_complete_bounded_grammar(self):
        plain=b'#ifndef GUARD\n#define GUARD\n#define NUMBER 0x500UL\n#ifdef OTHER\n#undef OTHER\n#else\n#endif\n#pragma once\n#\n#endif\n'
        self.assertEqual(linked_repin.classify_source(plain),([],[]))
        includes,opaque=linked_repin.classify_source(b'#include "shared.h" /* simple comment */\n')
        self.assertEqual([name for name,form in includes],['shared.h']);self.assertEqual(opaque,[])

    def test_review2_actual_apply_missing_opt_in_does_not_publish_header(self):
        import promote
        header=self.root/'src/shared.h';header.write_bytes(b'#define VALUE 1\n')
        (self.root/self.other).write_bytes(b'#inc\\\r\r\nlude "shared.h"\nint other=VALUE;\n')
        self.refresh_source_pins()
        state_path=self.root/'recovery.json';before=json.dumps(self.state,indent=2).encode();state_path.write_bytes(before)
        candidate=self.root/'work/candidate.h';candidate.write_bytes(b'#define VALUE 2\n')
        plan=dict(state_sha256=linked.digest(before),promotions=[],sources={'src/shared.h':dict(
            candidate='work/candidate.h',before_sha256=linked.digest(header.read_bytes()),candidate_sha256=linked.digest(candidate.read_bytes()))})
        plan_path=self.root/'work/plan.json';plan_path.write_text(json.dumps(plan))
        before_path=self.root/'work/before.json';before_path.write_text(json.dumps(dict(state_sha256=linked.digest(before),results=[])))
        with patch.object(repin,'ROOT',self.root),patch.object(repin,'frozen_before'), \
             patch.object(promote,'lock',base.no_lock),patch.object(promote,'JOURNAL',self.root/'work/promotion-journal.json'):
            with self.assertRaisesRegex(ValueError,'opt-in required'):
                repin.apply(plan_path,before_path,self.root,self.root/'work/apply')
        self.assertEqual(header.read_bytes(),b'#define VALUE 1\n')
        self.assertEqual(state_path.read_bytes(),before)
        self.assertEqual(self.flags,[])
        self.assertFalse((self.root/'work/apply/journal.json').exists())

    def test_comment_separated_literal_include_discovers_header(self):
        (self.root/'src/shared.h').write_bytes(b'int value;\n')
        (self.root/self.other).write_bytes(b'#/**/include/**/"shared.h" // trailing comment\n')
        self.plan={'sources':{'src/shared.h':{}}}
        with self.assertRaisesRegex(ValueError,'opt-in required'):self.prepare()

    def test_opaque_module_must_be_named_even_without_direct_source_overlap(self):
        self.add_module('TWO.DLL')
        # The same opaque TU is in both plans; opt-in must cover both modules.
        (self.root/self.other).write_bytes(b'#include HEADER\n')
        self.plan={'sources':{'src/unit.c':{}},'linked_object_unchanged':{'src/unit.c':['ONE.DLL']}}
        with self.assertRaisesRegex(ValueError,'all affected modules'):self.prepare()

    def test_post_gate_new_header_or_frozen_input_edit_is_detected(self):
        gate=self.prepare();self.finish(gate)
        (self.root/'src/new.h').write_bytes(b'new dependency\n')
        with self.assertRaisesRegex(ValueError,'repository input file inventory'):
            gate.assert_after({self.root/self.source:(self.root/self.source).read_bytes()})
        (self.root/'src/new.h').unlink()
        (self.root/self.other).write_bytes(b'int tampered;\n')
        with self.assertRaisesRegex(ValueError,'frozen input '):
            gate.assert_after({self.root/self.source:(self.root/self.source).read_bytes()})

    def test_review3_discovery_race_is_rejected_even_for_a_skipped_module(self):
        outer=self.root/'src/outer.h';outer.write_bytes(b'#define VALUE 1\n')
        header=self.root/'src/shared.h';header.write_bytes(b'#define VALUE 1\n')
        (self.root/self.source).write_bytes(b'#include "outer.h"\nint unit;\n')
        self.refresh_source_pins();self.plan={'sources':{'src/shared.h':{}}}
        ordinary=linked_repin.include_inputs
        def race(plan):
            result=ordinary(plan);outer.write_bytes(b'#include "shared.h"\n');return result
        with patch.object(linked_repin,'include_inputs',race),self.assertRaisesRegex(ValueError,'frozen input read again'):
            self.prepare()
        self.assertEqual(self.flags,[])

    def test_review3_skipped_module_is_still_frozen_after_scan_and_finish(self):
        outer=self.root/'src/outer.h';outer.write_bytes(b'#define VALUE 1\n')
        header=self.root/'src/shared.h';header.write_bytes(b'#define VALUE 1\n')
        (self.root/self.source).write_bytes(b'#include "outer.h"\nint unit;\n')
        self.refresh_source_pins();self.plan={'sources':{'src/shared.h':{}}}
        gate=self.prepare();self.assertEqual(gate.modules,{})
        self.assertIn(outer,gate.inputs);self.assertEqual(gate.inputs[outer],linked.digest(outer.read_bytes()))
        header.write_bytes(b'#define VALUE 2\n');candidates={header:header.read_bytes()}
        self.assertEqual(gate.finish(self.state,candidates,self.root),[])
        outer.write_bytes(b'#include "shared.h"\n')
        with self.assertRaisesRegex(ValueError,'frozen input'):gate.assert_after(candidates)

    def test_review3_actual_macro_dependency_outside_boundary_refuses(self):
        external=self.root/'work/external.h';external.write_bytes(b'#define VALUE 1\n')
        (self.root/self.source).write_bytes(b'#define H "../work/external.h"\n#include H\nint unit;\n')
        self.refresh_source_pins()
        with patch.object(linked_repin,'compiler_dependencies',return_value=({self.root/self.source,external},{})), \
             self.assertRaisesRegex(ValueError,'outside src/ and pinned toolchain'):self.prepare()
        self.assertEqual(self.flags,[])

    def test_literal_relative_escape_and_absolute_external_include_refuse(self):
        external=self.root/'work/external.h';external.write_bytes(b'int value;\n')
        for name in ('../work/external.h',external.as_posix()):
            (self.root/self.source).write_bytes(('#include "'+name+'"\n').encode())
            self.refresh_source_pins()
            with self.subTest(name=name),self.assertRaisesRegex(ValueError,'outside src/ and pinned toolchain'):self.prepare()

    def test_new_external_dependency_after_candidate_write_refuses(self):
        external=self.root/'work/external.h';external.write_bytes(b'#define VALUE 1\n')
        gate=self.prepare()
        (self.root/self.source).write_bytes(b'#define H "../work/external.h"\n#include H\nint unit;\n')
        candidates={self.root/self.source:(self.root/self.source).read_bytes()}
        with patch.object(linked_repin,'compiler_dependencies',return_value=({self.root/self.source,external},{})), \
             self.assertRaisesRegex(ValueError,'outside src/ and pinned toolchain'):
            gate.finish(self.state,candidates,self.root)

    def test_used_toolchain_header_edit_after_comparison_refuses(self):
        header=self.toolchain/'include/used.h'
        def used(source,flags):return {source,header},{}
        with patch.object(linked_repin,'compiler_dependencies',side_effect=used):
            gate=self.prepare();receipts=self.finish(gate)
        self.assertIn(str(header),receipts[0]['dependencies_before'][0]['compile_inputs'])
        header.write_bytes(b'/* edited after the linked checks */\n')
        with self.assertRaisesRegex(ValueError,'frozen input'):
            gate.assert_after({self.root/self.source:(self.root/self.source).read_bytes()})

    def test_toolchain_inventory_addition_and_discovery_plan_edit_refuse(self):
        gate=self.prepare();self.finish(gate)
        new=self.toolchain/'include/new.h';new.write_bytes(b'new dependency\n')
        with self.assertRaisesRegex(ValueError,'toolchain input file inventory'):
            gate.assert_after({self.root/self.source:(self.root/self.source).read_bytes()})
        new.unlink();linked.plan_path('ONE.DLL').write_bytes(b'changed plan')
        with self.assertRaisesRegex(ValueError,'frozen input'):
            gate.assert_after({self.root/self.source:(self.root/self.source).read_bytes()})

    def test_dependency_reparse_alias_is_refused_before_discovery(self):
        alias=self.root/'src/alias.h';alias.write_bytes(b'/* alias */\n')
        target=self.root/'src/target.h';target.write_bytes(b'/* target */\n')
        real=Path.resolve
        def resolve(path,*a,**kw):return real(target) if path==alias else real(path,*a,**kw)
        with patch.object(Path,'resolve',resolve),self.assertRaisesRegex(ValueError,'Aliased dependency tree path'):
            self.prepare()

    def test_shared_freezer_preserves_logical_paths_and_first_digest(self):
        logical=self.root/'work/logical.json';logical.write_bytes(b'first')
        target=self.root/'work/target.json';target.write_bytes(b'other')
        real=Path.resolve
        def resolve(path,*a,**kw):return real(target) if path==logical else real(path,*a,**kw)
        frozen={}
        with patch.object(Path,'resolve',resolve):
            repin.merge_frozen_inputs(frozen,{logical:linked.digest(b'first')})
        self.assertIn(logical,frozen);self.assertNotIn(target,frozen)
        with self.assertRaisesRegex(ValueError,'frozen input read again'):
            repin.merge_frozen_inputs(frozen,{logical:linked.digest(b'other')})
        logical.write_bytes(b'other')
        with self.assertRaisesRegex(ValueError,'frozen input'):repin.assert_frozen_inputs(frozen)

    def test_fresh_linked_row_or_function_pin_failure_refuses(self):
        gate=self.prepare();self.bad_equal=True
        with self.assertRaisesRegex(ValueError,'fresh linked equality'):self.finish(gate)
        self.bad_equal=False;self.bad_hash=True
        with self.assertRaisesRegex(ValueError,'function proof'):self.finish(gate)


class ApplyTests(unittest.TestCase):
    setUp=base.EndToEndApplyTests.setUp
    tearDown=base.EndToEndApplyTests.tearDown
    before_report=base.EndToEndApplyTests.before_report
    run_apply=base.EndToEndApplyTests.run_apply

    def test_apply_runs_linked_gate_and_receipts_inside_transaction(self):
        class FakeGate:
            guard={}
            inputs={}
            def assert_after(gate,candidates):pass
            def finish(gate,state,candidates,original_root):
                self.assertEqual((self.root/'src/callee.c').read_bytes(),base.Fixture.CALLEE_AFTER)
                return [dict(proofs_before={'linked.fixture':{}})]
        with patch.object(linked_repin,'prepare',return_value=FakeGate()) as prep:
            receipt=self.run_apply(self.fx.plan(self.fx.review()))
        self.assertTrue(prep.called)
        self.assertIn('linked.fixture',receipt['affected'])
        self.assertEqual(len(receipt['linked_object_unchanged']),1)

    def test_apply_linked_difference_rolls_back_exact_source_and_ledger(self):
        before=(self.root/'recovery.json').read_bytes()
        class FakeGate:
            guard={}
            inputs={}
            def assert_after(gate,candidates):pass
            def finish(gate,*args):raise ValueError('complete object bytes fixture')
        with patch.object(linked_repin,'prepare',return_value=FakeGate()),self.assertRaisesRegex(ValueError,'complete object'):
            self.run_apply(self.fx.plan(self.fx.review()))
        self.assertEqual((self.root/'recovery.json').read_bytes(),before)
        self.assertEqual((self.root/'src/callee.c').read_bytes(),base.Fixture.CALLEE_BEFORE)
        self.assertFalse((self.root/'work/repin/journal.json').exists())



class RealGateApplyTests(unittest.TestCase):
    setUp=base.EndToEndApplyTests.setUp
    tearDown=base.EndToEndApplyTests.tearDown
    before_report=base.EndToEndApplyTests.before_report
    run_apply=base.EndToEndApplyTests.run_apply

    def exercise(self,different=False,opt_in=True):
        source='src/linked.c';old=b'void linked(void) {}\n';new=old+b'/* excluded source addition */\n'
        (self.root/source).write_bytes(old)
        (self.root/'work/cand/linked.c').write_bytes(new)
        (self.root/'assets').mkdir();(self.root/'assets/LINKED.DLL').write_bytes(b'oracle')
        plan=dict(module='LINKED.DLL',target='assets/LINKED.DLL',target_sha256=linked.digest(b'oracle'),
                  toolchain='fixture',objects=[dict(source=source,flags=['/O2'])])
        path=self.root/'config/linked/link_plan.json';path.parent.mkdir(parents=True);path.write_text(json.dumps(plan))
        toolchain=self.root/'pinned_toolchain';(toolchain/'include').mkdir(parents=True)
        (toolchain/'include/used.h').write_bytes(b'/* fixture header */\n')
        obj=base.write_coff(self.root/'objects/linked.obj',[('_linked',b'\xc3'+b'\x90'*15,[])],[])
        function_hash=linked.digest(b'\xc3'+b'\x90'*15)
        proof=dict(scope=linked.SCOPE,source_sha256=linked.digest(old),sources_sha256='historical',
                   link_plan='config/linked/link_plan.json',link_plan_sha256=linked.digest(path.read_bytes()),
                   toolchain='fixture',libraries={},function_bytes_sha256=function_hash,
                   image_sha256='old image',image_normalized_sha256='old normalized image')
        state=json.loads((self.root/'recovery.json').read_bytes())
        state['functions'].append(dict(id='linked.a',status=linked.STATUS,target='assets/LINKED.DLL',source=source,
                                       symbol='_linked',flags=['/O2'],proof=proof))
        (self.root/'recovery.json').write_text(json.dumps(state,indent=2)+'\n')
        before=(self.root/'recovery.json').read_bytes();self.publication_baseline=before
        request=dict(state_sha256=linked.digest(before),promotions=[],sources={source:dict(
            candidate='work/cand/linked.c',before_sha256=linked.digest(old),candidate_sha256=linked.digest(new))})
        if opt_in:request['linked_object_unchanged']={source:['LINKED.DLL']}
        def build(module,out,**kw):
            (out/'objects').mkdir(parents=True)
            data=bytearray(obj.read_bytes())
            if different:data[8]^=1
            (out/'objects/00_linked.obj').write_bytes(data)
            sources=linked.source_hashes(linked.snapshot(plan,self.root),self.root)
            return dict(link_plan_sha256=linked.digest(path.read_bytes()),toolchain='fixture',libraries=[],
                        sources=sources,sources_sha256=linked.jdigest(sources),
                        objects=[dict(source=source,flags=['/O2'],object='00_linked.obj')],
                        rows=[dict(id='linked.a',equal=True,function_bytes_sha256=function_hash)])
        with patch.object(linked,'ROOT',self.root),patch.object(linked_repin,'ROOT',self.root), \
             patch.object(linked,'validate_plan'),patch.object(linked,'libraries',return_value=[]), \
             patch.object(linked,'DEFAULT_TOOLCHAIN',toolchain), \
             patch.object(linked_repin,'compiler_dependencies',side_effect=lambda source,flags: ({source.resolve(),toolchain/'include/used.h'},{})), \
             patch.object(linked,'compile_source',return_value=(obj,[])) as compiler,patch.object(linked,'build',build):
            if different or not opt_in:
                with self.assertRaisesRegex(ValueError,'complete object bytes' if different else 'opt-in required'):
                    self.run_apply(request)
                self.assertEqual((self.root/source).read_bytes(),old)
                self.assertEqual((self.root/'recovery.json').read_bytes(),before)
                if not opt_in:compiler.assert_not_called()
            else:
                receipt=self.run_apply(request)
                after=json.loads((self.root/'recovery.json').read_bytes())['functions'][-1]['proof']
                self.assertEqual(after['source_sha256'],linked.digest(new))
                self.assertEqual(after['image_sha256'],proof['image_sha256'])
                self.assertEqual(after['image_normalized_sha256'],proof['image_normalized_sha256'])
                self.assertIn('linked.a',receipt['affected'])
                snapshot='evidence/repin_snapshots/'+linked.digest(old)+'.txt'
                self.assertEqual((self.root/snapshot).read_bytes(),old)
                self.assertEqual(receipt['linked_object_unchanged'][0]['proofs_before']['linked.a'],proof)

    def test_late_toolchain_header_edit_during_raw_verification_rolls_back(self):
        real=self.fx.verify
        def race(rows,**kwargs):
            result=real(rows,**kwargs)
            source=self.root/'src/linked.c'
            if source.is_file() and source.read_bytes().endswith(b'/* excluded source addition */\n'):
                (self.root/'pinned_toolchain/include/used.h').write_bytes(b'/* concurrent edit */\n')
            return result
        with patch.object(match,'verify_rows',side_effect=race),self.assertRaisesRegex(ValueError,'frozen input'):
            self.exercise()
        self.assertEqual((self.root/'recovery.json').read_bytes(),self.publication_baseline)
        self.assertEqual((self.root/'src/linked.c').read_bytes(),b'void linked(void) {}\n')

    def test_last_publication_guard_detects_edit_after_receipt_write(self):
        import promote
        real=promote.atomic_bytes
        def race(path,data):
            result=real(path,data)
            if path.name=='after.json':
                (self.root/'pinned_toolchain/include/used.h').write_bytes(b'/* edit at publication boundary */\n')
            return result
        with patch.object(promote,'atomic_bytes',side_effect=race),self.assertRaisesRegex(ValueError,'frozen input'):
            self.exercise()
        self.assertEqual((self.root/'recovery.json').read_bytes(),self.publication_baseline)
        self.assertEqual((self.root/'src/linked.c').read_bytes(),b'void linked(void) {}\n')

    def test_identical_object_actual_apply_accepts(self):self.exercise()
    def test_differing_object_actual_apply_restores_source_and_ledger(self):self.exercise(different=True)
    def test_missing_opt_in_actual_apply_refuses(self):self.exercise(opt_in=False)


class CompilerDependencyTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'work')
        self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name).resolve()
        self.toolchain=self.root/'tc';(self.toolchain/'include').mkdir(parents=True)
        (self.root/'src').mkdir();(self.root/'work').mkdir()
        self.source=self.root/'src/unit.c';self.source.write_bytes(b'int unit;\n')
        self.header=self.toolchain/'include/used.h';self.header.write_bytes(b'/* header */\n')
        for mod,key,value in ((linked_repin,'ROOT',self.root),(linked,'DEFAULT_TOOLCHAIN',self.toolchain)):
            p=patch.object(mod,key,value);p.start();self.addCleanup(p.stop)
        p=patch.object(linked,'verify_toolchain',return_value='fixture');p.start();self.addCleanup(p.stop)
        p=patch.dict(os.environ,{'HERCULES_COMPILE_METRICS_DIR':''});p.start();self.addCleanup(p.stop)

    def marker(self,path):return b'#line 1 "'+str(path).replace('\\','\\\\').encode('ascii')+b'"\n'

    def run_scan(self,stdout,flags=None):
        from types import SimpleNamespace
        with patch.object(linked_repin.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=stdout,stderr=b'')) as run:
            result=linked_repin.compiler_dependencies(self.source,flags or ['/O2'])
        return result,run.call_args

    def test_exact_profile_and_sanitized_include_environment(self):
        with patch.dict(os.environ,{'CL':'/FIbad','_CL_':'@bad','INCLUDE':'work/external'}):
            (paths,record),call=self.run_scan(self.marker(self.source)+self.marker(self.header),['/O2','/DVALUE=1'])
        self.assertEqual(paths,{self.source,self.header})
        command=call.args[0];env=call.kwargs['env']
        self.assertEqual(command[2:-1],['/E','/Gy','/O2','/DVALUE=1'])
        self.assertEqual(env['INCLUDE'],str(self.toolchain/'include'))
        self.assertNotIn('CL',env);self.assertNotIn('_CL_',env)

    def test_external_relative_missing_and_malformed_markers_refuse(self):
        external=self.root/'work/external.h';external.write_bytes(b'/* external */\n')
        cases=[(self.marker(external),'outside src/ and pinned toolchain'),
               (b'#line 1 "relative.h"\n','Relative compiler dependency marker'),
               (self.marker(self.root/'src/missing.h'),'Missing compiler dependency'),
               (b'#line invalid "fake.h"\n','Unparsed compiler dependency marker'),
               (b'int value;\n','omitted its source')]
        for suffix,reason in cases:
            stdout=suffix if reason=='omitted its source' else self.marker(self.source)+suffix
            with self.subTest(reason=reason),self.assertRaisesRegex(ValueError,reason):self.run_scan(stdout)

    def test_include_forced_include_and_response_flags_refuse_before_launch(self):
        for flag in ('/Iwork','/I','/FIwork/external.h','@work/flags.rsp','-Iwork'):
            with patch.object(linked_repin.subprocess,'run') as run,self.subTest(flag=flag):
                with self.assertRaisesRegex(ValueError,'Compiler flag not allowed'):
                    linked_repin.compiler_dependencies(self.source,[flag])
                run.assert_not_called()


if __name__=='__main__':unittest.main()
