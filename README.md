<div align="center">

<img src="Icon.png" alt="YAY Logo" width="128" height="128">

# YAY — Yangi Alifboda Yoz

**O'zbek tilining isloh qilingan yangi lotin alifbosida tez va oson yozish uchun yengil, zamonaviy Windows yordamchisi.**
*An ultra-lightweight, native Windows keyboard assistant for the reformed Uzbek Latin alphabet.*

[![Sponsor](https://img.shields.io/badge/Homiy%20%2F%20Sponsor-%40juraboshibot-26A5E4?logo=telegram&logoColor=white)](https://t.me/juraboshibot)
[![Platform](https://img.shields.io/badge/Platform-Windows%207%20%7C%208%20%7C%2010%20%7C%2011-blue?logo=windows)](https://github.com/nodiranor/yangi_alifboda_yoz)
[![C++](https://img.shields.io/badge/Language-C%2B%2B%20%28Pure%20Win32%29-00599C?logo=c%2B%2B)](src)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Size](https://img.shields.io/badge/Size-~57%20KB-success.svg)](#-yuklab-olish--installation)
[![Zero Dependency](https://img.shields.io/badge/Dependencies-None-orange.svg)](#-texnik-xususiyatlar--technical-notes)

[O'zbekcha](#-ozbekcha) • [English](#-english) • [Qoidalar / Rules](#%EF%B8%8F-alifbo-qoidalari--typing-rules) • [O'rnatish / Install](#-ornatish--installation) • [Kompilyatsiya / Build](#-yigish--building-from-source)

</div>

---

## 🇺🇿 O'zbekcha

**YAY (Yangi Alifboda Yoz)** — isloh qilingan yangi o'zbek lotin alifbosi harflarini (`ş`, `ç`, `ö`, `ğ`) oddiy QWERTY klaviaturada tez yozish uchun ixcham dastur.

Dastur boshqa tillarga (`ENG`, `RUS`) xalaqit bermaydi. U faqat **Win + Space** orqali O'zbek (`UZB`) tili tanlanganda ishlaydi.

### Asosiy afzalliklari
- 🚀 **Ikki marta bosish**: `ss` → `ş`, `oo` → `ö`, `gg` → `ğ`, `cc` → `ç`.
- 🔁 **Uch marta bosish**: ikkita oddiy harf kerak bo'lsa (*Rossiya*, *issiq*, *massa*) uchinchi marta bosing: `sss` → `ss`. Enter, Tab va Escape tugmalari hech qachon ishlatilmaydi, shuning uchun brauzer, Telegram va har qanday formada xavfsiz.
- 🔄 **Eski usul ham ishlaydi**: `sh` → `ş`, `ch` → `ç`, `o'` → `ö`, `g'` → `ğ`.
- 🪟 **Windows 7, 8, 8.1, 10, 11** — bitta fayl, 32 va 64 bitli tizimlarda.
- 🧘 **Oddiy Windows oynasi**: tizim shriftlari va tugmalari, yuqori aniqlikdagi (DPI) ekranlarda ham tiniq.
- 🔒 **Maxfiy**: internetga ulanmaydi, hech narsa yozib olmaydi.
- 🪶 **Juda yengil**: bitta ~57 KB `.exe`, qo'shimcha kutubxonalar yo'q, kutish holatida protsessorni ishlatmaydi.

---

## 🇬🇧 English

**YAY (Yangi Alifboda Yoz)** is a tiny native Windows input assistant for the reformed Uzbek Latin alphabet (`ş`, `ç`, `ö`, `ğ`). It sits in the tray and only acts while the **Uzbek Latin** layout is selected with **Win + Space**. English and Russian typing are untouched.

### Key features
- 🚀 **Double tap**: `ss` → `ş`, `oo` → `ö`, `gg` → `ğ`, `cc` → `ç`.
- 🔁 **Triple tap**: need the plain double letter (*Rossiya*, *issiq*)? Tap a third time: `sss` → `ss`. Enter, Tab and Escape are never intercepted, so it is safe in browsers, Telegram and any form.
- 🔄 **Legacy digraphs**: `sh` → `ş`, `ch` → `ç`, `o'` → `ö`, `g'` → `ğ` keep working.
- 🪟 **Windows 7 through 11**, one 32-bit exe for every edition.
- 🧘 **Plain Windows dialog**: standard controls, system font, crisp on high-DPI screens.
- 🔒 **Private**: no network, no logging.
- 🪶 **Lightweight**: one ~57 KB `.exe`, no runtime dependencies, zero CPU while idle.

---

## ⌨️ Alifbo qoidalari / Typing rules

| Harf / Letter | Tez usul / Fast | Oddiy ikkita harf / Plain double | Eski usul / Legacy | Misol / Example |
| :---: | :---: | :---: | :---: | :--- |
| **ş** / **Ş** | `ss` / `SS` | `sss` → `ss` | `sh` / `SH` | *şahar*, *Rossiya* (`Rosssiya`) |
| **ç** / **Ç** | `cc` / `CC` | `ccc` → `cc` | `ch` / `CH` | *çoy* |
| **ö** / **Ö** | `oo` / `OO` | `ooo` → `oo` | `o'` / `O'` | *ördak*, *zoologiya* (`zooologiya`) |
| **ğ** / **Ğ** | `gg` / `GG` | `ggg` → `gg` | `g'` / `G'` | *ğalaba* |

Uch marta bosish tez bosilganda (taxminan 0,35 soniya ichida) ishlaydi. Qo'shimcha bosishlar oddiy harf sifatida qoladi: `ssss` → `sss`.

> [!NOTE]
> **ş dan keyin s** (*işsiz*, *tuşsa*): `sh` + `s` deb yozing, yoki ikkinchi `s` dan keyin biroz kutib uchinchisini bosing.
> **ş followed by s** (*işsiz*, *tuşsa*): type `sh` + `s`, or pause briefly before the last `s`.

> [!TIP]
> **Bitta "c" rejimi**: oynadagi *"Bitta 'c' harfini darhol 'ç' qilib yozish"* sozlamasi yoqilsa, har bir `c` darhol `ç` bo'ladi.

---

## 📥 O'rnatish / Installation

### 1. O'rnatuvchi (tavsiya etiladi)
1. [`Output/YAY_Setup.exe`](Output/YAY_Setup.exe) faylini yuklab oling va ishga tushiring (O'zbek va Rus tillarida).
2. O'rnatuvchi Windows 8 va undan yangi tizimlarda **O'zbek (lotin)** tilini til paneliga qo'shadi. Windows 7 da buni dastur oynasidagi tugma bajaradi (keyin tizimdan chiqib, qayta kiring).

### 2. Portativ
[`YAY.exe`](YAY.exe) faylini istalgan papkaga joylashtirib ishga tushiring. `/tray` parametri bilan ishga tushirilsa, oyna ochilmaydi.

---

## 🖥 Tizim talablari / System requirements

- Windows 7 SP1, 8, 8.1, 10, 11 (32 yoki 64 bit).
- Qo'shimcha paketlar kerak emas (VC++ Redistributable yoki .NET talab qilinmaydi).

---

## 🛠 Yig'ish / Building from source

### MSVC (Visual Studio 2022, Desktop C++)
```bat
build.cmd
```
Skript quyidagilarni bajaradi: `tests\test_engine.cpp` testlarini ishga tushiradi, resurslarni kompilyatsiya qiladi, `YAY.exe` ni C runtime kutubxonasisiz (x86) yig'adi, Inno Setup 6 topilsa `Output\YAY_Setup.exe` ni yaratadi.

Haqiqiy klaviatura bosishlari bilan to'liq tekshirish (dastur oynasidagi sinov maydoniga yozadi):
```bat
tests\e2e.cmd
```

### MinGW-w64 (sinab ko'rilmagan / not verified)
```bash
mkdir -p build
(cd res && windres yay.rc -O coff -o ../build/yay_res.o)
g++ -std=c++17 -Os -s -municode -mwindows -static -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0800 src/*.cpp build/yay_res.o -o YAY.exe -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32
```

### Ikonkalar / Icons
```bash
python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico
```

---

## 📂 Loyiha tuzilmasi / Project structure

```text
├── src/                 # C++ manba kodi: engine (qoidalar), hook, dashboard, tray, pill, layout, settings, main
├── res/                 # resource.h, yay.rc (dialog), yay.manifest, ikonkalar
├── tests/               # engine testlari (test_engine.cpp) va e2e (e2e.cmd, e2e.ps1)
├── tools/make_icons.py  # Icon.png dan .ico yaratish
├── build.cmd            # MSVC yig'ish skripti
├── YAY_Setup.iss        # Inno Setup skripti (O'zbek va Rus)
├── Uzbek.isl            # Inno Setup o'zbekcha tarjimasi
├── install_uzbek_layout.ps1
├── YAY.exe              # Tayyor portativ dastur
└── Output/YAY_Setup.exe # Tayyor o'rnatuvchi
```

---

## 🔧 Texnik xususiyatlar / Technical notes

- Sof Win32 API: C++ standart kutubxonasi, istisnolar va C runtime ishlatilmaydi. Natijada bitta ~57 KB `.exe`.
- Klaviatura hodisalari `WH_KEYBOARD_LL` orqali kuzatiladi, almashtirish `SendInput` bilan yuboriladi. Til o'zgarishi Windows xabarlari orqali bilinadi, taymer bilan so'ralmaydi.
- Oyna — oddiy `DIALOGEX` resursi; manifest orqali Common Controls v6 va DPI (PerMonitorV2) qo'llab-quvvatlanadi.
- Ma'lum cheklov: matn maydonida avtomatik to'ldirish tanlangan bo'lsa (masalan, Chrome manzil satrida URL taklifi), yuborilgan Backspace harf o'rniga taklifni o'chirishi mumkin. Bu barcha hook asosidagi yordamchilarga xos.

---

## 🤝 Hissa qo'shish / Contributing

[CONTRIBUTING.md](CONTRIBUTING.md) faylini o'qing. Takliflar va xatoliklar uchun [Issue](https://github.com/nodiranor/yangi_alifboda_yoz/issues) oching.

**Homiy / Sponsor:** [t.me/juraboshibot](https://t.me/juraboshibot)

## 📜 Litsenziya / License

[MIT License](LICENSE).
