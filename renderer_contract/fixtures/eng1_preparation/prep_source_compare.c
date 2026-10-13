/* Bounded VC5 source replay for the selected ENG1 2E3B0 path.
 *
 * The C source under test is the current behavioral mesh_prepare.c. Its
 * external HERC callbacks are bound to strict, data-only spies that accept
 * only the observed zero-angle / uniform-scale case and use the shared
 * integer projection provider for B920/B740. This is not an original-code
 * emulator or a general HERC service implementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "renderer_contract/hercules_projection.h"
#include "src/eng1/behavioral/mesh_visit.h"
#include "calibration/engine_interface.h"

typedef struct NativePrepCase {
    int mesh_call_index;
    int mesh_index;
    int return_value;
    int return_depth;
    unsigned long header_000_map_offset;
    unsigned long header_004_map_offset;
    unsigned long child_map_offset;
    int camera_angle_x;
    long scale_longs[3];
    int expected_b6f0_xyz[3];
    int entry_matrix_words[9];
    int expected_matrix_words[9];
    int entry_translation[3];
    int expected_translation[3];
    int expected_camera[3];
    unsigned int expected_screen[2];
    unsigned int expected_callback_count;
    unsigned int expected_callback_ids[12];
    unsigned char entry_visit[0x74];
    unsigned char return_visit[0x74];
    unsigned char entry_matrix_bytes[0x40];
    unsigned char return_matrix_bytes[0x40];
    unsigned char entry_scratch[0x60];
    unsigned char return_scratch[0x60];
} NativePrepCase;

#include "prep_case_data.h"

EngineInterface g_engine_interface;
long g_camera_angle_x_70cd0;
long g_mesh_scale_5ceb0[3];
long g_mesh_cached_column_6ffb8;

static unsigned char *map_bytes;
static short trig_table[4096];
static unsigned char matrix_bytes[0xa0];
static struct hercules_projection_config_view projection_config;
static struct hercules_projection_point_view projection_input;
static struct hercules_projection_result_view projection_result;
static const NativePrepCase *active_case;
static Eng1MeshVisit *active_visit;
static unsigned int callback_ids[16];
static unsigned int callback_count;
static int check_count, failure_count;
static int projection_ready;

#define CHECK(c, msg) do { ++check_count; if (!(c)) { \
    ++failure_count; printf("FAIL call=%d mesh=%d: %s\n", \
        active_case ? active_case->mesh_call_index : -1, \
        active_case ? active_case->mesh_index : -1, msg); \
} } while (0)

static void require_supported(int condition, const char *identity)
{
    if (!condition) {
        printf("UNSUPPORTED host service input call=%d mesh=%d service=%s\n",
               active_case ? active_case->mesh_call_index : -1,
               active_case ? active_case->mesh_index : -1, identity);
        fflush(stdout);
        exit(90);
    }
}

static void log_callback(unsigned int identity)
{
    if (callback_count >= sizeof(callback_ids) / sizeof(callback_ids[0])) {
        require_supported(0, "callback sequence exceeds measured bound");
        return;
    }
    callback_ids[callback_count++] = identity;
}

/* Raw HERC slot 164 reached through original ENG1 wrapper 2C920. The source
 * wrapper's local prototype has the reversed parameter names; the physical
 * stack order here is angles, matrix, as established by native reads at BB00. */
static void __cdecl slot164_angles_matrix(void *angles_arg,
                                           const void *matrix_arg)
{
    const short *angles = (const short *)angles_arg;
    short *matrix = (short *)matrix_arg;
    int i;
    log_callback(0xBB00);
    require_supported(matrix_arg == matrix_bytes, "BB00 matrix pointer");
    require_supported(angles[0] == 0 && angles[1] == 0 && angles[2] == 0,
                      "BB00 nonzero Euler angles");
    for (i = 0; i != 9; ++i)
        matrix[i] = (short)((i % 4) == 0 ? 0x1000 : 0);
}

/* Raw HERC slot 165, bounded to the observed identity matrix and scale. */
static void __cdecl slot165_scale_matrix(void *matrix_arg,
                                          const void *scale_arg)
{
    short *matrix = (short *)matrix_arg;
    const long *scale = (const long *)scale_arg;
    int i;
    log_callback(0xBC30);
    require_supported(matrix_arg == matrix_bytes, "BC30 matrix pointer");
    require_supported(scale == g_mesh_scale_5ceb0, "BC30 scale pointer");
    require_supported(scale[0] == 0x2000 && scale[1] == 0x2000 &&
                       scale[2] == 0x2000, "BC30 unmeasured scale");
    for (i = 0; i != 9; ++i) {
        short expected = (short)((i % 4) == 0 ? 0x1000 : 0);
        require_supported(matrix[i] == expected, "BC30 nonidentity source matrix");
        if ((i % 4) == 0) matrix[i] = (short)scale[i / 4];
    }
}

/* HERC B880: measured 18-byte matrix load from the source-owned workspace. */
static void __cdecl slot108_load_matrix(const void *source)
{
    int i;
    log_callback(0xB880);
    require_supported(source == matrix_bytes, "B880 matrix source");
    for (i = 0; i != 9; ++i)
        projection_config.matrix_row_major[i] = ((const short *)source)[i];
}

/* HERC B8F0: three translation DWORDs from matrix+0x14. */
static void __cdecl slot109_load_translation(const void *source)
{
    int i;
    const unsigned char *bytes = (const unsigned char *)source;
    log_callback(0xB8F0);
    require_supported(source == matrix_bytes, "B8F0 matrix source");
    for (i = 0; i != 3; ++i)
        memcpy(&projection_config.translation_xyz[i], bytes + 0x14 + i * 4, 4);
}

/* HERC B6F0: read the three input WORDs, preserving their signed values. */
static void __cdecl slot101_point_input(const void *point_words)
{
    int i;
    const short *words = (const short *)point_words;
    log_callback(0xB6F0);
    require_supported(point_words == matrix_bytes + 0x40, "B6F0 scratch input address");
    for (i = 0; i != 3; ++i) {
        projection_input.xyz[i] = (renderer_i16)words[i];
        CHECK(words[i] == active_case->expected_b6f0_xyz[i],
              "B6F0 source words differ from native original reads");
    }
    projection_ready = 0;
}

/* HERC B920 plus nested B740 are bound to the already verified source model. */
static void __cdecl slot106_project(void)
{
    log_callback(0xB920);
    log_callback(0xB740);
    hercules_projection_eval(&projection_config, &projection_input,
                             &projection_result);
    CHECK(projection_result.camera_x == active_case->expected_camera[0] &&
          projection_result.camera_y == active_case->expected_camera[1] &&
          projection_result.camera_z == active_case->expected_camera[2],
          "B740 camera lanes differ from original native state");
    CHECK(projection_result.projected_x == active_case->expected_screen[0] &&
          projection_result.projected_y == active_case->expected_screen[1],
          "B920 screen lanes differ from original native state");
    projection_ready = 1;
}

/* HERC B840 copies camera lanes back to the shared translation cells. */
static void __cdecl slot103_camera_snapshot(void)
{
    log_callback(0xB840);
    require_supported(projection_ready, "B840 without immediately preceding B920");
    projection_config.translation_xyz[0] = projection_result.camera_x;
    projection_config.translation_xyz[1] = projection_result.camera_y;
    projection_config.translation_xyz[2] = projection_result.camera_z;
}

/* HERC B720: copy two screen WORDs to the producer's scratch vector. */
static void __cdecl slot114_screen_output(void *screen_words)
{
    short *out = (short *)screen_words;
    log_callback(0xB720);
    require_supported(projection_ready, "B720 without B920 result");
    require_supported(screen_words == matrix_bytes + 0x40,
                      "B720 scratch output address");
    out[0] = (short)projection_result.projected_x;
    out[1] = (short)projection_result.projected_y;
}

/* HERC B810: three WORD stores overlap visit+0x10 (the incoming depth). */
static void __cdecl slot117_camera_to_visit(void *visit_words)
{
    short *out = (short *)visit_words;
    log_callback(0xB810);
    require_supported(projection_ready, "B810 without B920 result");
    require_supported(visit_words == (void *)((unsigned char *)active_visit + 0x0c),
                      "B810 visit destination");
    out[0] = (short)projection_result.camera_x;
    out[1] = (short)projection_result.camera_y;
    out[2] = (short)projection_result.camera_z;
    projection_ready = 0;
}

static void *read_exact(const char *path, unsigned long count)
{
    FILE *file = fopen(path, "rb");
    void *bytes;
    unsigned long got;
    if (!file) return NULL;
    bytes = malloc(count);
    if (!bytes) { fclose(file); return NULL; }
    got = (unsigned long)fread(bytes, 1, count, file);
    fclose(file);
    if (got != count) { free(bytes); return NULL; }
    return bytes;
}

static void bind_callback_cells(void)
{
    g_engine_interface.opaque_174[8] = (unsigned long)slot101_point_input;
    g_engine_interface.opaque_174[10] = (unsigned long)slot103_camera_snapshot;
    g_engine_interface.opaque_174[13] = (unsigned long)slot106_project;
    g_engine_interface.opaque_174[15] = (unsigned long)slot108_load_matrix;
    g_engine_interface.opaque_174[16] = (unsigned long)slot109_load_translation;
    g_engine_interface.opaque_174[20] = (unsigned long)slot117_camera_to_visit;
    g_engine_interface.opaque_174[21] = (unsigned long)slot114_screen_output;
    g_engine_interface.opaque_284[3] = (unsigned long)slot164_angles_matrix;
    g_engine_interface.opaque_284[4] = (unsigned long)slot165_scale_matrix;
}

static void rebase_visit_pointers(unsigned char *raw,
                                  const NativePrepCase *case_data)
{
    unsigned long value;
    value = (unsigned long)(map_bytes + case_data->header_000_map_offset);
    memcpy(raw + 0x00, &value, 4);
    value = (unsigned long)(map_bytes + case_data->header_004_map_offset);
    memcpy(raw + 0x04, &value, 4);
    value = (unsigned long)(map_bytes + case_data->child_map_offset);
    memcpy(raw + 0x08, &value, 4);
}

static int same_callback_order(const NativePrepCase *case_data)
{
    unsigned int i;
    if (callback_count != case_data->expected_callback_count) return 0;
    for (i = 0; i != callback_count; ++i)
        if (callback_ids[i] != case_data->expected_callback_ids[i]) return 0;
    return 1;
}

static void initialize_case(const NativePrepCase *case_data,
                            const unsigned char *transport)
{
    int i;
    active_case = case_data;
    callback_count = 0;
    projection_ready = 0;
    memset(matrix_bytes, 0, sizeof matrix_bytes);
    memcpy(matrix_bytes, case_data->entry_matrix_bytes,
           sizeof case_data->entry_matrix_bytes);
    memcpy(matrix_bytes + 0x40, case_data->entry_scratch,
           sizeof case_data->entry_scratch);
    memcpy(&g_engine_interface, transport, sizeof g_engine_interface);
    g_engine_interface.opaque_010[0] = (unsigned long)matrix_bytes;
    g_engine_interface.opaque_010[1] = (unsigned long)trig_table;
    bind_callback_cells();
    g_camera_angle_x_70cd0 = case_data->camera_angle_x;
    for (i = 0; i != 3; ++i) g_mesh_scale_5ceb0[i] = case_data->scale_longs[i];
    memset(&projection_config, 0, sizeof projection_config);
    for (i = 0; i != 9; ++i)
        projection_config.matrix_row_major[i] =
            (renderer_i16)case_data->entry_matrix_words[i];
    memcpy(projection_config.translation_xyz, case_data->entry_translation,
           sizeof projection_config.translation_xyz);
    projection_config.center_x = NATIVE_CAMERA_CENTER_X;
    projection_config.center_y = NATIVE_CAMERA_CENTER_Y;
    projection_config.focal = NATIVE_CAMERA_FOCAL;
}

static void run_case(const NativePrepCase *case_data,
                     const unsigned char *transport)
{
    unsigned char expected_visit[0x74];
    unsigned char expected_matrix[0x40];
    unsigned char expected_scratch[0x60];
    int result;
    unsigned int i;
    Eng1MeshVisit visit;
    initialize_case(case_data, transport);
    memcpy(&visit, case_data->entry_visit, sizeof visit);
    rebase_visit_pointers((unsigned char *)&visit, case_data);
    active_visit = &visit;
    memcpy(expected_visit, case_data->return_visit, sizeof expected_visit);
    rebase_visit_pointers(expected_visit, case_data);
    memcpy(expected_matrix, case_data->return_matrix_bytes, sizeof expected_matrix);
    memcpy(expected_scratch, case_data->return_scratch, sizeof expected_scratch);

    CHECK(visit.child_008 == map_bytes + 0x26d30,
          "original resource child pointer was not rebased to archived map bytes");
    CHECK(visit.cell_02a == 0 && visit.variant_025 == 0,
          "fixture no longer identifies the measured ordinary mesh path");
    result = dispatch_local_2e3b0(&visit);
    CHECK(result == case_data->return_value,
          "mesh_prepare source return differs from actual 2E3B0 result");
    CHECK(visit.depth_010 == case_data->return_depth,
          "signed WORD depth after B810 alias/SAR2 differs from native return");
    CHECK(memcmp(&visit, expected_visit, sizeof visit) == 0,
          "mesh_prepare visit writes differ from original native return bytes");
    CHECK(memcmp(matrix_bytes, expected_matrix, sizeof expected_matrix) == 0,
          "mesh_prepare local matrix/translation writes differ from original");
    CHECK(memcmp(matrix_bytes + 0x40, expected_scratch,
                 sizeof expected_scratch) == 0,
          "mesh_prepare scratch writes differ from original");
    CHECK(same_callback_order(case_data),
          "source callback order differs from captured original instruction path");
    CHECK(projection_config.matrix_row_major[0] == case_data->expected_matrix_words[0] &&
          projection_config.matrix_row_major[4] == case_data->expected_matrix_words[4] &&
          projection_config.matrix_row_major[8] == case_data->expected_matrix_words[8],
          "post-prep matrix config differs from native B880/B740 state");
    for (i = 0; i != 3; ++i)
        CHECK(projection_config.translation_xyz[i] ==
              case_data->expected_translation[i],
              "post-B840 translation state differs from native return");
    printf("case=%d mesh=%d return=%d depth=%d callbacks=%u\n",
           case_data->mesh_call_index, case_data->mesh_index, result,
           visit.depth_010, callback_count);
}

int main(int argc, char **argv)
{
    unsigned char *transport;
    unsigned char *map;
    unsigned char *trig;
    int i;
    if (argc != 4) {
        printf("usage: prep_source_compare map.bin transport.bin trig.bin\n");
        return 2;
    }
    map = (unsigned char *)read_exact(argv[1], 1304576UL);
    transport = (unsigned char *)read_exact(argv[2], 1000UL);
    trig = (unsigned char *)read_exact(argv[3], 8192UL);
    if (!map || !transport || !trig) {
        printf("FAIL required map/transport/trig input was not exact length\n");
        return 2;
    }
    map_bytes = map;
    memcpy(trig_table, trig, sizeof trig_table);
    for (i = 0; i != NATIVE_PREP_CASE_COUNT; ++i)
        run_case(&g_native_prep_cases[i], transport);
    printf("prep_cases=%d checks=%d failures=%d\n",
           NATIVE_PREP_CASE_COUNT, check_count, failure_count);
    free(trig);
    free(transport);
    free(map);
    return failure_count ? 1 : 0;
}
