/* Private ENG1 descriptor-child data closure. All byte prefixes below are
 * exact initialized spans from the pinned ENG1.DLL; parent-row capacities are
 * observed prefixes only. The child row extent is read from native 24-byte
 * indexed loads and observed relocation boundaries, not inferred full capacity.
 * Include in the same translation unit as the frame/configuration tables. */
typedef struct Eng1ObservedDescriptorChildRow24 {
    eng1_u32 field_000;
    eng1_u32 field_004;
    void *payload_008;
    eng1_u32 field_00c;
    eng1_u32 field_010;
    eng1_u32 field_014;
} Eng1ObservedDescriptorChildRow24;
typedef char eng1_observed_descriptor_child_row24_size[
    (sizeof(Eng1ObservedDescriptorChildRow24) == 24) ? 1 : -1];
typedef struct Eng1ObservedDescriptorPointerRow24 {
    void *pointer_000; void *pointer_004; void *pointer_008;
    void *pointer_00c; void *pointer_010; void *pointer_014;
} Eng1ObservedDescriptorPointerRow24;
typedef char eng1_observed_descriptor_pointer_row24_size[
    (sizeof(Eng1ObservedDescriptorPointerRow24) == 24) ? 1 : -1];
extern unsigned char eng1_frame_target_04fec0[];
extern unsigned char eng1_frame_target_050d88[];
#include "initial_unreadable_identities.h"
/* Byte-list views share the measured animation/pool owners and read guards. */
#define eng1_frame_target_04fec0 eng1_resource_byte_list_04fec0
#define eng1_frame_target_050d88 eng1_resource_byte_list_050d88

typedef struct Eng1DescriptorPayload_68600 {
    eng1_u8 header_0685d8[40];
    Eng1ObservedDescriptorChildRow24 child_rows[8];
} Eng1DescriptorPayload_68600;
Eng1DescriptorPayload_68600 g_eng1_descriptor_payload_68600 = {
    {
        0x01U, 0x02U, 0x04U, 0x27U, 0x0dU, 0x0fU, 0x23U, 0xffU, 0x1fU, 0x10U, 0xffU, 0x00U,
        0x2eU, 0xffU, 0x00U, 0x00U, 0x20U, 0xffU, 0x00U, 0x00U, 0x1fU, 0x2eU, 0xffU, 0x00U,
        0x2eU, 0xffU, 0x00U, 0x00U, 0x11U, 0xffU, 0x00U, 0x00U, 0xffU, 0x46U, 0x70U, 0x01U,
        0x0eU, 0x1aU, 0xffU, 0x00U
    },
    {
        { 0x02cc04deUL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02cc2040UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[12], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02cc3cd0UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[16], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x010c4d60UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[20], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x012c63d0UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[24], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x010c7aa6UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[28], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x012c58c0UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[24], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x010c7aa6UL, 0x00000001UL, &g_eng1_descriptor_payload_68600.header_0685d8[28], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_4f260 {
    eng1_u8 header_04f220[64];
    Eng1ObservedDescriptorChildRow24 child_rows[10];
} Eng1DescriptorPayload_4f260;
Eng1DescriptorPayload_4f260 g_eng1_descriptor_payload_4f260 = {
    {
        0x01U, 0x02U, 0x04U, 0x27U, 0x3aU, 0xffU, 0x00U, 0x00U, 0x26U, 0x2dU, 0x2eU, 0x3bU,
        0xffU, 0x00U, 0x00U, 0x00U, 0x1bU, 0x26U, 0x2dU, 0x2eU, 0x3aU, 0x3bU, 0xffU, 0x00U,
        0x2dU, 0x2eU, 0x3aU, 0x3bU, 0x2bU, 0xffU, 0x00U, 0x00U, 0x2bU, 0x2dU, 0x2eU, 0x3aU,
        0x3bU, 0xffU, 0x00U, 0x00U, 0x3cU, 0x2aU, 0x2dU, 0x2eU, 0x3aU, 0x3bU, 0xffU, 0x00U,
        0x44U, 0x3aU, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xffU, 0x50U, 0x6eU, 0x03U,
        0x1cU, 0x10U, 0xffU, 0x00U
    },
    {
        { 0x024c03deUL, 0x00000000UL, &g_eng1_descriptor_payload_4f260.header_04f220[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x01d014c0UL, 0x00000003UL, &g_eng1_descriptor_payload_4f260.header_04f220[16], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02601de0UL, 0x00000003UL, &g_eng1_descriptor_payload_4f260.header_04f220[24], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02902980UL, 0x00000004UL, &g_eng1_descriptor_payload_4f260.header_04f220[32], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x026038c0UL, 0x00000003UL, &g_eng1_descriptor_payload_4f260.header_04f220[40], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02a04840UL, 0x00000004UL, &g_eng1_descriptor_payload_4f260.header_04f220[48], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02a003c0UL, 0x00000000UL, &g_eng1_descriptor_payload_4f260.header_04f220[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02a003c0UL, 0x00000000UL, &g_eng1_descriptor_payload_4f260.header_04f220[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02a003c0UL, 0x00000000UL, &g_eng1_descriptor_payload_4f260.header_04f220[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x02a003c0UL, 0x00000000UL, &g_eng1_descriptor_payload_4f260.header_04f220[8], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_61780 {
    eng1_u8 header_061738[72];
    Eng1ObservedDescriptorChildRow24 child_rows[8];
} Eng1DescriptorPayload_61780;
Eng1DescriptorPayload_61780 g_eng1_descriptor_payload_61780 = {
    {
        0x01U, 0x02U, 0x04U, 0x27U, 0xffU, 0x00U, 0x00U, 0x00U, 0x26U, 0x14U, 0x17U, 0x18U,
        0x1cU, 0xffU, 0x00U, 0x00U, 0x19U, 0x26U, 0xffU, 0x00U, 0x19U, 0x28U, 0x26U, 0xffU,
        0x26U, 0x1eU, 0x29U, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x14U, 0x19U, 0x16U,
        0x15U, 0x24U, 0xffU, 0x00U, 0x26U, 0x19U, 0x16U, 0x1dU, 0xffU, 0x00U, 0x00U, 0x00U,
        0x26U, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x26U, 0x17U, 0x18U, 0x19U,
        0x24U, 0x1cU, 0xffU, 0x00U, 0xffU, 0x46U, 0x6eU, 0x05U, 0x12U, 0x1eU, 0xffU, 0x00U
    },
    {
        { 0x048c0120UL, 0x00000000UL, &g_eng1_descriptor_payload_61780.header_061738[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x048c12f0UL, 0x00000000UL, &g_eng1_descriptor_payload_61780.header_061738[16], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x040c18a0UL, 0x00000001UL, &g_eng1_descriptor_payload_61780.header_061738[20], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034c2270UL, 0x00000000UL, &g_eng1_descriptor_payload_61780.header_061738[24], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x038c30b0UL, 0x00ff0001UL, &g_eng1_descriptor_payload_61780.header_061738[32], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x048c3d80UL, 0x00000000UL, &g_eng1_descriptor_payload_61780.header_061738[40], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x040c3940UL, 0x00000001UL, &g_eng1_descriptor_payload_61780.header_061738[48], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x038c4820UL, 0x00000000UL, &g_eng1_descriptor_payload_61780.header_061738[56], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_58878 {
    eng1_u8 header_058860[24];
    Eng1ObservedDescriptorChildRow24 child_rows[2];
} Eng1DescriptorPayload_58878;
Eng1DescriptorPayload_58878 g_eng1_descriptor_payload_58878 = {
    {
        0x01U, 0x04U, 0x02U, 0x4fU, 0xffU, 0x00U, 0x00U, 0x00U, 0xffU, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U, 0xffU, 0x50U, 0x6eU, 0x06U, 0x1fU, 0x13U, 0xffU, 0x00U
    },
    {
        { 0x03e008c0UL, 0x00000001UL, &g_eng1_descriptor_payload_58878.header_058860[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x030008c0UL, 0x00000001UL, &g_eng1_descriptor_payload_58878.header_058860[8], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_5a030 {
    eng1_u8 header_05a018[24];
    Eng1ObservedDescriptorChildRow24 child_rows[2];
} Eng1DescriptorPayload_5a030;
Eng1DescriptorPayload_5a030 g_eng1_descriptor_payload_5a030 = {
    {
        0x01U, 0x04U, 0x02U, 0x4dU, 0x4eU, 0xffU, 0x00U, 0x00U, 0x27U, 0xffU, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U, 0xffU, 0x50U, 0x6eU, 0x07U, 0x20U, 0x14U, 0xffU, 0x00U
    },
    {
        { 0x02801340UL, 0x00000001UL, &g_eng1_descriptor_payload_5a030.header_05a018[8], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x00800300UL, 0x00000000UL, &g_eng1_descriptor_payload_5a030.header_05a018[8], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_63108 {
    eng1_u8 header_0630e4[36];
    Eng1ObservedDescriptorChildRow24 child_rows[12];
} Eng1DescriptorPayload_63108;
Eng1DescriptorPayload_63108 g_eng1_descriptor_payload_63108 = {
    {
        0x30U, 0x02U, 0xffU, 0x00U, 0x3fU, 0x31U, 0x3eU, 0x40U, 0xffU, 0x00U, 0x00U, 0x00U,
        0x3fU, 0x32U, 0x40U, 0xffU, 0x3fU, 0x34U, 0x40U, 0xffU, 0xffU, 0x48U, 0x6eU, 0xfeU,
        0x2fU, 0x09U, 0x16U, 0x22U, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
    },
    {
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x03002340UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x03005380UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
        { 0x034000c0UL, 0x00000001UL, &g_eng1_descriptor_payload_63108.header_0630e4[4], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_6b178 {
    eng1_u8 header_06b158[32];
    Eng1ObservedDescriptorChildRow24 child_rows[1];
} Eng1DescriptorPayload_6b178;
Eng1DescriptorPayload_6b178 g_eng1_descriptor_payload_6b178 = {
    {
        0x01U, 0x04U, 0x02U, 0x50U, 0xffU, 0x00U, 0x00U, 0x00U, 0xffU, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U, 0xffU, 0x48U, 0x64U, 0xfeU, 0x28U, 0x0bU, 0x18U, 0x24U,
        0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
    },
    {
        { 0x01a00090UL, 0x00000001UL, &g_eng1_descriptor_payload_6b178.header_06b158[8], 0x00000000UL, 0x00000000UL, 0x00000000UL }
    }
};

typedef struct Eng1DescriptorPayload_4f350 {
    eng1_u8 header_04f350[16];
    Eng1ObservedDescriptorChildRow24 first_child_row;
    Eng1ObservedDescriptorPointerRow24 following_pointer_record;
} Eng1DescriptorPayload_4f350;
Eng1DescriptorPayload_4f350 g_eng1_descriptor_payload_4f350 = {
    {
        0x01U, 0x02U, 0x04U, 0xffU, 0x27U, 0xffU, 0x00U, 0x00U, 0x04U, 0x63U, 0x11U, 0xffU,
        0x00U, 0x00U, 0x00U, 0x00U
    },
    { 0x02801340UL, 0x00000000UL, &g_eng1_descriptor_payload_4f350.header_04f350[4], 0x00000000UL, 0x00000000UL, 0x00000000UL },
    {
        &eng1_frame_target_04fec0[0], &eng1_frame_target_04fec0[0],
        &eng1_frame_target_050d88[0], &eng1_frame_target_04fec0[0],
        &eng1_frame_target_04fec0[0], 0
    }
};

/* Same-translation-unit target aliases; no independent storage. */
#define eng1_frame_target_04f220 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[0])
#define eng1_frame_target_04f228 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[8])
#define eng1_frame_target_04f230 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[16])
#define eng1_frame_target_04f238 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[24])
#define eng1_frame_target_04f240 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[32])
#define eng1_frame_target_04f248 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[40])
#define eng1_frame_target_04f250 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[48])
#define eng1_frame_target_04f258 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.header_04f220[56])
#define eng1_frame_target_04f260 ((eng1_u8 *)&g_eng1_descriptor_payload_4f260.child_rows[0])
#define eng1_frame_target_04f350 ((eng1_u8 *)&g_eng1_descriptor_payload_4f350.header_04f350[0])
#define eng1_frame_target_04f354 ((eng1_u8 *)&g_eng1_descriptor_payload_4f350.header_04f350[4])
#define eng1_frame_target_04f358 ((eng1_u8 *)&g_eng1_descriptor_payload_4f350.header_04f350[8])
#define eng1_frame_target_04f360 ((eng1_u8 *)&g_eng1_descriptor_payload_4f350.first_child_row)
#define eng1_frame_target_04f378 ((eng1_u8 *)&g_eng1_descriptor_payload_4f350.following_pointer_record)
#define eng1_frame_target_058860 ((eng1_u8 *)&g_eng1_descriptor_payload_58878.header_058860[0])
#define eng1_frame_target_058868 ((eng1_u8 *)&g_eng1_descriptor_payload_58878.header_058860[8])
#define eng1_frame_target_058870 ((eng1_u8 *)&g_eng1_descriptor_payload_58878.header_058860[16])
#define eng1_frame_target_058878 ((eng1_u8 *)&g_eng1_descriptor_payload_58878.child_rows[0])
#define eng1_frame_target_05a018 ((eng1_u8 *)&g_eng1_descriptor_payload_5a030.header_05a018[0])
#define eng1_frame_target_05a020 ((eng1_u8 *)&g_eng1_descriptor_payload_5a030.header_05a018[8])
#define eng1_frame_target_05a028 ((eng1_u8 *)&g_eng1_descriptor_payload_5a030.header_05a018[16])
#define eng1_frame_target_05a030 ((eng1_u8 *)&g_eng1_descriptor_payload_5a030.child_rows[0])
#define eng1_frame_target_061738 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[0])
#define eng1_frame_target_061740 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[8])
#define eng1_frame_target_061748 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[16])
#define eng1_frame_target_06174c ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[20])
#define eng1_frame_target_061750 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[24])
#define eng1_frame_target_061758 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[32])
#define eng1_frame_target_061760 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[40])
#define eng1_frame_target_061768 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[48])
#define eng1_frame_target_061770 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[56])
#define eng1_frame_target_061778 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.header_061738[64])
#define eng1_frame_target_061780 ((eng1_u8 *)&g_eng1_descriptor_payload_61780.child_rows[0])
#define eng1_frame_target_0630e4 ((eng1_u8 *)&g_eng1_descriptor_payload_63108.header_0630e4[0])
#define eng1_frame_target_0630e8 ((eng1_u8 *)&g_eng1_descriptor_payload_63108.header_0630e4[4])
#define eng1_frame_target_0630f8 ((eng1_u8 *)&g_eng1_descriptor_payload_63108.header_0630e4[20])
#define eng1_frame_target_063108 ((eng1_u8 *)&g_eng1_descriptor_payload_63108.child_rows[0])
#define eng1_frame_target_0685d8 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[0])
#define eng1_frame_target_0685e0 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[8])
#define eng1_frame_target_0685e4 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[12])
#define eng1_frame_target_0685e8 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[16])
#define eng1_frame_target_0685ec ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[20])
#define eng1_frame_target_0685f0 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[24])
#define eng1_frame_target_0685f4 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[28])
#define eng1_frame_target_0685f8 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.header_0685d8[32])
#define eng1_frame_target_068600 ((eng1_u8 *)&g_eng1_descriptor_payload_68600.child_rows[0])
#define eng1_frame_target_06b158 ((eng1_u8 *)&g_eng1_descriptor_payload_6b178.header_06b158[0])
#define eng1_frame_target_06b160 ((eng1_u8 *)&g_eng1_descriptor_payload_6b178.header_06b158[8])
#define eng1_frame_target_06b168 ((eng1_u8 *)&g_eng1_descriptor_payload_6b178.header_06b158[16])
#define eng1_frame_target_06b178 ((eng1_u8 *)&g_eng1_descriptor_payload_6b178.child_rows[0])
