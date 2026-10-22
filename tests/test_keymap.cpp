// Checks CyrillicCharForKey against the Russian and Uzbek (Cyrillic) keyboard layouts that ship
// with Windows. Built and run by build.cmd.
#include "../src/keymap.h"
#include <stdio.h>

static int g_failures = 0;

struct Case {
  UINT vk;
  bool upper;
  wchar_t want;
  const char *name;
};

static void RunCases(HKL layout, const Case *cases, unsigned count) {
  for (unsigned i = 0; i < count; i++) {
    UINT scan = MapVirtualKeyExW(cases[i].vk, MAPVK_VK_TO_VSC, layout);
    wchar_t got = CyrillicCharForKey(cases[i].vk, scan, cases[i].upper, layout);
    if (got == cases[i].want) {
      printf("  ok    %s\n", cases[i].name);
    } else {
      g_failures++;
      printf("  FAIL  %s: expected U+%04X got U+%04X\n", cases[i].name, (unsigned)cases[i].want, (unsigned)got);
    }
  }
}

int main() {
  printf("keymap tests\n");

  HKL ru = LoadKeyboardLayoutW(L"00000419", 0);
  if (ru) {
    const Case cases[] = {
      { 'E',       false, 0x0443, "RU: E -> u" },
      { 'E',       true,  0x0423, "RU: Shift+E -> U" },
      { 'I',       false, 0x0448, "RU: I -> sh" },
      { 'X',       false, 0x0447, "RU: X -> ch" },
      { VK_OEM_4,  false, 0x0445, "RU: [ -> kh" },
      { VK_OEM_4,  true,  0x0425, "RU: Shift+[ -> KH" },
      { VK_SPACE,  false, 0,      "RU: Space -> nothing" },
      { '1',       false, 0,      "RU: 1 -> nothing" },
      { VK_RETURN, false, 0,      "RU: Enter -> nothing" },
    };
    RunCases(ru, cases, sizeof(cases) / sizeof(cases[0]));
  } else {
    printf("  skipped: Russian layout is not available\n");
  }

  HKL uz = LoadKeyboardLayoutW(L"00000843", 0);
  if (uz) {
    const Case cases[] = {
      { 'O',        false, 0x045E, "UZ Cyrillic: O -> u-breve" },
      { 'O',        true,  0x040E, "UZ Cyrillic: Shift+O -> U-breve" },
      { 'S',        false, 0x049B, "UZ Cyrillic: S -> qa" },
      { 'S',        true,  0x049A, "UZ Cyrillic: Shift+S -> QA" },
      { VK_OEM_PLUS,  false, 0x04B3, "UZ Cyrillic: = -> ha" },
      { VK_OEM_PLUS,  true,  0x04B2, "UZ Cyrillic: Shift+= -> HA" },
      { VK_OEM_MINUS, false, 0x0493, "UZ Cyrillic: - -> ghayn" },
      { VK_OEM_MINUS, true,  0x0492, "UZ Cyrillic: Shift+- -> GHAYN" },
    };
    RunCases(uz, cases, sizeof(cases) / sizeof(cases[0]));
  } else {
    printf("  skipped: Uzbek (Cyrillic) layout is not available\n");
  }

  if (g_failures) printf("%d FAILED\n", g_failures);
  else printf("all passed\n");
  return g_failures ? 1 : 0;
}
