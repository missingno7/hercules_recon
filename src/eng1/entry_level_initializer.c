/* ENG1 33e10: initialize the measured actor fields, run the existing frame
 * services, attach the complete Entry table, then reload the resulting count.
 * Uses the existing WORD owner at55478+120 rather than duplicate storage.
 * This field view/physical pointer ABI does not identify historical C types.
 */
#include <stddef.h>

typedef struct EntryLevelActor {
    unsigned char unknown00[0x22];
    unsigned char category, mode;
    unsigned char unknown24[0x12];
    unsigned short entry_count;
    unsigned char unknown38[0x12];
    unsigned short unknown4a;
    long parameter;
    unsigned long flags, render_flags;
    unsigned char unknown58[12];
    unsigned short *image64;
    unsigned char unknown68[12];
    unsigned short angle74;
    unsigned char unknown76[0x22];
    unsigned short unknown98;
} EntryLevelActor;

extern unsigned short g_eng1_config_word_pool_55478[96];
extern unsigned char g_level_entries_5e688[272];
void reset_frame_state(void *);
void eng1_interpret_word_stream(void *);
void attach_entries(void *, void *);

typedef char entry_level_layout[(offsetof(EntryLevelActor, entry_count)==0x36 &&
    offsetof(EntryLevelActor, image64)==0x64 &&
    offsetof(EntryLevelActor, unknown98)==0x98)?1:-1];

void eng1_33e10(EntryLevelActor *p)
{
    p->flags |= 0x20800;
    p->render_flags |= 0x20000000;
    p->mode = 4;
    p->angle74 = 0;
    p->unknown4a = 0xe0;
    p->image64 = g_eng1_config_word_pool_55478 + 60;
    p->category = 2;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
    attach_entries(p, g_level_entries_5e688);
    p->unknown98 = p->entry_count;
    p->parameter = 6;
}
