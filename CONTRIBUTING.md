# Hissa qöşiş

Takliflar va xatoliklar uçun Issue oçing. Xatolik haqida yozganda YAY va Windows versiyasini, dastur nomini (Word, Chrome, Telegram...) va aynan qaysi tugmalar bosilganini körsating.

## Yiğiş va sinov

Batafsil [README.md](README.md) da. Qisqaça: `build.cmd` dvigatel testlarini işga tuşiradi va exe faylni yiğadi, `tests\e2e.cmd` esa real klaviatura bosişlarini dasturning sinov maydoniga yuboradi.

## Kod qayerda

- `src/engine.cpp`: lotin yoziş qoidalari (ikki va uç marta bosiş). Unda Win32 çaqiruvlari yöq, şuning uçun uni `tests/test_engine.cpp` bevosita tekşiradi.
- `src/translit.cpp`: kirilldan yangi özbek lotiniga ögiriş, harfma-harf (`tests/test_translit.cpp`).
- `src/keymap.cpp`: faol klaviaturada tugma qaysi kirill harfini berişini Windowsdan söraydi (`tests/test_keymap.cpp`).
- `src/hook.cpp`: klaviatura hodisalarini dvigatelga uzatadi va almaştirişni yuboradi.
- `src/inject.cpp`: almaştiriş matnini `SendInput` orqali faol dasturga yuboradi.
- `src/settings.cpp`: sozlamalar va Windows bilan birga işga tuşiriş yozuvi. Ikkalasi röyxatda saqlanadi.
- `src/main.cpp`: kiriş nuqtasi, dastur ikkinçi marta işga tuşmasligini tekşiriş, xabarlar tsikli va umumiy holat. Fayllar baham köradigan hamma narsa `src/app.h` da keltirilgan.
- `src/nocrt.cpp`: exe C runtime kutubxonasiz yiğilgani uçun hali ham kerak bölgan ikkita C funksiyasi.
- `src/dashboard.cpp`, `src/tray.cpp`, `src/pill.cpp`: dastur oynasi, soat yonidagi belgi (tray) va kiçik maslahat oynaçasi.
- `src/layout.cpp`: faol klaviaturani aniqlaş va Özbek klaviaturasini qöşiş.
- `res/`: oyna joylaşuvi (dialog), manifest va ikonkalar.

## Kod yoziş qoidalari

- Yoziş qoidasini özgartirmoqçi bölsangiz, avval test qöşing, söng kodni özgartiring.
- Enter, Tab va Escape tugmalariga heç qaçon tegmang. Birinçi versiyadagi muammo aynan şunda edi.
- Faqat Windows API. Exe C runtime kutubxonasiz yiğiladi, şuning uçun `printf` yoki `wcscpy` örniga `wsprintfW`, `lstrcpynW` va şunga öxşaş funksiyalarni işlating. STL va istisnolar (exceptions) işlatilmaydi.
- Windows 7 da işlaş saqlanib qolişi şart. Windows 7 dan yangi funksiyalarni `GetProcAddress` orqali yuklang.
- Oynada dialog resursidagi standart elementlar işlatiladi. Elementlarni özingiz çizmang.
- Tarmoqqa ulanmang va yozilgan matnni jurnalga yozmang.
- Izohda nima uçun deb yozing, nima qilayotganini emas. Kod nima qilayotganini özi körsatadi, izoh esa nega aynan şunday bölişi kerakligini tuşuntirişi kerak.
- Exe hajmini kiçik saqlang.
