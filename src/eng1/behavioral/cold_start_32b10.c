/* Private ordinary-C model for ENG1 entry 0x32b10.
 * Address labels are observed cells, not original names, aggregates, TU or
 * native placement claims. The byte view is only the 0x1c span touched here. */
#include "../../../calibration/engine_interface.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/* Native writes cover [0x70820, 0x7083c). No full owner/extent is known. */
extern u8 g_cold_start_state_bytes_70820[0x1c];

/* The first four target words at RVA 0x5df00, kept as values rather than an
 * address/placement alias into the separately represented profile rows. */
extern u16 g_stpanel1_upload_rectangle_words[4];

extern void *__cdecl engine_pool_allocate(void **output_slot, u32 bytes,
                                          u32 flags);
extern u32 __cdecl engine_file_load_sync(const char *prefix,
                                          const char *suffix,
                                          void *destination, u32 bytes);
extern void __cdecl dispatch_engine_2c7f0(void *rectangle,
                                           const void *texels);
extern void __cdecl dispatch_engine_2c630(u32 value);
extern void __cdecl dispatch_engine_2c2d0(void *allocation);

void __cdecl dispatch_local_32b10(void)
{
    u8 *cells;
    void *allocation;

    cells = g_cold_start_state_bytes_70820;
    *(u16 *)(void *)(cells + 0x00) = 0;
    *(u16 *)(void *)(cells + 0x02) = 0;
    *(u16 *)(void *)(cells + 0x04) = 0xffff;
    *(u16 *)(void *)(cells + 0x06) = 0xffff;
    cells[0x08] = 2;
    cells[0x09] = 3;
    cells[0x0a] = 2;
    *(u16 *)(void *)(cells + 0x10) = 0;
    *(u16 *)(void *)(cells + 0x12) = 0;
    cells[0x14] = 0;
    *(u16 *)(void *)(cells + 0x18) = 0x00d0;
    *(u16 *)(void *)(cells + 0x1a) = 0x00d0;

    allocation = (void *)0;
    (void)engine_pool_allocate(&allocation, 0x21000UL, 0UL);
    (void)engine_file_load_sync("", "m:\\language\\srclang\\stpanel1.dat",
                                allocation, 0x21000UL);
    dispatch_engine_2c7f0((void *)g_stpanel1_upload_rectangle_words,
                          allocation);
    dispatch_engine_2c630(0UL);
    dispatch_engine_2c2d0(allocation);
}
