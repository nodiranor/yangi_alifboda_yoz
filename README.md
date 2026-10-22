<div align="center">

<img src="Icon.png" alt="YAY Logo" width="128" height="128">

# YAY — Yangi Alifboda Yoz

**O'zbek tilining isloh qilingan yangi lotin alifbosida tez va oson yozish uchun yengil, zamonaviy Windows yordamchisi.**  
*An ultra-lightweight, native Windows keyboard assistant designed for the reformed Uzbek Latin alphabet.*

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011-blue?logo=windows)](https://github.com/nodiranor/yangi_alifboda_yoz)
[![C++](https://img.shields.io/badge/Language-C%2B%2B17%20%28Pure%20Win32%29-00599C?logo=c%2B%2B)](yay.cpp)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Zero Dependency](https://img.shields.io/badge/Dependencies-Zero%20Runtime-orange.svg)](#texnik-xususiyatlari--technical-specs)
[![Size](https://img.shields.io/badge/Size-~300%20KB-success.svg)](#yuklab-olish--download)

[O'zbekcha](#-ozbekcha) • [English](#-english) • [Qoidalar / Rules](#-alifbo-qoidalari--typing-rules) • [O'rnatish / Install](#-ornatish--installation) • [Kompilyatsiya / Build](#-yigish-va-kompilyatsiya--building-from-source)

</div>

---

## 🇺🇿 O'zbekcha

**YAY (Yangi Alifboda Yoz)** — O'zbekiston Respublikasining isloh qilingan yangi lotin alifbosi harflarini (`ş`, `ç`, `ö`, `ğ`) standart ingliz (QWERTY) klaviaturasida tezkor va qulay yozish imkonini beruvchi ixcham, zamonaviy dastur.

Dastur tizimning boshqa tillariga (`ENG`, `RUS`) umuman xalaqit bermaydi. Siz **Win + Space** orqali O'zbek tiliga (`UZB`) o'tganingizdagina faollashadi.

### Asosiy Afzalliklari:
- 🚀 **Tezkor kiritish (Double-Tap)**: `ss` -> `ş`, `oo` -> `ö`, `gg` -> `ğ`, `cc` -> `ç`.
- ↩️ **Darhol bekor qilish (Enter Undo)**: Agar sizga aynan ikkita bir xil harf kerak bo'lsa (masalan, *hissa*, *massiv*), harf almashgandan so'ng darhol **Enter** tugmasini bosing — asl holatiga (`ss`) qaytadi!
- 🔄 **Eski usul bilan to'liq moslik**: `sh` -> `ş`, `ch` -> `ç`, `o'` -> `ö`, `g'` -> `ğ` kombinatsiyalari ham avtomatik ishlaydi.
- 🧘 **Xalaqit bermaydigan zamonaviy UI**: Qalqib chiquvchi yo'riqnoma tugmalar matn terish zonasini to'smaydi va keyingi harf bosilishi bilanoq ko'z ochib yumguncha (0 ms) yo'qoladi.
- 🔒 **100% Xavfsiz va Maxfiy**: Internetga ulanmaydi, ma'lumotlarni yozib olmaydi (No keylogger, No telemetry).
- 🪶 **Juda yengil**: Hajmi bor-yo'g'i ~300 KB. Hech qanday og'ir freymvorklarsiz (sof C++17 va Windows Win32 API).

---

## 🇬🇧 English

**YAY (Yangi Alifboda Yoz)** is an ultra-lightweight, high-performance native Windows input assistant specifically built for the reformed Uzbek Latin alphabet (`ş`, `ç`, `ö`, `ğ`).

It runs quietly in the system tray and monitors keystrokes strictly when the **Uzbek Latin (`uz-Latn-UZ`)** input layout is active via **Win + Space**. English (`ENG`) and Russian (`RUS`) input methods remain completely unaffected.

### Key Features:
- 🚀 **Fast Double-Tap Conversion**: `ss` -> `ş`, `oo` -> `ö`, `gg` -> `ğ`, `cc` -> `ç`.
- ↩️ **Instant Undo on Enter**: Convert accidentally? Pressing `Enter` immediately reverts the substitution back to literal double letters (`ss`, `oo`, etc.).
- 🔄 **Legacy Digraph Support**: Full backward compatibility with traditional digraphs (`sh` -> `ş`, `ch` -> `ç`, `o'` -> `ö`, `g'` -> `ğ`).
- 🧘 **Non-Blocking Pill UI**: Subtle dark-capsule hints float near the caret without obstructing text, automatically vanishing the instant you type another letter.
- 🔒 **100% Private & Offline**: Zero network calls, zero keystroke logging, no telemetry.
- 🪶 **Zero-Dependency Native Binary**: Compiled directly with native Win32 C++17 into a standalone ~300 KB executable.

---

## ⌨️ Alifbo qoidalari / Typing Rules

| Yangi harf / Target | Tezkor usul (Double-Tap) | An'anaviy usul (Legacy) | Bekor qilish (Undo) | Misol / Example |
| :---: | :---: | :---: | :---: | :--- |
| **ş** / **Ş** | `ss` / `SS` | `sh` / `SH` | <kbd>Enter</kbd> bosilsa `ss` bo'ladi | *şahar* / *shahar* |
| **ç** / **Ç** | `cc` / `CC` | `ch` / `CH` | <kbd>Enter</kbd> bosilsa `cc` bo'ladi | *çoy* / *choy* |
| **ö** / **Ö** | `oo` / `OO` | `o'` / `O'` | <kbd>Enter</kbd> bosilsa `oo` bo'ladi | *ördek* / *o'rdak* |
| **ğ** / **Ğ** | `gg` / `GG` | `g'` / `G'` | <kbd>Enter</kbd> bosilsa `gg` bo'ladi | *ğalaba* / *g'alaba* |

> [!TIP]
> **Yagona "c" rejimi (Ixtiyoriy)**: Dashboard orqali *"Bitta 'c' harfida to'g'ridan-to'g'ri 'ç' yozish"* sozlamasini yoqsangiz, har bitta `c` bosilganda darhol `ç` belgisi qo'yiladi.

---

## 📥 O'rnatish / Installation

### 1. Avtomatik o'rnatuvchi (Tavsiya etiladi)
1. Eng so'nggi [`Output/YAY_Setup.exe`](Output/YAY_Setup.exe) faylini yuklab oling.
2. O'rnatish dasturini ishga tushiring (O'zbek va Rus tillarida mavjud).
3. O'rnatuvchi avtomatik ravishda Windows til paneliga **O'zbek (Lotin)** tilini qo'shadi va yorliqlarni yaratadi.

### 2. Portativ versiya (Portable)
1. [`YAY.exe`](YAY.exe) faylini yuklab oling.
2. Istalgan qulay papkaga joylashtirib, ishga tushiring. Dastur to'g'ridan-to'g'ri tizim patnisida (System Tray) ishlay boshlaydi.

---

## 🖥 Tizim talablari / System Requirements

- **Operatsion tizim**: Windows 10, Windows 11 (32-bit va 64-bit).
- **Protsessor va Xotira**: Har qanday zamonaviy kompyuter (~5 MB RAM talab qiladi).
- **Qo'shimcha paketlar**: Hech narsa talab qilinmaydi (VC++ Redistributable yoki .NET kerak emas).

---

## 🛠 Yig'ish va Kompilyatsiya / Building from Source

Loyihani manba kodidan yig'ish uchun **MinGW-w64** yoki **MSVC** kerak bo'ladi.

### MinGW-w64 (`g++` va `windres`) yordamida:
```powershell
# 1. Resurs faylini (ikonka va versiya ma'lumotlarini) yig'ish
windres yay.rc -O coff -o yay_res.o

# 2. Asosiy dasturni kompilyatsiya qilish
g++ -O2 -municode -mwindows -static yay.cpp yay_res.o -o YAY.exe -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32

# 3. Vaqtinchalik resurs faylini tozalash
Remove-Item yay_res.o

# 4. O'rnatish paketini (YAY_Setup.exe) Inno Setup orqali yig'ish
ISCC.exe YAY_Setup.iss
```

### MSVC (`cl.exe` va `rc.exe`) yordamida:
```cmd
rc.exe /fo yay.res yay.rc
cl.exe /O2 /W4 /DUNICODE /D_UNICODE yay.cpp yay.res /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib /OUT:YAY.exe
```

---

## 📂 Loyiha tuzilmasi / Project Structure

```text
├── Icon.png            # 1254x1254 yuqori aniqlikdagi rasmiy brend belgisi
├── app.ico             # Dastur va o'rnatuvchi uchun multi-resolution ikonka (16-256px)
├── app_paused.ico      # Tizim patnisidagi pauza holati uchun ikonka
├── yay.cpp             # Asosiy C++17 dastur kodi (Win32 API, to'liq izohlar bilan)
├── yay.rc              # Windows resurs fayli (ikonkalar va VERSIONINFO)
├── YAY_Setup.iss       # Inno Setup o'rnatish skripti (O'zbek va Rus tillari)
├── Uzbek.isl           # Inno Setup uchun o'zbekcha tarjima fayli
├── YAY.exe             # Yig'ilgan tayyor portativ dastur (~300 KB)
├── Output/             # O'rnatuvchi chiqish papkasi
│   └── YAY_Setup.exe   # Tayyor to'liq o'rnatuvchi paket (~2.5 MB)
├── CONTRIBUTING.md     # Ochiq kodga hissa qo'shish yo'riqnomasi
├── LICENSE             # MIT Ochiq kod litsenziyasi
└── README.md           # Asosiy hujjat
```

---

## 🤝 Hissa qo'shish / Contributing

Loyiha to'liq ochiq kodli va jamoatchilik uchun ochiq. Takliflar, xatoliklar va qo'shimchalar bo'lsa, [Pull Request](https://github.com/nodiranor/yangi_alifboda_yoz/pulls) yoki [Issue](https://github.com/nodiranor/yangi_alifboda_yoz/issues) ochishingiz mumkin. Qo'shimcha ma'lumot uchun [CONTRIBUTING.md](CONTRIBUTING.md) faylini o'qing.

---

## 📜 Litsenziya / License

Ushbu loyiha [MIT License](LICENSE) asosida litsenziyalangan. Undan erkin foydalanish, o'zgartirish va tarqatish mumkin.
