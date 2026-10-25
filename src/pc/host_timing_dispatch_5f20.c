#include <windows.h>
#include <mmsystem.h>

/* Address-labelled host-cell views. Their aggregate/TU and larger extents are
 * not inferred from neighboring cells. */
extern int g_exe_91c46c;
extern LONG g_exe_cell_91c470;
extern DWORD g_exe_cell_91c474;
extern BYTE g_host_engine_call_flag_7a01fc;
extern void __cdecl host_timing_service_409cd0(void);
extern void __cdecl host_engine_dispatch_5ec0(void);

void __cdecl host_timing_dispatch_5f20(void)
{
    DWORD now;
    DWORD previous;
    DWORD accumulator_bits;
    LONG accumulator;
    int iteration;

    host_timing_service_409cd0();
    now = timeGetTime();
    previous = g_exe_cell_91c474;

    if (previous == 0) {
        g_exe_91c46c = 0;
        accumulator = 0;
    } else if (g_exe_91c46c != 0) {
        g_exe_91c46c = 0;
        accumulator = 0;
    } else {
        accumulator_bits = (DWORD)g_exe_cell_91c470 + (now - previous);
        accumulator = (LONG)accumulator_bits;
    }

    g_exe_cell_91c474 = now;
    if (accumulator > 0x42)
        accumulator = 0x42;
    if (accumulator < -0x42)
        accumulator = -0x42;
    g_exe_cell_91c470 = accumulator;

    iteration = 0;
    while (g_exe_cell_91c470 >= 0x21 && iteration < 2) {
        g_host_engine_call_flag_7a01fc = 1;
        host_engine_dispatch_5ec0();
        g_exe_cell_91c470 -= 0x21;
        ++iteration;
    }

    g_host_engine_call_flag_7a01fc = 0;
    if (g_exe_cell_91c470 >= 0) {
        g_exe_cell_91c470 -= 0x21;
        host_engine_dispatch_5ec0();
    }
}
