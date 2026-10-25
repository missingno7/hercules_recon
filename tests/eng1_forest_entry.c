#include <stdio.h>
#include "../src/eng1/forest_entry_data.h"

static unsigned long unrelated_before = 0x13579bdfUL;
static unsigned long unrelated_after = 0x2468ace0UL;
static unsigned short pointer_sentinel[4] = {1U, 2U, 3U, 4U};

static unsigned long pointer_cell_bits(void)
{
    return *(unsigned long *)(void *)&g_actor_pipeline_storage_71440.byte_cells[0x2d4];
}

static int check(int condition, const char *label)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        return 0;
    }
    return 1;
}

int main(void)
{
    int checks = 0;

    eng1_native_cell_06ef10 = 0x89abcdefUL;
    g_actor_pipeline_storage_71440.dword_cells[0x2d0 / 4] = 0xa1b2c3d4UL;
    g_actor_pipeline_storage_71440.dword_cells[0x2d8 / 4] = 0x5a6b7c8dUL;
    eng1_native_ptr_071714 = pointer_sentinel;
    dispatch_local_00e3e0();
    checks += check(eng1_native_cell_06ef10 == 0UL, "DWORD state reset");
    checks += check(eng1_native_ptr_071714 == eng1_native_words_04dce8,
                    "event WORD pointer rebound to 0x4dce8 owner");
    checks += check(pointer_cell_bits() ==
                    (unsigned long)(void *)eng1_native_words_04dce8,
                    "raw owner DWORD at offset 0x2d4 contains the table pointer");
    checks += check(eng1_native_words_04dce8[0] == 0x00d0U &&
                    eng1_native_words_04dce8[1] == 0x0000U &&
                    eng1_native_words_04dce8[2] == 0x00d3U &&
                    eng1_native_words_04dce8[3] == 0x000fU &&
                    eng1_native_words_04dce8[4] == 0x0010U &&
                    eng1_native_words_04dce8[5] == 0x0000U &&
                    eng1_native_words_04dce8[6] == 0x0000U &&
                    eng1_native_words_04dce8[7] == 0x0000U,
                    "pointed-to initialized WORD prefix preserved");
    checks += check(unrelated_before == 0x13579bdfUL &&
                    unrelated_after == 0x2468ace0UL &&
                    pointer_sentinel[0] == 1U && pointer_sentinel[3] == 4U &&
                    g_actor_pipeline_storage_71440.dword_cells[0x2d0 / 4] ==
                        0xa1b2c3d4UL &&
                    g_actor_pipeline_storage_71440.dword_cells[0x2d8 / 4] ==
                        0x5a6b7c8dUL,
                    "unrelated cells and pointer-neighbor cells unchanged");

    eng1_native_cell_06ef10 = 0xffffffffUL;
    eng1_native_ptr_071714 = 0;
    dispatch_local_00e3e0();
    checks += check(eng1_native_cell_06ef10 == 0UL,
                    "DWORD state reset is repeatable");
    checks += check(eng1_native_ptr_071714 == eng1_native_words_04dce8,
                    "pointer rebind is repeatable");
    checks += check(pointer_cell_bits() ==
                    (unsigned long)(void *)eng1_native_words_04dce8,
                    "repeat writes to the same owner DWORD");

    if (checks != 8) {
        fprintf(stderr, "FAIL: %d/8 checks passed\n", checks);
        return 1;
    }
    puts("forest entry fixture: 8 checks passed");
    return 0;
}
