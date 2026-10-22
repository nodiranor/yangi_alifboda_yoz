#include "app.h"

static NOTIFYICONDATAW g_nid = {0};

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

void TrayRemove() {
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void UpdateTrayStatus() {
  bool activeNow = g_bEnabled && IsUzbekInputMethodActive();
  HICON icon = activeNow ? g_hIconActive : g_hIconPaused;
  const wchar_t *tip = !g_bEnabled ? L"YAY: O'chirilgan (pauza)"
                     : activeNow   ? L"YAY: Faol (UZB tanlangan)"
                                   : L"YAY: Kutish rejimi (boshqa til)";
  if (g_nid.hIcon != icon || lstrcmpW(g_nid.szTip, tip) != 0) {
    g_nid.hIcon = icon;
    lstrcpynW(g_nid.szTip, tip, 128);
    g_nid.uFlags = NIF_ICON | NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  }
  UpdateDashboardUI();
}

void TrayShowMenu(HWND hWnd) {
  HMENU hMenu = CreatePopupMenu();
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_DASHBOARD, L"YAY oynasini ochish");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING | (g_bEnabled ? MF_CHECKED : 0), ID_TRAY_ENABLE, L"Faol (yoqilgan)");
  AppendMenuW(hMenu, MF_STRING | (g_bOnlyUzbekLayout ? MF_CHECKED : 0), ID_TRAY_ONLY_UZBEK, L"Faqat UZB tilida ishlash");
  AppendMenuW(hMenu, MF_STRING | (g_bInstantSingleC ? MF_CHECKED : 0), ID_TRAY_SINGLE_C, L"Bitta 'c' → 'ç' rejimi");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Chiqish");

  POINT pt;
  GetCursorPos(&pt);
  SetForegroundWindow(hWnd);
  TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
  PostMessageW(hWnd, WM_NULL, 0, 0);
  DestroyMenu(hMenu);
}
