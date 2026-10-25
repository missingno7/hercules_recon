"""Falsifiers for the bounded complete WORD-loop format proof."""
from pathlib import Path
import copy
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from word_program_differential import loop_profile,same_local_targets


class ClosedWordProfileTests(unittest.TestCase):
    def test_sound_operand_is_not_a_restart(self):
        p=loop_profile([3,6,0xffd8,0xffff,7,0xffff])
        self.assertEqual(p['restart_index'],5)
        self.assertEqual(p['frame_indices'],[1,4])
        self.assertEqual(p['sound_indices'],[dict(opcode=2,operand=3)])

    def test_operand_marker_cannot_supply_missing_restart(self):
        for words in [[3,6,0xffd8,0xffff],[3,6,0xffd8]]:
            with self.subTest(words=words),self.assertRaises(ValueError):loop_profile(words)

    def test_loop_must_reach_a_frame(self):
        for words in [[3,0xffff],[3,0xffd8,6,0xffff]]:
            with self.subTest(words=words),self.assertRaises(ValueError):loop_profile(words)

    def test_callbacks_and_removal_are_not_certified(self):
        for opcode in [0xfff6,0xfffa,0xfff9,0xfff8,0xfff7]:
            with self.subTest(opcode=opcode),self.assertRaises(ValueError):loop_profile([3,6,opcode,0xffff])

    def test_tail_after_restart_is_not_a_complete_object(self):
        with self.assertRaises(ValueError):loop_profile([3,6,0xffff,0])

    def test_raw_words_are_exact_unsigned_integers(self):
        for word in [-1,65536,True,1.0,'1']:
            with self.subTest(word=word),self.assertRaises(ValueError):loop_profile([3,word,0xffff])

    def test_duration_is_data_even_when_negative(self):
        p=loop_profile([0xffff,0,9,0xffff])
        self.assertEqual(p['duration'],-1)
        self.assertEqual(p['frame_indices'],[1,2])

    def test_sound_operand_can_look_like_an_opcode(self):
        p=loop_profile([0,0xffd8,0xffd8,6,0xffff])
        self.assertEqual(p['frame_indices'],[3])
        self.assertEqual(p['stable_return_states'],[0,3])


class ParsedFixture:
    def __init__(self,section,label):
        self.record=dict(name='.text',size=16,data=bytes(16),flags=0x60001020,relocations=[dict(offset=4,symbol=label,type=6)])
        self.symbols={0:dict(name='_test',value=0,section=section,type=32,storage=2,aux=[]),
                      1:dict(name=label,value=12,section=section,type=0,storage=6,aux=[])}

    def function(self,name):
        assert name=='_test'
        return self.record


class FixtureLabelTests(unittest.TestCase):
    def test_only_label_number_and_section_ordinal_can_change(self):
        self.assertEqual(same_local_targets(ParsedFixture(1,'$L1'),ParsedFixture(7,'$L99'),'_test'),1)

    def test_retargeting_or_nonlocal_symbol_metadata_rejects(self):
        good=ParsedFixture(1,'$L1')
        for field,value in [('value',4),('value',16),('section',0),('section',8),('storage',105),('storage',2),('type',32),('aux',[bytes(18)])]:
            bad=ParsedFixture(7,'$L99');bad.symbols[1][field]=value
            with self.subTest(field=field,value=value),self.assertRaises(ValueError):same_local_targets(good,bad,'_test')
        bad=ParsedFixture(7,'_external');
        with self.assertRaises(ValueError):same_local_targets(good,bad,'_test')
        bad=ParsedFixture(7,'$L99');bad.symbols[2]=copy.deepcopy(bad.symbols[1])
        with self.assertRaises(ValueError):same_local_targets(good,bad,'_test')

    def test_bytes_and_relocation_fields_cannot_be_normalized(self):
        good=ParsedFixture(1,'$L1')
        for field,value in [('data',bytes([1])+bytes(15)),('flags',0x60000020),('size',17)]:
            bad=ParsedFixture(7,'$L99');bad.record[field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):same_local_targets(good,bad,'_test')
        for field,value in [('offset',5),('type',20)]:
            bad=ParsedFixture(7,'$L99');bad.record['relocations'][0][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):same_local_targets(good,bad,'_test')


if __name__=='__main__':unittest.main()
