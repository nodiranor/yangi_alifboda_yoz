#include "app.h"

static const wchar_t *REG_CONFIG_KEY = L"Software\\YAY_YangiAlifbo";
static const wchar_t *REG_RUN_KEY    = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t *RUN_VALUE_NAME = L"YAY_UzbekKeyboard";

static void ReadBool(HKEY hKey, const wchar_t *name, bool *out) {
  DWORD val = 0, size = sizeof(val), type = 0;
  if (RegQueryValueExW(hKey, name, NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
    *out = (val != 0);
}

static void WriteBool(HKEY hKey, const wchar_t *name, bool value) {
  DWORD val = value ? 1 : 0;
  RegSetValueExW(hKey, name, 0, REG_DWORD, (const BYTE *)&val, sizeof(val));
}

void LoadSettings() {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_CONFIG_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;
  ReadBool(hKey, L"OnlyUzbek", &g_bOnlyUzbekLayout);
  ReadBool(hKey, L"SingleC", &g_bInstantSingleC);
  ReadBool(hKey, L"ShowTips", &g_bShowTips);
  RegCloseKey(hKey);
}

void SaveSettings() {
  HKEY hKey;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_CONFIG_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) return;
  WriteBool(hKey, L"OnlyUzbek", g_bOnlyUzbekLayout);
  WriteBool(hKey, L"SingleC", g_bInstantSingleC);
  WriteBool(hKey, L"ShowTips", g_bShowTips);
  RegCloseKey(hKey);
}

bool IsInStartup() {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return false;
  DWORD type = 0, size = 0;
  bool exists = (RegQueryValueExW(hKey, RUN_VALUE_NAME, NULL, &type, NULL, &size) == ERROR_SUCCESS);
  RegCloseKey(hKey);
  return exists;
}

void ToggleStartup(bool enable) {
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) return;
  if (!enable) {
    RegDeleteValueW(hKey, RUN_VALUE_NAME);
  } else {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t cmd[MAX_PATH + 16];
    wsprintfW(cmd, L"\"%s\" /tray", path);
    RegSetValueExW(hKey, RUN_VALUE_NAME, 0, REG_SZ, (const BYTE *)cmd, (DWORD)((lstrlenW(cmd) + 1) * sizeof(wchar_t)));
  }
  RegCloseKey(hKey);
}
