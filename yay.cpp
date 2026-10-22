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

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")

#define WM_TRAYICON (WM_USER + 1)

#define ID_TRAY_OPEN_DASHBOARD 1000
#define ID_TRAY_ENABLE 1001
#define ID_TRAY_ONLY_UZBEK 1002
#define ID_TRAY_SINGLE_C 1003
#define ID_TRAY_STARTUP 1004
#define ID_TRAY_EXIT 1005

#define IDC_CHK_ONLY_UZ 3001
#define IDC_CHK_SINGLE_C 3002
#define IDC_CHK_STARTUP 3003
#define IDC_BTN_HIDE 3004
#define IDC_BTN_EXIT 3005
#define IDC_EDIT_TEST 3006
#define IDC_STATUS_TEXT 3007
#define IDC_LANG_TEXT 3008

#define TIMER_CHOICE 2001
#define TIMER_COACH 2002
#define TIMER_CHECK_LANG 2003

// Display durations for floating popups (auto-hide if idle; typing immediately dismisses)
#define DURATION_CHOICE_MS 4000  // 4 seconds
#define DURATION_COACH_MS 4000   // 4 seconds

HHOOK g_hHook = NULL;
HWND g_hMainWnd = NULL;
HWND g_hChoiceWnd = NULL;
HWND g_hCoachWnd = NULL;
HFONT g_hFont = NULL;
HFONT g_hTitleFont = NULL;
HFONT g_hBoldFont = NULL;
HICON g_hIconActive = NULL;
HICON g_hIconPaused = NULL;

bool g_bEnabled = true;
// Default: Strict isolation (Active only when uz-Latn-UZ is selected in Windows)
bool g_bOnlyUzbekLayout = true;
bool g_bInstantSingleC = false;
bool g_bSendingInput = false;

// Double-tap state: 'ss' immediately outputs 'ş'.
// If [Enter] is tapped right after, it reverts back to 'ss'.
bool g_bUndoDoubleTapActive = false;
wchar_t g_undoOriginalChars[8] = {0};
wchar_t g_undoReformedChar = 0;

wchar_t g_choiceText[256] = {0};
wchar_t g_coachText[256] = {0};

wchar_t g_lastChar = 0;
DWORD g_lastTime = 0;
const DWORD DOUBLE_TAP_MS = 350;

NOTIFYICONDATAW g_nid = {0};

// Forward Declarations
void UpdateDashboardUI();
void UpdateTrayStatus();

// Dynamically generate a clean "UZ" Input Method Indicator Icon (16x16)
HICON CreateUzbekBadgeIcon(bool active) {
  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);
  HBITMAP hbmColor = CreateCompatibleBitmap(hdcScreen, 16, 16);
  HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, NULL);
  HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);

  RECT rc = {0, 0, 16, 16};
  HBRUSH hBrush =
      CreateSolidBrush(active ? RGB(0, 120, 215) : RGB(90, 90, 100));
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

// Check if current Windows Input Method is specifically Uzbek Latin (0x0443)
bool IsUzbekInputMethodActive() {
  if (!g_bEnabled)
    return false;

  HWND hForeground = GetForegroundWindow();
  DWORD dwThreadId = hForeground ? GetWindowThreadProcessId(hForeground, NULL) : 0;
  HKL hkl = GetKeyboardLayout(dwThreadId);
  WORD langId = LOWORD((DWORD_PTR)hkl);
  WORD primaryLang = PRIMARYLANGID(langId);
  WORD subLang = SUBLANGID(langId);

  // Russian / Uzbek Cyrillic: ALWAYS PAUSED so Cyrillic typing is untouched
  if (primaryLang == LANG_RUSSIAN || langId == 0x0843)
    return false;

  if (g_bOnlyUzbekLayout) {
    // Isolated Mode: Active ONLY when uz-Latn-UZ (0x0443) is chosen in Windows.
    if (langId == 0x0443 || (primaryLang == LANG_UZBEK && subLang == SUBLANG_UZBEK_LATIN))
      return true;
    return false;
  } else {
    // Permissive Mode: Active on English (0x0409) AND Uzbek (0x0443)
    return true;
  }
}

void UpdateTrayStatus() {
  bool activeNow = g_bEnabled && IsUzbekInputMethodActive();
  HICON newIcon = activeNow ? g_hIconActive : g_hIconPaused;
  
  wchar_t newTip[128];
  if (!g_bEnabled) {
    wcscpy_s(newTip, 128,
             L"YAY: O'chirilgan (Pauza) / \x040E\x0447\x0438\x0440\x0438\x043B\x0433\x0430\x043D");
  } else if (activeNow) {
    wcscpy_s(newTip, 128,
             L"YAY: Faol (UZ tanlangan) / \x0424\x0430\x043E\x043B (UZ)");
  } else {
    wcscpy_s(newTip, 128,
             L"YAY: Kutish rejimi (ENG/RUS) / \x041A\x0443\x0442\x0438\x0448 \x0440\x0435\x0436\x0438\x043C\x0438");
  }

  if (g_nid.hIcon != newIcon || wcscmp(g_nid.szTip, newTip) != 0) {
    g_nid.hIcon = newIcon;
    wcscpy_s(g_nid.szTip, 128, newTip);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
  }

  UpdateDashboardUI();
}

bool IsInStartup() {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                    KEY_READ, &hKey) == ERROR_SUCCESS) {
    DWORD type = 0, size = 0;
    bool exists = (RegQueryValueExW(hKey, L"YAY_UzbekKeyboard", NULL, &type,
                                    NULL, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
  }
  return false;
}

void ToggleStartup(bool enable) {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                    KEY_WRITE, &hKey) == ERROR_SUCCESS) {
    if (!enable) {
      RegDeleteValueW(hKey, L"YAY_UzbekKeyboard");
    } else {
      wchar_t path[MAX_PATH];
      GetModuleFileNameW(NULL, path, MAX_PATH);
      RegSetValueExW(hKey, L"YAY_UzbekKeyboard", 0, REG_SZ, (BYTE *)path,
                     (DWORD)((wcslen(path) + 1) * sizeof(wchar_t)));
    }
    RegCloseKey(hKey);
  }
}

void LoadSettings() {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\YAY_YangiAlifbo", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
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
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\YAY_YangiAlifbo", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
    DWORD val = g_bOnlyUzbekLayout ? 1 : 0;
    RegSetValueExW(hKey, L"OnlyUzbek", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
    val = g_bInstantSingleC ? 1 : 0;
    RegSetValueExW(hKey, L"SingleC", 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
    RegCloseKey(hKey);
  }
}

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

inline void DismissUndoPopup() {
  DismissAllPopups();
}

void ShowFloatingPill(HWND hWnd, const wchar_t *text, wchar_t *buffer,
                      UINT timerId, UINT duration) {
  wcscpy_s(buffer, 256, text);

  HDC hdc = GetDC(hWnd);
  HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
  RECT rc = {0, 0, 800, 0};
  DrawTextW(hdc, buffer, -1, &rc, DT_CALCRECT | DT_LEFT | DT_NOPREFIX);
  SelectObject(hdc, hOldFont);
  ReleaseDC(hWnd, hdc);

  int width = (rc.right - rc.left) + 26;   // sleek pill width
  int height = (rc.bottom - rc.top) + 12;  // sleek compact height (~26px)

  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);

  int posX = 0;
  int posY = 0;

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
    // Caret found: place safely below the input line without blocking letters
    posX = ptCaret.x + 8;
    posY = ptCaret.y + 18;

    // Boundary check: if near bottom of screen, flip above the line
    if (posY + height > screenH - 55) {
      posY = ptCaret.y - height - (gti.rcCaret.bottom - gti.rcCaret.top) - 8;
    }
    if (posX + width > screenW - 12) posX = screenW - width - 12;
    if (posX < 12) posX = 12;
  } else {
    // No caret (Chrome, Word, VS Code, Discord):
    // Anchor to the top-right corner of the active application window
    // so it never obstructs the typing line or search/chat boxes!
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

  // Set smooth rounded pill shape
  HRGN hRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, 14, 14);
  SetWindowRgn(hWnd, hRgn, TRUE);

  SetWindowPos(hWnd, HWND_TOPMOST, posX, posY, width, height,
               SWP_NOACTIVATE | SWP_SHOWWINDOW);
  InvalidateRect(hWnd, NULL, TRUE);
  SetTimer(g_hMainWnd, timerId, duration, NULL);
}

void ShowUndoPopup(const wchar_t *origChars, wchar_t targetChar) {
  wchar_t msg[128];
  // Ultra-sleek, clean pill hint: "↵ Enter: 'ss'"
  swprintf_s(msg, 128, L"\x21B5 Enter: '%s'", origChars);
  ShowFloatingPill(g_hChoiceWnd, msg, g_choiceText, TIMER_CHOICE, DURATION_CHOICE_MS);
}

void ShowCoachingTip(const wchar_t *doubleTapHint) {
  wchar_t msg[128];
  // Ultra-sleek coaching hint: "Maslahat: 'ss' tezroq"
  swprintf_s(msg, 128, L"Maslahat: '%s' tezroq", doubleTapHint);
  ShowFloatingPill(g_hCoachWnd, msg, g_coachText, TIMER_COACH, DURATION_COACH_MS);
}

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

      // If Ctrl, Alt, or Win combinations are pressed (e.g. Ctrl+S, Ctrl+C, Ctrl+H, Alt+Tab),
      // pass through without interfering with Office / Browser shortcuts
      if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
          (GetAsyncKeyState(VK_MENU) & 0x8000) ||
          (GetAsyncKeyState(VK_LWIN) & 0x8000) ||
          (GetAsyncKeyState(VK_RWIN) & 0x8000)) {
        DismissUndoPopup();
        g_lastChar = 0;
        return CallNextHookEx(g_hHook, nCode, wParam, lParam);
      }

      // If user tapped Enter right after double-tap (e.g. "ss"), revert 'ş' back to literal "ss"!
      if (g_bUndoDoubleTapActive) {
        if (vk == VK_RETURN || vk == VK_TAB) {
          DismissUndoPopup();
          // Replace the single reformed char with the original double characters
          SendBackspaceAndChars(1, g_undoOriginalChars);
          g_lastChar = 0;
          return 1; // Suppress Enter/Tab so literal "ss" is kept without newline
        } else if (vk == VK_ESCAPE) {
          DismissUndoPopup();
          g_lastChar = 0;
          return 1;
        } else {
          // Any other key means user accepted the reformed character (e.g. 'ş')!
          DismissUndoPopup();
          // Fall through to process this new key normally
        }
      }

      bool isUpper = IsCapsActive();
      DWORD now = GetTickCount();

      // Mode: Single 'c' -> 'ç'
      if (g_bInstantSingleC && vk == 'C') {
        DismissUndoPopup();
        SendBackspaceAndUnicode(0, isUpper ? 0x00C7 : 0x00E7);
        g_lastChar = 0;
        return 1;
      }

      // Legacy combination: Sh -> Ş / Ch -> Ç
      if (vk == 'H') {
        if (g_lastChar == L's' && !isUpper) {
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x015F);
          ShowCoachingTip(L"ss");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'S') {
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x015E);
          ShowCoachingTip(L"SS");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'c' && !isUpper) {
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x00E7);
          ShowCoachingTip(L"cc");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'C') {
          DismissUndoPopup();
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
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x00F6);
          ShowCoachingTip(L"oo");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'O') {
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x00D6);
          ShowCoachingTip(L"OO");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'g') {
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x011F);
          ShowCoachingTip(L"gg");
          g_lastChar = 0;
          return 1;
        }
        if (g_lastChar == L'G') {
          DismissUndoPopup();
          SendBackspaceAndUnicode(1, 0x011E);
          ShowCoachingTip(L"GG");
          g_lastChar = 0;
          return 1;
        }
      }

      // Double tap: ss -> ş, oo -> ö, gg -> ğ, cc -> ç
      // By default: IMMEDIATELY becomes the reformed letter ('ş', 'ö', 'ğ', 'ç')!
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

          // Replace the first character with reformed letter immediately!
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
          DismissUndoPopup();
          g_lastChar = curChar;
          g_lastTime = now;
        }
        return CallNextHookEx(g_hHook, nCode, wParam, lParam);
      }

      DismissUndoPopup();
      g_lastChar = 0;
    }
  }
  return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam) {
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

    // Sleek 1px accent border
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

void UpdateDashboardUI() {
  if (!g_hMainWnd || !IsWindowVisible(g_hMainWnd))
    return;

  HWND hStatus = GetDlgItem(g_hMainWnd, IDC_STATUS_TEXT);
  HWND hLang = GetDlgItem(g_hMainWnd, IDC_LANG_TEXT);

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

  if (hLang) {
    HWND hForeground = GetForegroundWindow();
    DWORD dwThreadId = hForeground ? GetWindowThreadProcessId(hForeground, NULL) : 0;
    HKL hkl = dwThreadId ? GetKeyboardLayout(dwThreadId) : GetKeyboardLayout(0);
    WORD langId = LOWORD((DWORD_PTR)hkl);

    wchar_t langBuf[256];
    if (langId == 0x0443) {
      swprintf_s(langBuf, 256, L"Hozirgi klaviatura: UZB (O'zbek Lotin) - Yangi alifbo faol!");
    } else if (langId == 0x0409) {
      if (!g_bOnlyUzbekLayout) {
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
    // 1. Title Header
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"YAY - Yangi Alifboda Yoz",
                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  24, 20, 500, 28, hWnd, NULL, NULL, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);

    HWND hSub = CreateWindowExW(0, L"STATIC", L"Yangi O'zbek alifbosida oson va tez yozish uchun yordamchi dastur",
                                WS_CHILD | WS_VISIBLE | SS_LEFT,
                                24, 52, 500, 20, hWnd, NULL, NULL, NULL);
    SendMessageW(hSub, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 2. Status Indicator
    HWND hStatus = CreateWindowExW(0, L"STATIC", L"Holat: FAOL",
                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  24, 85, 500, 24, hWnd, (HMENU)IDC_STATUS_TEXT, NULL, NULL);
    SendMessageW(hStatus, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    HWND hLang = CreateWindowExW(0, L"STATIC", L"Klaviatura: Aniqlanmoqda...",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT,
                                 24, 112, 500, 22, hWnd, (HMENU)IDC_LANG_TEXT, NULL, NULL);
    SendMessageW(hLang, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 3. Live Interactive Typing Test Area
    HWND hTestLbl = CreateWindowExW(0, L"STATIC", L"Tezkor sinov (Bu yerga yozib ko'ring: 'ss' -> 'ş', 'oo' -> 'ö', 'gg' -> 'ğ'):",
                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    24, 145, 500, 20, hWnd, NULL, NULL, NULL);
    SendMessageW(hTestLbl, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                 24, 170, 500, 32, hWnd, (HMENU)IDC_EDIT_TEST, NULL, NULL);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 4. Quick Rule Guide
    const wchar_t *guideText =
        L"Qoidalar:  ss -> ş   |   oo -> ö   |   gg -> ğ   |   cc -> ç\n"
        L"Agar 'ss' (massa, kassa) kerak bo'lsa: 'ss' ketidan [Enter] bosing!\n"
        L"Eski usul ham ishlaydi:  sh -> ş,  ch -> ç,  o' -> ö,  g' -> ğ";
    HWND hGuide = CreateWindowExW(0, L"STATIC", guideText,
                                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  24, 215, 500, 60, hWnd, NULL, NULL, NULL);
    SendMessageW(hGuide, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // 5. Settings Checkboxes
    HWND hChkOnlyUz = CreateWindowExW(0, L"BUTTON",
                                      L"Faqat O'zbek tili (UZB) tanlanganda ishlash (Inglizchaga aralashmaslik)",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      24, 285, 500, 24, hWnd, (HMENU)IDC_CHK_ONLY_UZ, NULL, NULL);
    SendMessageW(hChkOnlyUz, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(hChkOnlyUz, BM_SETCHECK, g_bOnlyUzbekLayout ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hChkSingleC = CreateWindowExW(0, L"BUTTON",
                                       L"Bitta 'C' tugmasi bosilganda to'g'ridan-to'g'ri 'Ç' yozish",
                                       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       24, 315, 500, 24, hWnd, (HMENU)IDC_CHK_SINGLE_C, NULL, NULL);
    SendMessageW(hChkSingleC, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(hChkSingleC, BM_SETCHECK, g_bInstantSingleC ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hChkStartup = CreateWindowExW(0, L"BUTTON",
                                       L"Windows bilan birga avtomatik ishga tushirish (Avtozagruzka)",
                                       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       24, 345, 500, 24, hWnd, (HMENU)IDC_CHK_STARTUP, NULL, NULL);
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
    if (id == IDC_CHK_ONLY_UZ) {
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
      ShowWindow(hWnd, SW_HIDE);
    } else if (id == IDC_BTN_EXIT || id == ID_TRAY_EXIT) {
      PostQuitMessage(0);
    } else if (id == ID_TRAY_OPEN_DASHBOARD) {
      ShowWindow(hWnd, SW_RESTORE);
      SetForegroundWindow(hWnd);
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
    // Closing the window minimizes to tray instead of quitting
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
    // Left-click on UZ badge toggles it on/off (Blue = Active, Gray = Paused)
    if (lParam == WM_LBUTTONUP) {
      g_bEnabled = !g_bEnabled;
      UpdateTrayStatus();
    } else if (lParam == WM_LBUTTONDBLCLK) {
      // Double left-click opens dashboard
      ShowWindow(hWnd, SW_RESTORE);
      SetForegroundWindow(hWnd);
      UpdateDashboardUI();
    } else if (lParam == WM_RBUTTONUP) {
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
    KillTimer(hWnd, TIMER_CHECK_LANG);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR lpCmdLine, int nCmdShow) {
  HANDLE hMutex = CreateMutexW(NULL, TRUE, L"YAY_YangiAlifbodaYoz_Mutex");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    // If already running, find existing window and bring to front
    HWND hExisting = FindWindowW(L"YAYDashboardClass", NULL);
    if (hExisting) {
      ShowWindow(hExisting, SW_RESTORE);
      SetForegroundWindow(hExisting);
    }
    return 0;
  }


  if (!hInstance)
    hInstance = GetModuleHandleW(NULL);

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&icex);

  LoadSettings();

  g_hFont =
      CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  g_hBoldFont =
      CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  g_hTitleFont =
      CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  g_hIconActive = CreateUzbekBadgeIcon(true);
  g_hIconPaused = CreateUzbekBadgeIcon(false);

  WNDCLASSW wcMain = {0};
  wcMain.lpfnWndProc = DashboardWndProc;
  wcMain.hInstance = hInstance;
  wcMain.lpszClassName = L"YAYDashboardClass";
  wcMain.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcMain.hIcon = g_hIconActive;
  ATOM aMain = RegisterClassW(&wcMain);

  WNDCLASSW wcPopup = {0};
  wcPopup.lpfnWndProc = PopupWndProc;
  wcPopup.hInstance = hInstance;
  wcPopup.lpszClassName = L"YAYPopupClass";
  wcPopup.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassW(&wcPopup);

  int winW = 560;
  int winH = 490;
  int scrW = GetSystemMetrics(SM_CXSCREEN);
  int scrH = GetSystemMetrics(SM_CYSCREEN);
  int startX = (scrW - winW) / 2;
  int startY = (scrH - winH) / 2;

  g_hMainWnd = CreateWindowExW(WS_EX_APPWINDOW, L"YAYDashboardClass",
                               L"YAY - Yangi Alifboda Yoz",
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                               startX, startY, winW, winH, NULL, NULL, hInstance, NULL);


  DWORD popStyle = WS_POPUP;
  DWORD popExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
  g_hChoiceWnd =
      CreateWindowExW(popExStyle, L"YAYPopupClass", NULL, popStyle, 0, 0,
                      100, 40, NULL, NULL, hInstance, NULL);
  g_hCoachWnd =
      CreateWindowExW(popExStyle, L"YAYPopupClass", NULL, popStyle, 0, 0,
                      100, 40, NULL, NULL, hInstance, NULL);

  // System Tray Notification Icon
  g_nid.cbSize = sizeof(NOTIFYICONDATAW);
  g_nid.hWnd = g_hMainWnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = g_hIconActive;
  wcscpy_s(g_nid.szTip, 128, L"YAY - Yangi Alifboda Yoz");

  // Show immediate toast notification so Windows highlights the tray icon
  wcscpy_s(g_nid.szInfoTitle, 64, L"YAY - Yangi Alifboda Yoz");
  wcscpy_s(g_nid.szInfo, 256,
           L"Dastur ishga tushdi! Yangi alifboda yozish tayyor.\nKlaviatura: UZB / ENG");
  g_nid.dwInfoFlags = NIIF_INFO;

  Shell_NotifyIconW(NIM_ADD, &g_nid);

  // Periodic language state check (every 250ms) to update UZ badge color and dashboard
  SetTimer(g_hMainWnd, TIMER_CHECK_LANG, 250, NULL);
  UpdateTrayStatus();

  // If started with /tray or /minimized, hide to tray; otherwise show dashboard window
  bool startInTray = (lpCmdLine && (wcsstr(lpCmdLine, L"/tray") != NULL || wcsstr(lpCmdLine, L"/minimized") != NULL));
  if (!startInTray) {
    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);
    UpdateDashboardUI();
  }

  g_hHook =
      SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);


  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

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
  CloseHandle(hMutex);
  return 0;
}

// Fallback entry point when compiled without -municode
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}