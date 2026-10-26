"""Synthetic positive and adversarial controls for opt-in proof forms.

These files are parsed but never linked or executed. Native campaign receipts
are separate, so the unit tests do not require original game files or CL.
"""
import copy
import hashlib
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import masked
from coff import COFF
from coff_identity import verify_pair
from pe import PE
from zero_compare_anchor import verify_native
from zero_storage import verify_native as legacy_native
from test_matching import make_pe, workspace_temporary_directory
from test_relocated import binding, coff_file
from test_zero_storage import fixture_pe, fixture_owner, fixture_caller, fixture_zero_section


def object_file(path, sections, symbols):
    """COFF fixture with auxiliary entries, long names and per-section fixups."""
    offset = 20 + 40 * len(sections)
    headers, payload, entries, strings = bytearray(), bytearray(), bytearray(), bytearray()
    for name, raw, flags, uses in sections:
        rels = b''.join(struct.pack('<IIH', *r) for r in uses)
        headers += struct.pack('<8sIIIIIIHHI', name.encode(), 0, 0, len(raw), offset,
                               offset + len(raw) if rels else 0, 0, len(uses), 0, flags)
        payload += raw + rels
        offset += len(raw) + len(rels)
    for name, value, section, typ, storage, aux in symbols:
        encoded = name.encode()
        if len(encoded) > 8:
            encoded = struct.pack('<II', 0, 4 + len(strings)); strings += name.encode() + b'\0'
        entries += struct.pack('<8sIhHBB', encoded, value, section, typ, storage, len(aux)) + b''.join(aux)
    path.write_bytes(struct.pack('<HHIIIHH', 0x14c, len(sections), 0, offset, len(entries)//18, 0, 0) +
                     headers + payload + entries + struct.pack('<I', 4 + len(strings)) + strings)


class PayloadTests(unittest.TestCase):
    def setUp(self):
        d = workspace_temporary_directory('payload-form-'); self.addCleanup(d.cleanup)
        self.obj, self.target = [Path(d.name) / n for n in ('caller.obj', 'native.dll')]
        make_pe(self.target)
        raw = bytearray(self.target.read_bytes())
        raw[0x400:0x406] = b'\xb8' + struct.pack('<I', 0x402000) + b'\xc3'
        raw[0x600:0x608] = struct.pack('<II', 0x401100, 0x1234)
        self.target.write_bytes(raw)
        self.sections = [('.text', b'\xb8\0\0\0\0\xc3', 0x60501020, [(1, 1, 6)]),
                         ('.data', struct.pack('<II', 0, 0x1234), 0xc0300040, [(0, 2, 6)])]
        self.symbols = [('_sample', 0, 1, 32, 2, []), ('_data', 0, 2, 0, 2, []),
                        ('_callee', 0, 0, 32, 2, [])]
        self.record = binding('data', 0x2000, 8, (1, 6, 0),
            sha256=hashlib.sha256(raw[0x600:0x608]).hexdigest(), data_profile='relocated-data-v1',
            payload_bindings=dict(symbols={'_callee': binding('function', 0x1100, 1, (0, 6, 0))}))

    def compare(self):
        object_file(self.obj, self.sections, self.symbols)
        return masked.relocated_compare(self.obj, '_sample', self.target, 0x1000, 6,
                                         dict(symbols={'_data': self.record}))

    def test_complete_payload_and_pointer_without_pe_fixups(self):
        result = self.compare()
        self.assertTrue(result['relocated_equal'])
        self.assertEqual(result['data_checks'][0]['relocations'], 1)

    def test_profile_is_explicit_and_unknown_profiles_reject(self):
        for value in (None, 'unknown'):
            with self.subTest(value=value):
                self.record['data_profile'] = value
                with self.assertRaises(ValueError): self.compare()
        del self.record['data_profile']
        with self.assertRaisesRegex(ValueError, 'separate verification'): self.compare()

    def test_wrong_bound_pointer_does_not_pass_masked_equality(self):
        self.record['payload_bindings']['symbols']['_callee']['rva'] += 1
        result = self.compare()
        self.assertFalse(result['relocated_equal'])
        self.assertTrue(any('destination differs' in e for e in result['errors']))

    def test_every_nonpointer_byte_and_payload_hash_is_checked(self):
        for off in (4, 7):
            original = self.sections[1]
            raw = bytearray(original[1]); raw[off] ^= 1
            self.sections[1] = (original[0], bytes(raw), original[2], original[3])
            self.assertFalse(self.compare()['relocated_equal'])
            self.sections[1] = original
        self.record['sha256'] = '0'*64
        self.assertFalse(self.compare()['relocated_equal'])

    def test_missing_extra_changed_and_duplicate_uses_reject(self):
        baseline = copy.deepcopy(self.record)
        changes = [lambda b:b['payload_bindings']['symbols'].clear(),
                   lambda b:b['payload_bindings']['symbols']['_callee'].update(evidence=[]),
                   lambda b:b['payload_bindings']['symbols']['_callee']['references'][0].update(addend=1),
                   lambda b:b['payload_bindings']['symbols']['_callee']['references'].append(dict(offset=0,type=6,addend=0)),
                   lambda b:b['payload_bindings']['symbols']['_callee']['references'][0].update(offset=4)]
        for mutate in changes:
            self.record = copy.deepcopy(baseline); mutate(self.record)
            with self.assertRaises(ValueError): self.compare()

    def test_overlapping_partial_and_unsupported_fixups_reject(self):
        for uses in [[(0,2,6),(0,2,6)], [(6,2,6)], [(0,2,20)]]:
            self.sections[1] = ('.data', bytes(8), 0xc0300040, uses)
            with self.assertRaises(ValueError): self.compare()

    def test_prefix_extent_and_aliases_reject(self):
        self.sections[1] = ('.data', self.sections[1][1] + bytes(4), 0xc0300040, [(0,2,6)])
        with self.assertRaisesRegex(ValueError, 'Complete initialized allocation'): self.compare()
        self.sections[1] = ('.data', self.sections[1][1][:8], 0xc0300040, [(0,2,6)])
        self.symbols.append(('_alias',0,2,0,2,[]))
        with self.assertRaisesRegex(ValueError, 'Ambiguous'): self.compare()

    def test_native_payload_fixups_must_be_exact_when_present(self):
        raw = bytearray(self.target.read_bytes())
        struct.pack_into('<II',raw,0x98+96+5*8,0x3000,24)
        struct.pack_into('<IIHH',raw,0x800,0x1000,12,0x3001,0)
        struct.pack_into('<IIHH',raw,0x80c,0x2000,12,0x3000,0)
        self.target.write_bytes(raw)
        self.assertTrue(self.compare()['relocated_equal'])
        struct.pack_into('<H',raw,0x814,0x3001); self.target.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'Payload HIGHLOW'): self.compare()

    def test_nested_defined_string_payload_is_compared(self):
        self.symbols[2] = ('_string',0,3,0,3,[])
        self.sections.append(('.rdata',b'hello\0',0x40301040,[]))
        raw = bytearray(self.target.read_bytes())
        raw[0x610:0x616] = b'hello\0'; struct.pack_into('<I',raw,0x600,0x402010)
        self.target.write_bytes(raw)
        self.record['sha256'] = hashlib.sha256(raw[0x600:0x608]).hexdigest()
        self.record['payload_bindings']['symbols'] = {'_string':binding('data',0x2010,6,(0,6,0),sha256=hashlib.sha256(b'hello\0').hexdigest())}
        self.assertTrue(self.compare()['relocated_equal'])
        self.sections[2] = ('.rdata',b'jello\0',0x40301040,[])
        self.assertFalse(self.compare()['relocated_equal'])

    def test_nested_relocated_object_and_cycle_controls(self):
        self.symbols[2] = ('_child',0,3,0,2,[])
        self.symbols.append(('_callee',0,0,32,2,[]))
        self.sections.append(('.data',bytes(4),0xc0300040,[(0,3,6)]))
        raw=bytearray(self.target.read_bytes())
        struct.pack_into('<I',raw,0x600,0x402010);struct.pack_into('<I',raw,0x610,0x401100)
        self.target.write_bytes(raw)
        self.record['sha256']=hashlib.sha256(raw[0x600:0x608]).hexdigest()
        child=binding('data',0x2010,4,(0,6,0),sha256=hashlib.sha256(raw[0x610:0x614]).hexdigest(),
                      data_profile='relocated-data-v1',payload_bindings=dict(symbols={'_callee':binding('function',0x1100,1,(0,6,0))}))
        self.record['payload_bindings']['symbols']={'_child':child}
        self.assertTrue(self.compare()['relocated_equal'])
        self.sections[2]=('.data',bytes(4),0xc0300040,[(0,1,6)])
        struct.pack_into('<I',raw,0x610,0x402000);self.target.write_bytes(raw)
        child['sha256']=hashlib.sha256(raw[0x610:0x614]).hexdigest()
        child['payload_bindings']['symbols']={'_data':copy.deepcopy(self.record)}
        child['payload_bindings']['symbols']['_data']['references']=[dict(offset=0,type=6,addend=0)]
        with self.assertRaisesRegex(ValueError,'Cyclic'):self.compare()

    def test_payload_self_function_cannot_move_the_compared_code(self):
        self.sections[1]=('.data',struct.pack('<II',0,0x1234),0xc0300040,[(0,0,6)])
        raw=bytearray(self.target.read_bytes());struct.pack_into('<I',raw,0x600,0x401100);self.target.write_bytes(raw)
        self.record['payload_bindings']['symbols']={'_sample':binding('function',0x1100,6,(0,6,0))}
        with self.assertRaisesRegex(ValueError,'Inconsistent'):self.compare()

    def test_payload_import_is_bound_to_the_actual_iat(self):
        raw=bytearray(self.target.read_bytes())
        struct.pack_into('<II',raw,0x98+96+8,0x2040,40)
        struct.pack_into('<IIIII',raw,0x640,0x2080,0,0,0x20c0,0x20a0)
        struct.pack_into('<II',raw,0x680,0x20d0,0);struct.pack_into('<II',raw,0x6a0,0x20d0,0)
        raw[0x6c0:0x6cd]=b'KERNEL32.dll\0';raw[0x6d0:0x6da]=b'\0\0GetTick\0'
        struct.pack_into('<I',raw,0x600,0x4020a0);self.target.write_bytes(raw)
        self.record['sha256']=hashlib.sha256(raw[0x600:0x608]).hexdigest()
        self.symbols[2]=('_iat',0,0,0,2,[])
        imp=binding('import',0x20a0,4,(0,6,0),dll='kernel32.dll',name='GetTick')
        self.record['payload_bindings']['symbols']={'_iat':imp}
        self.assertTrue(self.compare()['relocated_equal'])
        imp['name']='AnotherApi'
        with self.assertRaisesRegex(ValueError,'IAT'):self.compare()


class DuplicateIdentityTests(unittest.TestCase):
    def setUp(self):
        d=workspace_temporary_directory('duplicate-form-'); self.addCleanup(d.cleanup)
        self.obj,self.target=[Path(d.name)/n for n in ('pair.obj','native.dll')]
        make_pe(self.target)
        self.name='??_GFixture'; self.proof=dict(profile='msvc-undef-comdat-v1',symbol=self.name)
        self.aux=bytearray(18); struct.pack_into('<I',self.aux,0,6);struct.pack_into('<H',self.aux,4,1);self.aux[14]=2
        self.symbols=[(self.name,0,0,32,2,[]),(self.name,0,1,32,2,[]),('.text',0,1,0,3,[bytes(self.aux)])]
        self.sections=[('.text',b'\xe8\0\0\0\0\xc3',0x60501020,[(1,0,20)])]
        raw=bytearray(self.target.read_bytes());raw[0x400:0x406]=b'\xe8'+struct.pack('<i',-5)+b'\xc3';self.target.write_bytes(raw)

    def compare(self, profile=True):
        object_file(self.obj,self.sections,self.symbols)
        bindings=dict(symbols={self.name:binding('function',0x1000,6,(1,20,0))})
        if profile:bindings['coff_identity']=self.proof
        return masked.relocated_compare(self.obj,self.name,self.target,0x1000,6,bindings)

    def test_both_symbol_indices_are_one_bound_identity(self):
        for index in (0,1):
            self.sections[0]=('.text',b'\xe8\0\0\0\0\xc3',0x60501020,[(1,index,20)])
            self.assertTrue(self.compare()['relocated_equal'])

    def test_opt_in_and_exact_shape_are_required(self):
        with self.assertRaisesRegex(ValueError,'duplicate COFF'):self.compare(False)
        original=copy.deepcopy(self.symbols)
        for symbols in [original+[original[0]], [original[1],original[1],original[2]],
                        [(self.name,4,0,32,2,[]),*original[1:]],
                        [(self.name,0,0,32,105,[]),*original[1:]]]:
            self.symbols=symbols
            with self.assertRaises(ValueError):self.compare()

    def test_comdat_selection_association_and_profile_cannot_change(self):
        for offset,value in [(14,1),(14,5),(12,1)]:
            aux=bytearray(self.aux);aux[offset]=value
            self.symbols[2]=('.text',0,1,0,3,[bytes(aux)])
            with self.assertRaises(ValueError):self.compare()
        self.symbols[2]=('.text',0,1,0,3,[bytes(self.aux)])
        self.proof['profile']='unknown'
        with self.assertRaises(ValueError):self.compare()


class CompareAnchorTests(unittest.TestCase):
    def setUp(self):
        d=workspace_temporary_directory('compare-anchor-');self.addCleanup(d.cleanup)
        self.target,self.obj,self.owner=[Path(d.name)/n for n in ('native.dll','caller.obj','owner.obj')]
        fixture_pe(self.target);fixture_caller(self.obj);fixture_owner(self.owner)
        self.anchor(bytes.fromhex('66833d')+struct.pack('<I',0x402200)+b'\x05')

    def anchor(self,code,operand=0,field=3):
        raw=bytearray(self.target.read_bytes());raw[0x500:0x510]=b'\xcc'*16;raw[0x500:0x500+len(code)]=code
        struct.pack_into('<H',raw,0x80a,0x3100+field);self.target.write_bytes(raw)
        self.identity=dict(rva=0x1100,size=len(code),operand=operand,sha256=hashlib.sha256(code).hexdigest())

    def check(self):return verify_native(PE(self.target),0x2200,2,self.identity,0x1000,6)

    def test_word_cmp_and_test_absolute_memory_are_direct_anchors(self):
        self.check()
        self.anchor(bytes.fromhex('66f705')+struct.pack('<I',0x402200)+b'\x01\0');self.check()
        with self.assertRaisesRegex(ValueError,'Unsupported'):legacy_native(PE(self.target),0x2200,2,self.identity,0x1000,6)

    def test_only_explicit_profile_wires_extension(self):
        # Caller reads a WORD, matching the independent owner's exact size.
        fixture_caller(self.obj,b'\x66\xa1\0\0\0\0\xc3',2);fixture_owner(self.owner,2)
        raw=bytearray(self.target.read_bytes());raw[0x400:0x407]=b'\x66\xa1'+struct.pack('<I',0x402200)+b'\xc3'
        struct.pack_into('<H',raw,0x808,0x3002);self.target.write_bytes(raw)
        rec=binding('bss',0x2200,2,(2,6,0),identity=self.identity,owner=dict(symbol='_cell'),anchor_profile='cmp-test-absolute-v1')
        def compare():return masked.relocated_compare(self.obj,'_sample',self.target,0x1000,7,dict(symbols={'_cell':rec}),storage_owners={'_cell':self.owner})
        self.assertTrue(compare()['relocated_equal'])
        del rec['anchor_profile']
        with self.assertRaisesRegex(ValueError,'Unsupported'):compare()

    def test_immediate_indexed_segment_and_oversized_operands_reject(self):
        for code,operand,field in [(b'\x3d'+struct.pack('<I',0x402200),0,1),
                                  (b'\x83\x3d'+struct.pack('<I',0x402200)+b'\x05',0,2),
                                  (b'\x66\x83\x3c\x85'+struct.pack('<I',0x402200)+b'\x05',0,4),
                                  (b'\x64\x66\x83\x3d'+struct.pack('<I',0x402200)+b'\x05',0,4)]:
            self.anchor(code,operand,field)
            with self.assertRaises(ValueError):self.check()

    def test_prefix_hash_wrong_cell_self_witness_and_fixup_reject(self):
        original=copy.deepcopy(self.identity)
        for changes in [dict(size=7),dict(sha256='0'*64),dict(rva=0x1000),dict(operand=1)]:
            self.identity=dict(original,**changes)
            with self.assertRaises(ValueError):self.check()
        self.identity=original
        raw=bytearray(self.target.read_bytes());struct.pack_into('<H',raw,0x80a,0x3104);self.target.write_bytes(raw)
        with self.assertRaisesRegex(ValueError,'HIGHLOW'):self.check()


class SameUnitBssRejectionTests(unittest.TestCase):
    def test_section_defined_caller_remains_closed(self):
        d=workspace_temporary_directory('same-unit-closed-');self.addCleanup(d.cleanup)
        target,obj,owner=[Path(d.name)/n for n in ('native.dll','caller.obj','owner.obj')]
        identity=fixture_pe(target);fixture_owner(owner)
        coff_file(obj,b'\xa1\0\0\0\0\xc3',[(1,1,6)],
                  [('_sample',0,1,32),('_cell',0,2,0)],data=bytes(4))
        raw=bytearray(obj.read_bytes());raw[60:68]=b'.bss\0\0\0\0'
        struct.pack_into('<I',raw,80,0);struct.pack_into('<I',raw,96,0xc0300080);obj.write_bytes(raw)
        record=binding('bss',0x2200,4,(1,6,0),identity=identity,owner=dict(symbol='_cell'),owner_storage='same-unit-bss-v1')
        with self.assertRaisesRegex(ValueError,'undefined or exact-size COMMON'):
            masked.relocated_compare(obj,'_sample',target,0x1000,6,dict(symbols={'_cell':record}),storage_owners={'_cell':owner})


if __name__=='__main__':unittest.main()
