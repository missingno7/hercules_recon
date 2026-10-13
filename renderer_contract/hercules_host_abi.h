#ifndef RENDERER_CROSS_MODULE_TABLE_CLONE_H
#define RENDERER_CROSS_MODULE_TABLE_CLONE_H

#include "hercules_types.h"

/* RVAs are for the pinned unpatched host only, never portable DLL addresses. */
#define HERCULES_HOST_TABLE_RVA ((renderer_u32)0x361b8u)
#define HERCULES_HOST_TABLE_POINTER_RVA ((renderer_u32)0x365a0u)
#define HERCULES_SLOT_FRAME_NOTIFY 80u
#define HERCULES_SLOT_CHAIN_INIT 81u
#define HERCULES_SLOT_SUBMIT 89u
#define HERCULES_SLOT_PRIMARY_CALLBACK_EXCHANGE 92u
#define HERCULES_SLOT_POINT_INPUT 101u
#define HERCULES_SLOT_BATCH_POINT_INPUT 102u
#define HERCULES_SLOT_BATCH_PROJECT 107u
#define HERCULES_SLOT_FT4_SCREEN_COPY 117u
#define HERCULES_SLOT_PROJECT 106u
#define HERCULES_SLOT_SCREEN_OUTPUT 114u
#define HERCULES_SLOT_UPLOAD_RECT 129u
#define HERCULES_SLOT_UPLOAD_PAGE 130u
#define HERCULES_SLOT_MOVE_RECT 131u
#define HERCULES_SLOT_COLOR_REMAP 143u
#define HERCULES_SLOT_PALETTE_RESOURCE 145u
#define HERCULES_SLOT_MOVIE_DISPLAY 148u
#define HERCULES_SLOT_NOTIFICATION_ENABLE_RESET 161u
#define HERCULES_SLOT_PROJECTION_CENTER 174u
#define HERCULES_SLOT_PROJECTION_FOCAL 175u
#define HERCULES_SLOT_NOTIFICATION_DISABLE 240u
#define HERCULES_SLOT_SECONDARY_CALLBACK_EXCHANGE 249u

/* Native adapters are opt-in; raw table transport also works on 64-bit hosts.
 * AC60's EAX is unspecified and ignored at the reviewed call sites; void here
 * describes that observed use, not the historical source declaration. */
#ifdef HERCULES_RENDERER_LIVE_X86
RENDERER_ASSERT_LIVE_X86_POINTER_WIDTH(hercules_live_callbacks_require_x86);
#if defined(_MSC_VER)
#define HERCULES_CDECL __cdecl
#else
#define HERCULES_CDECL __attribute__((cdecl))
#endif
typedef int (HERCULES_CDECL *HerculesChainInitCallback)(void *, renderer_u32);
typedef void (HERCULES_CDECL *HerculesSubmitCallback)(void *);
/* BDA0 exchanges one raw x86 callback address and returns the previous one.
 * The installed callback's own ABI is unresolved; no callable type is inferred. */
typedef renderer_raw32 (HERCULES_CDECL *HerculesPrimaryCallbackExchange)(
    renderer_raw32 callback_address);
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Raw transport view of the observed 250-DWORD prefix. The authoritative
 * partial typed view remains calibration/engine_interface.h. */
#define ENGINE_TABLE_WORD_COUNT ((renderer_u32)250u)
#define ENGINE_TABLE_SIGNATURE ((renderer_u32)0x5ac00cacu)
#define ENGINE_TABLE_MAX_REPLACEMENTS ((renderer_u32)15u)

enum EngineTableCloneStatus {
    ENGINE_TABLE_CLONE_OK = 0,
    ENGINE_TABLE_CLONE_INVALID_ARGUMENT = 1,
    ENGINE_TABLE_CLONE_BAD_SIGNATURE = 2,
    ENGINE_TABLE_CLONE_SLOT_OUT_OF_RANGE = 3,
    ENGINE_TABLE_CLONE_SLOT_NOT_APPROVED = 4,
    ENGINE_TABLE_CLONE_DUPLICATE_SLOT = 5,
    ENGINE_TABLE_CLONE_TOO_MANY_REPLACEMENTS = 6
};

/* A narrow, pure operation: validate first, clone all 250 words, replace
 * approved callback words, and publish output only after full success. */
int engine_table_clone_selected(const renderer_u32 *source_words,
                                renderer_u32 *output_words,
                                const renderer_u32 *slots,
                                const renderer_u32 *values,
                                renderer_u32 replacement_count);

#ifdef __cplusplus
}
#endif

#endif
