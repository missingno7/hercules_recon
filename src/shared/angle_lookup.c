/* PC fixed-angle table searches. Keep the historical signed quotient behavior. */
extern long g_tangent_quarter_degrees[360];
extern short g_243e0[1024];
typedef char angle_long_is_32[sizeof(long) == 4 ? 1 : -1];
typedef char angle_short_is_16[sizeof(short) == 2 ? 1 : -1];

long __cdecl lookup_tangent_angle(unsigned short a, unsigned short b)
{
    long ratio;
    int lo = 0, hi = 359, mid;
    if (!a) return 0x5a0000;
    ratio = (long)((unsigned long)b << 16) / a;
    mid = 179;
    while (g_tangent_quarter_degrees[mid] != ratio) {
        if (g_tangent_quarter_degrees[mid] < ratio) lo = mid;
        else hi = mid;
        if (lo >= hi - 1) return lo << 14;
        mid = (hi - lo) / 2 + lo;
    }
    return mid << 14;
}

unsigned short __cdecl lookup_cosine_phase(int a, int b)
{
    long ratio;
    int lo = 0, hi = 511, mid, value;
    if (!b) return 0;
    if (!a) return 255;
    ratio = (long)((unsigned long)a << 15) / b;
    mid = 255;
    value = g_243e0[mid + 256];
    while (value != ratio) {
        if (value > ratio) lo = mid;
        else hi = mid;
        if (lo >= hi - 1) return (unsigned short)lo;
        mid = (hi - lo) / 2 + lo;
        value = g_243e0[mid + 256];
    }
    return (unsigned short)mid;
}
