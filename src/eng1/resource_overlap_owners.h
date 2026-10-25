/* Measured source views; one owner per native interval. */
#ifndef ENG1_RESOURCE_OVERLAP_OWNERS_H
#define ENG1_RESOURCE_OVERLAP_OWNERS_H
#include <stddef.h>
#include "frame_programs.h"
typedef char eng1_pair_is_four_bytes[(sizeof(Eng1CommandPair) == 4) ? 1 : -1];
typedef struct Eng1ResourceOwner_04fec0 {
    unsigned char prefix_04fec0_to_0500e8[0x228];
    Eng1CommandPair row2_actor_pair_0500e8[2];
} Eng1ResourceOwner_04fec0;
typedef char eng1_owner_04fec0_pair_offset[(offsetof(Eng1ResourceOwner_04fec0, row2_actor_pair_0500e8) == 0x228) ? 1 : -1];
typedef char eng1_owner_04fec0_size[(sizeof(Eng1ResourceOwner_04fec0) == 560) ? 1 : -1];
typedef struct Eng1ResourceOwner_050d88 {
    unsigned char prefix_050d88_to_050e20[0x98];
    unsigned short config_word_pool_50e20[159];
} Eng1ResourceOwner_050d88;
typedef char eng1_owner_050d88_pool_offset[(offsetof(Eng1ResourceOwner_050d88, config_word_pool_50e20) == 0x98) ? 1 : -1];
typedef char eng1_owner_050d88_size[(sizeof(Eng1ResourceOwner_050d88) == 470) ? 1 : -1];
extern Eng1ResourceOwner_04fec0 g_eng1_resource_owner_04fec0;
extern Eng1ResourceOwner_050d88 g_eng1_resource_owner_050d88;
#define eng1_resource_byte_list_04fec0 ((unsigned char *)&g_eng1_resource_owner_04fec0)
#define eng1_resource_byte_list_050d88 ((unsigned char *)&g_eng1_resource_owner_050d88)
#define g_eng1_unreadable_initialized_identity_04fec0 (eng1_resource_byte_list_04fec0[0])
#define g_eng1_unreadable_initialized_identity_050d88 (eng1_resource_byte_list_050d88[0])
#define g_eng1_row2_actor_pair_0500e8 (g_eng1_resource_owner_04fec0.row2_actor_pair_0500e8)
#define g_eng1_config_word_pool_50e20 (g_eng1_resource_owner_050d88.config_word_pool_50e20)
typedef char eng1_pair_alias_width[(sizeof(g_eng1_row2_actor_pair_0500e8) == 8) ? 1 : -1];
typedef char eng1_word_pool_alias_width[(sizeof(g_eng1_config_word_pool_50e20) == 318) ? 1 : -1];
typedef char eng1_identity_04fec0_alias_width[(sizeof(g_eng1_unreadable_initialized_identity_04fec0) == 1) ? 1 : -1];
typedef char eng1_identity_050d88_alias_width[(sizeof(g_eng1_unreadable_initialized_identity_050d88) == 1) ? 1 : -1];
#endif
