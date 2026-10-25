#ifndef ENG1_SPATIAL_READ_GUARD_H
#define ENG1_SPATIAL_READ_GUARD_H
/* Registered source object identities only; original RVAs identify diagnostics.
 * Unknown runtime-owned pointers retain the native unchecked access policy. */
void eng1_guard_spatial_read(const void *base, unsigned long index,
                             unsigned long stride, unsigned long width,
                             const char *site);
#endif
