/* Private ordinary-C model of ENG1:0x8117 and its missing one-cell bridge. */
#include "../shared/engine_interface.h"
#include "../shared/engine_fx_record.h"

extern FxSoundRecordView g_engine_fx_records[24];
extern void __cdecl dispatch_local_2c650(void);
extern void __cdecl dispatch_local_2c660(void);
extern void __cdecl dispatch_engine_2cb70(unsigned long channel);
extern void __cdecl dispatch_local_01000(const void *message);

void __cdecl engine_init(void)
{
    int index;

    dispatch_local_2c650();
    dispatch_local_01000("\n ClearAllSoundFx()");

    for (index = 0; index < 24; ++index) {
        if (g_engine_fx_records[index].active_03c != 0)
            dispatch_engine_2cb70((unsigned long)(unsigned short)index);

        *(unsigned short *)(void *)&g_engine_fx_records[index].unknown_00c[0] =
            0x00ff;
        *(unsigned short *)(void *)&g_engine_fx_records[index].unknown_00c[2] =
            0;
        g_engine_fx_records[index].unknown_030 = 0;
    }

    dispatch_local_2c660();
}
