// hook.cpp - the low-level keyboard hook, the heart of YAY.
//
// Windows calls LowLevelKeyboardProc for every key press in every program, before the program
// sees it. For each key we decide one of two things:
//
//   * return CallNextHookEx(...)   let the key through unchanged (what happens to almost every key)
//   * return 1                     swallow the key; we typed something in its place instead
//
// The decision depends on the keyboard layout of the window you are typing into
// (GetActiveProfile in layout.cpp):
//
//   AP_NONE      not our layout (for example plain English or Russian): do nothing at all
//   AP_LATIN     the Latin layout: feed the key to the double-tap engine (engine.cpp)
//   AP_CYRILLIC  a Cyrillic layout: convert the letter to Latin (translit.cpp)
//
// Rules for code in this file:
//   * It runs inside the system's input path. Keep it fast; if a hook is too slow, Windows
//     silently stops calling it (see the LowLevelHooksTimeout registry value).
//   * It runs on the thread that installed it (the main thread), through its message loop. It
//     needs no locking, and it may call SendInput.
//   * Never keep or log what the user types. Only the last couple of letters are remembered, in
//     the engine and translit states, and only to make the next decision.
#include "app.h"

// True for keys that change the meaning of the next key but type nothing themselves.
static bool IsModifierVk(DWORD vk) {
  return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
         vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
         vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
         vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN;
}

// Is this key physically down right now? (Bit 15 of GetAsyncKeyState.)
static bool KeyHeld(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

// Cyrillic layout: turn the Cyrillic letter this key produces into its new Latin form.
// Returns true when the key was replaced and must not reach the application.
static bool TransliterateKey(const KBDLLHOOKSTRUCT *k, HKL layout) {
  // Caps Lock on, Shift not held: the user is typing a run of capitals. translit.cpp uses this
  // to write "YU" instead of "Yu" for a capital Ю.
  const bool capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

  // Which Cyrillic letter does this key give on the active layout, in the case the user wants?
  const wchar_t cyr = CyrillicCharForKey(k->vkCode, k->scanCode, IsCapsActive(), layout);
  if (cyr) {
    const bool allCaps = capsLockOn && !KeyHeld(VK_SHIFT);
    wchar_t text[4];
    const int n = TranslitLetter(&g_translit, cyr, allCaps, text);
    if (n >= 0) {
      // n == 0 is the soft sign: it types nothing, but the key is still swallowed.
      // No Backspace is needed: the Cyrillic key never reached the application.
      if (n > 0) SendBackspaceAndChars(0, text);
      return true;
    }
  }
  // Not a letter we convert (space, punctuation, arrows...): it ends the word.
  TranslitReset(&g_translit);
  return false;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  // nCode < 0 means "pass it on without looking" (a Windows rule for all hooks).
  // g_bSendingInput and g_bEnabled: we are in the middle of typing, or the user paused YAY.
  if (nCode != HC_ACTION || g_bSendingInput || !g_bEnabled)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  const KBDLLHOOKSTRUCT *k = (const KBDLLHOOKSTRUCT *)lParam;

  // Only key presses matter. WM_SYSKEYDOWN is a key pressed while Alt is held.
  const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

  // Keys that another program injected (SendInput, on-screen keyboards, macro tools) are not
  // real typing, and our own replacement text must never be processed again, or it would loop.
#ifdef YAY_ACCEPT_INJECTED
  // Test builds also process keys injected by other programs (tests/e2e.ps1), but never YAY's own.
  const bool skipInjected = (k->flags & LLKHF_INJECTED) && k->dwExtraInfo == YAY_EXTRA_INFO;
#else
  const bool skipInjected = (k->flags & LLKHF_INJECTED) != 0;
#endif
  if (!isDown || skipInjected)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  // Which converter, if any, applies to the window the user is typing into?
  HKL layout = NULL;
  const ActiveProfile profile = GetActiveProfile(&layout);
  if (profile == AP_NONE)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  // The user switched between the Latin and Cyrillic layouts (or came from another layout):
  // forget half-finished work in both converters.
  static ActiveProfile lastProfile = AP_NONE;
  if (profile != lastProfile) {
    lastProfile = profile;
    EngineReset(&g_engine);
    TranslitReset(&g_translit);
  }

  // Shift, Ctrl, Alt, Caps Lock and Win on their own never type anything and never end a word.
  const DWORD vk = k->vkCode;
  if (IsModifierVk(vk))
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  // A shortcut such as Ctrl+C, Alt+Tab or Win+E. These must always reach the application
  // untouched, so we never convert them.
  const bool combo = (wParam == WM_SYSKEYDOWN || KeyHeld(VK_CONTROL) || KeyHeld(VK_MENU) || KeyHeld(VK_LWIN) || KeyHeld(VK_RWIN));

  // --- Cyrillic layout: replace each Cyrillic letter with its Latin form -------------------
  if (profile == AP_CYRILLIC) {
    if (combo) {
      TranslitReset(&g_translit);
    } else if (vk == VK_BACK) {
      // Backspace deletes a character in the application; keep our memory of the text in step.
      TranslitBackspace(&g_translit);
    } else if (TransliterateKey(k, layout)) {
      return 1;  // swallowed: the Latin text has been typed instead
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
  }

  // --- Latin layout: the double-tap engine --------------------------------------------------
  // Describe the key to the engine (engine.h explains the kinds).
  EngineKeyKind kind;
  wchar_t ch = 0;
  if (combo) {
    kind = EK_COMBO;
  } else if (vk >= 'A' && vk <= 'Z') {
    // The virtual-key codes for A-Z are the capital ASCII letters, so vk + 32 is the lowercase one.
    kind = EK_LETTER;
    ch = IsCapsActive() ? (wchar_t)vk : (wchar_t)(vk + 32);
  } else if ((vk == VK_OEM_7 || vk == VK_OEM_3) && !KeyHeld(VK_SHIFT)) {
    // The ' key (VK_OEM_7) and the ` key (VK_OEM_3) on a US keyboard, without Shift. Both are
    // accepted as the apostrophe in o' and g'.
    kind = EK_APOSTROPHE;
  } else {
    kind = EK_OTHER;
  }

  // Any new key press dismisses the hint popup right away, so it never covers what is being typed.
  HidePill();

  // Optional setting: a lone c always types ç, with no double tap needed.
  if (g_bInstantSingleC && kind == EK_LETTER && (ch == L'c' || ch == L'C')) {
    EngineReset(&g_engine);
    SendBackspaceAndChars(0, ch == L'C' ? L"Ç" : L"ç");
    return 1;
  }

  EngineDecision d = EngineProcess(&g_engine, kind, ch, GetTickCount());
  if (d.action == EA_REPLACE) {
    // Delete the first letter of the pair (already in the application), type the new one, and
    // swallow the key that triggered all this.
    SendBackspaceAndChars(d.backspaces, d.text);
    if (d.tip && g_bShowTips) ShowCoachingTip(d.tip);
    return 1;
  }
  return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}
