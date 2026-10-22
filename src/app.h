// app.h - declarations shared by the Win32 source files of YAY.
//
// How the program is organised (CONTRIBUTING.md has the same map):
//
//   engine.*      Latin typing rules: double tap, triple tap, sh/ch/o'/g'. Pure logic, no Win32.
//   translit.*    Cyrillic to Latin conversion, one letter at a time. Pure logic, no Win32.
//   keymap.*      Asks Windows which Cyrillic letter a key produces on the active layout.
//   hook.cpp      The low-level keyboard hook. Decides what to do with every key press.
//   inject.cpp    Sends replacement text to the focused application with SendInput.
//   layout.cpp    Which keyboard layout is active, and adding the Uzbek layout to Windows.
//   settings.cpp  Settings and the "start with Windows" entry, both in the registry.
//   dashboard.cpp The main window (a dialog from res/yay.rc).
//   tray.cpp      The notification-area icon and its menu.
//   pill.cpp      The small hint popup shown after the old sh/ch/o'/g' forms.
//   main.cpp      Entry point, single-instance check, message loop and the global state.
//
// Everything runs on one thread (the one that owns the window and the keyboard hook), so the
// globals below need no locking. The only other thread is the short-lived worker in layout.cpp
// that waits for PowerShell, and it only posts a message back.
#pragma once

// UNICODE makes the Windows headers pick the wide (UTF-16) versions of every API.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// The oldest supported system is Windows 7 (0x0601). The headers then hide anything newer, so
// the compiler stops us from calling an API that Windows 7 does not have. If a newer API is
// really needed, load it at run time with GetProcAddress (see IsWindows8OrGreaterReal).
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
#include "translit.h"

// ---------------------------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------------------------

// Private window messages. The WM_USER range is reserved by the dialog manager (it uses
// WM_USER+0 for DM_GETDEFID and friends), so a dialog must use WM_APP and above.
#define WM_TRAYICON            (WM_APP + 1)  // sent by the tray icon; lParam is the mouse message
#define WM_LAYOUT_INSTALL_DONE (WM_APP + 2)  // posted by the PowerShell worker thread when it ends

// Timer IDs. Both timers belong to the dashboard window, even while it is hidden.
#define TIMER_COACH            2002  // hides the hint popup after DURATION_COACH_MS
#define TIMER_DASH_REFRESH     2004  // refreshes the window text once a second while it is visible

#define DURATION_COACH_MS      4000  // how long the hint popup stays on screen
#define DOUBLE_TAP_MS          350   // longest gap between two taps that still counts as a double tap

// Stamped into dwExtraInfo of every key YAY injects. The hook uses it to recognise its own output
// (see hook.cpp and inject.cpp). The value is the ASCII for "YAY!".
#define YAY_EXTRA_INFO         ((ULONG_PTR)0x59415921)

// Strings that several files need. Defined in main.cpp.
extern const wchar_t *APP_NAME;         // window caption and message box title
extern const wchar_t *DASHBOARD_CLASS;  // window class of the dashboard; also how a second launch finds the first
extern const wchar_t *SPONSOR_URL;      // opened by the sponsor link in the window

// ---------------------------------------------------------------------------------------------
// Global state (defined in main.cpp)
// ---------------------------------------------------------------------------------------------

extern HINSTANCE   g_hInst;
extern HHOOK       g_hHook;             // the low-level keyboard hook; NULL if installing it failed
extern HWND        g_hMainWnd;          // the dashboard window (hidden or visible); NULL once destroyed
extern HICON       g_hIconActive;       // 16 px (system small icon size): tray icon while YAY is acting
extern HICON       g_hIconPaused;       // grey tray icon while YAY is paused or the layout is not ours
extern HICON       g_hIconBig;          // 32 px: window and Alt+Tab icon

// Settings. The first four are stored in the registry (settings.cpp).
extern bool        g_bEnabled;          // master switch; toggled from the tray, not saved
extern bool        g_bOnlyUzbekLayout;  // true: Latin rules only on the Uzbek (Latin) layout; false: English too
extern bool        g_bInstantSingleC;   // a single 'c' key types ç at once
extern bool        g_bShowTips;         // show the hint popup after sh/ch/o'/g'
extern bool        g_bCyrillicOnRussian;  // also convert Cyrillic to Latin on the Russian layout

extern bool        g_bSendingInput;     // true only while SendInput runs, see inject.cpp
extern bool        g_bWin7PreloadWritten;  // Windows 7: layout registry entries written, sign-out pending
extern EngineState g_engine;            // state of the Latin double-tap engine
extern TranslitState g_translit;        // state of the Cyrillic converter

// ---------------------------------------------------------------------------------------------
// Functions, grouped by the file that defines them
// ---------------------------------------------------------------------------------------------

// settings.cpp
void LoadSettings();             // read the saved settings into the globals above
void SaveSettings();             // write them back
bool IsInStartup();              // is YAY registered to start with Windows?
void ToggleStartup(bool enable); // add or remove that registration

// inject.cpp
// Press Backspace `backspaces` times, then type `chars` into the focused application.
void SendBackspaceAndChars(int backspaces, const wchar_t *chars);
// True when letters should currently come out as capitals (Shift XOR Caps Lock).
bool IsCapsActive();

// layout.cpp
// What YAY does with keys going to the foreground window right now.
enum ActiveProfile {
  AP_NONE,      // leave the keys alone
  AP_LATIN,     // apply the Latin double-tap rules (engine.cpp)
  AP_CYRILLIC   // convert Cyrillic letters to Latin (translit.cpp)
};
// Decides the profile from the foreground window's keyboard layout. When `outLayout` is not NULL
// it receives that layout, which CyrillicCharForKey needs.
ActiveProfile GetActiveProfile(HKL *outLayout);
bool IsUzbekInputMethodActive();   // true unless GetActiveProfile() is AP_NONE
bool IsUzbekLayoutInstalled();     // is the Uzbek (Latin) layout in the user's layout list?
bool IsWindows8OrGreaterReal();    // the true OS version; the Windows 7 and 8+ install paths differ
void TriggerAddUzbekLayout();      // Windows 8 and later: add the layout in the background
bool AddUzbekPreloadWin7();        // Windows 7: add it through the registry (needs a sign-out)

// tray.cpp
void TrayAdd();                    // create the tray icon (also after Explorer restarts)
void TrayRemove();                 // delete it on exit
void UpdateTrayStatus();           // refresh the icon, its tooltip and the window text
void TrayShowMenu(HWND hWnd);      // the right-click menu

// pill.cpp
bool PillCreate();                 // create the hint popup window, hidden
void PillDestroy();
void ShowCoachingTip(const wchar_t *doubleTapHint);  // e.g. "ss": "Hint: 'ss' is faster"
void HidePill();

// dashboard.cpp
INT_PTR CALLBACK DashboardDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateDashboardUI();          // refresh the status lines and the "add layout" button
void ShowDashboard();              // show, restore and focus the window

// hook.cpp
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
