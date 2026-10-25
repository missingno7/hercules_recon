"""WORD alias proofs reject changed code/storage, stale inputs and races."""
import copy
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import match
import masked
import promote
import storage_alias
from test_relocated import binding, coff_file
import test_zero_storage as zero_fixtures
from test_zero_storage import fixture_owner

WORD_CODE = b'\x0f\xb7\x05\0\0\0\0\xc3'


class AliasAcceptanceTests(unittest.TestCase):
    write_bindings = zero_fixtures.ZeroStorageAcceptanceTests.write_bindings
    verify = zero_fixtures.ZeroStorageAcceptanceTests.verify

    def setUp(self):
        zero_fixtures.ZeroStorageAcceptanceTests.setUp(self)
        self.alias_header = self.root / 'src/alias.h'
        self.alias_header.write_text('#define cell owned\n')
        self.source.write_text('extern unsigned short cell; unsigned int sample(void){return cell;}\n')
        self.owner_source.write_text('#include "owner.h"\nunsigned short owned;\n')
        data = bytearray(self.target.read_bytes())
        data[0x400:0x408] = WORD_CODE[:3] + struct.pack('<I', 0x402200) + b'\xc3'
        struct.pack_into('<H', data, 0x808, 0x3003)
        self.target.write_bytes(data)
        coff_file(self.obj, WORD_CODE, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])
        fixture_owner(self.owner_obj, symbols=[('_owned', 2, 0, 0)])
        self.alias_obj = self.root / 'objects/alias.obj'
        coff_file(self.alias_obj, WORD_CODE, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0)])
        record = self.document['functions']['cell-reader']['symbols']['_cell']
        record.update(size=2, references=[dict(offset=3, type=masked.DIR32, addend=0)],
                      source_alias=dict(kind='compile-renaming', header='src/alias.h', renames={'cell': 'owned'}))
        record['owner']['symbol'] = '_owned'
        self.document['evidence_files'] = {p.relative_to(self.root).as_posix(): match.digest(p.read_bytes())
                                          for p in [self.owner_source, self.header, self.note, self.alias_header]}
        self.write_bindings()
        self.row.update(size=8, target_sha256=match.digest(self.target.read_bytes()))
        self.row['proof']['source_sha256'] = match.digest(self.source.read_bytes())
        self.state.write_text(__import__('json').dumps(dict(functions=[self.row])) + '\n')

    def compile(self, source, flags, toolchain):
        source = Path(source)
        if source == self.owner_source:
            obj = self.owner_obj
        elif source == self.source:
            obj = self.obj
        else:
            self.assertTrue(source.is_relative_to(self.root / 'build/match'))
            self.assertIn(self.alias_header.as_posix(), source.read_text())
            obj = self.alias_obj
        return obj, ['synthetic', str(source)]

    @property
    def record(self):
        return self.document['functions']['cell-reader']['symbols']['_cell']

    def test_fresh_original_owner_and_adapter_each_run(self):
        result, stats = self.verify()
        self.assertTrue(result[0]['exact'])
        self.assertEqual(stats['compiler_calls'], 3)
        self.assertIn('_cell', result[0]['storage_alias_objects'])
        self.assertEqual(result[0]['source_alias_checks']['_cell']['functions'], 1)
        promote.promote_relocated(['cell-reader'], self.bindings)
        import json
        accepted = json.loads(self.state.read_text())['functions'][0]
        self.assertEqual(accepted['status'], 'FUNCTION_MATCH_RELOCATED')
        result, _ = match.verify_rows([accepted])
        self.assertTrue(result[0]['exact'])
        self.assertEqual(self.compiler.call_count, 9)

    def test_unpinned_alias_header_rejected_before_compile(self):
        del self.document['evidence_files']['src/alias.h']; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'independently pinned'): self.verify()
        self.assertEqual(self.compiler.call_count, 0)

    def test_same_name_owner_cannot_claim_adapter(self):
        self.record['owner']['symbol'] = '_cell'; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'Same-name'): self.verify()

    def test_plain_alias_still_rejected(self):
        del self.record['source_alias']; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'explicit compiler-renaming'): self.verify()

    def test_pointer_or_other_extent_alias_rejected(self):
        for size in [4, 2.0]:
            self.record['size'] = size; self.write_bindings()
            with self.assertRaisesRegex(ValueError, 'WORD scalars'): self.verify()
        self.assertEqual(self.compiler.call_count, 0)

    def test_wrong_mapping_and_chains_rejected(self):
        for mapping in [{'cell': 'elsewhere'}, {'cell': 'owned', 'owned': 'other'},
                        {'cell': 'owned', 'other': 'owned'}, {'cell': 'owned /Dunsafe'}]:
            with self.subTest(mapping=mapping):
                self.record['source_alias']['renames'] = mapping; self.write_bindings()
                with self.assertRaises(ValueError): self.verify()

    def test_noncanonical_alias_header_rejected(self):
        header = self.root / 'work/alias.h'; header.write_bytes(self.alias_header.read_bytes())
        self.record['source_alias']['header'] = 'work/alias.h'; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'canonical source'): self.verify()

    def test_changed_signed_extension_rejected(self):
        code = WORD_CODE.replace(b'\xb7', b'\xbf')
        coff_file(self.alias_obj, code, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0)])
        with self.assertRaisesRegex(ValueError, 'complete function'): self.verify()

    def test_changed_relocation_addend_rejected(self):
        code = WORD_CODE[:3] + struct.pack('<I', 2) + b'\xc3'
        coff_file(self.alias_obj, code, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0)])
        with self.assertRaisesRegex(ValueError, 'complete function'): self.verify()

    def test_source_common_cannot_be_merged_into_owner(self):
        coff_file(self.obj, WORD_CODE, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_cell', 2, 0, 0)])
        with self.assertRaisesRegex(ValueError, 'never object merging'): self.verify()

    def test_extra_external_and_noncode_storage_rejected(self):
        coff_file(self.alias_obj, WORD_CODE, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0), ('_other', 2, 0, 0)])
        with self.assertRaisesRegex(ValueError, 'external symbol inventory'): self.verify()
        coff_file(self.alias_obj, WORD_CODE, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0)], data=b'\1')
        with self.assertRaisesRegex(ValueError, 'non-code'): self.verify()

    def test_missing_adapter_cannot_use_owner_alone(self):
        with self.assertRaisesRegex(ValueError, 'freshly compiled rename object'):
            masked.relocated_compare(self.obj, '_sample', self.target, 0x1000, 8,
                                     dict(symbols={'_cell': self.record}), storage_owners={'_cell': self.owner_obj})

    def test_alias_header_race_during_owner_compile_rejected(self):
        original = self.compile
        def raced(source, flags, toolchain):
            if Path(source) == self.owner_source: self.alias_header.write_text('/* raced */\n')
            return original(source, flags, toolchain)
        self.compiler.side_effect = raced
        with self.assertRaisesRegex(ValueError, 'changed'): self.verify()

    def test_generated_wrapper_race_rejected(self):
        original = self.compile
        def raced(source, flags, toolchain):
            result = original(source, flags, toolchain)
            if Path(source) not in (self.owner_source, self.source): Path(source).write_text('/* raced */\n')
            return result
        self.compiler.side_effect = raced
        with self.assertRaisesRegex(ValueError, 'changed'): self.verify()

    def publication_race(self, operation, message):
        original = promote.verify_rows
        def raced(*args, **kwargs):
            result = original(*args, **kwargs); operation(); return result
        before = self.state.read_bytes()
        with patch.object(promote, 'verify_rows', side_effect=raced):
            with self.assertRaisesRegex(ValueError, message):
                promote.promote_relocated(['cell-reader'], self.bindings)
        self.assertEqual(before, self.state.read_bytes())

    def test_alias_object_race_before_publication_rejected(self):
        def mutate():
            data = bytearray(self.alias_obj.read_bytes()); data[4] ^= 1; self.alias_obj.write_bytes(data)
        self.publication_race(mutate, 'Compiler-renaming object changed')

    def test_alias_header_race_before_publication_rejected(self):
        self.publication_race(lambda: self.alias_header.write_text('/* raced */\n'), 'Compiler-renaming input changed')

    def test_alias_object_race_during_original_compile_rejected(self):
        original = self.compile
        def raced(source, flags, toolchain):
            result = original(source, flags, toolchain)
            if Path(source) == self.source:
                data = bytearray(self.alias_obj.read_bytes()); data[4] ^= 1; self.alias_obj.write_bytes(data)
            return result
        self.compiler.side_effect = raced
        with self.assertRaisesRegex(ValueError, 'Object changed'): self.verify()

    def test_function_symbol_cannot_be_renamed_as_data(self):
        coff_file(self.alias_obj, WORD_CODE, [(3, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0x20)])
        with self.assertRaisesRegex(ValueError, 'undefined strong data'): self.verify()

    def test_same_zero_neighbor_still_has_no_identity(self):
        self.record['rva'] += 2; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'another storage cell'): self.verify()

    def test_wider_owner_is_not_a_word_prefix(self):
        fixture_owner(self.owner_obj, symbols=[('_owned', 4, 0, 0)])
        with self.assertRaisesRegex(ValueError, 'exact-size COMMON'): self.verify()

    def test_wider_native_access_remains_rejected(self):
        code = b'\xa1\0\0\0\0\xc3'
        data = bytearray(self.target.read_bytes())
        data[0x400:0x406] = b'\xa1' + struct.pack('<I', 0x402200) + b'\xc3'
        struct.pack_into('<H', data, 0x808, 0x3001); self.target.write_bytes(data)
        coff_file(self.obj, code, [(1, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_cell', 0, 0, 0)])
        coff_file(self.alias_obj, code, [(1, 1, masked.DIR32)],
                  [('_sample', 0, 1, 0x20), ('_owned', 0, 0, 0)])
        self.record['references'] = [dict(offset=1, type=masked.DIR32, addend=0)]
        self.row.update(size=6, target_sha256=match.digest(self.target.read_bytes())); self.write_bindings()
        with self.assertRaises(ValueError): self.verify()


class AliasWholeObjectTests(unittest.TestCase):
    def objects(self):
        from test_reprove import FakeObject
        a = FakeObject()
        for item in a.symbols.values():
            item.setdefault('storage', 2); item.setdefault('value', 0)
        a.symbols[2].update(value=0)
        a.sections[0]['relocations'] = [dict(offset=0, type=6, symbol='_cell')]
        b = copy.deepcopy(a); b.symbols[2]['name'] = '_owned'
        b.sections[0]['relocations'][0]['symbol'] = '_owned'
        return a, b

    def compare(self, a, b):
        with patch.object(storage_alias, 'COFF', side_effect=[a, b]):
            return storage_alias.verify_alias_object('before', 'after', '_accepted', {'_cell': '_owned'})

    def test_entire_object_equivalence(self):
        self.assertEqual(self.compare(*self.objects())['functions'], 2)

    def test_other_function_byte_change_rejected(self):
        a, b = self.objects(); b.sections[1]['data'] = b'\x90'
        with self.assertRaisesRegex(ValueError, 'complete function'): self.compare(a, b)

    def test_other_function_added_rejected(self):
        a, b = self.objects(); b.symbols[3] = dict(name='_extra', section=2, type=0x20, storage=2, value=0)
        with self.assertRaisesRegex(ValueError, 'complete function'): self.compare(a, b)

    def test_changed_linker_directives_rejected(self):
        a, b = self.objects(); b.sections[2]['name'] = '.drectve'
        with self.assertRaisesRegex(ValueError, 'non-code'): self.compare(a, b)

    def test_preexisting_owner_name_cannot_be_coalesced(self):
        a, b = self.objects(); a.symbols[3] = dict(name='_owned', section=0, value=0, storage=2, type=0)
        with self.assertRaisesRegex(ValueError, 'pre-existing'): self.compare(a, b)

    def test_unclassified_executable_storage_cannot_be_added(self):
        a, b = self.objects()
        b.sections.append(dict(name='.text$x', flags=0x60001020, size=1, data=b'\x90', relocations=[]))
        b.symbols[3] = dict(name='_hidden', section=4, type=0, storage=3, value=0)
        with self.assertRaisesRegex(ValueError, 'unclassified executable'): self.compare(a, b)


if __name__ == '__main__': unittest.main()
