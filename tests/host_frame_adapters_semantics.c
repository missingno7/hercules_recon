#include <stdio.h>
extern void __cdecl host_pair_frame_initialize(void *, void *);
extern int __cdecl host_frame_header_copy(const unsigned long *);
extern void __cdecl host_mode_packet_adapter(void *, unsigned long, unsigned long, unsigned long, unsigned long);

unsigned long g_host_frame_prefix[25];

static unsigned long checks;
static unsigned long failures;

static void verify(int condition)
{
    ++checks;
    if (!condition)
        ++failures;
}

static void fill(unsigned char *bytes, unsigned long size, unsigned char seed)
{
    unsigned long i;
    for (i = 0; i != size; ++i)
        bytes[i] = (unsigned char)(seed + i * 13);
}

static int pair_byte_is_expected(unsigned long offset, unsigned char before)
{
    if (offset == 0x64 || offset == 0x65 || offset == 0x66 ||
        offset == 0x67 || offset == 0x68 || offset == 0x69)
        return 0;
    if (offset == 0x6a)
        return 240 & 0xff;
    if (offset == 0x6b)
        return (240 >> 8) & 0xff;
    return before;
}

int main(void)
{
    unsigned char first[0x70];
    unsigned char second[0x70];
    unsigned char first_before[0x70];
    unsigned char second_before[0x70];
    unsigned long header[23];
    unsigned char packet_storage[18];
    unsigned char packet_before[18];
    unsigned long i;

    fill(first, sizeof(first), 0x21);
    fill(second, sizeof(second), 0x83);
    for (i = 0; i != sizeof(first); ++i) {
        first_before[i] = first[i];
        second_before[i] = second[i];
    }

    host_pair_frame_initialize(first, second);
    for (i = 0; i != sizeof(first); ++i) {
        unsigned char first_expected =
            (unsigned char)pair_byte_is_expected(i, first_before[i]);
        unsigned char second_expected =
            (unsigned char)pair_byte_is_expected(i, second_before[i]);
        verify(first[i] == first_expected);
        verify(second[i] == second_expected);
    }

    for (i = 0; i != 23; ++i)
        header[i] = 0x10203040UL + i * 0x01010101UL;
    g_host_frame_prefix[23] = 0xfeed1234UL;
    g_host_frame_prefix[24] = 0xabcd9876UL;
    verify(host_frame_header_copy(header) == 0);
    for (i = 0; i != 23; ++i) {
        verify(g_host_frame_prefix[i] == header[i]);
    }
    verify(g_host_frame_prefix[23] == 0xfeed1234UL);
    verify(g_host_frame_prefix[24] == 0xabcd9876UL);

    fill(packet_storage, sizeof(packet_storage), 0x45);
    for (i = 0; i != sizeof(packet_storage); ++i)
        packet_before[i] = packet_storage[i];
    host_mode_packet_adapter(&packet_storage[1], 0x12345678UL, 0xfedcba98UL,
                             0xffffffffUL, 0xdeadbeefUL);
    verify(packet_storage[0] == packet_before[0]);
    for (i = 0; i != 3; ++i)
        verify(packet_storage[1 + i] == packet_before[1 + i]);
    verify(packet_storage[5] == 0xff && packet_storage[6] == 0xff &&
           packet_storage[7] == 0xff && packet_storage[8] == 0xe1);
    verify(packet_storage[9] == 0xef && packet_storage[10] == 0xbe &&
           packet_storage[11] == 0xad && packet_storage[12] == 0xde);
    for (i = 13; i != sizeof(packet_storage); ++i)
        verify(packet_storage[i] == packet_before[i]);

    printf("host frame adapter fixture: %lu checks, %lu failures\n",
           checks, failures);
    return failures != 0;
}
