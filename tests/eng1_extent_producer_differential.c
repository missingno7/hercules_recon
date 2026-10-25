#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/extent_producer_30e40.h"
#include "../src/eng1/resource_runtime_data.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef int (__cdecl *ExtentFn)(void *, void *, u32);
extern ActorResource g_actor_resources[81];

static HMODULE oracle;
static unsigned int cases, checks, failures;

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void *native_rva(u32 rva)
{
    return (void *)((u8 *)oracle + rva);
}

static void fail_unexpected_dependency(unsigned long rva, const char *reason)
{
    printf("UNEXPECTED incomplete path %08lx: %s\n", rva, reason);
    ExitProcess(97U);
}

void __cdecl eng1_incomplete_behavior(unsigned long rva, const char *reason)
{
    fail_unexpected_dependency(rva, reason);
}

int __cdecl ensure_actor_resource_ready(unsigned long resource_id,
                                        int metadata_index)
{
    printf("UNEXPECTED resource readiness call id=%lu metadata=%d\n",
           resource_id, metadata_index);
    ExitProcess(98U);
    return 0;
}

static int make_writable(void *pointer, unsigned long bytes, DWORD *old_protect)
{
    return VirtualProtect(pointer, bytes, PAGE_READWRITE, old_protect) != 0;
}

static void write_u32(u8 *destination, u32 value)
{
    *(u32 *)(void *)destination = value;
}

static void set_actor(void *actor, u8 resource_id, u16 metadata_index,
                      u8 flags, u16 scale_a, u16 scale_b, u16 scale_c)
{
    u8 *bytes = (u8 *)actor;
    memset(bytes, 0, 256U);
    bytes[0x22] = resource_id;
    *(u16 *)(void *)(bytes + 0x34) = metadata_index;
    bytes[0x54] = flags;
    *(u16 *)(void *)(bytes + 0x3a) = scale_a;
    *(u16 *)(void *)(bytes + 0x3c) = scale_b;
    *(u16 *)(void *)(bytes + 0x3e) = scale_c;
}

static void seed_profile_root(u8 *profile, u8 count)
{
    memset(profile, 0, 1024U);
    write_u32(profile + 4, 0x20UL);
    profile[0x26] = count;
    write_u32(profile + 0x28, 0x01020304UL);
    write_u32(profile + 0x2c, 0x05060708UL);
    write_u32(profile + 0x30, 0x090a0b0cUL);
    write_u32(profile + 0x34, 0x0d0e0f10UL);
    write_u32(profile + 0x38, 0x11121314UL);
}

static unsigned long seed_metadata(u8 *metadata, unsigned long bytes,
                                   unsigned short entry_count,
                                   u8 record_count)
{
    unsigned long metadata_base = (unsigned long)entry_count * 4UL + 8UL;
    u8 *record;
    memset(metadata, 0, bytes);
    write_u32(metadata + metadata_base + 4UL, 0x1000UL);
    record = metadata + metadata_base + 16UL;
    record[1] = record_count;
    record[2] = 0x80U;
    record[3] = 0x20U;
    record[4] = 0xfeU;
    record[5] = 0x60U;
    record[6] = 0x7fU;
    record[7] = 0x81U;
    record[8] = 0x40U;
    record[9] = 0xf0U;
    record[10] = 0x82U;
    record[11] = 0x20U;
    record[12] = 0x7eU;
    record[13] = 0x40U;
    return metadata_base;
}

static void compare_call(ExtentFn native_fn, ExtentFn source_fn,
                         void *actor, u32 mode, const char *label)
{
    u8 native_output[16];
    u8 source_output[16];
    int native_result;
    int source_result;

    memset(native_output, 0xa5, sizeof(native_output));
    memset(source_output, 0xa5, sizeof(source_output));
    native_result = native_fn(actor, native_output, mode);
    source_result = source_fn(actor, source_output, mode);
    check(source_result == native_result, label);
    if (memcmp(source_output, native_output, sizeof(native_output)) != 0) {
        unsigned int i;
        printf("OUTPUT-DIFF %s mode=%lu result native=%d source=%d\n",
               label, mode, native_result, source_result);
        printf(" native:");
        for (i = 0; i < sizeof(native_output); ++i)
            printf(" %02x", native_output[i]);
        printf("\n source:");
        for (i = 0; i < sizeof(source_output); ++i)
            printf(" %02x", source_output[i]);
        printf("\n");
    }
    check(memcmp(source_output, native_output, sizeof(native_output)) == 0,
          "native/source 16-byte output and untouched tail agree");
    ++cases;
}

static int test_native_and_source(void)
{
    ExtentFn native_extent = (ExtentFn)native_rva(0x30e40UL);
    ExtentFn native_profile = (ExtentFn)native_rva(0x31ec0UL);
    u8 actor[256];
    u8 profile[1024];
    u8 metadata[4096];
    ActorResource *native_resources =
        (ActorResource *)native_rva(0x5cee0UL);
    ActorResource saved_native_row;
    ActorResource saved_source_row;
    unsigned char **native_profile_slot =
        (unsigned char **)native_rva(0x70d2cUL);
    unsigned char *saved_native_profile_root;
    DWORD row_protect, slot_protect;
    unsigned long metadata_base;
    int result;

    if (!make_writable(&native_resources[2], sizeof(ActorResource), &row_protect) ||
        !make_writable(native_profile_slot, sizeof(void *), &slot_protect)) {
        printf("VirtualProtect could not open the loaded data fixture\n");
        return 2;
    }
    saved_native_row = native_resources[2];
    saved_source_row = g_actor_resources[2];
    saved_native_profile_root = *native_profile_slot;

    seed_profile_root(profile, 2U);
    g_header_pointer_70d2c = profile;
    *native_profile_slot = profile;
    set_actor(actor, 0U, 1U, 0U, 0x0100U, 0x0100U, 0x0100U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 0UL, "profile-table mode zero preserves native return");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 1UL, "profile-table mode one preserves selected-record result");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 2UL, "profile-table mode two selects the next profile record");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 3UL, "profile-table mode beyond count rejects without writes");

    set_actor(actor, 0U, 1U, 0x80U, 0x0100U, 0x0200U, 0x0080U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 1UL, "profile-table flagged three-scale path matches");
    set_actor(actor, 0U, 1U, 0x80U, 0x0100U, 0U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 1UL, "profile-table combined 0x100 scale sentinel skips scaling");
    seed_profile_root(profile, 0U);
    compare_call(native_profile, profile_extent_31ec0,
                 actor, 0UL, "zero profile count rejects mode zero without writes");
    seed_profile_root(profile, 2U);
    set_actor(actor, 0U, 1U, 0U, 0x0180U, 0x0100U, 0U);
    compare_call(native_profile, profile_extent_31ec0,
                 actor, 0UL, "direct profile helper unflagged fixed-point scaling matches");
    seed_profile_root(profile, 1U);
    write_u32(profile + 0x28, 0x7f7fff7fUL);
    write_u32(profile + 0x2c, 0x7f7fff7fUL);
    write_u32(profile + 0x30, 0x01fffe80UL);
    set_actor(actor, 0U, 1U, 0U, 0x0100U, 0U, 0U);
    compare_call(native_profile, profile_extent_31ec0,
                 actor, 0UL, "profile helper preserves overflowing low-WORD sums");
    set_actor(actor, 0U, 1U, 0U, 0xffffU, 0U, 0U);
    compare_call(native_profile, profile_extent_31ec0,
                 actor, 0UL, "profile helper wraps signed 32-bit fixed-point products");

    metadata_base = seed_metadata(metadata, sizeof(metadata),
                                  g_actor_resources[2].entry_count, 2U);
    g_actor_resources[2].references = (ActorReference *)(void *)metadata;
    g_actor_resources[2].residency_state = 3U;
    native_resources[2].references = (ActorReference *)(void *)metadata;
    native_resources[2].residency_state = 3U;
    (void)metadata_base;
    set_actor(actor, 2U, 1U, 0U, 0x0100U, 0x0100U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 0UL, "resource metadata default record matches native");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 1UL, "resource metadata explicit first record matches native");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 2UL, "resource metadata second record matches native");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 3UL, "resource metadata mode after count rejects without writes");

    set_actor(actor, 2U, 1U, 0x80U, 0x0180U, 0x00c0U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 2UL, "resource metadata split fixed-point scales match native");
    set_actor(actor, 2U, 1U, 0x80U, 0x0180U, 0x0100U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 2UL, "resource metadata +0x3c sentinel bypasses second scale pair");
    (void)seed_metadata(metadata, sizeof(metadata),
                        g_actor_resources[2].entry_count, 0xfeU);
    set_actor(actor, 2U, 1U, 0U, 0x0100U, 0U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 0UL, "negative signed metadata count remains full return value");
    compare_call(native_extent, extent_producer_30e40,
                 actor, 1UL, "positive mode rejects negative signed metadata count");
    set_actor(actor, 2U, 0xffffU, 0U, 0x0100U, 0U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 0UL, "resource table count rejects oversized metadata index");

    set_actor(actor, 2U, 0U, 0U, 0x0100U, 0U, 0U);
    compare_call(native_extent, extent_producer_30e40,
                 actor, 0UL, "metadata-index zero path returns zero without writes");

    native_resources[2] = saved_native_row;
    g_actor_resources[2] = saved_source_row;
    *native_profile_slot = saved_native_profile_root;
    g_header_pointer_70d2c = 0;
    (void)VirtualProtect(&native_resources[2], sizeof(ActorResource),
                         row_protect, &row_protect);
    (void)VirtualProtect(native_profile_slot, sizeof(void *),
                         slot_protect, &slot_protect);
    result = (int)failures;
    return result == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    oracle = LoadLibraryA(argv[1]);
    if (oracle == 0) {
        printf("LoadLibrary failed: %lu\n", (u32)GetLastError());
        return 3;
    }
    if (test_native_and_source() != 0)
        return 4;
    FreeLibrary(oracle);
    printf("extent_producer_30e40_cases=%u checks=%u failures=%u\n",
           cases, checks, failures);
    return failures == 0 ? 0 : 1;
}
