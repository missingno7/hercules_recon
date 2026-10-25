/* ENG1 fixed-degree table helpers. The two rounding/phase orders differ. */
extern short g_243e0[1024];
typedef char long_is_32[sizeof(long)==4?1:-1];
#define ADD(a,b) ((long)((unsigned long)(a)+(unsigned long)(b)))

long __cdecl rounded_sine_angle(long angle)
{
    return (long)g_243e0[((ADD(angle/45,256))>>9)&1023]*2;
}

long __cdecl cosine_angle(long angle)
{
    return (long)g_243e0[((angle/45>>9)+256)&1023]*2;
}
