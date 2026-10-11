/* Reconstruction for HERCULES.EXE RVA 0x4bc0.
   Working views reuse the observed registry prefix; original type/TU remain unproved. */
extern unsigned char g_registry_config[72];

long __cdecl host_packed_code_04bc0(long value)
{
    if (value != -1) {
        *(long *)(g_registry_config + 8) = value;
        return value;
    }

    value = *(long *)(g_registry_config + 8);
    g_registry_config[12] = (unsigned char)(value >> 8);
    return value;
}
