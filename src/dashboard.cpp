// dashboard.cpp - the main window.
//
// The window is a dialog laid out in res/yay.rc (IDD_MAIN). It only uses standard Windows controls,
// so it looks native on every Windows version and scales with the display DPI by itself.
// DashboardDlgProc is its message handler. Because this is the one window YAY always keeps (even
// when hidden), it also receives the messages of the tray icon, the timers and the system's
// "layout changed" notifications.
#include "app.h"

// Message numbers that Windows hands out at run time (RegisterWindowMessage), filled in by
// WM_INITDIALOG. One is the shell's "something changed" broadcast, the other is sent when
// Explorer restarts. Being run-time values, they cannot be `case` labels.
static UINT g_shellHookMsg = 0;
static UINT g_taskbarCreatedMsg = 0;

// Check or read a checkbox of the window.
static void SetCheck(HWND hDlg, int id, bool on) { CheckDlgButton(hDlg, id, on ? BST_CHECKED : BST_UNCHECKED); }
static bool GetCheck(HWND hDlg, int id) { return IsDlgButtonChecked(hDlg, id) == BST_CHECKED; }

// Updates a control's text only when it differs, so the 1 s refresh never causes flicker.
static void SetTextIfChanged(HWND hDlg, int id, const wchar_t *text) {
  wchar_t cur[300];
  UINT n = GetDlgItemTextW(hDlg, id, cur, 300);
  if (n == 0 || lstrcmpW(cur, text) != 0) SetDlgItemTextW(hDlg, id, text);
}

// Brings the window up from the tray: show it, undo a minimise, and give it the focus.
void ShowDashboard() {
  if (!g_hMainWnd) return;
  ShowWindow(g_hMainWnd, SW_SHOW);
  ShowWindow(g_hMainWnd, SW_RESTORE);
  SetForegroundWindow(g_hMainWnd);
  UpdateDashboardUI();
}

// Refreshes everything in the window that depends on the current state: the status line, the
// "add layout" button and the line that names the current keyboard. Cheap and safe to call
// often; it does nothing while the window is hidden.
void UpdateDashboardUI() {
  if (!g_hMainWnd || !IsWindowVisible(g_hMainWnd)) return;

  // Status line: what is YAY doing right now?
  const ActiveProfile profile = g_bEnabled ? GetActiveProfile(NULL) : AP_NONE;
  const wchar_t *status = !g_bEnabled            ? L"Holat: Öçirilgan (pauza). Yoqiş uçun tray belgisini bosing."
                        : profile == AP_LATIN    ? L"Holat: Faol. UZB (Özbek lotin) tanlangan, yozişingiz mumkin."
                        : profile == AP_CYRILLIC ? L"Holat: Faol. Kirillda yozing, yangi lotin alifbosida çiqadi."
                                                 : L"Holat: Kutiş rejimi. Boşqa til tanlangan.";
  SetTextIfChanged(g_hMainWnd, IDC_STATUS_TEXT, status);

  // The "add layout" button: once the layout exists it says so and is disabled. On Windows 7,
  // after the registry entries are written, only the sign-out is missing.
  bool uzInstalled = IsUzbekLayoutInstalled();
  HWND hBtn = GetDlgItem(g_hMainWnd, IDC_BTN_ADD_UZBEK);
  if (uzInstalled) {
    SetTextIfChanged(g_hMainWnd, IDC_BTN_ADD_UZBEK, L"Özbek (lotin) tili tizimda mavjud (Win + Space)");
    EnableWindow(hBtn, FALSE);
  } else if (g_bWin7PreloadWritten) {
    SetTextIfChanged(g_hMainWnd, IDC_BTN_ADD_UZBEK, L"Qayta kirgaç UZB tili paydo böladi");
    EnableWindow(hBtn, FALSE);
  } else {
    SetTextIfChanged(g_hMainWnd, IDC_BTN_ADD_UZBEK, L"Özbek (lotin) tilini til paneliga qöşiş");
    EnableWindow(hBtn, TRUE);
  }

  // Keyboard line: name the current layout and say what it means for YAY. langId is the low
  // 16 bits of the layout handle (0x0443 Uzbek Latin, 0x0843 Uzbek Cyrillic). Note that this is
  // the layout of the foreground window; when this window itself is in front, that is its own.
  HWND hFg = GetForegroundWindow();
  DWORD tid = hFg ? GetWindowThreadProcessId(hFg, NULL) : 0;
  WORD langId = LOWORD((DWORD_PTR)GetKeyboardLayout(tid));
  wchar_t buf[256];
  if (langId == 0x0443) {
    lstrcpynW(buf, L"Hozirgi klaviatura: UZB (Özbek lotin). Yangi alifbo faol.", 256);
  } else if (langId == 0x0843) {
    lstrcpynW(buf, L"Hozirgi klaviatura: UZB (Özbek kirill). Kirill harflar lotinga ögiriladi.", 256);
  } else if (PRIMARYLANGID(langId) == LANG_ENGLISH) {
    if (!uzInstalled)             lstrcpynW(buf, L"Hozirgi klaviatura: ENG. UZB tili tizimda yöq, pastdagi tugmani bosing.", 256);
    else if (!g_bOnlyUzbekLayout) lstrcpynW(buf, L"Hozirgi klaviatura: ENG. YAY şu tilda ham işlaydi.", 256);
    else                          lstrcpynW(buf, L"Hozirgi klaviatura: ENG. UZB tiliga ötiş: Win + Space", 256);
  } else if (PRIMARYLANGID(langId) == LANG_RUSSIAN) {
    if (g_bCyrillicOnRussian) lstrcpynW(buf, L"Hozirgi klaviatura: RUS. Kirill harflar lotinga ögiriladi.", 256);
    else                      lstrcpynW(buf, L"Hozirgi klaviatura: RUS. Rus tiliga xalaqit bermaydi.", 256);
  } else {
    wsprintfW(buf, L"Hozirgi klaviatura: 0x%04X. Tilni almaştiriş: Win + Space", langId);
  }
  SetTextIfChanged(g_hMainWnd, IDC_LANG_TEXT, buf);
}

// The "add layout" button. Windows 8 and later: start the PowerShell job in the background
// (WM_LAYOUT_INSTALL_DONE comes back when it ends). Windows 7: write the registry and ask the
// user to sign out and in.
static void OnAddUzbekLayout(HWND hDlg) {
  if (IsWindows8OrGreaterReal()) {
    SetDlgItemTextW(hDlg, IDC_BTN_ADD_UZBEK, L"Örnatilmoqda...");
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_ADD_UZBEK), FALSE);
    TriggerAddUzbekLayout();
    return;
  }
  if (AddUzbekPreloadWin7()) {
    g_bWin7PreloadWritten = true;
    MessageBoxW(hDlg,
                L"Özbek (lotin) tili qöşildi.\nTil panelida körinişi uçun tizimdan çiqib (Log off), qayta kiring.",
                APP_NAME, MB_OK | MB_ICONINFORMATION);
  } else {
    MessageBoxW(hDlg,
                L"Tilni avtomatik qöşib bölmadi.\nControl Panel > Region and Language > Keyboards and Languages "
                L"orqali \"Uzbek (Latin)\" tilini qöşing.",
                APP_NAME, MB_OK | MB_ICONWARNING);
  }
  UpdateDashboardUI();
}

// The window procedure. A dialog procedure returns TRUE when it handled the message and FALSE to
// let the default dialog code run. (WM_INITDIALOG is the exception: there the return value says
// whether Windows should set the focus.)
INT_PTR CALLBACK DashboardDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  // The shell broadcasts a message to windows registered with RegisterShellHookWindow whenever the
  // keyboard layout or the active window changes. That is how the tray icon follows the layout
  // without a polling timer.
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

  // The real messages. Each case ends with return TRUE (handled) or FALSE (use the default).
  switch (msg) {
  // Set the window icons, tick every box to match the current settings, and sign up for the
  // shell notifications above. FALSE: we set the focus ourselves (to the test box).
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

  // The one-second refresh timer only runs while the window is visible, so a hidden YAY costs
  // no CPU at all.
  case WM_SHOWWINDOW:
    if (wParam) SetTimer(hDlg, TIMER_DASH_REFRESH, 1000, NULL);
    else KillTimer(hDlg, TIMER_DASH_REFRESH);
    return FALSE;

  // Commands from three sources: this window's buttons and boxes (IDC_*), the Esc key
  // (IDCANCEL), and the tray menu (ID_TRAY_*). A second launch of YAY also sends
  // ID_TRAY_OPEN_DASHBOARD here (see main.cpp).
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

  // The sponsor link is a SysLink control. It sends WM_NOTIFY when it is clicked, or activated
  // with the keyboard, and we open the address in the default browser.
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

  // The PowerShell job finished: the layout list probably changed, so refresh.
  case WM_LAYOUT_INSTALL_DONE:
    UpdateTrayStatus();
    return TRUE;

  // TIMER_COACH: time to hide the hint popup. TIMER_DASH_REFRESH: the once-a-second refresh while
  // the window is visible. It catches changes the shell notification does not, such as the Uzbek
  // layout appearing a moment after PowerShell finishes.
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

  // The X button only hides the window; YAY keeps running in the tray. Use the Exit button or
  // the tray menu to quit.
  case WM_CLOSE:
    ShowWindow(hDlg, SW_HIDE);
    return TRUE;

  // Reached from the Exit button or the tray menu. Clean up, then end the message loop in main.cpp.
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
