typedef unsigned long AudioDword;

extern AudioDword g_exe_96e62c;
extern AudioDword g_exe_96e620;
extern AudioDword g_exe_96e61c;
extern AudioDword g_exe_96e628;
extern AudioDword g_exe_96e624;
extern unsigned char g_audio_cleanup_iteration_flag;
extern const char audio_stop_command[];

extern int __cdecl cleanup_native_9c30(void);
extern void __cdecl cleanup_native_f710(unsigned int index);

__declspec(dllimport) unsigned int __stdcall mciSendStringA(
    const char *command,
    char *return_buffer,
    unsigned int return_length,
    AudioDword callback);

void __cdecl cleanup_native_9e10(void)
{
    if (g_exe_96e62c != 0 && cleanup_native_9c30() != 0)
        mciSendStringA(audio_stop_command, 0, 0, 0);

    g_exe_96e62c = 0;
    g_exe_96e620 = 0;
    g_exe_96e61c = 0;
    g_exe_96e628 = 0;
    g_exe_96e624 = 0;
}

void __cdecl cleanup_native_ff40(int value)
{
    int index;

    g_audio_cleanup_iteration_flag = (unsigned char)value;
    for (index = 0; index < 1200; ++index)
        cleanup_native_f710(index);
    g_audio_cleanup_iteration_flag = 0;
}
