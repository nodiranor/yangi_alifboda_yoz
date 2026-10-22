// Checks CyrillicCharForKey against the Russian keyboard layout that ships with Windows.
// Build and run with build.cmd.
#include "../src/keymap.h"
#include <stdio.h>

static int g_failures = 0;

struct Case {
  UINT vk;
  bool upper;
  wchar_t want;
  const char *name;
};

int main() {
  printf("keymap tests\n");
  HKL ru = LoadKeyboardLayoutW(L"00000419", 0);
  if (!ru) {
    printf("  skipped: Russian layout is not available\n");
    return 0;
  }

  const Case cases[] = {
    { 'E',       false, 0x0443, "E -> u" },
    { 'E',       true,  0x0423, "Shift+E -> U" },
    { 'R',       false, 0x043A, "R -> k" },
    { 'R',       true,  0x041A, "Shift+R -> K" },
    { 'U',       false, 0x0433, "U -> g" },
    { 'U',       true,  0x0413, "Shift+U -> G" },
    { VK_OEM_4,  false, 0x0445, "[ -> kh" },
    { VK_OEM_4,  true,  0x0425, "Shift+[ -> KH" },
    { 'A',       false, 0x0444, "A -> ef" },
    { VK_SPACE,  false, 0,      "Space -> nothing" },
    { '1',       false, 0,      "1 -> nothing" },
    { VK_RETURN, false, 0,      "Enter -> nothing" },
  };

  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    UINT scan = MapVirtualKeyExW(cases[i].vk, MAPVK_VK_TO_VSC, ru);
    wchar_t got = CyrillicCharForKey(cases[i].vk, scan, cases[i].upper, ru);
    if (got == cases[i].want) {
      printf("  ok    %s\n", cases[i].name);
    } else {
      g_failures++;
      printf("  FAIL  %s: expected U+%04X got U+%04X\n", cases[i].name, (unsigned)cases[i].want, (unsigned)got);
    }
  }

  if (g_failures) printf("%d FAILED\n", g_failures);
  else printf("all passed\n");
  return g_failures ? 1 : 0;
}
