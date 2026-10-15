#include <windows.h>
/* Measured PC256-entry virtual-key caption table and nine working DWORD slots.
 * Total surrounding storage and original declarations/TU remain unproved. */
LPCSTR g_keyboard_labels_rva_36cf0[256] = {
    0, (LPCSTR)-1, (LPCSTR)-1, (LPCSTR)-1, (LPCSTR)-1, 0, 0, 0, /*00..07*/
    "BackSpace", (LPCSTR)-1, 0, 0, (LPCSTR)-1, (LPCSTR)-1, 0, 0, /*08..0f*/
    (LPCSTR)-1, (LPCSTR)-1, (LPCSTR)-1, "Pause", "CapsLock", 0, 0, 0, /*10..17*/
    0, 0, 0, (LPCSTR)-1, 0, 0, 0, 0, /*18..1f*/
    (LPCSTR)-1, "PageUp", "PageDown", "End", "Home", "Left", "Up", "Right", /*20..27*/
    "Down", "Select", 0, "Execute", (LPCSTR)-1, "Insert", "Delete", (LPCSTR)-1, /*28..2f*/
    "0", "1", "2", "3", "4", "5", "6", "7", /*30..37*/
    "8", "9", 0, 0, 0, 0, 0, 0, /*38..3f*/
    0, "A", "B", "C", "D", "E", "F", "G", /*40..47*/
    "H", "I", "J", "K", "L", "M", "N", "O", /*48..4f*/
    "P", "Q", "R", "S", "T", "U", "V", "W", /*50..57*/
    "X", "Y", "Z", 0, 0, 0, 0, 0, /*58..5f*/
    "Pad0", "Pad1", "Pad2", "Pad3", "Pad4", "Pad5", "Pad6", "Pad7", /*60..67*/
    "Pad8", "Pad9", "PadMul", "PadAdd", "PadSep", "PadSub", "PadDec", "PadDiv", /*68..6f*/
    (LPCSTR)-1, (LPCSTR)-1, (LPCSTR)-1, "F4", "F5", "F6", "F7", "F8", /*70..77*/
    "F9", (LPCSTR)-1, "F11", "F12", "F13", "F14", "F15", "F16", /*78..7f*/
    "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", /*80..87*/
    0, 0, 0, 0, 0, 0, 0, 0, /*88..8f*/
    (LPCSTR)-1, "ScrollLock", 0, 0, 0, 0, 0, 0, /*90..97*/
    0, 0, 0, 0, 0, 0, 0, 0, /*98..9f*/
    0, 0, 0, 0, 0, 0, 0, 0, /*a0..a7*/
    0, 0, 0, 0, 0, 0, 0, 0, /*a8..af*/
    0, 0, 0, 0, 0, 0, 0, 0, /*b0..b7*/
    0, 0, 0, 0, 0, 0, 0, 0, /*b8..bf*/
    0, 0, 0, 0, 0, 0, 0, 0, /*c0..c7*/
    0, 0, 0, 0, 0, 0, 0, 0, /*c8..cf*/
    0, 0, 0, 0, 0, 0, 0, 0, /*d0..d7*/
    0, 0, 0, 0, 0, 0, 0, 0, /*d8..df*/
    0, 0, 0, 0, 0, 0, "ProcessKey", 0, /*e0..e7*/
    0, 0, 0, 0, 0, 0, 0, 0, /*e8..ef*/
    0, 0, 0, 0, 0, 0, 0, 0, /*f0..f7*/
    0, 0, 0, 0, 0, 0, 0, 0 /*f8..ff*/
};
DWORD g_keyboard_working_rva_56e5f4[9] = {0};
