// inject.cpp - typing text into the focused application with SendInput.
#include "app.h"

// Presses Backspace `backspaces` times, then types `chars`.
//
// The characters are sent as Unicode events (KEYEVENTF_UNICODE): wScan carries the UTF-16 code
// unit itself and Windows delivers it as a typed character. That is why ş, ç, ö and ğ work
// whatever keyboard layout is active, with no keyboard-layout tricks.
//
// Limits: at most 3 Backspaces and 4 characters per call, which fits the 16-slot buffer
// (2 events per key). The callers ask for at most 1 Backspace and 2 characters.
//
// Our own events carry LLKHF_INJECTED, so the hook skips them (that is the real guard against a
// loop). g_bSendingInput is a second guard and is only true while this function runs.
void SendBackspaceAndChars(int backspaces, const wchar_t *chars) {
  g_bSendingInput = true;
  INPUT inputs[16] = {0};
  int idx = 0;

  // Each Backspace is a key-down event followed by a key-up event.
  // YAY_EXTRA_INFO marks every event as ours (the test build relies on it, see hook.cpp).
  for (int i = 0; i < backspaces && idx < 6; i++) {
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.dwExtraInfo = YAY_EXTRA_INFO;
    inputs[idx].ki.wVk = VK_BACK;
    idx++;
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.dwExtraInfo = YAY_EXTRA_INFO;
    inputs[idx].ki.wVk = VK_BACK;
    inputs[idx].ki.dwFlags = KEYEVENTF_KEYUP;
    idx++;
  }

  // Then every character, again as a down event and an up event.
  for (int i = 0; chars[i] != 0 && idx < 14; i++) {
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.dwExtraInfo = YAY_EXTRA_INFO;
    inputs[idx].ki.wScan = chars[i];
    inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE;
    idx++;
    inputs[idx].type = INPUT_KEYBOARD;
    inputs[idx].ki.dwExtraInfo = YAY_EXTRA_INFO;
    inputs[idx].ki.wScan = chars[i];
    inputs[idx].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    idx++;
  }

  // All events go in one call, so nothing the user types can slip in between them.
  SendInput(idx, inputs, sizeof(INPUT));
  g_bSendingInput = false;
}

// Letters come out as capitals when Shift is held or Caps Lock is on, but not both
// (Shift cancels Caps Lock, as on a real keyboard).
// Bit 15 of GetAsyncKeyState is "down now"; bit 0 of GetKeyState is the Caps Lock toggle.
bool IsCapsActive() {
  bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
  bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
  return shift != caps;
}
