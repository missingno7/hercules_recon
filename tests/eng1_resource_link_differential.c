#include <windows.h>
#include <stdio.h>
#include <string.h>

#define NATIVE_LINK_ROWS_RVA 0x72880UL
#define NATIVE_ENTRY_RVA 0x39b0UL
#define LINK_ROW_COUNT 256UL
#define LINK_ROW_BYTES 0x0cUL

typedef struct ActorResourceLinkRow {
    unsigned short live_mask;
    unsigned short unknown_002;
    unsigned short next[4];
} ActorResourceLinkRow;

typedef char actor_resource_link_row_size_is_12[
    sizeof(ActorResourceLinkRow) == 12 ? 1 : -1];

extern ActorResourceLinkRow g_actor_link_rows[256];
extern void __cdecl eng1_target_039b0(void);

static unsigned long checks;
static unsigned long failures;

static void check(const char *case_name, const char *label, int ok)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL %s: %s\n", case_name, label);
    }
}

static unsigned short expected_descriptor(unsigned long index)
{
    return (unsigned short)(0x008cUL |
                            ((index & 0x08UL) << 1) |
                            ((index & 0x80UL) >> 6));
}

static unsigned short expected_live_mask(unsigned long index)
{
    if (index == 0UL || index == 16UL || index == 4UL || index == 1UL)
        return 0x000fU;
    return 0;
}

static void seed_rows(ActorResourceLinkRow *rows, unsigned short seed)
{
    unsigned long index;
    unsigned long slot;

    for (index = 0; index < LINK_ROW_COUNT; ++index) {
        rows[index].live_mask =
            (unsigned short)(seed ^ (unsigned short)(0x1357U + index * 37UL));
        rows[index].unknown_002 =
            (unsigned short)(seed ^ (unsigned short)(0x2468U + index * 73UL));
        for (slot = 0; slot < 4UL; ++slot) {
            rows[index].next[slot] =
                (unsigned short)(seed ^ (unsigned short)
                    (0x5a31U + index * 13UL + slot * 0x211UL));
        }
    }
}

static int run_seed(HMODULE oracle, const char *case_name,
                    unsigned short seed)
{
    ActorResourceLinkRow *native_rows;
    ActorResourceLinkRow native_before[256];
    ActorResourceLinkRow source_before[256];
    void (__cdecl *native_entry)(void);
    unsigned long index;
    unsigned long slot;

    native_rows = (ActorResourceLinkRow *)
        ((unsigned char *)oracle + NATIVE_LINK_ROWS_RVA);
    native_entry = (void (__cdecl *)(void))
        ((unsigned char *)oracle + NATIVE_ENTRY_RVA);

    seed_rows(native_rows, seed);
    memcpy(native_before, native_rows, sizeof(native_before));
    seed_rows(g_actor_link_rows, seed);
    memcpy(source_before, g_actor_link_rows, sizeof(source_before));

    __try {
        native_entry();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        check(case_name, "unpatched native entry completed", 0);
        return 0;
    }
    eng1_target_039b0();

    check(case_name, "full measured 0xc00-byte tables match",
          memcmp(native_rows, g_actor_link_rows,
                 LINK_ROW_COUNT * LINK_ROW_BYTES) == 0);

    for (index = 0; index < LINK_ROW_COUNT; ++index) {
        unsigned short mask;
        unsigned short descriptor;

        mask = expected_live_mask(index);
        descriptor = expected_descriptor(index);
        check(case_name, "native row mask matches the four observed anchors",
              native_rows[index].live_mask == mask);
        check(case_name, "source row mask matches the four observed anchors",
              g_actor_link_rows[index].live_mask == mask);
        check(case_name, "native descriptor matches the two input-bit fields",
              native_rows[index].unknown_002 == descriptor);
        check(case_name, "source descriptor matches the two input-bit fields",
              g_actor_link_rows[index].unknown_002 == descriptor);
        for (slot = 0; slot < 4UL; ++slot) {
            check(case_name, "native successor key remains unchanged",
                  native_rows[index].next[slot] ==
                      native_before[index].next[slot]);
            check(case_name, "source successor key remains unchanged",
                  g_actor_link_rows[index].next[slot] ==
                      source_before[index].next[slot]);
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    HMODULE oracle;

    if (argc != 2) {
        printf("usage: native_diff ENG1.DLL\n");
        return 2;
    }
    oracle = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (oracle == 0) {
        printf("LoadLibraryEx failed: %lu\n", GetLastError());
        return 3;
    }

    run_seed(oracle, "seed-1357", 0x1357U);
    run_seed(oracle, "seed-ace0", 0xace0U);

    printf("eng1_039b0_native_diff checks=%lu failures=%lu\n",
           checks, failures);
    FreeLibrary(oracle);
    return failures == 0 ? 0 : 1;
}
