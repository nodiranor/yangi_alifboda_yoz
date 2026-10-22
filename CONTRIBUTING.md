# Hissa qo'shish / Contributing to YAY

YAY loyihasiga hissa qo'shishni xohlaganingiz uchun tashakkur! Loyiha ochiq kodli bo'lib, har qanday yaxshilanishlar, takliflar va xatoliklar bo'yicha yordamingiz mamnuniyat bilan qabul qilinadi.

Thank you for your interest in contributing to YAY! This project is 100% open-source and welcomes contributions, bug reports, and suggestions.

---

## 🛠 Ishni boshlash / Getting Started

### 1. Talablar / Prerequisites
- **C++ Kompilyator**: Visual Studio 2022 (MSVC) yoki MinGW-w64.
- **Resurs kompilyatori**: `windres` (MinGW tarkibida).
- **Inno Setup 6**: O'rnatuvchi paketini (`.exe`) yig'ish uchun.
- **Python (Pillow)**: Agar `Icon.png` rasmidan yangi `.ico` yaratmoqchi bo'lsangiz.

### 2. Kodni olish / Clone Repository
```bash
git clone https://github.com/nodiranor/yangi_alifboda_yoz.git
cd yangi_alifboda_yoz
```

### 3. Dasturni kompilyatsiya qilish / Building
MSVC (Visual Studio 2022):
```bat
build.cmd
```
Skript testlarni ishga tushiradi, `YAY.exe` ni yig'adi va Inno Setup topilsa o'rnatuvchini yaratadi. Haqiqiy klaviatura bosishlari bilan tekshirish uchun: `tests\e2e.cmd`.

MinGW-w64 (sinab ko'rilmagan / not verified):
```bash
mkdir -p build
(cd res && windres yay.rc -O coff -o ../build/yay_res.o)
g++ -std=c++17 -Os -s -municode -mwindows -static -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0800 src/*.cpp build/yay_res.o -o YAY.exe -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32
```

Qoidalar mantiqi `src/engine.cpp` da, testlari `tests/test_engine.cpp` da. Qoidalarni o'zgartirsangiz, avval test qo'shing.

---

## 📐 Kod yozish qoidalari / Code Guidelines

- **Zero-Dependency**: Dastur hech qanday og'ir kutubxonalar (Qt, Electron, .NET runtime) va C runtime ishlatmaydi. Faqat sof va yengil Win32 API ishlatiladi (`printf`, `strlen` o'rniga `wsprintfW`, `lstrlenW` kabilar).
- **Unicode**: Barcha matnlar va API chaqiruvlari `wchar_t` / `Unicode` (`UTF-16`) formatida bo'lishi shart.
- **Xavfsizlik va Maxfiylik**: Dastur klaviatura bosilishlarini hech qayerga saqlamaydi, yozmaydi va tarmoqqa yubormaydi. Telemetriya taqiqlangan.
- **Til izolyatsiyasi**: Boshqa tillarga (`ENG`, `RUS`) va tizim tezkor tugmalariga (`Ctrl+C`, `Ctrl+V`, `Alt+Tab`) ta'sir ko'rsatilmasligi shart.

---

## 🐛 Xatoliklar va Takliflar / Issues & Feature Requests

Xatolik haqida xabar berishda quyidagilarni ko'rsatishni unutmang:
1. Windows versiyasi (Windows 10, Windows 11).
2. Xatolik kuzatilgan dastur (Word, Chrome, Telegram va h.k.).
3. Qanday tugmalar bosilganda xatolik yuz bergani.

---

## 📄 Litsenziya / License

Loyiha [MIT License](LICENSE) asosida tarqatiladi.
