# YAY v2 — Telex-style typing engine, native dashboard, Windows 7–11

Date: 2026-10-22
Status: approved by the maintainer in the brainstorming session

## 1. Goals

1. Fix the double-tap ambiguity. Typing `ss` must never require Enter, Tab or
   Escape to recover the literal letters. Those keys already mean submit, send,
   accept-autocomplete or indent in every host application.
2. Make the dashboard look like a current Windows dialog using only built-in
   controls. No custom drawing, no images or icons inside the client area.
3. Keep the program as light as possible: one static exe, zero runtime
   dependencies, event-driven idle behaviour, smallest practical binary.
4. Support Windows 7, 8, 8.1, 10 and 11 from the same exe.
5. Show the sponsor link `https://t.me/juraboshibot` in the dashboard footer,
   the README and the installer finish page.

## 2. Typing engine

### 2.1 Rules

| Input | Output | Note |
| --- | --- | --- |
| `s` `s` | ş | second tap within the window |
| `s` `s` `s` | ss | third tap within the window cancels, case preserved |
| `s` `s` `s` `s` | sss | after a cancel, further taps pass through |
| `sh` | ş | legacy digraph, immediate |
| `ch` | ç | legacy digraph, immediate |
| `o'` / `` o` `` | ö | legacy, immediate |
| `g'` / `` g` `` | ğ | legacy, immediate |
| single `c` | ç | only when the "single c" option is on |

The same three-tap rule applies to `oo`/ö, `gg`/ğ and `cc`/ç. Capitalisation
follows the current logic: the target is uppercase when the first letter was
uppercase or Shift/Caps is active on the second tap; a cancel restores the
exact two characters that were typed.

Known trade-off: ş followed by s (işsiz, tuşsa, boşsiz) is typed as `sh` + `s`,
or by pausing longer than the window before the last `s`. Literal `ss` words
(Rossiya, assalomu, issiq, hissa, massa, kassa, klass, sessiya, professor) take
the fast path because they previously had no safe path at all.

### 2.2 State machine

One run state, shared by the four letters:

```
enum RunState { None, One, Converted, Cancelled };
struct EngineState {
  RunState state;
  wchar_t  letter;        // base letter of the run, lowercase: s o g c
  wchar_t  typed[2];      // the two characters as typed, for cancel
  DWORD    lastTime;      // tick of the last tap in the run
};
```

Decision for a key-down of letter L (s/o/g/c), with `now`:

| State | Condition | Action | Next |
| --- | --- | --- | --- |
| None / other letter / expired | — | pass through | One |
| One | same letter, within window | Backspace×1 + target | Converted |
| Converted | same letter, within window | Backspace×1 + typed[0] typed[1], suppress key | Cancelled |
| Converted | expired or other letter | pass through | One |
| Cancelled | same letter | pass through | Cancelled |
| Cancelled | other letter | pass through | One |

`h` after `s`/`c` and apostrophe or backtick after `o`/`g` apply the legacy
conversion when `state == One` and the letter matches, then reset to None.
Every other key, including Backspace, resets to None. Modifier keys alone are
ignored. Ctrl, Alt or Win combinations reset to None and pass through.

Window: `DOUBLE_TAP_MS = 350`, used for both the second and the third tap.

The engine is a pure function with no Win32 calls so it can be unit-tested:

```
enum Action { Pass, Suppress, ReplaceLast };   // ReplaceLast = Backspace + text
struct Decision { Action action; wchar_t text[3]; const wchar_t *tip; };
Decision EngineProcess(EngineState &st, wchar_t typedChar, KeyKind kind, DWORD now);
```

`tip` is non-null after a legacy conversion and names the faster double-tap
("ss", "SS", "cc", "oo", "gg").

### 2.3 Removed

Enter/Tab/Escape handling, the undo pill window, `TIMER_CHOICE`, the
`g_undo*` globals and `ShowUndoPopup`.

### 2.4 Coach pill

Kept. Shown after a legacy conversion, text "Maslahat: 'ss' tezroq", auto-hides
after 4 s or on the next key. Controlled by a new setting `ShowTips`
(REG_DWORD, default 1) exposed as the checkbox "Eski usulda yozganda maslahat
ko'rsatish".

### 2.5 Layout detection fix

Permissive mode activates only on English (`PRIMARYLANGID == LANG_ENGLISH`) or
Uzbek Latin, not on every non-Russian layout.

## 3. Dashboard

### 3.1 Native dialog

The main window becomes a `DIALOGEX` resource, `FONT 9, "Segoe UI"` with
`DS_SHELLFONT | DS_CENTER`, created with `CreateDialogParamW` as a modeless
dialog. The message loop calls `IsDialogMessageW` so Tab, Space, Escape and
Alt+letter mnemonics behave like every Windows dialog. Escape and the close
button hide to tray.

Layout, top to bottom, in dialog units:

```
YAY — Yangi Alifboda Yoz                                   [—][✕]
 Holat: Faol — UZB (O'zbek lotin) tanlangan            (IDC_STATUS_TEXT)
 Tilni almashtirish: Win + Space                       (IDC_LANG_TEXT)
 [ O'zbek (lotin) tilini til paneliga qo'shish ]       (IDC_BTN_ADD_UZBEK)

 ┌ Qoidalar ────────────────────────────────────────────────┐
 │  ss → ş     oo → ö     gg → ğ     cc → ç                 │
 │  Uch marta bossangiz asl harflar qoladi: sss → ss        │
 │  Eski usul ham ishlaydi: sh → ş, ch → ç, o' → ö, g' → ğ  │
 │  Sinab ko'ring: [________________________]  (IDC_EDIT_TEST)
 └──────────────────────────────────────────────────────────┘
 ┌ Sozlamalar ──────────────────────────────────────────────┐
 │  [x] Faqat O'zbek (UZB) tili tanlanganda ishlash  (IDC_CHK_ONLY_UZ)
 │  [ ] Bitta 'c' harfini darhol 'ç' qilib yozish    (IDC_CHK_SINGLE_C)
 │  [x] Eski usulda yozganda maslahat ko'rsatish     (IDC_CHK_TIPS)
 │  [x] Windows bilan birga ishga tushirish          (IDC_CHK_STARTUP)
 └──────────────────────────────────────────────────────────┘
 Homiy: t.me/juraboshibot (SysLink)   [Trayga yashirish] [Chiqish]
```

All text is Uzbek Latin. The tray menu becomes Uzbek Latin only. The installer
keeps its Uzbek and Russian wizard languages.

The sponsor SysLink (`IDC_SPONSOR`) opens `https://t.me/juraboshibot` with
`ShellExecuteW` on `NM_CLICK` / `NM_RETURN`. `InitCommonControlsEx` adds
`ICC_LINK_CLASS`.

### 3.2 Manifest

`res/yay.manifest`, embedded through the .rc as `1 RT_MANIFEST`:

- `Microsoft.Windows.Common-Controls` 6.0.0.0 dependency (themed controls)
- `requestedExecutionLevel level="asInvoker"`
- `supportedOS` GUIDs for Windows 7, 8, 8.1 and 10/11
- `dpiAware` true/pm and `dpiAwareness` PerMonitorV2, system

Under PerMonitorV2 Windows rescales dialog templates on DPI change, so no
manual DPI code is required. Windows 7 gets system-DPI scaling from the same
manifest.

The MSVC link step uses `/MANIFEST:NO` so the linker does not generate a
second manifest.

## 4. Platform

### 4.1 Windows 7 layout install

`IsWindows8OrGreater()` is evaluated through `RtlGetVersion` so the manifest
does not mask the real version.

- Windows 8 and later: existing hidden PowerShell `Set-WinUserLanguageList`
  path, unchanged.
- Windows 7: append `00000443` as the next numbered value under
  `HKCU\Keyboard Layout\Preload`, call
  `LoadKeyboardLayoutW(L"00000443", KLF_ACTIVATE | KLF_SUBSTITUTE_OK)` for the
  current session, then show a message box: "O'zbek (lotin) tili qo'shildi.
  Til panelida ko'rinishi uchun tizimdan chiqib, qayta kiring." The button
  then reads "Qayta kirgach UZB paydo bo'ladi" and is disabled.

The installer's PowerShell `[Run]` entry gets `MinVersion: 6.2` so it is
skipped on Windows 7.

### 4.2 Language tracking

The 250 ms `TIMER_CHECK_LANG` is removed. The dashboard window calls
`RegisterShellHookWindow` and handles `WM_SHELLHOOKMESSAGE` with
`HSHELL_LANGUAGE` and `HSHELL_WINDOWACTIVATED` by calling `UpdateTrayStatus`.
While the dashboard is visible a 1000 ms timer refreshes the status text as a
fallback; it is killed when the window hides.

### 4.3 API policy

Every API newer than Windows 7 is loaded with `GetProcAddress` and skipped when
absent. The link step sets `/SUBSYSTEM:WINDOWS,6.01`.

## 5. Source layout and build

```
src/engine.h / engine.cpp     pure typing state machine (no Win32)
src/inject.cpp                SendInput helpers
src/layout.cpp                layout detection, Win7/Win8+ layout install
src/settings.cpp              registry settings and startup entry
src/tray.cpp                  tray icon and menu
src/pill.cpp                  coach pill window
src/dashboard.cpp             dialog procedure
src/main.cpp                  wWinMain, single instance, hook, message loop
src/app.h                     shared globals and declarations
res/yay.rc, res/yay.manifest, res/resource.h, res/app.ico, res/app_paused.ico
tests/test_engine.cpp         console unit tests for engine.cpp
build.cmd                     MSVC build: resources, tests, exe, optional ISCC
YAY_Setup.iss, Uzbek.isl, install_uzbek_layout.ps1
```

MSVC flags: `/O1 /Os /GL /MT /GS- /EHs-c- /DUNICODE /D_UNICODE
/D_WIN32_WINNT=0x0601`, link `/OPT:REF /OPT:ICF /LTCG /MANIFEST:NO
/SUBSYSTEM:WINDOWS,6.01`. The MinGW command stays documented in README.

Icons: `app.ico` and `app_paused.ico` are re-encoded with Pillow so the 256 px
layer is PNG-compressed and only 16/20/24/32/48/256 sizes are kept. `Icon.png`
stays as the source. Target exe size: under 150 KB.

## 6. Sponsor and documentation

- Dashboard footer SysLink, see 3.1.
- README: sponsor badge under the title, rewritten rules table (three-tap rule
  and the ş+s note), Windows 7–11 badge, MSVC build script instructions, Enter
  undo text removed.
- Installer: a clickable "Homiy: t.me/juraboshibot" `TNewStaticText` on the
  finish page via `[Code]`, opening the URL with `ShellExec`.

## 7. Testing

- `tests/test_engine.cpp` drives `EngineProcess` through: issiq, Rossiya,
  assalomu, massa, işsiz via `sh`, tuşsa with a pause, şahar via `ss` and via
  `sh`, çoy, ördek via `oo` and `o'`, ğalaba via `gg` and `g'`, uppercase
  Ş/SS/Ss, Backspace reset, window expiry, and Ctrl combinations. The build
  script runs it and stops on failure.
- Manual: Notepad, Chrome address bar, Telegram Desktop, Word.
- Windows 7 cannot be exercised on the development machine; the maintainer
  tests the exe on a Windows 7 machine or VM.

## 8. Out of scope

Dark mode for the dialog (native controls have no supported dark theme), a
dictionary-based disambiguation, long-press input, and any change to Russian
or English layouts.
