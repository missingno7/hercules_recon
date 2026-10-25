/* Bounded ENG1 copied-interface wrapper candidate.
 *
 * The table is the measured local copy in calibration/engine_interface.h.
 * Members which are still opaque are read through explicit function-pointer
 * ABI views here; this file does not change the shared representation.
 */
#include "../shared/engine_interface.h"

typedef void (__cdecl *Eng1Slot0)(void);
typedef void (__cdecl *Eng1SlotWord)(unsigned long);
typedef void (__cdecl *Eng1SlotWords2)(unsigned long, unsigned long);
typedef void (__cdecl *Eng1SlotWords3)(unsigned long, unsigned long,
                                       unsigned long);
typedef void (__cdecl *Eng1SlotCallback)(EngineNoArgCallback);
typedef void (__cdecl *Eng1SlotConstPointer)(const void *);
typedef void (__cdecl *Eng1SlotPointer)(void *);
typedef void (__cdecl *Eng1SlotPointerPair)(void *, const void *);

/* ENG1:2c530 -> copied-table offset 0xd0, slot 52, host EXE:1a00.
 * The only argument is a no-argument callback pointer. */
void __cdecl dispatch_local_2c530(EngineNoArgCallback callback)
{
    ((Eng1SlotCallback)g_engine_interface.opaque_0d0[0])(callback);
}

/* ENG1:2c5b0 -> copied-table offset 0xf0, slot 60, host EXE:1f50. */
void __cdecl dispatch_local_2c5b0(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_0f0[0])();
}

/* ENG1:2c650 -> copied-table offset 0x178, slot 94, host EXE:be30. */
void __cdecl dispatch_local_2c650(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_174[1])();
}

/* ENG1:2c660 -> copied-table offset 0x180, slot 96, host EXE:be30. */
void __cdecl dispatch_local_2c660(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_174[3])();
}

/* ENG1:2ca90 -> copied-table offset 0x324, slot 201, host EXE:be30. */
void __cdecl dispatch_local_2ca90(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_2f0[13])();
}

/* ENG1:2cad0 -> copied-table offset 0x368, slot 218, host EXE:c220. */
void __cdecl dispatch_local_2cad0(unsigned long value)
{
    ((Eng1SlotWord)g_engine_interface.opaque_334[13])(value);
}

/* ENG1:2cae0 -> copied-table offset 0x36c, slot 219, host EXE:be30. */
void __cdecl dispatch_local_2cae0(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_334[14])();
}

/* Geometry and packet helpers. Pointer mutability is kept opaque where the
 * observed caller proves only a 32-bit address, not a source-level type. */
void __cdecl dispatch_engine_2c670(unsigned long first,
                                   unsigned long second,
                                   unsigned long third)
{
    ((Eng1SlotWords3)g_engine_interface.opaque_174[7])(
        first, second, third);
}

void __cdecl dispatch_engine_2c690(const void *vector)
{
    ((Eng1SlotConstPointer)g_engine_interface.opaque_174[8])(vector);
}

void __cdecl dispatch_engine_2c6d0(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_174[11])();
}

void __cdecl dispatch_engine_2c6e0(void)
{
    ((Eng1Slot0)g_engine_interface.opaque_174[13])();
}

void __cdecl dispatch_engine_2c700(const void *matrix)
{
    ((Eng1SlotConstPointer)g_engine_interface.opaque_174[15])(matrix);
}

void __cdecl dispatch_engine_2c710(const void *translation)
{
    ((Eng1SlotConstPointer)g_engine_interface.opaque_174[16])(translation);
}

void __cdecl dispatch_engine_2c740(void *result)
{
    ((Eng1SlotPointer)g_engine_interface.opaque_174[19])(result);
}

void __cdecl dispatch_engine_2c770(void *packet_vertex)
{
    ((Eng1SlotPointer)g_engine_interface.opaque_174[24])(packet_vertex);
}

void __cdecl dispatch_engine_2c790(void *packet_vertex)
{
    ((Eng1SlotPointer)g_engine_interface.opaque_174[26])(packet_vertex);
}

void __cdecl dispatch_engine_2c7f0(void *rectangle, const void *texels)
{
    ((Eng1SlotPointerPair)g_engine_interface.opaque_1fc[2])(
        rectangle, texels);
}

void __cdecl dispatch_engine_2c920(void *matrix, const void *angles)
{
    ((Eng1SlotPointerPair)g_engine_interface.opaque_284[3])(
        matrix, angles);
}

void __cdecl dispatch_engine_2c940(void *matrix, const void *vector)
{
    ((Eng1SlotPointerPair)g_engine_interface.opaque_284[4])(
        matrix, vector);
}

void __cdecl dispatch_engine_2c9d0(unsigned long center_x,
                                   unsigned long center_y)
{
    ((Eng1SlotWords2)g_engine_interface.opaque_2b0[2])(
        center_x, center_y);
}

void __cdecl dispatch_engine_2c9f0(unsigned long focal_value)
{
    ((Eng1SlotWord)g_engine_interface.opaque_2b0[3])(focal_value);
}
