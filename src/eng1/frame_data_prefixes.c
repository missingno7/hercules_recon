/*
 * Bounded initialized-data prefixes for ENG1's frame callback dispatch and
 * scripted actor sequence. These objects reproduce only the measured prefix;
 * they do not claim either source array's original declaration or capacity.
 */
typedef void (__cdecl *Eng1FrameNodeCallback)(void *node);
typedef signed long Eng1ScriptDword;

typedef char eng1_frame_prefix_pointer_is_32bit[
    (sizeof(Eng1FrameNodeCallback) == 4) ? 1 : -1];
typedef char eng1_script_prefix_dword_is_32bit[
    (sizeof(Eng1ScriptDword) == 4) ? 1 : -1];

/* The native 0x3a50 consumer passes one node and caller-cleans four bytes.
 * The observed first 40 bytes at 0x4d6f8 are nine relocated targets and NULL.
 * Initializer 0x21980 writes index 5 at node +0x23, proving one normal
 * in-prefix value. No complete bound is claimed for the consumer's byte index. */
extern void __cdecl eng1_native_code_target_003f60(void *node);
extern void __cdecl eng1_native_code_target_004010(void *node);
extern void __cdecl eng1_native_code_target_0303d0(void *node);
extern void __cdecl eng1_native_code_target_030430(void *node);
extern void __cdecl eng1_native_code_target_0304f0(void *node);
extern void __cdecl eng1_native_code_target_030a20(void *node);
extern void __cdecl eng1_native_code_target_004c10(void *node);
extern void __cdecl eng1_empty_c720(void *node);
extern void __cdecl eng1_native_code_target_0310e0(void *node);

Eng1FrameNodeCallback g_frame_callbacks_4d6f8_prefix[10] = {
    eng1_native_code_target_003f60,
    eng1_native_code_target_004010,
    eng1_native_code_target_0303d0,
    eng1_native_code_target_030430,
    eng1_native_code_target_0304f0,
    eng1_native_code_target_030a20,
    eng1_native_code_target_004c10,
    eng1_empty_c720,
    eng1_native_code_target_0310e0,
    0
};

/* The 0x21980 initializer (paired with tick_scripted_actor at 0x219e0 in
 * the initialized pair at 0x59f80) clears its phase word at +0x32. The tick
 * consumer reads this table at stride four and stops on the negative value at
 * index eight. Thus this nine-DWORD prefix covers that observed normal path;
 * it is not a claim about bytes or indices beyond the sentinel. */
Eng1ScriptDword g_frame_script_58d50_prefix[9] = {
    0L, 0L, 1L, 1L, 2L, 2L, 3L, 3L, -1L
};
