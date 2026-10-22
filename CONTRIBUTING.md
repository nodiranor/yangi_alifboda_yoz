# Hissa qo'shish / Contributing

Takliflar va xatoliklar uchun Issue oching. Xatolik haqida yozganda Windows versiyasini, dastur nomini (Word, Chrome, Telegram...) va aynan qaysi tugmalar bosilganini ko'rsating.

Open an issue for bugs and ideas. For a bug, include your Windows version, the application, and the exact keys you pressed.

## Build and test

See [README.md](README.md). In short: `build.cmd` runs the engine tests and builds the exe, `tests\e2e.cmd` types real keys into the program's test box.

## Where things are

- `src/engine.cpp`: the typing rules for the Latin and Cyrillic alphabets. It has no Win32 calls, so it is tested directly by `tests/test_engine.cpp`.
- `src/keymap.cpp`: asks Windows which Cyrillic letter a key produces on the active layout (`tests/test_keymap.cpp`).
- `src/hook.cpp`: turns keyboard events into engine input and sends the replacement.
- `src/dashboard.cpp`, `src/tray.cpp`, `src/pill.cpp`: the window, the tray icon and the small hint popup.
- `src/layout.cpp`: keyboard layout detection and adding the Uzbek layout.
- `res/`: dialog layout, manifest, icons.

## Guidelines

- Change a typing rule by adding the test case first, then the code.
- Never intercept Enter, Tab or Escape. That was the problem with the first version.
- Windows API only. The exe is linked without the C runtime, so use `wsprintfW`, `lstrcpynW` and similar instead of `printf` or `wcscpy`. No STL, no exceptions.
- Windows 7 must keep working. Load anything newer than Windows 7 with `GetProcAddress`.
- The window uses standard controls from the dialog resource. No custom drawing.
- No network access and no logging of typed text.
- Keep the exe small.
