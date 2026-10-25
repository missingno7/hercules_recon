/* Bounded native/source differential for ENG1 RVA 0x2b130.
 * The native path calls the original DLL entry. The source path calls the
 * private C candidate plus the real calibration/action_registry.c helpers.
 * Factory/callback/table-guard functions are failfast fixture spies; all
 * exercised cases must leave their counters at zero. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/eng1/interpreters.h"
extern int __cdecl eng1_native_code_target_002b130(Actor *actor);

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef struct SpawnRecord SpawnRecord;
struct SpawnRecord {
    short x, y, z;
    u16 kind, sprite, state;
};

typedef int (__cdecl *NativeEntry)(void *actor);

unsigned short pending_action_group_count;
PendingActionGroup *pending_action_group_table[20];
unsigned short g_configuration_word_71394;
unsigned long g_configuration_dword_71370;
unsigned long g_configuration_dword_7137c;
unsigned long g_configuration_dword_7139c;

static HMODULE g_native_module;
static int g_spawn_calls;
static int g_callback_guard_calls;
static int g_remove_calls;

void eng1_guard_worklist_table_read(const void *base, unsigned long index,
                                    unsigned long stride,
                                    unsigned long width, const char *site)
{
    (void)base;
    (void)index;
    (void)stride;
    (void)width;
    (void)site;
    ++g_callback_guard_calls;
}

void *__cdecl spawn_normal(long x, long y, long z, unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++g_spawn_calls;
    return 0;
}

void *__cdecl spawn_middle_277e0(long x, long y, long z,
                                  unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++g_spawn_calls;
    return 0;
}

void *__cdecl spawn_kind(long x, long y, long z, unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++g_spawn_calls;
    return 0;
}

int __cdecl remove_actor_variant(void *actor)
{
    (void)actor;
    ++g_remove_calls;
    return 0;
}

static u8 *native_rva(u32 rva)
{
    return (u8 *)(void *)g_native_module + rva;
}

static void set_native_word(u32 rva, u16 value)
{
    *(volatile u16 *)(void *)native_rva(rva) = value;
}

static void set_native_dword(u32 rva, u32 value)
{
    *(volatile u32 *)(void *)native_rva(rva) = value;
}

static void initialize_case(const char *case_name, u8 *actor,
                            SpawnRecord records[2], u8 group_bytes[0x20],
                            void **marker, u16 *table_count, u16 *capacity)
{
    int i;

    for (i = 0; i < 0x124; ++i)
        actor[i] = (u8)(0x39U + (unsigned int)i * 11U +
                        ((unsigned int)i >> 3));
    memset(records, 0, sizeof(SpawnRecord) * 2U);
    memset(group_bytes, 0xcc, 0x20U);
    *(long *)(void *)group_bytes = -1L;
    *marker = (void *)0x12345678UL;
    *(u32 *)(void *)(actor + 0x118) = (u32)(void *)records;
    *(u32 *)(void *)(actor + 0x120) = (u32)(void *)*marker;

    *table_count = 1U;
    *capacity = 4U;
    if (strcmp(case_name, "early-null") == 0) {
        *(u32 *)(void *)(actor + 0x118) = 0UL;
    } else if (strcmp(case_name, "acquire-null") == 0) {
        records[0].kind = 0U;
        *table_count = 0U;
    } else if (strcmp(case_name, "empty") == 0) {
        records[0].kind = 0U;
    } else if (strcmp(case_name, "capacity") == 0) {
        records[0].kind = 1U;
        *capacity = 0U;
    } else if (strcmp(case_name, "unsupported") == 0) {
        records[0].x = -3;
        records[0].y = 5;
        records[0].z = -7;
        records[0].kind = 0x1001U;
        records[0].sprite = 0x4567U;
        records[0].state = 0x89abU;
    }
}

static unsigned int owner_code(const u8 *actor, const void *group,
                               const void *marker)
{
    u32 address = *(const u32 *)(const void *)(actor + 0x120);
    if (address == (u32)group) return 2U;
    if (address == (u32)marker) return 1U;
    if (address == 0UL) return 0U;
    return 3U;
}

static void print_normalized_actor(const u8 *actor, unsigned int owner)
{
    unsigned int i;
    for (i = 0; i < 0x124U; ++i) {
        unsigned int value = actor[i];
        if (i >= 0x118U && i < 0x11cU)
            value = 0x52U;
        else if (i >= 0x120U && i < 0x124U)
            value = owner == 2U ? 0x47U : owner == 1U ? 0x49U :
                    owner == 0U ? 0x00U : 0x3fU;
        printf("%02x", value);
    }
}

static void print_group(const u8 *group_bytes)
{
    unsigned int i;
    for (i = 0; i < 0x20U; ++i)
        printf("%02x", group_bytes[i]);
}

int main(int argc, char **argv)
{
    u8 actor[0x124];
    SpawnRecord records[2];
    u8 group_bytes[0x20];
    void *marker;
    u16 table_count;
    u16 capacity;
    unsigned int result;
    unsigned int owner;
    u16 saved_group_count, saved_capacity;
    u32 saved_group_pointer, saved_normal, saved_middle, saved_alternate;

    if (argc != 4) {
        fprintf(stderr, "usage: neutralfixture.exe ENG1.DLL native|source case\n");
        return 2;
    }
    if (strcmp(argv[2], "native") != 0 && strcmp(argv[2], "source") != 0) {
        fprintf(stderr, "invalid execution mode\n");
        return 2;
    }
    g_native_module = LoadLibraryA(argv[1]);
    if (g_native_module == 0) {
        fprintf(stderr, "LoadLibraryA failed: %lu\n", GetLastError());
        return 3;
    }

    initialize_case(argv[3], actor, records, group_bytes, &marker,
                    &table_count, &capacity);
    pending_action_group_count = table_count;
    pending_action_group_table[0] = (PendingActionGroup *)(void *)group_bytes;
    g_configuration_word_71394 = capacity;
    g_configuration_dword_71370 = 0UL;
    g_configuration_dword_7137c = 0UL;
    g_configuration_dword_7139c = 0UL;

    saved_group_count = *(u16 *)(void *)native_rva(0x71380UL);
    saved_capacity = *(u16 *)(void *)native_rva(0x71394UL);
    saved_group_pointer = *(u32 *)(void *)native_rva(0x713e0UL);
    saved_normal = *(u32 *)(void *)native_rva(0x71370UL);
    saved_middle = *(u32 *)(void *)native_rva(0x7137cUL);
    saved_alternate = *(u32 *)(void *)native_rva(0x7139cUL);
    set_native_word(0x71380UL, table_count);
    set_native_word(0x71394UL, capacity);
    set_native_dword(0x713e0UL, (u32)(void *)group_bytes);
    set_native_dword(0x71370UL, 0UL);
    set_native_dword(0x7137cUL, 0UL);
    set_native_dword(0x7139cUL, 0UL);

    g_spawn_calls = 0;
    g_callback_guard_calls = 0;
    g_remove_calls = 0;
    if (strcmp(argv[2], "native") == 0) {
        NativeEntry entry = (NativeEntry)(void *)native_rva(0x2b130UL);
        result = (unsigned int)entry(actor);
    } else {
        result = (unsigned int)eng1_native_code_target_002b130((Actor *)(void *)actor);
    }
    owner = owner_code(actor, group_bytes, marker);
    printf("case=%s mode=%s result=%u owner=%u spawn=%d guard=%d remove=%d actor=",
           argv[3], argv[2], result, owner, g_spawn_calls,
           g_callback_guard_calls, g_remove_calls);
    print_normalized_actor(actor, owner);
    printf(" group=");
    print_group(group_bytes);
    printf("\n");
    set_native_word(0x71380UL, saved_group_count);
    set_native_word(0x71394UL, saved_capacity);
    set_native_dword(0x713e0UL, saved_group_pointer);
    set_native_dword(0x71370UL, saved_normal);
    set_native_dword(0x7137cUL, saved_middle);
    set_native_dword(0x7139cUL, saved_alternate);
    if (!FreeLibrary(g_native_module)) return 4;
    return 0;
}
