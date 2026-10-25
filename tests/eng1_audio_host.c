#include "../src/shared/engine_interface.h"

EngineInterface g_engine_interface;

static int calls[7];
static int failures;
static signed char *expected_status;
static unsigned long *expected_info;
static short *expected_left;
static short *expected_right;

static void __cdecl spy_2caf0(void)
{
    ++calls[0];
}

static void __cdecl spy_2ca60(signed char *status)
{
    ++calls[1];
    if (status != expected_status) ++failures;
    status[0] = -3;
}

static void __cdecl spy_2ca70(int channel, unsigned long *info)
{
    ++calls[2];
    if (channel != 19 || info != expected_info) ++failures;
    *info = 0x87654321UL;
}

static void __cdecl spy_2cab0(short left, short right)
{
    ++calls[3];
    if (left != -123 || right != 234) ++failures;
}

static void __cdecl spy_2cb00(short channel, short *left, short *right)
{
    ++calls[4];
    if (channel != -7 || left != expected_left || right != expected_right)
        ++failures;
    *left = -321;
    *right = 654;
}

static short __cdecl spy_2cb80(unsigned short bank, unsigned short sample,
                               int zero_a, int frequency, int zero_b,
                               short left, short right)
{
    ++calls[5];
    if (bank != 0x1234 || sample != 0xabcd || zero_a != 0 ||
        frequency != 0x3c || zero_b != 0 || left != -111 || right != 222)
        ++failures;
    return -9;
}

static void __cdecl spy_2cbb0(short channel, short left, short right)
{
    ++calls[6];
    if (channel != 5 || left != -5 || right != 6) ++failures;
}

void __cdecl dispatch_engine_2caf0(void);
void __cdecl dispatch_engine_2ca60(signed char *status);
void __cdecl dispatch_engine_2ca70(int channel, unsigned long *info);
void __cdecl dispatch_engine_2cab0(short left, short right);
void __cdecl dispatch_engine_2cb00(short channel, short *left, short *right);
short __cdecl dispatch_engine_2cb80(unsigned short bank, unsigned short sample,
                                    int zero_a, int frequency, int zero_b,
                                    short left, short right);
void __cdecl dispatch_engine_2cbb0(short channel, short left, short right);

int main(void)
{
    signed char status[24];
    unsigned long info = 0;
    short left = 0;
    short right = 0;

    expected_status = status;
    expected_info = &info;
    expected_left = &left;
    expected_right = &right;
    g_engine_interface.opaque_2f0[4] = (unsigned long)spy_2ca60;
    g_engine_interface.opaque_2f0[8] = (unsigned long)spy_2ca70;
    g_engine_interface.opaque_334[10] = (unsigned long)spy_2cab0;
    g_engine_interface.opaque_334[15] = (unsigned long)spy_2caf0;
    g_engine_interface.opaque_334[16] = (unsigned long)spy_2cb00;
    g_engine_interface.opaque_334[21] = (unsigned long)spy_2cb80;
    g_engine_interface.opaque_334[23] = (unsigned long)spy_2cbb0;

    dispatch_engine_2caf0();
    dispatch_engine_2ca60(status);
    dispatch_engine_2ca70(19, &info);
    dispatch_engine_2cab0(-123, 234);
    dispatch_engine_2cb00(-7, &left, &right);
    if (dispatch_engine_2cb80(0x1234, 0xabcd, 0, 0x3c, 0, -111, 222) != -9)
        ++failures;
    dispatch_engine_2cbb0(5, -5, 6);

    if (status[0] != -3 || info != 0x87654321UL || left != -321 || right != 654)
        ++failures;
    if (calls[0] != 1 || calls[1] != 1 || calls[2] != 1 || calls[3] != 1 ||
        calls[4] != 1 || calls[5] != 1 || calls[6] != 1)
        ++failures;
    return failures;
}
