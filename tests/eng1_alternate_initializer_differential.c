#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/interpreters.h"
#include "../src/eng1/read_guard.h"
#include "../src/eng1/flags_word_programs.h"

typedef void (__cdecl *NativeEfa0)(Actor *);

static unsigned char *native_module;
static const short *guard_program;
static unsigned long guard_words;
static unsigned long guard_calls;
static unsigned long guard_bad;
static unsigned long checks;
static unsigned long failures;

extern void __cdecl eng1_native_code_target_00efa0(Actor *actor);

unsigned long g_configuration_dword_71384;

static void check(int condition, const char *label)
{
    ++checks;
    if (!condition) {
        ++failures;
        printf("FAIL %s\n", label);
    }
}

static void forbidden(const char *name)
{
    printf("unexpected interpreter service: %s\n", name);
    ExitProcess(0xe0);
}

void eng1_guard_program_read(const void *program, long index,
                             unsigned int stride, unsigned int width,
                             const char *site)
{
    (void)site;
    ++guard_calls;
    if (program != (const void *)guard_program || index < 0 ||
        stride != 2U || width != 2U || (unsigned long)index >= guard_words)
        ++guard_bad;
}

void eng1_guard_frame_slot(const void *frames, long index, const char *site)
{
    (void)frames; (void)index; (void)site;
    forbidden("eng1_guard_frame_slot");
}

void __cdecl eng1_guarded_advance_frame(Actor *actor)
{
    (void)actor;
    forbidden("eng1_guarded_advance_frame");
}

void __cdecl eng1_guarded_select_frame(Actor *actor, int index)
{
    (void)actor; (void)index;
    forbidden("eng1_guarded_select_frame");
}

int __cdecl remove_actor(Actor *actor)
{
    (void)actor;
    forbidden("remove_actor");
    return 0;
}

void __cdecl remove_actor_preserving_resource(Actor *actor)
{
    (void)actor;
    forbidden("remove_actor_preserving_resource");
}

void __cdecl mark_actor_resource_for_release(Actor *actor)
{
    (void)actor;
    forbidden("mark_actor_resource_for_release");
}

void __cdecl spawn_actor_with_owner(Actor *actor, long x, long y, long z,
                                    unsigned short kind)
{
    (void)actor; (void)x; (void)y; (void)z; (void)kind;
    forbidden("spawn_actor_with_owner");
}

void __cdecl spawn_actor_at_group(long x, long y, long z,
                                  unsigned short kind,
                                  unsigned short group)
{
    (void)x; (void)y; (void)z; (void)kind; (void)group;
    forbidden("spawn_actor_at_group");
}

void __cdecl play_sound(unsigned short effect_id, Actor *actor)
{
    (void)effect_id; (void)actor;
    forbidden("play_sound");
}

void __cdecl stop_actor_sounds(Actor *actor)
{
    (void)actor;
    forbidden("stop_actor_sounds");
}

int __cdecl wrapped_ratio(int current, int target, int duration)
{
    (void)current; (void)target; (void)duration;
    forbidden("wrapped_ratio");
    return 0;
}

unsigned int __cdecl dispatch_engine_2c460(int opcode)
{
    (void)opcode;
    forbidden("dispatch_engine_2c460");
    return 0;
}

static unsigned char *native_rva(unsigned long rva)
{
    return native_module + rva;
}

static void seed_actor(Actor *actor, const short *program,
                       unsigned short old_word_04a,
                       unsigned long old_flags,
                       unsigned long old_render_flags)
{
    unsigned char *bytes = (unsigned char *)actor;

    memset(bytes, 0xa5, sizeof(*actor));
    *(unsigned short *)(void *)(bytes + 0x22) = 0x3b17U;
    bytes[0x23] = 0xc9U;
    bytes[0x24] = 0x62U;
    *(unsigned short *)(void *)(bytes + 0x2a) = 0x7143U;
    *(unsigned short *)(void *)(bytes + 0x2e) = 0x2022U;
    *(unsigned short *)(void *)(bytes + 0x30) = 0x6789U;
    *(unsigned short *)(void *)(bytes + 0x32) = 0x2345U;
    *(unsigned short *)(void *)(bytes + 0x48) = 0x5511U;
    *(unsigned short *)(void *)(bytes + 0x4a) = old_word_04a;
    *(unsigned long *)(void *)(bytes + 0x4c) = 0x76543210UL;
    *(unsigned long *)(void *)(bytes + 0x50) = old_flags;
    *(unsigned long *)(void *)(bytes + 0x54) = old_render_flags;
    *(unsigned long *)(void *)(bytes + 0x64) = (unsigned long)program;
    *(unsigned short *)(void *)(bytes + 0x124) = 0x1a2bU;
}

static void run_case(NativeEfa0 native, const char *label,
                     const short *source_program,
                     const short *native_program,
                     unsigned short old_word_04a,
                     unsigned long old_flags,
                     unsigned long old_render_flags,
                     unsigned long expected_guard_reads,
                     short expected_state, short expected_command,
                     short expected_ticks, unsigned long expected_render)
{
    Actor source_actor;
    Actor native_actor;
    unsigned char *source_bytes = (unsigned char *)&source_actor;
    unsigned char *native_bytes = (unsigned char *)&native_actor;
    unsigned char source_copy[sizeof(Actor)];
    unsigned char native_copy[sizeof(Actor)];
    unsigned long before_guard_calls = guard_calls;

    seed_actor(&source_actor, source_program, old_word_04a, old_flags,
               old_render_flags);
    seed_actor(&native_actor, native_program, old_word_04a, old_flags,
               old_render_flags);
    guard_program = source_program;
    guard_words = 3UL;
    guard_bad = 0;

    native(&native_actor);
    eng1_native_code_target_00efa0(&source_actor);

    memcpy(source_copy, source_bytes, sizeof(source_copy));
    memcpy(native_copy, native_bytes, sizeof(native_copy));
    memset(source_copy + 0x64, 0, 4U);
    memset(native_copy + 0x64, 0, 4U);
    check(memcmp(source_copy, native_copy, sizeof(source_copy)) == 0,
          "whole Actor state source/native equality");
    check(*(unsigned long *)(void *)(source_bytes + 0x64) ==
          (unsigned long)source_program &&
          *(unsigned long *)(void *)(native_bytes + 0x64) ==
          (unsigned long)native_program, "selected owner pointer retained");
    check(*(unsigned long *)(void *)(source_bytes + 0x50) == 0x00010000UL,
          "DWORD +0x50 overwritten");
    check(*(unsigned long *)(void *)(source_bytes + 0x54) == expected_render,
          "DWORD +0x54 OR and interpreter result");
    check(source_bytes[0x23] == 5U, "BYTE +0x23 set");
    check(*(unsigned short *)(void *)(source_bytes + 0x4a) == 0,
          "WORD +0x4a cleared");
    check(source_bytes[0x22] == 0x17U &&
          source_bytes[0x24] == 0x62U, "category neighbors preserved");
    check(*(unsigned short *)(void *)(source_bytes + 0x48) == 0x5511U &&
          *(unsigned long *)(void *)(source_bytes + 0x4c) == 0x76543210UL,
          "word +0x4a neighbors preserved");
    check(*(short *)(void *)(source_bytes + 0x32) == expected_state,
          "word-stream frame state");
    check(*(short *)(void *)(source_bytes + 0x34) == expected_command,
          "word-stream active command");
    check(*(short *)(void *)(source_bytes + 0x30) == expected_ticks,
          "word-stream duration");
    check(guard_bad == 0, "source interpreter stayed within test prefix");
    check(guard_calls - before_guard_calls == expected_guard_reads,
          "source interpreter read count");

    if (failures == 0)
        printf("PASS efa0 %s source/native owner pointers retained\n", label);
    else
        printf("CASE efa0 %s has failures\n", label);
}

int main(int argc, char **argv)
{
    HMODULE module;
    NativeEfa0 native;
    const short *source_program;
    const short *native_program;
    unsigned short *source_words;
    const unsigned short *native_words;

    if (argc != 2) return 2;
    module = LoadLibraryExA(argv[1], 0, DONT_RESOLVE_DLL_REFERENCES);
    if (module == 0) return 3;
    native_module = (unsigned char *)module;
    native = (NativeEfa0)(void *)native_rva(0xefa0UL);
    source_words = g_eng1_flag_program_051b20;
    native_words = (const unsigned short *)(const void *)native_rva(0x51b20UL);
    source_program = (const short *)(const void *)source_words;
    native_program = (const short *)(const void *)native_words;

    check(memcmp(source_words, native_words, 6U) == 0,
          "existing 0x51b20 owner matches native bytes");
    check(source_words[0] == 999U && source_words[1] == 9U &&
          source_words[2] == 0xffffU, "bounded 0x51b20 program values");

    run_case(native, "kind-34 asset program", source_program, native_program,
             0x5a6bU, 0xdeadbeefUL, 0x12340010UL, 2UL,
             1, 9, 999, 0x12340614UL);
    run_case(native, "alternate initial render state", source_program,
             native_program, 0xbeefU, 0x11223344UL, 0x00000010UL, 2UL,
             1, 9, 999, 0x00000614UL);

    check(FreeLibrary(module) != 0, "native DLL unloaded");
    printf("eng1_call_efa0 cases=2 checks=%lu failures=%lu\n",
           checks, failures);
    return failures == 0 ? 0 : 1;
}
