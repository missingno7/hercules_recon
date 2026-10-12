/* Private behavioral candidates for two ENG1 context-byte dispatchers.
 * The observed byte at context+0 is intentionally kept unnamed. */
#include "../../../calibration/engine_interface.h"

extern void __cdecl eng1_native_code_target_003ca10(void); /* ENG1 0x3ca10 */
extern void __cdecl eng1_native_code_target_00e3e0(void);  /* ENG1 0x0e3e0 */
extern void __cdecl eng1_native_code_target_0033700(void); /* ENG1 0x33700 */
extern void __cdecl eng1_native_code_target_001ac10(void); /* ENG1 0x1ac10 */
extern void __cdecl eng1_native_code_target_0021050(void); /* ENG1 0x21050 */
extern void __cdecl eng1_native_code_target_003c170(void); /* ENG1 0x3c170 */
extern void __cdecl eng1_native_code_target_0044120(void); /* ENG1 0x44120 */

extern void __cdecl eng1_native_code_target_003ca50(void); /* ENG1 0x3ca50 */
extern void __cdecl eng1_native_code_target_0033720(void); /* ENG1 0x33720 */
extern void __cdecl eng1_native_code_target_001ae00(void); /* ENG1 0x1ae00 */
extern void __cdecl eng1_native_code_target_0021210(void); /* ENG1 0x21210 */
extern void __cdecl eng1_native_code_target_0044240(void); /* ENG1 0x44240 */

/* ENG1 RVA 0x1fd20. The signed context byte at offset 0 indexes an 11-entry table.
 * Empty entries are genuine no-ops; the selected targets remain RVA-labelled
 * external dependencies until their own source providers are recovered. */
void __cdecl dispatch_local_1fd20(void)
{
    signed char selector =
        (signed char)g_engine_interface.context_004->unknown_000[0];

    switch (selector) {
    case 1:
        eng1_native_code_target_003ca10();
        return;
    case 3:
        eng1_native_code_target_00e3e0();
        return;
    case 5:
        eng1_native_code_target_0033700();
        return;
    case 6:
        eng1_native_code_target_001ac10();
        return;
    case 7:
        eng1_native_code_target_0021050();
        return;
    case 9:
        eng1_native_code_target_003c170();
        return;
    case 11:
        eng1_native_code_target_0044120();
        return;
    default:
        return;
    }
}

/* ENG1 RVA 0x1fd90. Native first maps selector-1 through the measured byte
 * sequence {0,5,5,5,1,2,3,5,5,5,4}, then indexes its six-entry jump table. */
void __cdecl dispatch_local_1fd90(void)
{
    signed char selector =
        (signed char)g_engine_interface.context_004->unknown_000[0];

    switch (selector) {
    case 1:
        eng1_native_code_target_003ca50();
        return;
    case 5:
        eng1_native_code_target_0033720();
        return;
    case 6:
        eng1_native_code_target_001ae00();
        return;
    case 7:
        eng1_native_code_target_0021210();
        return;
    case 11:
        eng1_native_code_target_0044240();
        return;
    default:
        return;
    }
}
