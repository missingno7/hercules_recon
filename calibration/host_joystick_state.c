#include <windows.h>
/* Only the measured seven-DWORD temporary joystick state prefix is owned here.
 * Surrounding native storage, original declarations and placement remain open. */
DWORD g_joystick_working_rva_56e5d8[7] = {0};
