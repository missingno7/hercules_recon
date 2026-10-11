#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "candidate_rvas.h"

#define NATIVE_TITLE_17F50 0x17f50UL
#define NATIVE_TITLE_181E0 0x181e0UL
#define NATIVE_TITLE_186E0 0x186e0UL
#define NATIVE_TITLE_184C0 0x184c0UL
#define NATIVE_TITLE_18BF0 0x18bf0UL
#define NATIVE_G_260C0 0x260c0UL
#define NATIVE_G_264F8 0x264f8UL
#define NATIVE_G_260F0 0x260f0UL
#define NATIVE_G_2CC04 0x2cc04UL
#define NATIVE_G_2DF40 0x2df40UL
#define NATIVE_G_2DFA0 0x2dfa0UL
#define NATIVE_G_2D340 0x2d340UL
#define NATIVE_G_2CBEC 0x2cbecUL
#define NATIVE_G_2CBF4 0x2cbf4UL
#define NATIVE_G_2CBE0 0x2cbe0UL
#define NATIVE_INTERFACE 0x2bb40UL

#define OT_BYTES 0x1478UL
#define PRIM_BYTES 80UL
#define CELL_BYTES 3072UL
#define OBJ_BYTES 0x134UL

typedef struct TestBuffer {
    unsigned char env[0x70];
    unsigned long ot[0x500];
    unsigned char tail[8];
} TestBuffer;

typedef struct TestObject { unsigned char bytes[OBJ_BYTES]; } TestObject;
typedef void (__cdecl *EmitFn)(unsigned char *, TestObject *, unsigned char);
typedef int (__cdecl *MeshFn)(TestObject *, unsigned char *, unsigned long);
typedef void (__cdecl *MeshHandler)(TestObject *, unsigned char *, int);
typedef int (__cdecl *ClipFn)(unsigned char *, unsigned char *, int);

static unsigned long rng = 0x6c8e9cf5UL;
static unsigned long next_word(void)
{
    rng = rng * 1664525UL + 1013904223UL;
    return rng;
}

static void fill_random(void *p, unsigned long n)
{
    unsigned char *b = (unsigned char *)p;
    unsigned long i;
    for (i = 0; i < n; ++i) b[i] = (unsigned char)(next_word() >> 24);
}

static unsigned short *word_at(TestObject *o, unsigned long off)
{
    return (unsigned short *)(o->bytes + off);
}

static unsigned long *dword_at(TestObject *o, unsigned long off)
{
    return (unsigned long *)(o->bytes + off);
}

static int __cdecl host_stub(void)
{
    return 1;
}

static int __cdecl host_clip_zero(int *clip)
{
    *clip = 0;
    return 0;
}

static void __cdecl mesh_handler(TestObject *object, unsigned char *stream, int count)
{
    (void)object;
    (void)stream;
    (void)count;
}

static void install_host_stubs(HMODULE module, unsigned long interface_rva)
{
    unsigned char *interface_data = (unsigned char *)module + interface_rva;
    unsigned long slots[] = { 0x194, 0x198, 0x1a8, 0x1ac, 0x1b0, 0x1b8,
                              0x1bc, 0x1c8, 0x1d4 };
    unsigned long i;
    for (i = 0; i < sizeof(slots) / sizeof(slots[0]); ++i)
        *(void **)(interface_data + slots[i]) = (void *)host_stub;
}

static void setup_module(HMODULE module, unsigned long interface_rva,
                         unsigned long cc04_rva, unsigned long df40_rva,
                         unsigned long dfa0_rva, unsigned long cells_rva,
                         TestBuffer *buffer, TestObject *scratch,
                         unsigned char cell_data[CELL_BYTES])
{
    unsigned char *base = (unsigned char *)module;
    *(TestBuffer **)(base + cc04_rva) = buffer;
    *(void **)(base + df40_rva) = 0;
    *(TestObject **)(base + dfa0_rva) = scratch;
    memcpy(base + cells_rva, cell_data, CELL_BYTES);
    install_host_stubs(module, interface_rva);
}

static void setup_mesh_module(HMODULE module, unsigned long interface_rva,
                              unsigned long handlers_rva, unsigned long cbec_rva,
                              unsigned long cbf4_rva, unsigned long cells_rva,
                              unsigned long df40_rva, unsigned long dfa0_rva,
                              unsigned long cc04_rva,
                              TestBuffer *buffer, TestObject *scratch,
                              unsigned char *pair_table, unsigned char *uv_table,
                              unsigned char *storage, unsigned char *context,
                              unsigned char cell_data[CELL_BYTES])
{
    unsigned char *base = (unsigned char *)module;
    unsigned char *interface_data = base + interface_rva;
    unsigned long slots[] = { 0x190, 0x1a0, 0x1b8, 0x1c0, 0x1d8 };
    unsigned long i;
    *(TestBuffer **)(base + cc04_rva) = buffer;
    *(void **)(base + df40_rva) = 0;
    *(TestObject **)(base + dfa0_rva) = scratch;
    *(unsigned int **)(base + cbec_rva) = (unsigned int *)pair_table;
    *(unsigned char **)(base + cbf4_rva) = uv_table;
    memcpy(base + cells_rva, cell_data, CELL_BYTES);
    for (i = 0; i < 8; ++i)
        ((MeshHandler *)(base + handlers_rva))[i] = mesh_handler;
    install_host_stubs(module, interface_rva);
    for (i = 0; i < sizeof(slots) / sizeof(slots[0]); ++i)
        *(void **)(interface_data + slots[i]) = (void *)host_stub;
    *(void **)(interface_data + 0x1c0) = (void *)host_clip_zero;
    *(void **)(interface_data + 0x004) = context;
    *(void **)(interface_data + 0x010) = storage;
}

static void reset_shared(TestBuffer *buffer, TestObject *object,
                         TestObject *scratch, unsigned char *stream,
                         unsigned char *pool,
                         const unsigned char *buffer_in,
                         const unsigned char *object_in,
                         const unsigned char *scratch_in,
                         const unsigned char *stream_in,
                         const unsigned char *pool_in,
                         unsigned long pool_bytes)
{
    memcpy(buffer, buffer_in, OT_BYTES);
    memcpy(object, object_in, OBJ_BYTES);
    memcpy(scratch, scratch_in, OBJ_BYTES);
    memcpy(stream, stream_in, 32);
    memcpy(pool, pool_in, pool_bytes);
}

static int run_case(const char *name, EmitFn native_fn, EmitFn candidate_fn,
                    HMODULE native_module, HMODULE candidate_module,
                    unsigned long native_df40_rva, unsigned long candidate_df40_rva,
                    TestBuffer *buffer, TestObject *object, TestObject *scratch,
                    unsigned char stream[32], unsigned char *pool,
                    unsigned long count, unsigned long test)
{
    unsigned char buffer_in[OT_BYTES];
    unsigned char object_in[OBJ_BYTES];
    unsigned char scratch_in[OBJ_BYTES];
    unsigned char stream_in[32];
    unsigned char pool_in[PRIM_BYTES];
    unsigned char expected_buffer[OT_BYTES];
    unsigned char expected_object[OBJ_BYTES];
    unsigned char expected_scratch[OBJ_BYTES];
    unsigned char expected_pool[PRIM_BYTES];
    unsigned long pool_bytes = count * 40UL;
    unsigned long depth;
    unsigned long *native_cursor;
    unsigned long *candidate_cursor;

    fill_random(buffer_in, sizeof(buffer_in));
    fill_random(object_in, sizeof(object_in));
    fill_random(scratch_in, sizeof(scratch_in));
    fill_random(stream_in, sizeof(stream_in));
    fill_random(pool_in, sizeof(pool_in));

    depth = next_word() & 31;
    *word_at(object, 0x10) = (unsigned short)depth;
    *word_at((TestObject *)object_in, 0x10) = (unsigned short)depth;
    *word_at(object, 0x46) = (unsigned short)(next_word() & 0xff);
    *word_at((TestObject *)object_in, 0x46) = *word_at(object, 0x46);
    *word_at(object, 0x1e) = (unsigned short)(next_word() & 0xff);
    *word_at((TestObject *)object_in, 0x1e) = *word_at(object, 0x1e);
    *word_at(object, 0x4a) = (unsigned short)next_word();
    *word_at((TestObject *)object_in, 0x4a) = *word_at(object, 0x4a);
    *dword_at(object, 0x54) = (next_word() & 3) | ((next_word() & 1) ? 0x10 : 0) |
                              ((next_word() & 1) ? 0x2000 : 0) |
                              ((next_word() & 1) ? 0x40000 : 0);
    *dword_at((TestObject *)object_in, 0x54) = *dword_at(object, 0x54);
    *word_at(object, 0x3e) = (unsigned short)(next_word() & 31);
    *word_at((TestObject *)object_in, 0x3e) = *word_at(object, 0x3e);
    *word_at(object, 0x28) = (unsigned short)next_word();
    *word_at((TestObject *)object_in, 0x28) = *word_at(object, 0x28);
    *word_at(object, 0x0c) = (unsigned short)next_word();
    *word_at((TestObject *)object_in, 0x0c) = *word_at(object, 0x0c);
    *word_at(object, 0x0e) = (unsigned short)next_word();
    *word_at((TestObject *)object_in, 0x0e) = *word_at(object, 0x0e);

    /* The streams are bounded to one or two quads; width/offset bytes stay arbitrary. */
    stream_in[2] = (unsigned char)((next_word() & 0x7f) | ((next_word() & 1) ? 0x80 : 0));
    stream_in[6] = (unsigned char)((next_word() & 0x7f) | ((next_word() & 1) ? 0x80 : 0));
    stream_in[10] = 0;
    memcpy(buffer, buffer_in, sizeof(buffer_in));
    memcpy(object, object_in, sizeof(object_in));
    memcpy(scratch, scratch_in, sizeof(scratch_in));
    memcpy(stream, stream_in, sizeof(stream_in));
    memcpy(pool, pool_in, sizeof(pool_in));

    *(void **)((unsigned char *)native_module + native_df40_rva) = pool;
    native_fn(stream, object, (unsigned char)count);
    native_cursor = *(unsigned long **)((unsigned char *)native_module + native_df40_rva);
    memcpy(expected_buffer, buffer, sizeof(expected_buffer));
    memcpy(expected_object, object, sizeof(expected_object));
    memcpy(expected_scratch, scratch, sizeof(expected_scratch));
    memcpy(expected_pool, pool, pool_bytes);

    reset_shared(buffer, object, scratch, stream, pool,
                 buffer_in, object_in, scratch_in, stream_in, pool_in, pool_bytes);
    *(void **)((unsigned char *)candidate_module + candidate_df40_rva) = pool;
    candidate_fn(stream, object, (unsigned char)count);
    candidate_cursor = *(unsigned long **)((unsigned char *)candidate_module + candidate_df40_rva);

    if (native_cursor != candidate_cursor || native_cursor != (unsigned long *)(pool + pool_bytes) ||
        memcmp(expected_buffer, buffer, sizeof(expected_buffer)) != 0 ||
        memcmp(expected_object, object, sizeof(expected_object)) != 0 ||
        memcmp(expected_scratch, scratch, sizeof(expected_scratch)) != 0 ||
        memcmp(expected_pool, pool, pool_bytes) != 0) {
        unsigned long i;
        printf("DIVERGENCE %s case=%lu cursor_native=%p cursor_candidate=%p\n",
               name, test, (void *)native_cursor, (void *)candidate_cursor);
        for (i = 0; i < pool_bytes; ++i)
            if (expected_pool[i] != pool[i]) {
                printf("pool byte=%lu native=%02x candidate=%02x\n", i,
                       (unsigned int)expected_pool[i], (unsigned int)pool[i]);
                break;
            }
        for (i = 0; i < sizeof(expected_buffer); ++i)
            if (expected_buffer[i] != ((unsigned char *)buffer)[i]) {
                printf("buffer byte=%lu native=%02x candidate=%02x\n", i,
                       (unsigned int)expected_buffer[i], (unsigned int)((unsigned char *)buffer)[i]);
                break;
            }
        for (i = 0; i < OBJ_BYTES; ++i)
            if (expected_object[i] != object->bytes[i]) {
                printf("object byte=%lu native=%02x candidate=%02x\n", i,
                       (unsigned int)expected_object[i], (unsigned int)object->bytes[i]);
                break;
            }
        for (i = 0; i < OBJ_BYTES; ++i)
            if (expected_scratch[i] != scratch->bytes[i]) {
                printf("scratch byte=%lu native=%02x candidate=%02x\n", i,
                       (unsigned int)expected_scratch[i], (unsigned int)scratch->bytes[i]);
                break;
            }
        return 0;
    }
    return 1;
}

static int run_mesh_case(MeshFn native_fn, MeshFn candidate_fn,
                        HMODULE native_module, HMODULE candidate_module,
                        unsigned long native_df40_rva, unsigned long candidate_df40_rva,
                        TestBuffer *buffer, TestObject *object, TestObject *scratch,
                        unsigned char q[64], unsigned char *pool,
                        unsigned int *pair_table, unsigned char *uv_table,
                        unsigned char *storage, unsigned char *context,
                        unsigned long test)
{
    unsigned char buffer_in[OT_BYTES];
    unsigned char object_in[OBJ_BYTES];
    unsigned char scratch_in[OBJ_BYTES];
    unsigned char q_in[64];
    unsigned char pool_in[PRIM_BYTES];
    unsigned char storage_in[0x218];
    unsigned int pair_in[16];
    unsigned char uv_in[32];
    unsigned char expected_buffer[OT_BYTES];
    unsigned char expected_object[OBJ_BYTES];
    unsigned char expected_pool[40];
    unsigned char expected_storage[0x218];
    unsigned long *native_cursor;
    unsigned long *candidate_cursor;
    unsigned long i;
    int native_result;
    int candidate_result;

    fill_random(buffer_in, sizeof(buffer_in));
    fill_random(object_in, sizeof(object_in));
    fill_random(scratch_in, sizeof(scratch_in));
    fill_random(q_in, sizeof(q_in));
    fill_random(pool_in, sizeof(pool_in));
    fill_random(storage_in, sizeof(storage_in));
    fill_random(pair_in, sizeof(pair_in));
    fill_random(uv_in, sizeof(uv_in));
    *(unsigned short *)(object_in + 0x10) = (unsigned short)(1 + (next_word() % 0x400));
    object_in[0x23] = 0;
    *(unsigned long *)(object_in + 0x54) = (next_word() & 1) ? 0x40 : 0;
    q_in[0] = q_in[1] = 0;                 /* pair-table offset */
    q_in[2] = q_in[3] = 0;                 /* skip count */
    *(unsigned long *)(q_in + 4) = 0;       /* no transform helper */
    *(unsigned short *)(q_in + 10) = 0;     /* empty encoded part stream */
    pair_in[0] = 1;                         /* one pair, no continuation */
    pair_in[1] = next_word() & 0x00ffffff; /* four indices, no mode flags */
    pair_in[2] = 0;                         /* UV table byte offset */
    *(unsigned int *)(uv_in + 0) = 0x00000123;
    *(unsigned int *)(uv_in + 4) = 0x00004567;
    *(unsigned int *)(uv_in + 8) = 0x000089ab;
    for (i = 0; i < 64; ++i) {
        unsigned char *vertex = storage_in + 0x18 + i * 8;
        *(unsigned long *)(vertex + 0) = next_word();
        *(unsigned long *)(vertex + 4) = next_word() & 0xff;
    }

    memcpy(buffer, buffer_in, sizeof(buffer_in));
    memcpy(object->bytes, object_in, sizeof(object_in));
    memcpy(scratch->bytes, scratch_in, sizeof(scratch_in));
    memcpy(q, q_in, sizeof(q_in));
    memcpy(pool, pool_in, sizeof(pool_in));
    memcpy(storage, storage_in, sizeof(storage_in));
    memcpy(pair_table, pair_in, sizeof(pair_in));
    memcpy(uv_table, uv_in, sizeof(uv_in));
    *(void **)((unsigned char *)native_module + native_df40_rva) = pool;
    native_result = native_fn(object, q, 0);
    native_cursor = *(unsigned long **)((unsigned char *)native_module + native_df40_rva);
    memcpy(expected_buffer, buffer, sizeof(expected_buffer));
    memcpy(expected_object, object->bytes, sizeof(expected_object));
    memcpy(expected_pool, pool, sizeof(expected_pool));
    memcpy(expected_storage, storage, sizeof(expected_storage));

    memcpy(buffer, buffer_in, sizeof(buffer_in));
    memcpy(object->bytes, object_in, sizeof(object_in));
    memcpy(scratch->bytes, scratch_in, sizeof(scratch_in));
    memcpy(q, q_in, sizeof(q_in));
    memcpy(pool, pool_in, sizeof(pool_in));
    memcpy(storage, storage_in, sizeof(storage_in));
    memcpy(pair_table, pair_in, sizeof(pair_in));
    memcpy(uv_table, uv_in, sizeof(uv_in));
    *(void **)((unsigned char *)candidate_module + candidate_df40_rva) = pool;
    candidate_result = candidate_fn(object, q, 0);
    candidate_cursor = *(unsigned long **)((unsigned char *)candidate_module + candidate_df40_rva);

    if (native_result != candidate_result || native_cursor != candidate_cursor ||
        native_cursor != (unsigned long *)(pool + 40) ||
        memcmp(expected_buffer, buffer, sizeof(expected_buffer)) != 0 ||
        memcmp(expected_object, object->bytes, sizeof(expected_object)) != 0 ||
        memcmp(expected_pool, pool, sizeof(expected_pool)) != 0 ||
        memcmp(expected_storage, storage, sizeof(expected_storage)) != 0) {
        printf("DIVERGENCE title_18bf0 case=%lu result_native=%d result_candidate=%d cursor_native=%p cursor_candidate=%p\n",
               test, native_result, candidate_result, (void *)native_cursor, (void *)candidate_cursor);
        for (i = 0; i < sizeof(expected_pool); ++i)
            if (expected_pool[i] != pool[i]) {
                printf("pool byte=%lu native=%02x candidate=%02x\n", i,
                       (unsigned int)expected_pool[i], (unsigned int)pool[i]); break;
            }
        for (i = 0; i < sizeof(expected_buffer); ++i)
            if (expected_buffer[i] != ((unsigned char *)buffer)[i]) {
                printf("buffer byte=%lu native=%02x candidate=%02x\n", i,
                       (unsigned int)expected_buffer[i], (unsigned int)((unsigned char *)buffer)[i]); break;
            }
        return 0;
    }
    return 1;
}

static int run_clip_case(ClipFn native_fn, ClipFn candidate_fn,
                         unsigned char object[0x58], unsigned char box[6],
                         unsigned long test)
{
    unsigned char object_in[0x58];
    unsigned char box_in[6];
    int expected;
    int actual;
    fill_random(object_in, sizeof(object_in));
    fill_random(box_in, sizeof(box_in));
    *(short *)(object_in + 0x0c) = (short)(short)next_word();
    *(short *)(object_in + 0x0e) = (short)(short)next_word();
    *(short *)(object_in + 0x10) = (short)(1 + (next_word() & 0xff));
    *(short *)(object_in + 0x28) = (short)((int)(next_word() & 0x1fff) - 0x1000);
    *(unsigned short *)(object_in + 0x3a) = (unsigned short)(next_word() & 0xff);
    *(unsigned long *)(object_in + 0x54) = (next_word() & 1) ? 0x10 : 0;
    memcpy(object, object_in, sizeof(object_in));
    memcpy(box, box_in, sizeof(box_in));
    expected = native_fn(object, box, (int)(test & 1));
    memcpy(object, object_in, sizeof(object_in));
    memcpy(box, box_in, sizeof(box_in));
    actual = candidate_fn(object, box, (int)(test & 1));
    if (expected != actual || memcmp(object, object_in, sizeof(object_in)) != 0 ||
        memcmp(box, box_in, sizeof(box_in)) != 0) {
        printf("DIVERGENCE title_184c0 case=%lu mode=%lu native=%d candidate=%d\n",
               test, test & 1, expected, actual);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    HMODULE original;
    HMODULE candidate;
    TestBuffer buffer;
    TestObject object;
    TestObject scratch;
    unsigned char stream[32];
    unsigned char pool[PRIM_BYTES];
    unsigned char cells[CELL_BYTES];
    unsigned int mesh_pairs[16];
    unsigned char mesh_uv[32];
    unsigned char data_store[0x218];
    unsigned char context[0x100];
    EmitFn native_17f50, native_181e0, native_186e0;
    EmitFn candidate_17f50, candidate_181e0, candidate_186e0;
    MeshFn native_18bf0, candidate_18bf0;
    ClipFn native_184c0, candidate_184c0;
    unsigned char clip_object[0x58];
    unsigned char clip_box[6];
    int center[3] = { 0xa0, 0x78, 0x100 };
    unsigned long i, test, pass;

    if (argc != 3) {
        printf("usage: native_emit_diff.exe <original-title.dll> <candidate-title.dll>\n");
        return 2;
    }
    if (sizeof(TestBuffer) != OT_BYTES || sizeof(TestObject) != OBJ_BYTES) {
        printf("HARNESS ERROR: local fixture layout mismatch\n");
        return 3;
    }
    for (i = 0; i < CELL_BYTES; ++i) cells[i] = (unsigned char)(i * 73 + 19);
    original = LoadLibraryA(argv[1]);
    candidate = LoadLibraryA(argv[2]);
    if (!original || !candidate || original == candidate) {
        printf("HARNESS ERROR: module load original=%p candidate=%p error=%lu\n",
               original, candidate, (unsigned long)GetLastError());
        return 4;
    }
    setup_module(original, NATIVE_INTERFACE, NATIVE_G_2CC04, NATIVE_G_2DF40,
                 NATIVE_G_2DFA0, NATIVE_G_2D340, &buffer, &scratch, cells);
    setup_module(candidate, CANDIDATE_INTERFACE_RVA, CANDIDATE_G_2CC04_RVA,
                 CANDIDATE_G_2DF40_RVA, CANDIDATE_G_2DFA0_RVA,
                 CANDIDATE_G_2D340_RVA, &buffer, &scratch, cells);
    if (memcmp((unsigned char *)original + NATIVE_G_260C0,
               (unsigned char *)candidate + CANDIDATE_G_260C0_RVA, 32) != 0 ||
        memcmp((unsigned char *)original + NATIVE_G_264F8,
               (unsigned char *)candidate + CANDIDATE_G_264F8_RVA, 16) != 0) {
        printf("HARNESS ERROR: input table bytes differ between module images\n");
        return 5;
    }
    memcpy((unsigned char *)original + NATIVE_G_2D340, cells, CELL_BYTES);
    memcpy((unsigned char *)candidate + CANDIDATE_G_2D340_RVA, cells, CELL_BYTES);

    native_17f50 = (EmitFn)((unsigned char *)original + NATIVE_TITLE_17F50);
    native_181e0 = (EmitFn)((unsigned char *)original + NATIVE_TITLE_181E0);
    native_186e0 = (EmitFn)((unsigned char *)original + NATIVE_TITLE_186E0);
    candidate_17f50 = (EmitFn)((unsigned char *)candidate + CANDIDATE_TITLE_17F50_RVA);
    candidate_181e0 = (EmitFn)((unsigned char *)candidate + CANDIDATE_TITLE_181E0_RVA);
    candidate_186e0 = (EmitFn)((unsigned char *)candidate + CANDIDATE_TITLE_186E0_RVA);

    pass = 0;
    for (test = 1; test <= 512; ++test) {
        unsigned long count = (test & 1) ? 1 : 2;
        fill_random(&object, sizeof(object));
        if (!run_case("title_17f50", native_17f50, candidate_17f50,
                      original, candidate, NATIVE_G_2DF40, CANDIDATE_G_2DF40_RVA,
                      &buffer, &object, &scratch, stream, pool, count, test)) return 1;
        ++pass;
    }
    printf("PASS title_17f50 cases=%lu outputs=FT4 pool+OT+object+scratch; helper slots stubbed\n", pass);

    pass = 0;
    for (test = 1; test <= 512; ++test) {
        unsigned long count = (test & 1) ? 1 : 2;
        fill_random(&object, sizeof(object));
        if (!run_case("title_181e0", native_181e0, candidate_181e0,
                      original, candidate, NATIVE_G_2DF40, CANDIDATE_G_2DF40_RVA,
                      &buffer, &object, &scratch, stream, pool, count, test)) return 1;
        ++pass;
    }
    printf("PASS title_181e0 cases=%lu outputs=FT4 pool+OT+object+scratch; helper slots stubbed\n", pass);

    pass = 0;
    for (test = 1; test <= 512; ++test) {
        unsigned long count = (test & 1) ? 1 : 2;
        fill_random(&object, sizeof(object));
        if (!run_case("title_186e0", native_186e0, candidate_186e0,
                      original, candidate, NATIVE_G_2DF40, CANDIDATE_G_2DF40_RVA,
                      &buffer, &object, &scratch, stream, pool, count, test)) return 1;
        ++pass;
    }
    printf("PASS title_186e0 cases=%lu outputs=FT4 pool+OT+object+scratch; no host helper calls\n", pass);

    memcpy((unsigned char *)original + NATIVE_G_2CBE0, center, sizeof(center));
    memcpy((unsigned char *)candidate + CANDIDATE_G_2CBE0_RVA, center, sizeof(center));
    native_184c0 = (ClipFn)((unsigned char *)original + NATIVE_TITLE_184C0);
    candidate_184c0 = (ClipFn)((unsigned char *)candidate + CANDIDATE_TITLE_184C0_RVA);
    pass = 0;
    for (test = 1; test <= 512; ++test) {
        if (!run_clip_case(native_184c0, candidate_184c0, clip_object, clip_box, test)) return 1;
        ++pass;
    }
    printf("PASS title_184c0 cases=%lu screen center=(160,120,256); clip return values and inputs match\n", pass);

    fill_random(data_store, sizeof(data_store));
    memset(context, 0, sizeof(context));
    setup_mesh_module(original, NATIVE_INTERFACE, NATIVE_G_260F0, NATIVE_G_2CBEC,
                      NATIVE_G_2CBF4, NATIVE_G_2D340, NATIVE_G_2DF40,
                      NATIVE_G_2DFA0, NATIVE_G_2CC04, &buffer, &scratch,
                      (unsigned char *)mesh_pairs,
                      mesh_uv, data_store, context, cells);
    setup_mesh_module(candidate, CANDIDATE_INTERFACE_RVA, CANDIDATE_G_260F0_RVA,
                      CANDIDATE_G_2CBEC_RVA, CANDIDATE_G_2CBF4_RVA,
                      CANDIDATE_G_2D340_RVA, CANDIDATE_G_2DF40_RVA,
                      CANDIDATE_G_2DFA0_RVA, CANDIDATE_G_2CC04_RVA,
                      &buffer, &scratch,
                      (unsigned char *)mesh_pairs, mesh_uv, data_store, context, cells);
    native_18bf0 = (MeshFn)((unsigned char *)original + NATIVE_TITLE_18BF0);
    candidate_18bf0 = (MeshFn)((unsigned char *)candidate + CANDIDATE_TITLE_18BF0_RVA);
    pass = 0;
    for (test = 1; test <= 512; ++test) {
        if (!run_mesh_case(native_18bf0, candidate_18bf0, original, candidate,
                           NATIVE_G_2DF40, CANDIDATE_G_2DF40_RVA,
                           &buffer, &object, &scratch, stream, pool, mesh_pairs,
                           mesh_uv, data_store, context, test)) return 1;
        ++pass;
    }
    printf("PASS title_18bf0 cases=%lu outputs=FT4 pool+OT+object+buffer cursor; mesh handler/helper slots controlled\n", pass);
    printf("PASS all five native-vs-source renderer fixtures cases=2560\n");
    FreeLibrary(candidate);
    FreeLibrary(original);
    return 0;
}
