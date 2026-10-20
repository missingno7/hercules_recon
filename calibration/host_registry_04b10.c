/* HERCULES.EXE 0x4b10 (lane h4 candidate): raise the per-slot level records of the
 * registry Config block (bytes 4..) from INDEX down to 0, then clamp slot 0 to 9.
 * Name/TU unproved; neighbours cleanup_native_4b00 (cleanup_registry.c) and
 * cleanup_native_4b50 (host_restart_selector.c) read the same Config bytes. */
typedef unsigned char BYTE;

extern BYTE g_registry_config[72];

void __cdecl host_04b10(int level, int index)
{
    for (; index >= 0; --index) {
        if (level > g_registry_config[4 + index])
            g_registry_config[4 + index] = (BYTE)level;
    }
    if (g_registry_config[4] > 9)
        g_registry_config[4] = 9;
}
