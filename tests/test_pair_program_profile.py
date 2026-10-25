"""Fixed-width pair grammar cannot use WORD-only stopping rules."""
from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from player_animation_differential import pair_profile

class PairProgramTests(unittest.TestCase):
    def test_pairs_have_own_ticks_and_no_duration_header(self):
        p=pair_profile([54,3,55,4,0xffff,0]);self.assertEqual(p['frame_indices'],[0,1]);self.assertEqual(p['frame_ticks'],[3,4]);self.assertEqual(p['bytes'],12)
    def test_restart_operand_is_part_of_complete_record(self):
        with self.assertRaises(ValueError):pair_profile([54,3,0xffff])
    def test_restart_must_delimit_object(self):
        for words in [[54,3],[54,3,0xffff,0,55,4]]:
            with self.subTest(words=words),self.assertRaises(ValueError):pair_profile(words)
    def test_empty_loop_cannot_reach_frame(self):
        with self.assertRaises(ValueError):pair_profile([0xffff,0])
    def test_word_sound_opcode_is_not_implicitly_supported(self):
        with self.assertRaises(ValueError):pair_profile([54,3,0xffd8,6,0xffff,0])
    def test_ticks_and_ignored_restart_operand_are_not_opcodes(self):
        p=pair_profile([54,0xffff,0xffff,0xffd8]);self.assertEqual(p['frame_ticks'],[-1]);self.assertEqual(p['stable_return_states'],[-1,0])
    def test_invalid_raw_word_values_reject(self):
        for value in [True,-1,65536,1.5]:
            with self.subTest(value=value),self.assertRaises(ValueError):pair_profile([54,value,0xffff,0])

if __name__=='__main__':unittest.main()
