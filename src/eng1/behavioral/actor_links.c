#include <stddef.h>
#include "../../../calibration/engine_interface.h"
#include "../../../calibration/engine_fx_record.h"

typedef struct ActorLinkView ActorLinkView;
struct ActorLinkView {
    long x_000, y_004, z_008;
    unsigned char unknown_00c[0x22 - 0x0c];
    unsigned char category_022, mode_023;
    unsigned char unknown_024[6];
    short state_02a;
    unsigned char unknown_02c[0x38 - 0x2c];
    unsigned short word_038;
    unsigned char unknown_03a[0x50 - 0x3a];
    unsigned long flags_050, render_flags_054;
    unsigned char unknown_058[4];
    ActorLinkView *attachment_05c;
    ActorLinkView *child_060;
};

typedef char actor_link_state_at_02a[
    (offsetof(ActorLinkView, state_02a) == 0x2a) ? 1 : -1];
typedef char actor_link_word_at_038[
    (offsetof(ActorLinkView, word_038) == 0x38) ? 1 : -1];
typedef char actor_link_flags_at_050[
    (offsetof(ActorLinkView, flags_050) == 0x50) ? 1 : -1];
typedef char actor_link_attachment_at_05c[
    (offsetof(ActorLinkView, attachment_05c) == 0x5c) ? 1 : -1];

typedef struct AlternateCallbackView {
    void (__cdecl *initialize)(ActorLinkView *actor);
    void *unknown_004;
} AlternateCallbackView;

typedef void (__cdecl *FxStopChannelCallback)(int channel);

extern long g_busy;
extern long g_countdown;
extern ActorLinkView *spawn_kind(long x, long y, long z, unsigned short kind);
extern void detach_attachment(ActorLinkView *actor);
extern AlternateCallbackView *g_alt_callbacks;
extern void __cdecl dispatch_local_2c650(void);
extern void __cdecl dispatch_local_2c660(void);

void finish_actor(ActorLinkView *actor)
{
    g_busy = 0;
    actor->state_02a = 0;
    g_countdown = 30;
}

ActorLinkView *attach_actor(ActorLinkView *actor, int kind)
{
    ActorLinkView *attached;

    if (actor->flags_050 & 0x00020000UL)
        return 0;

    if (actor->attachment_05c)
        detach_attachment(actor);

    attached = spawn_kind(actor->x_000, actor->y_004, actor->z_008,
                          (unsigned short)kind);
    if (attached) {
        actor->attachment_05c = attached;
        attached->attachment_05c = actor;
        attached->flags_050 = 0x00010000UL;
        attached->word_038 = actor->word_038;
        g_alt_callbacks[kind & 0x0fff].initialize(attached);
    }

    return attached;
}

void remove_fx_links(ActorLinkView *owner)
{
    int i;

    if (!owner)
        return;

    dispatch_local_2c650();
    for (i = 0; i < 24; ++i) {
        FxSoundRecordView *row = &g_engine_fx_records[i];

        if (row->unknown_030 == (unsigned long)owner) {
            if ((row->unknown_034 & 0x00008000UL) == 0) {
                ((FxStopChannelCallback)g_engine_interface.opaque_334[20])(i);
            }
            row->unknown_030 = 0;
        }
    }
    dispatch_local_2c660();
}
