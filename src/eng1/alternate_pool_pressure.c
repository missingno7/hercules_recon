/* ENG1 1b1d0: release the first requested kind only when no primary slot is free.
 * This sparse view records only the observed kind offset and slot stride.
 * The DWORD argument is compared with a zero-extended WORD, without truncation.
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct AlternateSlot {
    unsigned char before_kind[0x2e];
    unsigned short kind;
    unsigned char remainder[0x94-0x30];
} AlternateSlot;

typedef char slot_is_94[sizeof(AlternateSlot)==0x94?1:-1];
typedef char int_is_32[sizeof(int)==4?1:-1];
extern AlternateSlot *g_alternate_actor_pool;
extern unsigned short g_configuration_word_713ba;
extern int __cdecl release_actor(void *);

void __cdecl eng1_native_code_target_001b1d0(int kind)
{
    int count=g_configuration_word_713ba;
    AlternateSlot *pool=g_alternate_actor_pool;
    AlternateSlot *at=pool;
    int i;

    for(i=0;i<count;++i,++at)
        if(at->kind==0)return;
    for(i=0,at=pool;i<count;++i,++at)
        if(at->kind==kind)goto release;
    return;
release:
    release_actor(at);
}

#ifdef __cplusplus
}
#endif
