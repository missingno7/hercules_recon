/* ENG1 26f70/27b00: first free slot from the current cursor, then the wrap range.
 * Success stores the found index, without advancing it or marking a reservation.
 * Sparse views recover only the kind offset and measured slot strides.
 * Direct global expressions preserve the original reload/ordering behavior.
 */
typedef unsigned short ActorKind;
typedef struct AlternatePoolSlot {
    unsigned char before_kind[0x2e];
    unsigned short kind;
    unsigned char after_kind[0x94-0x30];
} AlternatePoolSlot;
typedef struct NormalPoolSlot {
    unsigned char before_kind[0x2e];
    unsigned short kind;
    unsigned char after_kind[0x134-0x30];
} NormalPoolSlot;
typedef char alternate_stride_is_94[sizeof(AlternatePoolSlot)==0x94?1:-1];
typedef char normal_stride_is_134[sizeof(NormalPoolSlot)==0x134?1:-1];
typedef char int_is_32[sizeof(int)==4?1:-1];
extern NormalPoolSlot *g_normal_actor_pool;
extern AlternatePoolSlot *g_alternate_actor_pool;
extern unsigned short g_configuration_word_71430;
extern unsigned short g_configuration_word_7138c;
extern unsigned short g_configuration_word_71374;
extern unsigned short g_configuration_word_713c4;
extern unsigned short g_configuration_word_71378;
extern unsigned short g_configuration_word_713ba;
extern unsigned short g_configuration_word_71308;
extern unsigned short g_configuration_word_713b4;
extern unsigned short g_configuration_word_713b6;
extern unsigned short g_configuration_word_713a8;

NormalPoolSlot *allocate_normal_actor(ActorKind kind)
{
    int i;
    if (kind & 0x8000) {
        for (i=g_configuration_word_71308; i<g_configuration_word_713b4; ++i)
            if (g_normal_actor_pool[i].kind==0) break;
        if (i<g_configuration_word_713b4) {
            g_configuration_word_71308=(unsigned short)i;
            return &g_normal_actor_pool[i];
        }
        for(i=g_configuration_word_713a8;i<g_configuration_word_71308;++i)
            if(g_normal_actor_pool[i].kind==0)break;
        if(i<g_configuration_word_71308) {
            g_configuration_word_71308=(unsigned short)i;
            return &g_normal_actor_pool[i];
        }
        return 0;
    }
    for(i=g_configuration_word_713c4;i<g_configuration_word_713a8;++i)
        if(g_normal_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_713a8) {
        g_configuration_word_713c4=(unsigned short)i;
        return &g_normal_actor_pool[i];
    }
    for(i=g_configuration_word_71378;i<g_configuration_word_713c4;++i)
        if(g_normal_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_713c4) {
        g_configuration_word_713c4=(unsigned short)i;
        return &g_normal_actor_pool[i];
    }
    return 0;
}

AlternatePoolSlot *allocate_alternate_actor(ActorKind kind)
{
    int i;
    if (kind & 0x8000) {
        for (i=g_configuration_word_71374; i<g_configuration_word_713ba; ++i)
            if (g_alternate_actor_pool[i].kind==0) break;
        if (i<g_configuration_word_713ba) {
            g_configuration_word_71374=(unsigned short)i;
            return &g_alternate_actor_pool[i];
        }
        for(i=g_configuration_word_713b6;i<g_configuration_word_71374;++i)
            if(g_alternate_actor_pool[i].kind==0)break;
        if(i<g_configuration_word_71374) {
            g_configuration_word_71374=(unsigned short)i;
            return &g_alternate_actor_pool[i];
        }
        return 0;
    }
    for(i=g_configuration_word_71430;i<g_configuration_word_713b6;++i)
        if(g_alternate_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_713b6) {
        g_configuration_word_71430=(unsigned short)i;
        return &g_alternate_actor_pool[i];
    }
    for(i=g_configuration_word_7138c;i<g_configuration_word_71430;++i)
        if(g_alternate_actor_pool[i].kind==0)break;
    if(i<g_configuration_word_71430) {
        g_configuration_word_71430=(unsigned short)i;
        return &g_alternate_actor_pool[i];
    }
    return 0;
}
