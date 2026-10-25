"""Data-owner renaming: exact COMMON inventories, typed wrappers and races."""
import copy,json,struct,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import match,masked,promote,storage_alias
import test_zero_storage as zero
from test_relocated import coff_file,binding
from test_matching import workspace_temporary_directory


class OwnerAliasAcceptanceTests(unittest.TestCase):
    write_bindings=zero.ZeroStorageAcceptanceTests.write_bindings
    verify=zero.ZeroStorageAcceptanceTests.verify

    def setUp(self):
        zero.ZeroStorageAcceptanceTests.setUp(self)
        self.source.write_text('extern unsigned char *cell; void *sample(void){return cell;}\n')
        self.owner_source.write_text('#include "owner.h"\nvoid *owned;\n')
        self.alias_header=self.root/'src/alias.h';self.alias_header.write_text('#define owned cell\n')
        zero.fixture_owner(self.owner_obj,symbols=[('_owned',4,0,0)])
        self.alias_obj=self.root/'objects/alias.obj';zero.fixture_owner(self.alias_obj,symbols=[('_cell',4,0,0)])
        self.record['owner']['symbol']='_owned'
        self.record['source_alias']=dict(kind='owner-compile-renaming',header='src/alias.h',renames={'cell':'owned'},pointer_types={'cell':'void'})
        self.document['evidence_files']={p.relative_to(self.root).as_posix():match.digest(p.read_bytes()) for p in [self.owner_source,self.header,self.note,self.alias_header]}
        self.write_bindings();self.row['proof']['source_sha256']=match.digest(self.source.read_bytes());self.state.write_text(json.dumps(dict(functions=[self.row]))+'\n')
        self.extra_profiles={}

    @property
    def record(self):return self.document['functions']['cell-reader']['symbols']['_cell']

    def compile(self,source,flags,toolchain):
        source=Path(source)
        if source==self.source:obj=self.obj
        elif source==self.owner_source:obj=self.owner_obj
        elif source in getattr(self,'extra_profiles',{}):obj=self.extra_profiles[source]
        else:
            text=source.read_text();self.assertTrue(source.is_relative_to(self.root/'build/match'))
            self.assertIn('/WX',flags);self.assertIn('sizeof(void *) == 4',text)
            self.assertIn('extern ',text)
            obj=self.alias_obj
            if 'extern unsigned char *cell2;' in text:obj=self.second_alias
        return obj,['synthetic',str(source)]

    def test_fresh_owner_adapter_caller_and_transaction(self):
        results,stats=self.verify();self.assertTrue(results[0]['exact']);self.assertEqual(stats['compiler_calls'],3)
        self.assertEqual(results[0]['source_alias_checks']['_cell']['functions'],0)
        promote.promote_relocated(['cell-reader'],self.bindings)
        accepted=json.loads(self.state.read_text())['functions'][0]
        self.assertEqual(accepted['status'],'FUNCTION_MATCH_RELOCATED')
        result,_=match.verify_rows([accepted]);self.assertTrue(result[0]['exact']);self.assertEqual(self.compiler.call_count,9)

    def test_exact_header_and_type_schema(self):
        for change in [dict(size=2),dict(size=4.0),dict(source_alias=dict(self.record['source_alias'],pointer_types={'cell':'int'})),dict(source_alias=dict(self.record['source_alias'],pointer_types={'cell':'void','extra':'void'}))]:
            saved=copy.deepcopy(self.record);self.record.update(change);self.write_bindings()
            with self.subTest(change=change),self.assertRaises(ValueError):self.verify()
            self.record.clear();self.record.update(saved)
        self.assertEqual(self.compiler.call_count,0)

    def test_header_cannot_add_includes_conditions_or_type_macros(self):
        for extra in ['#include "owner.h"\n','#define void unsigned long\n','#if 1\n#endif\n','#define extra owned\n','#define owned cell\n']:
            self.alias_header.write_text('#define owned cell\n'+extra)
            self.document['evidence_files']['src/alias.h']=match.digest(self.alias_header.read_bytes());self.write_bindings()
            with self.subTest(extra=extra),self.assertRaisesRegex(ValueError,'header'):self.verify()
        self.assertEqual(self.compiler.call_count,0)

    def test_wrong_header_direction_rejected(self):
        self.alias_header.write_text('#define cell owned\n');self.document['evidence_files']['src/alias.h']=match.digest(self.alias_header.read_bytes());self.write_bindings()
        with self.assertRaisesRegex(ValueError,'inverse macro map'):self.verify()

    def test_unpinned_header_rejected_before_compile(self):
        del self.document['evidence_files']['src/alias.h'];self.write_bindings()
        with self.assertRaisesRegex(ValueError,'independently pinned'):self.verify()
        self.assertEqual(self.compiler.call_count,0)

    def test_owner_must_use_c_profile(self):
        self.record['owner']['flags']=['/O2','/TP'];self.write_bindings()
        with self.assertRaisesRegex(ValueError,'C data-only'):self.verify()

    def test_caller_common_function_or_weak_cannot_alias(self):
        for value,typ,weak in [(4,0,False),(0,0x20,False),(0,0,True)]:
            coff_file(self.obj,b'\xa1\0\0\0\0\xc3',[(1,1,6)],[('_sample',0,1,0x20),('_cell',value,0,typ)])
            if weak:
                data=bytearray(self.obj.read_bytes());at=struct.unpack_from('<I',data,8)[0]+18+16;data[at]=105;self.obj.write_bytes(data)
            with self.subTest(value=value,typ=typ,weak=weak),self.assertRaisesRegex(ValueError,'undefined|weak/alias'):self.verify()

    def test_preexisting_caller_owner_name_cannot_coalesce(self):
        coff_file(self.obj,b'\xa1\0\0\0\0\xc3',[(1,1,6)],[('_sample',0,1,0x20),('_cell',0,0,0),('_owned',0,0,0)])
        with self.assertRaisesRegex(ValueError,'pre-existing caller'):self.verify()

    def test_original_and_adapted_must_have_exact_common_extent(self):
        for obj,name in [(self.owner_obj,'_owned'),(self.alias_obj,'_cell')]:
            original=obj.read_bytes()
            for extent in (0,2,8):
                zero.fixture_owner(obj,symbols=[(name,extent,0,0)])
                with self.subTest(obj=obj,extent=extent),self.assertRaisesRegex(ValueError,'exact-size COMMON'):self.verify()
            obj.write_bytes(original)

    def test_original_target_and_retained_owner_names_cannot_merge(self):
        zero.fixture_owner(self.owner_obj,symbols=[('_owned',4,0,0),('_cell',4,0,0)])
        with self.assertRaisesRegex(ValueError,'merge pre-existing'):self.verify()
        zero.fixture_owner(self.owner_obj,symbols=[('_owned',4,0,0)])
        zero.fixture_owner(self.alias_obj,symbols=[('_cell',4,0,0),('_owned',4,0,0)])
        with self.assertRaisesRegex(ValueError,'merge pre-existing'):self.verify()

    def test_weak_owner_rejected(self):
        data=bytearray(self.owner_obj.read_bytes());at=struct.unpack_from('<I',data,8)[0]+16;data[at]=105;self.owner_obj.write_bytes(data)
        with self.assertRaisesRegex(ValueError,'weak symbols'):self.verify()

    def test_any_owner_function_or_executable_bytes_rejected(self):
        for code,symbols in [(b'\xc3',[('_owned',4,0,0),('_helper',0,1,0x20)]),(b'\x90',[('_owned',4,0,0)])]:
            coff_file(self.owner_obj,code,[],symbols)
            with self.subTest(code=code),self.assertRaisesRegex(ValueError,'data-only unit'):self.verify()

    def test_common_inventory_cannot_lose_other_objects(self):
        zero.fixture_owner(self.owner_obj,symbols=[('_owned',4,0,0),('_other',8,0,0)])
        with self.assertRaisesRegex(ValueError,'storage/symbol inventory'):self.verify()

    def test_data_and_directive_payloads_cannot_change(self):
        coff_file(self.owner_obj,b'',[],[('_owned',4,0,0)],data=b'old')
        coff_file(self.alias_obj,b'',[],[('_cell',4,0,0)],data=b'new')
        with self.assertRaisesRegex(ValueError,'data/directive sections'):self.verify()

    def test_weak_linker_directive_cannot_replace_strong_ownership(self):
        for obj,name in [(self.owner_obj,'_owned'),(self.alias_obj,'_cell')]:
            coff_file(obj,b'',[],[(name,4,0,0)],data=b'/ALTERNATENAME:_x=_owned')
            data=bytearray(obj.read_bytes());data[60:68]=b'.drectve';obj.write_bytes(data)
        with self.assertRaisesRegex(ValueError,'weak linker alias'):self.verify()

    def test_static_symbols_auxiliaries_and_section_flags_are_preserved(self):
        original=storage_alias.COFF
        for kind in ('flags','aux','extra_static','relocation'):
            caller,left,right=map(original,(self.obj,self.owner_obj,self.alias_obj))
            if kind=='flags':right.sections[0]['flags']^=0x40000000
            elif kind=='aux':right.symbols[0]['aux']=[b'\0'*18]
            elif kind=='extra_static':right.symbols[1]=dict(name='local',value=0,section=1,type=0,storage=3,aux=[])
            else:right.sections[0]['relocations']=[dict(offset=0,type=6,symbol='_cell')]
            with self.subTest(kind=kind),patch.object(storage_alias,'COFF',side_effect=[caller,left,right]),self.assertRaisesRegex(ValueError,'changed'):
                storage_alias.verify_owner_alias_object(self.obj,self.owner_obj,self.alias_obj,{'_cell':'_owned'})

    def test_file_metadata_cannot_hide_storage(self):
        caller,left,right=map(storage_alias.COFF,(self.obj,self.owner_obj,self.alias_obj))
        right.symbols[1]=dict(name='hidden',section=1,value=0,type=0,storage=103,aux=[])
        with patch.object(storage_alias,'COFF',side_effect=[caller,left,right]),self.assertRaisesRegex(ValueError,'malformed FILE'):
            storage_alias.verify_owner_alias_object(self.obj,self.owner_obj,self.alias_obj,{'_cell':'_owned'})

    def test_defined_initialized_data_cannot_be_a_common_owner(self):
        coff_file(self.owner_obj,b'',[],[('_owned',0,2,0)],data=b'\0'*4)
        with self.assertRaisesRegex(ValueError,'exact-size COMMON'):self.verify()

    def test_multi_alias_maps_and_one_past_bss_remain_unsupported(self):
        self.record['source_alias']['renames']['other']='another'
        self.record['source_alias']['pointer_types']['other']='void';self.write_bindings()
        with self.assertRaisesRegex(ValueError,'one explicit'):self.verify()
        del self.record['source_alias']['renames']['other'];del self.record['source_alias']['pointer_types']['other']
        self.record['references'][0].update(addend=4,one_past=True);self.write_bindings()
        with self.assertRaisesRegex(ValueError,'one-past'):self.verify()

    def test_nonreference_and_native_identity_gate_unchanged(self):
        data=bytearray(self.target.read_bytes());data[0x405]=0xcc;self.target.write_bytes(data)
        self.row['target_sha256']=match.digest(self.target.read_bytes())
        result,_=self.verify();self.assertFalse(result[0]['exact'])
        data[0x405]=0xc3;self.target.write_bytes(data);self.row['target_sha256']=match.digest(self.target.read_bytes())
        self.record['rva']+=4;self.write_bindings()
        with self.assertRaisesRegex(ValueError,'another storage cell'):self.verify()

    def test_missing_adapter_rejected(self):
        with self.assertRaisesRegex(ValueError,'freshly compiled rename object'):
            masked.relocated_compare(self.obj,'_sample',self.target,0x1000,6,dict(symbols={'_cell':self.record}),storage_owners={'_cell':self.owner_obj})

    def publication_race(self,operation,message):
        original=promote.verify_rows
        def raced(*args,**kwargs):
            result=original(*args,**kwargs);operation();return result
        before=self.state.read_bytes()
        with patch.object(promote,'verify_rows',side_effect=raced),self.assertRaisesRegex(ValueError,message):promote.promote_relocated(['cell-reader'],self.bindings)
        self.assertEqual(before,self.state.read_bytes())

    def test_adapter_object_race_before_publication(self):
        self.publication_race(lambda:self.alias_obj.write_bytes(self.alias_obj.read_bytes()+b'changed'),'Compiler-renaming object changed')

    def test_adapter_header_race_before_publication(self):
        self.publication_race(lambda:self.alias_header.write_text('/* changed */\n'),'Compiler-renaming input changed')

    def test_wrapper_race_during_compile(self):
        original=self.compile
        def raced(source,flags,toolchain):
            result=original(source,flags,toolchain)
            if Path(source) not in (self.source,self.owner_source):Path(source).write_text('/* changed */\n')
            return result
        self.compiler.side_effect=raced
        with self.assertRaisesRegex(ValueError,'changed'):self.verify()

    def test_adapter_race_during_caller_compile(self):
        original=self.compile
        def raced(source,flags,toolchain):
            result=original(source,flags,toolchain)
            if Path(source)==self.source:self.alias_obj.write_bytes(self.alias_obj.read_bytes()+b'changed')
            return result
        self.compiler.side_effect=raced
        with self.assertRaisesRegex(ValueError,'Object changed'):self.verify()

    def add_second(self):
        owner=self.root/'src/owner2.c';owner.write_text('unsigned char *own2;\n')
        header=self.root/'src/alias2.h';header.write_text('#define own2 cell2\n')
        owner_obj=self.root/'objects/owner2.obj';zero.fixture_owner(owner_obj,symbols=[('_own2',4,0,0)])
        self.second_alias=self.root/'objects/alias2.obj';zero.fixture_owner(self.second_alias,symbols=[('_cell2',4,0,0)])
        self.extra_profiles[owner]=owner_obj
        data=bytearray(self.target.read_bytes());data[0x400:0x40c]=b'\xa1'+struct.pack('<I',0x402200)+b'\x8b\x0d'+struct.pack('<I',0x402204)+b'\xc3'
        data[0x505:0x50a]=b'\x68'+struct.pack('<I',0x402204)
        struct.pack_into('<IIHHHH',data,0x800,0x1000,16,0x3001,0x3007,0x3101,0x3106)
        self.target.write_bytes(data);coff_file(self.obj,b'\xa1\0\0\0\0\x8b\x0d\0\0\0\0\xc3',[(1,1,6),(7,2,6)],[('_sample',0,1,0x20),('_cell',0,0,0),('_cell2',0,0,0)])
        anchor=b'\x68'+struct.pack('<I',0x402204)
        self.document['functions']['cell-reader']['symbols']['_cell2']=binding('bss',0x2204,4,(7,6,0),owner=dict(source='src/owner2.c',flags=[],symbol='_own2'),identity=dict(rva=0x1105,size=5,operand=0,sha256=match.digest(anchor)),source_alias=dict(kind='owner-compile-renaming',header='src/alias2.h',renames={'cell2':'own2'},pointer_types={'cell2':'byte'}))
        for p in [owner,header]:self.document['evidence_files'][p.relative_to(self.root).as_posix()]=match.digest(p.read_bytes())
        self.row.update(size=12,target_sha256=match.digest(self.target.read_bytes()));self.state.write_text(json.dumps(dict(functions=[self.row]))+'\n');self.write_bindings()

    def test_two_distinct_owner_profiles_in_one_caller(self):
        self.add_second();results,stats=self.verify();self.assertTrue(results[0]['exact']);self.assertEqual(stats['compiler_calls'],5)
        self.assertEqual(set(results[0]['storage_alias_objects']),{'_cell','_cell2'})

    def test_second_adapter_is_guarded_before_publication(self):
        self.add_second();self.publication_race(lambda:self.second_alias.write_bytes(self.second_alias.read_bytes()+b'changed'),'Compiler-renaming object changed')


class OwnerAliasCompilerTypes(unittest.TestCase):
    def build(self,declaration,pointer_type):
        directory=workspace_temporary_directory('owner-pointer-type-');self.addCleanup(directory.cleanup);root=Path(directory.name)
        source=root/'owner.c';source.write_text(declaration+'\n');header=root/'alias.h';header.write_text('#define owned cell\n')
        wrapper=storage_alias.wrapper_source(source,header,root,pointer_types={'cell':pointer_type})
        return match.compile_source(wrapper,['/O2','/WX'])

    def test_real_compiler_accepts_both_explicit_pointer_types(self):
        for declaration,typ in [('void *owned;','void'),('unsigned char *owned;','byte')]:
            with self.subTest(declaration=declaration):self.build(declaration,typ)

    def test_real_compiler_rejects_integer_array_function_and_wrong_pointer_type(self):
        for declaration in ['unsigned long owned;','void *owned[1];','void (*owned)(void);','unsigned char *owned;','void *volatile owned;','const void *owned;']:
            with self.subTest(declaration=declaration),self.assertRaises(RuntimeError):self.build(declaration,'void')


if __name__=='__main__':unittest.main()
