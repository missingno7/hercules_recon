#ifndef HERCULES_DISPLAY_MODE_CONTRACT_H
#define HERCULES_DISPLAY_MODE_CONTRACT_H
/* WORD source contract consistent with measured F2/F3 save/restore. The code
 * consumes only the low input BYTE; original declaration remains unproved.
 * Upper return-register/argument bits are outside this WORD contract. */
#ifdef __cplusplus
extern "C" {
#endif
unsigned short __cdecl cleanup_native_6ec0(unsigned short value);
#ifdef __cplusplus
}
#endif
#endif
