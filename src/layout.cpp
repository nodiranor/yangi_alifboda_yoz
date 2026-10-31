// layout.cpp - keyboard layouts: which one is active, and adding the Uzbek one to Windows.
//
// A keyboard layout is identified by an HKL handle. Its low 16 bits are the language ID:
//   0x0409 English (US)       0x0419 Russian
//   0x0443 Uzbek (Latin)      0x0843 Uzbek (Cyrillic)
// Windows keeps a separate layout for every window thread, so we always ask about the thread of
// the foreground window, not about "the" system layout.
#include "app.h"

// Decides which converter applies to keys going to the foreground window.
//
//   Uzbek Latin (0x0443)                 -> Latin rules
//   English, unless strict mode is on    -> Latin rules (Uzbek can then be typed on a plain US keyboard)
//   Uzbek Cyrillic (0x0843)              -> Cyrillic to Latin conversion
//   Russian, only if the option is on    -> Cyrillic to Latin conversion
//
// "Strict mode" is g_bOnlyUzbekLayout, on by default. Every other layout is left alone, and
// nothing applies while YAY is paused. This runs for every key press, so it only makes a few
// cheap system calls.
ActiveProfile GetActiveProfile(HKL *outLayout) {
  if (outLayout) *outLayout = NULL;
  if (!g_bEnabled) return AP_NONE;
  // The layout belongs to the thread that owns the foreground window.
  HWND hFg = GetForegroundWindow();
  DWORD tid = hFg ? GetWindowThreadProcessId(hFg, NULL) : 0;
  HKL hkl = GetKeyboardLayout(tid);
  if (outLayout) *outLayout = hkl;
  // Split the language ID into its primary language (Uzbek, Russian...) and sublanguage
  // (Latin or Cyrillic script for Uzbek). The numeric tests catch the same layouts directly.
  WORD langId = LOWORD((DWORD_PTR)hkl);
  WORD primary = PRIMARYLANGID(langId);
  WORD sub = SUBLANGID(langId);
  if (langId == 0x0443 || (primary == LANG_UZBEK && sub == SUBLANG_UZBEK_LATIN)) return AP_LATIN;
  if (langId == 0x0843 || (primary == LANG_UZBEK && sub == SUBLANG_UZBEK_CYRILLIC)) return AP_CYRILLIC;
  if (primary == LANG_RUSSIAN) return g_bCyrillicOnRussian ? AP_CYRILLIC : AP_NONE;
  if (!g_bOnlyUzbekLayout && primary == LANG_ENGLISH) return AP_LATIN;
  return AP_NONE;
}

// Is the Uzbek (Latin) layout in the user's list of layouts? It decides whether the window
// offers the "add layout" button, and whether start-up should add it.
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

// True on Windows 8 and later. The two install paths differ: Windows 8 and later add the layout
// with InstallLayoutOrTip (see AddUzbekLayout), Windows 7 through the registry (see AddUzbekPreloadWin7).
// RtlGetVersion always reports the real version, even where a compatibility mode or a missing
// manifest entry would make GetVersionEx report an older one. ntdll.dll exports it, but the
// normal Windows headers do not declare it, so it is looked up at run time.
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

// Windows 8 and later: adds the Uzbek (Latin) layout to the user's language list, and nothing else.
//
// The list belongs to the user and may hold input methods YAY knows nothing about, such as the
// Chinese ones from Google or Sogou. So exactly one entry is added, with InstallLayoutOrTip, which
// enables only the entries it is given. Do not read the whole list and write it back (for example
// with PowerShell's Get-/Set-WinUserLanguageList): Windows then rewrites every entry from the copy
// that was read, and an input method missing from that copy is lost.
//
// The entry "0443:00000409" is the Uzbek (Latin) language with the US keyboard, the same entry the
// Settings app adds. input.dll has no import library, so it is loaded at run time, by its full
// system path so that no other DLL of that name can be picked up. Returns true on success.
//
// Windows can record the entry in the language list and still not load it into the running
// session. Then the layout is missing from the tray until the next sign-in. This happens when an
// older install left some of its entries behind (HKCU\Keyboard Layout\Preload and Substitutes).
// Adding the entry a second time does not help, but removing this one entry and adding it again does,
// so that is done when the layout has not appeared. Other entries are never touched.
static const wchar_t UZBEK_LATIN_ENTRY[] = L"0x0443:0x00000409";
#define ILOT_UNINSTALL 0x00000001  // InstallLayoutOrTip flag; it is not in any header

// Waits up to `ms` milliseconds for the Uzbek (Latin) layout to show up in this session.
static bool WaitForUzbekLayout(int ms) {
  for (int waited = 0;; waited += 100) {
    if (IsUzbekLayoutInstalled()) return true;
    if (waited >= ms) return false;
    Sleep(100);
  }
}

bool AddUzbekLayout() {
  if (IsUzbekLayoutInstalled()) return true;  // already usable: leave the user's list alone
  wchar_t path[MAX_PATH];
  UINT n = GetSystemDirectoryW(path, MAX_PATH);
  if (n == 0 || n + 11 > MAX_PATH) return false;  // 11 = "\input.dll" (10) and the end mark
  lstrcatW(path, L"\\input.dll");
  HMODULE hInput = LoadLibraryW(path);
  if (!hInput) return false;
  typedef BOOL(WINAPI * InstallLayoutOrTipFn)(LPCWSTR, DWORD);
  InstallLayoutOrTipFn fn = (InstallLayoutOrTipFn)GetProcAddress(hInput, "InstallLayoutOrTip");
  BOOL ok = FALSE;
  if (fn) {
    // Flags 0: enable the entry now. Never pass ILOT_CLEANINSTALL (0x40): it disables all the other layouts.
    ok = fn(UZBEK_LATIN_ENTRY, 0);
    if (ok && !WaitForUzbekLayout(1500)) {
      fn(UZBEK_LATIN_ENTRY, ILOT_UNINSTALL);
      ok = fn(UZBEK_LATIN_ENTRY, 0);
    }
  }
  FreeLibrary(hInput);
  return ok != FALSE;
}

// The worker thread for TriggerAddUzbekLayout. It is a thread so that the window does not freeze
// while Windows registers the layout. When it ends it posts WM_LAYOUT_INSTALL_DONE, so the window
// can refresh itself.
static DWORD WINAPI InstallUzbekThreadProc(LPVOID) {
  AddUzbekLayout();
  if (g_hMainWnd) PostMessageW(g_hMainWnd, WM_LAYOUT_INSTALL_DONE, 0, 0);
  return 0;
}

// Starts InstallUzbekThreadProc in the background and returns at once.
void TriggerAddUzbekLayout() {
  HANDLE h = CreateThread(NULL, 0, InstallUzbekThreadProc, NULL, 0, NULL);
  if (h) CloseHandle(h);
}

// Reads the leading decimal digits of `s`. No sign and no error handling, because the only input
// is the numeric value names ("1", "2", ...) of the Preload registry key below.
static int ParseInt(const wchar_t *s) {
  int n = 0;
  while (*s >= L'0' && *s <= L'9') n = n * 10 + (*s++ - L'0');
  return n;
}

// Writes a REG_SZ (text) registry value. Returns true on success.
static bool SetStringValue(HKEY hKey, const wchar_t *name, const wchar_t *value) {
  return RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE *)value,
                        (DWORD)((lstrlenW(value) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

// Windows 7 fallback. Windows 7 has no command for adding a language, so we write what the
// Region and Language control panel would write. The user then signs out and in once.
//
//   HKCU\Keyboard Layout\Preload      numbered list of the user's layouts: "1" = 00000409,
//                                     "2" = 00000419, ... We append "00000443" (Uzbek Latin)
//                                     as the next number, unless it is already there.
//   HKCU\Keyboard Layout\Substitutes  Uzbek Latin has no keyboard file of its own, so Windows
//                                     pairs it with the US layout. We add 00000443 -> 00000409.
//
// Both take effect at the next sign-in; the dashboard says so. Returns false if the registry
// could not be written.
bool AddUzbekPreloadWin7() {
  HKEY hPre;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, NULL, 0,
                      KEY_READ | KEY_WRITE, NULL, &hPre, NULL) != ERROR_SUCCESS)
    return false;

  // Find the highest number in use and whether Uzbek Latin is already listed.
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
  // Add it as the next number, unless it is already in the list.
  bool ok = true;
  if (!exists) {
    wchar_t name[16];
    wsprintfW(name, L"%d", maxIdx + 1);
    ok = SetStringValue(hPre, name, L"00000443");
  }
  RegCloseKey(hPre);
  if (!ok) return false;

  // Tell Windows which keyboard to use for it: the US layout.
  HKEY hSub;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0, NULL, 0,
                      KEY_WRITE, NULL, &hSub, NULL) != ERROR_SUCCESS)
    return false;
  ok = SetStringValue(hSub, L"00000443", L"00000409");
  RegCloseKey(hSub);
  return ok;
}
