"""Compiler-free focused tests for the function-difference classifier."""
import sys
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from funcdiff import align_and_classify, _family, _first_real_difference


class FunctionDifferenceTests(unittest.TestCase):
    def test_register_swap_only(self):
        result = align_and_classify(bytes.fromhex("89 d8"), bytes.fromhex("89 d9"))
        self.assertEqual(result["classes"], {"register only": 1})
        self.assertEqual(result["register_map"], {"eax": ["ecx"]})
        self.assertTrue(result["register_map_consistent"])
        self.assertTrue(result["permutation_only"])
        self.assertEqual(_family(result), "allocator-only")

    def test_stack_offset_only(self):
        result = align_and_classify(bytes.fromhex("8b 44 24 04"), bytes.fromhex("8b 44 24 08"))
        self.assertEqual(result["classes"], {"stack-offset only": 1})
        self.assertEqual(result["stack_slot_map"], {"4": [8]})
        self.assertTrue(result["permutation_only"])
        self.assertEqual(_family(result), "allocator-only")

    def test_reordered_pair_is_scheduling(self):
        result = align_and_classify(bytes.fromhex("31 c0 40"), bytes.fromhex("40 31 c0"))
        self.assertTrue(result["same_instruction_multiset"])
        self.assertTrue(result["reordered"])
        self.assertGreater(result["moved_pair_count"], 0)
        self.assertEqual(_family(result), "scheduling")

    def test_branch_to_same_aligned_instruction_is_identical(self):
        result = align_and_classify(bytes.fromhex("eb 01 90 c3"), bytes.fromhex("eb 00 c3"))
        self.assertEqual(result["classes"], {"identical": 2, "unaligned": 1})

    def test_real_opcode_change(self):
        result = align_and_classify(bytes.fromhex("05 01 00 00 00"), bytes.fromhex("2d 01 00 00 00"))
        self.assertEqual(result["classes"], {"different mnemonic/opcode": 1})
        self.assertIsNotNone(result["first_real_difference"])

    def test_insertion_is_unaligned(self):
        result = align_and_classify(bytes.fromhex("90"), bytes.fromhex("90 90"))
        self.assertEqual(result["classes"], {"identical": 1, "unaligned": 1})
        self.assertEqual(_family(result), "extent")

    def test_equal_size_unaligned_is_not_extent(self):
        # Two one-byte NOPs versus one two-byte NOP: same extent, different
        # masked instruction multiset, so this is real logic rather than extent.
        result = align_and_classify(bytes.fromhex("90 90"), bytes.fromhex("66 90"))
        self.assertTrue(result["size_equal"])
        self.assertEqual(result["classes"].get("unaligned"), 1)
        self.assertEqual(_family(result), "real logic")

    def test_relocation_masks_encoded_field(self):
        result = align_and_classify(
            bytes.fromhex("a1 11 22 33 44"), bytes.fromhex("a1 00 00 00 00"),
            relocations=[{"offset": 1, "type": 0x0006, "symbol": "_global"}],
        )
        self.assertEqual(result["classes"], {"relocation-operand only": 1})
        self.assertIsNone(result["first_real_difference"])

    def test_relocation_byte_coverage_across_x86_operand_encodings(self):
        cases = [
            ("disp32", "8b 3d 18 56 ba 00", "8b 3d 00 00 00 00", 2, 0x0006),
            ("moffs-a1", "a1 18 56 ba 00", "a1 00 00 00 00", 1, 0x0006),
            ("moffs-a3", "a3 18 56 ba 00", "a3 00 00 00 00", 1, 0x0006),
            ("push-imm32", "68 18 56 ba 00", "68 00 00 00 00", 1, 0x0006),
            ("mov-imm32", "b8 18 56 ba 00", "b8 00 00 00 00", 1, 0x0006),
            ("call-rel32", "e8 20 00 00 00", "e8 00 00 00 00", 1, 0x0014),
            ("jmp-rel32", "e9 20 00 00 00", "e9 00 00 00 00", 1, 0x0014),
        ]
        for name, native, compiled, offset, reloc_type in cases:
            with self.subTest(encoding=name):
                result = align_and_classify(
                    bytes.fromhex(native), bytes.fromhex(compiled),
                    relocations=[{"offset": offset, "type": reloc_type, "symbol": "_reloc"}],
                )
                self.assertEqual(result["classes"], {"relocation-operand only": 1})

    def test_moved_relocation_pair_is_not_reported_as_first_real_difference(self):
        pair = {
            "class": "relocation-operand only", "moved": True,
            "native_offset": "0x2c", "compiled_offset": "0x5a",
            "native_instruction": "mov edi, dword ptr [0xba5618]",
            "compiled_instruction": "mov edi, dword ptr [0]",
        }
        self.assertIsNone(_first_real_difference([pair], reordered=False))


if __name__ == "__main__":
    unittest.main()
