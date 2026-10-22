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

  const DWORD vk = k->vkCode;
  EngineKeyKind kind;
  wchar_t ch = 0;
  if (IsModifierVk(vk)) {
    kind = EK_MODIFIER;
  } else if (wParam == WM_SYSKEYDOWN || KeyHeld(VK_CONTROL) || KeyHeld(VK_MENU) || KeyHeld(VK_LWIN) || KeyHeld(VK_RWIN)) {
    kind = EK_COMBO;
  } else if (profile == AP_CYRILLIC) {
    // Ask Windows which letter this key produces on the active layout.
    ch = CyrillicCharForKey(vk, k->scanCode, IsCapsActive(), layout);
    kind = ch ? EK_LETTER : EK_OTHER;
  } else if (vk >= 'A' && vk <= 'Z') {
    kind = EK_LETTER;
    ch = IsCapsActive() ? (wchar_t)vk : (wchar_t)(vk + 32);
  } else if ((vk == VK_OEM_7 || vk == VK_OEM_3) && !KeyHeld(VK_SHIFT)) {
    kind = EK_APOSTROPHE;
  } else {
    kind = EK_OTHER;
  }

  EngineSetProfile(&g_engine, profile == AP_CYRILLIC ? EP_CYRILLIC : EP_LATIN);
  if (kind != EK_MODIFIER) HidePill();

  if (profile == AP_LATIN && g_bInstantSingleC && kind == EK_LETTER && (ch == L'c' || ch == L'C')) {
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
