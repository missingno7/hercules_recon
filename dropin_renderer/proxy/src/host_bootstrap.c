/* Modern integration only. No I/O, allocation, library loading, backend
 * invocation, threads, COM or display work in this attach path. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "dropin_renderer/host_bootstrap.h"

/* FNV-1a over mapped .text VirtualSize and the full initialized table.
 * These are compatibility discriminators, not an authenticity/security check.
 * The pinned host has no relocations and must use its preferred base.
 * Provenance and SHA-256 identities: docs/combined_dll.md. */
#if defined(DROPIN_SYNTHETIC_HOST_PROFILE)
/* Generated only by the synthetic runner; never part of a production build. */
#include "synthetic_host_profile.h"
#else
#define HS_HOST_BASE 0x400000u
#define HS_HOST_IMAGE_SIZE 0x7ad000u
#define HS_HOST_TEXT_RVA 0x1000u
#define HS_HOST_TEXT_SIZE 0x2cdbeu
#define HS_HOST_TEXT_HASH 0x080fb4c55acc7402ULL
#define HS_HOST_TABLE_HASH 0x97949ff63f2069e7ULL
#define HS_HOST_TABLE_RVA HERCULES_HOST_TABLE_RVA
#define HS_HOST_POINTER_RVA HERCULES_HOST_TABLE_POINTER_RVA
#define HS_HOST_PRIMARY_RVA 0x68e780u
#endif

static HsBootstrapStatus g_status;

static unsigned __int64 content_hash(const unsigned char *bytes, unsigned size)
{
    unsigned __int64 hash = 0xcbf29ce484222325ULL;
    unsigned i;
    for (i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

static HsBootstrapStatus publish_known_host(unsigned char *base)
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS32 *nt;
    const IMAGE_SECTION_HEADER *sections;
    const renderer_u32 *table;
    renderer_u32 *cell;
    unsigned i;
    int found_text = 0;
    HsConfig config = {sizeof(HsConfig), HS_MODE_FORWARD_ONLY, 0, 0};

    if ((UINT_PTR)base != HS_HOST_BASE || dos->e_magic != IMAGE_DOS_SIGNATURE ||
        dos->e_lfanew < sizeof(*dos) || dos->e_lfanew > 0x1000)
        return HS_BOOTSTRAP_UNSUPPORTED_HOST;
    nt = (const IMAGE_NT_HEADERS32 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER32) ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->OptionalHeader.ImageBase != HS_HOST_BASE ||
        nt->OptionalHeader.SizeOfImage != HS_HOST_IMAGE_SIZE ||
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size != 0 ||
        nt->FileHeader.NumberOfSections == 0 || nt->FileHeader.NumberOfSections > 32)
        return HS_BOOTSTRAP_UNSUPPORTED_HOST;
    sections = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        const IMAGE_SECTION_HEADER *s = &sections[i];
        if (s->Name[0] == '.' && s->Name[1] == 't' && s->Name[2] == 'e' &&
            s->Name[3] == 'x' && s->Name[4] == 't' && s->Name[5] == 0) {
            if (found_text || s->VirtualAddress != HS_HOST_TEXT_RVA ||
                s->Misc.VirtualSize != HS_HOST_TEXT_SIZE ||
                HS_HOST_TEXT_RVA > HS_HOST_IMAGE_SIZE ||
                HS_HOST_TEXT_SIZE > HS_HOST_IMAGE_SIZE - HS_HOST_TEXT_RVA ||
                !(s->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
                content_hash(base + HS_HOST_TEXT_RVA, HS_HOST_TEXT_SIZE) != HS_HOST_TEXT_HASH)
                return HS_BOOTSTRAP_UNSUPPORTED_HOST;
            found_text = 1;
        }
    }
    if (!found_text)
        return HS_BOOTSTRAP_UNSUPPORTED_HOST;
    table = (const renderer_u32 *)(base + HS_HOST_TABLE_RVA);
    cell = (renderer_u32 *)(base + HS_HOST_POINTER_RVA);
    if (content_hash((const unsigned char *)table, ENGINE_TABLE_WORD_COUNT * 4) !=
            HS_HOST_TABLE_HASH || *cell != (renderer_u32)(UINT_PTR)table)
        return HS_BOOTSTRAP_TABLE_MISMATCH;
    config.primary_callback_cell = (const volatile renderer_u32 *)(base + HS_HOST_PRIMARY_RVA);
    return hs_install(table, cell, &config, 0) == HS_INSTALLED
        ? HS_BOOTSTRAP_INSTALLED : HS_BOOTSTRAP_INSTALL_FAILED;
}

void hs_bootstrap_attach(void)
{
    wchar_t disabled[2];
    /* Any nonempty value opts out, including values larger than this buffer.
     * GetEnvironmentVariable/GetModuleHandle are kernel calls; no deferred
     * initialization or backend work is initiated here. */
    if (GetEnvironmentVariableW(L"HERCULES_SHIM_DISABLE", disabled, 2) != 0) {
        g_status = HS_BOOTSTRAP_DISABLED;
        return;
    }
    __try {
        g_status = publish_known_host((unsigned char *)GetModuleHandleW(0));
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                    ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        g_status = HS_BOOTSTRAP_UNSUPPORTED_HOST;
    }
}

HS_API HsBootstrapStatus HS_CALL hs_bootstrap_status(void)
{
    return g_status;
}
