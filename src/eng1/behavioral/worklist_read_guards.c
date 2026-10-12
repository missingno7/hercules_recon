/* Exact-identity preflight registry for finite ENG1 worklist table prefixes.
 * Limits are initialized bytes observed through each configuration row's
 * next-root boundary; they do not claim logical runtime capacities. */
#include <stdio.h>
#include "worklist_read_guards.h"
#include "incomplete.h"
#include "row0_layouts.h"

#if defined(ENG1_WORKLIST_GUARD_FIXTURE)
extern unsigned char eng1_fixture_normal_assets[128];
extern unsigned char eng1_fixture_middle_assets[64];
extern unsigned char eng1_fixture_alternate_assets[64];
extern unsigned char eng1_fixture_normal_actions[96];
extern unsigned char eng1_fixture_middle_actions[64];
extern unsigned char eng1_fixture_alternate_actions[64];
#else
extern unsigned char g_eng1_config_assets_067118[];
extern unsigned char g_eng1_config_assets_04dcf8[];
extern unsigned char g_eng1_config_assets_0602c8[];
extern unsigned char g_eng1_config_assets_057bb0[];
extern unsigned char g_eng1_config_assets_0593c0[];
extern unsigned char g_eng1_config_assets_061840[];
extern unsigned char g_eng1_config_assets_06a6b8[];
extern unsigned char g_eng1_action_flags_0679c0[];
extern unsigned char g_eng1_action_flags_04e680[];
extern unsigned char g_eng1_action_flags_060b00[];
extern unsigned char g_eng1_action_flags_058198[];
extern unsigned char g_eng1_action_flags_0599a8[];
extern unsigned char g_eng1_action_flags_062638[];
extern unsigned char g_eng1_action_flags_06ab70[];
extern unsigned char g_eng1_initializer_flags_067de0[];
extern unsigned char g_eng1_initializer_flags_04eb10[];
extern unsigned char g_eng1_initializer_flags_060de0[];
extern unsigned char g_eng1_initializer_flags_058428[];
extern unsigned char g_eng1_initializer_flags_059c48[];
extern unsigned char g_eng1_initializer_flags_062920[];
extern unsigned char g_eng1_initializer_flags_06adf0[];
extern unsigned char g_eng1_callback_record_block_067608[];
extern unsigned char g_eng1_callback_record_block_04e268[];
extern unsigned char g_eng1_callback_record_block_060778[];
extern unsigned char g_eng1_callback_record_prefix_057f10[];
extern unsigned char g_eng1_callback_record_prefix_059720[];
extern unsigned char g_eng1_callback_record_block_061f80[];
extern unsigned char g_eng1_callback_record_block_06a968[];
extern unsigned char g_eng1_callback_record_prefix_067bd0[];
extern unsigned char g_eng1_callback_record_prefix_04e8c8[];
extern unsigned char g_eng1_callback_record_prefix_060c70[];
extern unsigned char g_eng1_callback_record_prefix_0582e0[];
extern unsigned char g_eng1_callback_record_prefix_059af8[];
extern unsigned char g_eng1_callback_record_prefix_062790[];
extern unsigned char g_eng1_callback_record_prefix_06acb0[];
extern unsigned char g_eng1_callback_record_prefix_067f80[];
extern unsigned char g_eng1_callback_record_prefix_04ec50[];
extern unsigned char g_eng1_callback_record_prefix_060f58[];
extern unsigned char g_eng1_callback_record_prefix_0585c0[];
extern unsigned char g_eng1_callback_record_prefix_059de8[];
extern unsigned char g_eng1_callback_record_prefix_062a60[];
extern unsigned char g_eng1_callback_record_prefix_06af60[];
#endif

typedef struct Eng1WorklistReadLimit {
    const void *base;
    unsigned long original_rva;
    unsigned long bytes;
} Eng1WorklistReadLimit;

#define BYTE_OWNER(symbol, rva, size) { (const void *)(symbol), rva##UL, size##UL }
#if defined(ENG1_WORKLIST_GUARD_FIXTURE)
static const Eng1WorklistReadLimit g_worklist_limits[] = {
    BYTE_OWNER(eng1_fixture_normal_assets, 0x5c408, 128),
    BYTE_OWNER(eng1_fixture_middle_assets, 0x5c868, 64),
    BYTE_OWNER(eng1_fixture_alternate_assets, 0x5cae8, 64),
    BYTE_OWNER(eng1_fixture_normal_actions, 0x5c688, 96),
    BYTE_OWNER(eng1_fixture_middle_actions, 0x5c9a8, 64),
    BYTE_OWNER(eng1_fixture_alternate_actions, 0x5cc28, 64)
};
#else
static const Eng1WorklistReadLimit g_worklist_limits[] = {
    /* +0x18: normal 16-byte asset records, rows 0 through 7. */
    BYTE_OWNER(g_eng1_row0_config_action_5c408, 0x5c408, 640),
    BYTE_OWNER(g_eng1_config_assets_067118, 0x67118, 1264),
    BYTE_OWNER(g_eng1_config_assets_04dcf8, 0x4dcf8, 1392),
    BYTE_OWNER(g_eng1_config_assets_0602c8, 0x602c8, 1200),
    BYTE_OWNER(g_eng1_config_assets_057bb0, 0x57bb0, 864),
    BYTE_OWNER(g_eng1_config_assets_0593c0, 0x593c0, 864),
    BYTE_OWNER(g_eng1_config_assets_061840, 0x61840, 1856),
    BYTE_OWNER(g_eng1_config_assets_06a6b8, 0x6a6b8, 688),
    /* +0x20: middle-family 8-byte asset/flag records. */
    BYTE_OWNER(g_eng1_row0_action_flags_5c868, 0x5c868, 320),
    BYTE_OWNER(g_eng1_action_flags_0679c0, 0x679c0, 528),
    BYTE_OWNER(g_eng1_action_flags_04e680, 0x4e680, 584),
    BYTE_OWNER(g_eng1_action_flags_060b00, 0x60b00, 368),
    BYTE_OWNER(g_eng1_action_flags_058198, 0x58198, 328),
    BYTE_OWNER(g_eng1_action_flags_0599a8, 0x599a8, 336),
    BYTE_OWNER(g_eng1_action_flags_062638, 0x62638, 344),
    BYTE_OWNER(g_eng1_action_flags_06ab70, 0x6ab70, 320),
    /* +0x28: alternate-family 8-byte asset/flag records. */
    BYTE_OWNER(g_eng1_row0_initializer_flags_5cae8, 0x5cae8, 320),
    BYTE_OWNER(g_eng1_initializer_flags_067de0, 0x67de0, 416),
    BYTE_OWNER(g_eng1_initializer_flags_04eb10, 0x4eb10, 320),
    BYTE_OWNER(g_eng1_initializer_flags_060de0, 0x60de0, 376),
    BYTE_OWNER(g_eng1_initializer_flags_058428, 0x58428, 408),
    BYTE_OWNER(g_eng1_initializer_flags_059c48, 0x59c48, 416),
    BYTE_OWNER(g_eng1_initializer_flags_062920, 0x62920, 320),
    BYTE_OWNER(g_eng1_initializer_flags_06adf0, 0x6adf0, 368),
    /* +0x1c: normal callback pointers at +0 of complete 12-byte records.
     * Four-byte zero gaps present in some next-root spans are deliberately
     * excluded: they are not admitted as extra table rows. */
    BYTE_OWNER(g_eng1_row0_handler_triples_5c688, 0x5c688, 480),
    BYTE_OWNER(g_eng1_callback_record_block_067608, 0x67608, 948),
    BYTE_OWNER(g_eng1_callback_record_block_04e268, 0x4e268, 1044),
    BYTE_OWNER(g_eng1_callback_record_block_060778, 0x60778, 900),
    BYTE_OWNER(g_eng1_callback_record_prefix_057f10, 0x57f10, 648),
    BYTE_OWNER(g_eng1_callback_record_prefix_059720, 0x59720, 648),
    BYTE_OWNER(g_eng1_callback_record_block_061f80, 0x61f80, 1716),
    BYTE_OWNER(g_eng1_callback_record_block_06a968, 0x6a968, 516),
    /* +0x24: middle callback pointers at +0 of 8-byte records. */
    BYTE_OWNER(g_eng1_row0_callback_pairs_5c9a8, 0x5c9a8, 320),
    BYTE_OWNER(g_eng1_callback_record_prefix_067bd0, 0x67bd0, 528),
    BYTE_OWNER(g_eng1_callback_record_prefix_04e8c8, 0x4e8c8, 584),
    BYTE_OWNER(g_eng1_callback_record_prefix_060c70, 0x60c70, 368),
    BYTE_OWNER(g_eng1_callback_record_prefix_0582e0, 0x582e0, 328),
    BYTE_OWNER(g_eng1_callback_record_prefix_059af8, 0x59af8, 336),
    BYTE_OWNER(g_eng1_callback_record_prefix_062790, 0x62790, 400),
    BYTE_OWNER(g_eng1_callback_record_prefix_06acb0, 0x6acb0, 320),
    /* +0x2c: alternate callback pointers at +0 of 8-byte records. */
    BYTE_OWNER(g_eng1_row0_factory_pairs_5cc28, 0x5cc28, 320),
    BYTE_OWNER(g_eng1_callback_record_prefix_067f80, 0x67f80, 416),
    BYTE_OWNER(g_eng1_callback_record_prefix_04ec50, 0x4ec50, 320),
    BYTE_OWNER(g_eng1_callback_record_prefix_060f58, 0x60f58, 376),
    BYTE_OWNER(g_eng1_callback_record_prefix_0585c0, 0x585c0, 408),
    BYTE_OWNER(g_eng1_callback_record_prefix_059de8, 0x59de8, 416),
    BYTE_OWNER(g_eng1_callback_record_prefix_062a60, 0x62a60, 320),
    BYTE_OWNER(g_eng1_callback_record_prefix_06af60, 0x6af60, 368)
};
#endif

void eng1_guard_worklist_table_read(const void *base, unsigned long index,
                                    unsigned long stride,
                                    unsigned long width, const char *site)
{
    unsigned int i;
    char reason[224];

    for (i = 0; i < sizeof(g_worklist_limits)/sizeof(g_worklist_limits[0]); ++i) {
        const Eng1WorklistReadLimit *limit = &g_worklist_limits[i];
        if (base != limit->base) continue;
        if (stride == 0UL || width == 0UL || width > stride ||
            width > limit->bytes ||
            index > (limit->bytes - width) / stride) {
            sprintf(reason,
                    "%s index %lu stride %lu width %lu observed-prefix %lu bytes",
                    site, index, stride, width, limit->bytes);
            eng1_incomplete_behavior(limit->original_rva, reason);
        }
        return;
    }
}

void eng1_guard_worklist_group_owner(unsigned long group_index)
{
    if (group_index != 0UL)
        eng1_incomplete_behavior(0x6fee8UL,
            "worklist factory requires kind-info row beyond the owned 8-byte row-0 prefix");
}
