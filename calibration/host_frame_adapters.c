#include <string.h>

extern int __cdecl host_zero_callback();
extern unsigned long g_host_frame_prefix[];

void __cdecl host_pair_frame_initialize(void *first, void *second)
{
    unsigned char *first_bytes = (unsigned char *)first;
    unsigned char *second_bytes = (unsigned char *)second;

    host_zero_callback(first, 0, 0, 320, 240);
    host_zero_callback(second, 0, 256, 320, 240);
    host_zero_callback(first_bytes + 0x5c, 0, 256, 320, 240);
    host_zero_callback(second_bytes + 0x5c, 0, 0, 320, 240);

    *(unsigned short *)(second_bytes + 0x64) = 0;
    *(unsigned short *)(first_bytes + 0x64) = 0;
    *(unsigned short *)(second_bytes + 0x66) = 0;
    *(unsigned short *)(first_bytes + 0x66) = 0;
    *(unsigned short *)(second_bytes + 0x6a) = 240;
    *(unsigned short *)(first_bytes + 0x6a) = 240;
    *(unsigned short *)(second_bytes + 0x68) = 0;
    *(unsigned short *)(first_bytes + 0x68) = 0;
}

int __cdecl host_frame_header_copy(const unsigned long *source)
{
    memcpy(g_host_frame_prefix, source, 23 * sizeof(unsigned long));
    return 0;
}
