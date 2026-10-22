# YAY v2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Enter-undo double-tap with a Telex-style triple-tap engine, rebuild the dashboard as a native Windows dialog, and ship one small static exe that runs on Windows 7 through 11, with the sponsor link in the dashboard, README and installer.

**Architecture:** The single `yay.cpp` is split into focused units under `src/`. The typing rules live in a pure state machine (`engine.cpp`) with no Win32 calls so a console test drives it with real Uzbek words. The dashboard is a `DIALOGEX` resource using only standard controls; a manifest gives themed controls and DPI awareness. A low-level keyboard hook feeds key events to the engine and injects replacements with `SendInput`.

**Tech Stack:** C++ (no STL, no exceptions), Win32 API, MSVC 2022 x86 toolset with static CRT, Windows SDK `rc.exe`, Inno Setup 6, Python + Pillow for icon re-encoding.

Spec: `docs/superpowers/specs/2026-10-22-yay-v2-native-telex-design.md`

Deviations from the spec, decided while planning:
- `/GS-` is not used. The stack cookie costs about 1 KB and is worth keeping.
- The Windows 7 path does not call `LoadKeyboardLayout`. Doing so makes `GetKeyboardLayoutList` report the layout before the language bar can show it, which would flip the dashboard to "already installed" and mislead the user. It writes `Preload` plus `Substitutes` and asks for a sign-out.
- The startup balloon "Dastur ishga tushdi" is removed. A tray app started at logon should be silent; a manual start shows the dashboard anyway.
- The binary is built as 32-bit x86 so one exe runs on 32-bit and 64-bit Windows 7 through 11.

Working rules for every task:
- All commands run from the repo root `D:\OutOfWork\yangi_alifboda_yoz` in `cmd.exe` or PowerShell unless stated otherwise.
- Before any `cl` or `rc` command, open the MSVC environment once per shell: `call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 -no_logo`
- Source files are UTF-8 without BOM. The compiler is always invoked with `/utf-8`.
- Commit messages: subject plus optional body, no trailers of any kind.

---

## File structure

| Path | Responsibility |
| --- | --- |
| `src/engine.h`, `src/engine.cpp` | Pure typing state machine. No Win32. |
| `src/app.h` | Shared constants, globals and function declarations for the Win32 units. |
| `src/inject.cpp` | `SendInput` helpers and Caps/Shift state. |
| `src/layout.cpp` | Active-layout detection, Uzbek layout presence, OS version, layout install for Win7 and Win8+. |
| `src/settings.cpp` | Registry settings and the startup Run entry. |
| `src/tray.cpp` | Tray icon add/remove/update and the tray menu. |
| `src/pill.cpp` | Coaching pill popup window. |
| `src/dashboard.cpp` | Dialog procedure and dashboard refresh. |
| `src/hook.cpp` | Low-level keyboard hook: maps keys to engine events, injects replacements. |
| `src/main.cpp` | Globals, single instance, window class, hook install, message loop. |
| `res/resource.h` | Resource and control IDs. |
| `res/yay.manifest` | Common Controls v6, asInvoker, supportedOS, DPI awareness. |
| `res/yay.rc` | Icons, manifest, version, dialog template. |
| `res/app.ico`, `res/app_paused.ico` | Re-encoded icons. |
| `tools/make_icons.py` | Icon re-encoder from `Icon.png`. |
| `tests/test_engine.cpp` | Console tests for the engine. |
| `build.cmd` | MSVC build: tests, resources, exe, installer. |
| `YAY_Setup.iss` | Installer with Win7 guard and sponsor link. |
| `README.md`, `CONTRIBUTING.md`, `.gitignore` | Docs. |

Deleted at the end of Task 6: `yay.cpp`, `yay.rc`, root `app.ico`, root `app_paused.ico` (moved to `res/`).

---

### Task 1: Typing engine with tests

**Files:**
- Create: `src/engine.h`
- Create: `src/engine.cpp`
- Create: `tests/test_engine.cpp`

- [ ] **Step 1: Write the engine header**

`src/engine.h`:

```cpp
#pragma once
// Pure typing-rule state machine for YAY. No Win32 dependencies so it can be unit-tested.

typedef unsigned long EngineTick;

enum EngineKeyKind {
  EK_LETTER,      // A-Z key; ch holds the case-resolved character
  EK_APOSTROPHE,  // ' or ` without Shift
  EK_MODIFIER,    // Shift, Ctrl, Alt, Caps, Win pressed alone: ignored
  EK_COMBO,       // a key pressed together with Ctrl, Alt or Win: resets
  EK_OTHER        // any other key (space, digits, Backspace, Enter...): resets
};

enum EngineRun { ER_NONE, ER_ONE, ER_CONVERTED, ER_CANCELLED };

struct EngineState {
  EngineRun  run;
  wchar_t    letter;     // lowercase base letter of the current run: s, o, g or c
  wchar_t    typed[2];   // the two characters exactly as typed, kept for cancel
  EngineTick lastTime;   // tick of the last tap in the run
  EngineTick window;     // max ms between taps of one run
};

enum EngineAction { EA_PASS, EA_REPLACE };

struct EngineDecision {
  EngineAction   action;
  int            backspaces;  // EA_REPLACE only: Backspaces to send before text
  wchar_t        text[3];     // EA_REPLACE only: NUL-terminated replacement text
  const wchar_t *tip;         // double-tap hint after a legacy digraph, else 0
};

void           EngineInit(EngineState *st, EngineTick windowMs);
void           EngineReset(EngineState *st);
EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now);
```

- [ ] **Step 2: Write the failing tests**

`tests/test_engine.cpp`:

```cpp
// Console tests for src/engine.cpp. Build and run with build.cmd, or:
//   cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /Fobuild\ /Febuild\test_engine.exe tests\test_engine.cpp src\engine.cpp
#include "../src/engine.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define S_LO L"\x015F"
#define S_UP L"\x015E"
#define C_LO L"\x00E7"
#define C_UP L"\x00C7"
#define O_LO L"\x00F6"
#define O_UP L"\x00D6"
#define G_LO L"\x011F"
#define G_UP L"\x011E"

static int g_failures = 0;

static void PrintW(const wchar_t *s) {
  for (; *s; s++) {
    if (*s < 128) putchar((int)*s);
    else printf("\\u%04X", (unsigned)*s);
  }
}

// Key script: letters -> EK_LETTER with that case; ' -> EK_APOSTROPHE; space -> EK_OTHER
// (and a space in the text); < -> Backspace (EK_OTHER, deletes one char); | -> 1000 ms pause;
// ^ -> a Ctrl combination; _ -> Shift pressed alone. Every key advances the clock by 100 ms.
static void Simulate(const char *keys, wchar_t *out, size_t outCap, EngineDecision *lastDecision) {
  EngineState st;
  EngineInit(&st, 350);
  EngineTick now = 10000;
  size_t len = 0;
  out[0] = 0;
  EngineDecision d = { EA_PASS, 0, {0, 0, 0}, 0 };
  for (const char *p = keys; *p; p++) {
    char c = *p;
    if (c == '|') { now += 1000; continue; }
    now += 100;
    EngineKeyKind kind;
    wchar_t ch = 0;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { kind = EK_LETTER; ch = (wchar_t)c; }
    else if (c == '\'') kind = EK_APOSTROPHE;
    else if (c == '_') kind = EK_MODIFIER;
    else if (c == '^') kind = EK_COMBO;
    else kind = EK_OTHER;
    if (c == '<' && len > 0) { len--; out[len] = 0; }
    d = EngineProcess(&st, kind, ch, now);
    if (d.action == EA_PASS) {
      wchar_t emit = 0;
      if (kind == EK_LETTER) emit = ch;
      else if (kind == EK_APOSTROPHE) emit = L'\'';
      else if (c == ' ') emit = L' ';
      if (emit && len + 1 < outCap) { out[len++] = emit; out[len] = 0; }
    } else {
      for (int i = 0; i < d.backspaces && len > 0; i++) len--;
      for (int i = 0; d.text[i] && len + 1 < outCap; i++) out[len++] = d.text[i];
      out[len] = 0;
    }
  }
  if (lastDecision) *lastDecision = d;
}

static void Expect(const char *keys, const wchar_t *expected) {
  wchar_t out[64];
  Simulate(keys, out, 64, 0);
  if (wcscmp(out, expected) == 0) {
    printf("  ok    %s\n", keys);
  } else {
    g_failures++;
    printf("  FAIL  %s  expected [", keys); PrintW(expected);
    printf("] got ["); PrintW(out); printf("]\n");
  }
}

static void ExpectTip(const char *keys, const wchar_t *expectedTip) {
  wchar_t out[64];
  EngineDecision d;
  Simulate(keys, out, 64, &d);
  bool ok = (expectedTip == 0) ? (d.tip == 0) : (d.tip != 0 && wcscmp(d.tip, expectedTip) == 0);
  if (ok) printf("  ok    tip %s\n", keys);
  else { g_failures++; printf("  FAIL  tip %s  expected [", keys); if (expectedTip) PrintW(expectedTip); printf("] got ["); if (d.tip) PrintW(d.tip); printf("]\n"); }
}

static void ExpectWindow(EngineTick gap, bool shouldConvert) {
  EngineState st;
  EngineInit(&st, 350);
  EngineProcess(&st, EK_LETTER, L's', 1000);
  EngineDecision d = EngineProcess(&st, EK_LETTER, L's', 1000 + gap);
  bool converted = (d.action == EA_REPLACE);
  if (converted == shouldConvert) printf("  ok    window gap %lu\n", gap);
  else { g_failures++; printf("  FAIL  window gap %lu: converted=%d expected=%d\n", gap, (int)converted, (int)shouldConvert); }
}

int main() {
  printf("engine tests\n");

  // Double tap and case
  Expect("ss", S_LO);
  Expect("SS", S_UP);
  Expect("Ss", S_UP);
  Expect("sS", S_UP);
  Expect("oo", O_LO);
  Expect("gg", G_LO);
  Expect("cc", C_LO);

  // Triple tap cancels, further taps are literal
  Expect("sss", L"ss");
  Expect("SSS", L"SS");
  Expect("Sss", L"Ss");
  Expect("ssss", L"sss");
  Expect("ooo", L"oo");
  Expect("ggg", L"gg");
  Expect("ccc", L"cc");

  // Real words
  Expect("isssiq", L"issiq");
  Expect("issiq", L"i" S_LO L"iq");
  Expect("Rosssiya", L"Rossiya");
  Expect("Asssalomu", L"Assalomu");
  Expect("ishsiz", L"i" S_LO L"siz");
  Expect("iss|siz", L"i" S_LO L"siz");
  Expect("tushsa", L"tu" S_LO L"sa");
  Expect("tusssa", L"tussa");               // documented trade-off: sss is always ss
  Expect("shahar", S_LO L"ahar");
  Expect("ssahar", S_LO L"ahar");
  Expect("Shahar", S_UP L"ahar");
  Expect("SHAHAR", S_UP L"AHAR");
  Expect("choy", C_LO L"oy");
  Expect("ccoy", C_LO L"oy");
  Expect("o'rdak", O_LO L"rdak");
  Expect("oordak", O_LO L"rdak");
  Expect("O'rdak", O_UP L"rdak");
  Expect("g'alaba", G_LO L"alaba");
  Expect("ggalaba", G_LO L"alaba");
  Expect("Toshkent", L"To" S_LO L"kent");

  // Resets and non-conversions
  Expect("s|s", L"ss");
  Expect("ss<s", L"s");
  Expect("s^s", L"ss");
  Expect("s_s", S_LO);
  Expect("s s", L"s s");
  Expect("ma'no", L"ma'no");
  Expect("s'", L"s'");
  Expect("oh", L"oh");
  Expect("sso", S_LO L"o");

  // Coaching tips only after legacy digraphs
  ExpectTip("sh", L"ss");
  ExpectTip("SH", L"SS");
  ExpectTip("ch", L"cc");
  ExpectTip("o'", L"oo");
  ExpectTip("G'", L"GG");
  ExpectTip("ss", 0);
  ExpectTip("sss", 0);

  // Window boundary
  ExpectWindow(350, true);
  ExpectWindow(351, false);

  if (g_failures) printf("%d FAILED\n", g_failures);
  else printf("all passed\n");
  return g_failures ? 1 : 0;
}
```

- [ ] **Step 3: Create a stub engine so the test compiles, then run it to see failures**

`src/engine.cpp` (stub):

```cpp
#include "engine.h"

void EngineInit(EngineState *st, EngineTick windowMs) { EngineReset(st); st->window = windowMs; }

void EngineReset(EngineState *st) {
  st->run = ER_NONE; st->letter = 0; st->typed[0] = 0; st->typed[1] = 0; st->lastTime = 0;
}

EngineDecision EngineProcess(EngineState *, EngineKeyKind, wchar_t, EngineTick) {
  EngineDecision d = { EA_PASS, 0, {0, 0, 0}, 0 };
  return d;
}
```

Run:

```
mkdir build
cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /Fobuild\ /Febuild\test_engine.exe tests\test_engine.cpp src\engine.cpp
build\test_engine.exe
```

Expected: many `FAIL` lines, last line `NN FAILED`, exit code 1.

- [ ] **Step 4: Implement the engine**

Replace `src/engine.cpp` with:

```cpp
#include "engine.h"

static wchar_t LowerAscii(wchar_t c) { return (c >= L'A' && c <= L'Z') ? (wchar_t)(c + 32) : c; }
static bool    UpperAscii(wchar_t c) { return c >= L'A' && c <= L'Z'; }
static bool    IsRunLetter(wchar_t lower) { return lower == L's' || lower == L'o' || lower == L'g' || lower == L'c'; }

static wchar_t TargetFor(wchar_t lower, bool upper) {
  switch (lower) {
    case L's': return upper ? 0x015E : 0x015F;  // Ş ş
    case L'c': return upper ? 0x00C7 : 0x00E7;  // Ç ç
    case L'o': return upper ? 0x00D6 : 0x00F6;  // Ö ö
    case L'g': return upper ? 0x011E : 0x011F;  // Ğ ğ
  }
  return 0;
}

static const wchar_t *TipFor(wchar_t lower, bool upper) {
  switch (lower) {
    case L's': return upper ? L"SS" : L"ss";
    case L'c': return upper ? L"CC" : L"cc";
    case L'o': return upper ? L"OO" : L"oo";
    case L'g': return upper ? L"GG" : L"gg";
  }
  return 0;
}

static EngineDecision Pass() {
  EngineDecision d = { EA_PASS, 0, {0, 0, 0}, 0 };
  return d;
}

static EngineDecision Replace1(wchar_t c, const wchar_t *tip) {
  EngineDecision d = { EA_REPLACE, 1, {c, 0, 0}, tip };
  return d;
}

void EngineInit(EngineState *st, EngineTick windowMs) { EngineReset(st); st->window = windowMs; }

void EngineReset(EngineState *st) {
  st->run = ER_NONE; st->letter = 0; st->typed[0] = 0; st->typed[1] = 0; st->lastTime = 0;
}

EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now) {
  if (kind == EK_MODIFIER) return Pass();
  if (kind == EK_OTHER || kind == EK_COMBO) { EngineReset(st); return Pass(); }

  const bool inWindow = (now - st->lastTime) <= st->window;

  // Legacy o' -> ö and g' -> ğ
  if (kind == EK_APOSTROPHE) {
    if (st->run == ER_ONE && (st->letter == L'o' || st->letter == L'g')) {
      const bool up = UpperAscii(st->typed[0]);
      const wchar_t l = st->letter;
      EngineReset(st);
      return Replace1(TargetFor(l, up), TipFor(l, up));
    }
    EngineReset(st);
    return Pass();
  }

  // kind == EK_LETTER from here on
  const wchar_t lower = LowerAscii(ch);

  // Legacy sh -> ş and ch -> ç
  if (lower == L'h') {
    if (st->run == ER_ONE && (st->letter == L's' || st->letter == L'c')) {
      const bool up = UpperAscii(st->typed[0]);
      const wchar_t l = st->letter;
      EngineReset(st);
      return Replace1(TargetFor(l, up), TipFor(l, up));
    }
    EngineReset(st);
    return Pass();
  }

  if (!IsRunLetter(lower)) { EngineReset(st); return Pass(); }

  if (st->letter == lower && inWindow) {
    if (st->run == ER_ONE) {
      const bool up = UpperAscii(st->typed[0]) || UpperAscii(ch);
      st->typed[1] = ch;
      st->run = ER_CONVERTED;
      st->lastTime = now;
      return Replace1(TargetFor(lower, up), 0);
    }
    if (st->run == ER_CONVERTED) {
      EngineDecision d = { EA_REPLACE, 1, {st->typed[0], st->typed[1], 0}, 0 };
      st->run = ER_CANCELLED;
      st->lastTime = now;
      return d;
    }
    if (st->run == ER_CANCELLED) {
      st->lastTime = now;
      return Pass();
    }
  }

  // Start a new run with this letter
  st->run = ER_ONE;
  st->letter = lower;
  st->typed[0] = ch;
  st->typed[1] = 0;
  st->lastTime = now;
  return Pass();
}
```

- [ ] **Step 5: Run the tests**

```
cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /Fobuild\ /Febuild\test_engine.exe tests\test_engine.cpp src\engine.cpp
build\test_engine.exe
```

Expected: every line `ok`, last line `all passed`, exit code 0.

- [ ] **Step 6: Commit**

```
git add src/engine.h src/engine.cpp tests/test_engine.cpp
git commit -m "Add Telex-style typing engine with console tests"
```

---

### Task 2: Resources: IDs, manifest, dialog template, icons

**Files:**
- Create: `res/resource.h`
- Create: `res/yay.manifest`
- Create: `res/yay.rc`
- Create: `tools/make_icons.py`
- Move: `app.ico` -> `res/app.ico`, `app_paused.ico` -> `res/app_paused.ico` (then re-encoded)

- [ ] **Step 1: Write resource IDs**

`res/resource.h`:

```c
#pragma once

// Icons
#define IDI_APPICON            101
#define IDI_PAUSEDICON         102

// Dialog
#define IDD_MAIN               200

// Dialog controls
#define IDC_STATUS_TEXT        3007
#define IDC_LANG_TEXT          3008
#define IDC_BTN_ADD_UZBEK      3009
#define IDC_GRP_RULES          3010
#define IDC_RULE1              3011
#define IDC_RULE2              3012
#define IDC_RULE3              3013
#define IDC_TEST_LABEL         3014
#define IDC_EDIT_TEST          3006
#define IDC_GRP_SETTINGS       3015
#define IDC_CHK_ONLY_UZ        3001
#define IDC_CHK_SINGLE_C       3002
#define IDC_CHK_TIPS           3016
#define IDC_CHK_STARTUP        3003
#define IDC_SPONSOR            3017
#define IDC_BTN_HIDE           3004
#define IDC_BTN_EXIT           3005

// Tray menu commands (also sent as WM_COMMAND by a second instance)
#define ID_TRAY_OPEN_DASHBOARD 1000
#define ID_TRAY_ENABLE         1001
#define ID_TRAY_ONLY_UZBEK     1002
#define ID_TRAY_SINGLE_C       1003
#define ID_TRAY_EXIT           1005
```

- [ ] **Step 2: Write the manifest**

`res/yay.manifest`:

```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity type="win32" name="YAY.YangiAlifbodaYoz" version="2.0.0.0" processorArchitecture="*"/>
  <description>YAY - Yangi Alifboda Yoz</description>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls" version="6.0.0.0"
                        processorArchitecture="*" publicKeyToken="6595b64144ccf1df" language="*"/>
    </dependentAssembly>
  </dependency>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/>
      <supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/>
      <supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/>
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
    </application>
  </compatibility>
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2, PerMonitor, system</dpiAwareness>
    </windowsSettings>
  </application>
</assembly>
```

The four GUIDs are Windows 7, 8, 8.1 and 10/11 in that order.

- [ ] **Step 3: Write the resource script with the dialog**

`res/yay.rc` (UTF-8, the pragma tells rc.exe and windres so):

```rc
#pragma code_page(65001)
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

1 RT_MANIFEST "yay.manifest"

IDI_APPICON    ICON "app.ico"
IDI_PAUSEDICON ICON "app_paused.ico"

VS_VERSION_INFO VERSIONINFO
FILEVERSION     2,0,0,0
PRODUCTVERSION  2,0,0,0
FILEFLAGSMASK   0x3fL
FILEFLAGS       0x0L
FILEOS          0x40004L
FILETYPE        0x1L
FILESUBTYPE     0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "Comments",         "YAY - Yangi Alifboda Yoz\0"
            VALUE "CompanyName",      "YAY Project\0"
            VALUE "FileDescription",  "YAY - Yangi Alifboda Yoz\0"
            VALUE "FileVersion",      "2.0.0.0\0"
            VALUE "InternalName",     "YAY\0"
            VALUE "LegalCopyright",   "MIT License\0"
            VALUE "OriginalFilename", "YAY.exe\0"
            VALUE "ProductName",      "YAY - Yangi Alifboda Yoz\0"
            VALUE "ProductVersion",   "2.0.0.0\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END

IDD_MAIN DIALOGEX 0, 0, 320, 240
STYLE DS_SETFONT | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
EXSTYLE WS_EX_APPWINDOW
CLASS "YAYDashboardClass"
CAPTION "YAY - Yangi Alifboda Yoz"
FONT 9, "Segoe UI", 400, 0, 0x0
BEGIN
    LTEXT           "Holat: aniqlanmoqda...", IDC_STATUS_TEXT, 7, 7, 306, 10
    LTEXT           "Tilni almashtirish: Win + Space", IDC_LANG_TEXT, 7, 19, 306, 10
    PUSHBUTTON      "O'zbek (lotin) tilini til paneliga qo'shish", IDC_BTN_ADD_UZBEK, 7, 33, 220, 14

    GROUPBOX        "Qoidalar", IDC_GRP_RULES, 7, 54, 306, 76
    LTEXT           "ss → ş          oo → ö          gg → ğ          cc → ç", IDC_RULE1, 15, 67, 290, 10
    LTEXT           "Uch marta bossangiz asl harflar qoladi: sss → ss, ooo → oo", IDC_RULE2, 15, 79, 290, 10
    LTEXT           "Eski usul ham ishlaydi: sh → ş, ch → ç, o' → ö, g' → ğ", IDC_RULE3, 15, 91, 290, 10
    LTEXT           "Sinab ko'ring:", IDC_TEST_LABEL, 15, 110, 48, 10
    EDITTEXT        IDC_EDIT_TEST, 66, 107, 239, 14, ES_AUTOHSCROLL

    GROUPBOX        "Sozlamalar", IDC_GRP_SETTINGS, 7, 137, 306, 70
    AUTOCHECKBOX    "Faqat O'zbek (UZB) tili tanlanganda ishlash", IDC_CHK_ONLY_UZ, 15, 150, 290, 10
    AUTOCHECKBOX    "Bitta 'c' harfini darhol 'ç' qilib yozish", IDC_CHK_SINGLE_C, 15, 164, 290, 10
    AUTOCHECKBOX    "Eski usulda yozganda maslahat ko'rsatish", IDC_CHK_TIPS, 15, 178, 290, 10
    AUTOCHECKBOX    "Windows bilan birga ishga tushirish", IDC_CHK_STARTUP, 15, 192, 290, 10

    CONTROL         "Homiy: <a href=""https://t.me/juraboshibot"">t.me/juraboshibot</a>", IDC_SPONSOR, "SysLink", WS_TABSTOP, 7, 221, 160, 10
    PUSHBUTTON      "Trayga &yashirish", IDC_BTN_HIDE, 191, 218, 72, 14
    PUSHBUTTON      "&Chiqish", IDC_BTN_EXIT, 267, 218, 46, 14
END
```

There is deliberately no `DEFPUSHBUTTON`: Enter in the test box must not hide the window.

- [ ] **Step 4: Move and re-encode the icons**

```
git mv app.ico res/app.ico
git mv app_paused.ico res/app_paused.ico
```

`tools/make_icons.py`:

```python
"""Re-encode the YAY icons from Icon.png with PNG-compressed layers.

Usage: python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico

The active icon keeps a 256 px layer for Explorer and shortcuts. The paused icon is
only ever shown in the tray, so it stops at 32 px.
"""
import sys
from PIL import Image, ImageOps

src = Image.open(sys.argv[1]).convert("RGBA")

active_sizes = [(16, 16), (20, 20), (24, 24), (32, 32), (48, 48), (256, 256)]
src.save(sys.argv[2], format="ICO", sizes=active_sizes, bitmap_format="png")

gray = ImageOps.grayscale(src)
alpha = src.getchannel("A").point(lambda a: int(a * 0.7))
paused = Image.merge("RGBA", (gray, gray, gray, alpha))
paused_sizes = [(16, 16), (20, 20), (24, 24), (32, 32)]
paused.save(sys.argv[3], format="ICO", sizes=paused_sizes, bitmap_format="png")
print("icons written")
```

Run:

```
python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico
dir res\*.ico
```

Expected: `icons written`; `res\app.ico` well under 154 KB, `res\app_paused.ico` under 10 KB.

- [ ] **Step 5: Compile the resources**

```
pushd res
rc /nologo /fo ..\build\yay.res yay.rc
popd
```

Expected: no output, `build\yay.res` exists. If rc reports a syntax error with a line number, the line refers to `yay.rc`.

- [ ] **Step 6: Commit**

```
git add res/resource.h res/yay.manifest res/yay.rc res/app.ico res/app_paused.ico tools/make_icons.py
git commit -m "Add native dialog resources, manifest and re-encoded icons"
```

---

### Task 3: Shared header, settings, input injection, layout detection

**Files:**
- Create: `src/app.h`
- Create: `src/settings.cpp`
- Create: `src/inject.cpp`
- Create: `src/layout.cpp`

- [ ] **Step 1: Write the shared header**

`src/app.h`:

```cpp
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
#ifndef _WIN32_IE
#define _WIN32_IE 0x0800
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>

#include "../res/resource.h"
#include "engine.h"

// Private window messages. WM_USER+n is reserved by the dialog manager (DM_SETDEFID), so use WM_APP.
#define WM_TRAYICON            (WM_APP + 1)
#define WM_LAYOUT_INSTALL_DONE (WM_APP + 2)

// Timers (all on the dashboard window)
#define TIMER_COACH            2002
#define TIMER_DASH_REFRESH     2004

#define DURATION_COACH_MS      4000
#define DOUBLE_TAP_MS          350

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
bool IsUzbekInputMethodActive();
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
```

- [ ] **Step 2: Write settings.cpp**

`src/settings.cpp`:

```cpp
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
    swprintf_s(cmd, MAX_PATH + 16, L"\"%s\" /tray", path);
    RegSetValueExW(hKey, RUN_VALUE_NAME, 0, REG_SZ, (const BYTE *)cmd, (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t)));
  }
  RegCloseKey(hKey);
}
```

- [ ] **Step 3: Write inject.cpp**

`src/inject.cpp`:

```cpp
#include "app.h"

// Sends `backspaces` Backspace presses followed by `chars` as Unicode key events.
// g_bSendingInput is only a belt-and-braces guard; injected events also carry LLKHF_INJECTED.
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

// Uppercase when exactly one of Shift / Caps Lock is active.
bool IsCapsActive() {
  bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
  bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
  return shift != caps;
}
```

- [ ] **Step 4: Write layout.cpp**

`src/layout.cpp`:

```cpp
#include "app.h"

// True when YAY should act on keys going to the foreground window.
// Strict mode: only Uzbek Latin (0x0443). Permissive mode: Uzbek Latin or English.
// Russian, Uzbek Cyrillic and every other layout are always left alone.
bool IsUzbekInputMethodActive() {
  if (!g_bEnabled) return false;
  HWND hFg = GetForegroundWindow();
  DWORD tid = hFg ? GetWindowThreadProcessId(hFg, NULL) : 0;
  WORD langId = LOWORD((DWORD_PTR)GetKeyboardLayout(tid));
  WORD primary = PRIMARYLANGID(langId);
  if (langId == 0x0443 || (primary == LANG_UZBEK && SUBLANGID(langId) == SUBLANG_UZBEK_LATIN)) return true;
  if (g_bOnlyUzbekLayout) return false;
  return primary == LANG_ENGLISH;
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

// The manifest lists supportedOS entries, so GetVersionEx would be truthful too,
// but RtlGetVersion never lies regardless of compatibility shims.
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

static bool SetStringValue(HKEY hKey, const wchar_t *name, const wchar_t *value) {
  return RegSetValueExW(hKey, name, 0, REG_SZ, (const BYTE *)value,
                        (DWORD)((wcslen(value) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
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
    int idx = _wtoi(name);
    if (idx > maxIdx) maxIdx = idx;
    if (type == REG_SZ && _wcsicmp(data, L"00000443") == 0) exists = true;
  }
  bool ok = true;
  if (!exists) {
    wchar_t name[16];
    swprintf_s(name, 16, L"%d", maxIdx + 1);
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
```

- [ ] **Step 5: Compile the three units**

```
cl /nologo /c /W3 /O1 /MT /EHs-c- /utf-8 /DUNICODE /D_UNICODE /Fobuild\ src\settings.cpp src\inject.cpp src\layout.cpp
```

Expected: three `.obj` files in `build\`, no errors. Warnings about unreferenced parameters are acceptable.

- [ ] **Step 6: Commit**

```
git add src/app.h src/settings.cpp src/inject.cpp src/layout.cpp
git commit -m "Add settings, input injection and layout detection units"
```

---

### Task 4: Coach pill and tray

**Files:**
- Create: `src/pill.cpp`
- Create: `src/tray.cpp`

- [ ] **Step 1: Write pill.cpp**

`src/pill.cpp`:

```cpp
#include "app.h"

static HWND    g_hCoachWnd = NULL;
static HFONT   g_hPillFont = NULL;
static wchar_t g_coachText[128] = {0};

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

void HidePill() {
  if (g_hMainWnd) KillTimer(g_hMainWnd, TIMER_COACH);
  if (g_hCoachWnd && IsWindowVisible(g_hCoachWnd)) ShowWindow(g_hCoachWnd, SW_HIDE);
}

// Places the pill just below the caret of the foreground window, or in the window's top-right
// corner when the caret is not exposed (browsers, Word, Electron apps).
static void ShowFloatingPill(const wchar_t *text) {
  if (!g_hCoachWnd) return;
  wcscpy_s(g_coachText, 128, text);

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

  HRGN hRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, 14, 14);
  SetWindowRgn(g_hCoachWnd, hRgn, TRUE);
  SetWindowPos(g_hCoachWnd, HWND_TOPMOST, posX, posY, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  InvalidateRect(g_hCoachWnd, NULL, TRUE);
  if (g_hMainWnd) SetTimer(g_hMainWnd, TIMER_COACH, DURATION_COACH_MS, NULL);
}

void ShowCoachingTip(const wchar_t *doubleTapHint) {
  wchar_t msg[128];
  swprintf_s(msg, 128, L"Maslahat: '%s' tezroq", doubleTapHint);
  ShowFloatingPill(msg);
}
```

- [ ] **Step 2: Write tray.cpp**

`src/tray.cpp`:

```cpp
#include "app.h"

static NOTIFYICONDATAW g_nid = {0};

void TrayAdd() {
  g_nid.cbSize = sizeof(NOTIFYICONDATAW);
  g_nid.hWnd = g_hMainWnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  g_nid.uCallbackMessage = WM_TRAYICON;
  g_nid.hIcon = g_hIconActive;
  wcscpy_s(g_nid.szTip, 128, APP_NAME);
  Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void TrayRemove() {
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void UpdateTrayStatus() {
  bool activeNow = g_bEnabled && IsUzbekInputMethodActive();
  HICON icon = activeNow ? g_hIconActive : g_hIconPaused;
  const wchar_t *tip = !g_bEnabled ? L"YAY: O'chirilgan (pauza)"
                     : activeNow   ? L"YAY: Faol (UZB tanlangan)"
                                   : L"YAY: Kutish rejimi (boshqa til)";
  if (g_nid.hIcon != icon || wcscmp(g_nid.szTip, tip) != 0) {
    g_nid.hIcon = icon;
    wcscpy_s(g_nid.szTip, 128, tip);
    g_nid.uFlags = NIF_ICON | NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  }
  UpdateDashboardUI();
}

void TrayShowMenu(HWND hWnd) {
  HMENU hMenu = CreatePopupMenu();
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_DASHBOARD, L"YAY oynasini ochish");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING | (g_bEnabled ? MF_CHECKED : 0), ID_TRAY_ENABLE, L"Faol (yoqilgan)");
  AppendMenuW(hMenu, MF_STRING | (g_bOnlyUzbekLayout ? MF_CHECKED : 0), ID_TRAY_ONLY_UZBEK, L"Faqat UZB tilida ishlash");
  AppendMenuW(hMenu, MF_STRING | (g_bInstantSingleC ? MF_CHECKED : 0), ID_TRAY_SINGLE_C, L"Bitta 'c' → 'ç' rejimi");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Chiqish");

  POINT pt;
  GetCursorPos(&pt);
  SetForegroundWindow(hWnd);
  TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
  PostMessageW(hWnd, WM_NULL, 0, 0);
  DestroyMenu(hMenu);
}
```

- [ ] **Step 3: Compile both units**

```
cl /nologo /c /W3 /O1 /MT /EHs-c- /utf-8 /DUNICODE /D_UNICODE /Fobuild\ src\pill.cpp src\tray.cpp
```

Expected: `build\pill.obj` and `build\tray.obj`, no errors.

- [ ] **Step 4: Commit**

```
git add src/pill.cpp src/tray.cpp
git commit -m "Add coach pill and tray units"
```

---

### Task 5: Dashboard dialog procedure

**Files:**
- Create: `src/dashboard.cpp`

- [ ] **Step 1: Write dashboard.cpp**

`src/dashboard.cpp`:

```cpp
#include "app.h"

static UINT g_shellHookMsg = 0;

static void SetCheck(HWND hDlg, int id, bool on) { CheckDlgButton(hDlg, id, on ? BST_CHECKED : BST_UNCHECKED); }
static bool GetCheck(HWND hDlg, int id) { return IsDlgButtonChecked(hDlg, id) == BST_CHECKED; }

void ShowDashboard() {
  if (!g_hMainWnd) return;
  ShowWindow(g_hMainWnd, SW_SHOW);
  ShowWindow(g_hMainWnd, SW_RESTORE);
  SetForegroundWindow(g_hMainWnd);
  UpdateDashboardUI();
}

void UpdateDashboardUI() {
  if (!g_hMainWnd || !IsWindowVisible(g_hMainWnd)) return;

  bool active = g_bEnabled && IsUzbekInputMethodActive();
  const wchar_t *status = !g_bEnabled ? L"Holat: O'chirilgan (pauza). Yoqish uchun tray belgisini bosing."
                        : active      ? L"Holat: Faol. UZB (O'zbek lotin) tanlangan, yozishingiz mumkin."
                                      : L"Holat: Kutish rejimi. Boshqa til tanlangan.";
  SetDlgItemTextW(g_hMainWnd, IDC_STATUS_TEXT, status);

  bool uzInstalled = IsUzbekLayoutInstalled();
  HWND hBtn = GetDlgItem(g_hMainWnd, IDC_BTN_ADD_UZBEK);
  if (uzInstalled) {
    SetWindowTextW(hBtn, L"O'zbek (lotin) tili tizimda mavjud (Win + Space)");
    EnableWindow(hBtn, FALSE);
  } else if (g_bWin7PreloadWritten) {
    SetWindowTextW(hBtn, L"Qayta kirgach UZB tili paydo bo'ladi");
    EnableWindow(hBtn, FALSE);
  } else {
    SetWindowTextW(hBtn, L"O'zbek (lotin) tilini til paneliga qo'shish");
    EnableWindow(hBtn, TRUE);
  }

  HWND hFg = GetForegroundWindow();
  DWORD tid = hFg ? GetWindowThreadProcessId(hFg, NULL) : 0;
  WORD langId = LOWORD((DWORD_PTR)GetKeyboardLayout(tid));
  wchar_t buf[256];
  if (langId == 0x0443) {
    wcscpy_s(buf, 256, L"Hozirgi klaviatura: UZB (O'zbek lotin). Yangi alifbo faol.");
  } else if (PRIMARYLANGID(langId) == LANG_ENGLISH) {
    if (!uzInstalled)            wcscpy_s(buf, 256, L"Hozirgi klaviatura: ENG. UZB tili tizimda yo'q, pastdagi tugmani bosing.");
    else if (!g_bOnlyUzbekLayout) wcscpy_s(buf, 256, L"Hozirgi klaviatura: ENG. YAY shu tilda ham ishlaydi.");
    else                         wcscpy_s(buf, 256, L"Hozirgi klaviatura: ENG. UZB tiliga o'tish: Win + Space");
  } else if (PRIMARYLANGID(langId) == LANG_RUSSIAN) {
    wcscpy_s(buf, 256, L"Hozirgi klaviatura: RUS. Rus tiliga xalaqit bermaydi.");
  } else {
    swprintf_s(buf, 256, L"Hozirgi klaviatura: 0x%04X. Tilni almashtirish: Win + Space", langId);
  }
  SetDlgItemTextW(g_hMainWnd, IDC_LANG_TEXT, buf);
}

static void OnAddUzbekLayout(HWND hDlg) {
  HWND hBtn = GetDlgItem(hDlg, IDC_BTN_ADD_UZBEK);
  if (IsWindows8OrGreaterReal()) {
    SetWindowTextW(hBtn, L"O'rnatilmoqda...");
    EnableWindow(hBtn, FALSE);
    TriggerAddUzbekLayout();
    return;
  }
  if (AddUzbekPreloadWin7()) {
    g_bWin7PreloadWritten = true;
    MessageBoxW(hDlg,
                L"O'zbek (lotin) tili qo'shildi.\nTil panelida ko'rinishi uchun tizimdan chiqib (Log off), qayta kiring.",
                APP_NAME, MB_OK | MB_ICONINFORMATION);
  } else {
    MessageBoxW(hDlg,
                L"Tilni avtomatik qo'shib bo'lmadi.\nControl Panel > Region and Language > Keyboards and Languages "
                L"orqali \"Uzbek (Latin)\" tilini qo'shing.",
                APP_NAME, MB_OK | MB_ICONWARNING);
  }
  UpdateDashboardUI();
}

INT_PTR CALLBACK DashboardDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (g_shellHookMsg && msg == g_shellHookMsg) {
    if (wParam == HSHELL_LANGUAGE || wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED)
      UpdateTrayStatus();
    return TRUE;
  }

  switch (msg) {
  case WM_INITDIALOG:
    g_hMainWnd = hDlg;
    SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconActive);
    SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig);
    SetCheck(hDlg, IDC_CHK_ONLY_UZ, g_bOnlyUzbekLayout);
    SetCheck(hDlg, IDC_CHK_SINGLE_C, g_bInstantSingleC);
    SetCheck(hDlg, IDC_CHK_TIPS, g_bShowTips);
    SetCheck(hDlg, IDC_CHK_STARTUP, IsInStartup());
    g_shellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");
    RegisterShellHookWindow(hDlg);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_TEST));
    return FALSE;

  case WM_SHOWWINDOW:
    if (wParam) SetTimer(hDlg, TIMER_DASH_REFRESH, 1000, NULL);
    else KillTimer(hDlg, TIMER_DASH_REFRESH);
    return FALSE;

  case WM_COMMAND: {
    int id = LOWORD(wParam);
    switch (id) {
    case IDC_BTN_ADD_UZBEK: OnAddUzbekLayout(hDlg); return TRUE;
    case IDC_CHK_ONLY_UZ:   g_bOnlyUzbekLayout = GetCheck(hDlg, id); SaveSettings(); UpdateTrayStatus(); return TRUE;
    case IDC_CHK_SINGLE_C:  g_bInstantSingleC = GetCheck(hDlg, id); SaveSettings(); return TRUE;
    case IDC_CHK_TIPS:      g_bShowTips = GetCheck(hDlg, id); SaveSettings(); return TRUE;
    case IDC_CHK_STARTUP:   ToggleStartup(GetCheck(hDlg, id)); return TRUE;
    case IDC_BTN_HIDE:
    case IDCANCEL:          ShowWindow(hDlg, SW_HIDE); return TRUE;
    case IDC_BTN_EXIT:
    case ID_TRAY_EXIT:      DestroyWindow(hDlg); return TRUE;
    case ID_TRAY_OPEN_DASHBOARD: ShowDashboard(); return TRUE;
    case ID_TRAY_ENABLE:    g_bEnabled = !g_bEnabled; UpdateTrayStatus(); return TRUE;
    case ID_TRAY_ONLY_UZBEK:
      g_bOnlyUzbekLayout = !g_bOnlyUzbekLayout; SaveSettings();
      SetCheck(hDlg, IDC_CHK_ONLY_UZ, g_bOnlyUzbekLayout); UpdateTrayStatus(); return TRUE;
    case ID_TRAY_SINGLE_C:
      g_bInstantSingleC = !g_bInstantSingleC; SaveSettings();
      SetCheck(hDlg, IDC_CHK_SINGLE_C, g_bInstantSingleC); return TRUE;
    }
    return FALSE;
  }

  case WM_NOTIFY: {
    NMHDR *hdr = (NMHDR *)lParam;
    if (hdr->idFrom == IDC_SPONSOR && (hdr->code == NM_CLICK || hdr->code == NM_RETURN)) {
      NMLINK *link = (NMLINK *)lParam;
      const wchar_t *url = link->item.szUrl[0] ? link->item.szUrl : SPONSOR_URL;
      ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
      return TRUE;
    }
    return FALSE;
  }

  case WM_LAYOUT_INSTALL_DONE:
    UpdateTrayStatus();
    return TRUE;

  case WM_TIMER:
    if (wParam == TIMER_COACH) HidePill();
    else if (wParam == TIMER_DASH_REFRESH) UpdateTrayStatus();
    return TRUE;

  case WM_TRAYICON:
    if (lParam == WM_LBUTTONUP) {
      g_bEnabled = !g_bEnabled;
      UpdateTrayStatus();
    } else if (lParam == WM_LBUTTONDBLCLK) {
      g_bEnabled = !g_bEnabled;   // undo the toggle made by the first click of the double click
      UpdateTrayStatus();
      ShowDashboard();
    } else if (lParam == WM_RBUTTONUP) {
      TrayShowMenu(hDlg);
    }
    return TRUE;

  case WM_CLOSE:
    ShowWindow(hDlg, SW_HIDE);
    return TRUE;

  case WM_DESTROY:
    KillTimer(hDlg, TIMER_DASH_REFRESH);
    KillTimer(hDlg, TIMER_COACH);
    DeregisterShellHookWindow(hDlg);
    TrayRemove();
    g_hMainWnd = NULL;
    PostQuitMessage(0);
    return TRUE;
  }
  return FALSE;
}
```

- [ ] **Step 2: Compile**

```
cl /nologo /c /W3 /O1 /MT /EHs-c- /utf-8 /DUNICODE /D_UNICODE /Fobuild\ src\dashboard.cpp
```

Expected: `build\dashboard.obj`, no errors.

- [ ] **Step 3: Commit**

```
git add src/dashboard.cpp
git commit -m "Add native dashboard dialog procedure"
```

---

### Task 6: Hook, main, build script, full build

**Files:**
- Create: `src/hook.cpp`
- Create: `src/main.cpp`
- Create: `build.cmd`
- Modify: `.gitignore`
- Delete: `yay.cpp`, `yay.rc`

- [ ] **Step 1: Write hook.cpp**

`src/hook.cpp`:

```cpp
#include "app.h"

static bool IsModifierVk(DWORD vk) {
  return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
         vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
         vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
         vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN;
}

static bool KeyHeld(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode != HC_ACTION || g_bSendingInput || !g_bEnabled)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  const KBDLLHOOKSTRUCT *k = (const KBDLLHOOKSTRUCT *)lParam;
  const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
  if (!isDown || (k->flags & LLKHF_INJECTED) || !IsUzbekInputMethodActive())
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  const DWORD vk = k->vkCode;
  EngineKeyKind kind;
  wchar_t ch = 0;
  if (IsModifierVk(vk)) {
    kind = EK_MODIFIER;
  } else if (wParam == WM_SYSKEYDOWN || KeyHeld(VK_CONTROL) || KeyHeld(VK_MENU) || KeyHeld(VK_LWIN) || KeyHeld(VK_RWIN)) {
    kind = EK_COMBO;
  } else if (vk >= 'A' && vk <= 'Z') {
    kind = EK_LETTER;
    ch = IsCapsActive() ? (wchar_t)vk : (wchar_t)(vk + 32);
  } else if ((vk == VK_OEM_7 || vk == VK_OEM_3) && !KeyHeld(VK_SHIFT)) {
    kind = EK_APOSTROPHE;
  } else {
    kind = EK_OTHER;
  }

  if (kind != EK_MODIFIER) HidePill();

  if (g_bInstantSingleC && kind == EK_LETTER && (ch == L'c' || ch == L'C')) {
    EngineReset(&g_engine);
    SendBackspaceAndChars(0, ch == L'C' ? L"\u00C7" : L"\u00E7");
    return 1;
  }

  EngineDecision d = EngineProcess(&g_engine, kind, ch, GetTickCount());
  if (d.action == EA_REPLACE) {
    SendBackspaceAndChars(d.backspaces, d.text);
    if (d.tip && g_bShowTips) ShowCoachingTip(d.tip);
    return 1;
  }
  return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}
```

- [ ] **Step 2: Write main.cpp**

`src/main.cpp`:

```cpp
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
bool        g_bSendingInput     = false;
bool        g_bWin7PreloadWritten = false;
EngineState g_engine            = {ER_NONE, 0, {0, 0}, 0, DOUBLE_TAP_MS};

static HICON LoadAppIcon(int resId, int size) {
  return (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(resId), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
  g_hInst = hInstance;
  const wchar_t *cmd = GetCommandLineW();
  const bool startInTray = cmd && (wcsstr(cmd, L"/tray") != NULL || wcsstr(cmd, L"/minimized") != NULL);

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
    MessageBoxW(NULL, L"Oyna sinfini ro'yxatdan o'tkazib bo'lmadi.", APP_NAME, MB_ICONERROR);
    return 1;
  }

  PillCreate();

  HWND hDlg = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), NULL, DashboardDlgProc, 0);
  if (!hDlg) {
    MessageBoxW(NULL, L"Asosiy oynani yaratib bo'lmadi.", APP_NAME, MB_ICONERROR);
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
```

- [ ] **Step 3: Write build.cmd**

`build.cmd`:

```bat
@echo off
setlocal
cd /d "%~dp0"

rem ---------------------------------------------------------------------------
rem YAY build: engine tests, resources, YAY.exe (x86, static CRT), installer.
rem Needs Visual Studio 2022 (Desktop C++ workload). Inno Setup 6 is optional.
rem ---------------------------------------------------------------------------

set "VSDEVCMD="
for %%E in (Community Professional Enterprise BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\Tools\VsDevCmd.bat" (
    set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\Tools\VsDevCmd.bat"
  )
)
if "%VSDEVCMD%"=="" (
  echo Visual Studio 2022 not found.
  exit /b 1
)
call "%VSDEVCMD%" -arch=x86 -host_arch=x64 -no_logo
if errorlevel 1 exit /b 1

if not exist build mkdir build

set CFLAGS=/nologo /W3 /O1 /Os /GL /MT /EHs-c- /GR- /utf-8 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 /DNDEBUG /Fobuild\
set LFLAGS=/nologo /LTCG /OPT:REF /OPT:ICF /MANIFEST:NO /SUBSYSTEM:WINDOWS,6.01 /INCREMENTAL:NO
set LIBS=user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib

echo [1/4] Engine tests
cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /DUNICODE /D_UNICODE /Fobuild\ /Febuild\test_engine.exe tests\test_engine.cpp src\engine.cpp
if errorlevel 1 exit /b 1
build\test_engine.exe
if errorlevel 1 exit /b 1

echo [2/4] Resources
pushd res
rc /nologo /fo ..\build\yay.res yay.rc
if errorlevel 1 ( popd & exit /b 1 )
popd

echo [3/4] YAY.exe
cl %CFLAGS% /FeYAY.exe src\main.cpp src\hook.cpp src\engine.cpp src\inject.cpp src\layout.cpp src\settings.cpp src\tray.cpp src\pill.cpp src\dashboard.cpp build\yay.res /link %LFLAGS% %LIBS%
if errorlevel 1 exit /b 1
for %%F in (YAY.exe) do echo YAY.exe: %%~zF bytes

echo [4/4] Installer
set "ISCC="
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" (
  echo Inno Setup 6 not found, installer skipped.
) else (
  "%ISCC%" /Q YAY_Setup.iss
  if errorlevel 1 exit /b 1
  for %%F in (Output\YAY_Setup.exe) do echo Output\YAY_Setup.exe: %%~zF bytes
)
echo Done.
```

- [ ] **Step 4: Update .gitignore and remove the old sources**

Append to `.gitignore`:

```
# Build output
build/
```

Then:

```
git rm -q yay.cpp yay.rc
```

- [ ] **Step 5: Run the full build**

From a plain `cmd.exe` or PowerShell (the script opens its own MSVC environment):

```
build.cmd
```

Expected output, in order: `engine tests` with all `ok` and `all passed`; `[2/4] Resources` with no error; `[3/4] YAY.exe` ending in `YAY.exe: NNNNN bytes`; `[4/4] Installer` either building `Output\YAY_Setup.exe` or saying it was skipped; `Done.`

If `YAY.exe` is larger than 160000 bytes, edit `tools/make_icons.py` so `active_sizes` ends with `(128, 128)` instead of `(256, 256)`, re-run the icon script and `build.cmd`.

- [ ] **Step 6: Smoke-test the exe**

```
start YAY.exe
```

Check: the dashboard opens centered with themed controls and Segoe UI text; the tray icon appears; typing `ss` in the test box gives ş and a third `s` gives `ss`; typing `sh` gives ş and shows the amber "Maslahat" pill, which disappears on the next key; Escape hides the window; double-clicking the tray icon brings it back; the sponsor link opens the browser; "Chiqish" removes the tray icon and the process exits.

- [ ] **Step 7: Commit**

```
git add src/hook.cpp src/main.cpp build.cmd .gitignore YAY.exe
git commit -m "Rebuild YAY as split Win32 units with native dialog and Telex engine"
```

---

### Task 7: Installer: Windows 7 guard and sponsor link

**Files:**
- Modify: `YAY_Setup.iss`

- [ ] **Step 1: Replace YAY_Setup.iss**

```iss
#define MyAppName "YAY - Yangi Alifboda Yoz"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "YAY"
#define MyAppExeName "YAY.exe"
#define SponsorUrl "https://t.me/juraboshibot"

[Setup]
AppId={{8E4A9B2C-5D1F-4A8E-9C3B-7F2D6E1A4B90}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName={autopf}\YAY
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
MinVersion=6.1sp1
OutputDir=Output
OutputBaseFilename=YAY_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=force
SetupIconFile=res\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
SourceDir=.

; Wizard offered in Uzbek and Russian
[Languages]
Name: "uzbek"; MessagesFile: "compiler:Default.isl,Uzbek.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "Ish stolida yorliq yaratish / Создать ярлык на рабочем столе"; GroupDescription: "Qo'shimcha amallar / Дополнительно:"
Name: "startup"; Description: "Windows bilan birga avtomatik ishga tushirish / Автозапуск вместе с Windows"; GroupDescription: "Qo'shimcha sozlamalar / Дополнительные параметры:"
Name: "addinputmethod"; Description: "Windows Til paneliga O'zbek Lotin tilini qo'shish (Win+Space) / Добавить Узбекский (Латиница) в языковую панель"; GroupDescription: "Qo'shimcha sozlamalar / Дополнительные параметры:"; MinVersion: 6.2

[Files]
Source: "{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "install_uzbek_layout.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "YAY_UzbekKeyboard"; ValueData: """{app}\{#MyAppExeName}"" /tray"; Flags: uninsdeletevalue; Tasks: startup

[Run]
; Windows 8+ only: the cmdlet does not exist on Windows 7. The app offers its own Windows 7 path.
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File ""{app}\install_uzbek_layout.ps1"""; Flags: runhidden waituntilterminated; Tasks: addinputmethod; MinVersion: 6.2
Filename: "{app}\{#MyAppExeName}"; Description: "Dasturni hozir ishga tushirish / Запустить программу сейчас"; Flags: nowait postinstall

[UninstallRun]
Filename: "taskkill.exe"; Parameters: "/F /IM {#MyAppExeName}"; Flags: runhidden; RunOnceId: "StopYAY"

[UninstallDelete]
Type: files; Name: "{app}\{#MyAppExeName}"
Type: files; Name: "{app}\install_uzbek_layout.ps1"

[Code]
var
  SponsorLink: TNewStaticText;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;

procedure SponsorLinkClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('open', '{#SponsorUrl}', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

procedure InitializeWizard();
begin
  SponsorLink := TNewStaticText.Create(WizardForm);
  SponsorLink.Parent := WizardForm.FinishedPage;
  SponsorLink.Caption := 'Homiy / Спонсор: t.me/juraboshibot';
  SponsorLink.Cursor := crHand;
  SponsorLink.Font.Color := clBlue;
  SponsorLink.Font.Style := [fsUnderline];
  SponsorLink.OnClick := @SponsorLinkClick;
  SponsorLink.Left := WizardForm.FinishedLabel.Left;
  SponsorLink.Top := WizardForm.FinishedPage.Height - SponsorLink.Height - ScaleY(12);
end;
```

- [ ] **Step 2: Build the installer**

```
"%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" /Q YAY_Setup.iss
dir Output\YAY_Setup.exe
```

Expected: no errors, `Output\YAY_Setup.exe` exists with a fresh timestamp.

- [ ] **Step 3: Run the installer once**

```
start Output\YAY_Setup.exe
```

Check: the finish page shows the underlined sponsor line at the bottom, clicking it opens the browser, the "Dasturni hozir ishga tushirish" checkbox launches the new dashboard.

- [ ] **Step 4: Commit**

```
git add YAY_Setup.iss Output/YAY_Setup.exe
git commit -m "Installer: Windows 7 minimum, skip PowerShell on Win7, sponsor link on finish page"
```

---

### Task 8: README, CONTRIBUTING

**Files:**
- Modify: `README.md`
- Modify: `CONTRIBUTING.md`

- [ ] **Step 1: Replace README.md**

```markdown
<div align="center">

<img src="Icon.png" alt="YAY Logo" width="128" height="128">

# YAY — Yangi Alifboda Yoz

**O'zbek tilining isloh qilingan yangi lotin alifbosida tez va oson yozish uchun yengil, zamonaviy Windows yordamchisi.**
*An ultra-lightweight, native Windows keyboard assistant for the reformed Uzbek Latin alphabet.*

[![Sponsor](https://img.shields.io/badge/Homiy%20%2F%20Sponsor-%40juraboshibot-26A5E4?logo=telegram&logoColor=white)](https://t.me/juraboshibot)
[![Platform](https://img.shields.io/badge/Platform-Windows%207%20%7C%208%20%7C%2010%20%7C%2011-blue?logo=windows)](https://github.com/nodiranor/yangi_alifboda_yoz)
[![C++](https://img.shields.io/badge/Language-C%2B%2B%20%28Pure%20Win32%29-00599C?logo=c%2B%2B)](src)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Zero Dependency](https://img.shields.io/badge/Dependencies-Zero%20Runtime-orange.svg)](#-texnik-xususiyatlar--technical-notes)

[O'zbekcha](#-ozbekcha) • [English](#-english) • [Qoidalar / Rules](#%EF%B8%8F-alifbo-qoidalari--typing-rules) • [O'rnatish / Install](#-ornatish--installation) • [Kompilyatsiya / Build](#-yigish--building-from-source)

</div>

---

## 🇺🇿 O'zbekcha

**YAY (Yangi Alifboda Yoz)** — isloh qilingan yangi o'zbek lotin alifbosi harflarini (`ş`, `ç`, `ö`, `ğ`) oddiy QWERTY klaviaturada tez yozish uchun ixcham dastur.

Dastur boshqa tillarga (`ENG`, `RUS`) xalaqit bermaydi. U faqat **Win + Space** orqali O'zbek (`UZB`) tili tanlanganda ishlaydi.

### Asosiy afzalliklari
- 🚀 **Ikki marta bosish**: `ss` → `ş`, `oo` → `ö`, `gg` → `ğ`, `cc` → `ç`.
- 🔁 **Uch marta bosish**: ikkita oddiy harf kerak bo'lsa (*Rossiya*, *issiq*, *massa*) uchinchi marta bosing: `sss` → `ss`. Enter yoki Tab tugmalari hech qachon ishlatilmaydi, shuning uchun brauzer, Telegram va har qanday formada xavfsiz.
- 🔄 **Eski usul ham ishlaydi**: `sh` → `ş`, `ch` → `ç`, `o'` → `ö`, `g'` → `ğ`.
- 🪟 **Windows 7, 8, 8.1, 10, 11** — bitta fayl, 32 va 64 bitli tizimlarda.
- 🧘 **Oddiy Windows oynasi**: tizim shriftlari va tugmalari, yuqori aniqlikdagi (DPI) ekranlarda ham tiniq.
- 🔒 **Maxfiy**: internetga ulanmaydi, hech narsa yozib olmaydi.
- 🪶 **Juda yengil**: bitta kichik `.exe`, qo'shimcha kutubxonalar yo'q.

---

## 🇬🇧 English

**YAY (Yangi Alifboda Yoz)** is a tiny native Windows input assistant for the reformed Uzbek Latin alphabet (`ş`, `ç`, `ö`, `ğ`). It sits in the tray and only acts while the **Uzbek Latin** layout is selected with **Win + Space**. English and Russian typing are untouched.

### Key features
- 🚀 **Double tap**: `ss` → `ş`, `oo` → `ö`, `gg` → `ğ`, `cc` → `ç`.
- 🔁 **Triple tap**: need the plain double letter (*Rossiya*, *issiq*)? Tap a third time: `sss` → `ss`. Enter and Tab are never intercepted, so it is safe in browsers, Telegram and any form.
- 🔄 **Legacy digraphs**: `sh` → `ş`, `ch` → `ç`, `o'` → `ö`, `g'` → `ğ` keep working.
- 🪟 **Windows 7 through 11**, one 32-bit exe for every edition.
- 🧘 **Plain Windows dialog**: standard controls, system font, crisp on high-DPI screens.
- 🔒 **Private**: no network, no logging.
- 🪶 **Lightweight**: one small `.exe`, zero runtime dependencies.

---

## ⌨️ Alifbo qoidalari / Typing rules

| Harf / Letter | Tez usul / Fast | Oddiy ikkita harf / Plain double | Eski usul / Legacy | Misol / Example |
| :---: | :---: | :---: | :---: | :--- |
| **ş** / **Ş** | `ss` / `SS` | `sss` → `ss` | `sh` / `SH` | *şahar*, *Rossiya* (`Rosssiya`) |
| **ç** / **Ç** | `cc` / `CC` | `ccc` → `cc` | `ch` / `CH` | *çoy* |
| **ö** / **Ö** | `oo` / `OO` | `ooo` → `oo` | `o'` / `O'` | *ördak*, *zoologiya* (`zooologiya`) |
| **ğ** / **Ğ** | `gg` / `GG` | `ggg` → `gg` | `g'` / `G'` | *ğalaba* |

> [!NOTE]
> **ş dan keyin s** (*işsiz*, *tuşsa*): `sh` + `s` deb yozing, yoki ikkinchi `s` dan keyin biroz kutib uchinchisini bosing.
> **ş followed by s** (*işsiz*, *tuşsa*): type `sh` + `s`, or pause briefly before the last `s`.

> [!TIP]
> **Bitta "c" rejimi**: oynadagi *"Bitta 'c' harfini darhol 'ç' qilib yozish"* sozlamasi yoqilsa, har bir `c` darhol `ç` bo'ladi.

---

## 📥 O'rnatish / Installation

### 1. O'rnatuvchi (tavsiya etiladi)
1. [`Output/YAY_Setup.exe`](Output/YAY_Setup.exe) faylini yuklab oling va ishga tushiring (O'zbek va Rus tillarida).
2. O'rnatuvchi Windows 8 va undan yangi tizimlarda **O'zbek (lotin)** tilini til paneliga qo'shadi. Windows 7 da buni dastur oynasidagi tugma bajaradi (keyin tizimdan chiqib, qayta kiring).

### 2. Portativ
[`YAY.exe`](YAY.exe) faylini istalgan papkaga joylashtirib ishga tushiring. `/tray` parametri bilan ishga tushirilsa, oyna ochilmaydi.

---

## 🖥 Tizim talablari / System requirements

- Windows 7 SP1, 8, 8.1, 10, 11 (32 yoki 64 bit).
- Qo'shimcha paketlar kerak emas (VC++ Redistributable yoki .NET talab qilinmaydi).

---

## 🛠 Yig'ish / Building from source

### MSVC (Visual Studio 2022, Desktop C++)
```bat
build.cmd
```
Skript quyidagilarni bajaradi: `tests\test_engine.cpp` testlarini ishga tushiradi, resurslarni kompilyatsiya qiladi, `YAY.exe` ni x86 va statik CRT bilan yig'adi, Inno Setup 6 topilsa `Output\YAY_Setup.exe` ni yaratadi.

### MinGW-w64
```bash
mkdir -p build
(cd res && windres yay.rc -O coff -o ../build/yay_res.o)
g++ -std=c++17 -Os -s -municode -mwindows -static -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0800 src/*.cpp build/yay_res.o -o YAY.exe -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32
```

### Ikonkalar / Icons
```bash
python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico
```

---

## 📂 Loyiha tuzilmasi / Project structure

```text
├── src/                 # C++ manba kodi: engine (qoidalar), hook, dashboard, tray, pill, layout, settings, main
├── res/                 # resource.h, yay.rc (dialog), yay.manifest, ikonkalar
├── tests/               # engine uchun konsol testlari
├── tools/make_icons.py  # Icon.png dan .ico yaratish
├── build.cmd            # MSVC yig'ish skripti
├── YAY_Setup.iss        # Inno Setup skripti (O'zbek va Rus)
├── Uzbek.isl            # Inno Setup o'zbekcha tarjimasi
├── install_uzbek_layout.ps1
├── YAY.exe              # Tayyor portativ dastur
└── Output/YAY_Setup.exe # Tayyor o'rnatuvchi
```

---

## 🔧 Texnik xususiyatlar / Technical notes

- Sof Win32 API, STL va istisnolarsiz; bitta statik `.exe`.
- Klaviatura hodisalari `WH_KEYBOARD_LL` orqali kuzatiladi, almashtirish `SendInput` bilan yuboriladi.
- Oyna — oddiy `DIALOGEX` resursi; manifest orqali Common Controls v6 va DPI (PerMonitorV2) qo'llab-quvvatlanadi.
- Ma'lum cheklov: matn maydonida avtomatik to'ldirish tanlangan bo'lsa (masalan, Chrome manzil satrida URL taklifi), yuborilgan Backspace harf o'rniga taklifni o'chirishi mumkin. Bu barcha hook asosidagi yordamchilarga xos.

---

## 🤝 Hissa qo'shish / Contributing

[CONTRIBUTING.md](CONTRIBUTING.md) faylini o'qing. Takliflar va xatoliklar uchun [Issue](https://github.com/nodiranor/yangi_alifboda_yoz/issues) oching.

**Homiy / Sponsor:** [t.me/juraboshibot](https://t.me/juraboshibot)

## 📜 Litsenziya / License

[MIT License](LICENSE).
```

- [ ] **Step 2: Update the build section of CONTRIBUTING.md**

Replace the "### 3. Dasturni kompilyatsiya qilish / Building" section with:

```markdown
### 3. Dasturni kompilyatsiya qilish / Building
MSVC (Visual Studio 2022):
```bat
build.cmd
```
Skript testlarni ishga tushiradi, `YAY.exe` ni yig'adi va Inno Setup topilsa o'rnatuvchini yaratadi.

MinGW-w64:
```bash
mkdir -p build
(cd res && windres yay.rc -O coff -o ../build/yay_res.o)
g++ -std=c++17 -Os -s -municode -mwindows -static -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0800 src/*.cpp build/yay_res.o -o YAY.exe -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32
```

Qoidalar mantiqi `src/engine.cpp` da, testlari `tests/test_engine.cpp` da. Qoidalarni o'zgartirsangiz, avval test qo'shing.
```

Also change the Prerequisites line `- **C++ Kompilyator**: MinGW-w64 (`g++` C++17) yoki MSVC (`cl.exe`).` to `- **C++ Kompilyator**: Visual Studio 2022 (MSVC) yoki MinGW-w64.` and add `- **Python (Pillow)**` stays as is.

- [ ] **Step 3: Commit**

```
git add README.md CONTRIBUTING.md
git commit -m "Docs: triple-tap rules, Windows 7-11, build script, sponsor"
```

---

### Task 9: Manual verification matrix

No files. Run `YAY.exe`, select the Uzbek layout with Win+Space, and type each row in Notepad, the Chrome address bar, Telegram Desktop and Word.

| Type | Expect |
| --- | --- |
| `ssahar` | şahar |
| `Rosssiya` | Rossiya |
| `Asssalomu alaykum` | Assalomu alaykum |
| `isssiq` | issiq |
| `ishsiz` | işsiz |
| `tushsa` | tuşsa |
| `choy`, `ccoy` | çoy, çoy |
| `o'rdak`, `oordak` | ördak, ördak |
| `g'alaba`, `ggalaba` | ğalaba, ğalaba |
| `SSAHAR` with Caps Lock | ŞAHAR |
| `ss` then Enter in Chrome search | ş is submitted, Enter is not swallowed |
| `ss` then Backspace then `s` | s |
| Ctrl+S after `s` | the app's Save dialog opens, nothing is converted |
| switch to RUS, type `сс` | untouched |

Also: tray icon turns gray on RUS and colored on UZB without any timer running while the dashboard is hidden (check with Process Explorer that CPU stays at 0%); "Windows bilan birga ishga tushirish" adds and removes the Run value; the window reopens from the tray after hiding; a second `YAY.exe` launch only brings the existing window forward.

Windows 7 cannot be exercised on this machine. Hand `YAY.exe` to the maintainer for a Windows 7 check of: themed controls, the "add Uzbek layout" button writing `HKCU\Keyboard Layout\Preload` and `Substitutes`, and UZB appearing in the language bar after sign-out.

---

## Self-review

- Spec coverage: 2.1/2.2 engine (Task 1), 2.3 removal (Task 6 deletes yay.cpp), 2.4 pill and ShowTips (Tasks 3, 4, 5), 2.5 layout fix (Task 3), 3.1 dialog (Tasks 2, 5), 3.2 manifest (Task 2, `/MANIFEST:NO` in Task 6), 4.1 Win7 (Tasks 3, 5, 7), 4.2 shell hook (Task 5), 4.3 API policy (RtlGetVersion via GetProcAddress, `/SUBSYSTEM:WINDOWS,6.01`), 5 layout and build (Tasks 2, 6), 6 sponsor and docs (Tasks 2, 5, 7, 8), 7 tests (Tasks 1, 9).
- Placeholders: none.
- Type consistency: `EngineProcess(EngineState*, EngineKeyKind, wchar_t, EngineTick)` is used identically in tests and hook; `WM_LAYOUT_INSTALL_DONE` is posted in layout.cpp and handled in dashboard.cpp; `PillCreate`/`PillDestroy`/`HidePill`/`ShowCoachingTip` match app.h; `TrayAdd`/`TrayRemove`/`UpdateTrayStatus`/`TrayShowMenu` match; `g_bWin7PreloadWritten` is declared in app.h and defined in main.cpp.

---

## Implementation notes (what changed while building)

- The exe links without the C runtime. `printf`/`wcs*` calls became `wsprintfW`/`lstrcpynW`/`lstrcmpW`, `src/nocrt.cpp` provides `memset`/`memcpy` (compiled without `/GL`), and `YayStartup` in `main.cpp` is the process entry point (`/NODEFAULTLIB /ENTRY:YayStartup`, `/GS-`). Result: 52,736 bytes instead of 322,148.
- Icons are written by `tools/make_icons.py` as palette PNG layers (the 256 px layer is 14 KB instead of 98 KB).
- Legacy digraphs (`sh`, `ch`, `o'`, `g'`) are not time limited, so slow typists still get the conversion.
- Every key YAY injects carries `YAY_EXTRA_INFO`. A test-only build (`YAY_ACCEPT_INJECTED`, built by `tests/e2e.cmd`) accepts other programs' injected keys so `tests/e2e.ps1` can type into the dashboard's test box.
- The tray double click only opens the dashboard; Windows already delivers two single clicks, which cancel each other out.
- The tray icon is re-added when Explorer restarts (`TaskbarCreated`).
- `build.cmd` stops a running `YAY.exe` so the linker can overwrite it.
