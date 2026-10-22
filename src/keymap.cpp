// keymap.cpp - see keymap.h.
#include "keymap.h"

wchar_t CyrillicCharForKey(UINT vk, UINT scanCode, bool upper, HKL hkl) {
  // ToUnicodeEx needs the state of all 256 keys. Only Shift matters for picking a letter, so we
  // describe a keyboard where nothing is down except, if wanted, Shift (0x80 = "down").
  BYTE state[256] = {0};
  if (upper) state[VK_SHIFT] = 0x80;

  // Returns the number of UTF-16 units written. 1 means "this key types exactly one character".
  // Anything else (0 = no character, negative = dead key, 2 = two characters) is not a letter for us.
  wchar_t buf[4] = {0};
  int n = ToUnicodeEx(vk, scanCode, state, buf, 4, 0, hkl);

  // U+0400 to U+04FF is the Unicode "Cyrillic" block: Russian letters and the Uzbek extras.
  if (n == 1 && buf[0] >= 0x0400 && buf[0] <= 0x04FF) return buf[0];
  return 0;
}
