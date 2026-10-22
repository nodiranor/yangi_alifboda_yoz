// main.cpp - the program entry point and the global state.
//
// Start-up, in order: read the command line, make sure no other copy is running, load the
// settings, create the window, the tray icon and the hint popup, install the keyboard hook, then
// run the message loop until the user chooses Exit. Everything after that is event-driven; with
// the window hidden YAY uses no CPU.
#include "app.h"

// Shared strings, declared in app.h.
const wchar_t *APP_NAME        = L"YAY - Yangi Alifboda Yoz";
const wchar_t *DASHBOARD_CLASS = L"YAYDashboardClass";
const wchar_t *SPONSOR_URL     = L"https://t.me/juraboshibot";

// Global state, declared in app.h where every variable is explained. The values here are the
// defaults for a first run; LoadSettings() replaces the saved options.
HINSTANCE   g_hInst             = NULL;
HHOOK       g_hHook             = NULL;
HWND        g_hMainWnd          = NULL;
HICON       g_hIconActive       = NULL;
HICON       g_hIconPaused       = NULL;
HICON       g_hIconBig          = NULL;
bool        g_bEnabled          = true;
bool        g_bOnlyUzbekLayout  = true;
bool        g_bInstantSingleC   = false;
bool        g_bShowTips         = true;
bool        g_bCyrillicOnRussian = false;
bool        g_bSendingInput     = false;
bool        g_bWin7PreloadWritten = false;
EngineState g_engine            = {ER_NONE, 0, {0, 0}, 0, DOUBLE_TAP_MS};
TranslitState g_translit        = {{0}, 0};

// Is `needle` somewhere inside `hay`? A minimal wcsstr: the C library is not linked (nocrt.cpp).
static bool ContainsW(const wchar_t *hay, const wchar_t *needle) {
  for (; *hay; hay++) {
    const wchar_t *h = hay, *n = needle;
    while (*n && *h == *n) { h++; n++; }
    if (!*n) return true;
  }
  return false;
}

// Loads an icon from the exe's resources at exactly `size` pixels. Asking for the size the
// system wants (small icon for the tray, large for the title bar) keeps it sharp at any DPI.
static HICON LoadAppIcon(int resId, int size) {
  return (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(resId), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
}

// The program proper. Returns the process exit code.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
  g_hInst = hInstance;
  // "/tray" (used by the start-with-Windows entry) means: start hidden in the tray, show no window.
  const wchar_t *cmd = GetCommandLineW();
  const bool startInTray = cmd && (ContainsW(cmd, L"/tray") || ContainsW(cmd, L"/minimized"));

  // A named mutex is how we notice another copy: creating it a second time sets
  // ERROR_ALREADY_EXISTS.
  // Single instance: a second launch brings the existing dashboard to the front and exits.
  HANDLE hMutex = CreateMutexW(NULL, TRUE, L"YAY_YangiAlifbodaYoz_Mutex");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND hExisting = FindWindowW(DASHBOARD_CLASS, NULL);
    if (!hExisting) { Sleep(150); hExisting = FindWindowW(DASHBOARD_CLASS, NULL); }
    if (hExisting) {
      if (!startInTray) {
        AllowSetForegroundWindow(ASFW_ANY);
        SendMessageW(hExisting, WM_COMMAND, MAKEWPARAM(ID_TRAY_OPEN_DASHBOARD, 0), 0);
        SetForegroundWindow(hExisting);
      }
      CloseHandle(hMutex);
      return 0;
    }
    // Mutex exists but no window: an orphaned instance. Carry on as the live one.
  }

  // Saved options from the registry. With none saved yet, the defaults above stay.
  LoadSettings();
// tests/e2e.cmd builds a special YAY_test.exe with this defined. It also accepts keys that other
// programs inject (see hook.cpp) and works on the English and Russian layouts, so the end-to-end
// test needs no particular keyboard setup. Never defined in the real build.
#ifdef YAY_ACCEPT_INJECTED
  g_bOnlyUzbekLayout = false;   // test build: work on the English layout
  g_bCyrillicOnRussian = true;  // test build: also work on the Russian layout
#endif

  // Registers the SysLink control used for the sponsor link. (The manifest, res/yay.manifest,
  // is what turns on the modern look of the buttons and checkboxes.)
  INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES | ICC_LINK_CLASS };
  InitCommonControlsEx(&icex);

  // Icons: small for the tray, a grey one for "paused", large for the window and Alt+Tab.
  g_hIconActive = LoadAppIcon(IDI_APPICON, GetSystemMetrics(SM_CXSMICON));
  g_hIconPaused = LoadAppIcon(IDI_PAUSEDICON, GetSystemMetrics(SM_CXSMICON));
  g_hIconBig    = LoadAppIcon(IDI_APPICON, GetSystemMetrics(SM_CXICON));

  // Register the window class named in the dialog template (res/yay.rc). DefDlgProcW is the
  // standard dialog window procedure; it passes messages on to DashboardDlgProc. Giving the
  // class a name of our own is what lets a second launch find this window with FindWindow.
  WNDCLASSEXW wc = {0};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = DefDlgProcW;
  wc.cbWndExtra = DLGWINDOWEXTRA;
  wc.hInstance = hInstance;
  wc.hIcon = g_hIconBig;
  wc.hIconSm = g_hIconActive;
  wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
  wc.lpszClassName = DASHBOARD_CLASS;
  if (!RegisterClassExW(&wc)) {
    MessageBoxW(NULL, L"Oyna sinfini röyxatdan ötkazib bölmadi.", APP_NAME, MB_ICONERROR);
    return 1;
  }

  // The hint popup, created hidden. If it cannot be created, YAY still works, just without hints.
  PillCreate();

  // Build the dashboard from the dialog template. It stays hidden until ShowDashboard() runs.
  HWND hDlg = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), NULL, DashboardDlgProc, 0);
  if (!hDlg) {
    MessageBoxW(NULL, L"Asosiy oynani yaratib bölmadi.", APP_NAME, MB_ICONERROR);
    return 1;
  }
  g_hMainWnd = hDlg;

  // Tray icon first. Then, on Windows 8 and later, add the Uzbek (Latin) layout in the background
  // if the user does not have it. On Windows 7 this needs a sign-out, so the user starts it
  // from the window instead.
  TrayAdd();

  if (IsWindows8OrGreaterReal() && !IsUzbekLayoutInstalled())
    TriggerAddUzbekLayout();

  // Install the hook last, when everything it uses exists. It stays until the program exits.
  // A low-level hook is global (thread id 0) and its code lives in this exe, hence hInstance.
  EngineInit(&g_engine, DOUBLE_TAP_MS);
  g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

  // Show the right icon for the current layout, and the window unless we started with /tray.
  UpdateTrayStatus();
  if (!startInTray) ShowDashboard();

  // The message loop. It must keep running: the keyboard hook is only called while this thread
  // is pumping messages. IsDialogMessageW gives the dashboard the usual dialog keys (Tab,
  // Enter, Esc, Alt+letter). GetMessage returns 0 after PostQuitMessage (see WM_DESTROY in
  // dashboard.cpp) and the program shuts down.
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    if (g_hMainWnd && IsDialogMessageW(g_hMainWnd, &msg)) continue;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  // Clean up. The system would do this anyway at exit; it keeps the code honest and tidy.
  if (g_hHook) UnhookWindowsHookEx(g_hHook);
  PillDestroy();
  if (g_hIconActive) DestroyIcon(g_hIconActive);
  if (g_hIconPaused) DestroyIcon(g_hIconPaused);
  if (g_hIconBig) DestroyIcon(g_hIconBig);
  if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
  return 0;
}

// Process entry point. The exe is linked without the C runtime (see build.cmd, /ENTRY:YayStartup),
// so Windows jumps here instead of into the usual CRT start-up code. We call wWinMain ourselves
// with the arguments the CRT would have prepared, and end the process with its return value.
#ifdef _MSC_VER
extern "C" void YayStartup() {
  ExitProcess((UINT)wWinMain(GetModuleHandleW(NULL), NULL, GetCommandLineW(), SW_SHOWDEFAULT));
}
#endif
