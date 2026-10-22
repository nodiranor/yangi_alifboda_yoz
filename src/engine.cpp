#include "engine.h"

// One doubling rule: the base letter typed twice becomes the target letter.
struct RunRule {
  wchar_t lower, upper;
  wchar_t targetLower, targetUpper;
  const wchar_t *tipLower, *tipUpper;  // hint shown after a legacy digraph, or 0 when there is none
};

static const RunRule kLatinRules[] = {
  { L's', L'S', 0x015F, 0x015E, L"ss", L"SS" },  // s -> ş
  { L'c', L'C', 0x00E7, 0x00C7, L"cc", L"CC" },  // c -> ç
  { L'o', L'O', 0x00F6, 0x00D6, L"oo", L"OO" },  // o -> ö
  { L'g', L'G', 0x011F, 0x011E, L"gg", L"GG" },  // g -> ğ
};

static const RunRule kCyrillicRules[] = {
  { 0x0443, 0x0423, 0x045E, 0x040E, 0, 0 },  // у -> ў
  { 0x043A, 0x041A, 0x049B, 0x049A, 0, 0 },  // к -> қ
  { 0x0433, 0x0413, 0x0493, 0x0492, 0, 0 },  // г -> ғ
  { 0x0445, 0x0425, 0x04B3, 0x04B2, 0, 0 },  // х -> ҳ
};

static const RunRule *FindRule(EngineProfile profile, wchar_t ch, bool *upper) {
  const RunRule *rules = (profile == EP_LATIN) ? kLatinRules : kCyrillicRules;
  for (int i = 0; i < 4; i++) {
    if (ch == rules[i].lower) { *upper = false; return &rules[i]; }
    if (ch == rules[i].upper) { *upper = true;  return &rules[i]; }
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

void EngineInit(EngineState *st, EngineTick windowMs) {
  EngineReset(st);
  st->window = windowMs;
  st->profile = EP_LATIN;
}

void EngineReset(EngineState *st) {
  st->run = ER_NONE; st->letter = 0; st->typed[0] = 0; st->typed[1] = 0; st->lastTime = 0;
}

void EngineSetProfile(EngineState *st, EngineProfile profile) {
  if (st->profile != profile) {
    st->profile = profile;
    EngineReset(st);
  }
}

// sh, ch, o' and g': the first letter of the pair is already typed, so replace it.
static EngineDecision LegacyDigraph(EngineState *st) {
  const bool up = (st->typed[0] >= L'A' && st->typed[0] <= L'Z');
  bool ignored = false;
  const RunRule *rule = FindRule(EP_LATIN, st->letter, &ignored);
  EngineReset(st);
  return Replace1(up ? rule->targetUpper : rule->targetLower, up ? rule->tipUpper : rule->tipLower);
}

EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now) {
  if (kind == EK_MODIFIER) return Pass();
  if (kind == EK_OTHER || kind == EK_COMBO) { EngineReset(st); return Pass(); }

  const bool latin = (st->profile == EP_LATIN);
  const bool inWindow = (now - st->lastTime) <= st->window;

  // Legacy o' -> ö and g' -> ğ (Latin only)
  if (kind == EK_APOSTROPHE) {
    if (latin && st->run == ER_ONE && (st->letter == L'o' || st->letter == L'g'))
      return LegacyDigraph(st);
    EngineReset(st);
    return Pass();
  }

  // Legacy sh -> ş and ch -> ç (Latin only)
  if (latin && (ch == L'h' || ch == L'H')) {
    if (st->run == ER_ONE && (st->letter == L's' || st->letter == L'c'))
      return LegacyDigraph(st);
    EngineReset(st);
    return Pass();
  }

  bool up = false;
  const RunRule *rule = FindRule(st->profile, ch, &up);
  if (!rule) { EngineReset(st); return Pass(); }

  if (st->letter == rule->lower && inWindow) {
    if (st->run == ER_ONE) {
      const bool bothUpper = up || (st->typed[0] == rule->upper);
      st->typed[1] = ch;
      st->run = ER_CONVERTED;
      st->lastTime = now;
      return Replace1(bothUpper ? rule->targetUpper : rule->targetLower, 0);
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
  st->letter = rule->lower;
  st->typed[0] = ch;
  st->typed[1] = 0;
  st->lastTime = now;
  return Pass();
}
