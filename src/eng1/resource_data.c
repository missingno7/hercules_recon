/*
 * ENG1 PC-backed resource state provider candidate.
 *
 * The 81 initialized ActorResource rows and the token/link prefixes remain in
 * calibration/resource_data.c.  The copied EngineInterface object and its
 * context_004/allocate_078 members remain in engine_interface_storage.c; this
 * file deliberately does not define g_resource_context or callback pointers.
 *
 * Mutable state below is in the loader-zero tail of the target .data section,
 * except the explicitly initialized loading id and read-only format strings.
 * Prefix lengths are those supported by the indexed PC accesses / adjacent
 * separately named cells.  They do not prove full production capacities,
 * original source declarations, TU ownership, or native linker placement.
 */
#include "resource_runtime_data.h"

/*
 * The selected file-backed registry records expose counts up to twenty, so
 * ENG1 RVA 0x713e0 needs this many zero-filled pointer slots.  The physical
 * table capacity and allocation owner are not established by that lower bound.
 */
PendingActionGroup *pending_action_group_table[20];

/*
 * ENG1 RVA 0x6fee8: resource_rebuild_blob reads an eight-byte stride here
 * (owner pointer, then 32-bit count). Each of the seven archive-matched
 * MAP_PSX.BIN files selected by the initialized ENG1 path prefixes has native
 * root-group count 1, so this is the observed one-entry prefix for those files.
 * It is not a
 * full-capacity claim; the initializer uses a runtime signed count and other
 * resource/runtime states remain unbounded by the available evidence.
 */
ResourceKindInfo g_kind_info[1];

/* ENG1 RVA 0x72864; writable zero-filled live active-record pointer. */
ActorResource *g_active_resource_record;

/* ENG1 RVA 0x4d7f8; the initialized target dword is 0xffffffff. */
int g_loading_resource = -1;

/* ENG1 RVAs 0x6ee6c and 0x6ee70; loader-zero four-byte fields. */
unsigned long g_reported_total_bytes;
unsigned long g_resource_start_tick;

/* File-backed target literals at ENG1 RVAs 0x4d914 and 0x4d924. */
const char g_resource_suffix[] = "\\ANIMPSX.BIN";
const char g_provider_bad_id_format[] = "Illegal Dbase %d \n";

/*
 * Scalar cells reached by the resource routines, all in the loader-zero
 * .data tail. Native stores/loads prove byte width for 0x70cf0..0x70cf2,
 * 0x70cfe, 0x70d00, 0x70d01; word width for 0x70d12 and 0x70d14.
 * g_special_kind is an alias/name used by actor-transition consumers for the
 * same physical byte as g_active_category_70cfe; define one owner only.
 * Likewise frame_bootstrap's g_frame_byte_70d01 and g_frame_word_70d12 names
 * refer to the two resource-facing cells below. Their native word/byte widths
 * and shared addresses are established; original source/TU ownership is not.
 */
unsigned char g_packed_byte_70cf0;
unsigned char g_packed_byte_70cf1;
unsigned char g_packed_byte_70cf2;
signed char g_active_category_70cfe;
unsigned char g_mode_table_value_70d00;
signed char g_active_row_70d01;
short g_selected_width_70d12;
short g_element_width_70d14;

/*
 * Rebind state cells, ENG1 RVAs 0x70d28..0x70ed7.  Each header pointer below
 * is one observed four-byte cell.  The four-entry root-row pointer prefix ends
 * immediately before the separately written header pointer at 0x70d6c; the
 * parser's input count has no proven maximum, so this is not a capacity claim.
 */
unsigned char *g_header_pointer_70d28;
unsigned char *g_header_pointer_70d2c;
unsigned char *g_header_pointer_70d30;
unsigned char *g_header_pointer_70d34;
unsigned char *g_header_pointer_70d38;
unsigned char *g_header_pointer_70d3c;
unsigned char *g_header_pointer_70d40;
unsigned char *g_header_pointer_70d44;
unsigned char *g_header_pointer_70d48;
unsigned char *g_header_pointer_70d4c;
unsigned char *g_header_pointer_70d50;
unsigned char *g_header_pointer_70d54;
unsigned char *g_header_pointer_70d58;
unsigned char *g_root_group_rows_70d5c[4];
unsigned char *g_header_pointer_70d6c;

/* Native selector reset loops clear exactly fifteen dwords and fifteen words. */
unsigned long g_reset_dwords_70d70[15];
unsigned short g_group_reset_count_70dac;
unsigned short g_reset_words_70dae[15];

/*
 * The row builder addresses sixteen pointer slots and two sixteen-word offset
 * lanes. Its signed row count is not clamped, so these are supported prefixes
 * only. The cumulative-word table and following six-byte bounds records are a
 * single observed contiguous prefix through 0x70ecb; no maximum row/group
 * count is established.
 */
unsigned char *g_rebuilt_row_pointers_70dcc[16];
unsigned short g_rebuilt_row_offsets_70e0c[32];
/*
 * The traversal reads one cumulative WORD per row at 0x70e4c. The same
 * adjacent allocation contains 16 observed six-byte bounds records beginning
 * at 0x70e6c. Native reset/camera writers use WORD offsets +0 and +2; +4 has
 * no established meaning. A signed runtime group count controls the writers,
 * so these are observed prefixes rather than proven maximum capacities.
 * Keep the contiguous cells in one storage owner; consumers should name
 * `g_resource_group_state_70e4c.cumulative_by_row` and `.bounds_by_row`.
 */
ResourceGroupState g_resource_group_state_70e4c;

/* Four independently addressed loader-zero bytes at 0x70ecc..0x70ecf. */
unsigned char g_clear_byte_70ecc;
signed char g_header_byte_70ecd;
signed char g_group_count_70ece;
signed char g_group_flag_70ecf;

/* Four independently addressed loader-zero bytes at 0x70ed0..0x70ed3. */
signed char g_root_count_70ed0;
signed char g_root_count_70ed1;
signed char g_root_count_70ed2;
signed char g_root_count_70ed3;

/* ENG1 RVA 0x70ed4; movable root allocation owner, loader-zero initially. */
unsigned char *g_resource_root;

/*
 * Independent row-allocation owner slots at ENG1 RVA 0x71e40. The explicit
 * teardown loop visits four category bases (0..3) and eight rows per category,
 * reading/freeing 32 DWORD pointer cells through the exclusive loop bound
 * 0x71e60. Allocation and rebind consumers calculate signed category*8+row
 * without a general bounds check, so this is the observed 32-slot ownership
 * region, not a claim that malformed runtime indices are safe or that no
 * larger original capacity exists. The following separately used cell starts
 * at 0x71ec0; no row slots beyond this observed region are synthesized.
 */
unsigned char *g_allocated_row_slots_71e40[32];
