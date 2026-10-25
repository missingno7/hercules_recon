#ifndef HERCULES_HOST_MUSIC_H
#define HERCULES_HOST_MUSIC_H
#include "../shared/engine_interface.h"
typedef unsigned long MusicU32;
typedef unsigned char MusicU8;
/* One observed eight-byte path record; total table extent/owner unknown. */
typedef struct HostMusicPathRecord { const char *path; MusicU32 unknown_004; } HostMusicPathRecord;
typedef char music_path_stride_8[(sizeof(HostMusicPathRecord)==8)?1:-1];
typedef char music_pointer_is_4[(sizeof(void *)==4)?1:-1];
typedef char music_offsets_fit_context[(0x29c+255*4+4<=sizeof(ResourceCallbackContext))?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
extern EngineInterface g_host_engine_interface;
extern HostMusicPathRecord g_host_music_paths_rva_35178[];
/* Sixteen-slot registration prefix, not a complete original ownership claim. */
extern void *g_host_music_bank_headers_rva_68e790[16];
extern char g_host_music_init_text[],g_host_music_load_text[],g_host_music_filename[];
extern char g_host_music_open_error[],g_host_music_download_error[];
void __cdecl host_music_apply_1000(void);
void __cdecl host_music_signal_1090(void);
void __cdecl host_music_set_bases_10b0(MusicU32,MusicU32);
void __cdecl host_music_initialize_10e0(void);
short __cdecl host_music_bank_register_c040(void *,short);
short __cdecl host_music_bank_preload_c090(void *,short);
int __cdecl host_music_load_1110(unsigned int,unsigned int);
#ifdef __cplusplus
}
#endif
#endif
