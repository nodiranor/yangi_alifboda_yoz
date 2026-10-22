#include "app.h"

// Sends `backspaces` Backspace presses followed by `chars` as Unicode key events.
// g_bSendingInput is only a belt-and-braces guard; injected events also carry LLKHF_INJECTED.
void SendBackspaceAndChars(int backspaces, const wchar_t *chars) {
  g_bSendingInput = true;
  INPUT inputs[16] = {0};
  int idx = 0;

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

  SendInput(idx, inputs, sizeof(INPUT));
  g_bSendingInput = false;
}

// Uppercase when exactly one of Shift / Caps Lock is active.
bool IsCapsActive() {
  bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
  bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
  return shift != caps;
}
