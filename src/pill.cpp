// pill.cpp - the small hint popup ("the pill").
//
// After someone types the old form (sh, ch, o', g') a small dark rounded label appears near the
// text cursor and says that the double tap is faster ("Maslahat: 'ss' tezroq"). It disappears on
// the next key press or after a few seconds. It is the only window in YAY that we paint ourselves,
// because it has to be small, borderless and rounded.
//
// The popup must never get in the way of typing, so it is created with:
//   WS_EX_NOACTIVATE  never takes the keyboard focus, even when shown
//   WS_EX_TOOLWINDOW  no taskbar button and not listed in Alt+Tab
//   WS_EX_TOPMOST     stays above the window being typed into
#include "app.h"

// The popup window, its font and the text it currently shows.
static HWND    g_hCoachWnd = NULL;
static HFONT   g_hPillFont = NULL;
static wchar_t g_coachText[128] = {0};

// Paints the pill: a dark fill, a thin amber border (a rounded rectangle) and the centred text.
// Only WM_PAINT is handled; everything else takes the default path.
static LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg != WM_PAINT) return DefWindowProcW(hWnd, msg, wParam, lParam);

  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);
  RECT rc;
  GetClientRect(hWnd, &rc);

  HBRUSH hBrush = CreateSolidBrush(RGB(28, 25, 20));
  FillRect(hdc, &rc, hBrush);
  DeleteObject(hBrush);

  HPEN hPen = CreatePen(PS_SOLID, 1, RGB(234, 179, 8));
  HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
  SelectObject(hdc, GetStockObject(NULL_BRUSH));
  RoundRect(hdc, 0, 0, rc.right, rc.bottom, 14, 14);
  SelectObject(hdc, hOldPen);
  DeleteObject(hPen);

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(254, 243, 199));
  HFONT hOldFont = (HFONT)SelectObject(hdc, g_hPillFont);
  RECT textRc = {10, 2, rc.right - 10, rc.bottom - 2};
  DrawTextW(hdc, g_coachText, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(hdc, hOldFont);

  EndPaint(hWnd, &ps);
  return 0;
}

// Registers the window class, creates the font and the (still hidden) popup window.
// The font is sized from the screen DPI: 10 points, converted to pixels.
bool PillCreate() {
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = PopupWndProc;
  wc.hInstance = g_hInst;
  wc.lpszClassName = L"YAYPopupClass";
  wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
  if (!RegisterClassW(&wc)) return false;

  HDC hdc = GetDC(NULL);
  int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
  ReleaseDC(NULL, hdc);
  g_hPillFont = CreateFontW(-MulDiv(10, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  g_hCoachWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"YAYPopupClass",
                                NULL, WS_POPUP, 0, 0, 100, 30, NULL, NULL, g_hInst, NULL);
  return g_hCoachWnd != NULL;
}

void PillDestroy() {
  if (g_hCoachWnd) { DestroyWindow(g_hCoachWnd); g_hCoachWnd = NULL; }
  if (g_hPillFont) { DeleteObject(g_hPillFont); g_hPillFont = NULL; }
}

// Hides the popup and cancels the timer that would hide it later. Called on every key press
// and when the timer fires.
void HidePill() {
  if (g_hMainWnd) KillTimer(g_hMainWnd, TIMER_COACH);
  if (g_hCoachWnd && IsWindowVisible(g_hCoachWnd)) ShowWindow(g_hCoachWnd, SW_HIDE);
}

// Shows `text` in the pill, near the place the user is typing.
//
// Where: just below the text cursor (the "caret") of the foreground window. Only classic Windows
// edit controls expose their caret to other programs; browsers, Word and Electron apps draw their
// own, so there we fall back to the top-right corner of the window, where the pill cannot cover
// the text. With no usable window at all it goes to the bottom-right of the screen, by the tray.
// The pill is kept inside the screen and clear of the taskbar (the "55" and "12" below are pixel
// margins).
static void ShowFloatingPill(const wchar_t *text) {
  if (!g_hCoachWnd) return;
  lstrcpynW(g_coachText, text, 128);

  // Measure the text to size the pill (DT_CALCRECT only measures, it draws nothing).
  HDC hdc = GetDC(g_hCoachWnd);
  HFONT hOld = (HFONT)SelectObject(hdc, g_hPillFont);
  RECT rc = {0, 0, 800, 0};
  DrawTextW(hdc, g_coachText, -1, &rc, DT_CALCRECT | DT_LEFT | DT_NOPREFIX);
  SelectObject(hdc, hOld);
  ReleaseDC(g_hCoachWnd, hdc);

  int width = (rc.right - rc.left) + 26;
  int height = (rc.bottom - rc.top) + 12;
  int screenW = GetSystemMetrics(SM_CXSCREEN);
  int screenH = GetSystemMetrics(SM_CYSCREEN);
  int posX = 0, posY = 0;

  // Ask the foreground window's thread where its caret is, and turn that into screen coordinates.
  GUITHREADINFO gti = {0};
  gti.cbSize = sizeof(gti);
  HWND hFg = GetForegroundWindow();
  bool hasCaret = false;
  POINT pt = {0};
  if (hFg) {
    DWORD tid = GetWindowThreadProcessId(hFg, NULL);
    if (GetGUIThreadInfo(tid, &gti) && gti.hwndCaret && IsWindow(gti.hwndCaret)) {
      pt.x = gti.rcCaret.left;
      pt.y = gti.rcCaret.bottom;
      ClientToScreen(gti.hwndCaret, &pt);
      hasCaret = (pt.x > 0 && pt.x < screenW && pt.y > 0 && pt.y < screenH);
    }
  }

  if (hasCaret) {
    posX = pt.x + 8;
    posY = pt.y + 18;
    if (posY + height > screenH - 55) posY = pt.y - height - (gti.rcCaret.bottom - gti.rcCaret.top) - 8;
    if (posX + width > screenW - 12) posX = screenW - width - 12;
    if (posX < 12) posX = 12;
  } else {
    RECT rf = {0};
    if (hFg && GetWindowRect(hFg, &rf) && (rf.right - rf.left > 200) && (rf.bottom - rf.top > 150)) {
      posX = rf.right - width - 24;
      posY = rf.top + 38;
      if (posX < rf.left + 12) posX = rf.left + 12;
      if (posY < 12) posY = 12;
    } else {
      posX = screenW - width - 24;
      posY = screenH - height - 55;
    }
  }

  // Clip the window to a rounded shape (the system owns the region after SetWindowRgn), then show
  // it without stealing focus, and start the timer that hides it again.
  HRGN hRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, 14, 14);
  SetWindowRgn(g_hCoachWnd, hRgn, TRUE);
  SetWindowPos(g_hCoachWnd, HWND_TOPMOST, posX, posY, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  InvalidateRect(g_hCoachWnd, NULL, TRUE);
  if (g_hMainWnd) SetTimer(g_hMainWnd, TIMER_COACH, DURATION_COACH_MS, NULL);
}

// The hint shown after a legacy form. `doubleTapHint` is the faster double tap, for example "ss".
void ShowCoachingTip(const wchar_t *doubleTapHint) {
  wchar_t msg[128];
  wsprintfW(msg, L"Maslahat: '%s' tezroq", doubleTapHint);
  ShowFloatingPill(msg);
}
