/* Private ordinary-C model of ENG1:0x5ea0.
 * Names are semantic/address views; they do not claim original source or TU. */
#include "../../../calibration/engine_interface.h"

extern int g_context_sequence_phase;
extern int g_context_sequence_index;
extern unsigned short *g_context_sequence_words;

extern void *__cdecl engine_pool_allocate(void **output_slot,
                                          unsigned long bytes,
                                          unsigned long flags);
extern unsigned long __cdecl engine_file_load_sync(const char *prefix,
                                                    const char *suffix,
                                                    void *destination,
                                                    unsigned long bytes);

/* The observed call passes this zero-filled byte as the file prefix. Its
 * containing object and full extent are not recovered. */
extern char g_cold_file_prefix_byte_6eef4;

static const char *const g_cold_sequence_paths[8] = {
    "M:\\DEMO\\DEMO.DAT",
    "M:\\DEMO\\DEMO1_1.DAT",
    "M:\\DEMO\\DEMO1_2.DAT",
    "M:\\DEMO\\DEMO1_3.DAT",
    "M:\\DEMO\\DEMO2_1.DAT",
    "M:\\DEMO\\DEMO3_1.DAT",
    "M:\\DEMO\\DEMO5_1.DAT",
    "M:\\DEMO\\DEMO5_2.DAT"
};

/* These are four DWORD masks copied into context +0x6c, not function pointers. */
static const unsigned long g_cold_context_masks[4] = {
    0x40008000UL, 0x10002000UL, 0x08000400UL, 0x02000100UL
};

#define CONTEXT_FLAGS(context) \
    (*(unsigned long *)(void *)((unsigned char *)(context) + 0x30))

void __cdecl dispatch_local_05ea0(void)
{
    ResourceCallbackContext *context;
    unsigned long flags;
    unsigned long path_index;
    unsigned long bytes;
    unsigned short *words;
    unsigned long *context_masks;

    context = g_engine_interface.context_004;
    flags = CONTEXT_FLAGS(context);
    if ((flags & 0x01000000UL) != 0)
        g_engine_interface.context_004->unknown_000[6] = 3;

    context = g_engine_interface.context_004;
    if (context->unknown_000[6] == 3) {
        g_context_sequence_index = 0x1518;
        return;
    }

    context = g_engine_interface.context_004;
    flags = CONTEXT_FLAGS(context);
    if ((flags & 0x08000000UL) == 0 &&
        g_engine_interface.context_004->unknown_000[6] != 1)
        return;

    bytes = 0x1800UL;
    context = g_engine_interface.context_004;
    path_index = context->unknown_000[8];
    context = g_engine_interface.context_004;
    if ((CONTEXT_FLAGS(context) & 0x02000000UL) != 0)
        path_index = 0;

    (void)engine_pool_allocate((void **)&g_context_sequence_words,
                               bytes, 0);
    (void)engine_file_load_sync(&g_cold_file_prefix_byte_6eef4,
                                g_cold_sequence_paths[path_index],
                                g_context_sequence_words, bytes);

    context = g_engine_interface.context_004;
    words = g_context_sequence_words;
    context->unknown_000[1] = (unsigned char)words[0];
    context->unknown_000[2] = context->unknown_000[1];
    CONTEXT_FLAGS(context) |= (unsigned long)words[1];
    context->unknown_000[0x0e] = ((unsigned char *)words)[4];

    context_masks = (unsigned long *)(void *)((unsigned char *)context + 0x6c);
    context_masks[0] = g_cold_context_masks[0];
    context_masks[1] = g_cold_context_masks[1];
    context_masks[2] = g_cold_context_masks[2];
    context_masks[3] = g_cold_context_masks[3];
    context->unknown_000[0x0c] = 3;
    g_context_sequence_index = 3;
    g_context_sequence_phase = 2;
}
