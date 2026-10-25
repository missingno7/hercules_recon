"""Adversarial rooted-callback proofs over synthetic, unexecuted PE/COFF only."""
import copy,hashlib,json,struct,sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
import match,masked,promote
from test_matching import workspace_temporary_directory
from test_relocated import binding,coff_file
from test_zero_storage import fixture_pe,fixture_owner
import test_zero_storage as zero


def callback_code(stride=8,reload=False):
    root=bytes.fromhex('8b150000000081e3ff01000057')
    return root+(bytes.fromhex('8d0c5bff148a') if stride==12 else bytes.fromhex('ff14da'))+(bytes.fromhex('8b06') if reload else b'')+bytes.fromhex('83c404')


def fixture(target,caller,owner,stride=8,reload=False,prefix=b''):
    fixture_pe(target);fixture_owner(owner)
    window=callback_code(stride,reload);native=bytearray(window);struct.pack_into('<I',native,2,0x402200)
    data=bytearray(target.read_bytes());data[0x400:0x400+len(prefix)+len(window)+1]=prefix+native+b'\xc3';data[0x500:0x500+len(window)]=native
    anchor=b'\x68'+struct.pack('<I',0x402200);data[0x580:0x585]=anchor
    struct.pack_into('<II',data,0x98+96+5*8,0x3000,16)
    struct.pack_into('<IIHHHH',data,0x800,0x1000,16,0x3002+len(prefix),0x3102,0x3181,0);target.write_bytes(data)
    code=prefix+window+b'\xc3';coff_file(caller,code,[(2+len(prefix),1,6)],[('_sample',0,1,0x20),('_cell',0,0,0)])
    root=binding('bss',0x2200,4,(2+len(prefix),6,0),owner=dict(source='src/owner.c',flags=[],symbol='_cell'),identity=dict(rva=0x1180,size=5,operand=0,sha256=hashlib.sha256(anchor).hexdigest()))
    proof=dict(kind='rooted-callback-v1',offset=len(prefix)+(16 if stride==12 else 13),root_offset=len(prefix),root='_cell',mask=511,stride=stride,identity=dict(rva=0x1100,size=len(window),sha256=hashlib.sha256(native).hexdigest()),evidence=['independent synthetic native window, root and ABI review'])
    return dict(symbols={'_cell':root},indirect_calls=[proof]),len(code)


class CallbackTransferTests(unittest.TestCase):
    def setUp(self):
        d=workspace_temporary_directory('callback-transfer-');self.addCleanup(d.cleanup);self.root=Path(d.name)
        self.target,self.caller,self.owner=[self.root/n for n in ['native.dll','caller.obj','owner.obj']]
        self.bindings,self.size=fixture(self.target,self.caller,self.owner)
    def compare(self):return masked.relocated_compare(self.caller,'_sample',self.target,0x1000,self.size,self.bindings,storage_owners={'_cell':self.owner})
    def edit_native(self,offset,raw,rehash=False):
        data=bytearray(self.target.read_bytes());data[offset:offset+len(raw)]=raw;self.target.write_bytes(data)
        if rehash:
            p=self.bindings['indirect_calls'][0];p['identity']['sha256']=hashlib.sha256(data[0x500:0x500+p['identity']['size']]).hexdigest()
    def edit_both(self,offset,raw):
        data=bytearray(self.caller.read_bytes());data[60+offset:60+offset+len(raw)]=raw;self.caller.write_bytes(data);self.edit_native(0x400+offset,raw)

    def test_both_strides_and_count_reload_profiles(self):
        for stride in [8,12]:
            for reload in [False,True]:
                with self.subTest(stride=stride,reload=reload):
                    self.bindings,self.size=fixture(self.target,self.caller,self.owner,stride,reload);r=self.compare();self.assertTrue(r['relocated_equal']);self.assertEqual(r['indirect_call_checks'][0]['stride'],stride)
    def test_unreviewed_indirect_call_still_rejects(self):
        del self.bindings['indirect_calls']
        with self.assertRaisesRegex(ValueError,'Unsupported indirect'):self.compare()
    def test_malformed_extra_duplicate_and_unused_certificates_reject(self):
        baseline=copy.deepcopy(self.bindings)
        for mutation in [lambda p:p.update(kind='anything'),lambda p:p.update(extra=True),lambda p:p.update(root=[]),lambda p:p.update(offset=True),lambda p:p.update(mask=True),lambda p:p.update(stride=True),lambda p:p.update(mask=1023),lambda p:p.update(stride=4),lambda p:p.update(root_offset=1),lambda p:p.update(offset=1),lambda p:p.update(evidence=[])]:
            self.bindings=copy.deepcopy(baseline);mutation(self.bindings['indirect_calls'][0])
            with self.assertRaises(ValueError):self.compare()
        self.bindings=copy.deepcopy(baseline);self.bindings['indirect_calls']*=2
        with self.assertRaisesRegex(ValueError,'duplicate'):self.compare()
        self.bindings=copy.deepcopy(baseline);self.bindings['indirect_calls']='anything'
        with self.assertRaises(ValueError):self.compare()
    def test_mask_and_index_provenance_cannot_be_changed(self):
        self.edit_both(8,struct.pack('<I',1023))
        with self.assertRaisesRegex(ValueError,'mask'):self.compare()
    def test_wrong_root_register_even_with_equal_bytes_rejects(self):
        self.edit_both(15,b'\xd8')
        with self.assertRaisesRegex(ValueError,'provenance'):self.compare()
    def test_slot_displacement_and_register_indirect_remain_unsupported(self):
        for call in [bytes.fromhex('ff540a'),bytes.fromhex('ffd290')]:
            self.bindings,self.size=fixture(self.target,self.caller,self.owner);self.edit_both(13,call)
            with self.assertRaises(ValueError):self.compare()
    def test_wrong_argument_width_and_root_argument_alias_reject(self):
        self.edit_both(12,b'\x52')
        with self.assertRaisesRegex(ValueError,'disjoint'):self.compare()
        self.bindings,self.size=fixture(self.target,self.caller,self.owner);self.edit_both(12,b'\x54')
        with self.assertRaisesRegex(ValueError,'argument'):self.compare()
    def test_cleanup_width_and_post_call_clobber_reject(self):
        self.edit_both(18,b'\x08')
        with self.assertRaisesRegex(ValueError,'clean exactly'):self.compare()
        self.bindings,self.size=fixture(self.target,self.caller,self.owner,8,True);self.edit_both(16,bytes.fromhex('31c0'))
        with self.assertRaises(ValueError):self.compare()
    def test_stride12_lea_must_preserve_root_index_and_argument(self):
        self.bindings,self.size=fixture(self.target,self.caller,self.owner,12)
        self.edit_both(15,b'\x9b')
        with self.assertRaisesRegex(ValueError,'provenance'):self.compare()
    def test_root_clobber_or_extra_argument_cannot_hide_in_window(self):
        for extra in [bytes.fromhex('31d2'),b'\x57']:
            self.bindings,self.size=fixture(self.target,self.caller,self.owner)
            window=callback_code();changed=window[:13]+extra+window[13:];native=bytearray(changed);struct.pack_into('<I',native,2,0x402200)
            coff_file(self.caller,changed+b'\xc3',[(2,1,6)],[('_sample',0,1,0x20),('_cell',0,0,0)])
            self.edit_native(0x400,native+b'\xc3');self.size=len(changed)+1;self.bindings['indirect_calls'][0]['offset']+=len(extra)
            with self.assertRaises(ValueError):self.compare()
    def test_branch_into_call_or_cleanup_rejects(self):
        for destination in [15,18]:
            self.bindings,self.size=fixture(self.target,self.caller,self.owner,prefix=bytes([0xeb,destination-2]))
            with self.assertRaisesRegex(ValueError,'enters callback'):self.compare()
    def test_local_address_reference_into_window_rejects(self):
        off=self.bindings['indirect_calls'][0]['offset'];field=self.size+1
        code=callback_code()+b'\xc3\xb8\0\0\0\0\xc3';native=bytearray(code);struct.pack_into('<I',native,2,0x402200);struct.pack_into('<I',native,field,0x1000+0x400000+off)
        coff_file(self.caller,code,[(2,1,6),(field,2,6)],[('_sample',0,1,0x20),('_cell',0,0,0),('_entry',off,1,0)])
        self.bindings['symbols']['_entry']=binding('local',0x1000+off,3,(field,6,0),role='code');self.size=len(code);self.edit_native(0x400,native);self.edit_native(0x80e,struct.pack('<H',0x3000+field))
        with self.assertRaisesRegex(ValueError,'Local reference enters callback'):self.compare()
    def test_ordinary_nonreference_differences_still_reject(self):
        self.edit_native(0x400+self.size-1,b'\xcc');self.assertFalse(self.compare()['relocated_equal'])
    def test_wrong_bound_root_and_common_size_still_reject(self):
        self.bindings['symbols']['_cell']['rva']+=4
        with self.assertRaises(ValueError):self.compare()
        self.bindings,self.size=fixture(self.target,self.caller,self.owner);fixture_owner(self.owner,size=8)
        with self.assertRaisesRegex(ValueError,'exact-size COMMON'):self.compare()
    def test_independent_witness_hash_and_root_are_verified(self):
        self.edit_native(0x508,b'\x00')
        with self.assertRaisesRegex(ValueError,'hash changed'):self.compare()
        self.bindings,self.size=fixture(self.target,self.caller,self.owner);self.edit_native(0x502,struct.pack('<I',0x402204),True)
        with self.assertRaisesRegex(ValueError,'another root'):self.compare()
    def test_rehashed_wrong_witness_shape_rejects(self):
        self.edit_native(0x50f,b'\xd8',True)
        with self.assertRaisesRegex(ValueError,'provenance'):self.compare()
    def test_witness_requires_exact_native_highlow_field(self):
        self.edit_native(0x80a,struct.pack('<H',0x3103))
        with self.assertRaisesRegex(ValueError,'exactly one native HIGHLOW'):self.compare()
    def test_witness_cannot_be_caller_noncode_or_oversized(self):
        baseline=copy.deepcopy(self.bindings)
        for identity in [dict(rva=0x1000),dict(rva=0x2000),dict(size=33),dict(size=True),dict(sha256='x'*64)]:
            self.bindings=copy.deepcopy(baseline);self.bindings['indirect_calls'][0]['identity'].update(identity)
            with self.assertRaises(ValueError):self.compare()


class CallbackAcceptanceTests(unittest.TestCase):
    def harness(self):
        h=zero.ZeroStorageAcceptanceTests(methodName='test_fresh_owner_every_run_and_normal_transaction');h.setUp();self.addCleanup(h.doCleanups)
        bindings,size=fixture(h.target,h.obj,h.owner_obj);h.document['functions']['cell-reader']=bindings;h.write_bindings()
        h.row.update(size=size,target_sha256=match.digest(h.target.read_bytes()));h.state.write_text(json.dumps(dict(functions=[h.row]))+'\n')
        return h
    def test_normal_fresh_transaction_and_regression(self):
        h=self.harness();r,stats=h.verify();self.assertTrue(r[0]['exact']);self.assertEqual(stats['compiler_calls'],2)
        promote.promote_relocated(['cell-reader'],h.bindings);row=json.loads(h.state.read_text())['functions'][0]
        r,_=match.verify_rows([row]);self.assertTrue(r[0]['exact']);self.assertEqual(h.compiler.call_count,6)
    def test_callback_witness_cannot_be_inside_another_pending_caller(self):
        h=self.harness();other=dict(copy.deepcopy(h.row),id='witness-caller',rva='0x1100');h.document['functions']['witness-caller']=copy.deepcopy(h.document['functions']['cell-reader']);h.write_bindings()
        with self.assertRaisesRegex(ValueError,'Callback identity witness overlaps a selected caller'):
            match.verify_rows([h.row,other],relocation_candidates={r['id']:'evidence/bindings.json' for r in [h.row,other]})
        self.assertEqual(h.compiler.call_count,0)


if __name__=='__main__':unittest.main()
