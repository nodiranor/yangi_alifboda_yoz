#include "engine.h"

static wchar_t LowerAscii(wchar_t c) { return (c >= L'A' && c <= L'Z') ? (wchar_t)(c + 32) : c; }
static bool    UpperAscii(wchar_t c) { return c >= L'A' && c <= L'Z'; }
static bool    IsRunLetter(wchar_t lower) { return lower == L's' || lower == L'o' || lower == L'g' || lower == L'c'; }

static wchar_t TargetFor(wchar_t lower, bool upper) {
  switch (lower) {
    case L's': return upper ? 0x015E : 0x015F;  // Ş ş
    case L'c': return upper ? 0x00C7 : 0x00E7;  // Ç ç
    case L'o': return upper ? 0x00D6 : 0x00F6;  // Ö ö
    case L'g': return upper ? 0x011E : 0x011F;  // Ğ ğ
  }
  return 0;
}

static const wchar_t *TipFor(wchar_t lower, bool upper) {
  switch (lower) {
    case L's': return upper ? L"SS" : L"ss";
    case L'c': return upper ? L"CC" : L"cc";
    case L'o': return upper ? L"OO" : L"oo";
    case L'g': return upper ? L"GG" : L"gg";
  }
  return 0;
}

static EngineDecision Pass() {
  EngineDecision d = { EA_PASS, 0, {0, 0, 0}, 0 };
  return d;
}

static EngineDecision Replace1(wchar_t c, const wchar_t *tip) {
  EngineDecision d = { EA_REPLACE, 1, {c, 0, 0}, tip };
  return d;
}

void EngineInit(EngineState *st, EngineTick windowMs) { EngineReset(st); st->window = windowMs; }

void EngineReset(EngineState *st) {
  st->run = ER_NONE; st->letter = 0; st->typed[0] = 0; st->typed[1] = 0; st->lastTime = 0;
}

EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now) {
  if (kind == EK_MODIFIER) return Pass();
  if (kind == EK_OTHER || kind == EK_COMBO) { EngineReset(st); return Pass(); }

  const bool inWindow = (now - st->lastTime) <= st->window;

  // Legacy o' -> ö and g' -> ğ
  if (kind == EK_APOSTROPHE) {
    if (st->run == ER_ONE && (st->letter == L'o' || st->letter == L'g')) {
      const bool up = UpperAscii(st->typed[0]);
      const wchar_t l = st->letter;
      EngineReset(st);
      return Replace1(TargetFor(l, up), TipFor(l, up));
    }
    EngineReset(st);
    return Pass();
  }

  // kind == EK_LETTER from here on
  const wchar_t lower = LowerAscii(ch);

  // Legacy sh -> ş and ch -> ç
  if (lower == L'h') {
    if (st->run == ER_ONE && (st->letter == L's' || st->letter == L'c')) {
      const bool up = UpperAscii(st->typed[0]);
      const wchar_t l = st->letter;
      EngineReset(st);
      return Replace1(TargetFor(l, up), TipFor(l, up));
    }
    EngineReset(st);
    return Pass();
  }

  if (!IsRunLetter(lower)) { EngineReset(st); return Pass(); }

  if (st->letter == lower && inWindow) {
    if (st->run == ER_ONE) {
      const bool up = UpperAscii(st->typed[0]) || UpperAscii(ch);
      st->typed[1] = ch;
      st->run = ER_CONVERTED;
      st->lastTime = now;
      return Replace1(TargetFor(lower, up), 0);
    }
    if (st->run == ER_CONVERTED) {
      EngineDecision d = { EA_REPLACE, 1, {st->typed[0], st->typed[1], 0}, 0 };
      st->run = ER_CANCELLED;
      st->lastTime = now;
      return d;
    }
    if (st->run == ER_CANCELLED) {
      st->lastTime = now;
      return Pass();
    }
  }

  // Start a new run with this letter
  st->run = ER_ONE;
  st->letter = lower;
  st->typed[0] = ch;
  st->typed[1] = 0;
  st->lastTime = now;
  return Pass();
}
