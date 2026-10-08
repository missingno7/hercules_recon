#include "../calibration/archive_positions.c"

#include <stdio.h>

int g_archive_logical_positions[1024];
static int expected_positions[1024];
static int checks;
static int failures;

static void check(const char *name, int passed)
{
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL: %s\n", name);
    }
}

static int expected_position(int handle)
{
    return expected_positions[handle - 1];
}

static int expected_close(int handle)
{
    expected_positions[handle - 1] = -1;
    return 0;
}

static void initialize_fixture_positions(void)
{
    int i;
    for (i = 0; i < 1024; ++i) {
        g_archive_logical_positions[i] = -1;
        expected_positions[i] = -1;
    }
}

int main(void)
{
    int i;
    initialize_fixture_positions();
    check("first one-based handle starts at sentinel", archive_position(1) == expected_position(1));
    check("last observed initialized handle starts at sentinel", archive_position(1024) == expected_position(1024));

    g_archive_logical_positions[0] = (-2147483647 - 1);
    expected_positions[0] = (-2147483647 - 1);
    g_archive_logical_positions[1] = 0x12345678;
    expected_positions[1] = 0x12345678;
    g_archive_logical_positions[512] = -123456789;
    expected_positions[512] = -123456789;
    g_archive_logical_positions[1023] = -0x1234567;
    expected_positions[1023] = -0x1234567;

    check("handle one returns signed INT_MIN unchanged", archive_position(1) == expected_position(1));
    check("handle two addresses the second dword", archive_position(2) == expected_position(2));
    check("middle handle preserves a negative logical position", archive_position(513) == expected_position(513));
    check("handle 1024 reaches the last observed initialized dword", archive_position(1024) == expected_position(1024));

    check("close handle one returns zero", archive_close(1) == expected_close(1));
    check("close stores -1 in only the first slot", g_archive_logical_positions[0] == expected_positions[0] && g_archive_logical_positions[1] == 0x12345678);
    check("position sees the close sentinel", archive_position(1) == expected_position(1));

    check("close handle 1024 returns zero", archive_close(1024) == expected_close(1024));
    check("close stores -1 in only the final tested slot", g_archive_logical_positions[1023] == expected_positions[1023] && g_archive_logical_positions[512] == -123456789);
    check("position sees the final-slot close sentinel", archive_position(1024) == expected_position(1024));

    check("repeated close is stable and returns zero", archive_close(1) == expected_close(1));
    check("interior signed position is unchanged by other closes", archive_position(513) == expected_position(513));

    for (i = 2; i <= 1023; i += 127) {
        check("interior dword value round-trips", archive_position(i) == expected_position(i));
    }

    printf("archive positions fixture: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
