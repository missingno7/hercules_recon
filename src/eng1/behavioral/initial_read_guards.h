#ifndef ENG1_INITIAL_READ_GUARDS_H
#define ENG1_INITIAL_READ_GUARDS_H
void eng1_guard_initial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site);
void eng1_guard_initial_palette(const unsigned short *source,
                                unsigned long word_count, const char *site);
void eng1_guard_initial_mask(const unsigned long *source,
                             unsigned long rows, const char *site);
void __cdecl eng1_guarded_apply_frame_motion_2d(void *actor);
void __cdecl eng1_guarded_apply_frame_motion_3d(void *actor);
void eng1_guard_unreadable_identity(const void *base, const char *site);
#endif
