extern void __cdecl audio_native_fa10(int value, int slot);
extern unsigned short g_exe_79e438;
extern unsigned short g_exe_79e43c;

void __cdecl audio_native_10010(void)
{
    int slot;
    for (slot = 0; slot < 24; ++slot)
        audio_native_fa10(-1, slot);
}

void __cdecl audio_native_fdd0(unsigned short first, unsigned short second)
{
    g_exe_79e438 = first;
    g_exe_79e43c = second;
}
