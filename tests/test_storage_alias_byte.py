"""compile-renaming-byte-v1: opt-in one-byte compile-renaming aliases (proposed tests/test_storage_alias_byte.py).

Must run with the patched match.py / fixed_disp32.py loaded (run_tests.py preloads the shadows).
Synthetic end-to-end cases use the existing WORD-alias harness; real-object cases use the
fresh ENG1/ENG3 objects frozen in fixtures/ by the diagnostic run (no compiler calls here).
"""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / 'tests'))
sys.path.insert(0, str(ROOT / 'scripts'))
import masked
import match
import storage_alias
import storage_alias_byte as sab
from test_relocated import coff_file
import test_storage_alias as word_tests

BYTE_CODE = b'\x0f\xbe\x05\0\0\0\0\xc3'  # movsx eax, byte ptr [cell]; ret
SYMBOLS = [('_sample', 0, 1, 0x20)]


class ByteAliasAcceptanceTests(unittest.TestCase):
    """The pinned WORD harness, retargeted to one signed byte."""
    setUp_word = word_tests.AliasAcceptanceTests.setUp
    compile = word_tests.AliasAcceptanceTests.compile
    record = word_tests.AliasAcceptanceTests.record
    write_bindings = word_tests.AliasAcceptanceTests.write_bindings
    verify = word_tests.AliasAcceptanceTests.verify

    def setUp(self):
        self.setUp_word()
        self.alias_header.write_text('/* proof-only */\n#define cell owned\nextern signed char owned;\n')
        self.source.write_text('extern signed char cell; int sample(void){return cell;}\n')
        self.owner_source.write_text('#include "owner.h"\nsigned char owned;\n')
        data = bytearray(self.target.read_bytes())
        data[0x400:0x408] = BYTE_CODE[:3] + struct.pack('<I', 0x402200) + b'\xc3'
        self.target.write_bytes(data)
        self.code(BYTE_CODE)
        word_tests.fixture_owner(self.owner_obj, symbols=[('_owned', 1, 0, 0)])
        self.record.update(size=1, alias_profile=sab.PROFILE)
        self.document['evidence_files'] = {p.relative_to(self.root).as_posix(): match.digest(p.read_bytes())
                                          for p in [self.owner_source, self.header, self.note, self.alias_header]}
        self.write_bindings()
        self.row.update(target_sha256=match.digest(self.target.read_bytes()))
        self.row['proof']['source_sha256'] = match.digest(self.source.read_bytes())
        self.state.write_text(json.dumps(dict(functions=[self.row])) + '\n')

    def code(self, code, offset=3, caller=True, adapted=True, extra=()):
        code = code + bytes([0x90]) * (len(BYTE_CODE) - len(code))  # keep the complete 8-byte extent
        if caller:
            coff_file(self.obj, code, [(offset, 1, masked.DIR32)], SYMBOLS + [('_cell', 0, 0, 0)] + list(extra))
        if adapted:
            coff_file(self.alias_obj, code, [(offset, 1, masked.DIR32)], SYMBOLS + [('_owned', 0, 0, 0)] + list(extra))

    def set_header(self, text):
        self.alias_header.write_text(text)
        self.document['evidence_files']['src/alias.h'] = match.digest(self.alias_header.read_bytes())
        self.write_bindings()

    # ---- positive ----
    def test_byte_alias_exact_through_patched_match(self):
        result, stats = self.verify()
        self.assertTrue(result[0]['exact'])
        self.assertEqual(stats['compiler_calls'], 3)
        check = result[0]['source_alias_checks']['_cell']
        self.assertEqual((check['profile'], check['size'], check['functions']), (sab.PROFILE, 1, 1))
        self.assertEqual(check['uses'], [dict(function='_sample', offset=3, mnemonic='movsx', width=1, addend=0)])
        self.assertIn('_cell', result[0]['storage_alias_objects'])

    def test_adapter_compiled_with_wx_and_caller_unchanged(self):
        seen = []
        original = self.compiler.side_effect
        def record_flags(source, flags, toolchain):
            seen.append((Path(source).name, tuple(flags)))
            return original(source, flags, toolchain)
        self.compiler.side_effect = record_flags
        self.verify()
        self.assertIn(('caller.c', ()), seen)
        self.assertTrue(any(name.startswith('alias-') and flags == ('/WX',) for name, flags in seen), seen)

    # ---- the pinned path still rejects bytes ----
    def test_without_opt_in_pinned_path_rejects(self):
        del self.record['alias_profile']; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'WORD scalars'): self.verify()
        self.assertEqual(self.compiler.call_count, 0)

    def test_pinned_masked_comparator_rejects_opted_in_record(self):
        binding = copy.deepcopy(self.document['functions']['cell-reader'])
        with self.assertRaisesRegex(ValueError, 'WORD scalars'):
            masked.relocated_compare(self.obj, '_sample', self.target, 0x1000, 8, binding,
                                     storage_owners={'_cell': self.owner_obj}, storage_aliases={'_cell': self.alias_obj})

    def test_unknown_alias_profile_rejected(self):
        self.record['alias_profile'] = 'compile-renaming-byte-v2'; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'Unsupported explicit alias profile'): self.verify()

    # ---- record shape ----
    def test_wrong_size_rejected(self):
        for size in (2, 4, 1.0, True):
            with self.subTest(size=size):
                self.record['size'] = size; self.write_bindings()
                with self.assertRaisesRegex(ValueError, 'one-byte'): self.verify()
        self.assertEqual(self.compiler.call_count, 0)

    def test_wrong_owner_name_rejected(self):
        self.record['owner']['symbol'] = '_elsewhere'; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'disagrees'): self.verify()
        self.record['owner']['symbol'] = '_cell'; self.write_bindings()
        with self.assertRaises(ValueError): self.verify()

    def test_chained_duplicate_and_multiple_renames_rejected(self):
        for mapping in [{'cell': 'owned', 'owned': 'other'}, {'cell': 'owned', 'other': 'owned'},
                        {'cell': 'owned', 'x': 'y'}, {'cell': 'cell'}, {'cell': 'owned /Dx'}, {}]:
            with self.subTest(mapping=mapping):
                self.record['source_alias']['renames'] = mapping; self.write_bindings()
                with self.assertRaises(ValueError): self.verify()
        self.assertEqual(self.compiler.call_count, 0)

    def test_other_alias_kinds_and_profiles_rejected(self):
        for key, value in [('kind', 'owner-compile-renaming'), ('kind', 'compile-renaming-byte')]:
            with self.subTest(value=value):
                self.record['source_alias'][key] = value; self.write_bindings()
                with self.assertRaises(ValueError): self.verify()
        self.record['source_alias']['kind'] = 'compile-renaming'
        for key, value in [('access_profile', 'loader-zero-indexed-v1'), ('anchor_profile', 'loader-zero-paired-v1'),
                           ('owner_storage', 'zero-section-v1')]:
            with self.subTest(key=key):
                record = copy.deepcopy(self.record); record[key] = value
                with self.assertRaisesRegex(ValueError, 'unchanged COMMON owner'):
                    sab.alias_names(record, '_cell')

    def test_cpp_owner_profile_rejected(self):
        self.record['owner']['flags'] = ['/Tp']; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'C owner profile'): self.verify()

    def test_non_bss_record_cannot_carry_profile(self):
        record = dict(copy.deepcopy(self.record), kind='data')
        with self.assertRaisesRegex(ValueError, 'one-byte bss'):
            sab.alias_names(record, '_cell')

    # ---- header ----
    def test_header_mismatches_rejected(self):
        for text in ['#define cell owned\n',                                      # no declaration
                     '#define cell owned\nextern short owned;\n',                 # not a byte type
                     '#define cell other\nextern signed char owned;\n',           # wrong macro
                     '#define cell owned\nextern signed char other;\n',           # wrong declared owner
                     '#define cell owned\n#define x y\nextern signed char owned;\n',
                     '#define cell owned\nextern signed char owned;\nextern signed char owned;\n',
                     '#include "x.h"\n#define cell owned\nextern signed char owned;\n',
                     '#define cell owned // c++ comment\nextern signed char owned;\n',
                     '#define cell \\\nowned\nextern signed char owned;\n']:
            with self.subTest(text=text):
                self.set_header(text)
                with self.assertRaisesRegex(ValueError, 'header'): self.verify()
        self.assertEqual(self.compiler.call_count, 0)

    def test_noncanonical_and_unpinned_header_rejected(self):
        header = self.root / 'work/alias.h'; header.write_bytes(self.alias_header.read_bytes())
        self.record['source_alias']['header'] = 'work/alias.h'; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'canonical source'): self.verify()
        self.record['source_alias']['header'] = 'src/alias.h'
        del self.document['evidence_files']['src/alias.h']; self.write_bindings()
        with self.assertRaisesRegex(ValueError, 'independently pinned'): self.verify()

    # ---- uses ----
    def test_non_byte_access_rejected(self):
        self.code(b'\x0f\xbf\x05\0\0\0\0\xc3')  # movsx eax, word ptr
        with self.assertRaisesRegex(ValueError, 'one-byte memory operand'): self.verify()
        self.code(b'\x8b\x05\0\0\0\0\xc3', offset=2)  # mov eax, dword ptr
        with self.assertRaisesRegex(ValueError, 'one-byte memory operand'): self.verify()

    def test_address_escape_rejected(self):
        self.code(b'\xb8\0\0\0\0\xc3', offset=1)  # mov eax, offset cell
        with self.assertRaisesRegex(ValueError, 'direct absolute memory displacement'): self.verify()
        self.code(b'\x8d\x05\0\0\0\0\xc3', offset=2)  # lea eax, [cell]
        with self.assertRaisesRegex(ValueError, 'one-byte memory operand'): self.verify()
        self.code(b'\x68\0\0\0\0\xc3', offset=1)  # push offset cell
        with self.assertRaisesRegex(ValueError, 'direct absolute memory displacement'): self.verify()

    def test_nonzero_addend_rejected(self):
        self.code(BYTE_CODE[:3] + struct.pack('<I', 1) + b'\xc3')
        with self.assertRaisesRegex(ValueError, 'addend 0'): self.verify()

    def test_indexed_byte_access_rejected(self):
        self.code(b'\x0f\xbe\x81\0\0\0\0\xc3')  # movsx eax, byte ptr [ecx+cell]
        with self.assertRaisesRegex(ValueError, 'one-byte memory operand'): self.verify()

    def test_changed_adapted_code_rejected(self):
        self.code(b'\x0f\xb6\x05\0\0\0\0\xc3', caller=False)  # movzx in the adapted object only
        with self.assertRaisesRegex(ValueError, 'complete function'): self.verify()

    def test_wider_owner_rejected(self):
        word_tests.fixture_owner(self.owner_obj, symbols=[('_owned', 2, 0, 0)])
        with self.assertRaisesRegex(ValueError, 'exact-size COMMON'): self.verify()

    def test_missing_adapter_rejected(self):
        with self.assertRaisesRegex(ValueError, 'freshly compiled rename object'):
            sab.verify_record(self.obj, None, '_sample', '_cell', self.record)


FIXTURES = ROOT / 'work/velocity_i/byteAlias/fixtures'  # diagnostic objects; skipped when absent


def real_record(module):
    owner = {'ENG1': '_g_active_category_70cfe', 'ENG3': '_g_eng3_layer_40638'}[module]
    return dict(kind='bss', size=1, alias_profile=sab.PROFILE, owner=dict(source='x.c', flags=['/O2'], symbol=owner),
                source_alias=dict(kind='compile-renaming', header='src/x.h', renames={'g_special_kind': owner[1:]}))


@unittest.skipUnless((FIXTURES / 'manifest.json').is_file(), 'diagnostic fixtures absent')
class RealObjectTests(unittest.TestCase):
    def setUp(self):
        for name, sha in json.loads((FIXTURES / 'manifest.json').read_text()).items():
            self.assertEqual(hashlib.sha256((FIXTURES / name).read_bytes()).hexdigest(), sha, name)

    def test_real_eng1_and_eng3_objects(self):
        for unit, symbol, uses in (('remove', '_remove_actor_variant', 4), ('release', '_release_actor', 2)):
            for module in ('eng1', 'eng3'):
                with self.subTest(unit=unit, module=module):
                    check = sab.verify_record(FIXTURES / (unit + '_caller.obj'), FIXTURES / ('%s_%s_adapted.obj' % (unit, module)),
                                              symbol, '_g_special_kind', real_record(module.upper()))
                    self.assertEqual(len(check['uses']), uses)
                    self.assertTrue(all(u['width'] == 1 and u['addend'] == 0 for u in check['uses']))

    def test_real_wrong_owner_adapted_object_rejected(self):
        with self.assertRaisesRegex(ValueError, 'missing or ambiguous'):
            sab.verify_record(FIXTURES / 'remove_caller.obj', FIXTURES / 'remove_eng3_adapted.obj',
                              '_remove_actor_variant', '_g_special_kind', real_record('ENG1'))

    def test_real_wrong_unit_rejected(self):
        with self.assertRaises(ValueError):
            sab.verify_record(FIXTURES / 'release_caller.obj', FIXTURES / 'remove_eng1_adapted.obj',
                              '_release_actor', '_g_special_kind', real_record('ENG1'))

    def test_real_owner_is_exact_one_byte_common(self):
        from zero_storage import verify_common
        verify_common(FIXTURES / 'eng1_owner_special.obj', '_g_active_category_70cfe', 1)
        verify_common(FIXTURES / 'eng3_owner_special.obj', '_g_eng3_layer_40638', 1)
        with self.assertRaises(ValueError):
            verify_common(FIXTURES / 'eng1_owner_special.obj', '_g_active_category_70cfe', 2)


class PinnedModulesUnchanged(unittest.TestCase):
    def test_pinned_modules_still_reject_bytes(self):
        record = real_record('ENG1')
        with self.assertRaisesRegex(ValueError, 'WORD scalars'):
            storage_alias.alias_names(dict(record, alias_profile=None), '_g_special_kind')


if __name__ == '__main__':
    unittest.main()
