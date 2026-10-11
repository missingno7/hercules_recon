/* Matching provider for ENG1 RVA 0x15600; original TU and names are unproved. */
#include "../engine_interface.h"

typedef struct ContextDefaultsObservedPrefix {
    unsigned char opaque_000[0x0c];
    signed char signed_byte_00c;
    unsigned char opaque_00d[0x11 - 0x0d];
    unsigned char byte_011;
    unsigned char opaque_012[0x1c - 0x12];
    unsigned short word_01c;
    unsigned short word_01e;
    unsigned short word_020;
} ContextDefaultsObservedPrefix;

typedef char context_defaults_view_is_22_bytes[
    sizeof(ContextDefaultsObservedPrefix) == 0x22 ? 1 : -1];
typedef char context_defaults_view_offsets[
    (offsetof(ContextDefaultsObservedPrefix, signed_byte_00c) == 0x0c &&
     offsetof(ContextDefaultsObservedPrefix, byte_011) == 0x11 &&
     offsetof(ContextDefaultsObservedPrefix, word_01c) == 0x1c &&
     offsetof(ContextDefaultsObservedPrefix, word_01e) == 0x1e &&
     offsetof(ContextDefaultsObservedPrefix, word_020) == 0x20) ? 1 : -1];

#define CURRENT_CONTEXT_DEFAULTS \
    ((ContextDefaultsObservedPrefix *)g_engine_interface.context_004)

extern unsigned long g_context_default_dword_721a8;
extern unsigned long g_context_default_dword_72190;

void __cdecl eng1_context_defaults(void)
{
    g_context_default_dword_721a8 = 1;
    g_context_default_dword_72190 = 0;
    CURRENT_CONTEXT_DEFAULTS->word_01e = 0x80;
    CURRENT_CONTEXT_DEFAULTS->word_020 = 0;
    CURRENT_CONTEXT_DEFAULTS->word_01c = 0;
    CURRENT_CONTEXT_DEFAULTS->byte_011 = 0;
    if (CURRENT_CONTEXT_DEFAULTS->signed_byte_00c == 0) {
        CURRENT_CONTEXT_DEFAULTS->signed_byte_00c = 1;
    }
    if (CURRENT_CONTEXT_DEFAULTS->signed_byte_00c > 9) {
        CURRENT_CONTEXT_DEFAULTS->signed_byte_00c = 9;
    }
    ((unsigned char *)g_engine_interface.context_004)[0x0d] = 0;
}
