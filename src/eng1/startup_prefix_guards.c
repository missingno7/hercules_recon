extern long g_frame_script_58d50_prefix[9];
#include "incomplete.h"
#include "startup_prefixes.h"
long __cdecl eng1_script_value_58d50(int index)
{
    if (index < 0 || index >= 9)
        eng1_incomplete_behavior(0x219e0UL, "script58d50 index outside proven sentinel prefix");
    return g_frame_script_58d50_prefix[index];
}
