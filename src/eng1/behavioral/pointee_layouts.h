#ifndef ENG1_POINTEE_LAYOUTS_H
#define ENG1_POINTEE_LAYOUTS_H
typedef struct Eng1LevelDirectoryPathBlock {
    char path_12[7][12];
    char path_16[2][16];
} Eng1LevelDirectoryPathBlock;
typedef struct Eng1DescriptorChildObservedRow24 {
    unsigned long field_000, field_004;
    void *payload_008;
    unsigned long field_00c, field_010, field_014;
} Eng1DescriptorChildObservedRow24;
/* Two observed rows; signed subindex capacity remains unproved. */
typedef struct Eng1DescriptorRow0PayloadObserved60 {
    unsigned long prefix_000, payload_view_004, prefix_008;
    Eng1DescriptorChildObservedRow24 child_rows[2];
} Eng1DescriptorRow0PayloadObserved60;
typedef char eng1_level_paths_size[sizeof(Eng1LevelDirectoryPathBlock) == 116 ? 1 : -1];
typedef char eng1_child_row24_size[sizeof(Eng1DescriptorChildObservedRow24) == 24 ? 1 : -1];
typedef char eng1_row0_payload_size[sizeof(Eng1DescriptorRow0PayloadObserved60) == 60 ? 1 : -1];
extern Eng1LevelDirectoryPathBlock g_eng1_level_directory_paths_5a7c0;
extern char g_eng1_overlay_name_slots_5e1e4[10][8];
extern Eng1DescriptorRow0PayloadObserved60 g_eng1_descriptor_row0_payload_5cdec;
#endif
