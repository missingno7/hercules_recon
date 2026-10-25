"""Adversarial loader-zero checks over synthetic PE/COFF files only."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import masked
import promote
from zero_storage import owner_profile, verify_common, verify_zero_section
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file


def fixture_pe(path, destination=0x2200):
    make_pe(path, base_relocation=True)
    data = bytearray(path.read_bytes())
    struct.pack_into('<I', data, 0x98 + 224 + 40 + 8, 0x400)
    data[0x400:0x406] = b'\xa1' + struct.pack('<I', 0x400000 + destination) + b'\xc3'
    anchor = b'\x68' + struct.pack('<I', 0x402200)
    data[0x500:0x505] = anchor
    struct.pack_into('<IIHH', data, 0x800, 0x1000, 12, 0x3001, 0x3101)
    path.write_bytes(data)
    return dict(rva=0x1100, size=5, operand=0, sha256=hashlib.sha256(anchor).hexdigest())


def fixture_owner(path, size=4, symbols=None):
    coff_file(path, b'', [], symbols or [('_cell', size, 0, 0)])


def fixture_caller(path, code=b'\xa1\0\0\0\0\xc3', offset=1):
    coff_file(path, code, [(offset, 1, masked.DIR32)],
              [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])


def fixture_zero_section(path, size=4, symbols=None, payload=None, relocations=(), extra=None):
    coff_file(path, b'' if payload is None else payload, relocations,
              symbols or [('_cell', 0, 1, 0)], data=extra)
    data = bytearray(path.read_bytes())
    data[20:28] = b'.bss\0\0\0\0'
    struct.pack_into('<I', data, 36, size)
    if payload is None:
        struct.pack_into('<I', data, 40, 0)
    struct.pack_into('<I', data, 56, 0xc0300080)
    path.write_bytes(data)


class ZeroStorageTests(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('zero-storage-')
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.target, self.caller, self.owner = [self.root / name for name in ('original.dll', 'caller.obj', 'owner.obj')]
        identity = fixture_pe(self.target)
        fixture_caller(self.caller); fixture_owner(self.owner)
        self.record = binding('bss', 0x2200, 4, (1, masked.DIR32, 0), identity=identity,
                              owner=dict(source='src/owner.c', flags=[], symbol='_cell'))

    def compare(self, **kwargs):
        return masked.relocated_compare(self.caller, '_sample', self.target, 0x1000,
            kwargs.pop('size', 6), dict(symbols={'_cell': self.record}),
            storage_owners=kwargs.pop('owners', {'_cell': self.owner}), **kwargs)

    def edit_pe(self, operation):
        data = bytearray(self.target.read_bytes()); operation(data); self.target.write_bytes(data)

    def test_exact_common_and_independent_anchor(self):
        self.assertTrue(self.compare()['relocated_equal'])

    def test_ordinary_data_cannot_silently_accept_a_zero_tail(self):
        self.record['kind']='data'
        self.record['sha256']=hashlib.sha256(bytes(4)).hexdigest()
        with self.assertRaisesRegex(ValueError,'file-backed storage'):
            self.compare()

    def test_identical_zero_neighbor_cannot_supply_identity(self):
        self.record['rva'] = 0x2204
        self.assertTrue(masked.masked_compare(self.caller, '_sample', self.target, 0x1000, 6)['masked_equal'])
        with self.assertRaisesRegex(ValueError, 'another storage cell'):
            self.compare()

    def test_owner_object_is_mandatory_and_must_be_complete_common(self):
        with self.assertRaisesRegex(ValueError, 'compiled owner'):
            self.compare(owners={})
        for symbols in ([('_cell', 0, 0, 0)], [('_cell', 8, 0, 0)],
                        [('_cell', 0, 1, 0)], [('_cell', 4, 0, 0x20)],
                        [('_cell', 4, 0, 0), ('_cell', 4, 0, 0)]):
            with self.subTest(symbols=symbols):
                fixture_owner(self.owner, symbols=symbols)
                with self.assertRaises(ValueError): self.compare()

    def test_file_backed_and_straddling_ranges_remain_rejected(self):
        for start in (0x2100, 0x21fe):
            self.record['rva'] = start
            with self.assertRaisesRegex(ValueError, 'zero tail'): self.compare()

    def test_read_only_executable_and_discardable_zero_tails_rejected(self):
        original = self.target.read_bytes()
        for flags in (0x40000040, 0xe0000040, 0xc2000040):
            self.target.write_bytes(original)
            self.edit_pe(lambda d: struct.pack_into('<I', d, 0x98 + 224 + 40 + 36, flags))
            with self.assertRaisesRegex(ValueError, 'zero tail'): self.compare()

    def test_overlapping_virtual_sections_cannot_claim_one_owner(self):
        self.edit_pe(lambda d: struct.pack_into('<I',d,0x98+224+2*40+12,0x2200))
        with self.assertRaisesRegex(ValueError,'unambiguous section'): self.compare()

    def test_zero_owner_cannot_extend_beyond_loaded_image(self):
        self.edit_pe(lambda d: struct.pack_into('<I',d,0x98+56,0x2202))
        with self.assertRaisesRegex(ValueError,'storage extent'): self.compare()

    def test_partial_and_circular_identity_anchors_rejected(self):
        original = copy.deepcopy(self.record['identity'])
        for change in (dict(sha256='0'*64), dict(rva=0x1000), dict(operand=1), dict(size=4)):
            self.record['identity'] = dict(original, **change)
            with self.assertRaises(ValueError): self.compare()

    def test_missing_or_wrong_anchor_fixup_rejected(self):
        self.edit_pe(lambda d: struct.pack_into('<H', d, 0x80a, 0x3102))
        with self.assertRaisesRegex(ValueError, 'HIGHLOW'): self.compare()

    def test_loader_fixup_inside_zero_owner_rejected(self):
        def add_fixup(data):
            struct.pack_into('<I', data, 0x98 + 96 + 5*8 + 4, 24)
            struct.pack_into('<IIHH', data, 0x80c, 0x2000, 12, 0x3200, 0)
        self.edit_pe(add_fixup)
        with self.assertRaisesRegex(ValueError, 'loader fixup'): self.compare()

    def test_loader_directory_tls_and_load_config_rejected(self):
        original = self.target.read_bytes()
        for index, rva in ((12, 0x2200), (9, 0x2100), (10, 0x2100)):
            self.target.write_bytes(original)
            self.edit_pe(lambda d: struct.pack_into('<II', d, 0x98 + 96 + index*8, rva, 4))
            with self.assertRaises(ValueError): self.compare()

    def test_partial_memory_read_cannot_cross_owner(self):
        fixture_caller(self.caller, b'\xa1\x02\0\0\0\xc3')
        self.record['references'][0]['addend'] = 2
        self.edit_pe(lambda d: struct.pack_into('<I', d, 0x401, 0x402202))
        with self.assertRaisesRegex(ValueError, 'exceeds.*scalar owner'): self.compare()

    def test_last_byte_access_is_bounded_but_indexing_is_not_assumed(self):
        fixture_caller(self.caller, b'\xa0\x03\0\0\0\xc3')
        self.record['references'][0]['addend'] = 3
        self.edit_pe(lambda d: d.__setitem__(slice(0x400,0x406), b'\xa0'+struct.pack('<I',0x402203)+b'\xc3'))
        self.assertTrue(self.compare()['relocated_equal'])
        fixture_caller(self.caller, bytes.fromhex('8b048500000000c3'), 3)
        self.record['references'][0].update(offset=3, addend=0)
        def indexed(data):
            data[0x400:0x408] = bytes.fromhex('8b0485')+struct.pack('<I',0x402200)+b'\xc3'
            struct.pack_into('<H',data,0x808,0x3003)
        self.edit_pe(indexed)
        with self.assertRaisesRegex(ValueError, 'scalar owner'): self.compare(size=8)

    def test_nonreference_bytes_and_original_use_still_have_to_match(self):
        self.edit_pe(lambda d: d.__setitem__(0x405,0xcc))
        self.assertFalse(self.compare()['relocated_equal'])
        self.target.write_bytes(self.target.read_bytes()[:0x405]+b'\xc3'+self.target.read_bytes()[0x406:])
        self.edit_pe(lambda d: struct.pack_into('<I',d,0x401,0x402204))
        self.assertFalse(self.compare()['relocated_equal'])


class ZeroSectionTests(ZeroStorageTests):
    def setUp(self):
        super().setUp()
        fixture_zero_section(self.owner)
        self.record['owner_storage'] = 'zero-section-v1'

    def test_owner_object_is_mandatory_and_must_be_complete_common(self):
        # The inherited mandatory/default-owner test has COMMON-specific cases.
        with self.assertRaisesRegex(ValueError, 'compiled owner'):
            self.compare(owners={})
        with self.assertRaises(ValueError):
            verify_common(self.owner, '_cell', 4)
        del self.record['owner_storage']
        with self.assertRaisesRegex(ValueError, 'COMMON'):
            self.compare()
        self.record['owner_storage'] = 'zero-section-v1'
        fixture_owner(self.owner)
        with self.assertRaises(ValueError):
            self.compare()

    def test_exact_section_extent_and_unique_object(self):
        for symbols in ([('_cell', 1, 1, 0)], [('_cell', 0, 1, 0x20)],
                        [('_cell', 0, 1, 0), ('_other', 0, 1, 0)],
                        [('_cell', 0, 1, 0), ('_cell', 0, 1, 0)],
                        [('_cell', 0, 1, 0), ('_code', 0, 1, 0x20)]):
            with self.subTest(symbols=symbols):
                fixture_zero_section(self.owner, symbols=symbols)
                with self.assertRaises(ValueError): self.compare()
        fixture_zero_section(self.owner)
        for extent in (3, 5):
            with self.assertRaises(ValueError): verify_zero_section(self.owner, '_cell', extent)

    def test_initialized_payload_relocations_and_other_storage_rejected(self):
        for options in (dict(payload=bytes(4)), dict(payload=b'\1\0\0\0'),
                        dict(relocations=[(0, 0, masked.DIR32)]), dict(extra=b'\0')):
            with self.subTest(options=options):
                fixture_zero_section(self.owner, **options)
                with self.assertRaises(ValueError): self.compare()
        fixture_zero_section(self.owner, extra=b'-alternatename:a=b ')
        data = bytearray(self.owner.read_bytes()); data[60:68] = b'.drectve'
        struct.pack_into('<I', data, 96, 0x100a00); self.owner.write_bytes(data)
        with self.assertRaisesRegex(ValueError, 'directives'): self.compare()

    def test_section_kind_flags_weak_symbols_and_static_aliases_rejected(self):
        fixture_zero_section(self.owner)
        original = self.owner.read_bytes()
        for flags in (0xc0300040, 0xe0300080, 0xc2300080, 0xc0301080,
                      0xc0300280, 0xc0f00080):
            data = bytearray(original); struct.pack_into('<I', data, 56, flags)
            self.owner.write_bytes(data)
            with self.assertRaises(ValueError): self.compare()
        self.owner.write_bytes(original)
        for storage in (3, 105):
            fixture_zero_section(self.owner, symbols=[('_cell', 0, 1, 0), ('_alias', 0, 1, 0)])
            data = bytearray(self.owner.read_bytes())
            symbols_at = struct.unpack_from('<I', data, 8)[0]
            data[symbols_at + 18 + 16] = storage; self.owner.write_bytes(data)
            with self.assertRaises(ValueError): self.compare()

    def test_section_definition_auxiliary_cannot_hide_comdat_selection(self):
        fixture_zero_section(self.owner)
        data = bytearray(self.owner.read_bytes())
        symbols_at, count = struct.unpack_from('<II', data, 8)
        strings_at = symbols_at + count * 18
        section_symbol = struct.pack('<8sIhHBB', b'.bss', 0, 1, 0, 3, 1)
        auxiliary = struct.pack('<I', 4) + bytes(14)
        data[strings_at:strings_at] = section_symbol + auxiliary
        struct.pack_into('<I', data, 12, count + 2)
        self.owner.write_bytes(data)
        self.assertTrue(self.compare()['relocated_equal'])
        data[strings_at + 18 + 14] = 2
        self.owner.write_bytes(data)
        with self.assertRaisesRegex(ValueError, 'objects, aliases or functions'):
            self.compare()

    def test_unknown_representation_and_non_c_or_alias_profile_rejected(self):
        for representation in ('guess-zero', None):
            self.record['owner_storage'] = representation
            with self.assertRaisesRegex(ValueError, 'representation'): self.compare()
            with self.assertRaisesRegex(ValueError, 'representation'):
                owner_profile(self.record, '_cell', self.root)
        self.record['owner_storage'] = 'zero-section-v1'
        for update in (dict(source='src/owner.cpp'), dict(flags=['/TP'])):
            record = copy.deepcopy(self.record); record['owner'].update(update)
            with self.assertRaisesRegex(ValueError, 'C data-only'):
                owner_profile(record, '_cell', self.root)
        record = copy.deepcopy(self.record); record['owner']['symbol'] = '_alias'
        with self.assertRaisesRegex(ValueError, 'aliases'): owner_profile(record, '_cell', self.root)
        record = copy.deepcopy(self.record); record['source_alias'] = {}
        with self.assertRaisesRegex(ValueError, 'aliases'): owner_profile(record, '_cell', self.root)

    def test_data_owner_does_not_authorize_indirect_control_flow(self):
        fixture_caller(self.caller, bytes.fromhex('ff1500000000'), 2)
        self.record['references'][0].update(offset=2)
        def indirect(data):
            data[0x400:0x406] = bytes.fromhex('ff15') + struct.pack('<I', 0x402200)
            struct.pack_into('<H', data, 0x808, 0x3002)
        self.edit_pe(indirect)
        with self.assertRaisesRegex(ValueError, 'indirect control transfer'):
            self.compare()


class ZeroStorageAcceptanceTests(unittest.TestCase):
    def setUp(self):
        directory = workspace_temporary_directory('zero-acceptance-')
        self.addCleanup(directory.cleanup); self.addCleanup(patch.stopall)
        self.root=Path(directory.name)
        for name in ('src','originals','evidence','toolchains','work','objects'):
            (self.root/name).mkdir()
        (self.root/'toolchains/manifest.json').write_text('{}\n')
        self.source=self.root/'src/caller.c';self.source.write_text('extern int cell; int sample(void) { return cell; }\n')
        self.owner_source=self.root/'src/owner.c';self.owner_source.write_text('#include "owner.h"\nint cell;\n')
        self.header=self.root/'src/owner.h';self.header.write_text('/* independently pinned */\n')
        self.note=self.root/'evidence/identity.txt';self.note.write_text('Independent synthetic cell role and native anchor.\n')
        self.target=self.root/'originals/original.dll';identity=fixture_pe(self.target)
        self.obj=self.root/'objects/caller.obj';fixture_caller(self.obj)
        self.owner_obj=self.root/'objects/owner.obj';fixture_owner(self.owner_obj)
        self.bindings=self.root/'evidence/bindings.json'
        self.document=dict(schema=1,evidence_files={p.relative_to(self.root).as_posix():match.digest(p.read_bytes()) for p in (self.owner_source,self.header,self.note)},
            functions={'cell-reader':dict(symbols={'_cell':binding('bss',0x2200,4,(1,masked.DIR32,0),identity=identity,
                owner=dict(source='src/owner.c',flags=[],symbol='_cell'))})})
        self.write_bindings()
        self.row=dict(id='cell-reader',status='CODEGEN_SIMILAR',source='src/caller.c',symbol='_sample',flags=[],
            target='originals/original.dll',target_sha256=match.digest(self.target.read_bytes()),rva='0x1000',size=6,
            proof=dict(scope='synthetic diagnostic',source_sha256=match.digest(self.source.read_bytes())))
        self.state=self.root/'recovery.json';self.state.write_text(json.dumps(dict(functions=[self.row]))+'\n')
        patch.object(match,'ROOT',self.root).start();patch.object(promote,'ROOT',self.root).start()
        patch.object(promote,'STATE',self.state).start();patch.object(promote,'JOURNAL',self.root/'work/promotion-journal.json').start()
        verifier=Mock(return_value='synthetic');verifier.cache_clear=Mock()
        patch.object(match,'verify_toolchain',verifier).start()
        self.compiler=patch.object(match,'compile_source',side_effect=self.compile).start()

    def write_bindings(self): self.bindings.write_text(json.dumps(self.document)+'\n')
    def compile(self,source,flags,toolchain):
        return (self.owner_obj if Path(source)==self.owner_source else self.obj),['synthetic',str(source)]
    def verify(self):
        return match.verify_rows([self.row],relocation_candidates={'cell-reader':'evidence/bindings.json'})

    def test_fresh_owner_every_run_and_normal_transaction(self):
        result,stats=self.verify();self.assertTrue(result[0]['exact']);self.assertEqual(stats['compiler_calls'],2)
        receipt=promote.promote_relocated(['cell-reader'],self.bindings)
        accepted=json.loads(self.state.read_text())['functions'][0]
        self.assertEqual(accepted['status'],'FUNCTION_MATCH_RELOCATED')
        checked,_=match.verify_rows([accepted]);self.assertTrue(checked[0]['exact'])
        self.assertEqual(self.compiler.call_count,6)
        self.assertIn('_cell',receipt['results'][0]['storage_objects'])

    def test_each_owner_include_must_be_pinned_in_its_manifest(self):
        del self.document['evidence_files']['src/owner.h'];self.write_bindings()
        with self.assertRaisesRegex(ValueError,'independently pinned'):self.verify()
        self.assertEqual(self.compiler.call_count,0)

    def test_source_owner_alias_and_escape_rejected(self):
        owner=self.document['functions']['cell-reader']['symbols']['_cell']['owner']
        owner['symbol']='_other';self.write_bindings()
        with self.assertRaisesRegex(ValueError,'aliases'):self.verify()
        owner['symbol']='_cell';owner['source']='../unreviewed.c';self.write_bindings()
        with self.assertRaisesRegex(ValueError,'canonical repository'):self.verify()
        owner['source']='src/owner.c';owner['flags']=['/Foarbitrary.obj'];self.write_bindings()
        with self.assertRaisesRegex(ValueError,'Unsupported grouped compile input flag'):self.verify()

    def test_owner_header_race_during_caller_compile_rejected(self):
        original=self.compile
        def raced(source,flags,toolchain):
            if Path(source)==self.source:self.header.write_text('/* raced */\n')
            return original(source,flags,toolchain)
        self.compiler.side_effect=raced
        with self.assertRaisesRegex(ValueError,'changed'):self.verify()

    def test_owner_object_race_before_publication_rejected(self):
        original=promote.verify_rows
        def raced(*args,**kwargs):
            result=original(*args,**kwargs)
            content=bytearray(self.owner_obj.read_bytes());content[4]^=1;self.owner_obj.write_bytes(content)
            return result
        before=self.state.read_bytes()
        with patch.object(promote,'verify_rows',side_effect=raced):
            with self.assertRaisesRegex(ValueError,'owner object changed before publication'):
                promote.promote_relocated(['cell-reader'],self.bindings)
        self.assertEqual(before,self.state.read_bytes())

    def test_native_anchor_cannot_be_inside_another_pending_caller(self):
        other=dict(copy.deepcopy(self.row),id='anchor-function',rva='0x1100')
        self.document['functions']['anchor-function']=copy.deepcopy(self.document['functions']['cell-reader'])
        self.write_bindings()
        with self.assertRaisesRegex(ValueError,'selected caller'):
            match.verify_rows([self.row,other],relocation_candidates={r['id']:'evidence/bindings.json' for r in (self.row,other)})

    def use_zero_section(self):
        self.document['functions']['cell-reader']['symbols']['_cell']['owner_storage'] = 'zero-section-v1'
        self.owner_source.write_text('#include "owner.h"\nint cell = {0};\n')
        self.document['evidence_files']['src/owner.c'] = match.digest(self.owner_source.read_bytes())
        fixture_zero_section(self.owner_obj)
        self.write_bindings()

    def test_zero_section_uses_fresh_owner_and_existing_relocated_category(self):
        self.use_zero_section()
        result, stats = self.verify()
        self.assertTrue(result[0]['exact']); self.assertEqual(stats['compiler_calls'], 2)
        promote.promote_relocated(['cell-reader'], self.bindings)
        accepted = json.loads(self.state.read_text())['functions'][0]
        self.assertEqual(accepted['status'], 'FUNCTION_MATCH_RELOCATED')
        checked, stats = match.verify_rows([accepted])
        self.assertTrue(checked[0]['exact']); self.assertEqual(stats['compiler_calls'], 2)
        self.assertEqual(self.compiler.call_count, 6)

    def test_zero_section_preserves_include_pinning_and_race_guards(self):
        self.use_zero_section()
        del self.document['evidence_files']['src/owner.h']; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'independently pinned'): self.verify()
        self.assertEqual(self.compiler.call_count, 0)
        self.document['evidence_files']['src/owner.h'] = match.digest(self.header.read_bytes())
        self.write_bindings()
        original = self.compile
        def raced(source, flags, toolchain):
            if Path(source) == self.source: self.header.write_text('/* raced */\n')
            return original(source, flags, toolchain)
        self.compiler.side_effect = raced
        with self.assertRaisesRegex(ValueError, 'changed'): self.verify()


if __name__=='__main__': unittest.main()
