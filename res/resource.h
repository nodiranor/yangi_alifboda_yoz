// resource.h - numeric IDs shared by the resource script (yay.rc) and the C++ code (app.h).
#pragma once

// Icons stored in the exe. The tray switches between the two.
#define IDI_APPICON            101
#define IDI_PAUSEDICON         102

// The dashboard window's dialog template.
#define IDD_MAIN               200

// Controls of the dashboard dialog (IDC_*). The numbers only have to be unique inside the dialog.
// They are not in order because controls were added over time; nothing depends on the order, so
// do not renumber. To add a control: give it an ID here, place it in yay.rc, and handle it in
// DashboardDlgProc (src/dashboard.cpp).
#define IDC_STATUS_TEXT        3007
#define IDC_LANG_TEXT          3008
#define IDC_BTN_ADD_UZBEK      3009
#define IDC_GRP_RULES          3010
#define IDC_RULE1              3011
#define IDC_RULE2              3012
#define IDC_RULE3              3013
#define IDC_TEST_LABEL         3014
#define IDC_EDIT_TEST          3006
#define IDC_GRP_SETTINGS       3015
#define IDC_CHK_ONLY_UZ        3001
#define IDC_CHK_SINGLE_C       3002
#define IDC_CHK_TIPS           3016
#define IDC_CHK_STARTUP        3003
#define IDC_SPONSOR            3017
#define IDC_RULE4              3018
#define IDC_CHK_CYR_RU         3019
#define IDC_BTN_HIDE           3004

// Commands from the tray menu (ID_TRAY_*). They arrive in the dashboard as WM_COMMAND. A second
// launch of YAY sends ID_TRAY_OPEN_DASHBOARD the same way, to bring the first window forward.
#define ID_TRAY_OPEN_DASHBOARD 1000
#define ID_TRAY_ENABLE         1001
#define ID_TRAY_ONLY_UZBEK     1002
#define ID_TRAY_SINGLE_C       1003
#define ID_TRAY_EXIT           1005
