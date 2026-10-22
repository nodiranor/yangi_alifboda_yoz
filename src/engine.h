#pragma once
// Pure typing-rule state machine for YAY. No Win32 dependencies so it can be unit-tested.

typedef unsigned long EngineTick;

enum EngineKeyKind {
  EK_LETTER,      // a letter key; ch holds the character it produces, case already applied
  EK_APOSTROPHE,  // ' or ` without Shift (Latin profile only)
  EK_MODIFIER,    // Shift, Ctrl, Alt, Caps, Win pressed alone: ignored
  EK_COMBO,       // a key pressed together with Ctrl, Alt or Win: resets
  EK_OTHER        // any other key (space, digits, Backspace, Enter...): resets
};

// Which alphabet the rules apply to.
//   EP_LATIN:    s, c, o, g  ->  ş, ç, ö, ğ   (plus sh, ch, o', g')
//   EP_CYRILLIC: у, к, г, х  ->  ў, қ, ғ, ҳ
enum EngineProfile { EP_LATIN, EP_CYRILLIC };

enum EngineRun { ER_NONE, ER_ONE, ER_CONVERTED, ER_CANCELLED };

struct EngineState {
  EngineRun  run;
  wchar_t    letter;     // lowercase base letter of the current run
  wchar_t    typed[2];   // the two characters exactly as typed, kept for cancel
  EngineTick lastTime;   // tick of the last tap in the run
  EngineTick window;     // max ms between taps of one run
  EngineProfile profile; // alphabet currently in use
};

enum EngineAction { EA_PASS, EA_REPLACE };

struct EngineDecision {
  EngineAction   action;
  int            backspaces;  // EA_REPLACE only: Backspaces to send before text
  wchar_t        text[3];     // EA_REPLACE only: NUL-terminated replacement text
  const wchar_t *tip;         // double-tap hint after a legacy digraph, else 0
};

void           EngineInit(EngineState *st, EngineTick windowMs);
void           EngineReset(EngineState *st);
void           EngineSetProfile(EngineState *st, EngineProfile profile);  // resets the run when it changes
EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now);
