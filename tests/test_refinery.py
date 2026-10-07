import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from refinery import dominates
from refinery_forms import replace_function,zero_store_forms

class RefineryTests(unittest.TestCase):
    def setUp(self):
        self.base=dict(strict_equal=False,normalized_different_bytes=8,instruction_percent=90,
                       candidate_instruction_count=57,original_instruction_count=57,
                       ordered_cfg_equal=True,direct_call_targets_equal=True)
    def test_byte_gain_cannot_hide_cfg_call_count_or_instruction_regression(self):
        for key,value in [('ordered_cfg_equal',False),('direct_call_targets_equal',False),
                          ('candidate_instruction_count',58),('instruction_percent',89)]:
            row=dict(self.base,normalized_different_bytes=2);row[key]=value
            self.assertFalse(dominates(row,self.base,True))
    def test_relocation_identity_is_a_gate(self):
        self.assertFalse(dominates(dict(self.base,normalized_different_bytes=0),self.base,False))
    def test_equal_is_not_an_improvement(self):
        self.assertFalse(dominates(self.base,self.base,True))
        self.assertTrue(dominates(dict(self.base,normalized_different_bytes=2),self.base,True))
    def test_replacement_preserves_nested_body_and_other_functions(self):
        text='int f(int x) { if(x) {return 1;} return 0;}\nint g(void) {return 2;}\n'
        self.assertEqual(replace_function(text,'f','int f(int x) {return !!x;}'),
                         'int f(int x) {return !!x;}\nint g(void) {return 2;}\n')

if __name__=='__main__':unittest.main()
