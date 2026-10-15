#ifndef HERCULES_HOST_RESTART_DIALOG_H
#define HERCULES_HOST_RESTART_DIALOG_H
#include <afxwin.h>
/* Provisional derived source name; native runtime identity is inherited CDialog. */
class CHostRestartDialog : public CDialog
{
public:
    CHostRestartDialog(CWnd* parent = 0);
    virtual BOOL OnInitDialog();
    afx_msg void OnRva_0000EFC0();
    afx_msg void OnRva_0000EFE0();
    afx_msg void OnRva_0000F000();
    afx_msg void OnRva_0000F020();
    afx_msg void OnRva_0000F040();
    afx_msg void OnRva_0000F060();
    afx_msg void OnRva_0000F080();
    afx_msg void OnRva_0000F0A0();
    afx_msg void OnRva_0000F0C0();
    afx_msg void OnRva_0000F0E0();
protected:
    virtual void DoDataExchange(CDataExchange*);
    DECLARE_MESSAGE_MAP()
};
#endif
