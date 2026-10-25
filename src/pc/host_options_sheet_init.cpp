#include "host_options_classes.h"
BOOL COptionsSheet::OnInitDialog()
{
    BOOL result = CPropertySheet::OnInitDialog();
    CTabCtrl* tab = GetTabControl();
    TC_ITEM item;
    item.mask = TCIF_TEXT;
    item.pszText = "Keyboard";
    tab->SetItem(0,&item);
    item.mask = TCIF_TEXT;
    item.pszText = "GamePad";
    tab->SetItem(1,&item);
    item.mask = TCIF_TEXT;
    item.pszText = "Sound";
    tab->SetItem(2,&item);
    item.mask = TCIF_TEXT;
    item.pszText = "Game Play";
    tab->SetItem(3,&item);
    return result;
}
