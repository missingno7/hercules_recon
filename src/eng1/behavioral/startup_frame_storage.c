/*
 * Private exact-cell providers needed by the reviewed ENG1 startup/level
 * callers. These declarations describe independent observed cells, not a
 * recovered aggregate or original source unit.
 */

/* ENG1 RVA 0x70d20: one loader-zero DWORD written/read by 0x1f750. */
unsigned long g_frame_file_size_70d20;

/* ENG1 RVA 0x5ce48: the native byte store sets this latch to one; the target
 * file byte is initially 0xff. No adjacent extent is claimed. */
unsigned char g_frame_transition_flag_5ce48 = 0xffU;

/*
 * ENG1 RVAs 0x73490, 0x734e4 and 0x734f8: the level initializer writes
 * 0xffffffff to each before traversing it. The cells lie in the loader-zero
 * tail of .data; DWORD access and single-cell extents are established. Their
 * linked-list pointees and original aggregate/source ownership are not.
 */
unsigned long g_frame_owner_list_73490;
unsigned long g_frame_resource_list_734e4;
unsigned long g_frame_actor_list_734f8;
