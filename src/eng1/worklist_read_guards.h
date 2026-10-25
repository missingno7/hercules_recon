#ifndef ENG1_WORKLIST_READ_GUARDS_H
#define ENG1_WORKLIST_READ_GUARDS_H

/* Behavioral guards for the finite source prefixes selected by the
 * ENG1 worklist factories. Exact unknown pointers retain native unchecked
 * indexing. */
void eng1_guard_worklist_table_read(const void *base, unsigned long index,
                                    unsigned long stride,
                                    unsigned long width, const char *site);
void eng1_guard_worklist_group_owner(unsigned long group_index);

#endif
