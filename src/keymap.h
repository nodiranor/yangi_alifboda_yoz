#pragma once
#include <windows.h>

// Returns the Cyrillic letter that key `vk` produces on keyboard layout `hkl`
// (upper or lower case as asked), or 0 if the key does not produce a single Cyrillic letter.
// Used on the Russian and Uzbek (Cyrillic) layouts, which have no dead keys, so asking Windows
// for the character does not disturb any typing state.
wchar_t CyrillicCharForKey(UINT vk, UINT scanCode, bool upper, HKL hkl);
