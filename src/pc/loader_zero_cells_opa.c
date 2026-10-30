/* Exact aggregate storage for the HERCULES.EXE GTE register block (RVA 0x68e708).
 * Same layout as the working callers in host_gte_0b2f0.c / host_gte_flags_0bd60.c.
 * Native base witness and extent basis: evidence/opa_gte_review_2026_10_31.json.
 * The 120-byte extent is the highest native field access (trans.pad at +0x74, width 4),
 * not neighbouring-symbol spacing. Working declaration, not proof of original name,
 * type, object start or TU layout. */
typedef struct HostVector { long vx, vy, vz, pad; } HostVector;
typedef struct HostGte {
    short v[3][3];
    long mac[3][3];
    short sxy[2][3];
    long ofx, ofy, h, mac0;
    short rot[3][3];
    HostVector trans;
} HostGte;

HostGte g_host_gte;
