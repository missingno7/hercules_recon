#ifndef ENG1_ACTOR_CONTROL_TABLES_H
#define ENG1_ACTOR_CONTROL_TABLES_H
/* Writable measured prefixes, preserving native row strides and interior views.
 * No wider capacity, historical declaration or linked placement is asserted. */
typedef char eng1_control_table_word16[sizeof(unsigned short) == 2 ? 1 : -1];
extern unsigned short g_eng1_control_lookup_words_5c3b8[40];
extern unsigned short g_eng1_classifier_rows_65678[18][6];
#endif
