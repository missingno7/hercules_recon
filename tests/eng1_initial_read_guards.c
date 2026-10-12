#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "../src/eng1/behavioral/initial_read_guards.h"
#include "../src/eng1/behavioral/interpreters.h"
#include "../src/eng1/behavioral/initial_custom_roots.h"
#include "../src/eng1/behavioral/initial_motion_roots.h"
#include "../src/eng1/behavioral/initial_unreadable_identities.h"
#include "../src/eng1/behavioral/descriptor_prefix_data.h"
extern unsigned short g_eng1_sound_words_66d68[8];
static jmp_buf escape;
static unsigned long reported;
void eng1_incomplete_behavior(unsigned long rva, const char *reason)
{ reported = (reason && *reason) ? rva : 0; longjmp(escape,1); }
/* Test-only uncalled callback definitions resolve genuine data relocations.
 * They are never providers for the production DLL or a callback behavior test. */
void eng1_native_code_target_034b40(void *p,int i) { (void)p; (void)i; }
void eng1_native_code_target_034bb0(void *p,int i) { (void)p; (void)i; }
short g_actor_token_references[32][2];
void *g_frame_owner_7348c;
unsigned char g_frame_global_73498, g_frame_setup_result_734ec;
unsigned short g_frame_seed_words_4d818[32];
void __cdecl dispatch_local_05780(const unsigned short *,unsigned long,unsigned long);

static int reject(const void *base,unsigned long index,unsigned long stride,
                  unsigned long width,unsigned long expected)
{
    reported=0;
    if (!setjmp(escape)) {
        eng1_guard_initial_read(base,index,stride,width,"boundary fixture");
        return 0;
    }
    return reported==expected;
}
static unsigned short destination[32*256];
int main(void)
{
    Actor actor;
    unsigned short dynamic_palette[256];
    short dynamic_steps[2] = { 2, -3 };
    const unsigned short *palette = (const unsigned short *)
                        &g_eng1_custom_mask_palette_05ebb8.mask_cells[5];
    unsigned long i;
    long x,y,z;
    if (setjmp(escape)) return 1;
    eng1_guard_initial_read(g_eng1_initial_motion_0527d0,8,4,4,"last pair");
    eng1_guard_initial_read(g_eng1_initial_motion_060148,14,6,6,"last triple");
    eng1_guard_initial_read(g_eng1_initial_motion_05fa20,6,6,6,"last related triple");
    eng1_guard_initial_read(g_eng1_initial_motion_066230,0,6,6,"single proven triple");
    eng1_guard_initial_read(&g_eng1_custom_image_05ebf0,0,1,36,"full header");
    eng1_guard_initial_read(eng1_resource_byte_list_04fec0,556,1,1,"last proven descriptor byte");
    eng1_guard_initial_read(eng1_resource_byte_list_050d88,166,1,1,"last proven descriptor byte");
    eng1_guard_initial_read(&g_eng1_descriptor_payload_4f350,15,1,1,"last header byte");
    eng1_guard_initial_read((const unsigned char *)&g_eng1_descriptor_payload_4f350+4,11,1,1,"last interior header byte");
    if (!reject(&g_eng1_descriptor_payload_4f350,16,1,1,0x4f350)) return 34;
    if (!reject((const unsigned char *)&g_eng1_descriptor_payload_4f350+4,12,1,1,0x4f354)) return 35;
    if (!reject(g_eng1_initial_motion_0527d0,9,4,4,0x527d0)) return 2;
    if (!reject(g_eng1_initial_motion_060148,15,6,6,0x60148)) return 3;
    if (!reject(g_eng1_initial_motion_060148,0xffffffffUL,6,6,0x60148)) return 4;
    if (!reject(g_eng1_initial_motion_0527d0,0,0,4,0x527d0)) return 5;
    if (!reject(&g_eng1_custom_image_05ebf0,36,1,1,0x5ebf0)) return 6;
    if (!reject(eng1_resource_byte_list_04fec0,557,1,1,0x4fec0)) return 7;
    if (!reject(eng1_resource_byte_list_050d88,167,1,1,0x50d88)) return 8;
    if (!reject(&g_eng1_unreadable_initialized_identity_065750,0,1,1,0x65750)) return 9;
    if (!reject(g_eng1_initial_motion_05fa20,7,6,6,0x5fa20)) return 30;
    if (!reject(g_eng1_initial_motion_066230,1,6,6,0x66230)) return 31;

    eng1_guard_initial_read(g_eng1_sound_words_66d68,7,2,2,"last sound WORD");
    if (!reject(g_eng1_sound_words_66d68,8,2,2,0x66d68)) return 36;
    if (!reject(g_eng1_sound_words_66d68,0xffffffffUL,2,2,0x66d68)) return 37;

    memset(&actor,0,sizeof(actor)); actor.x=7; actor.y=11; actor.z=13;
    actor.steps=(void *)1; actor.step_index=(long)0x80000000UL; actor.active_step=-1;
    if (setjmp(escape)) return 10;
    eng1_guarded_apply_frame_motion_2d(&actor);
    eng1_guarded_apply_frame_motion_3d(&actor);
    if (actor.x!=7 || actor.y!=11 || actor.z!=13) return 11;
    actor.steps=g_eng1_initial_motion_0527d0; actor.step_index=8; actor.active_step=0;
    actor.direction_x=1; actor.vertical_step=-2; x=actor.x; y=actor.y; z=actor.z;
    eng1_guarded_apply_frame_motion_2d(&actor);
    if (actor.x!=x-((long)g_eng1_initial_motion_0527d0[8].x<<16) ||
        actor.y!=y+((long)g_eng1_initial_motion_0527d0[8].y<<16) || actor.z!=z-(2L<<16)) return 12;
    actor.step_index=9; x=actor.x; y=actor.y; z=actor.z; reported=0;
    if (!setjmp(escape)) { eng1_guarded_apply_frame_motion_2d(&actor); return 13; }
    if (reported!=0x527d0 || actor.x!=x || actor.y!=y || actor.z!=z) return 14;
    actor.steps=g_eng1_initial_motion_060148; actor.step_index=14;
    actor.direction_x=0; actor.direction_z=0; x=actor.x;y=actor.y;z=actor.z;
    if (setjmp(escape)) return 15;
    eng1_guarded_apply_frame_motion_3d(&actor);
    if (actor.x!=x+((long)g_eng1_initial_motion_060148[14].x<<16) ||
        actor.y!=y+((long)g_eng1_initial_motion_060148[14].y<<16) ||
        actor.z!=z-((long)g_eng1_initial_motion_060148[14].z<<16)) return 16;
    actor.steps=dynamic_steps;actor.step_index=0;actor.direction_x=0;x=actor.x;y=actor.y;
    eng1_guarded_apply_frame_motion_2d(&actor);
    if (actor.x!=x+(2L<<16) || actor.y!=y-(3L<<16)) return 17;

    for (i=0;i<32UL*256UL;i++) destination[i]=0xaaaa;
    g_frame_owner_7348c=destination;g_frame_global_73498=0;
    reported=0;
    if (!setjmp(escape)) { dispatch_local_05780(palette,0,256); return 18; }
    if (reported!=0x5ebd0 || g_frame_global_73498) return 19;
    for (i=0;i<32UL*256UL;i++) if (destination[i]!=0xaaaa) return 20;
    if (!setjmp(escape)) { dispatch_local_05780(palette,0,0xffffffffUL); return 21; }
    if (reported!=0x5ebd0 || destination[0]!=0xaaaa) return 22;
    if (setjmp(escape)) return 23;
    dispatch_local_05780(palette,0,16);
    if (destination[0]!=0 || destination[15]!=(unsigned short)(palette[15]|0x8000U) ||
        destination[16]!=0 || destination[31]!=destination[15] || !g_frame_global_73498) return 24;
    for (i=0;i<256;i++) dynamic_palette[i]=(unsigned short)(i+3);
    dispatch_local_05780(dynamic_palette,1,256);
    for (i=0;i<256;i++) if (destination[256+i]!=dynamic_palette[i]) return 25;
    dispatch_local_05780((const unsigned short *)&g_eng1_unreadable_initialized_identity_04fec0,2,0);
    dispatch_local_05780((const unsigned short *)&g_eng1_unreadable_initialized_identity_04fec0,2,1);
    eng1_guard_initial_mask(&g_eng1_custom_mask_palette_05ebb8.columns,3,"initial mask");
    reported=0;
    if (!setjmp(escape)) { eng1_guard_initial_mask(&g_eng1_custom_mask_palette_05ebb8.columns,4,"continued mask"); return 26; }
    if (reported!=0x5ebb8) return 27;
    if (!setjmp(escape)) { eng1_guard_initial_mask(&g_eng1_custom_mask_palette_05ebb8.columns,0xffffffffUL,"overflow mask"); return 28; }
    if (reported!=0x5ebb8) return 29;
    g_eng1_custom_mask_palette_05ebb8.columns=0xffffffffUL;
    if (!setjmp(escape)) { eng1_guard_initial_mask(&g_eng1_custom_mask_palette_05ebb8.columns,2,"column overflow mask"); return 32; }
    if (reported!=0x5ebb8) return 33;
    g_eng1_custom_mask_palette_05ebb8.columns=4;
    puts("Initial reads: boundaries, overflow, motion suppression/directions, palette reject-before-writes and finite mask passed");
    return 0;
}
