// Console tests for src/translit.cpp. Built and run by build.cmd.
#include "../src/translit.h"
#include <stdio.h>
#include <wchar.h>

static int g_failures = 0;

static void PrintW(const wchar_t *s) {
  for (; *s; s++) {
    if (*s < 128) putchar((int)*s);
    else printf("\\u%04X", (unsigned)*s);
  }
}

// Feeds `cyr` through the transliterator the way the keyboard hook does.
// ' ' resets the word state, '<' is a Backspace, every other character is a letter.
static void Run(const wchar_t *cyr, bool allCaps, wchar_t *out, size_t cap) {
  TranslitState st;
  TranslitReset(&st);
  size_t len = 0;
  out[0] = 0;
  for (const wchar_t *p = cyr; *p; p++) {
    if (*p == L' ') {
      TranslitReset(&st);
      if (len + 1 < cap) { out[len++] = L' '; out[len] = 0; }
    } else if (*p == L'<') {
      TranslitBackspace(&st);
      if (len > 0) out[--len] = 0;
    } else {
      wchar_t piece[4];
      int n = TranslitLetter(&st, *p, allCaps, piece);
      for (int i = 0; i < n && len + 1 < cap; i++) out[len++] = piece[i];
      out[len] = 0;
    }
  }
}

static void Expect(const wchar_t *cyr, const wchar_t *latin, bool allCaps = false) {
  wchar_t out[64];
  Run(cyr, allCaps, out, 64);
  if (wcscmp(out, latin) == 0) {
    printf("  ok    "); PrintW(cyr); printf(" -> "); PrintW(out); printf("\n");
  } else {
    g_failures++;
    printf("  FAIL  "); PrintW(cyr); printf("  expected ["); PrintW(latin);
    printf("] got ["); PrintW(out); printf("]\n");
  }
}

int main() {
  printf("translit tests\n");

  // The four new letters
  Expect(L"шаҳар", L"şahar");
  Expect(L"чой", L"çoy");
  Expect(L"Ўзбекистон", L"Özbekiston");
  Expect(L"ғалаба", L"ğalaba");
  Expect(L"Тошкент", L"Toşkent");
  Expect(L"Чирчиқ", L"Çirçiq");
  Expect(L"беш", L"beş");
  Expect(L"ёнғоқ", L"yonğoq");

  // The other Uzbek letters
  Expect(L"қалам", L"qalam");
  Expect(L"ҳаво", L"havo");
  Expect(L"яхши", L"yaxşi");
  Expect(L"Самарқанд", L"Samarqand");
  Expect(L"мактаб", L"maktab");
  Expect(L"юрак", L"yurak");
  Expect(L"ёмон", L"yomon");
  Expect(L"йигит", L"yigit");
  Expect(L"икки", L"ikki");
  Expect(L"эшик", L"eşik");
  Expect(L"маъно", L"ma\x02BCno");
  Expect(L"аъзо", L"a\x02BCzo");

  // е: ye at the start of a word and after a vowel, e after a consonant
  Expect(L"Ер", L"Yer");
  Expect(L"етти", L"yetti");
  Expect(L"Европа", L"Yevropa");
  Expect(L"поезд", L"poyezd");
  Expect(L"а е", L"a ye");
  Expect(L"бе", L"be");
  Expect(L"беш", L"beş");

  // Russian letters that Uzbek Cyrillic also uses
  Expect(L"Россия", L"Rossiya");
  Expect(L"щи", L"şçi");
  Expect(L"Щи", L"Şçi");
  Expect(L"цирк", L"tsirk");
  Expect(L"ыш", L"iş");
  Expect(L"сентябрь", L"sentyabr");

  // Capitals: title case by default, full capitals while Caps Lock is on
  Expect(L"Юлдуз", L"Yulduz");
  Expect(L"Ёш", L"Yoş");
  Expect(L"ЮЛДУЗ", L"YULDUZ", true);
  Expect(L"ШАҲАР", L"ŞAHAR", true);
  Expect(L"ЩИ", L"ŞÇI", true);
  Expect(L"Ш", L"Ş");
  Expect(L"Ч", L"Ç");
  Expect(L"Ў", L"Ö");
  Expect(L"Ғ", L"Ğ");

  // Word boundaries and Backspace
  Expect(L"Ўзбек тили", L"Özbek tili");
  Expect(L"бе<е", L"be");    // Backspace, then е after б: still e
  Expect(L"а<е", L"ye");     // Backspace back to the start of the word: ye
  Expect(L"ае<е", L"aye");   // after the vowel а: ye

  // A letter that is not Cyrillic Uzbek or Russian is not handled
  {
    TranslitState st;
    TranslitReset(&st);
    wchar_t out[4];
    int n = TranslitLetter(&st, 0x0453, false, out);  // ѓ
    if (n == -1) printf("  ok    unknown letter returns -1\n");
    else { g_failures++; printf("  FAIL  unknown letter returned %d\n", n); }
  }

  if (g_failures) printf("%d FAILED\n", g_failures);
  else printf("all passed\n");
  return g_failures ? 1 : 0;
}
