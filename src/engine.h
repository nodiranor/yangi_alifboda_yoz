// engine.h - the Latin typing rules of YAY.
//
// This unit has no Win32 dependencies on purpose: it only looks at "which key, at what time"
// and answers "let it through" or "replace it with this". That keeps it easy to test
// (tests/test_engine.cpp) and easy to read. hook.cpp does the Windows part.
//
// The rules, for the four letters s, c, o, g:
//
//   double tap   ss -> ş   cc -> ç   oo -> ö   gg -> ğ   (second tap within the time window)
//   triple tap   sss -> ss, ccc -> cc, ...               (third tap gives the plain letters back)
//   legacy       sh -> ş   ch -> ç   o' -> ö   g' -> ğ   (any speed)
//
// One "run" is a series of taps of the same letter. It moves through these states:
//
//   ER_NONE ----letter----> ER_ONE ----same letter, in time----> ER_CONVERTED
//                             |                                       |
//                             |                                       | same letter, in time
//                             | h or '  (legacy form)                 v
//                             +--> replaced at once, back to NONE   ER_CANCELLED  (further taps pass through)
//
// Any other key, a Ctrl/Alt/Win combination, or a pause longer than the window ends the run.
// Capital letters follow the same rules: SS and Ss both give Ş.
#pragma once

// A millisecond tick count (GetTickCount). Unsigned, so "now - then" stays correct when the
// counter wraps around after about 49 days.
typedef unsigned long EngineTick;

// What kind of key was pressed. hook.cpp works this out from the raw key event.
enum EngineKeyKind {
  EK_LETTER,      // an A-Z key; `ch` holds the letter with Shift and Caps Lock already applied
  EK_APOSTROPHE,  // ' or ` without Shift (the key next to Enter, or the one under Esc)
  EK_MODIFIER,    // Shift, Ctrl, Alt, Caps Lock or Win pressed on their own: ignored
  EK_COMBO,       // any key pressed together with Ctrl, Alt or Win (a shortcut): ends the run
  EK_OTHER        // everything else (space, digits, Backspace, Enter, arrows...): ends the run
};

// Where the current run of one letter stands, see the diagram at the top of this file.
enum EngineRun { ER_NONE, ER_ONE, ER_CONVERTED, ER_CANCELLED };

// Everything the engine remembers between key presses.
struct EngineState {
  EngineRun  run;
  wchar_t    letter;     // lowercase base letter of the current run: s, o, g or c
  wchar_t    typed[2];   // the two characters exactly as typed, so a third tap can restore them
  EngineTick lastTime;   // tick of the last tap in the run
  EngineTick window;     // longest gap between taps of one run, in milliseconds
};

// What the hook should do with the key it just asked about.
enum EngineAction {
  EA_PASS,     // let the key reach the application unchanged
  EA_REPLACE   // swallow the key, send `backspaces` Backspaces and then `text`
};

struct EngineDecision {
  EngineAction   action;
  int            backspaces;  // EA_REPLACE only: how many characters to delete first
  wchar_t        text[3];     // EA_REPLACE only: replacement text, NUL-terminated
  const wchar_t *tip;         // set after a legacy form: the faster double tap to suggest ("ss"), else 0
};

// Prepares `st` for use. `windowMs` is the double-tap window (DOUBLE_TAP_MS).
void           EngineInit(EngineState *st, EngineTick windowMs);

// Forgets the current run. Call it when the user switches layout or window.
void           EngineReset(EngineState *st);

// Feeds one key press to the engine and returns what to do with it.
//   kind  what sort of key it was
//   ch    the character for EK_LETTER, otherwise ignored
//   now   when it happened (GetTickCount)
EngineDecision EngineProcess(EngineState *st, EngineKeyKind kind, wchar_t ch, EngineTick now);
