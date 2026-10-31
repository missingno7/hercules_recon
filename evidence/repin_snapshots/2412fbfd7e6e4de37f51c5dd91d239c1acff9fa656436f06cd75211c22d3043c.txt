/* Private ENG1 copied-interface forwarding wrappers, remaining geometry subset.
 * Prototype shapes are physical x86 cdecl views measured from PC wrapper bytes
 * and direct callers; pointer pointees and original source declarations remain
 * opaque. The table stays the real copied EngineInterface object.
 */
#include "../shared/engine_interface.h"

typedef void (__cdecl *Eng1Slot0)(void);
typedef void (__cdecl *Eng1SlotPointer)(void *);

/* ENG1:2c6c0 is a tail jump with no stack arguments. The copied cell is
 * interface+0x19c (opaque_174[10]), host EXE RVA 0xb840. */
void __cdecl dispatch_engine_2c6c0(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_174[10])();
}

/* ENG1:2c750 forwards its sole caller-supplied address unchanged to
 * interface+0x1c4 (opaque_174[20]), host EXE RVA 0xb810. */
void __cdecl dispatch_engine_2c750(void *visit_vector)
{
    ((Eng1SlotPointer)g_engine_interface.opaque_174[20])(visit_vector);
}

/* ENG1:2c760 forwards one writable output address to interface+0x1c8
 * (opaque_174[21]), host EXE RVA 0xb720. */
void __cdecl dispatch_engine_2c760(void *screen_words)
{
    ((Eng1SlotPointer)g_engine_interface.opaque_174[21])(screen_words);
}

/* ENG1:2c7b0 forwards one address to interface+0x1e4 (opaque_174[28]),
 * host EXE RVA 0xb870. Its native caller later consumes only the written AX
 * word; this wrapper does not infer a higher-level depth type. */
void __cdecl dispatch_engine_2c7b0(void *depth_word)
{
    ((Eng1SlotPointer)g_engine_interface.opaque_174[28])(depth_word);
}
