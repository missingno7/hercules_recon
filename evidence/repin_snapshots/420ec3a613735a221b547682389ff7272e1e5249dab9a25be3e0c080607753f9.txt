/* ENG1 1a280/1a2b0/1a390. Numeric fields are evidence-backed views;
 * WORD-map roots use the existing complete owner, not duplicate storage.
 * Preserve live root reloads, low-WORD arithmetic and post-service flag reads.
 */
#include <stddef.h>

typedef struct PairActor {
    long x,y;
    unsigned long z;
    unsigned char unknown0c[4];
    unsigned short w10;
    unsigned char unknown12[0x10];
    unsigned char b22,b23;
    unsigned char unknown24[10];
    unsigned short w2e,w30,w32,w34,w36,w38,w3a,w3c,w3e;
    unsigned char unknown40[10];
    unsigned short w4a;
    long d4c;
    unsigned long d50,d54,d58,d5c,d60;
    unsigned short *d64;
    unsigned char unknown68[8];
    unsigned short w70,w72,w74;
    unsigned char unknown76[0x1e];
    unsigned char b94,b95;
} PairActor;

extern unsigned char *g_eng1_setup_pointer_72018,*g_eng1_setup_pointer_72164;
extern unsigned char *g_collision_context;
extern void *g_normal_actor_pool;
extern unsigned short g_eng1_config_word_pool_55478[96];
void reset_frame_state(void *);
int eng1_interpret_word_stream(void *);
void remove_middle_actor(void *);

#define COLLISION ((PairActor *)g_collision_context)
#define NORMAL ((PairActor *)g_normal_actor_pool)
typedef char pair_view[(offsetof(PairActor,w2e)==0x2e &&
    offsetof(PairActor,w3e)==0x3e && offsetof(PairActor,d64)==0x64 &&
    offsetof(PairActor,b94)==0x94)?1:-1];

void eng1_native_code_target_001a280(void)
{
    if (((PairActor *)g_eng1_setup_pointer_72018)->w2e != 0) {
        remove_middle_actor(g_eng1_setup_pointer_72018);
        remove_middle_actor(g_eng1_setup_pointer_72164);
    }
}

void eng1_native_code_target_01a2b0(PairActor *p)
{
    if (p == (PairActor *)g_eng1_setup_pointer_72018) {
        p->d64=g_eng1_config_word_pool_55478+24;
        p->w3e=(unsigned short)(COLLISION->w10+0x20);
    } else {
        p->d64=g_eng1_config_word_pool_55478+36;
        p->w3e=(unsigned short)(COLLISION->w10-0x20);
    }
    p->x=(long)((unsigned long)COLLISION->x-0x80000UL);
    p->y=(long)((unsigned long)COLLISION->y-0x300000UL);
    p->z=NORMAL->z;
    p->w3a=0x100;
    p->w70=0xa0;
    p->w72=0;
    p->w74=0;
    p->b22=2;
    reset_frame_state(p);
    eng1_interpret_word_stream(p);
    p->d50|=0x20000;
    p->d54|=0x2005;
    p->b23=5;
    p->w4a=0xa0;
    p->b94=COLLISION->b94;
    p->b95=0xff;
}

void eng1_native_code_target_01a390(PairActor *p)
{
    int index=NORMAL->w34;
    index-=0x15f;
    if(index<0)index=0;
    if(index>9)index=9;
    p->w72=g_eng1_config_word_pool_55478[48+index];
    if(eng1_interpret_word_stream(p)==1)eng1_native_code_target_001a280();
}

/* Native ENG1 0x1a410: conditionally unlink the setup actor and its sound id. */
extern PairActor *g_eng1_setup_pointer_72020;
extern PairActor *g_native_pointer_7217c;
extern int remove_actor_variant(PairActor *actor);
extern void RemoveFxIdLinks_7f8e(unsigned short id, void *owner);

void eng1_native_code_target_001a410(void)
{
    PairActor *actor = g_eng1_setup_pointer_72020;

    if (actor->w2e != 0) {
        remove_actor_variant(actor);
        RemoveFxIdLinks_7f8e(0x26, g_native_pointer_7217c);
    }
}
