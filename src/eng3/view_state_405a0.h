/* Original-backed ENG3 view prefix: nine 32-bit fields, Y at +4. */
#ifndef HERCULES_ENG3_VIEW_STATE_405A0_H
#define HERCULES_ENG3_VIEW_STATE_405A0_H
typedef struct Eng3View {
    long x, y, z;
    long pitch, yaw;
    long unknown_14, unknown_18, unknown_1c;
    long column;
} Eng3View;
#endif
