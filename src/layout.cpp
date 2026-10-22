#include "app.h"

// Decides which alphabet applies to keys going to the foreground window.
//   Uzbek Latin (0x0443)                 -> Latin rules
//   English, unless strict mode is on    -> Latin rules
//   Uzbek Cyrillic (0x0843)              -> Cyrillic to Latin conversion
//   Russian, only if the option is on    -> Cyrillic to Latin conversion
// Every other layout is left alone.
ActiveProfile GetActiveProfile(HKL *outLayout) {
  if (outLayout) *outLayout = NULL;
  if (!g_bEnabled) return AP_NONE;
  HWND hFg = GetForegroundWindow();
  DWORD tid = hFg ? GetWindowThreadProcessId(hFg, NULL) : 0;
  HKL hkl = GetKeyboardLayout(tid);
  if (outLayout) *outLayout = hkl;
  WORD langId = LOWORD((DWORD_PTR)hkl);
  WORD primary = PRIMARYLANGID(langId);
  WORD sub = SUBLANGID(langId);
  if (langId == 0x0443 || (primary == LANG_UZBEK && sub == SUBLANG_UZBEK_LATIN)) return AP_LATIN;
  if (langId == 0x0843 || (primary == LANG_UZBEK && sub == SUBLANG_UZBEK_CYRILLIC)) return AP_CYRILLIC;
  if (primary == LANG_RUSSIAN) return g_bCyrillicOnRussian ? AP_CYRILLIC : AP_NONE;
  if (!g_bOnlyUzbekLayout && primary == LANG_ENGLISH) return AP_LATIN;
  return AP_NONE;
}

bool IsUzbekInputMethodActive() {
  return GetActiveProfile(NULL) != AP_NONE;
}

bool IsUzbekLayoutInstalled() {
  int count = GetKeyboardLayoutList(0, NULL);
  if (count <= 0) return false;
  HKL *hkls = (HKL *)HeapAlloc(GetProcessHeap(), 0, sizeof(HKL) * count);
  if (!hkls) return false;
  GetKeyboardLayoutList(count, hkls);
  bool found = false;
  for (int i = 0; i < count && !found; i++) {
    WORD langId = LOWORD((DWORD_PTR)hkls[i]);
    if (langId == 0x0443 || (PRIMARYLANGID(langId) == LANG_UZBEK && SUBLANGID(langId) == SUBLANG_UZBEK_LATIN)) found = true;
  }
  HeapFree(GetProcessHeap(), 0, hkls);
  return found;
}

// RtlGetVersion never lies, regardless of compatibility shims or manifest state.
bool IsWindows8OrGreaterReal() {
  typedef LONG(WINAPI * RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
  HMODULE hNt = GetModuleHandleW(L"ntdll.dll");
  RtlGetVersionFn fn = hNt ? (RtlGetVersionFn)GetProcAddress(hNt, "RtlGetVersion") : NULL;
  RTL_OSVERSIONINFOW vi = {0};
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (fn && fn(&vi) == 0)
    return vi.dwMajorVersion > 6 || (vi.dwMajorVersion == 6 && vi.dwMinorVersion >= 2);
  return true;
}

// Windows 8+: add uz-Latn-UZ through PowerShell, hidden, on a worker thread.
// The encoded command is UTF-16 base64 of:
//   $langs = Get-WinUserLanguageList; if (-not ($langs | Where-Object { $_.LanguageTag -like 'uz*' }))
//   { $uz = New-WinUserLanguageList 'uz-Latn-UZ'; $langs.Add($uz[0]); Set-WinUserLanguageList $langs -Force }
static DWORD WINAPI InstallUzbekThreadProc(LPVOID) {
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
  if (g_hMainWnd) PostMessageW(g_hMainWnd, WM_LAYOUT_INSTALL_DONE, 0, 0);
  return 0;
}

void TriggerAddUzbekLayout() {
  HANDLE h = CreateThread(NULL, 0, InstallUzbekThreadProc, NULL, 0, NULL);
  if (h) CloseHandle(h);
}

static int ParseInt(const wchar_t *s) {
  int n = 0;
  while (*s >= L'0' && *s <= L'9') n = n * 10 + (*s++ - L'0');
  return n;
}

static bool SetStringValue(HKEY hKey, const wchar_t *name, const wchar_t *value) {
  return RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE *)value,
                        (DWORD)((lstrlenW(value) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

// Windows 7: the language list is HKCU\Keyboard Layout\Preload (values "1", "2", ...).
// Uzbek Latin has no keyboard DLL of its own, so Windows pairs it with the US layout
// through HKCU\Keyboard Layout\Substitutes. Both take effect at the next sign-in.
bool AddUzbekPreloadWin7() {
  HKEY hPre;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, NULL, 0,
                      KEY_READ | KEY_WRITE, NULL, &hPre, NULL) != ERROR_SUCCESS)
    return false;

  int maxIdx = 0;
  bool exists = false;
  for (DWORD i = 0;; i++) {
    wchar_t name[32], data[64];
    DWORD nameLen = 32, dataLen = sizeof(data), type = 0;
    LONG r = RegEnumValueW(hPre, i, name, &nameLen, NULL, &type, (LPBYTE)data, &dataLen);
    if (r == ERROR_MORE_DATA) continue;
    if (r != ERROR_SUCCESS) break;
    int idx = ParseInt(name);
    if (idx > maxIdx) maxIdx = idx;
    if (type == REG_SZ && lstrcmpiW(data, L"00000443") == 0) exists = true;
  }
  bool ok = true;
  if (!exists) {
    wchar_t name[16];
    wsprintfW(name, L"%d", maxIdx + 1);
    ok = SetStringValue(hPre, name, L"00000443");
  }
  RegCloseKey(hPre);
  if (!ok) return false;

  HKEY hSub;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0, NULL, 0,
                      KEY_WRITE, NULL, &hSub, NULL) != ERROR_SUCCESS)
    return false;
  ok = SetStringValue(hSub, L"00000443", L"00000409");
  RegCloseKey(hSub);
  return ok;
}
