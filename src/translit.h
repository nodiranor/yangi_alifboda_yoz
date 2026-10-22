#pragma once
// Cyrillic -> new Uzbek Latin alphabet, one letter at a time. No Win32 dependencies.
//
//   ш -> ş   ч -> ç   ў -> ö   ғ -> ğ   қ -> q   ҳ -> h   х -> x   ъ -> ʼ
//   ё -> yo  ю -> yu  я -> ya  ц -> ts  щ -> şç  ы -> i   ь -> (nothing)
//   е -> ye at the start of a word or after a vowel, e elsewhere;  э -> e
//
// The only memory needed is the last few Latin letters produced, for the "ye" rule.

// Design notes.
//   * hook.cpp calls TranslitLetter for every Cyrillic letter typed on a Cyrillic layout and
//     types the result instead of the key.
//   * The conversion works letter by letter. It needs no dictionary, never waits for the next
//     key, and behaves the same in every program.
//   * The one letter that depends on its surroundings is е (ye or e), so we remember the last
//     few Latin letters we produced. Backspace and word breaks keep that memory in step with
//     the text on screen.
//   * To change or add a mapping, edit kRules in translit.cpp and add cases to
//     tests/test_translit.cpp.
//
// The memory of the last Latin letters produced.
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
