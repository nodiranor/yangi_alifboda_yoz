#include "keymap.h"

wchar_t CyrillicCharForKey(UINT vk, UINT scanCode, bool upper, HKL hkl) {
  BYTE state[256] = {0};
  if (upper) state[VK_SHIFT] = 0x80;
  wchar_t buf[4] = {0};
  int n = ToUnicodeEx(vk, scanCode, state, buf, 4, 0, hkl);
  if (n == 1 && buf[0] >= 0x0400 && buf[0] <= 0x04FF) return buf[0];
  return 0;
}
