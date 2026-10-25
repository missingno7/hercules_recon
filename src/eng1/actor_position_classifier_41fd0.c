/* Bounded ordinary-C reconstruction of ENG1.DLL RVA 0x41fd0.
 *
 * Native reads the first DWORD of the measured table for its initial low-WORD
 * sentinel check, then reads one WORD from each 12-byte row. The source owner
 * below has the measured full 216-byte writable extent; row columns 1..5 are
 * included as data even though this function does not consume them.
 */
#include "actor_control_update.h"
#include "incomplete.h"



int __cdecl eng1_classify_actor_position_checked(unsigned long value,
                                 unsigned char *result)
{
    short signed_high_word;
    unsigned long first_dword;
    unsigned int row;

    signed_high_word = (short)(unsigned short)(value >> 16);

    /* Native 0x41fd4 loads a DWORD, but only the low WORD participates in
     * the initial sentinel comparison at 0x41fe4. */
    first_dword = *(volatile unsigned long *)(void *)
        &g_eng1_classifier_rows_65678[0][0];
    if ((unsigned short)first_dword == 0xffffU) {
        *result = 0xffU;
        return 1;
    }

    for (row = 0U; row < 18U; ++row) {
        unsigned short threshold;

        threshold = *(volatile unsigned short *)(void *)
            &g_eng1_classifier_rows_65678[row][0];
        if (signed_high_word > 0 &&
            (unsigned short)signed_high_word < threshold) {
            *result = (unsigned char)row;
            return 1;
        }
        if (threshold == 0xffffU) {
            *result = 0xffU;
            return 1;
        }
    }

    /* Native continues scanning beyond this measured 18-row owner when a
     * sentinel is missing. Report that source bound as unsupported, and leave
     * the output untouched instead of inventing a classifier result. */
    return 0;
}

/* Native public ABI is one DWORD cdecl, with all reviewed direct callers
 * consuming AL. Incidental upper EAX and transitive propagation are unproved. */
unsigned char __cdecl eng1_native_code_target_0041fd0(unsigned long value)
{
    unsigned char result;
    if (!eng1_classify_actor_position_checked(value, &result))
        eng1_incomplete_behavior(0x65678UL,
            "41fd0 scan exceeds measured 18-row threshold prefix");
    return result;
}
