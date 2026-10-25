/* Private typed pointee providers for ENG1 frame/config closures.
 * Source cells were read from ENG1.DLL SHA-256
 * 01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757.
 * This does not claim original C names, translation-unit ownership, or placement.
 */
#include "pointee_layouts.h"
typedef unsigned char eng1_u8;
typedef unsigned long eng1_u32;
typedef char eng1_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_requires_32bit_ulong[(sizeof(eng1_u32) == 4) ? 1 : -1];

/* Exact initialized path block observed at 0x5a7c0..0x5a834.
 * Seven path slots have a 12-byte stride; the final two have a 16-byte stride.
 * The named path starts used by descriptor rows are interior views into this
 * single storage block. */
Eng1LevelDirectoryPathBlock g_eng1_level_directory_paths_5a7c0 = {
    {
        "O:\\VORTEX\\", "O:\\TITANS\\", "O:\\MEDUSA\\",
        "O:\\HYDRA\\", "O:\\THEBES\\", "O:\\NESSUS\\",
        "O:\\FOREST\\"
    },
    {
        "O:\\TRAINING\\", "O:\\PLAYROOM\\"
    }
};

/* Each exact 8-byte slot contains a six-character overlay suffix, NUL, and
 * one observed pad byte. The profile table points at interior slot starts. */
char g_eng1_overlay_name_slots_5e1e4[10][8] = {
    "VX.OVL", "TN.OVL", "MD.OVL", "HY.OVL", "TB.OVL",
    "NS.OVL", "FO.OVL", "------", "TR.OVL", "PR.OVL"
};


/* 0x5cdec begins a 12-byte prefix followed immediately by two observed
 * 24-byte child rows at 0x5cdf8. Both row +8 pointers alias the DWORD at
 * 0x5cdf0 (bundle +4). Signed subindex use does not prove runtime capacity. */
Eng1DescriptorRow0PayloadObserved60 g_eng1_descriptor_row0_payload_5cdec = {
    0x00ff0201UL,
    0x000000ffUL,
    0xff18630bUL,
    {
        { 0x00800200UL, 1UL, &g_eng1_descriptor_row0_payload_5cdec.payload_view_004,
          0UL, 0UL, 0UL },
        { 0x00800300UL, 0UL, &g_eng1_descriptor_row0_payload_5cdec.payload_view_004,
          0UL, 0UL, 0UL }
    }
};
