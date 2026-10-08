#include <stdio.h>

extern int __cdecl frame_chain_write(void *first, unsigned long raw_count);

static int checks;
static int failures;

#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        printf("FAIL line %d: %s\n", __LINE__, #condition); \
    } \
} while (0)

static void fill_links(void **links, unsigned long count, void *value)
{
    unsigned long i;
    for (i = 0; i < count; ++i)
        links[i] = value;
}

int main(void)
{
    void *small[4];
    void *large[0x501];
    int marker;
    void *sentinel;
    unsigned long i;

    marker = 0;
    sentinel = &marker;

    fill_links(small, 4, sentinel);
    CHECK(frame_chain_write(small, 0) == 0);
    CHECK(small[0] == 0);
    CHECK(small[1] == sentinel);

    fill_links(small, 4, sentinel);
    CHECK(frame_chain_write(small, 1) == 0);
    CHECK(small[0] == 0);
    CHECK(small[1] == sentinel);

    fill_links(small, 4, sentinel);
    CHECK(frame_chain_write(small, 2) == 0);
    CHECK(small[0] == &small[1]);
    CHECK(small[1] == 0);
    CHECK(small[2] == sentinel);

    fill_links(large, 0x501, sentinel);
    CHECK(frame_chain_write(large, 0x500) == 0);
    for (i = 0; i < 0x4ff; ++i)
        CHECK(large[i] == &large[i + 1]);
    CHECK(large[0x4ff] == 0);
    CHECK(large[0x500] == sentinel);

    fill_links(small, 4, sentinel);
    CHECK(frame_chain_write(small, 0x80000001UL) == 0);
    CHECK(small[0] == 0);
    CHECK(small[1] == sentinel);

    fill_links(small, 4, sentinel);
    CHECK(frame_chain_write(small, 0xffffffffUL) == 0);
    CHECK(small[0] == 0);
    CHECK(small[1] == sentinel);

    printf("frame chain writer fixture: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
