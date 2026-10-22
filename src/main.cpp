#include "app.h"

const wchar_t *APP_NAME        = L"YAY - Yangi Alifboda Yoz";
const wchar_t *DASHBOARD_CLASS = L"YAYDashboardClass";
const wchar_t *SPONSOR_URL     = L"https://t.me/juraboshibot";

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

static bool ContainsW(const wchar_t *hay, const wchar_t *needle) {
  for (; *hay; hay++) {
    const wchar_t *h = hay, *n = needle;
    while (*n && *h == *n) { h++; n++; }
    if (!*n) return true;
  }
  return false;
}

static HICON LoadAppIcon(int resId, int size) {
  return (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(resId), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
  g_hInst = hInstance;
  const wchar_t *cmd = GetCommandLineW();
  const bool startInTray = cmd && (ContainsW(cmd, L"/tray") || ContainsW(cmd, L"/minimized"));

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

  LoadSettings();
#ifdef YAY_ACCEPT_INJECTED
  g_bOnlyUzbekLayout = false;   // test build: work on the English layout
  g_bCyrillicOnRussian = true;  // test build: also work on the Russian layout
#endif

  INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES | ICC_LINK_CLASS };
  InitCommonControlsEx(&icex);

  g_hIconActive = LoadAppIcon(IDI_APPICON, GetSystemMetrics(SM_CXSMICON));
  g_hIconPaused = LoadAppIcon(IDI_PAUSEDICON, GetSystemMetrics(SM_CXSMICON));
  g_hIconBig    = LoadAppIcon(IDI_APPICON, GetSystemMetrics(SM_CXICON));

  // The dialog template names this class so FindWindow can locate the dashboard.
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

  PillCreate();

  HWND hDlg = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), NULL, DashboardDlgProc, 0);
  if (!hDlg) {
    MessageBoxW(NULL, L"Asosiy oynani yaratib bölmadi.", APP_NAME, MB_ICONERROR);
    return 1;
  }
  g_hMainWnd = hDlg;

  TrayAdd();

  if (IsWindows8OrGreaterReal() && !IsUzbekLayoutInstalled())
    TriggerAddUzbekLayout();

  EngineInit(&g_engine, DOUBLE_TAP_MS);
  g_hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

  UpdateTrayStatus();
  if (!startInTray) ShowDashboard();

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    if (g_hMainWnd && IsDialogMessageW(g_hMainWnd, &msg)) continue;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (g_hHook) UnhookWindowsHookEx(g_hHook);
  PillDestroy();
  if (g_hIconActive) DestroyIcon(g_hIconActive);
  if (g_hIconPaused) DestroyIcon(g_hIconPaused);
  if (g_hIconBig) DestroyIcon(g_hIconBig);
  if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
  return 0;
}

// Process entry point. The exe links without the C runtime (see build.cmd), so the loader
// jumps here instead of the CRT startup code.
#ifdef _MSC_VER
extern "C" void YayStartup() {
  ExitProcess((UINT)wWinMain(GetModuleHandleW(NULL), NULL, GetCommandLineW(), SW_SHOWDEFAULT));
}
#endif
