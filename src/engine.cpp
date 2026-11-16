// engine.cpp - the Latin typing rules. Read engine.h first for the overview and the state diagram.
#include "engine.h"

static wchar_t LowerAscii(wchar_t c) { return (c >= L'A' && c <= L'Z') ? (wchar_t)(c + 32) : c; }
static bool    UpperAscii(wchar_t c) { return c >= L'A' && c <= L'Z'; }

// The four letters that can start a run. To support another letter, add it here and in
// TargetFor/TipFor, and add test cases to tests/test_engine.cpp.
static bool    IsRunLetter(wchar_t lower) { return lower == L's' || lower == L'o' || lower == L'g' || lower == L'c'; }

// The letter a run of `lower` turns into: s -> ş, c -> ç, o -> ö, g -> ğ (or the capital).
// The numbers are Unicode code points, written out so the file does not depend on its encoding.
static wchar_t TargetFor(wchar_t lower, bool upper) {
  switch (lower) {
    case L's': return upper ? 0x015E : 0x015F;  // Ş ş
    case L'c': return upper ? 0x00C7 : 0x00E7;  // Ç ç
    case L'o': return upper ? 0x00D6 : 0x00F6;  // Ö ö
    case L'g': return upper ? 0x011E : 0x011F;  // Ğ ğ
  }
  return 0;
}

// The double tap that does the same job as a legacy form. Shown in the hint popup after
// someone types sh, ch, o' or g' ("ss is faster").
static const wchar_t *TipFor(wchar_t lower, bool upper) {
  switch (lower) {
    case L's': return upper ? L"SS" : L"ss";
    case L'c': return upper ? L"CC" : L"cc";
    case L'o': return upper ? L"OO" : L"oo";
    case L'g': return upper ? L"GG" : L"gg";
  }
  return 0;
}

// Decision helpers: "let the key through" and "replace the previous character with one letter".
static EngineDecision Pass() {
  EngineDecision d = { EA_PASS, 0, {0, 0, 0}, 0 };
  return d;
}

static EngineDecision Replace1(wchar_t c, const wchar_t *tip) {
  EngineDecision d = { EA_REPLACE, 1, {c, 0, 0}, tip };
  return d;
}

void EngineInit(EngineState *st, EngineTick windowMs, EngineTick undoMs) {
  EngineReset(st);
  st->window = windowMs;
  st->undoWindow = undoMs;
}

void EngineReset(EngineState *st) {
  st->run = ER_NONE; st->letter = 0; st->typed[0] = 0; st->typed[1] = 0; st->lastTime = 0;
  st->undo[0] = 0; st->undoTime = 0;
}

void EngineSetUndo(EngineState *st, const wchar_t *typed, EngineTick now) {
  int n = 0;
  while (typed[n] && n < 2) { st->undo[n] = typed[n]; n++; }
  st->undo[n] = 0;
  st->undoTime = now;
}

// The same, for the two keys of a pair: "s" + "h", "o" + "'", "s" + "s".
static void SetUndo2(EngineState *st, wchar_t first, wchar_t second, EngineTick now) {
  const wchar_t typed[3] = { first, second, 0 };
  EngineSetUndo(st, typed, now);
}

// A note on timing: the first letter of every pair has already reached the application by the
// time the second key arrives (we cannot know a second tap is coming). So a conversion never
// swallows two letters; it swallows the second key and asks the hook to delete the first
// (backspaces = 1) and type the new letter in its place.
EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now) {
  // Shift, Ctrl and friends pressed alone are part of typing a capital or a shortcut, not a
  // new key. They must not end a run: typing "sS" presses Shift between the two taps.
  if (kind == EK_MODIFIER) return Pass();

  // Backspace right after a conversion undoes it: delete the new letter and type back the keys
  // it replaced, so sh -> ş -> sh and ss -> ş -> ss. The Backspace itself is swallowed. Later,
  // or with nothing to undo, it is an ordinary Backspace. Either way the run is over.
  if (kind == EK_BACKSPACE) {
    const bool canUndo = st->undo[0] && (now - st->undoTime) <= st->undoWindow;
    EngineDecision d = Pass();
    if (canUndo) {
      d.action = EA_REPLACE;
      d.backspaces = 1;
      d.text[0] = st->undo[0]; d.text[1] = st->undo[1]; d.text[2] = 0;
    }
    EngineReset(st);
    return d;
  }

  // Any other key means the user has accepted the last conversion: it can no longer be undone.
  st->undo[0] = 0;

  // Spaces, digits, arrows, shortcuts and so on end the run.
  if (kind == EK_OTHER || kind == EK_COMBO) { EngineReset(st); return Pass(); }

  // Unsigned subtraction: still right if the tick counter wrapped in between.
  const bool inWindow = (now - st->lastTime) <= st->window;

  // Legacy o' -> ö and g' -> ğ. Only valid right after a lone o or g. There is no time limit,
  // so a slow typist still gets the conversion.
  if (kind == EK_APOSTROPHE) {
    if (st->run == ER_ONE && (st->letter == L'o' || st->letter == L'g')) {
      const bool up = UpperAscii(st->typed[0]);
      const wchar_t l = st->letter;
      const wchar_t first = st->typed[0];
      EngineReset(st);
      SetUndo2(st, first, ch ? ch : L'\'', now);
      return Replace1(TargetFor(l, up), TipFor(l, up));
    }
    EngineReset(st);
    return Pass();
  }

  // From here on kind == EK_LETTER.
  const wchar_t lower = LowerAscii(ch);

  // Legacy sh -> ş and ch -> ç. Only valid right after a lone s or c. An h anywhere else
  // (for example "ssh", which is ş then h) is just a letter and ends the run.
  if (lower == L'h') {
    if (st->run == ER_ONE && (st->letter == L's' || st->letter == L'c')) {
      const bool up = UpperAscii(st->typed[0]);
      const wchar_t l = st->letter;
      const wchar_t first = st->typed[0];
      EngineReset(st);
      SetUndo2(st, first, ch, now);
      return Replace1(TargetFor(l, up), TipFor(l, up));
    }
    EngineReset(st);
    return Pass();
  }

  // Any letter that is not s, c, o or g ends the run.
  if (!IsRunLetter(lower)) { EngineReset(st); return Pass(); }

  // The same letter again, quickly enough: this is the second or third tap of a run.
  if (st->letter == lower && inWindow) {
    if (st->run == ER_ONE) {
      // Second tap: ss -> ş. The result is a capital if either tap was a capital.
      const bool up = UpperAscii(st->typed[0]) || UpperAscii(ch);
      st->typed[1] = ch;
      st->run = ER_CONVERTED;
      st->lastTime = now;
      SetUndo2(st, st->typed[0], ch, now);
      return Replace1(TargetFor(lower, up), 0);
    }
    if (st->run == ER_CONVERTED) {
      // Third tap: give back the two plain letters exactly as they were typed (so "Ss" stays "Ss").
      // The third key itself is swallowed, so three taps leave two letters.
      EngineDecision d = { EA_REPLACE, 1, {st->typed[0], st->typed[1], 0}, 0 };
      st->run = ER_CANCELLED;
      st->lastTime = now;
      return d;
    }
    if (st->run == ER_CANCELLED) {
      // Fourth tap and beyond: ordinary letters. "ssss" gives "sss".
      st->lastTime = now;
      return Pass();
    }
  }

  // A different letter, or too slow: start a new run with this letter. The key itself passes
  // through; if a second tap follows we will replace it.
  st->run = ER_ONE;
  st->letter = lower;
  st->typed[0] = ch;
  st->typed[1] = 0;
  st->lastTime = now;
  return Pass();
}
