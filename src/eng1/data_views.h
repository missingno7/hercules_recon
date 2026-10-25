#ifndef HERCULES_ENG1_BEHAVIORAL_DATA_VIEWS_H
#define HERCULES_ENG1_BEHAVIORAL_DATA_VIEWS_H
#include "../shared/engine_interface.h"
#include "data_layouts.h"
#include "group_layouts.h"

/* Interior views of measured native objects. Include after a recovered unit's
 * declarations: the old extern names acquire no independent storage. These
 * views preserve original row strides and actual shared cells. */
#define g_frame_descriptor_links_5a69c ((unsigned char *)eng1_descriptor_rows_5a690 + 12)
#define g_dispatch_descriptors_5a6a4 ((unsigned long *)((unsigned char *)eng1_descriptor_rows_5a690 + 20))
#define g_frame_config_5df08 ((unsigned char *)eng1_profile_rows_5df08)
#define g_mode_lookup_5df0b (g_frame_config_5df08 + 3)

/* The context pointer is member +4 of the copied 250-DWORD interface. */
#define g_resource_context (g_engine_interface.context_004)

#endif
