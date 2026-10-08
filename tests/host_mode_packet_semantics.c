#include <stdio.h>
#include <string.h>

extern void __cdecl host_mode_packet_adapter(void *, unsigned long, unsigned long,
                                           unsigned long, unsigned long);
static int checks, failures;
#define CHECK(c) do { ++checks; if (!(c)) ++failures; } while (0)

int main(void)
{
    unsigned long storage[6];
    static const unsigned long fourth[] = {0x12000001UL, 0xff123456UL, 0x009a9876UL};
    static const unsigned long fifth[] = {0, 0xdeadbeefUL, 0xffffffffUL};
    unsigned long i;
    for (i = 0; i != 3; ++i) {
        memset(storage, 0xa5, sizeof(storage));
        storage[1] = 0x13579bdfUL;
        host_mode_packet_adapter(&storage[1], 0xffffffffUL, i, fourth[i], fifth[i]);
        CHECK(storage[0] == 0xa5a5a5a5UL);
        CHECK(storage[1] == 0x13579bdfUL);
        CHECK(storage[2] == ((fourth[i] & 0x00ffffffUL) | 0xe1000000UL));
        CHECK(storage[3] == fifth[i]);
        CHECK(storage[4] == 0xa5a5a5a5UL);
        CHECK(storage[5] == 0xa5a5a5a5UL);
    }
    printf("host mode packet: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
