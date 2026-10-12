#ifndef ENG1_INCOMPLETE_H
#define ENG1_INCOMPLETE_H
/* An explicit unsupported path reports its original identity and cannot return
 * success. Entries belong in evidence/eng1_behavioral_incomplete.json. */
#if defined(_MSC_VER) && _MSC_VER >= 1200
#define ENG1_NORETURN __declspec(noreturn)
#else
#define ENG1_NORETURN
#endif
ENG1_NORETURN void __cdecl eng1_incomplete_behavior(
    unsigned long original_rva, const char *reason);
#endif
