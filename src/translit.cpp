#include "translit.h"

struct Rule {
  wchar_t lower, upper;  // the Cyrillic letter
  const wchar_t *latin;  // its lowercase Latin form
};

static const Rule kRules[] = {
  { 0x0430, 0x0410, L"a" },            // а
  { 0x0431, 0x0411, L"b" },            // б
  { 0x0432, 0x0412, L"v" },            // в
  { 0x0433, 0x0413, L"g" },            // г
  { 0x0434, 0x0414, L"d" },            // д
  { 0x0435, 0x0415, L"e" },            // е  (ye after a vowel or at the start of a word, see below)
  { 0x0451, 0x0401, L"yo" },           // ё
  { 0x0436, 0x0416, L"j" },            // ж
  { 0x0437, 0x0417, L"z" },            // з
  { 0x0438, 0x0418, L"i" },            // и
  { 0x0439, 0x0419, L"y" },            // й
  { 0x043A, 0x041A, L"k" },            // к
  { 0x043B, 0x041B, L"l" },            // л
  { 0x043C, 0x041C, L"m" },            // м
  { 0x043D, 0x041D, L"n" },            // н
  { 0x043E, 0x041E, L"o" },            // о
  { 0x043F, 0x041F, L"p" },            // п
  { 0x0440, 0x0420, L"r" },            // р
  { 0x0441, 0x0421, L"s" },            // с
  { 0x0442, 0x0422, L"t" },            // т
  { 0x0443, 0x0423, L"u" },            // у
  { 0x0444, 0x0424, L"f" },            // ф
  { 0x0445, 0x0425, L"x" },            // х
  { 0x0446, 0x0426, L"ts" },           // ц
  { 0x0447, 0x0427, L"\x00E7" },       // ч -> ç
  { 0x0448, 0x0428, L"\x015F" },       // ш -> ş
  { 0x0449, 0x0429, L"\x015F\x00E7" }, // щ -> şç
  { 0x044A, 0x042A, L"\x02BC" },       // ъ -> ʼ (tutuq belgisi)
  { 0x044B, 0x042B, L"i" },            // ы
  { 0x044C, 0x042C, L"" },             // ь (dropped)
  { 0x044D, 0x042D, L"e" },            // э
  { 0x044E, 0x042E, L"yu" },           // ю
  { 0x044F, 0x042F, L"ya" },           // я
  { 0x045E, 0x040E, L"\x00F6" },       // ў -> ö
  { 0x049B, 0x049A, L"q" },            // қ
  { 0x0493, 0x0492, L"\x011F" },       // ғ -> ğ
  { 0x04B3, 0x04B2, L"h" },            // ҳ
};

static wchar_t UpperLatin(wchar_t c) {
  if (c >= L'a' && c <= L'z') return (wchar_t)(c - 32);
  if (c == 0x015F || c == 0x011F) return (wchar_t)(c - 1);  // ş ğ
  if (c == 0x00E7 || c == 0x00F6) return (wchar_t)(c - 32); // ç ö
  return c;
}

static bool IsVowelOrTutuq(wchar_t c) {
  switch (c) {
    case L'a': case L'e': case L'i': case L'o': case L'u': case 0x00F6:
    case L'A': case L'E': case L'I': case L'O': case L'U': case 0x00D6:
    case 0x02BC:
      return true;
  }
  return false;
}

void TranslitReset(TranslitState *st) { st->len = 0; }

void TranslitBackspace(TranslitState *st) { if (st->len > 0) st->len--; }

static void Push(TranslitState *st, wchar_t c) {
  if (st->len == 8) {
    for (int i = 1; i < 8; i++) st->tail[i - 1] = st->tail[i];
    st->len = 7;
  }
  st->tail[st->len++] = c;
}

int TranslitLetter(TranslitState *st, wchar_t cyrillic, bool allCaps, wchar_t out[4]) {
  const Rule *rule = 0;
  bool upper = false;
  for (unsigned i = 0; i < sizeof(kRules) / sizeof(kRules[0]); i++) {
    if (cyrillic == kRules[i].lower) { rule = &kRules[i]; break; }
    if (cyrillic == kRules[i].upper) { rule = &kRules[i]; upper = true; break; }
  }
  if (!rule) return -1;

  const wchar_t *src = rule->latin;
  if (rule->lower == 0x0435) {  // е
    const bool ye = (st->len == 0) || IsVowelOrTutuq(st->tail[st->len - 1]);
    src = ye ? L"ye" : L"e";
  }

  int n = 0;
  while (src[n] && n < 3) { out[n] = src[n]; n++; }
  if (upper && n > 0) {
    out[0] = UpperLatin(out[0]);
    if (allCaps) for (int i = 1; i < n; i++) out[i] = UpperLatin(out[i]);
  }
  out[n] = 0;
  for (int i = 0; i < n; i++) Push(st, out[i]);
  return n;
}
