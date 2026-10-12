#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/read_guard.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef void (__cdecl *NativeTarget)(Actor *);
typedef void (__cdecl *CandidateTarget)(Actor *);

typedef struct GuardedActor GuardedActor;
struct GuardedActor {
    u8 before[16];
    Actor actor;
    u8 after[16];
};

typedef struct DataBackup DataBackup;
struct DataBackup {
    u8 *base;
    DWORD size;
    u8 *copy;
};

static HMODULE native_module;
static unsigned int actor_comparisons;
static int remove_calls;
static int unexpected_calls;
static DataBackup backups[32];
static unsigned int backup_count;
static u8 native_code_before[272];
static u8 *ignored_native_cells[2];
static u8 ignored_native_values[2][4];
static unsigned int ignored_native_sizes[2];
static unsigned int ignored_native_count;

unsigned short pending_action_group_count;
PendingActionGroup *pending_action_group_table[20];
unsigned short g_configuration_word_71394;
unsigned long g_configuration_dword_71370;
unsigned long g_configuration_dword_7137c;
unsigned long g_configuration_dword_7139c;

extern void __cdecl eng1_native_code_target_03ca80(Actor *actor);

static u8 *native_rva(u32 rva)
{
    return (u8 *)(void *)native_module + rva;
}

static void fail(const char *message, unsigned int index)
{
    printf("FAIL case=%u %s\n", index, message);
    ExitProcess(1);
}

static void ignore_native_cell(u8 *address, unsigned int size)
{
    unsigned int slot = ignored_native_count++;
    if (slot >= 2U || size > 4U)
        fail("too many controlled native data cells", slot);
    ignored_native_cells[slot] = address;
    ignored_native_sizes[slot] = size;
    memcpy(ignored_native_values[slot], address, size);
}

/* Source callback runs the actual canonical 2b130 and registry providers. */

void __cdecl eng1_native_code_target_0041000(Actor *actor, u32 category)
{
    (void)actor;
    (void)category;
    ++unexpected_calls;
}

void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    (void)actor;
    (void)index;
    ++unexpected_calls;
}

int __cdecl remove_actor(Actor *actor)
{
    ++remove_calls;
    if (actor->kind != 0U)
        fail("fixture exceeded kind-zero remove_actor precondition", 0);
    return 1;
}

void eng1_guard_worklist_table_read(const void *base, unsigned long index,
                                    unsigned long stride, unsigned long width,
                                    const char *site)
{
    (void)base; (void)index; (void)stride; (void)width; (void)site;
    ++unexpected_calls;
}

void *__cdecl spawn_normal(long x, long y, long z, unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++unexpected_calls;
    return 0;
}

void *__cdecl spawn_middle_277e0(long x, long y, long z, unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++unexpected_calls;
    return 0;
}

void *__cdecl spawn_kind(long x, long y, long z, unsigned short kind)
{
    (void)x; (void)y; (void)z; (void)kind;
    ++unexpected_calls;
    return 0;
}

int __cdecl remove_actor_variant(void *actor)
{
    (void)actor;
    ++unexpected_calls;
    return 0;
}

void __cdecl eng1_incomplete_behavior(u32 rva, const char *reason)
{
    printf("FAIL unexpected incomplete guard rva=%08lx: %s\n", rva, reason);
    ExitProcess(1);
}

static void backup_writable_sections(void)
{
    PIMAGE_DOS_HEADER dos;
    PIMAGE_NT_HEADERS nt;
    PIMAGE_SECTION_HEADER section;
    unsigned int i;

    dos = (PIMAGE_DOS_HEADER)(void *)native_module;
    nt = (PIMAGE_NT_HEADERS)(void *)((u8 *)(void *)native_module + dos->e_lfanew);
    section = IMAGE_FIRST_SECTION(nt);
    backup_count = 0;
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        DWORD size;
        if ((section->Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
            continue;
        size = section->Misc.VirtualSize;
        if (size == 0)
            size = section->SizeOfRawData;
        if (size == 0 || backup_count >= 32)
            fail("invalid writable-section backup span", i);
        backups[backup_count].base = (u8 *)(void *)native_module +
                                     section->VirtualAddress;
        backups[backup_count].size = size;
        backups[backup_count].copy = (u8 *)malloc(size);
        if (backups[backup_count].copy == 0)
            fail("writable-section backup allocation failed", i);
        memcpy(backups[backup_count].copy, backups[backup_count].base, size);
        ++backup_count;
    }
}

static unsigned int restore_writable_sections(void)
{
    unsigned int i;
    unsigned int changed_sections = 0;
    for (i = 0; i < backup_count; ++i) {
        DWORD byte_index;
        int section_changed = 0;
        for (byte_index = 0; byte_index < backups[i].size; ++byte_index) {
            unsigned int slot;
            u8 *address = backups[i].base + byte_index;
            int controlled_cell = 0;
            for (slot = 0; slot < ignored_native_count; ++slot) {
                unsigned int cell_index;
                for (cell_index = 0; cell_index < ignored_native_sizes[slot];
                     ++cell_index) {
                    if (address == ignored_native_cells[slot] + cell_index) {
                        if (*address != ignored_native_values[slot][cell_index])
                            section_changed = 1;
                        controlled_cell = 1;
                        break;
                    }
                }
                if (controlled_cell)
                    break;
            }
            if (!controlled_cell && *address != backups[i].copy[byte_index])
                section_changed = 1;
        }
        if (section_changed)
            ++changed_sections;
        memcpy(backups[i].base, backups[i].copy, backups[i].size);
    }
    ignored_native_count = 0;
    return changed_sections;
}

static void set_native_word(u32 rva, u16 value)
{
    *(volatile u16 *)(void *)native_rva(rva) = value;
}

static void set_native_dword(u32 rva, u32 value)
{
    *(volatile u32 *)(void *)native_rva(rva) = value;
}

static void free_writable_backups(void)
{
    unsigned int i;
    for (i = 0; i < backup_count; ++i) {
        free(backups[i].copy);
        backups[i].copy = 0;
    }
}

static void initialize_actor(GuardedActor *box, u16 high_word)
{
    u32 x_bits;
    memset(box, 0xa7, sizeof(*box));
    memset(box->before, 0x39, sizeof(box->before));
    memset(box->after, 0xc6, sizeof(box->after));
    x_bits = ((u32)high_word << 16) + 0x00020000UL;
    box->actor.x = (long)x_bits;
    box->actor.y = 0x00900000L;
    box->actor.category = 0x5aU;
    box->actor.kind = 0U;
    box->actor.initial_steps = 0;
    *(u32 *)(void *)((u8 *)(void *)&box->actor + 0x120U) = 0x12345678UL;
}

static int expected_category(u16 high_word)
{
    static const u16 thresholds[18] = {
        0x0500U, 0x0950U, 0x0c50U, 0x0f50U, 0x1950U, 0x1df0U,
        0x2280U, 0x2f80U, 0x3d00U, 0x4540U, 0x4800U, 0x4e20U,
        0x57beU, 0x6040U, 0x65faU, 0x7700U, 0x7d00U, 0xffffU
    };
    unsigned int row;
    short value = (short)high_word;
    if (value <= 0)
        return 0xff;
    for (row = 0; row < 18; ++row) {
        if ((u16)value < thresholds[row])
            return (int)row;
        if (thresholds[row] == 0xffffU)
            return 0xff;
    }
    return 0xff;
}

static void check_case(unsigned int case_index, u16 high_word,
                       NativeTarget native_target,
                       CandidateTarget candidate_target)
{
    GuardedActor native_box;
    GuardedActor candidate_box;
    unsigned int changed_sections;
    unsigned int guard_index;
    u8 *native_code;
    int expected;

    initialize_actor(&native_box, high_word);
    initialize_actor(&candidate_box, high_word);
    native_code = native_rva(0x3ca80UL);
    memcpy(native_code_before, native_code, sizeof(native_code_before));
    native_target(&native_box.actor);
    changed_sections = restore_writable_sections();
    if (changed_sections != 0U)
        fail("oracle target changed a loaded writable data section", case_index);
    if (memcmp(native_code_before, native_code, sizeof(native_code_before)) != 0)
        fail("oracle code bytes changed during native call", case_index);

    candidate_target(&candidate_box.actor);
    if (remove_calls != (int)(case_index + 1U))
        fail("candidate helper/remove call count mismatch", case_index);
    if (unexpected_calls != 0)
        fail("candidate entered untested child-processing dependency", case_index);
    if (memcmp(&native_box.actor, &candidate_box.actor, sizeof(Actor)) != 0)
        fail("native/candidate Actor bytes differ", case_index);
    ++actor_comparisons;
    for (guard_index = 0; guard_index < sizeof(native_box.before); ++guard_index) {
        if (native_box.before[guard_index] != 0x39U ||
            candidate_box.before[guard_index] != 0x39U ||
            native_box.after[guard_index] != 0xc6U ||
            candidate_box.after[guard_index] != 0xc6U)
            fail("actor-adjacent canary changed", case_index);
    }
    if (*((u8 *)(void *)&native_box.actor + 0x21U) !=
        (u8)expected_category(high_word))
        fail("native category did not match the measured classifier table", case_index);
    if (native_box.actor.category != 0x5aU)
        fail("native write unexpectedly changed Actor.category at +0x22", case_index);
    if (native_box.actor.x != (long)(((u32)high_word << 16) & 0xffffffffUL))
        fail("native x subtraction mismatch", case_index);
    if (*(u32 *)(void *)((u8 *)(void *)&native_box.actor + 0xd8U) != 0x00100000UL)
        fail("native d8 translation mismatch", case_index);
    if (*(u32 *)(void *)((u8 *)(void *)&native_box.actor + 0xdcU) != 0x40UL ||
        *(u32 *)(void *)((u8 *)(void *)&native_box.actor + 0xe0U) != 0xa0UL)
        fail("native dc/e0 setup mismatch", case_index);
    if (*(u32 *)(void *)((u8 *)(void *)&native_box.actor + 0x120U) != 0x12345678UL)
        fail("native 2b130 null-input early path changed actor+0x120", case_index);
}

static void check_empty_success(unsigned int case_index,
                                NativeTarget native_target,
                                CandidateTarget candidate_target)
{
    GuardedActor native_box;
    GuardedActor candidate_box;
    u8 record_bytes[12];
    u8 group_bytes[0x20];
    u8 native_group_after[0x20];
    u16 original_count;
    u32 original_table;
    unsigned int changed_sections;
    unsigned int guard_index;
    u8 *native_code = native_rva(0x3ca80UL);

    initialize_actor(&native_box, 0x0001U);
    initialize_actor(&candidate_box, 0x0001U);
    memset(record_bytes, 0, sizeof(record_bytes));
    memset(group_bytes, 0xcc, sizeof(group_bytes));
    *(long *)(void *)group_bytes = -1L;
    native_box.actor.initial_steps = record_bytes;
    candidate_box.actor.initial_steps = record_bytes;

    original_count = *(volatile u16 *)(void *)native_rva(0x71380UL);
    original_table = *(volatile u32 *)(void *)native_rva(0x713e0UL);
    set_native_word(0x71380UL, 1U);
    set_native_dword(0x713e0UL, (u32)(void *)group_bytes);
    ignore_native_cell(native_rva(0x71380UL), 2U);
    ignore_native_cell(native_rva(0x713e0UL), 4U);

    memcpy(native_code_before, native_code, sizeof(native_code_before));
    native_target(&native_box.actor);
    if (*(long *)(void *)group_bytes != 0L)
        fail("native 2b130 empty-record path did not change group count -1 to 0",
             case_index);
    if (*(u32 *)(void *)((u8 *)(void *)&native_box.actor + 0x120U) !=
        (u32)(void *)group_bytes)
        fail("native 2b130 empty-record path did not install group at +0x120",
             case_index);
    memcpy(native_group_after, group_bytes, sizeof(native_group_after));
    changed_sections = restore_writable_sections();
    if (changed_sections != 0U)
        fail("native target changed writable data outside controlled input cells",
             case_index);
    if (*(volatile u16 *)(void *)native_rva(0x71380UL) != original_count ||
        *(volatile u32 *)(void *)native_rva(0x713e0UL) != original_table)
        fail("controlled native table cells were not restored", case_index);
    if (memcmp(native_code_before, native_code, sizeof(native_code_before)) != 0)
        fail("oracle code bytes changed during empty-success call", case_index);

    *(long *)(void *)group_bytes = -1L;
    pending_action_group_count = 1U;
    pending_action_group_table[0] =
        (PendingActionGroup *)(void *)group_bytes;
    g_configuration_word_71394 = 4U;
    g_configuration_dword_71370 = 0UL;
    g_configuration_dword_7137c = 0UL;
    g_configuration_dword_7139c = 0UL;
    candidate_target(&candidate_box.actor);
    if (remove_calls != 18 || unexpected_calls != 0)
        fail("empty-success source path made an unexpected helper call",
             case_index);
    if (memcmp(&native_box.actor, &candidate_box.actor, sizeof(Actor)) != 0)
        fail("empty-success native/candidate Actor bytes differ", case_index);
    ++actor_comparisons;
    if (memcmp(native_group_after, group_bytes, sizeof(native_group_after)) != 0)
        fail("empty-success native/candidate group bytes differ", case_index);
    if (*(long *)(void *)group_bytes != 0L)
        fail("source 2b130 empty-record path did not change group count -1 to 0",
             case_index);
    if (*(u32 *)(void *)((u8 *)(void *)&candidate_box.actor + 0x120U) !=
        (u32)(void *)group_bytes)
        fail("source 2b130 empty-record path did not install group at +0x120",
             case_index);
    for (guard_index = 0; guard_index < sizeof(native_box.before); ++guard_index) {
        if (native_box.before[guard_index] != 0x39U ||
            candidate_box.before[guard_index] != 0x39U ||
            native_box.after[guard_index] != 0xc6U ||
            candidate_box.after[guard_index] != 0xc6U)
            fail("empty-success actor-adjacent canary changed", case_index);
    }
    if (*((u8 *)(void *)&native_box.actor + 0x21U) != 0U ||
        native_box.actor.category != 0x5aU)
        fail("empty-success class byte/category-byte discriminator failed",
             case_index);
}

int main(int argc, char **argv)
{
    static const u16 high_words[] = {
        0x0001U, 0x0500U, 0x0950U, 0x0c50U, 0x0f50U, 0x1950U,
        0x1df0U, 0x2280U, 0x2f80U, 0x3d00U, 0x4540U, 0x4800U,
        0x4e20U, 0x57beU, 0x6040U, 0x65faU, 0x7700U, 0x7d00U
    };
    unsigned int i;
    NativeTarget native_target;
    CandidateTarget candidate_target;
    unsigned int cases = sizeof(high_words) / sizeof(high_words[0]);

    if (argc != 2) {
        fprintf(stderr, "usage: eng1_call_3ca80_fixture.exe ENG1.DLL\n");
        return 2;
    }
    native_module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (native_module == 0) {
        fprintf(stderr, "LoadLibraryExA failed: %lu\n", GetLastError());
        return 3;
    }
    backup_writable_sections();
    native_target = (NativeTarget)(void *)native_rva(0x3ca80UL);
    candidate_target = eng1_native_code_target_03ca80;

    for (i = 0; i < cases; ++i)
        check_case(i, high_words[i], native_target, candidate_target);
    check_empty_success(cases, native_target, candidate_target);

    free_writable_backups();
    if (!FreeLibrary(native_module)) fail("FreeLibrary failed", 19);
    printf("PASS native_entry=0x3ca80 abi=Actor*_cdecl cases=%u classes=0..17 empty_success=1 native_writable_sections_restored=1 child_iteration=uncovered\n",
           cases + 1U);
    printf("forest_actor_callback_native_cases=%u checks=%u failures=0\n", cases+1U, actor_comparisons);
    return 0;
}
