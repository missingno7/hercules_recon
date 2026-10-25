/*
 * Private ENG1 resource-state candidate interface.
 *
 * Prefix bounds and ownership are evidence-backed observations, not claims
 * about the original source declarations or full runtime capacities.
 */
#ifndef ENG1_RESOURCE_RUNTIME_DATA_H
#define ENG1_RESOURCE_RUNTIME_DATA_H

#include "../shared/resource_record.h"
#include <stddef.h>

typedef union PendingActionGroup PendingActionGroup;
union PendingActionGroup {
    long count;
    void *entries[1];
};

typedef struct ResourceKindInfo ResourceKindInfo;
struct ResourceKindInfo {
    void *owner;
    unsigned long count;
};

#include "group_layouts.h"

typedef char resource_kind_info_is_8[(sizeof(ResourceKindInfo) == 8) ? 1 : -1];
typedef char resource_kind_info_count_at_4[(offsetof(ResourceKindInfo, count) == 4) ? 1 : -1];

extern PendingActionGroup *pending_action_group_table[20];
extern ResourceKindInfo g_kind_info[1];
extern ActorResource *g_active_resource_record;
extern int g_loading_resource;
extern unsigned long g_reported_total_bytes;
extern unsigned long g_resource_start_tick;
extern const char g_resource_suffix[];
extern const char g_provider_bad_id_format[];

extern unsigned char g_packed_byte_70cf0;
extern unsigned char g_packed_byte_70cf1;
extern unsigned char g_packed_byte_70cf2;
extern signed char g_active_category_70cfe;
extern unsigned char g_mode_table_value_70d00;
extern signed char g_active_row_70d01;
extern short g_selected_width_70d12;
extern short g_element_width_70d14;

extern unsigned char *g_header_pointer_70d28;
extern unsigned char *g_header_pointer_70d2c;
extern unsigned char *g_header_pointer_70d30;
extern unsigned char *g_header_pointer_70d34;
extern unsigned char *g_header_pointer_70d38;
extern unsigned char *g_header_pointer_70d3c;
extern unsigned char *g_header_pointer_70d40;
extern unsigned char *g_header_pointer_70d44;
extern unsigned char *g_header_pointer_70d48;
extern unsigned char *g_header_pointer_70d4c;
extern unsigned char *g_header_pointer_70d50;
extern unsigned char *g_header_pointer_70d54;
extern unsigned char *g_header_pointer_70d58;
extern unsigned char *g_root_group_rows_70d5c[4];
extern unsigned char *g_header_pointer_70d6c;
extern unsigned long g_reset_dwords_70d70[15];
extern unsigned short g_group_reset_count_70dac;
extern unsigned short g_reset_words_70dae[15];
extern unsigned char *g_rebuilt_row_pointers_70dcc[16];
extern unsigned short g_rebuilt_row_offsets_70e0c[32];

/* One physical owner covers the cumulative-word and following bounds lanes. */
extern ResourceGroupState g_resource_group_state_70e4c;
extern unsigned char g_clear_byte_70ecc;
extern signed char g_header_byte_70ecd;
extern signed char g_group_count_70ece;
extern signed char g_group_flag_70ecf;
extern signed char g_root_count_70ed0;
extern signed char g_root_count_70ed1;
extern signed char g_root_count_70ed2;
extern signed char g_root_count_70ed3;
extern unsigned char *g_resource_root;
extern unsigned char *g_allocated_row_slots_71e40[32];

/* Source-level names for independently observed references to shared cells. */
#define g_special_kind g_active_category_70cfe
#define g_frame_byte_70d01 g_active_row_70d01
#define g_frame_word_70d12 g_selected_width_70d12

#endif
