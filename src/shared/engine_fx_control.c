#include "engine_interface.h"
#include "engine_fx_record.h"

extern void __cdecl title_0c6f0(void);
extern void __cdecl dispatch_local_2ca90(void);
extern void __cdecl dispatch_local_2cad0(unsigned long value);
extern void __cdecl dispatch_local_2cae0(void);
extern void __cdecl title_0c700(void);
extern void __cdecl title_0c5e0(void);
extern void __cdecl title_0c550(EngineNoArgCallback callback);
extern void __cdecl dispatch_local_0666b(void);
extern unsigned long g_setup_restore_latch_6ee8c;
extern unsigned long g_setup_previous_mute_6ee98;

void __cdecl dispatch_local_0827f(int mode)
{
    title_0c6f0();
    dispatch_local_2ca90();
    if (mode == 1)
        dispatch_local_2cad0(0x1000);
    else
        dispatch_local_2cad0(5);
    dispatch_local_2cae0();
    title_0c700();
}

void __cdecl dispatch_local_082b7(int mode)
{
    int i;

    title_0c5e0();
    g_setup_restore_latch_6ee8c = 0;
    g_setup_previous_mute_6ee98 = 0;
    if (mode == 0)
        title_0c550(dispatch_local_0666b);
    else
        title_0c550((EngineNoArgCallback)0);
    for (i = 0; i < 24; ++i) {
        *(unsigned short *)(void *)&g_engine_fx_records[i].unknown_00c[0] = 0x00ff;
        g_engine_fx_records[i].unknown_030 = 0;
    }
}
