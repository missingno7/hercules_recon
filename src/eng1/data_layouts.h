#ifndef HERCULES_ENG1_BEHAVIORAL_DATA_LAYOUTS_H
#define HERCULES_ENG1_BEHAVIORAL_DATA_LAYOUTS_H
#include <stddef.h>

typedef struct Eng1FrameDescriptorRow24 {
    void *pointer_000;
    unsigned long field_004;
    unsigned long field_008;
    void *pointer_00c;
    void *pointer_010;
    void *pointer_014;
} Eng1FrameDescriptorRow24;

typedef struct Eng1ProfileRow28 {
    unsigned char prefix_000_00b[12];
    void *pointer_00c;
    void *pointer_010;
    unsigned long unknown_014;
    unsigned long unknown_018;
} Eng1ProfileRow28;

typedef char eng1_data_rows_have_observed_layout[
    (sizeof(Eng1FrameDescriptorRow24) == 24 &&
     offsetof(Eng1FrameDescriptorRow24, pointer_00c) == 12 &&
     offsetof(Eng1FrameDescriptorRow24, pointer_014) == 20 &&
     sizeof(Eng1ProfileRow28) == 28 &&
     offsetof(Eng1ProfileRow28, pointer_00c) == 12) ? 1 : -1];

extern Eng1FrameDescriptorRow24 eng1_descriptor_rows_5a690[12];
extern Eng1ProfileRow28 eng1_profile_rows_5df08[12];
#endif
