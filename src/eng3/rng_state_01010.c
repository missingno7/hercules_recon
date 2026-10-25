/* ENG3.DLL 0x1010: wrapping 32-bit random-state update and bounded result.
 *
 * The original file-backed DWORDs at 0x33070/0x33074 start at 123/456.
 * Observed native references to these cells are confined to this helper.
 * These descriptive names and private storage do not establish original
 * names or translation-unit ownership. This unit requires 32-bit unsigned int.
 * See evidence/eng3_rng_provider_2026_10_25.json; historical output is NEAR.
 */
static unsigned int eng3_rng_state_33070 = 0x7bU;
static unsigned int eng3_rng_state_33074 = 0x1c8U;

unsigned int __cdecl eng3_01010(unsigned int limit)
{
    eng3_rng_state_33074 += eng3_rng_state_33070;
    eng3_rng_state_33070 += eng3_rng_state_33074;
    if ((eng3_rng_state_33074 & 0x80000000U) != 0)
        eng3_rng_state_33070++;

    /* Native DIV has no zero guard. Callers require a nonzero limit. */
    return eng3_rng_state_33070 % limit;
}
