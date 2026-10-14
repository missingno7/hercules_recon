#ifndef HERCULES_HOST_SYSTEM_KEYS_H
#define HERCULES_HOST_SYSTEM_KEYS_H

#include <afxwin.h>

/* CWnd receiver view for the measured member ABI. Original class spelling,
 * complete base chain and object extent remain unproved; no window is created. */
class HostWindowInputHandlers : public CWnd
{
public:
    void OnSysKeyDown(UINT key, UINT repeatCount, UINT flags);
    void OnSysKeyUp(UINT key, UINT repeatCount, UINT flags);
};

#endif
