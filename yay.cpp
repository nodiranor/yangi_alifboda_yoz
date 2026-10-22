/**
 * ============================================================================
 * YAY - Yangi Alifboda Yoz (Uzbek New Alphabet Native Input Assistant)
 * ============================================================================
 *
 * @file        yay.cpp
 * @project     YAY - Yangi Alifboda Yoz
 * @license     MIT License (Open Source)
 * @description Ultra-lightweight, zero-dependency native Windows input assistant
 *              that implements the reformed Uzbek Latin Alphabet typing rules.
 *
 * ----------------------------------------------------------------------------
 * ALPHABET REFORM MAPPINGS & TYPING RULES:
 * ----------------------------------------------------------------------------
 * 1. Double-Tap Fast Conversion (Default):
 *    - 'ss' (within 350ms) -> 'ş'  (Upper: 'SS' -> 'Ş')
 *    - 'oo' (within 350ms) -> 'ö'  (Upper: 'OO' -> 'Ö')
 *    - 'gg' (within 350ms) -> 'ğ'  (Upper: 'GG' -> 'Ğ')
 *    - 'cc' (within 350ms) -> 'ç'  (Upper: 'CC' -> 'Ç')
 *
 * 2. Instant Revert / Undo Mechanism:
 *    - When a double-tap is converted (e.g. 'ss' -> 'ş'), typing [Enter]
 *      immediately reverts the character back to literal double letters ('ss').
 *    - Any other key accepts the reformed character and closes the hint.
 *
 * 3. Legacy Digraph Combinations (Backwards Compatible):
 *    - 'sh' -> 'ş'  |  'SH' -> 'Ş'
 *    - 'ch' -> 'ç'  |  'CH' -> 'Ç'
 *    - 'o'' -> 'ö'  |  'O'' -> 'Ö'
 *    - 'g'' -> 'ğ'  |  'G'' -> 'Ğ'
 *
 * 4. Strict Input Method Isolation:
 *    - Active exclusively when Uzbek Latin (uz-Latn-UZ / 0x0443) is chosen
 *      via Windows Input Method Switcher [Win + Space].
 *    - Completely passes through keystrokes on English (0x0409) and
 *      Russian (0x0419) so other typing and shortcuts are 100% untouched.
 *
 * 5. Modern Non-Blocking UI:
 *    - Floating tips appear as sleek, rounded dark pills (capsules).
 *    - Positioned safely below the caret or in the corner of active window.
 *    - Instantly dismissed (0 ms) the moment the user resumes typing.
 *
 * ============================================================================
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>

// Link core Windows system libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")

// ============================================================================
// 1. CONSTANTS, RESOURCE IDS & CONFIGURATION
// ============================================================================

// Embedded Icon Resource Identifiers (defined in yay.rc)
#define IDI_APPICON    101
#define IDI_PAUSEDICON 102

// Custom Window Messages
#define WM_TRAYICON (WM_USER + 1)

// Tray Popup Menu Command IDs
#define ID_TRAY_OPEN_DASHBOARD 1000
#define ID_TRAY_ENABLE         1001
#define ID_TRAY_ONLY_UZBEK     1002
#define ID_TRAY_SINGLE_C       1003
#define ID_TRAY_STARTUP        1004
#define ID_TRAY_EXIT           1005

// Dashboard Dialog Control IDs
#define IDC_STATIC_LOGO        3000
#define IDC_CHK_ONLY_UZ        3001
#define IDC_CHK_SINGLE_C       3002
#define IDC_CHK_STARTUP        3003
#define IDC_BTN_HIDE           3004
#define IDC_BTN_EXIT           3005
#define IDC_EDIT_TEST          3006
#define IDC_STATUS_TEXT        3007
#define IDC_LANG_TEXT          3008
#define IDC_BTN_ADD_UZBEK      3009

// Timer Identifiers
#define TIMER_CHOICE           2001
#define TIMER_COACH            2002
#define TIMER_CHECK_LANG       2003

// Display durations for floating popups (auto-hide when idle; typing immediately dismisses)
#define DURATION_CHOICE_MS     4000  // 4 seconds
#define DURATION_COACH_MS      4000  // 4 seconds

// Maximum threshold between keystrokes to register a double-tap
const DWORD DOUBLE_TAP_MS = 350;

// Registry configuration path
static const wchar_t *REG_CONFIG_KEY = L"Software\\YAY_YangiAlifbo";
static const wchar_t *REG_RUN_KEY    = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t *APP_NAME       = L"YAY - Yangi Alifboda Yoz";

// ============================================================================
// 2. GLOBAL APPLICATION STATE & HANDLES
// ============================================================================

HHOOK  g_hHook       = NULL;  // Global low-level keyboard hook handle
HWND   g_hMainWnd    = NULL;  // Main dashboard settings window
HWND   g_hChoiceWnd  = NULL;  // Floating pill window for [Enter] undo hint
HWND   g_hCoachWnd   = NULL;  // Floating pill window for faster-typing tips

HFONT  g_hFont       = NULL;  // Base application font (Segoe UI 10.5pt)
HFONT  g_hTitleFont  = NULL;  // Header title font (Segoe UI Bold 15pt)
HFONT  g_hBoldFont   = NULL;  // Subheader bold font (Segoe UI Bold 10.5pt)

HICON  g_hIconActive = NULL;  // Active state icon (Project Icon / Blue UZ badge)
HICON  g_hIconPaused = NULL;  // Paused state icon (Grayscale Icon / Gray badge)

// Operational Settings
bool   g_bEnabled          = true;   // Master toggle: Enabled (true) or Paused (false)
bool   g_bOnlyUzbekLayout  = true;   // Strict mode: Active only when uz-Latn-UZ is active
bool   g_bInstantSingleC   = false;  // Optional mode: Typing single 'c' directly outputs 'ç'
bool   g_bSendingInput     = false;  // Guard to prevent recursive hook re-entry

// State Machine for Double-Tap & Undo
bool    g_bUndoDoubleTapActive       = false; // True when 'ss'->'ş' is awaiting possible Enter undo
wchar_t g_undoOriginalChars[8]       = {0};   // Stores original double chars (e.g. L"ss")
wchar_t g_undoReformedChar           = 0;     // Stores target reformed char (e.g. L'ş')

wchar_t g_choiceText[256]            = {0};   // Text buffer for undo pill
wchar_t g_coachText[256]             = {0};   // Text buffer for coach pill

wchar_t g_lastChar                   = 0;     // Previous character typed
DWORD   g_lastTime                   = 0;     // Timestamp of previous character

NOTIFYICONDATAW g_nid                = {0};   // System tray icon notification data

// Forward Declarations
void UpdateDashboardUI();
void UpdateTrayStatus();
void DismissAllPopups();

inline void AppLog(const wchar_t *msg) {
  OutputDebugStringW(msg);
}

// ============================================================================
// 3. ICON CREATION & FALLBACK GENERATOR
// ============================================================================

/**
 * Creates a clean programmatic 16x16 "UZ" badge icon as a guaranteed runtime fallback
 * in case binary icon resources are unavailable.
 */
HICON CreateUzbekBadgeIcon(bool active) {
  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);
  HBITMAP hbmColor = CreateCompatibleBitmap(hdcScreen, 16, 16);
  HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, NULL);
  HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);

  RECT rc = {0, 0, 16, 16};
  HBRUSH hBrush = CreateSolidBrush(active ? RGB(0, 120, 215) : RGB(90, 90, 100));
  FillRect(hdcMem, &rc, hBrush);
  DeleteObject(hBrush);

  SetBkMode(hdcMem, TRANSPARENT);
  SetTextColor(hdcMem, RGB(255, 255, 255));
  HFONT hBadgeFont = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  HFONT hOldFont = (HFONT)SelectObject(hdcMem, hBadgeFont);
  DrawTextW(hdcMem, L"UZ", 2, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  SelectObject(hdcMem, hOldFont);
  DeleteObject(hBadgeFont);
  SelectObject(hdcMem, hbmOld);

  ICONINFO ii = {0};
  ii.fIcon = TRUE;
  ii.hbmMask = hbmMask;
  ii.hbmColor = hbmColor;
  HICON hIcon = CreateIconIndirect(&ii);

  DeleteObject(hbmColor);
  DeleteObject(hbmMask);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdcScreen);
  return hIcon;
}

// ============================================================================
// 4. LANGUAGE DETECTION & ISOLATION ENGINE
// ============================================================================

/**
 * Determines whether the active foreground application is currently using
 * Uzbek Latin input method (0x0443 / uz-Latn).
 *
 * This ensures YAY operates with strict isolation:
 * - English typing (0x0409) is completely unaffected.
 * - Russian / Cyrillic typing (0x0419) is completely unaffected.
 * - Office, browser, and IDE shortcuts (Ctrl+C, Ctrl+V, etc.) work transparently.
 */
bool IsUzbekInputMethodActive() {
  if (!g_bEnabled)
    return false;

  HWND hForeground = GetForegroundWindow();
  DWORD dwThreadId = hForeground ? GetWindowThreadProcessId(hForeground, NULL) : 0;
  HKL hkl = GetKeyboardLayout(dwThreadId);
  WORD langId = LOWORD((DWORD_PTR)hkl);
  WORD primaryLang = PRIMARYLANGID(langId);
  WORD subLang = SUBLANGID(langId);

  // Russian (0x0419) and Uzbek Cyrillic (0x0843): always untouched
  if (primaryLang == LANG_RUSSIAN || langId == 0x0843)
    return false;

  if (g_bOnlyUzbekLayout) {
    // Strict Mode: active only when uz-Latn-UZ (0x0443) is chosen
    if (langId == 0x0443 || (primaryLang == LANG_UZBEK && subLang == SUBLANG_UZBEK_LATIN))
      return true;
    return false;
  } else {
    // Permissive Mode: active on English (0x0409) AND Uzbek (0x0443)
    return true;
  }
}

/**
 * Checks whether the Uzbek Latin keyboard layout is currently registered in Windows.
 */
bool IsUzbekLayoutInstalled() {
  int count = GetKeyboardLayoutList(0, NULL);
  if (count <= 0)
    return false;
  HKL *hkls = (HKL *)malloc(sizeof(HKL) * count);
  if (!hkls)
    return false;
  GetKeyboardLayoutList(count, hkls);
  bool found = false;
  for (int i = 0; i < count; i++) {
    WORD langId = LOWORD((DWORD_PTR)hkls[i]);
    if (langId == 0x0443 || PRIMARYLANGID(langId) == LANG_UZBEK) {
      found = true;
      break;
    }
  }
  free(hkls);
  return found;
}

/**
 * Background worker to silently install uz-Latn-UZ via PowerShell
 */
DWORD WINAPI InstallUzbekThreadProc(LPVOID lpParam) {
  const wchar_t *params =
      L"-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -EncodedCommand "
      L"JABsAGEAbgBnAHMAIAA9ACAARwBlAHQALQBXAGkAbgBVAHMAZQByAEwAYQBuAGcAdQBhAGcAZQBMAGkAcwB0ADsAIABpAGYAIAAoAC0AbgBvAHQAIAAoACQAbABhAG4AZwBzACAAfAAgAFcAaABlAHIAZQAtAE8AYgBqAGUAYwB0ACAAewAgACQAXwAuAEwAYQBuAGcAdQBhAGcAZQBUAGEAZwAgAC0AbABpAGsAZQAgACcAdQB6ACoAJwAgAH0AKQApACAAewAgACQAdQB6ACAAPQAgAE4AZQB3AC0AVwBpAG4AVQBzAGUAcgBMAGEAbgBnAHUAYQBnAGUATABpAHMAdAAgACcAdQB6AC0ATABhAHQAbgAtAFUAWgAnADsAIAAkAGwAYQBuAGcAcwAuAEEAZABkACgAJAB1AHoAWwAwAF0AKQA7ACAAUwBlAHQALQBXAGkAbgBVAHMAZQByAEwAYQBuAGcAdQBhAGcAZQBMAGkAcwB0ACAAJABsAGEAbgBnAHMAIAAtAEYAbwByAGMAZQAgAH0A";

  SHELLEXECUTEINFOW sei = {0};
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = L"open";
  sei.lpFile = L"powershell.exe";
  sei.lpParameters = params;
  sei.nShow = SW_HIDE;

  if (ShellExecuteExW(&sei) && sei.hProcess) {
    WaitForSingleObject(sei.hProcess, 15000);
    CloseHandle(sei.hProcess);
  }

  if (g_hMainWnd) {
    PostMessageW(g_hMainWnd, WM_COMMAND, MAKEWPARAM(IDC_BTN_ADD_UZBEK, 999), 0);
  }
  return 0;
}

void TriggerAddUzbekLayout() {
  CreateThread(NULL, 0, InstallUzbekThreadProc, NULL, 0, NULL);
}

// ============================================================================
// 5. SETTINGS & REGISTRY PERSISTENCE
// ============================================================================

void LoadSettings() {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_CONFIG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    DWORD val = 0, size = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"OnlyUzbek", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
      g_bOnlyUzbekLayout = (val != 0);
    size = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"SingleC", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS)
      g_bInstantSingleC = (val != 0);
    RegCloseKey(hKey);
  }
}

void SaveSettings() {
  HKEY hKey;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_CONFIG_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
    DWORD val = g_bOnlyUzbekLayout ? 1 : 0;
    RegSetValueExW(hKey, L"OnlyUzbek", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
    val = g_bInstantSingleC ? 1 : 0;
    RegSetValueExW(hKey, L"SingleC", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
    RegCloseKey(hKey);
  }
}

bool IsInStartup() {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    DWORD type = 0, size = 0;
    bool exists = (RegQueryValueExW(hKey, L"YAY_UzbekKeyboard", NULL, &type, NULL, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
  }
  return false;
}

void ToggleStartup(bool enable) {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
    if (!enable) {
      RegDeleteValueW(hKey, L"YAY_UzbekKeyboard");
    } else {
      wchar_t path[MAX_PATH];
      GetModuleFileNameW(NULL, path, MAX_PATH);
      wchar_t cmd[MAX_PATH + 16];
      swprintf_s(cmd, MAX_PATH + 16, L"\"%s\" /tray", path);
      RegSetValueExW(hKey, L"YAY_UzbekKeyboard", 0, REG_SZ, (BYTE *)cmd,
                     (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t)));
    }
    RegCloseKey(hKey);
  }
}

// ============================================================================
// 6. SYSTEM TRAY BADGE & NOTIFICATION ENGINE
// ============================================================================

void UpdateTrayStatus() {
  bool activeNow = g_bEnabled && IsUzbekInputMethodActive();
  HICON newIcon = activeNow ? g_hIconActive : g_hIconPaused;

  wchar_t newTip[128];
  if (!g_bEnabled) {
    wcscpy_s(newTip, 128, L"YAY: O'chirilgan (Pauza) / Ўчирилган (Пауза)");
  } else if (activeNow) {
    wcscpy_s(newTip, 128, L"YAY: Faol (UZ tanlangan) / Фаол (UZ)");
  } else {
    wcscpy_s(newTip, 128, L"YAY: Kutish rejimi (ENG/RUS) / Кутиш режими");
  }

  if (g_nid.hIcon != newIcon || wcscmp(g_nid.szTip, newTip) != 0) {
    g_nid.hIcon = newIcon;
    wcscpy_s(g_nid.szTip, 128, newTip);
    g_nid.uFlags = NIF_ICON | NIF_TIP;
    g_nid.szInfo[0] = L'\0';
    g_nid.szInfoTitle[0] = L'\0';
    g_nid.dwInfoFlags = 0;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  }

  UpdateDashboardUI();
}

// ============================================================================
// 7. SLEEK FLOATING PILL UI (NON-INTRUSIVE COACHING & UNDO HINTS)
// ============================================================================

/**
 * Immediately dismisses all visible floating pills and cancels active timers.
 * Called as soon as the user continues typing so UI never blocks the user.
 */
void DismissAllPopups() {
  g_bUndoDoubleTapActive = false;
  g_undoOriginalChars[0] = 0;
  g_undoReformedChar = 0;

  if (g_hMainWnd) {
    KillTimer(g_hMainWnd, TIMER_CHOICE);
    KillTimer(g_hMainWnd, TIMER_COACH);
  }
  if (g_hChoiceWnd && IsWindowVisible(g_hChoiceWnd))
    ShowWindow(g_hChoiceWnd, SW_HIDE);
  if (g_hCoachWnd && IsWindowVisible(g_hCoachWnd))
    ShowWindow(g_hCoachWnd, SW_HIDE);
}

// Backward compatibility alias
inline void DismissUndoPopup() {
  DismissAllPopups();
}

/**
 * Displays a modern, compact, rounded pill badge (capsule) that hints
 * the user without blocking their typing area.
 */
void ShowFloatingPill(HWND hWnd, const wchar_t *text, wchar_t *buffer,
                      UINT timerId, UINT duration) {
  wcscpy_s(buffer, 256, text);

  HDC hdc = GetDC(hWnd);
  HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
  RECT rc = {0, 0, 800, 0};
  DrawTextW(hdc, buffer, -1, &rc, DT_CALCRECT | DT_LEFT | DT_NOPREFIX);
  SelectObject(hdc, hOldFont);
  ReleaseDC(hWnd, hdc);

  int width = (rc.right - rc.left) + 26;   // Compact pill padding
  int height = (rc.bottom - rc.top) + 12;  // Compact pill height (~26px)

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);

  int posX = 0;
  int posY = 0;

  // Attempt to locate the active text caret
  GUITHREADINFO gti = {0};
  gti.cbSize = sizeof(GUITHREADINFO);
  HWND hForeground = GetForegroundWindow();
  bool hasCaret = false;
  POINT ptCaret = {0};

  if (hForeground) {
    DWORD dwThreadId = GetWindowThreadProcessId(hForeground, NULL);
    if (GetGUIThreadInfo(dwThreadId, &gti) && gti.hwndCaret && IsWindow(gti.hwndCaret)) {
      ptCaret.x = gti.rcCaret.left;
      ptCaret.y = gti.rcCaret.bottom;
      ClientToScreen(gti.hwndCaret, &ptCaret);
      if (ptCaret.x > 0 && ptCaret.x < screenW && ptCaret.y > 0 && ptCaret.y < screenH) {
        hasCaret = true;
      }
    }
  }

  if (hasCaret) {
    // Caret found: place cleanly below the input line without obstructing characters
    posX = ptCaret.x + 8;
    posY = ptCaret.y + 18;

    // Flip above caret line if near bottom of the screen
    if (posY + height > screenH - 55) {
      posY = ptCaret.y - height - (gti.rcCaret.bottom - gti.rcCaret.top) - 8;
    }
    if (posX + width > screenW - 12) posX = screenW - width - 12;
    if (posX < 12) posX = 12;
  } else {
    // Caret not exposed (e.g. Chrome, Word, VS Code, Discord):
    // Anchor to the top-right corner of the active window to stay 100% away from text inputs
    RECT rcFore = {0};
    if (hForeground && GetWindowRect(hForeground, &rcFore) &&
        (rcFore.right - rcFore.left > 200) && (rcFore.bottom - rcFore.top > 150)) {
      posX = rcFore.right - width - 24;
      posY = rcFore.top + 38;
      if (posX < rcFore.left + 12) posX = rcFore.left + 12;
      if (posY < 12) posY = 12;
    } else {
      // Fallback: bottom-right near system tray
      posX = screenW - width - 24;
      posY = screenH - height - 55;
    }
  }

  // Apply smooth rounded pill shape (capsule)
  HRGN hRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, 14, 14);
  SetWindowRgn(hWnd, hRgn, TRUE);

  SetWindowPos(hWnd, HWND_TOPMOST, posX, posY, width, height,
               SWP_NOACTIVATE | SWP_SHOWWINDOW);
  InvalidateRect(hWnd, NULL, TRUE);
  SetTimer(g_hMainWnd, timerId, duration, NULL);
}

void ShowUndoPopup(const wchar_t *origChars, wchar_t targetChar) {
  wchar_t msg[128];
  // Ultra-sleek hint: "↵ Enter: 'ss'"
  swprintf_s(msg, 128, L"\x21B5 Enter: '%s'", origChars);
  ShowFloatingPill(g_hChoiceWnd, msg, g_choiceText, TIMER_CHOICE, DURATION_CHOICE_MS);
}

void ShowCoachingTip(const wchar_t *doubleTapHint) {
  wchar_t msg[128];
  // Ultra-sleek coaching hint: "Maslahat: 'ss' tezroq"
  swprintf_s(msg, 128, L"Maslahat: '%s' tezroq", doubleTapHint);
  ShowFloatingPill(g_hCoachWnd, msg, g_coachText, TIMER_COACH, DURATION_COACH_MS);
}

// Window procedure for the sleek floating pill popups
LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_PAINT) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc;
    GetClientRect(hWnd, &rc);

    bool isChoice = (hWnd == g_hChoiceWnd);

    // Modern dark pill background
    HBRUSH hBrush = CreateSolidBrush(isChoice ? RGB(20, 24, 38) : RGB(28, 25, 20));
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // 1px subtle accent border (Indigo for undo, Amber for coaching)
    HPEN hPen = CreatePen(PS_SOLID, 1, isChoice ? RGB(99, 102, 241) : RGB(234, 179, 8));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, 0, 0, rc.right, rc.bottom, 14, 14);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isChoice ? RGB(241, 245, 249) : RGB(254, 243, 199));
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);

    RECT textRc = {10, 2, rc.right - 10, rc.bottom - 2};
    DrawTextW(hdc, isChoice ? g_choiceText : g_coachText, -1, &textRc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, hOldFont);
    EndPaint(hWnd, &ps);
    return 0;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// 8. SYNTHETIC INPUT INJECTION
// ============================================================================

void SendBackspaceAndUnicode(int backspaces, wchar_t ch) {
  g_bSendingInput = true;
  INPUT inputs[8] = {0};
  int idx = 0;

  for (int i = 0; i < backspaces && idx < 6; i++) {
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.wVk = VK_BACK;
    idx++;

    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.wVk = VK_BACK;
    inputs[idx].ki.dwFlags = KEYEVENTF_KEYUP;
    idx++;
  }

  inputs[idx].type = INPUT_KEYBOARD;
  inputs[idx].ki.wScan = ch;
  inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE;
  idx++;

  inputs[idx].type = INPUT_KEYBOARD;
  inputs[idx].ki.wScan = ch;
  inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
  idx++;

  SendInput(idx, inputs, sizeof(INPUT));
  g_bSendingInput = false;
}

void SendBackspaceAndChars(int backspaces, const wchar_t *chars) {
  g_bSendingInput = true;
  INPUT inputs[16] = {0};
  int idx = 0;

  for (int i = 0; i < backspaces && idx < 6; i++) {
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.wVk = VK_BACK;
    idx++;

    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.wVk = VK_BACK;
    inputs[idx].ki.dwFlags = KEYEVENTF_KEYUP;
    idx++;
  }

  for (int i = 0; chars[i] != 0 && idx < 14; i++) {
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.wScan = chars[i];
    inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE;
    idx++;

    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.wScan = chars[i];
    inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    idx++;
  }

  SendInput(idx, inputs, sizeof(INPUT));
  g_bSendingInput = false;
}

bool IsCapsActive() {
  bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
  bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
  return shift != caps;
}

// ============================================================================
// 9. LOW-LEVEL KEYBOARD HOOK ENGINE (STATE MACHINE)
// ============================================================================

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION && !g_bSendingInput && g_bEnabled && IsUzbekInputMethodActive()) {
    KBDLLHOOKSTRUCT *pKey = (KBDLLHOOKSTRUCT *)lParam;

    if ((pKey->flags & LLKHF_INJECTED) == 0 && wParam == WM_KEYDOWN) {
      DWORD vk = pKey->vkCode;

      // Allow modifier keys alone to pass freely
      if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
          vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
          vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
          vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN) {
        return CallNextHookEx(g_hHook, nCode, wParam, lParam);
      }

      // If Ctrl, Alt, or Win combinations are pressed (e.g. Ctrl+S, Ctrl+C, Ctrl+V, Alt+Tab),
      // pass through without interfering with Office / Browser shortcuts
      if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
          (GetAsyncKeyState(VK_MENU) & 0x8000) ||
          (GetAsyncKeyState(VK_LWIN) & 0x8000) ||
          (GetAsyncKeyState(VK_RWIN) & 0x8000)) {
        DismissAllPopups();
        g_lastChar = 0;
        return CallNextHookEx(g_hHook, nCode, wParam, lParam);
      }

      // If user tapped Enter right after double-tap (e.g. "ss"), revert 'ş' back to literal "ss"!
      if (g_bUndoDoubleTapActive) {
        if (vk == VK_RETURN || vk == VK_TAB) {
          wchar_t origChars[8] = {0};
          if (g_undoOriginalChars[0] != 0) {
            wcscpy_s(origChars, 8, g_undoOriginalChars);
          }
          DismissAllPopups();
          // Replace the single reformed char with the original double characters
          if (origChars[0] != 0) {
            SendBackspaceAndChars(1, origChars);
          }
          g_lastChar = 0;
          return 1; // Suppress Enter/Tab so literal "ss" is kept without newline
        } else if (vk == VK_ESCAPE) {
          DismissAllPopups();
          g_lastChar = 0;
          return 1;
        } else {
          // Any other key means user continued typing -> dismiss popup immediately!
          DismissAllPopups();
          // Fall through to process this new key normally
        }
      }

      bool isUpper = IsCapsActive();
      DWORD now = GetTickCount();

      // Mode: Optional Single 'c' -> 'ç'
      if (g_bInstantSingleC && vk == 'C') {
        DismissAllPopups();
        SendBackspaceAndUnicode(0, isUpper ? 0x00C7 : 0x00E7);
        g_lastChar = 0;
        return 1;
      }

      // Legacy combination: Sh -> Ş / Ch -> Ç
      if (vk == 'H') {
        if (g_lastChar == L's' && !isUpper) {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x015F);
          ShowCoachingTip(L"ss");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'S') {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x015E);
          ShowCoachingTip(L"SS");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'c' && !isUpper) {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x00E7);
          ShowCoachingTip(L"cc");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'C') {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x00C7);
          ShowCoachingTip(L"CC");
          g_lastChar = 0;
          return 1;
        }
      }

      // Legacy combination: o' -> ö / g' -> ğ
      bool isApostrophe =
          (vk == VK_OEM_7 && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) ||
          (vk == VK_OEM_3 && !(GetAsyncKeyState(VK_SHIFT) & 0x8000));
      if (isApostrophe) {
        if (g_lastChar == L'o') {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x00F6);
          ShowCoachingTip(L"oo");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'O') {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x00D6);
          ShowCoachingTip(L"OO");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'g') {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x011F);
          ShowCoachingTip(L"gg");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'G') {
          DismissAllPopups();
          SendBackspaceAndUnicode(1, 0x011E);
          ShowCoachingTip(L"GG");
          g_lastChar = 0;
          return 1;
        }
      }

      // Double tap: ss -> ş, oo -> ö, gg -> ğ, cc -> ç
      // Immediately replaces first character with reformed letter ('ş', 'ö', 'ğ', 'ç')!
      // If user taps [Enter] right after, it reverts to literal "ss", "oo", "gg", "cc".
      if (vk == 'S' || vk == 'O' || vk == 'G' || vk == 'C') {
        wchar_t curLower = (wchar_t)(vk + 32);
        wchar_t curChar = isUpper ? (wchar_t)vk : curLower;
        wchar_t lastLower = (g_lastChar >= L'A' && g_lastChar <= L'Z')
                                ? (g_lastChar + 32)
                                : g_lastChar;

        if (lastLower == curLower && (now - g_lastTime <= DOUBLE_TAP_MS)) {
          bool makeUpper =
              (g_lastChar >= L'A' && g_lastChar <= L'Z') || isUpper;
          wchar_t target = 0;
          if (vk == 'S')
            target = makeUpper ? 0x015E : 0x015F;
          if (vk == 'O')
            target = makeUpper ? 0x00D6 : 0x00F6;
          if (vk == 'G')
            target = makeUpper ? 0x011E : 0x011F;
          if (vk == 'C')
            target = makeUpper ? 0x00C7 : 0x00E7;

          DismissAllPopups();

          // Replace first character with the reformed letter immediately
          SendBackspaceAndUnicode(1, target);

          // Save original 2 characters for undo if user presses Enter
          g_undoOriginalChars[0] = g_lastChar;
          g_undoOriginalChars[1] = curChar;
          g_undoOriginalChars[2] = L'\0';
          g_undoReformedChar = target;
          g_bUndoDoubleTapActive = true;

          ShowUndoPopup(g_undoOriginalChars, target);
          g_lastChar = 0;
          return 1; // Suppress second key event because target character is already typed
        } else {
          DismissAllPopups();
          g_lastChar = curChar;
          g_lastTime = now;
        }
        return CallNextHookEx(g_hHook, nCode, wParam, lParam);
      }

      // Any other key: immediately hide all active instruction popups!
      DismissAllPopups();
      g_lastChar = 0;
    }
  }
  return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// ============================================================================
// 10. DASHBOARD GUI & SETTINGS WINDOW
// ============================================================================

void UpdateDashboardUI() {
  if (!g_hMainWnd || !IsWindowVisible(g_hMainWnd))
    return;

  HWND hStatus = GetDlgItem(g_hMainWnd, IDC_STATUS_TEXT);
  HWND hLang = GetDlgItem(g_hMainWnd, IDC_LANG_TEXT);
  HWND hBtnAdd = GetDlgItem(g_hMainWnd, IDC_BTN_ADD_UZBEK);

  bool active = g_bEnabled && IsUzbekInputMethodActive();
  if (hStatus) {
    if (!g_bEnabled) {
      SetWindowTextW(hStatus, L"Holat: O'chirilgan (Pauza) / Статус: Отключено");
    } else if (active) {
      SetWindowTextW(hStatus, L"Holat: FAOL (Yozish tayyor) / Статус: АКТИВНО");
    } else {
      SetWindowTextW(hStatus, L"Holat: Kutish rejimi (Boshqa til) / Статус: Ожидание");
    }
  }

  bool uzInstalled = IsUzbekLayoutInstalled();
  if (hBtnAdd) {
    if (uzInstalled) {
      SetWindowTextW(hBtnAdd, L"\x2713 O'zbek tili tizimda mavjud (Win+Space orqali tanlang)");
      EnableWindow(hBtnAdd, FALSE);
    } else {
      SetWindowTextW(hBtnAdd, L"+ Til paneliga O'zbekchani qo'shish (Win + Space)");
      EnableWindow(hBtnAdd, TRUE);
    }
  }

  if (hLang) {
    HWND hForeground = GetForegroundWindow();
    DWORD dwThreadId = hForeground ? GetWindowThreadProcessId(hForeground, NULL) : 0;
    HKL hkl = dwThreadId ? GetKeyboardLayout(dwThreadId) : GetKeyboardLayout(0);
    WORD langId = LOWORD((DWORD_PTR)hkl);

    wchar_t langBuf[256];
    if (langId == 0x0443) {
      swprintf_s(langBuf, 256, L"Hozirgi klaviatura: UZB (O'zbek Lotin) - Yangi alifbo faol!");
    } else if (langId == 0x0409) {
      if (!uzInstalled) {
        swprintf_s(langBuf, 256, L"Hozirgi klaviatura: ENG. Diqqat: UZB tili tizimda yo'q, qo'shish tugmasini bosing!");
      } else if (!g_bOnlyUzbekLayout) {
        swprintf_s(langBuf, 256, L"Hozirgi klaviatura: ENG (Inglizcha) - YAY faol ishlamoqda");
      } else {
        swprintf_s(langBuf, 256, L"Hozirgi klaviatura: ENG (Inglizcha). UZB ga o'tish: [Win + Space]");
      }
    } else if (PRIMARYLANGID(langId) == LANG_RUSSIAN) {
      swprintf_s(langBuf, 256, L"Hozirgi klaviatura: RUS (Русский). Rus tiliga xalaqit bermaydi");
    } else {
      swprintf_s(langBuf, 256, L"Hozirgi klaviatura: 0x%04X", langId);
    }
    SetWindowTextW(hLang, langBuf);
  }
}

LRESULT CALLBACK DashboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CREATE: {
    // 0. Project Logo Icon
    HWND hLogo = CreateWindowExW(0, L"STATIC", NULL,
                                 WS_CHILD | WS_VISIBLE | SS_ICON,
                                 24, 20, 32, 32, hWnd, (HMENU)IDC_STATIC_LOGO, NULL, NULL);
    SendMessageW(hLogo, STM_SETICON, (WPARAM)g_hIconActive, 0);

    // 1. Title Header
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"YAY - Yangi Alifboda Yoz",
                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  64, 18, 460, 28, hWnd, NULL, NULL, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);

    HWND hSub = CreateWindowExW(0, L"STATIC", L"Yangi O'zbek alifbosida oson va tez yozish uchun yordamchi dastur",
                                WS_CHILD | WS_VISIBLE | SS_LEFT,
                                64, 48, 460, 20, hWnd, NULL, NULL, NULL);
    SendMessageW(hSub, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 2. Status Indicator
    HWND hStatus = CreateWindowExW(0, L"STATIC", L"Holat: FAOL",
                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  24, 78, 500, 24, hWnd, (HMENU)IDC_STATUS_TEXT, NULL, NULL);
    SendMessageW(hStatus, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    HWND hLang = CreateWindowExW(0, L"STATIC", L"Klaviatura: Aniqlanmoqda...",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT,
                                 24, 104, 500, 22, hWnd, (HMENU)IDC_LANG_TEXT, NULL, NULL);
    SendMessageW(hLang, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    HWND hBtnAdd = CreateWindowExW(0, L"BUTTON", L"+ Til paneliga O'zbekchani qo'shish (Win + Space)",
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   24, 130, 500, 30, hWnd, (HMENU)IDC_BTN_ADD_UZBEK, NULL, NULL);
    SendMessageW(hBtnAdd, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 3. Live Interactive Typing Test Area
    HWND hTestLbl = CreateWindowExW(0, L"STATIC", L"Tezkor sinov (Bu yerga yozib ko'ring: 'ss' -> 'ş', 'oo' -> 'ö', 'gg' -> 'ğ'):",
                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    24, 168, 500, 20, hWnd, NULL, NULL, NULL);
    SendMessageW(hTestLbl, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 24, 192, 500, 32, hWnd, (HMENU)IDC_EDIT_TEST, NULL, NULL);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 4. Quick Rule Guide
    const wchar_t *guideText =
        L"Qoidalar:  ss -> ş   |   oo -> ö   |   gg -> ğ   |   cc -> ç\n"
        L"Agar 'ss' (massa, kassa) kerak bo'lsa: 'ss' ketidan [Enter] bosing!\n"
        L"Eski usul ham ishlaydi:  sh -> ş,  ch -> ç,  o' -> ö,  g' -> ğ";
    HWND hGuide = CreateWindowExW(0, L"STATIC", guideText,
                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  24, 234, 500, 55, hWnd, NULL, NULL, NULL);
    SendMessageW(hGuide, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 5. Settings Checkboxes
    HWND hChkOnlyUz = CreateWindowExW(0, L"BUTTON",
                                      L"Faqat O'zbek tili (UZB) tanlanganda ishlash (Inglizchaga aralashmaslik)",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      24, 298, 500, 24, hWnd, (HMENU)IDC_CHK_ONLY_UZ, NULL, NULL);
    SendMessageW(hChkOnlyUz, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(hChkOnlyUz, BM_SETCHECK, g_bOnlyUzbekLayout ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hChkSingleC = CreateWindowExW(0, L"BUTTON",
                                       L"Bitta 'C' tugmasi bosilganda to'g'ridan-to'g'ri 'Ç' yozish",
                                       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       24, 326, 500, 24, hWnd, (HMENU)IDC_CHK_SINGLE_C, NULL, NULL);
    SendMessageW(hChkSingleC, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(hChkSingleC, BM_SETCHECK, g_bInstantSingleC ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hChkStartup = CreateWindowExW(0, L"BUTTON",
                                       L"Windows bilan birga avtomatik ishga tushirish (Avtozagruzka)",
                                       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       24, 354, 500, 24, hWnd, (HMENU)IDC_CHK_STARTUP, NULL, NULL);
    SendMessageW(hChkStartup, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(hChkStartup, BM_SETCHECK, IsInStartup() ? BST_CHECKED : BST_UNCHECKED, 0);

    // 6. Action Buttons
    HWND hBtnHide = CreateWindowExW(0, L"BUTTON", L"Trayga yashirish (Yopish)",
                                    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                    140, 395, 230, 36, hWnd, (HMENU)IDC_BTN_HIDE, NULL, NULL);
    SendMessageW(hBtnHide, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    HWND hBtnExit = CreateWindowExW(0, L"BUTTON", L"Chiqish",
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    385, 395, 135, 36, hWnd, (HMENU)IDC_BTN_EXIT, NULL, NULL);
    SendMessageW(hBtnExit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    return 0;
  }

  case WM_CTLCOLORSTATIC: {
    HDC hdcStatic = (HDC)wParam;
    SetBkMode(hdcStatic, TRANSPARENT);
    return (INT_PTR)GetStockObject(WHITE_BRUSH);
  }

  case WM_ERASEBKGND: {
    HDC hdc = (HDC)wParam;
    RECT rc;
    GetClientRect(hWnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    return 1;
  }

  case WM_COMMAND: {
    int id = LOWORD(wParam);
    if (id == IDC_BTN_ADD_UZBEK) {
      if (HIWORD(wParam) == 999) {
        UpdateDashboardUI();
      } else {
        SetWindowTextW(GetDlgItem(hWnd, IDC_BTN_ADD_UZBEK), L"O'rnatilmoqda... / \x0423\x0441\x0442\x0430\x043D\x0430\x0432\x043B\x0438\x0432\x0430\x0435\x0442\x0441\x044F...");
        EnableWindow(GetDlgItem(hWnd, IDC_BTN_ADD_UZBEK), FALSE);
        TriggerAddUzbekLayout();
      }
    } else if (id == IDC_CHK_ONLY_UZ) {
      g_bOnlyUzbekLayout = (SendMessageW(GetDlgItem(hWnd, IDC_CHK_ONLY_UZ), BM_GETCHECK, 0, 0) == BST_CHECKED);
      SaveSettings();
      UpdateTrayStatus();
    } else if (id == IDC_CHK_SINGLE_C) {
      g_bInstantSingleC = (SendMessageW(GetDlgItem(hWnd, IDC_CHK_SINGLE_C), BM_GETCHECK, 0, 0) == BST_CHECKED);
      SaveSettings();
    } else if (id == IDC_CHK_STARTUP) {
      bool enable = (SendMessageW(GetDlgItem(hWnd, IDC_CHK_STARTUP), BM_GETCHECK, 0, 0) == BST_CHECKED);
      ToggleStartup(enable);
    } else if (id == IDC_BTN_HIDE) {
      AppLog(L"IDC_BTN_HIDE clicked: hiding window to tray");
      ShowWindow(hWnd, SW_HIDE);
    } else if (id == IDC_BTN_EXIT || id == ID_TRAY_EXIT) {
      AppLog(L"Exit command received (IDC_BTN_EXIT / ID_TRAY_EXIT): calling DestroyWindow");
      DestroyWindow(hWnd);
      return 0;
    } else if (id == ID_TRAY_OPEN_DASHBOARD) {
      AppLog(L"ID_TRAY_OPEN_DASHBOARD received: restoring and showing window");
      ShowWindow(hWnd, SW_SHOW);
      ShowWindow(hWnd, SW_RESTORE);
      SetForegroundWindow(hWnd);
      SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
      UpdateDashboardUI();
    } else if (id == ID_TRAY_ENABLE) {
      g_bEnabled = !g_bEnabled;
      UpdateTrayStatus();
    } else if (id == ID_TRAY_ONLY_UZBEK) {
      g_bOnlyUzbekLayout = !g_bOnlyUzbekLayout;
      SaveSettings();
      SendMessageW(GetDlgItem(hWnd, IDC_CHK_ONLY_UZ), BM_SETCHECK, g_bOnlyUzbekLayout ? BST_CHECKED : BST_UNCHECKED, 0);
      UpdateTrayStatus();
    } else if (id == ID_TRAY_SINGLE_C) {
      g_bInstantSingleC = !g_bInstantSingleC;
      SaveSettings();
      SendMessageW(GetDlgItem(hWnd, IDC_CHK_SINGLE_C), BM_SETCHECK, g_bInstantSingleC ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    return 0;
  }

  case WM_CLOSE:
    AppLog(L"WM_CLOSE received: minimizing window to tray (SW_HIDE)");
    ShowWindow(hWnd, SW_HIDE);
    return 0;

  case WM_TIMER:
    if (wParam == TIMER_CHOICE || wParam == TIMER_COACH) {
      DismissAllPopups();
    }
    if (wParam == TIMER_CHECK_LANG) {
      UpdateTrayStatus();
    }
    return 0;

  case WM_TRAYICON:
    // Left single click: toggle enabled / paused
    if (lParam == WM_LBUTTONUP) {
      g_bEnabled = !g_bEnabled;
      UpdateTrayStatus();
    } else if (lParam == WM_LBUTTONDBLCLK) {
      // Left double click: open dashboard
      ShowWindow(hWnd, SW_SHOW);
      ShowWindow(hWnd, SW_RESTORE);
      SetForegroundWindow(hWnd);
      SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
      UpdateDashboardUI();
    } else if (lParam == WM_RBUTTONUP) {
      // Right click: bilingual options popup menu
      HMENU hMenu = CreatePopupMenu();
      AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_DASHBOARD, L"YAY Oynasini ochish / \x041E\x0442\x043A\x0440\x044B\x0442\x044C YAY");
      AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
      AppendMenuW(hMenu, MF_STRING | (g_bEnabled ? MF_CHECKED : 0),
                  ID_TRAY_ENABLE,
                  g_bEnabled ? L"Faol (Yoqilgan) / \x0424\x0430\x043E\x043B (\x0401\x049B\x0438\x043B\x0433\x0430\x043D)"
                             : L"Pauza (O'chirilgan) / \x041F\x0430\x0443\x0437\x0430 (\x040E\x0447\x0438\x0440\x0438\x043B\x0433\x0430\x043D)");
      AppendMenuW(hMenu, MF_STRING | (g_bOnlyUzbekLayout ? MF_CHECKED : 0),
                  ID_TRAY_ONLY_UZBEK,
                  L"Faqat 'UZ' tilida ishlash / \x0422\x043E\x043B\x044C\x043A\x043E \x0432 'UZ'");
      AppendMenuW(
          hMenu, MF_STRING | (g_bInstantSingleC ? MF_CHECKED : 0),
          ID_TRAY_SINGLE_C,
          L"Bitta 'C' -> '\x00C7' rejimi / \x0420\x0435\x0436\x0438\x043C 'C' -> '\x00C7'");
      AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
      AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT,
                  L"Chiqish / \x0412\x044B\x0445\x043E\x0434");

      POINT pt;
      GetCursorPos(&pt);
      SetForegroundWindow(hWnd);
      TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0,
                     hWnd, NULL);
      DestroyMenu(hMenu);
    }
    return 0;

  case WM_DESTROY:
    AppLog(L"WM_DESTROY: removing tray icon and posting quit message");
    KillTimer(hWnd, TIMER_CHECK_LANG);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// 11. APPLICATION LIFECYCLE & ENTRY POINT
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR lpCmdLine, int nCmdShow) {
  const wchar_t *fullCmd = GetCommandLineW();
  wchar_t startLog[512];
  swprintf_s(startLog, 512, L"1. wWinMain started: [%s]", fullCmd ? fullCmd : L"");
  AppLog(startLog);

  // Prevent duplicate instances using a named system mutex
  HANDLE hMutex = CreateMutexW(NULL, TRUE, L"YAY_YangiAlifbodaYoz_Mutex");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    bool wantTray = (fullCmd && (wcsstr(fullCmd, L"/tray") != NULL || wcsstr(fullCmd, L"/minimized") != NULL));
    AppLog(wantTray ? L"1a. Instance already running, wantTray=true -> exiting immediately"
                    : L"1a. Instance already running, activating existing window");

    // Look for existing window
    HWND hExisting = FindWindowW(L"YAYDashboardClass", NULL);
    if (!hExisting) {
      // In case another instance is right in the middle of creating its window, wait briefly
      Sleep(150);
      hExisting = FindWindowW(L"YAYDashboardClass", NULL);
    }

    if (hExisting) {
      if (!wantTray) {
        AllowSetForegroundWindow(ASFW_ANY);
        SendMessageW(hExisting, WM_COMMAND, MAKEWPARAM(ID_TRAY_OPEN_DASHBOARD, 0), 0);
        ShowWindow(hExisting, SW_SHOW);
        ShowWindow(hExisting, SW_RESTORE);
        SetForegroundWindow(hExisting);
        SetWindowPos(hExisting, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
      }
      CloseHandle(hMutex);
      return 0;
    } else {
      AppLog(L"1b. Mutex existed but no active window found (orphaned/stuck). Taking over as active instance.");
      // Continue startup so the user is never locked out from opening the app!
    }
  }
  AppLog(L"2. Mutex initialized for this active instance");

  // Ensure Uzbek Latin input method is registered in Windows
  if (!IsUzbekLayoutInstalled()) {
    AppLog(L"3. Uzbek layout not installed, triggering background installer");
    TriggerAddUzbekLayout();
  } else {
    AppLog(L"3. Uzbek layout already installed");
  }

  if (!hInstance)
    hInstance = GetModuleHandleW(NULL);

  // Initialize Common Controls for modern visual styles
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&icex);

  LoadSettings();

  // Typography tokens
  g_hFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  g_hBoldFont = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  g_hTitleFont = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  // Load project icons (Active and Paused) from resources with fallbacks
  g_hIconActive = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  if (!g_hIconActive)
    g_hIconActive = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
  if (!g_hIconActive)
    g_hIconActive = (HICON)LoadImageW(hInstance, L"app.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
  if (!g_hIconActive)
    g_hIconActive = CreateUzbekBadgeIcon(true);

  g_hIconPaused = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_PAUSEDICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  if (!g_hIconPaused)
    g_hIconPaused = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_PAUSEDICON));
  if (!g_hIconPaused)
    g_hIconPaused = (HICON)LoadImageW(hInstance, L"app_paused.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
  if (!g_hIconPaused)
    g_hIconPaused = CreateUzbekBadgeIcon(false);

  // Register Main Dashboard Window Class
  WNDCLASSW wcMain = {0};
  wcMain.lpfnWndProc = DashboardWndProc;
  wcMain.hInstance = hInstance;
  wcMain.lpszClassName = L"YAYDashboardClass";
  wcMain.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcMain.hIcon = g_hIconActive;
  ATOM aRes = RegisterClassW(&wcMain);
  wchar_t regMsg[128];
  swprintf_s(regMsg, 128, L"4. RegisterClassW result: %p, err: %lu", (void*)(DWORD_PTR)aRes, GetLastError());
  AppLog(regMsg);

  // Register Floating Pill Window Class
  WNDCLASSW wcPopup = {0};
  wcPopup.lpfnWndProc = PopupWndProc;
  wcPopup.hInstance = hInstance;
  wcPopup.lpszClassName = L"YAYPopupClass";
  wcPopup.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassW(&wcPopup);

  int winW = 560;
  int winH = 500;
  int scrW = GetSystemMetrics(SM_CXSCREEN);
  int scrH = GetSystemMetrics(SM_CYSCREEN);
  int startX = (scrW - winW) / 2;
  int startY = (scrH - winH) / 2;

  g_hMainWnd = CreateWindowExW(WS_EX_APPWINDOW, L"YAYDashboardClass",
                               APP_NAME,
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                               startX, startY, winW, winH, NULL, NULL, hInstance, NULL);

  wchar_t createMsg[128];
  swprintf_s(createMsg, 128, L"5. CreateWindowExW result: %p, err: %lu", (void*)g_hMainWnd, GetLastError());
  AppLog(createMsg);

  // Set big and small window icons
  HICON hBigIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
  if (hBigIcon)
    SendMessageW(g_hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
  SendMessageW(g_hMainWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconActive);

  // Floating Pill Windows (no WS_BORDER, clipped to rounded shape)
  DWORD popStyle = WS_POPUP;
  DWORD popExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
  g_hChoiceWnd = CreateWindowExW(popExStyle, L"YAYPopupClass", NULL, popStyle, 0, 0,
                                 100, 30, NULL, NULL, hInstance, NULL);
  g_hCoachWnd = CreateWindowExW(popExStyle, L"YAYPopupClass", NULL, popStyle, 0, 0,
                                100, 30, NULL, NULL, hInstance, NULL);

  // System Tray Notification Icon setup
  g_nid.cbSize = sizeof(NOTIFYICONDATAW);
  g_nid.hWnd = g_hMainWnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = g_hIconActive;
  wcscpy_s(g_nid.szTip, 128, APP_NAME);

  // Immediate balloon welcome toast
  wcscpy_s(g_nid.szInfoTitle, 64, APP_NAME);
  wcscpy_s(g_nid.szInfo, 256,
           L"Dastur ishga tushdi! Yangi alifboda yozish tayyor.\nKlaviatura: UZB / ENG");
  g_nid.dwInfoFlags = NIIF_INFO;

  Shell_NotifyIconW(NIM_ADD, &g_nid);

  // Clear NIF_INFO immediately so periodic language switches do not re-trigger notifications!
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  g_nid.szInfo[0] = L'\0';
  g_nid.szInfoTitle[0] = L'\0';
  g_nid.dwInfoFlags = 0;

  // Periodic language state check (every 250ms) to update tray icon and dashboard
  SetTimer(g_hMainWnd, TIMER_CHECK_LANG, 250, NULL);
  UpdateTrayStatus();

  // If started with /tray or /minimized, hide to tray; otherwise show dashboard window
  bool startInTray = (fullCmd && (wcsstr(fullCmd, L"/tray") != NULL || wcsstr(fullCmd, L"/minimized") != NULL));
  AppLog(startInTray ? L"6. Starting hidden in tray" : L"6. Starting visible dashboard");
  if (!startInTray) {
    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);
    UpdateDashboardUI();
  }

  // Install Low-Level Keyboard Hook
  g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

  AppLog(L"7. Entering message loop");
  // Main Win32 Message Loop
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  wchar_t loopEndMsg[128];
  swprintf_s(loopEndMsg, 128, L"8. Message loop ended, msg: 0x%X wParam: %lu", msg.message, (DWORD)msg.wParam);
  AppLog(loopEndMsg);

  // Cleanup upon exit
  if (g_hHook)
    UnhookWindowsHookEx(g_hHook);
  if (g_hFont)
    DeleteObject(g_hFont);
  if (g_hBoldFont)
    DeleteObject(g_hBoldFont);
  if (g_hTitleFont)
    DeleteObject(g_hTitleFont);
  if (g_hIconActive)
    DestroyIcon(g_hIconActive);
  if (g_hIconPaused)
    DestroyIcon(g_hIconPaused);
  if (hBigIcon)
    DestroyIcon(hBigIcon);

  if (hMutex) {
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
  }
  AppLog(L"9. Process exiting cleanly");
  ExitProcess(0);
  return 0;
}

// Fallback entry point when compiled without -municode
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}