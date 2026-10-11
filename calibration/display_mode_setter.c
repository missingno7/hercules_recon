/* Private reconstruction of the native requested display-mode setter at EXE RVA 0x6ec0. */
extern unsigned char g_exe_96e3d0;
extern unsigned char g_fatal_suppress_dialog;

unsigned short __cdecl cleanup_native_6ec0(int value)
{
    g_exe_96e3d0 = (unsigned char)value;
    return g_fatal_suppress_dialog;
}
