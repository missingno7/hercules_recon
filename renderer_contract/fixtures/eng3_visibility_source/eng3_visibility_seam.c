/*
 * Bounded ENG3 visibility seam reconstructed from original x86 observations.
 *
 * BOUNDED_EQUIVALENT: the endpoint helper's +4/+8 writes and modulo-32-bit
 * addition, plus the coordinate/index/interval predicate, are compared with
 * controlled original 0x204C0/0x24A80 cases by the neighboring C harness and
 * native runner. This is not a complete reconstruction of either function:
 * selector production, live bounds initialization, and the positive transform
 * tail are outside this seam. Callers must supply a live table and keep the
 * indexed row in range; no allocator, resource loader, or default bounds are
 * invented here.
 */

typedef unsigned int U32;
typedef unsigned short U16;
typedef signed int S32;
typedef signed short S16;

typedef char assert_u32_is_4_bytes[(sizeof(U32) == 4) ? 1 : -1];
typedef char assert_u16_is_2_bytes[(sizeof(U16) == 2) ? 1 : -1];

typedef struct Eng3VisibilityRow {
    U32 selector_bits; /* +0: writer copies current object's DWORD at +0x38. */
    U32 lower_bits;    /* +4: writer stores caller arg4. */
    U32 upper_bits;    /* +8: writer stores (arg4 + arg5) modulo 2^32. */
} Eng3VisibilityRow;

typedef char assert_row_stride_is_12[(sizeof(Eng3VisibilityRow) == 12) ? 1 : -1];

/* Explicit two's-complement conversions avoid implementation-defined casts. */
static S32 eng3_signed16(U32 bits)
{
    bits &= 0xffffU;
    return (bits & 0x8000U) ? (S32)bits - 0x10000L : (S32)bits;
}

static S32 eng3_signed32(U32 bits)
{
    if ((bits & 0x80000000U) == 0)
        return (S32)bits;
    return -1L - (S32)(~bits);
}

/* Original 0x24A80: signed high WORD of object DWORD at +0, then SAR 7. */
S32 eng3_indexed_coordinate_from_object_x(U32 object_x_bits)
{
    S32 high_word = eng3_signed16(object_x_bits >> 16);
    if (high_word < 0)
        return -(((-high_word) + 127L) / 128L);
    return high_word / 128L;
}

/*
 * Original 0x24A80 interval core: index is a WORD zero-extended to 32 bits;
 * the LEAs form base + 12*index; the two record DWORDs are signed and the
 * accepted interval is lower <= coordinate < upper. selector_bits is not
 * read by this predicate.
 */
int eng3_indexed_interval_contains(const Eng3VisibilityRow *rows,
                                   U16 index,
                                   S32 coordinate)
{
    const Eng3VisibilityRow *row = rows + (U32)index;
    S32 lower = eng3_signed32(row->lower_bits);
    S32 upper = eng3_signed32(row->upper_bits);
    return coordinate >= lower && coordinate < upper;
}

/*
 * Bounded 0x204C0 endpoint path. An original-code fixture with a zero count
 * byte executed the stores at +4/+8 and returned; the original adds arg4 and
 * arg5 in a 32-bit register before storing +8. The optional/nested +0 store
 * and the full 0x20190 producer are deliberately outside this helper.
 */
void eng3_write_interval_endpoints(Eng3VisibilityRow *rows,
                                   U32 row_index,
                                   U32 lower_bits,
                                   U32 extent_bits)
{
    Eng3VisibilityRow *row = rows + row_index;
    row->lower_bits = lower_bits;
    row->upper_bits = lower_bits + extent_bits;
}
