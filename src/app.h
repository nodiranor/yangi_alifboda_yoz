#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0800
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>



#include "../res/resource.h"
#include "engine.h"
#include "keymap.h"

// Private window messages. WM_USER+n is reserved by the dialog manager (DM_SETDEFID), so use WM_APP.
#define WM_TRAYICON            (WM_APP + 1)
#define WM_LAYOUT_INSTALL_DONE (WM_APP + 2)

// Timers (all on the dashboard window)
#define TIMER_COACH            2002
#define TIMER_DASH_REFRESH     2004

#define DURATION_COACH_MS      4000
#define DOUBLE_TAP_MS          350

// dwExtraInfo value stamped on every key YAY injects, so it can recognise its own output.
#define YAY_EXTRA_INFO         ((ULONG_PTR)0x59415921)

extern const wchar_t *APP_NAME;
extern const wchar_t *DASHBOARD_CLASS;
extern const wchar_t *SPONSOR_URL;

// Globals, defined in main.cpp
extern HINSTANCE   g_hInst;
extern HHOOK       g_hHook;
extern HWND        g_hMainWnd;
extern HICON       g_hIconActive;
extern HICON       g_hIconPaused;
extern HICON       g_hIconBig;
extern bool        g_bEnabled;
extern bool        g_bOnlyUzbekLayout;
extern bool        g_bInstantSingleC;
extern bool        g_bShowTips;
extern bool        g_bCyrillicOnRussian;  // also act on the Russian layout (Cyrillic profile)
extern bool        g_bSendingInput;
extern bool        g_bWin7PreloadWritten;
extern EngineState g_engine;

// settings.cpp
void LoadSettings();
void SaveSettings();
bool IsInStartup();
void ToggleStartup(bool enable);

// inject.cpp
void SendBackspaceAndChars(int backspaces, const wchar_t *chars);
bool IsCapsActive();

// layout.cpp
// What YAY does with keys going to the foreground window right now.
enum ActiveProfile { AP_NONE, AP_LATIN, AP_CYRILLIC };
ActiveProfile GetActiveProfile(HKL *outLayout);
bool IsUzbekInputMethodActive();  // true unless GetActiveProfile() is AP_NONE
bool IsUzbekLayoutInstalled();
bool IsWindows8OrGreaterReal();
void TriggerAddUzbekLayout();
bool AddUzbekPreloadWin7();

// tray.cpp
void TrayAdd();
void TrayRemove();
void UpdateTrayStatus();
void TrayShowMenu(HWND hWnd);

// pill.cpp
bool PillCreate();
void PillDestroy();
void ShowCoachingTip(const wchar_t *doubleTapHint);
void HidePill();

// dashboard.cpp
INT_PTR CALLBACK DashboardDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateDashboardUI();
void ShowDashboard();

// hook.cpp
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
