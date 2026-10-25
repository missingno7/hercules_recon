#include <stdio.h>

/* PC .data virtual-zero cells; original TU/declarations are unproved. */
FILE *g_archive_stream;
unsigned long g_archive_directory_count;
int g_public_file_count;
/* PC file-backed initialized cell at RVA 0x365a4. */
int g_archive_last_stream_handle = -1;

/* Working char-array representation follows native writable .data.
 * Consumer extern qualification must agree. Historical formal source
 * constness, original TU and whole-image layout remain unproved. */
/* Exact complete PC strings; source constness and TU unproved. */
char g_archive_read_mode[] = "rb";
char g_archive_prefix_sonix[] = "M:\\LANGUAGE\\SONIX";
char g_archive_prefix_chop[] = "M:\\GRAFIX\\CHOP";
char g_archive_prefix_maps[] = "M:\\GRAFIX\\MAPS";
char g_archive_init_sort_failure[] = "PC_InitFileSys(3)";
char g_archive_init_read_failure[] = "PC_InitFileSys(2)";
char g_archive_init_open_failure[] = "PC_InitFileSys(1)";
char g_archive_filename[] = "hercules.fs";
char g_archive_open_error_format[] = "PC_fopen(\"%s\")";
char g_open_mode_error[] = "PCopen(2)";
char g_open_capacity_error[] = "PCopen(1)";
char g_seek_handle_error[] = "PClseek(1)";
char g_read_handle_error[] = "PCread(1)";
