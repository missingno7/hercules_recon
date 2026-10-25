#include <stdio.h>

typedef void (__cdecl *ModeHandler)(void);

extern void __cdecl eng1_native_code_target_0016840(void);
extern void __cdecl eng1_native_code_target_0016b40(void);
extern void __cdecl eng1_native_code_target_0016da0(void);
extern void __cdecl eng1_native_code_target_0016f40(void);
extern void __cdecl eng1_native_code_target_00171b0(void);
extern void __cdecl eng1_native_code_target_0017400(void);
extern void __cdecl eng1_native_code_target_00177a0(void);
extern void __cdecl eng1_native_code_target_00178f0(void);
extern void __cdecl eng1_native_code_target_0017a90(void);
extern void __cdecl eng1_native_code_target_0017c30(void);
extern void __cdecl eng1_native_code_target_0017c70(void);
extern void __cdecl eng1_native_code_target_0017cd0(void);
extern void __cdecl eng1_native_code_target_0017eb0(void);
extern void __cdecl eng1_native_code_target_0018240(void);
extern void __cdecl eng1_native_code_target_00182c0(void);
extern void __cdecl eng1_native_code_target_0018550(void);
extern void __cdecl eng1_native_code_target_0018770(void);
extern void __cdecl eng1_native_code_target_0018830(void);
extern void __cdecl eng1_native_code_target_00379e0(void);

static unsigned calls[19];

#define DEFINE_TARGET(rva, slot) \
    void __cdecl eng1_##rva(void) { ++calls[slot]; }

DEFINE_TARGET(16840, 0)
DEFINE_TARGET(16b40, 1)
DEFINE_TARGET(16da0, 2)
DEFINE_TARGET(16f40, 3)
DEFINE_TARGET(171b0, 4)
DEFINE_TARGET(17400, 5)
DEFINE_TARGET(177a0, 6)
DEFINE_TARGET(178f0, 7)
DEFINE_TARGET(17a90, 8)
DEFINE_TARGET(17c30, 9)
DEFINE_TARGET(17c70, 10)
DEFINE_TARGET(17cd0, 11)
DEFINE_TARGET(17eb0, 12)
DEFINE_TARGET(18240, 13)
DEFINE_TARGET(182c0, 14)
DEFINE_TARGET(18550, 15)
DEFINE_TARGET(18770, 16)
DEFINE_TARGET(18830, 17)
DEFINE_TARGET(379e0, 18)

int main(void)
{
    static ModeHandler const aliases[19] = {
        eng1_native_code_target_0016840, eng1_native_code_target_0016b40,
        eng1_native_code_target_0016da0, eng1_native_code_target_0016f40,
        eng1_native_code_target_00171b0, eng1_native_code_target_0017400,
        eng1_native_code_target_00177a0,
        eng1_native_code_target_00178f0, eng1_native_code_target_0017a90,
        eng1_native_code_target_0017c30, eng1_native_code_target_0017c70,
        eng1_native_code_target_0017cd0, eng1_native_code_target_0017eb0,
        eng1_native_code_target_0018240, eng1_native_code_target_00182c0,
        eng1_native_code_target_0018550, eng1_native_code_target_0018770,
        eng1_native_code_target_0018830,
        eng1_native_code_target_00379e0
    };
    unsigned i, j;

    for (i = 0; i < 19; ++i) {
        unsigned before[19];
        for (j = 0; j < 19; ++j)
            before[j] = calls[j];
        aliases[i]();
        for (j = 0; j < 19; ++j) {
            unsigned expected = before[j] + (i == j ? 1U : 0U);
            if (calls[j] != expected) {
                printf("FAIL alias %u target %u: got %u expected %u\n",
                       i, j, calls[j], expected);
                return 1;
            }
        }
    }

    puts("19 no-argument cdecl mode-handler bindings reached their corresponding bodies");
    return 0;
}
