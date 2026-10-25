extern unsigned long g_host_sequence;

unsigned long __cdecl host_sequence_next(int ignored)
{
    return g_host_sequence++;
}
