// Console tests for src/engine.cpp. Build and run with build.cmd, or:
//   cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /Fobuild\ /Febuild\test_engine.exe tests\test_engine.cpp src\engine.cpp
// Covered: double tap, triple tap, capitals, the old forms (sh ch o' g'), the timing window,
// resets by other keys, and the hint text. Add a case here before changing a rule in engine.cpp.
#include "../src/engine.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define S_LO L"\x015F"
#define S_UP L"\x015E"
#define C_LO L"\x00E7"
#define C_UP L"\x00C7"
#define O_LO L"\x00F6"
#define O_UP L"\x00D6"
#define G_LO L"\x011F"
#define G_UP L"\x011E"

static int g_failures = 0;

static void PrintW(const wchar_t *s) {
  for (; *s; s++) {
    if (*s < 128) putchar((int)*s);
    else printf("\u%04X", (unsigned)*s);
  }
}

// Key script: letters -> EK_LETTER with that case; ' -> EK_APOSTROPHE; space -> EK_OTHER
// (and a space in the text); < -> Backspace (EK_OTHER, deletes one char); | -> 1000 ms pause;
// ^ -> a Ctrl combination; _ -> Shift pressed alone. Every key advances the clock by 100 ms.
static void Simulate(const char *keys, wchar_t *out, size_t outCap, EngineDecision *lastDecision) {
  EngineState st;
  EngineInit(&st, 350);
  EngineTick now = 10000;
  size_t len = 0;
  out[0] = 0;
  EngineDecision d = { EA_PASS, 0, {0, 0, 0}, 0 };
  for (const char *p = keys; *p; p++) {
    char c = *p;
    if (c == '|') { now += 1000; continue; }
    now += 100;
    EngineKeyKind kind;
    wchar_t ch = 0;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { kind = EK_LETTER; ch = (wchar_t)c; }
    else if (c == '\'') kind = EK_APOSTROPHE;
    else if (c == '_') kind = EK_MODIFIER;
    else if (c == '^') kind = EK_COMBO;
    else kind = EK_OTHER;
    if (c == '<' && len > 0) { len--; out[len] = 0; }
    d = EngineProcess(&st, kind, ch, now);
    if (d.action == EA_PASS) {
      wchar_t emit = 0;
      if (kind == EK_LETTER) emit = ch;
      else if (kind == EK_APOSTROPHE) emit = L'\'';
      else if (c == ' ') emit = L' ';
      if (emit && len + 1 < outCap) { out[len++] = emit; out[len] = 0; }
    } else {
      for (int i = 0; i < d.backspaces && len > 0; i++) len--;
      for (int i = 0; d.text[i] && len + 1 < outCap; i++) out[len++] = d.text[i];
      out[len] = 0;
    }
  }
  if (lastDecision) *lastDecision = d;
}

static void Expect(const char *keys, const wchar_t *expected) {
  wchar_t out[64];
  Simulate(keys, out, 64, 0);
  if (wcscmp(out, expected) == 0) {
    printf("  ok    %s\n", keys);
  } else {
    g_failures++;
    printf("  FAIL  %s  expected [", keys); PrintW(expected);
    printf("] got ["); PrintW(out); printf("]\n");
  }
}

static void ExpectTip(const char *keys, const wchar_t *expectedTip) {
  wchar_t out[64];
  EngineDecision d;
  Simulate(keys, out, 64, &d);
  bool ok = (expectedTip == 0) ? (d.tip == 0) : (d.tip != 0 && wcscmp(d.tip, expectedTip) == 0);
  if (ok) printf("  ok    tip %s\n", keys);
  else { g_failures++; printf("  FAIL  tip %s  expected [", keys); if (expectedTip) PrintW(expectedTip); printf("] got ["); if (d.tip) PrintW(d.tip); printf("]\n"); }
}

static void ExpectWindow(EngineTick gap, bool shouldConvert) {
  EngineState st;
  EngineInit(&st, 350);
  EngineProcess(&st, EK_LETTER, L's', 1000);
  EngineDecision d = EngineProcess(&st, EK_LETTER, L's', 1000 + gap);
  bool converted = (d.action == EA_REPLACE);
  if (converted == shouldConvert) printf("  ok    window gap %lu\n", gap);
  else { g_failures++; printf("  FAIL  window gap %lu: converted=%d expected=%d\n", gap, (int)converted, (int)shouldConvert); }
}

int main() {
  printf("engine tests\n");

  // Double tap and case
  Expect("ss", S_LO);
  Expect("SS", S_UP);
  Expect("Ss", S_UP);
  Expect("sS", S_UP);
  Expect("oo", O_LO);
  Expect("gg", G_LO);
  Expect("cc", C_LO);

  // Triple tap cancels, further taps are literal
  Expect("sss", L"ss");
  Expect("SSS", L"SS");
  Expect("Sss", L"Ss");
  Expect("ssss", L"sss");
  Expect("ooo", L"oo");
  Expect("ggg", L"gg");
  Expect("ccc", L"cc");

  // Real words
  Expect("isssiq", L"issiq");
  Expect("issiq", L"i" S_LO L"iq");
  Expect("Rosssiya", L"Rossiya");
  Expect("Asssalomu", L"Assalomu");
  Expect("ishsiz", L"i" S_LO L"siz");
  Expect("iss|siz", L"i" S_LO L"siz");
  Expect("tushsa", L"tu" S_LO L"sa");
  Expect("tusssa", L"tussa");               // documented trade-off: sss is always ss
  Expect("shahar", S_LO L"ahar");
  Expect("ssahar", S_LO L"ahar");
  Expect("Shahar", S_UP L"ahar");
  Expect("SHAHAR", S_UP L"AHAR");
  Expect("choy", C_LO L"oy");
  Expect("ccoy", C_LO L"oy");
  Expect("o'rdak", O_LO L"rdak");
  Expect("oordak", O_LO L"rdak");
  Expect("O'rdak", O_UP L"rdak");
  Expect("g'alaba", G_LO L"alaba");
  Expect("ggalaba", G_LO L"alaba");
  Expect("Toshkent", L"To" S_LO L"kent");

  // Resets and non-conversions
  Expect("s|s", L"ss");
  Expect("ss<s", L"s");
  Expect("s^s", L"ss");
  Expect("s_s", S_LO);
  Expect("s s", L"s s");
  Expect("ma'no", L"ma'no");
  Expect("s'", L"s'");
  Expect("oh", L"oh");
  Expect("sso", S_LO L"o");

  // Legacy digraphs are not time limited: a slow typist still gets the conversion
  Expect("s|h", S_LO);
  Expect("c|h", C_LO);
  Expect("o|'", O_LO);
  Expect("g|'", G_LO);

  // Coaching tips only after legacy digraphs
  ExpectTip("sh", L"ss");
  ExpectTip("SH", L"SS");
  ExpectTip("ch", L"cc");
  ExpectTip("o'", L"oo");
  ExpectTip("G'", L"GG");
  ExpectTip("ss", 0);
  ExpectTip("sss", 0);

  // Window boundary
  ExpectWindow(350, true);
  ExpectWindow(351, false);

  if (g_failures) printf("%d FAILED\n", g_failures);
  else printf("all passed\n");
  return g_failures ? 1 : 0;
}
