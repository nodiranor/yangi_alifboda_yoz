// keymap.h - asking Windows which letter a key produces.
#pragma once
#include <windows.h>

// Returns the Cyrillic letter that key `vk` produces on keyboard layout `hkl`
// (upper or lower case as asked), or 0 if the key does not produce a single Cyrillic letter.
//
// Why ask Windows instead of keeping our own table? The Russian layout and the Uzbek (Cyrillic)
// layout put letters on different keys (the Uzbek layout has ў қ ғ ҳ), and users can install
// other variants. Windows always knows what the active layout does.
//
// Only call it on the Russian and Uzbek (Cyrillic) layouts. They have no dead keys, so asking
// Windows for the character does not disturb any typing state. Other layouts may have dead
// keys, and ToUnicodeEx could swallow one.
//
//   vk        virtual-key code of the key (KBDLLHOOKSTRUCT::vkCode)
//   scanCode  hardware scan code of the key (KBDLLHOOKSTRUCT::scanCode)
//   upper     true to get the capital letter (as if Shift were held)
//   hkl       the keyboard layout of the window being typed into
wchar_t CyrillicCharForKey(UINT vk, UINT scanCode, bool upper, HKL hkl);
