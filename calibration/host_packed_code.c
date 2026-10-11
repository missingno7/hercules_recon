/* Reconstruction for HERCULES.EXE RVA 0x4bc0.
   Data objects remain external; their aggregate extent and owner are unproved. */
extern long g_packed_code_79e860;
extern unsigned char g_packed_code_aux_79e864;

long __cdecl host_packed_code_04bc0(long value)
{
    if (value != -1) {
        g_packed_code_79e860 = value;
        return value;
    }

    value = g_packed_code_79e860;
    g_packed_code_aux_79e864 = (unsigned char)(value >> 8);
    return value;
}
