# YAY — Yangi Alifboda Yoz

![YAY oynasi / YAY window](docs/dashboard.png)

**YAY** o'zbek lotin alifbosining yangi harflarini (ş, ç, ö, ğ) oddiy klaviaturada yozish uchun kichik Windows dasturi. U tizim patnisida ishlaydi va faqat O'zbek (lotin) yoki O'zbek (kirill) klaviaturasi tanlanganda harflarni almashtiradi (Win + Space). Ingliz klaviaturasiga tegmaydi. Rus klaviaturasiga faqat sozlamada yoqsangiz tegadi.

**YAY** is a small Windows program for typing the new Uzbek Latin letters (ş, ç, ö, ğ) on a regular keyboard. It runs in the tray and only acts while the Uzbek (Latin) or Uzbek (Cyrillic) keyboard layout is selected (Win + Space). The English layout is left alone, and the Russian layout is only used if you turn it on in the settings.

Homiy / Sponsor: [t.me/juraboshibot](https://t.me/juraboshibot)

## Qoidalar / Typing rules

| Harf / Letter | Tez usul / Fast | Oddiy ikki harf / Plain double | Eski usul / Legacy |
| :---: | :---: | :---: | :---: |
| ş Ş | `ss` `SS` | `sss` → `ss` | `sh` `SH` |
| ç Ç | `cc` `CC` | `ccc` → `cc` | `ch` `CH` |
| ö Ö | `oo` `OO` | `ooo` → `oo` | `o'` `O'` |
| ğ Ğ | `gg` `GG` | `ggg` → `gg` | `g'` `G'` |

- Bir xil harfni ikki marta tez bossangiz, ular bitta yangi harfga almashadi: `ss` → ş. Oddiy ikki harf kerak bo'lsa (*Rossiya*, *issiq*), uchinchi marta bosing: `sss` → `ss`. Keyingi bosishlar oddiy harf bo'lib qoladi: `ssss` → `sss`.
- "Tez" taxminan 0,35 soniya. Eski usullar (`sh`, `ch`, `o'`, `g'`) tezlikka bog'liq emas.
- Enter, Tab va Escape tugmalariga hech qachon tegilmaydi.
- ş dan keyin s keladigan so'zlarda (*işsiz*, *tuşsa*) `sh` + `s` yozing yoki ikkinchi `s` dan keyin biroz to'xtang.
- Dastur oynasida "Bitta 'c' harfini darhol 'ç' qilib yozish" sozlamasi bor.

Tap the same letter twice quickly to get the new letter: `ss` → ş. If you need the plain double letter (*Rossiya*, *issiq*), tap a third time: `sss` → `ss`. Further taps stay plain: `ssss` → `sss`. "Quickly" means about 0.35 seconds. The legacy forms (`sh`, `ch`, `o'`, `g'`) have no time limit. Enter, Tab and Escape are never touched. For words with ş followed by s (*işsiz*, *tuşsa*), type `sh` + `s`, or pause briefly before the last `s`.

### Kirill / Cyrillic

O'zbek kirill alifbosining ў, қ, ғ, ҳ harflari rus klaviaturasida yo'q. YAY ularni xuddi shu usulda beradi. Bu O'zbek (kirill) klaviaturasi tanlanganda o'zi ishlaydi. Rus (RU) klaviaturasida esa dastur oynasida "Rus (RU) klaviaturasida ham ishlash" belgilansa ishlaydi, chunki rus tilida yozganda ham ishlatiladi.

| Harf / Letter | Tez usul / Fast | Oddiy ikki harf / Plain double |
| :---: | :---: | :---: |
| ў Ў | `уу` `УУ` | `ууу` → `уу` |
| қ Қ | `кк` `КК` | `ккк` → `кк` |
| ғ Ғ | `гг` `ГГ` | `ггг` → `гг` |
| ҳ Ҳ | `хх` `ХХ` | `ххх` → `хх` |

Qoidalar lotin bilan bir xil. Eslatma: *икки* so'zini `иккки` deb yozing (`кк` қ ga aylanadi). Rus (RU) da yoqilgan bo'lsa, rus so'zlaridagi `кк`, `уу` (*аккорд*, *вакуум*) ham almashadi, ularni ham uch marta bosib yozing.

The Cyrillic Uzbek letters ў, қ, ғ, ҳ are not on a Russian keyboard. YAY gives them the same way: double tap, triple tap to get the plain letters. It is active on its own while the Uzbek (Cyrillic) layout is selected. On the Russian (RU) layout it is active only if you tick "Rus (RU) klaviaturasida ham ishlash" in the program window, because you may also use that layout to write real Russian. With it on, Russian words with `кк` or `уу` (*аккорд*, *вакуум*) are converted too, and you tap a third time to undo. Type *икки* as `иккки`.

## O'rnatish / Install

[Releases](https://github.com/nodiranor/yangi_alifboda_yoz/releases) sahifasidan `YAY_Setup.exe` (o'rnatuvchi) yoki `YAY.exe` (o'rnatmasdan ishlaydi) ni yuklab oling. Windows 7 SP1 va undan yangisi, 32 yoki 64 bit. `YAY.exe /tray` oynasiz, faqat tizim patnisida ishga tushadi.

Download `YAY_Setup.exe` (installer) or the portable `YAY.exe` from the Releases page. Windows 7 SP1 or newer, 32 or 64 bit. `YAY.exe /tray` starts in the tray without a window.

O'zbek (lotin) klaviaturasi Windows 8 va undan yangisida o'rnatuvchi yoki dastur oynasidagi tugma orqali qo'shiladi. Windows 7 da tugma ro'yxatga yozuv qo'shadi, undan keyin tizimdan chiqib qayta kirish kerak.

On Windows 8 and later the Uzbek (Latin) layout is added by the installer or by the button in the program window. On Windows 7 the button writes the registry entries, and you need to sign out and back in.

## Yig'ish / Building

Visual Studio 2022 ("Desktop development with C++") kerak. O'rnatuvchi uchun Inno Setup 6 ham kerak.

You need Visual Studio 2022 with the "Desktop development with C++" workload, and Inno Setup 6 for the installer.

```bat
build.cmd
```

Skript dvigatel testlarini ishga tushiradi, so'ng `YAY.exe` va (Inno Setup topilsa) `Output\YAY_Setup.exe` ni yig'adi. The script runs the engine tests, then builds `YAY.exe` and, if Inno Setup is found, `Output\YAY_Setup.exe`.

```bat
tests\e2e.cmd
```

Bu real klaviatura bosishlarini dastur oynasidagi sinov maydoniga yuboradi va natijani tekshiradi. U ishlayotgan YAY ni to'xtatadi. This types real key presses into the program's test box and checks the result. It stops any running YAY.

Ikonkalarni `Icon.png` dan qayta yaratish (Pillow kerak) / regenerate the icons from `Icon.png` (needs Pillow):

```bat
python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico
```

## Eslatmalar / Notes

- Klaviatura `WH_KEYBOARD_LL` hook orqali kuzatiladi, almashtirish `SendInput` bilan yuboriladi. Dastur tarmoqqa ulanmaydi va yozilganlarni saqlamaydi. / Keys are watched with a low-level keyboard hook and replacements are sent with `SendInput`. The program makes no network connections and does not store what you type.
- `YAY.exe` C runtime kutubxonasisiz yig'iladi (taxminan 57 KB), shuning uchun kodda `printf` yoki STL o'rniga Win32 funksiyalari ishlatiladi. / `YAY.exe` is linked without the C runtime (about 57 KB), so the code uses Win32 functions instead of `printf` or the STL.
- Cheklov: avtomatik to'ldirish bor maydonlarda (masalan, Chrome manzil satri) YAY yuboradigan Backspace harf o'rniga taklif qilingan matnni o'chirishi mumkin. / Known limitation: in fields with inline autocomplete (for example the Chrome address bar), the Backspace that YAY sends can delete the suggested text instead of the typed letter.

## Litsenziya / License

[MIT](LICENSE)
