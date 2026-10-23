# YAY — Yangi Alifboda Yoz

![YAY oynasi](docs/dashboard.png)

**YAY** özbek lotin alifbosining yangi harflarini (ş, ç, ö, ğ) oddiy klaviaturada yoziş uçun kiçik Windows dasturi. Kirill klaviaturaga örganib qolganlar uçun esa kirillda yozganini yangi lotin alifbosiga ögiradi. U soat yonida, ekranning pastki öng burçagida belgi bölib turadi va faqat Özbek (lotin) yoki Özbek (kirill) klaviaturasi tanlanganda işlaydi (Win + Space). Ingliz klaviaturasiga tegmaydi. Rus klaviaturasiga faqat sozlamada yoqsangiz tegadi.

**Yuklab oliş:** [YangiAlifbodaYoz.uz](https://yangialifbodayoz.uz) · [YAY_Setup.exe (GitHub)](https://github.com/nodiranor/yangi_alifboda_yoz/raw/main/Output/YAY_Setup.exe)

Homiy: [t.me/juraboshibot](https://t.me/juraboshibot)

## Qoidalar

| Harf | Tez usul | Oddiy ikki harf | Eski usul |
| :---: | :---: | :---: | :---: |
| ş Ş | `ss` `SS` | `sss` → `ss` | `sh` `SH` |
| ç Ç | `cc` `CC` | `ccc` → `cc` | `ch` `CH` |
| ö Ö | `oo` `OO` | `ooo` → `oo` | `o'` `O'` |
| ğ Ğ | `gg` `GG` | `ggg` → `gg` | `g'` `G'` |

- Bir xil harfni ikki marta tez bossangiz, ular bitta yangi harfga almaşadi: `ss` → ş. Oddiy ikki harf kerak bölsa (*Rossiya*, *issiq*), uçinçi marta bosing: `sss` → `ss`. Keyingi bosişlar oddiy harf bölib qoladi: `ssss` → `sss`.
- "Tez" taxminan 0,35 soniya. Eski usullar (`sh`, `ch`, `o'`, `g'`) tezlikka boğliq emas.
- Enter, Tab va Escape tugmalariga heç qaçon tegilmaydi.
- ş dan keyin s keladigan sözlarda (*işsiz*, *tuşsa*) `sh` + `s` yozing yoki ikkinçi `s` dan keyin biroz töxtang.
- Dastur oynasida "Bitta 'c' harfini darhol 'ç' qilib yoziş" sozlamasi bor.

### Kirilldan lotinga

Kirill klaviaturaga örganganlar uçun: Özbek (kirill) klaviaturasi tanlansa, kirillda yozganingiz yangi lotin alifbosiga ögiriladi. Tugmalar odatdagidek, faqat ekranda lotin çiqadi. Rus (RU) klaviaturasida bu faqat dastur oynasida "Rus (RU) klaviaturasida ham ögiriş" belgilansa işlaydi, çunki RU da rus tilida ham yozasiz. Belgilangan bölsa, rus matni ham lotinga ögiriladi: rus tilida yoziş uçun belgini olib taşlang yoki soat yonidagi YAY belgisini bosib, dasturni töxtating.

| Kirill | Lotin | Misol |
| :---: | :---: | :--- |
| ш Ш | ş Ş | шаҳар → şahar |
| ч Ч | ç Ç | чой → çoy |
| ў Ў | ö Ö | Ўзбекистон → Özbekiston |
| ғ Ғ | ğ Ğ | ғалаба → ğalaba |
| қ ҳ х | q h x | қалам → qalam, ҳаво → havo, яхши → yaxşi |
| ё ю я | yo yu ya | ёмон → yomon, юрак → yurak |
| е | ye söz boşida va unlidan keyin, boşqa joyda e | Европа → Yevropa, беш → beş |
| э | e | эшик → eşik |
| ц щ ы | ts şç i | цирк → tsirk |
| ъ | ʼ | маъно → maʼno |
| ь | yozilmaydi | сентябрь → sentyabr |

Shift bilan bosilgan ё, ю, я katta harf bilan boşlanadi (*Юлдуз* → *Yulduz*). Caps Lock yoqilgan bölsa töliq katta harf çiqadi (*ЮЛДУЗ* → *YULDUZ*). ў, қ, ғ, ҳ tugmalari faqat Özbek (kirill) klaviaturasida bor.

## Örnatiş

Örnatuvçi `YAY_Setup.exe` ni ikki joydan yuklab olişingiz mumkin:

- Rasmiy veb-sayt: [YangiAlifbodaYoz.uz](https://yangialifbodayoz.uz)
- GitHub repozitoriysi: [Output/YAY_Setup.exe](https://github.com/nodiranor/yangi_alifboda_yoz/raw/main/Output/YAY_Setup.exe)

Faylni işga tuşirib, körsatmalarga amal qiling. Dasturni özingiz yiğişingiz şart emas. Örnatmasdan işlaydigan `YAY.exe` ni [Releases](https://github.com/nodiranor/yangi_alifboda_yoz/releases) sahifasidan yuklab oling. Windows 7 SP1 va undan yangisi, 32 yoki 64 bit. `YAY.exe /tray` oynasiz, faqat soat yonidagi belgi bölib işga tuşadi.

Özbek (lotin) klaviaturasi Windows 8 va undan yangisida örnatuvçi yoki dastur oynasidagi tugma orqali qöşiladi. Windows 7 da tugma röyxatga yozuv qöşadi, undan keyin tizimdan çiqib qayta kiriş kerak.

## Yiğiş

Bu bölim faqat dastur kodini özgartirmoqçi bölganlar uçun. Oddiy foydalanuvçilar yuqoridagi örnatuvçini yuklab olsa kifoya. Visual Studio 2022 ("Desktop development with C++") kerak. Örnatuvçi uçun Inno Setup 6 ham kerak.

```bat
build.cmd
```

Skript dvigatel testlarini işga tuşiradi, söng `YAY.exe` va (Inno Setup topilsa) `Output\YAY_Setup.exe` ni yiğadi.

```bat
tests\e2e.cmd
```

Bu real klaviatura bosişlarini dastur oynasidagi sinov maydoniga yuboradi va natijani tekşiradi. U işlayotgan YAY ni töxtatadi.

Ikonkalarni `Icon.png` dan qayta yaratiş (Pillow kerak):

```bat
python tools/make_icons.py Icon.png res/app.ico res/app_paused.ico
```

## Eslatmalar

- Klaviatura `WH_KEYBOARD_LL` hook orqali kuzatiladi, almaştiriş `SendInput` bilan yuboriladi. Dastur tarmoqqa ulanmaydi va yozilganlarni saqlamaydi.
- `YAY.exe` C runtime kutubxonasiz yiğiladi (taxminan 51 KB), şuning uçun kodda `printf` yoki STL örniga Win32 funksiyalari işlatiladi.
- Çeklov: avtomatik töldiriş bor maydonlarda (masalan, Chrome manzil satri) YAY yuboradigan Backspace harf örniga taklif qilingan matnni öçirişi mumkin.

## Litsenziya

[MIT](LICENSE)
