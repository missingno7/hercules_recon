#include <stdio.h>
#include <stdarg.h>
#include "../calibration/host_pool_shrink.c"

const char g_fmt_remalloc[] = "ReMalloc Heap%d @ 0x%x,Size %d\n";
static U32 checks;
static U32 failures;
static PoolWord arena[48];
static PoolWord *old_header;
static PoolWord *new_header;
static void *owner_value;
static void **owner_slot;
static U32 expected_pool;
static U32 expected_new_words;
static U32 expected_flags;
static U32 expected_header_owner_word;
static U32 expected_payload_words[16];
static U32 expected_remainder;
static U32 expected_copy_bytes;
static U32 expected_payload_bytes;
static U32 expect_success;
static U32 expected_owner_only;
U32 copy_calls;
static U32 coalesce_calls;
static U32 diagnostic_calls;
U32 call_order;
U32 copy_order_error;
void *copy_destination;
const void *copy_source;
U32 copy_length;
static U32 seen_pool;
static void *seen_payload;
static U32 seen_bytes;

static void check(int ok, const char *label)
{
    ++checks;
    if (!ok) {
        ++failures;
        if (failures <= 20) printf("FAIL %s\n", label);
    }
}

static void prepare(U32 total_words, U32 flags)
{
    U32 i;
    for (i = 0; i < 48; ++i) arena[i].descriptor = 0xa5a5a5a5UL;
    old_header = &arena[4];
    owner_slot = &owner_value;
    old_header[0].descriptor = flags | total_words;
    old_header[1].owner_slot = owner_slot;
    owner_value = &old_header[2];
    for (i = 2; i < total_words; ++i)
        old_header[i].descriptor = 0x19370000UL + i * 0x101UL;
    new_header = 0;
    expected_pool = expected_new_words = expected_remainder = 0;
    expected_copy_bytes = expected_payload_bytes = 0;
    expect_success = expected_owner_only = 0;
    copy_calls = coalesce_calls = diagnostic_calls = call_order = 0;
    copy_order_error = 0;
    copy_destination = 0; copy_source = 0; copy_length = 0;
    seen_pool = seen_bytes = 0; seen_payload = 0;
}

static void expect_shrink(U32 requested_bytes, U32 pool_id, U32 owner_only)
{
    U32 rounded = (requested_bytes + 3) & ~3UL;
    U32 old_words = old_header[0].descriptor & POOL_SIZE;
    U32 i;
    expected_new_words = (rounded >> 2) + 2;
    expected_remainder = old_words - expected_new_words;
    expected_pool = pool_id;
    expected_owner_only = owner_only;
    expected_flags = old_header[0].descriptor & POOL_PINNED;
    expected_header_owner_word = expected_remainder == 1 ?
        (expected_flags | expected_new_words) : (U32)owner_slot;
    for (i=0; i<expected_new_words-2; ++i)
        expected_payload_words[i] = old_header[2+i].descriptor;
    expected_copy_bytes = (expected_new_words << 2) - 4;
    expected_payload_bytes = (expected_new_words << 2) - 8;
    new_header = old_header + expected_remainder;
    expect_success = 1;
}

void __cdecl coalesce_pool(U32 pool_id)
{
    ++coalesce_calls;
    check(pool_id == expected_pool, "coalesce receives arg3 pool id");
    if (expect_success) {
        check(old_header[0].descriptor == (POOL_FREE | expected_remainder),
              "old low header is FREE with residual word count before coalesce");
        check(new_header[0].descriptor ==
              (expected_flags | expected_new_words),
              "new high header carries its word count");
        check(new_header[1].descriptor == expected_header_owner_word,
              "new header transfers the owner-slot pointer");
        check(owner_value == &new_header[2],
              "owner slot is updated before coalesce");
        check(call_order == (expected_owner_only ? 0UL : 1UL),
              "coalesce follows optional data copy");
    }
    call_order = 2;
}

void __cdecl host_diagnostic(const char *format, ...)
{
    va_list args;
    ++diagnostic_calls;
    check(format == g_fmt_remalloc, "ReMalloc diagnostic format");
    check(call_order == 2, "diagnostic follows coalesce");
    va_start(args, format);
    seen_pool = va_arg(args, U32);
    seen_payload = va_arg(args, void *);
    seen_bytes = va_arg(args, U32);
    va_end(args);
    check(seen_pool == expected_pool, "diagnostic pool id");
    if (expect_success) {
        check(seen_payload == &new_header[2], "diagnostic new payload");
        check(seen_bytes == expected_payload_bytes, "diagnostic rounded payload size");
    }
    call_order = 3;
}

static void test_null_equal_and_growth(void)
{
    PoolWord before[48];
    void *saved_owner;
    void *old_payload;
    U32 i;
    prepare(10, POOL_PINNED);
    old_payload = &old_header[2];
    for (i = 0; i < 48; ++i) before[i] = arena[i];
    saved_owner = owner_value;
    check(shrink_relocate(0, 16, 0, 0) == 0, "NULL input returns zero");
    check(copy_calls == 0 && coalesce_calls == 0 && diagnostic_calls == 0,
          "NULL input has no helper calls");
    check(shrink_relocate(old_payload, 31, 0, 0) == 0,
          "size rounding equal to current payload returns zero");
    check(shrink_relocate(old_payload, 33, 0, 0) == old_payload,
          "larger rounded size returns the old payload");
    check(copy_calls == 0 && coalesce_calls == 0 && diagnostic_calls == 0,
          "equal and larger sizes have no helper calls");
    check(owner_value == saved_owner, "no-op cases preserve owner slot");
    for (i = 0; i < 48; ++i)
        check(arena[i].descriptor == before[i].descriptor,
              "no-op cases preserve all arena words");
}

static void test_overlapping_copy_shrink(void)
{
    U32 i;
    void *old_payload;
    prepare(10, POOL_PINNED);
    old_payload = &old_header[2];
    expect_shrink(27, 2, 0);
    check(shrink_relocate(old_payload, 27, 2, 0) == old_payload,
          "successful shrink returns observed old payload value");
    check(copy_calls == 1 && copy_length == expected_copy_bytes,
          "copy mode moves owner plus retained payload prefix");
    check(copy_destination == &new_header[1] && copy_source == &old_header[1],
          "copy uses old/new owner-header word addresses");
    check(copy_order_error == 0, "overlap copy precedes coalesce");
    check(coalesce_calls == 1 && diagnostic_calls == 1 && call_order == 3,
          "shrink calls coalesce then diagnostic exactly once");
    for (i = 0; i < 7; ++i)
        check(new_header[2+i].descriptor == expected_payload_words[i],
              "rightward overlapping memmove preserves retained payload prefix");
    check((new_header[0].descriptor & POOL_PINNED) != 0,
          "new descriptor preserves pinned flag");
    check(old_header[0].descriptor == (POOL_FREE | 1),
          "one-word low residual becomes free");
    check(owner_value == &new_header[2], "owner pointer follows moved payload");
    check(seen_bytes == 28 && seen_pool == 2,
          "diagnostic reports rounded retained payload and pool id");
}

static void test_owner_only_shrink(void)
{
    PoolWord before[48];
    U32 i;
    void *old_payload;
    prepare(10, 0);
    old_payload = &old_header[2];
    for (i = 0; i < 48; ++i) before[i] = arena[i];
    expect_shrink(9, 0, 1);
    check(shrink_relocate(old_payload, 9, 0, 1) == old_payload,
          "owner-only shrink returns observed old payload value");
    check(copy_calls == 0, "owner-only mode does not copy payload bytes");
    check(coalesce_calls == 1 && diagnostic_calls == 1,
          "owner-only shrink still coalesces and diagnoses");
    for (i = 0; i < 48; ++i) {
        if (i == 4 || i == 9 || i == 10) continue;
        check(arena[i].descriptor == before[i].descriptor,
              "owner-only path changes only old/new headers and owner word");
    }
    check(owner_value == &new_header[2], "owner-only path updates owner slot");
    check(new_header[0].descriptor == 5, "owner-only new size is five total words");
    check(old_header[0].descriptor == (POOL_FREE | 5),
          "owner-only low remainder has five words");
}


static void test_owner_only_one_word_alias(void)
{
    PoolWord before[48];
    U32 i;
    void *old_payload;
    prepare(10, POOL_PINNED);
    old_payload = &old_header[2];
    for (i = 0; i < 48; ++i) before[i] = arena[i];
    expect_shrink(27, 0, 1);
    check(expected_remainder == 1, "owner-only alias case has one-word residual");
    check(shrink_relocate(old_payload, 27, 0, 1) == old_payload,
          "one-word owner-only shrink returns old payload value");
    check(copy_calls == 0, "one-word owner-only mode avoids memmove");
    check(new_header[0].descriptor == (POOL_PINNED | 9),
          "new descriptor is written over old owner word at one-word remainder");
    check(new_header[1].descriptor == (POOL_PINNED | 9),
          "owner-only copy reads the aliased word after descriptor store");
    check(owner_value == &new_header[2],
          "saved owner-slot pointee still follows relocated payload");
    check(coalesce_calls == 1 && diagnostic_calls == 1 && call_order == 3,
          "one-word alias still coalesces then diagnoses");
    for (i = 0; i < 48; ++i) {
        if (i == 4 || i == 5 || i == 6) continue;
        check(arena[i].descriptor == before[i].descriptor,
              "one-word owner-only path preserves all other arena words");
    }
}

static void test_zero_size_copy(void)
{
    void *old_payload;
    prepare(10, POOL_PINNED);
    old_payload = &old_header[2];
    expect_shrink(0, 3, 0);
    check(shrink_relocate(old_payload, 0, 3, 0) == old_payload,
          "zero-size shrink returns old payload value");
    check(expected_new_words == 2 && expected_remainder == 8,
          "zero size retains the two-word header only");
    check(copy_calls == 1 && copy_length == 4,
          "zero-size copy path still moves the owner dword");
    check(owner_value == &new_header[2], "zero-size owner points after new header");
    check(seen_bytes == 0 && seen_pool == 3,
          "zero-size diagnostic reports no payload bytes");
}

int main(void)
{
    test_null_equal_and_growth();
    test_overlapping_copy_shrink();
    test_owner_only_shrink();
    test_owner_only_one_word_alias();
    test_zero_size_copy();
    printf("checks=%lu failures=%lu\n", checks, failures);
    return failures != 0;
}

/* Test-only byte copy model; not a recovered CRT implementation. */
static unsigned char *as_bytes(void *p) { return (unsigned char *)p; }
static const unsigned char *as_const_bytes(const void *p) { return (const unsigned char *)p; }
extern U32 copy_calls;
extern U32 call_order;
extern U32 copy_order_error;
extern void *copy_destination;
extern const void *copy_source;
extern U32 copy_length;
void *__cdecl memmove(void *destination, const void *source, U32 bytes)
{
    U32 i;
    unsigned char *to=as_bytes(destination);
    const unsigned char *from=as_const_bytes(source);
    ++copy_calls;
    copy_order_error=(call_order!=0);
    copy_destination=destination;
    copy_source=source;
    copy_length=bytes;
    if (to>from) {
        for (i=bytes; i!=0; --i) to[i-1]=from[i-1];
    } else {
        for (i=0; i<bytes; ++i) to[i]=from[i];
    }
    call_order=1;
    return destination;
}
