#include <windows.h>

#include "hercules_host_abi.h"
#include "probe_diag.h"

/* Pinned PE32 host has no base-relocation directory, so it must load at its
 * preferred base 0x400000. The controller verifies that before publication. */
#define HOST_PREFERRED_BASE 0x00400000UL

static renderer_u32 g_private_clone[ENGINE_TABLE_WORD_COUNT];
static renderer_u32 g_selected_slot = HERCULES_SLOT_SUBMIT;
static renderer_u32 g_selected_value = 0xf17e0089UL;

__declspec(dllexport) ProbeDiagnosticV1 ProbeDiagnostic = {
    PREENTRY_DIAG_MAGIC, PREENTRY_DIAG_STATE_INITIAL, 0UL, 0UL, 0UL,
    0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL
};

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    renderer_u32 *pointer_cell;
    renderer_u32 *source_words;
    renderer_u32 status;

    (void)instance;
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    ProbeDiagnostic.process_attach_calls += 1UL;
    ProbeDiagnostic.state = PREENTRY_DIAG_STATE_ATTACHED;
    ProbeDiagnostic.module_base = HOST_PREFERRED_BASE;
    pointer_cell = (renderer_u32 *)(HOST_PREFERRED_BASE +
        HERCULES_HOST_TABLE_POINTER_RVA);
    source_words = (renderer_u32 *)(HOST_PREFERRED_BASE +
        HERCULES_HOST_TABLE_RVA);
    ProbeDiagnostic.source_pointer_cell = (renderer_u32)pointer_cell;
    ProbeDiagnostic.source_pointer_before = *pointer_cell;
    ProbeDiagnostic.source_word0 = source_words[0];
    ProbeDiagnostic.source_word1 = source_words[1];
    ProbeDiagnostic.source_slot89 = source_words[HERCULES_SLOT_SUBMIT];

    if (ProbeDiagnostic.source_pointer_before != (renderer_u32)source_words ||
        source_words[0] != ENGINE_TABLE_SIGNATURE) {
        ProbeDiagnostic.state = PREENTRY_DIAG_STATE_SOURCE_MISMATCH;
        return TRUE;
    }

    status = (renderer_u32)engine_table_clone_selected(
        source_words, g_private_clone, &g_selected_slot,
        &g_selected_value, 1UL);
    ProbeDiagnostic.clone_status = status;
    if (status != ENGINE_TABLE_CLONE_OK) {
        ProbeDiagnostic.state = PREENTRY_DIAG_STATE_CLONE_FAILED;
        return TRUE;
    }

    ProbeDiagnostic.clone_pointer = (renderer_u32)g_private_clone;
    ProbeDiagnostic.clone_word0 = g_private_clone[0];
    ProbeDiagnostic.clone_word1 = g_private_clone[1];
    ProbeDiagnostic.clone_slot89 = g_private_clone[HERCULES_SLOT_SUBMIT];
    *pointer_cell = (renderer_u32)g_private_clone;
    ProbeDiagnostic.source_pointer_after = *pointer_cell;
    ProbeDiagnostic.state = PREENTRY_DIAG_STATE_PUBLISHED;
    return TRUE;
}

HRESULT WINAPI DirectDrawCreate(void *guid, void **out_object, void *outer)
{
    (void)guid;
    (void)outer;
    ProbeDiagnostic.directdrawcreate_calls += 1UL;
    if (out_object != 0)
        *out_object = 0;
    return (HRESULT)0x80004001UL; /* E_NOTIMPL; no system forwarding. */
}
