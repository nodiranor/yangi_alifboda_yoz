// tray.cpp - the notification-area ("system tray") icon and its right-click menu.
//
// The icon shows the state at a glance: the colourful icon while YAY is acting, a grey one while it
// is paused or the active layout is not one of ours. Clicks arrive in the dashboard window as
// WM_TRAYICON (see DashboardDlgProc): left click pauses or resumes, double click opens the window,
// right click shows the menu.
#include "app.h"

// Describes our tray icon. Kept for the whole run, because every later call (modify, delete)
// has to identify the same icon with the same window handle and ID.
static NOTIFYICONDATAW g_nid = {0};

// Creates the icon. Also called again when Explorer restarts, because it forgets all tray icons.
void TrayAdd() {
  g_nid.cbSize = sizeof(NOTIFYICONDATAW);
  g_nid.hWnd = g_hMainWnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = g_hIconActive;
  lstrcpynW(g_nid.szTip, APP_NAME, 128);
  Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// Removes the icon on exit. Without this, a dead icon would stay until the mouse passes over it.
void TrayRemove() {
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

// Picks the icon and tooltip for the current state and refreshes the dashboard text too.
// The tray is only touched when something actually changed, which avoids flicker.
void UpdateTrayStatus() {
  // The tooltip names what YAY is doing, not which layout is selected: the Latin rules also run
  // on the English layout, and the Cyrillic conversion also runs on the Russian layout.
  const ActiveProfile profile = g_bEnabled ? GetActiveProfile(NULL) : AP_NONE;
  const bool activeNow = (profile != AP_NONE);
  HICON icon = activeNow ? g_hIconActive : g_hIconPaused;
  const wchar_t *tip = !g_bEnabled            ? L"YAY: Öçirilgan (pauza)"
                     : profile == AP_LATIN    ? L"YAY: Faol (lotin)"
                     : profile == AP_CYRILLIC ? L"YAY: Faol (kirilldan lotinga)"
                                              : L"YAY: Kutiş rejimi (boşqa til)";
  if (g_nid.hIcon != icon || lstrcmpW(g_nid.szTip, tip) != 0) {
    g_nid.hIcon = icon;
    lstrcpynW(g_nid.szTip, tip, 128);
    g_nid.uFlags = NIF_ICON | NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  }
  UpdateDashboardUI();
}

// The right-click menu. Commands come back to the dashboard window as WM_COMMAND with ID_TRAY_*.
// A tray menu needs SetForegroundWindow before it opens and a harmless WM_NULL after it closes;
// without these, the menu would not close when the user clicks elsewhere. This is documented
// Windows behaviour for notification-area menus.
void TrayShowMenu(HWND hWnd) {
  HMENU hMenu = CreatePopupMenu();
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_DASHBOARD, L"YAY oynasini oçiş");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING | (g_bEnabled ? MF_CHECKED : 0), ID_TRAY_ENABLE, L"Faol (yoqilgan)");
  AppendMenuW(hMenu, MF_STRING | (g_bOnlyUzbekLayout ? MF_CHECKED : 0), ID_TRAY_ONLY_UZBEK, L"Faqat UZB tilida işlaş");
  AppendMenuW(hMenu, MF_STRING | (g_bInstantSingleC ? MF_CHECKED : 0), ID_TRAY_SINGLE_C, L"Bitta 'c' → 'ç' rejimi");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Çiqiş");

  POINT pt;
  GetCursorPos(&pt);
  SetForegroundWindow(hWnd);
  TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
  PostMessageW(hWnd, WM_NULL, 0, 0);
  DestroyMenu(hMenu);
}
