#include "app.h"

static UINT g_shellHookMsg = 0;
static UINT g_taskbarCreatedMsg = 0;

static void SetCheck(HWND hDlg, int id, bool on) { CheckDlgButton(hDlg, id, on ? BST_CHECKED : BST_UNCHECKED); }
static bool GetCheck(HWND hDlg, int id) { return IsDlgButtonChecked(hDlg, id) == BST_CHECKED; }

// Updates a control's text only when it differs, so the 1 s refresh never causes flicker.
static void SetTextIfChanged(HWND hDlg, int id, const wchar_t *text) {
  wchar_t cur[300];
  UINT n = GetDlgItemTextW(hDlg, id, cur, 300);
  if (n == 0 || lstrcmpW(cur, text) != 0) SetDlgItemTextW(hDlg, id, text);
}

void ShowDashboard() {
  if (!g_hMainWnd) return;
  ShowWindow(g_hMainWnd, SW_SHOW);
  ShowWindow(g_hMainWnd, SW_RESTORE);
  SetForegroundWindow(g_hMainWnd);
  UpdateDashboardUI();
}

void UpdateDashboardUI() {
  if (!g_hMainWnd || !IsWindowVisible(g_hMainWnd)) return;

  const ActiveProfile profile = g_bEnabled ? GetActiveProfile(NULL) : AP_NONE;
  const wchar_t *status = !g_bEnabled            ? L"Holat: O'chirilgan (pauza). Yoqish uchun tray belgisini bosing."
                        : profile == AP_LATIN    ? L"Holat: Faol. UZB (O'zbek lotin) tanlangan, yozishingiz mumkin."
                        : profile == AP_CYRILLIC ? L"Holat: Faol. Kirill rejimi: ў қ ғ ҳ ni ikki marta bosib yozing."
                                                 : L"Holat: Kutish rejimi. Boshqa til tanlangan.";
  SetTextIfChanged(g_hMainWnd, IDC_STATUS_TEXT, status);

  bool uzInstalled = IsUzbekLayoutInstalled();
  HWND hBtn = GetDlgItem(g_hMainWnd, IDC_BTN_ADD_UZBEK);
  if (uzInstalled) {
    SetTextIfChanged(g_hMainWnd, IDC_BTN_ADD_UZBEK, L"O'zbek (lotin) tili tizimda mavjud (Win + Space)");
    EnableWindow(hBtn, FALSE);
  } else if (g_bWin7PreloadWritten) {
    SetTextIfChanged(g_hMainWnd, IDC_BTN_ADD_UZBEK, L"Qayta kirgach UZB tili paydo bo'ladi");
    EnableWindow(hBtn, FALSE);
  } else {
    SetTextIfChanged(g_hMainWnd, IDC_BTN_ADD_UZBEK, L"O'zbek (lotin) tilini til paneliga qo'shish");
    EnableWindow(hBtn, TRUE);
  }

  HWND hFg = GetForegroundWindow();
  DWORD tid = hFg ? GetWindowThreadProcessId(hFg, NULL) : 0;
  WORD langId = LOWORD((DWORD_PTR)GetKeyboardLayout(tid));
  wchar_t buf[256];
  if (langId == 0x0443) {
    lstrcpynW(buf, L"Hozirgi klaviatura: UZB (O'zbek lotin). Yangi alifbo faol.", 256);
  } else if (langId == 0x0843) {
    lstrcpynW(buf, L"Hozirgi klaviatura: UZB (O'zbek kirill). ў қ ғ ҳ faol.", 256);
  } else if (PRIMARYLANGID(langId) == LANG_ENGLISH) {
    if (!uzInstalled)             lstrcpynW(buf, L"Hozirgi klaviatura: ENG. UZB tili tizimda yo'q, pastdagi tugmani bosing.", 256);
    else if (!g_bOnlyUzbekLayout) lstrcpynW(buf, L"Hozirgi klaviatura: ENG. YAY shu tilda ham ishlaydi.", 256);
    else                          lstrcpynW(buf, L"Hozirgi klaviatura: ENG. UZB tiliga o'tish: Win + Space", 256);
  } else if (PRIMARYLANGID(langId) == LANG_RUSSIAN) {
    if (g_bCyrillicOnRussian) lstrcpynW(buf, L"Hozirgi klaviatura: RUS. YAY ў қ ғ ҳ uchun ishlaydi.", 256);
    else                      lstrcpynW(buf, L"Hozirgi klaviatura: RUS. Rus tiliga xalaqit bermaydi.", 256);
  } else {
    wsprintfW(buf, L"Hozirgi klaviatura: 0x%04X. Tilni almashtirish: Win + Space", langId);
  }
  SetTextIfChanged(g_hMainWnd, IDC_LANG_TEXT, buf);
}

static void OnAddUzbekLayout(HWND hDlg) {
  if (IsWindows8OrGreaterReal()) {
    SetDlgItemTextW(hDlg, IDC_BTN_ADD_UZBEK, L"O'rnatilmoqda...");
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_ADD_UZBEK), FALSE);
    TriggerAddUzbekLayout();
    return;
  }
  if (AddUzbekPreloadWin7()) {
    g_bWin7PreloadWritten = true;
    MessageBoxW(hDlg,
                L"O'zbek (lotin) tili qo'shildi.\nTil panelida ko'rinishi uchun tizimdan chiqib (Log off), qayta kiring.",
                APP_NAME, MB_OK | MB_ICONINFORMATION);
  } else {
    MessageBoxW(hDlg,
                L"Tilni avtomatik qo'shib bo'lmadi.\nControl Panel > Region and Language > Keyboards and Languages "
                L"orqali \"Uzbek (Latin)\" tilini qo'shing.",
                APP_NAME, MB_OK | MB_ICONWARNING);
  }
  UpdateDashboardUI();
}

INT_PTR CALLBACK DashboardDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (g_shellHookMsg && msg == g_shellHookMsg) {
    if (wParam == HSHELL_LANGUAGE || wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED)
      UpdateTrayStatus();
    return TRUE;
  }
  // Explorer restarted: its tray was rebuilt without our icon.
  if (g_taskbarCreatedMsg && msg == g_taskbarCreatedMsg) {
    TrayAdd();
    UpdateTrayStatus();
    return TRUE;
  }

  switch (msg) {
  case WM_INITDIALOG:
    g_hMainWnd = hDlg;
    SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconActive);
    SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig);
    SetCheck(hDlg, IDC_CHK_ONLY_UZ, g_bOnlyUzbekLayout);
    SetCheck(hDlg, IDC_CHK_SINGLE_C, g_bInstantSingleC);
    SetCheck(hDlg, IDC_CHK_TIPS, g_bShowTips);
    SetCheck(hDlg, IDC_CHK_CYR_RU, g_bCyrillicOnRussian);
    SetCheck(hDlg, IDC_CHK_STARTUP, IsInStartup());
    g_shellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");
    g_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    RegisterShellHookWindow(hDlg);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_TEST));
    return FALSE;

  case WM_SHOWWINDOW:
    if (wParam) SetTimer(hDlg, TIMER_DASH_REFRESH, 1000, NULL);
    else KillTimer(hDlg, TIMER_DASH_REFRESH);
    return FALSE;

  case WM_COMMAND: {
    int id = LOWORD(wParam);
    switch (id) {
    case IDC_BTN_ADD_UZBEK: OnAddUzbekLayout(hDlg); return TRUE;
    case IDC_CHK_ONLY_UZ:   g_bOnlyUzbekLayout = GetCheck(hDlg, id); SaveSettings(); UpdateTrayStatus(); return TRUE;
    case IDC_CHK_SINGLE_C:  g_bInstantSingleC = GetCheck(hDlg, id); SaveSettings(); return TRUE;
    case IDC_CHK_TIPS:      g_bShowTips = GetCheck(hDlg, id); SaveSettings(); return TRUE;
    case IDC_CHK_CYR_RU:    g_bCyrillicOnRussian = GetCheck(hDlg, id); SaveSettings(); UpdateTrayStatus(); return TRUE;
    case IDC_CHK_STARTUP:   ToggleStartup(GetCheck(hDlg, id)); return TRUE;
    case IDC_BTN_HIDE:
    case IDCANCEL:          ShowWindow(hDlg, SW_HIDE); return TRUE;
    case IDC_BTN_EXIT:
    case ID_TRAY_EXIT:      DestroyWindow(hDlg); return TRUE;
    case ID_TRAY_OPEN_DASHBOARD: ShowDashboard(); return TRUE;
    case ID_TRAY_ENABLE:    g_bEnabled = !g_bEnabled; UpdateTrayStatus(); return TRUE;
    case ID_TRAY_ONLY_UZBEK:
      g_bOnlyUzbekLayout = !g_bOnlyUzbekLayout; SaveSettings();
      SetCheck(hDlg, IDC_CHK_ONLY_UZ, g_bOnlyUzbekLayout); UpdateTrayStatus(); return TRUE;
    case ID_TRAY_SINGLE_C:
      g_bInstantSingleC = !g_bInstantSingleC; SaveSettings();
      SetCheck(hDlg, IDC_CHK_SINGLE_C, g_bInstantSingleC); return TRUE;
    }
    return FALSE;
  }

  case WM_NOTIFY: {
    NMHDR *hdr = (NMHDR *)lParam;
    if (hdr->idFrom == IDC_SPONSOR && (hdr->code == NM_CLICK || hdr->code == NM_RETURN)) {
      NMLINK *link = (NMLINK *)lParam;
      const wchar_t *url = link->item.szUrl[0] ? link->item.szUrl : SPONSOR_URL;
      ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
      return TRUE;
    }
    return FALSE;
  }

#ifdef YAY_ACCEPT_INJECTED
  case WM_APP + 10:  // test builds only: switch this thread's keyboard layout (tests/e2e.ps1)
    ActivateKeyboardLayout((HKL)lParam, 0);
    return TRUE;
#endif

  case WM_LAYOUT_INSTALL_DONE:
    UpdateTrayStatus();
    return TRUE;

  case WM_TIMER:
    if (wParam == TIMER_COACH) HidePill();
    else if (wParam == TIMER_DASH_REFRESH) UpdateTrayStatus();
    return TRUE;

  // A double click delivers up, double-click, up. The two single-click toggles cancel out,
  // so the double-click itself only has to open the dashboard.
  case WM_TRAYICON:
    if (lParam == WM_LBUTTONUP) {
      g_bEnabled = !g_bEnabled;
      UpdateTrayStatus();
    } else if (lParam == WM_LBUTTONDBLCLK) {
      ShowDashboard();
    } else if (lParam == WM_RBUTTONUP) {
      TrayShowMenu(hDlg);
    }
    return TRUE;

  case WM_CLOSE:
    ShowWindow(hDlg, SW_HIDE);
    return TRUE;

  case WM_DESTROY:
    KillTimer(hDlg, TIMER_DASH_REFRESH);
    KillTimer(hDlg, TIMER_COACH);
    DeregisterShellHookWindow(hDlg);
    TrayRemove();
    g_hMainWnd = NULL;
    PostQuitMessage(0);
    return TRUE;
  }
  return FALSE;
}
