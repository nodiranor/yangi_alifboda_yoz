#include "app.h"

static bool IsModifierVk(DWORD vk) {
  return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
         vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
         vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
         vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN;
}

static bool KeyHeld(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

// Cyrillic layout: turn the Cyrillic letter this key produces into its new Latin form.
// Returns true when the key was replaced and must not reach the application.
static bool TransliterateKey(const KBDLLHOOKSTRUCT *k, HKL layout) {
  const bool capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
  const wchar_t cyr = CyrillicCharForKey(k->vkCode, k->scanCode, IsCapsActive(), layout);
  if (cyr) {
    const bool allCaps = capsLockOn && !KeyHeld(VK_SHIFT);
    wchar_t text[4];
    const int n = TranslitLetter(&g_translit, cyr, allCaps, text);
    if (n >= 0) {
      if (n > 0) SendBackspaceAndChars(0, text);
      return true;
    }
  }
  TranslitReset(&g_translit);  // space, punctuation, arrows and so on end the word
  return false;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode != HC_ACTION || g_bSendingInput || !g_bEnabled)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  const KBDLLHOOKSTRUCT *k = (const KBDLLHOOKSTRUCT *)lParam;
  const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
#ifdef YAY_ACCEPT_INJECTED
  // Test builds also process keys injected by other programs (tests/e2e.ps1), but never YAY's own.
  const bool skipInjected = (k->flags & LLKHF_INJECTED) && k->dwExtraInfo == YAY_EXTRA_INFO;
#else
  const bool skipInjected = (k->flags & LLKHF_INJECTED) != 0;
#endif
  if (!isDown || skipInjected)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  HKL layout = NULL;
  const ActiveProfile profile = GetActiveProfile(&layout);
  if (profile == AP_NONE)
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  // Switching between the Latin and Cyrillic layouts starts both converters fresh.
  static ActiveProfile lastProfile = AP_NONE;
  if (profile != lastProfile) {
    lastProfile = profile;
    EngineReset(&g_engine);
    TranslitReset(&g_translit);
  }

  const DWORD vk = k->vkCode;
  if (IsModifierVk(vk))
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);

  const bool combo = (wParam == WM_SYSKEYDOWN || KeyHeld(VK_CONTROL) || KeyHeld(VK_MENU) || KeyHeld(VK_LWIN) || KeyHeld(VK_RWIN));

  if (profile == AP_CYRILLIC) {
    if (combo) {
      TranslitReset(&g_translit);
    } else if (vk == VK_BACK) {
      TranslitBackspace(&g_translit);
    } else if (TransliterateKey(k, layout)) {
      return 1;
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
  }

  // Latin layout: the double-tap engine.
  EngineKeyKind kind;
  wchar_t ch = 0;
  if (combo) {
    kind = EK_COMBO;
  } else if (vk >= 'A' && vk <= 'Z') {
    kind = EK_LETTER;
    ch = IsCapsActive() ? (wchar_t)vk : (wchar_t)(vk + 32);
  } else if ((vk == VK_OEM_7 || vk == VK_OEM_3) && !KeyHeld(VK_SHIFT)) {
    kind = EK_APOSTROPHE;
  } else {
    kind = EK_OTHER;
  }

  HidePill();

  if (g_bInstantSingleC && kind == EK_LETTER && (ch == L'c' || ch == L'C')) {
    EngineReset(&g_engine);
    SendBackspaceAndChars(0, ch == L'C' ? L"Ç" : L"ç");
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
