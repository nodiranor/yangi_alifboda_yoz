#pragma once
// Cyrillic -> new Uzbek Latin alphabet, one letter at a time. No Win32 dependencies.
//
//   ш -> ş   ч -> ç   ў -> ö   ғ -> ğ   қ -> q   ҳ -> h   х -> x   ъ -> ʼ
//   ё -> yo  ю -> yu  я -> ya  ц -> ts  щ -> şç  ы -> i   ь -> (nothing)
//   е -> ye at the start of a word or after a vowel, e elsewhere;  э -> e
//
// The only memory needed is the last few Latin letters produced, for the "ye" rule.

struct TranslitState {
  wchar_t tail[8];  // last Latin characters produced, newest last
  int     len;
};

void TranslitReset(TranslitState *st);      // a word boundary or an unknown key: start fresh
void TranslitBackspace(TranslitState *st);  // the user deleted the last character

// Converts one Cyrillic letter. Writes up to 2 characters plus a NUL to `out` and returns how many
// (0 for the soft sign), or -1 if `cyrillic` is not a letter this module handles.
// `allCaps` is true while Caps Lock is typing capitals: multi-letter results then come out in
// full capitals (YU) instead of title case (Yu).
int TranslitLetter(TranslitState *st, wchar_t cyrillic, bool allCaps, wchar_t out[4]);
