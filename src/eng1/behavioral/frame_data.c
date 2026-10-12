/* Behavioral ENG1 startup/frame data reconstruction.
 * Derived from the read-only ENG1.DLL named in the accompanying README.
 * Non-null relocated cells use external target-RVA symbols; no fixed preferred
 * base addresses or fabricated pointee storage are emitted.
 */
#include "data_layouts.h"
#include "../../../calibration/frame_configuration_row.h"

typedef unsigned char eng1_u8;
typedef unsigned short eng1_u16;
typedef unsigned long eng1_u32;
typedef char eng1_requires_32bit_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_requires_32bit_ulong[(sizeof(eng1_u32) == 4) ? 1 : -1];

extern unsigned char eng1_frame_target_002fa0[];
extern unsigned char eng1_frame_target_00e400[];
extern unsigned char eng1_frame_target_01efa0[];
extern unsigned char eng1_frame_target_036c40[];
extern unsigned char eng1_frame_target_03c600[];
extern unsigned char eng1_frame_target_042320[];
extern unsigned char eng1_frame_target_04dcf8[];
extern unsigned char eng1_frame_target_04e268[];
extern unsigned char eng1_frame_target_04e680[];
extern unsigned char eng1_frame_target_04e8c8[];
extern unsigned char eng1_frame_target_04eb10[];
extern unsigned char eng1_frame_target_04ec50[];
extern unsigned char eng1_frame_target_04ed90[];
extern unsigned char eng1_frame_target_04eda8[];
extern unsigned char eng1_frame_target_04edd0[];
extern unsigned char eng1_frame_target_04ede8[];
extern unsigned char eng1_frame_target_04f1e0[];
extern unsigned char eng1_frame_target_04f220[];
extern unsigned char eng1_frame_target_04f258[];
extern unsigned char eng1_frame_target_04f260[];
extern unsigned char eng1_frame_target_04f350[];
extern unsigned char eng1_frame_target_04f358[];
extern unsigned char eng1_frame_target_04f360[];
extern unsigned char eng1_frame_target_057bb0[];
extern unsigned char eng1_frame_target_057f10[];
extern unsigned char eng1_frame_target_058198[];
extern unsigned char eng1_frame_target_0582e0[];
extern unsigned char eng1_frame_target_058428[];
extern unsigned char eng1_frame_target_0585c0[];
extern unsigned char eng1_frame_target_058758[];
extern unsigned char eng1_frame_target_058770[];
extern unsigned char eng1_frame_target_058788[];
extern unsigned char eng1_frame_target_0587a0[];
extern unsigned char eng1_frame_target_058830[];
extern unsigned char eng1_frame_target_058860[];
extern unsigned char eng1_frame_target_058870[];
extern unsigned char eng1_frame_target_058878[];
extern unsigned char eng1_frame_target_0593c0[];
extern unsigned char eng1_frame_target_059720[];
extern unsigned char eng1_frame_target_0599a8[];
extern unsigned char eng1_frame_target_059af8[];
extern unsigned char eng1_frame_target_059c48[];
extern unsigned char eng1_frame_target_059de8[];
extern unsigned char eng1_frame_target_059f88[];
extern unsigned char eng1_frame_target_059fa0[];
extern unsigned char eng1_frame_target_059fc0[];
extern unsigned char eng1_frame_target_059fd8[];
extern unsigned char eng1_frame_target_05a018[];
extern unsigned char eng1_frame_target_05a028[];
extern unsigned char eng1_frame_target_05a030[];
extern unsigned char eng1_frame_target_05a7c0[];
extern unsigned char eng1_frame_target_05a7cc[];
extern unsigned char eng1_frame_target_05a7d8[];
extern unsigned char eng1_frame_target_05a7e4[];
extern unsigned char eng1_frame_target_05a7f0[];
extern unsigned char eng1_frame_target_05a7fc[];
extern unsigned char eng1_frame_target_05a808[];
extern unsigned char eng1_frame_target_05a814[];
extern unsigned char eng1_frame_target_05a824[];
extern unsigned char eng1_frame_target_05c408[];
extern unsigned char eng1_frame_target_05c688[];
extern unsigned char eng1_frame_target_05c868[];
extern unsigned char eng1_frame_target_05c9a8[];
extern unsigned char eng1_frame_target_05cae8[];
extern unsigned char eng1_frame_target_05cc28[];
extern unsigned char eng1_frame_target_05cd68[];
extern unsigned char eng1_frame_target_05cd80[];
extern unsigned char eng1_frame_target_05cd98[];
extern unsigned char eng1_frame_target_05cdb0[];
extern unsigned char eng1_frame_target_05cdec[];
extern unsigned char eng1_frame_target_05cdf4[];
extern unsigned char eng1_frame_target_05cdf8[];
extern unsigned char eng1_frame_target_05e1e4[];
extern unsigned char eng1_frame_target_05e1ec[];
extern unsigned char eng1_frame_target_05e1f4[];
extern unsigned char eng1_frame_target_05e1fc[];
extern unsigned char eng1_frame_target_05e204[];
extern unsigned char eng1_frame_target_05e20c[];
extern unsigned char eng1_frame_target_05e214[];
extern unsigned char eng1_frame_target_05e21c[];
extern unsigned char eng1_frame_target_05e224[];
extern unsigned char eng1_frame_target_05e22c[];
extern unsigned char eng1_frame_target_0602c8[];
extern unsigned char eng1_frame_target_060778[];
extern unsigned char eng1_frame_target_060b00[];
extern unsigned char eng1_frame_target_060c70[];
extern unsigned char eng1_frame_target_060de0[];
extern unsigned char eng1_frame_target_060f58[];
extern unsigned char eng1_frame_target_0610d0[];
extern unsigned char eng1_frame_target_0610e8[];
extern unsigned char eng1_frame_target_061118[];
extern unsigned char eng1_frame_target_061130[];
extern unsigned char eng1_frame_target_0616f8[];
extern unsigned char eng1_frame_target_061738[];
extern unsigned char eng1_frame_target_061778[];
extern unsigned char eng1_frame_target_061780[];
extern unsigned char eng1_frame_target_061840[];
extern unsigned char eng1_frame_target_061f80[];
extern unsigned char eng1_frame_target_062638[];
extern unsigned char eng1_frame_target_062790[];
extern unsigned char eng1_frame_target_062920[];
extern unsigned char eng1_frame_target_062a60[];
extern unsigned char eng1_frame_target_062ba0[];
extern unsigned char eng1_frame_target_062bb8[];
extern unsigned char eng1_frame_target_062be0[];
extern unsigned char eng1_frame_target_062bf8[];
extern unsigned char eng1_frame_target_0630a8[];
extern unsigned char eng1_frame_target_0630e4[];
extern unsigned char eng1_frame_target_0630f8[];
extern unsigned char eng1_frame_target_063108[];
extern unsigned char eng1_frame_target_067118[];
extern unsigned char eng1_frame_target_067608[];
extern unsigned char eng1_frame_target_0679c0[];
extern unsigned char eng1_frame_target_067bd0[];
extern unsigned char eng1_frame_target_067de0[];
extern unsigned char eng1_frame_target_067f80[];
extern unsigned char eng1_frame_target_068120[];
extern unsigned char eng1_frame_target_068138[];
extern unsigned char eng1_frame_target_068158[];
extern unsigned char eng1_frame_target_068170[];
extern unsigned char eng1_frame_target_068590[];
extern unsigned char eng1_frame_target_0685d8[];
extern unsigned char eng1_frame_target_0685f8[];
extern unsigned char eng1_frame_target_068600[];
extern unsigned char eng1_frame_target_06a6b8[];
extern unsigned char eng1_frame_target_06a968[];
extern unsigned char eng1_frame_target_06ab70[];
extern unsigned char eng1_frame_target_06acb0[];
extern unsigned char eng1_frame_target_06adf0[];
extern unsigned char eng1_frame_target_06af60[];
extern unsigned char eng1_frame_target_06b0d0[];
extern unsigned char eng1_frame_target_06b0e8[];
extern unsigned char eng1_frame_target_06b100[];
extern unsigned char eng1_frame_target_06b118[];
extern unsigned char eng1_frame_target_06b158[];
extern unsigned char eng1_frame_target_06b168[];
extern unsigned char eng1_frame_target_06b178[];
extern unsigned char eng1_frame_target_06fe60[];
extern unsigned char eng1_frame_target_06ff90[];
extern unsigned char eng1_frame_target_0702a8[];

#include "pointee_views.h"
#include "descriptor_pointees.inc"
#include "frame_data_callback_records.inc"
#include "frame_data_asset_flag_tables.inc"
#include "config_root_views.inc"

typedef char eng1_configuration_row72_size[(sizeof(ConfigurationRow72) == 72) ? 1 : -1];
ConfigurationRow72 g_configuration_rows_68af0[9] = {
    { 0x0000U, 0x0014U, 0x003cU, 0x0028U, 0x0002U, 0x0003U, 0x0002U, 0x0014U, 0x003cU, 0x0028U, 0x001eU, 0x0004U, (eng1_u32)&eng1_frame_target_05c408[0], (eng1_u32)&eng1_frame_target_05c688[0], (eng1_u32)&eng1_frame_target_05c868[0], (eng1_u32)&eng1_frame_target_05c9a8[0], (eng1_u32)&eng1_frame_target_05cae8[0], (eng1_u32)&eng1_frame_target_05cc28[0], (eng1_u32)&eng1_frame_target_05cd68[0], (eng1_u32)&eng1_frame_target_05cd80[0], (eng1_u32)&eng1_frame_target_05cd98[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_06ff90[0], (eng1_u32)&eng1_frame_target_05cdb0[0] }, /* row 0 */
    { 0x0001U, 0x003cU, 0x0078U, 0x0062U, 0x0002U, 0x0003U, 0x000aU, 0x003cU, 0x0078U, 0x0062U, 0x0014U, 0x0008U, (eng1_u32)&eng1_frame_target_067118[0], (eng1_u32)&eng1_frame_target_067608[0], (eng1_u32)&eng1_frame_target_0679c0[0], (eng1_u32)&eng1_frame_target_067bd0[0], (eng1_u32)&eng1_frame_target_067de0[0], (eng1_u32)&eng1_frame_target_067f80[0], (eng1_u32)&eng1_frame_target_068120[0], (eng1_u32)&eng1_frame_target_068138[0], (eng1_u32)&eng1_frame_target_068158[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_068170[0], (eng1_u32)&eng1_frame_target_068590[0] }, /* row 1 */
    { 0x0003U, 0x003cU, 0x0050U, 0x004cU, 0x0002U, 0x0003U, 0x000aU, 0x003cU, 0x0050U, 0x004cU, 0x0018U, 0x0008U, (eng1_u32)&eng1_frame_target_04dcf8[0], (eng1_u32)&eng1_frame_target_04e268[0], (eng1_u32)&eng1_frame_target_04e680[0], (eng1_u32)&eng1_frame_target_04e8c8[0], (eng1_u32)&eng1_frame_target_04eb10[0], (eng1_u32)&eng1_frame_target_04ec50[0], (eng1_u32)&eng1_frame_target_04ed90[0], (eng1_u32)&eng1_frame_target_04eda8[0], (eng1_u32)&eng1_frame_target_04edd0[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_04ede8[0], (eng1_u32)&eng1_frame_target_04f1e0[0] }, /* row 2 */
    { 0x0005U, 0x0032U, 0x0050U, 0x006eU, 0x0002U, 0x0003U, 0x000aU, 0x0032U, 0x0050U, 0x006cU, 0x001eU, 0x0002U, (eng1_u32)&eng1_frame_target_0602c8[0], (eng1_u32)&eng1_frame_target_060778[0], (eng1_u32)&eng1_frame_target_060b00[0], (eng1_u32)&eng1_frame_target_060c70[0], (eng1_u32)&eng1_frame_target_060de0[0], (eng1_u32)&eng1_frame_target_060f58[0], (eng1_u32)&eng1_frame_target_0610d0[0], (eng1_u32)&eng1_frame_target_0610e8[0], (eng1_u32)&eng1_frame_target_061118[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_061130[0], (eng1_u32)&eng1_frame_target_0616f8[0] }, /* row 3 */
    { 0x0006U, 0x0026U, 0x0064U, 0x0060U, 0x0003U, 0x0003U, 0x000aU, 0x0026U, 0x0016U, 0x0054U, 0x000cU, 0x0014U, (eng1_u32)&eng1_frame_target_057bb0[0], (eng1_u32)&eng1_frame_target_057f10[0], (eng1_u32)&eng1_frame_target_058198[0], (eng1_u32)&eng1_frame_target_0582e0[0], (eng1_u32)&eng1_frame_target_058428[0], (eng1_u32)&eng1_frame_target_0585c0[0], (eng1_u32)&eng1_frame_target_058758[0], (eng1_u32)&eng1_frame_target_058770[0], (eng1_u32)&eng1_frame_target_058788[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_0587a0[0], (eng1_u32)&eng1_frame_target_058830[0] }, /* row 4 */
    { 0x0007U, 0x0050U, 0x0028U, 0x0094U, 0x0002U, 0x0003U, 0x000aU, 0x0050U, 0x0028U, 0x0080U, 0x001eU, 0x000cU, (eng1_u32)&eng1_frame_target_0593c0[0], (eng1_u32)&eng1_frame_target_059720[0], (eng1_u32)&eng1_frame_target_0599a8[0], (eng1_u32)&eng1_frame_target_059af8[0], (eng1_u32)&eng1_frame_target_059c48[0], (eng1_u32)&eng1_frame_target_059de8[0], (eng1_u32)&eng1_frame_target_059f88[0], (eng1_u32)&eng1_frame_target_059fa0[0], (eng1_u32)&eng1_frame_target_059fc0[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_06fe60[0], (eng1_u32)&eng1_frame_target_059fd8[0] }, /* row 5 */
    { 0x0009U, 0x003cU, 0x0028U, 0x0029U, 0x0002U, 0x0005U, 0x0002U, 0x003cU, 0x0028U, 0x0028U, 0x000cU, 0x0014U, (eng1_u32)&eng1_frame_target_061840[0], (eng1_u32)&eng1_frame_target_061f80[0], (eng1_u32)&eng1_frame_target_062638[0], (eng1_u32)&eng1_frame_target_062790[0], (eng1_u32)&eng1_frame_target_062920[0], (eng1_u32)&eng1_frame_target_062a60[0], (eng1_u32)&eng1_frame_target_062ba0[0], (eng1_u32)&eng1_frame_target_062bb8[0], (eng1_u32)&eng1_frame_target_062be0[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_062bf8[0], (eng1_u32)&eng1_frame_target_0630a8[0] }, /* row 6 */
    { 0x000bU, 0x0014U, 0x003cU, 0x0094U, 0x0002U, 0x0003U, 0x000aU, 0x0014U, 0x003cU, 0x0080U, 0x001eU, 0x0004U, (eng1_u32)&eng1_frame_target_06a6b8[0], (eng1_u32)&eng1_frame_target_06a968[0], (eng1_u32)&eng1_frame_target_06ab70[0], (eng1_u32)&eng1_frame_target_06acb0[0], (eng1_u32)&eng1_frame_target_06adf0[0], (eng1_u32)&eng1_frame_target_06af60[0], (eng1_u32)&eng1_frame_target_06b0d0[0], (eng1_u32)&eng1_frame_target_06b0e8[0], (eng1_u32)&eng1_frame_target_06b100[0], 0x00000000UL, (eng1_u32)&eng1_frame_target_0702a8[0], (eng1_u32)&eng1_frame_target_06b118[0] }, /* row 7 */
    { 0x03e7U, 0x000aU, 0x000aU, 0x000aU, 0x0002U, 0x0002U, 0x0002U, 0x000aU, 0x000aU, 0x000aU, 0x0001U, 0x0001U, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00000000UL, 0, 0 }, /* row 8 */
};
/* Exact nine-row prefix through ID=999 sentinel; not full table capacity. */

typedef char eng1_descriptor_row_size[(sizeof(Eng1FrameDescriptorRow24) == 24) ? 1 : -1];
Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12] = {
    { &eng1_frame_target_05a824[0], 0x000ea000UL, 0x00000000UL, &eng1_frame_target_05cdf8[0], &eng1_frame_target_05cdec[0], &eng1_frame_target_05cdf4[0] }, /* row 0, RVA 0x05a690 */
    { &eng1_frame_target_05a814[0], 0x00103000UL, 0x00000000UL, &eng1_frame_target_068600[0], &eng1_frame_target_0685d8[0], &eng1_frame_target_0685f8[0] }, /* row 1, RVA 0x05a6a8 */
    { 0, 0x00000000UL, 0x00000000UL, 0, 0, 0 }, /* row 2, RVA 0x05a6c0 */
    { &eng1_frame_target_05a808[0], 0x00106800UL, 0x00000000UL, &eng1_frame_target_04f260[0], &eng1_frame_target_04f220[0], &eng1_frame_target_04f258[0] }, /* row 3, RVA 0x05a6d8 */
    { &eng1_frame_target_05a7fc[0], 0x00131000UL, 0x00000000UL, &eng1_frame_target_04f360[0], &eng1_frame_target_04f350[0], &eng1_frame_target_04f358[0] }, /* row 4, RVA 0x05a6f0 */
    { &eng1_frame_target_05a7f0[0], 0x000fc800UL, 0x00000000UL, &eng1_frame_target_061780[0], &eng1_frame_target_061738[0], &eng1_frame_target_061778[0] }, /* row 5, RVA 0x05a708 */
    { &eng1_frame_target_05a7e4[0], 0x000f0800UL, 0x00000000UL, &eng1_frame_target_058878[0], &eng1_frame_target_058860[0], &eng1_frame_target_058870[0] }, /* row 6, RVA 0x05a720 */
    { &eng1_frame_target_05a7d8[0], 0x000cc000UL, 0x00000000UL, &eng1_frame_target_05a030[0], &eng1_frame_target_05a018[0], &eng1_frame_target_05a028[0] }, /* row 7, RVA 0x05a738 */
    { 0, 0x00000000UL, 0x00000000UL, 0, 0, 0 }, /* row 8, RVA 0x05a750 */
    { &eng1_frame_target_05a7cc[0], 0x000f7000UL, 0x00000000UL, &eng1_frame_target_063108[0], &eng1_frame_target_0630e4[0], &eng1_frame_target_0630f8[0] }, /* row 9, RVA 0x05a768 */
    { 0, 0x00000000UL, 0x00000000UL, 0, 0, 0 }, /* row 10, RVA 0x05a780 */
    { &eng1_frame_target_05a7c0[0], 0x000df800UL, 0x00000000UL, &eng1_frame_target_06b178[0], &eng1_frame_target_06b158[0], &eng1_frame_target_06b168[0] }, /* row 11, RVA 0x05a798 */
};
/* Prefix through observed context/config ID 11; total count remains unproved. */

typedef char eng1_profile_row_size[(sizeof(Eng1ProfileRow28) == 28) ? 1 : -1];
Eng1ProfileRow28 eng1_profile_rows_5df08[12] = {
    { { 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x07U, 0x1eU, 0x05U, 0x00U, 0x00U }, 0, &eng1_frame_target_05e22c[0], 0x00000000UL, 0x00000000UL }, /* row 0 */
    { { 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x08U, 0x1eU, 0x05U, 0x00U, 0x00U }, &eng1_frame_target_042320[0], &eng1_frame_target_05e224[0], 0x00000000UL, 0x00000000UL }, /* row 1 */
    { { 0xffU, 0xffU, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U }, 0, &eng1_frame_target_05e21c[0], 0x00000000UL, 0x00000000UL }, /* row 2 */
    { { 0x00U, 0x40U, 0x80U, 0x00U, 0x02U, 0x00U, 0x00U, 0x07U, 0x1eU, 0x03U, 0x00U, 0x00U }, &eng1_frame_target_00e400[0], &eng1_frame_target_05e214[0], 0x00000000UL, 0x00000000UL }, /* row 3 */
    { { 0xffU, 0xffU, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1eU, 0x05U, 0x00U, 0x00U }, 0, &eng1_frame_target_05e20c[0], 0x00000000UL, 0x00000000UL }, /* row 4 */
    { { 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x06U, 0x1eU, 0x05U, 0x00U, 0x00U }, &eng1_frame_target_036c40[0], &eng1_frame_target_05e204[0], 0x00000000UL, 0x00000000UL }, /* row 5 */
    { { 0x3fU, 0x53U, 0x6fU, 0x01U, 0x02U, 0x00U, 0x00U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U }, &eng1_frame_target_01efa0[0], &eng1_frame_target_05e1fc[0], 0x00000000UL, 0x00000000UL }, /* row 6 */
    { { 0x00U, 0x00U, 0x00U, 0x02U, 0x02U, 0x00U, 0x00U, 0x07U, 0x00U, 0x00U, 0x00U, 0x00U }, 0, &eng1_frame_target_05e1f4[0], 0x00000000UL, 0x00000000UL }, /* row 7 */
    { { 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1eU, 0x05U, 0x00U, 0x00U }, 0, &eng1_frame_target_05e21c[0], 0x00000000UL, 0x00000000UL }, /* row 8 */
    { { 0x00U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x08U, 0x1eU, 0x05U, 0x00U, 0x00U }, &eng1_frame_target_03c600[0], &eng1_frame_target_05e1ec[0], 0x00000000UL, 0x00000000UL }, /* row 9 */
    { { 0xffU, 0xffU, 0xffU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U }, 0, &eng1_frame_target_05e21c[0], 0x00000000UL, 0x00000000UL }, /* row 10 */
    { { 0x00U, 0x20U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x07U, 0x1eU, 0x05U, 0x00U, 0x00U }, &eng1_frame_target_002fa0[0], &eng1_frame_target_05e1e4[0], 0x00000000UL, 0x00000000UL }, /* row 11 */
};
/* Twelve initialized rows observed; signed unchecked index does not prove capacity. */

/* Access-width data cells. Zero-initialized declarations represent loader-zero
 * storage only; they do not assert a common original aggregate or source unit.
 */
eng1_u32 g_dispatch_entry_state_6fcec;
eng1_u8 g_dispatch_startup_gate_6fce8;
eng1_u32 g_dispatch_callback_flag_71ec0;
eng1_u8 *g_dispatch_frame_current_71ec4;
eng1_u8 *g_dispatch_frame_base_71ec8;

/* Selected byte 71860 is owned by actor_pipeline_storage_71440. */
eng1_u16 g_configuration_word_713b4, g_configuration_word_71432, g_configuration_word_713ba;
eng1_u16 g_configuration_word_71378, g_configuration_word_713c4;
eng1_u16 g_configuration_word_713aa, g_configuration_word_713b8;
eng1_u16 g_configuration_word_7138c, g_configuration_word_71430;
eng1_u16 g_configuration_word_713a8, g_configuration_word_71308;
eng1_u16 g_configuration_word_713a0, g_configuration_word_713ac;
eng1_u16 g_configuration_word_713b6, g_configuration_word_71374;
eng1_u16 g_configuration_word_71394, g_configuration_word_71380;
eng1_u32 g_configuration_dword_71388, g_configuration_dword_71370;
eng1_u32 g_configuration_dword_71390, g_configuration_dword_7137c;
eng1_u32 g_configuration_dword_713a4, g_configuration_dword_7139c;
eng1_u32 g_configuration_dword_713b0, g_configuration_dword_71384;
eng1_u32 g_configuration_dword_713c0, g_configuration_dword_71398;
eng1_u32 g_configuration_dword_713bc;
eng1_u32 g_context_default_dword_721a8, g_context_default_dword_72190;

/* Frame allocation owner/pointer cells. The actor-pool cells at 0x72860,
 * 0x73494 and 0x734e0 are deliberately omitted and have another owner.
 */
void *g_frame_owner_70c80, *g_frame_owner_7348c;
void *g_frame_owner_72870, *g_frame_owner_734e8;
eng1_u32 g_frame_global_70c54;
eng1_u16 g_frame_word_6ee84;
eng1_u32 g_frame_global_70d24, g_frame_global_70ce8;
eng1_u32 g_frame_global_70cec, g_frame_global_70cf4;
eng1_u32 g_frame_global_73484, g_frame_global_734f4;
eng1_u8 g_frame_global_734f0, g_frame_global_734f1, g_frame_global_734f2;
eng1_u8 g_frame_global_73498;

/* Only an observed 64-byte payload prefix at 0x58a60; original extent/type unknown. */
eng1_u8 g_frame_setup_callback_58a60[64] = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x40U, 0x01U, 0x00U, 0x02U, 0x4dU, 0x41U, 0x50U, 0x5fU, 0x50U, 0x53U, 0x58U, 0x2eU,
    0x42U, 0x49U, 0x4eU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x14U, 0x00U, 0x00U, 0x00U, 0x00U, 0xf6U, 0x23U, 0x00U, 0x00U, 0x2eU, 0x01U, 0x00U,
    0x00U, 0xd8U, 0x02U, 0x00U, 0x7fU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
};
/* NUL-terminated string portion at 0x58ac8, kept separate from following bytes. */
eng1_u8 g_dispatch_diagnostic_58ac8[45] = {
    0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU,
    0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU,
    0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x2dU, 0x0aU, 0x00U
};
