/* Native1fdf0 reads a signed level byte and tail-dispatches four cases.
 * Jump table1fe1c..1fe40 is native instruction-section data, not a second
 * function or an initialized source data owner. */
#include "../shared/engine_interface.h"
extern void __cdecl eng1_native_code_target_00c720(void);
extern void __cdecl eng1_337b0(void);
extern void __cdecl eng1_native_code_target_01abf0(void);
extern void __cdecl eng1_3c130(void);
void __cdecl eng1_call_1fdf0(void)
{
    switch ((signed char)g_engine_interface.context_004->unknown_000[0]) {
    case 1: eng1_native_code_target_00c720();return;
    case 5: eng1_337b0();return;
    case 6: eng1_native_code_target_01abf0();return;
    case 9: eng1_3c130();return;
    default:return;
    }
}


extern void __cdecl eng1_25190(void *actor);
extern void __cdecl eng1_3fc60(void *actor);

void __cdecl eng1_20000(void *actor)
{
    switch ((signed char)g_engine_interface.context_004->unknown_000[0]) {
    case 1:
        eng1_3fc60(actor);
        break;
    case 5:
        eng1_25190(actor);
        break;
    }
}
