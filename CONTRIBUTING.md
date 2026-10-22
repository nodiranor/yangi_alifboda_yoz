# Hissa qo'shish / Contributing to YAY

YAY loyihasiga hissa qo'shishni xohlaganingiz uchun tashakkur! Loyiha ochiq kodli bo'lib, har qanday yaxshilanishlar, takliflar va xatoliklar bo'yicha yordamingiz mamnuniyat bilan qabul qilinadi.

Thank you for your interest in contributing to YAY! This project is 100% open-source and welcomes contributions, bug reports, and suggestions.

---

## 🛠 Ishni boshlash / Getting Started

### 1. Talablar / Prerequisites
- **C++ Kompilyator**: MinGW-w64 (`g++` C++17) yoki MSVC (`cl.exe`).
- **Resurs kompilyatori**: `windres` (MinGW tarkibida).
- **Inno Setup 6**: O'rnatuvchi paketini (`.exe`) yig'ish uchun.
- **Python (Pillow)**: Agar `Icon.png` rasmidan yangi `.ico` yaratmoqchi bo'lsangiz.

### 2. Kodni olish / Clone Repository
```bash
git clone https://github.com/nodiranor/yangi_alifboda_yoz.git
cd yangi_alifboda_yoz
```

### 3. Dasturni kompilyatsiya qilish / Building
MinGW-w64 orqali:
```bash
# 1. Resurs faylini yig'ish
windres yay.rc -O coff -o yay_res.o

# 2. Asosiy dasturni kompilyatsiya qilish
g++ -O2 -municode -mwindows -static yay.cpp yay_res.o -o YAY.exe -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32

# 3. O'rnatuvchini yig'ish (Inno Setup)
ISCC.exe YAY_Setup.iss
```

---

## 📐 Kod yozish qoidalari / Code Guidelines

- **Zero-Dependency**: Dastur hech qanday og'ir kutubxonalar (Qt, Electron, .NET runtime) ishlatmaydi. Faqat sof va yengil Win32 API ishlatiladi.
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
