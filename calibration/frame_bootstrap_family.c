#include <string.h>
#include "engine_dispatch_externs.h"

/* Address-backed views used by the reviewed bootstrap/setup callers.  These
 * names describe observed cells and table bases only; they do not claim the
 * original global names, declarations, or full table extents. */
extern u16 g_configuration_word_713b4;
extern u16 g_configuration_word_71432;
extern u16 g_configuration_word_713ba;
extern void *g_frame_owner_72860;
extern void *g_frame_owner_73494;
extern void *g_frame_owner_734e0;
extern void *g_frame_owner_70c80;
extern void *g_frame_owner_7348c;
extern void *g_frame_owner_72870;
extern void *g_frame_owner_734e8;
extern u32 g_frame_global_70c54;
extern u16 g_frame_word_6ee84;
extern u16 g_frame_word_70d12;
extern u8 g_frame_byte_70d01;
extern u32 g_frame_global_70d24;
extern u32 g_frame_global_70ce8;
extern u32 g_frame_global_70cec;
extern u32 g_frame_global_70cf4;
extern u32 g_frame_global_73484;
extern u32 g_frame_global_734f4;
extern u8 g_frame_global_734f0;
extern u8 g_frame_global_734f1;
extern u8 g_frame_global_734f2;
extern u8 g_frame_global_73498;
extern u8 g_frame_config_5df08[];
extern u8 g_frame_descriptor_links_5a69c[];
extern const u8 g_frame_setup_callback_58a60[];

/* Recovered providers share these PC addresses; names remain diagnostic. */
extern void __cdecl frame_configuration_280f0(void);
extern void *__cdecl engine_pool_allocate(void **, u32, u32);
extern void __cdecl dispatch_engine_2c7e0(void);
extern void __cdecl dispatch_local_05710(void);
extern void __cdecl frame_context_defaults_15600(void);
extern EngineNoArgCallback __cdecl dispatch_engine_2c640(EngineNoArgCallback);
extern int __cdecl engine_file_cancel(void);
extern void __cdecl dispatch_engine_2c280(void);
extern void __cdecl dispatch_engine_2c2d0(void *);
extern void __cdecl dispatch_engine_2c2e0(u32);
extern void __cdecl dispatch_engine_2c5e0(const void *, u32, u32, u32);
extern void __cdecl dispatch_engine_2c500(void *, void *);
extern void __cdecl dispatch_engine_2c980(void *, u32, u32, u32, u32);
extern void __cdecl dispatch_engine_2c8f0(void *);
extern void __cdecl dispatch_engine_2c8e0(void *);
extern void __cdecl dispatch_local_02f30(void);
extern void __cdecl dispatch_local_03270(void *, void *);
extern void __cdecl dispatch_local_15090(void);
extern void __cdecl dispatch_local_05dc0(u32);
extern void __cdecl dispatch_local_25620(void);
extern void __cdecl dispatch_local_058b0(void);
extern void __cdecl dispatch_local_2f840(void);
extern void __cdecl dispatch_local_2fb70(void);
extern void __cdecl dispatch_local_2da90(void);
extern void __cdecl dispatch_local_262a0(void);
extern void __cdecl dispatch_local_050d0(void);
extern void __cdecl resource_rebuild_blob(void);
extern void __cdecl resource_rebuild_group_rows(void);
extern int __cdecl engine_frame_chain_write(void *, unsigned long);

void __cdecl dispatch_local_1f0f0(void)
{
    int level;
    unsigned long bytes;

    frame_configuration_280f0();

    bytes = 0x134UL * (unsigned long)g_configuration_word_713b4;
    engine_pool_allocate(&g_frame_owner_72860, bytes, 16UL);

    bytes = 0xecUL * (unsigned long)g_configuration_word_71432 + 0x134UL;
    engine_pool_allocate(&g_frame_owner_73494, bytes, 16UL);

    bytes = 0x94UL * (unsigned long)g_configuration_word_713ba + 0x134UL;
    engine_pool_allocate(&g_frame_owner_734e0, bytes, 16UL);

    dispatch_engine_2c7e0();
    engine_pool_allocate((void **)&g_dispatch_frame_base_71ec8,
                          0x2910UL, 16UL);

    level = (int)(s8)g_engine_interface.context_004->unknown_000[0];
    bytes = 80UL * (unsigned long)*(u16 *)(g_frame_config_5df08 +
                                           level * 28 + 6);
    engine_pool_allocate(&g_frame_owner_70c80, bytes, 16UL);

    engine_pool_allocate(&g_frame_owner_7348c, 0x4000UL, 0UL);
    engine_pool_allocate(&g_frame_owner_72870, 0x8000UL, 0UL);
    engine_pool_allocate(&g_frame_owner_734e8, 0x300UL, 0UL);

    memset(g_frame_owner_72870, 0, 0x8000UL);
    memset(g_frame_owner_734e8, 0, 0x300UL);

    dispatch_local_05710();
    frame_context_defaults_15600();
}

void __cdecl dispatch_local_1f220(void)
{
    ResourceCallbackContext *context;

    g_dispatch_callback_flag_71ec0 = 0;
    dispatch_engine_2c640((EngineNoArgCallback)0);
    dispatch_engine_2cc40((EngineNoArgCallback)0);

    context = g_engine_interface.context_004;
    if (context->mode_09c == 1) {
        engine_file_cancel();
        context = g_engine_interface.context_004;
        if (context->load_failed != 0)
            context->load_failed(0);
    } else if (context->mode_09c != 0) {
        dispatch_engine_2c280();
    }

    dispatch_engine_2c630(0);
    dispatch_engine_2cc30(0);

    if (g_frame_owner_72860 != 0)
        dispatch_engine_2c2d0(g_frame_owner_72860);
    if (g_frame_owner_73494 != 0)
        dispatch_engine_2c2d0(g_frame_owner_73494);
    if (g_frame_owner_734e0 != 0)
        dispatch_engine_2c2d0(g_frame_owner_734e0);
    if (g_frame_owner_70c80 != 0)
        dispatch_engine_2c2d0(g_frame_owner_70c80);
    if (g_dispatch_frame_base_71ec8 != 0)
        dispatch_engine_2c2d0(g_dispatch_frame_base_71ec8);

    dispatch_engine_2c2e0(0);

    context = g_engine_interface.context_004;
    *(u32 *)((u8 *)context + 0x6c) = *(u32 *)((u8 *)context + 0x7c);
    *(u32 *)((u8 *)context + 0x70) = *(u32 *)((u8 *)context + 0x80);
    *(u32 *)((u8 *)context + 0x74) = *(u32 *)((u8 *)context + 0x84);
    *(u32 *)((u8 *)context + 0x78) = *(u32 *)((u8 *)context + 0x88);

    context = g_engine_interface.context_004;
    ((u8 *)context)[0x0e] = ((u8 *)context)[0x0f];
}

void __cdecl dispatch_local_1f320(void)
{
    ResourceCallbackContext *context;
    u8 *frame;
    u8 *descriptor;
    void *descriptor_child;
    void *descriptor_item;
    int level;
    int subindex;
    u8 link_flag;
    u8 config_flags;
    u32 scaled_word;

    g_frame_global_70c54 = 0;
    context = g_engine_interface.context_004;
    *(u16 *)((u8 *)context + 0x60) = 0;
    *(u16 *)((u8 *)context + 0x5e) = 0;
    *(u16 *)((u8 *)context + 0x5c) = 0;
    *(u16 *)((u8 *)context + 0x66) = 0;
    *(u16 *)((u8 *)context + 0x64) = 0;
    *(u16 *)((u8 *)context + 0x62) = 0;
    g_dispatch_key_7286c = 0;
    g_frame_word_6ee84 = 0;
    context->unknown_000[3] = 0;
    context->tick_038 = 0;
    g_frame_byte_70d01 = 0xff;

    g_dispatch_frame_current_71ec4 = g_dispatch_frame_base_71ec8;
    dispatch_engine_2c5e0(g_frame_setup_callback_58a60, 0, 0, 0);

    context = g_engine_interface.context_004;
    level = (int)(s8)context->unknown_000[0];
    g_frame_global_734f4 = (u32)g_frame_owner_72860;
    g_frame_global_70d24 = *(u32 *)(g_frame_config_5df08 + level * 28 + 12);
    resource_rebuild_blob();

    scaled_word = (u32)((int)(short)g_frame_word_70d12 * 64 - 0xa0);
    g_frame_global_70ce8 = 0xa0;
    g_frame_global_70cec = scaled_word;
    dispatch_local_02f30();

    context = g_engine_interface.context_004;
    level = (int)(s8)context->unknown_000[0];
    subindex = (int)(s8)context->unknown_000[1];
    descriptor = g_frame_descriptor_links_5a69c + level * 24;
    descriptor_child = *(void **)descriptor;
    descriptor_item = *(void **)(descriptor + 4);
    dispatch_local_03270(
        descriptor_item,
        *(void **)((u8 *)descriptor_child + subindex * 24 + 8));

    dispatch_local_15090();
    dispatch_local_1f870(0);
    resource_rebuild_group_rows();

    context = g_engine_interface.context_004;
    context->unknown_128 |= 4;
    level = (int)(s8)context->unknown_000[0];
    g_frame_global_734f0 = g_frame_config_5df08[level * 28];
    g_frame_global_734f1 = g_frame_config_5df08[level * 28 + 1];
    g_frame_global_734f2 = g_frame_config_5df08[level * 28 + 2];
    g_frame_global_70cf4 = 0xffffffffUL;
    dispatch_local_05dc0(0);
    dispatch_local_25620();

    frame = g_dispatch_frame_base_71ec8;
    engine_frame_chain_write(frame + 0x88, 0x500UL);
    frame = g_dispatch_frame_base_71ec8;
    engine_frame_chain_write(frame + 0x1510, 0x500UL);

    if (g_dispatch_frame_current_71ec4 == g_dispatch_frame_base_71ec8) {
        context = g_engine_interface.context_004;
        level = (int)(s8)context->unknown_000[0];
        scaled_word = (u32)*(u16 *)(g_frame_config_5df08 + level * 28 + 6);
        g_frame_global_73484 = (u32)g_frame_owner_70c80 +
                               scaled_word * 40UL;
    } else {
        g_frame_global_73484 = (u32)g_frame_owner_70c80;
    }

    frame = g_dispatch_frame_base_71ec8;
    dispatch_engine_2c500(frame, frame + 0x1488);

    context = g_engine_interface.context_004;
    level = (int)(s8)context->unknown_000[0];
    config_flags = g_frame_config_5df08[level * 28 + 4];
    link_flag = config_flags & 1;
    frame = g_dispatch_frame_base_71ec8;
    frame[0x149e] = link_flag;
    frame = g_dispatch_frame_base_71ec8;
    frame[0x16] = link_flag;
    frame = g_dispatch_frame_base_71ec8;
    frame[0x149f] = 0;
    frame = g_dispatch_frame_base_71ec8;
    frame[0x17] = 0;

    context = g_engine_interface.context_004;
    level = (int)(s8)context->unknown_000[0];
    config_flags = g_frame_config_5df08[level * 28 + 4];
    frame = g_dispatch_frame_base_71ec8;
    frame[0x14a0] = config_flags & 2;
    frame = g_dispatch_frame_base_71ec8;
    frame[0x18] = frame[0x14a0];

    frame = g_dispatch_frame_base_71ec8;
    dispatch_engine_2c980(frame + 0x70, 0, link_flag, 0x9c, 0);
    frame = g_dispatch_frame_base_71ec8;
    dispatch_engine_2c980(frame + 0x14f8, 0, link_flag, 0x9c, 0);
    frame = g_dispatch_frame_base_71ec8;
    dispatch_engine_2c980(frame + 0x7c, 0, link_flag, 0x9e, 0);
    frame = g_dispatch_frame_base_71ec8;
    dispatch_engine_2c980(frame + 0x1504, 0, link_flag, 0x9e, 0);

    g_frame_global_73498 = 1;
    dispatch_local_058b0();

    dispatch_engine_2c570(g_dispatch_frame_current_71ec4);
    dispatch_engine_2c570(g_dispatch_frame_current_71ec4);
    dispatch_local_2f840();
    dispatch_local_2fb70();
    dispatch_local_2da90();
    dispatch_local_262a0();
    dispatch_local_050d0();
    dispatch_engine_2c8f0(g_dispatch_frame_current_71ec4);
    dispatch_engine_2c8e0(g_dispatch_frame_current_71ec4 + 0x5c);

    frame = g_dispatch_frame_base_71ec8;
    if (g_dispatch_frame_current_71ec4 == frame)
        g_dispatch_frame_current_71ec4 = frame + 0x1488;
    else
        g_dispatch_frame_current_71ec4 = frame;
}
