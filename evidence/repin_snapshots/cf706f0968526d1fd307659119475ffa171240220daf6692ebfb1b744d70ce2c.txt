/*
 * ENG3 RVA 0x3fbd0, in the loader-zero tail of .data: the kind table read
 * with an eight-byte stride by the ENG3 handlers (dword +0, then a signed
 * dword count at +4).
 *
 * Element 0 is the proven minimum. The reload loop at 0x1c1b5 forms
 * &g_kind_info[0].unknown through one HIGHLOW immediate (owner + 4), then
 * immediately loads g_kind_info[0].sidecar_offset through [edi-4]. The loader
 * at 0x1c069 also stores element 0 first. That makes [1] a declared minimum
 * extent only. It does not establish the table capacity, the element count,
 * the native object size, the original declaration or TU ownership. Indices
 * come from runtime file data, and nothing bounds them here.
 */
typedef struct KindInfo KindInfo;
struct KindInfo {
    unsigned long sidecar_offset;
    unsigned long unknown;
};

KindInfo g_kind_info[1];
