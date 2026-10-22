; *** Inno Setup Uzbek (Latin) Messages ***
; Custom Uzbek translation overlay for YAY installer
; Uses Default.isl as the base, overrides with Uzbek text

[LangOptions]
LanguageName=O'zbekcha
LanguageID=$0443
LanguageCodePage=0

[Messages]
; === Window titles ===
SetupAppTitle=O'rnatish
SetupWindowTitle=O'rnatish - %1
UninstallAppTitle=O'chirish
UninstallAppFullTitle=%1 dasturini o'chirish

; === Common titles ===
InformationTitle=Ma'lumot
ConfirmTitle=Tasdiqlash
ErrorTitle=Xatolik

; === Startup ===
SetupLdrStartupMessage=%1 dasturi kompyuteringizga o'rnatiladi. Davom ettirishni xohlaysizmi?
LdrCannotCreateTemp=Vaqtinchalik fayl yaratib bo'lmadi. O'rnatish to'xtatildi
LdrCannotExecTemp=Vaqtinchalik papkada faylni ishga tushirib bo'lmadi. O'rnatish to'xtatildi
HelpTextNote=

; === Language selection ===
SelectLanguageTitle=O'rnatish tilini tanlang
SelectLanguageLabel=O'rnatish jarayonida foydalaniladigan tilni tanlang:

; === Welcome page (uses [name] and [name/ver] placeholders) ===
WelcomeLabel1=[name] o'rnatish ustasiga xush kelibsiz!
WelcomeLabel2=Bu dastur [name/ver] ni kompyuteringizga o'rnatadi.%n%nDavom etishdan oldin boshqa dasturlarni yopish tavsiya etiladi.

; === Navigation ===
ClickNext=Davom etish uchun "Keyingisi" tugmasini, o'rnatishdan chiqish uchun esa "Bekor qilish" tugmasini bosing.
BeveledLabel=O'zbekcha

; === Buttons ===
ButtonNext=&Keyingisi >
ButtonBack=< &Orqaga
ButtonInstall=&O'rnatish
ButtonOK=OK
ButtonCancel=Bekor qilish
ButtonYes=&Ha
ButtonYesToAll=Barchasiga &ha
ButtonNo=&Yo'q
ButtonNoToAll=Barchasiga yo'&q
ButtonFinish=&Tugatish
ButtonBrowse=&Ko'rish...

; === Install mode selection (PrivilegesRequired=lowest with overrides) ===
PrivilegesRequiredOverrideTitle=O'rnatish rejimini tanlang
PrivilegesRequiredOverrideInstruction=O'rnatish usulini tanlang
PrivilegesRequiredOverrideText1=%1 dasturini barcha foydalanuvchilar uchun (administrator huquqi kerak) yoki faqat siz uchun o'rnatish mumkin.
PrivilegesRequiredOverrideText2=%1 dasturini faqat siz uchun yoki barcha foydalanuvchilar uchun (administrator huquqi kerak) o'rnatish mumkin.
PrivilegesRequiredOverrideAllUsers=&Barcha foydalanuvchilar uchun o'rnatish
PrivilegesRequiredOverrideAllUsersRecommended=&Barcha foydalanuvchilar uchun o'rnatish (tavsiya etiladi)
PrivilegesRequiredOverrideCurrentUser=Faqat &men uchun o'rnatish
PrivilegesRequiredOverrideCurrentUserRecommended=Faqat &men uchun o'rnatish (tavsiya etiladi)

; === Directory selection (uses [name] placeholder) ===
WizardSelectDir=O'rnatish papkasini tanlash
SelectDirDesc=[name] dasturi qayerga o'rnatilsin?
SelectDirLabel3=[name] quyidagi papkaga o'rnatiladi.
SelectDirBrowseLabel=Boshqa papkaga o'rnatish uchun "Ko'rish" tugmasini bosing:
DiskSpaceMBLabel=O'rnatish uchun kamida [mb] MB bo'sh joy kerak.

; === Tasks selection ===
WizardSelectTasks=Qo'shimcha sozlamalar
SelectTasksDesc=Qanday qo'shimcha amallar bajarilsin?
SelectTasksLabel2=Bajarilishi kerak bo'lgan qo'shimcha amallarni tanlang:

; === Ready page (uses [name] placeholder) ===
WizardReady=O'rnatishga tayyor
ReadyLabel1=[name] dasturini kompyuterga o'rnatish uchun barcha parametrlar tayyor.
ReadyLabel2a=O'rnatishni boshlash uchun "O'rnatish" tugmasini bosing:
ReadyLabel2b=O'rnatishni boshlash uchun "O'rnatish" tugmasini bosing.

; === Installing page (uses [name] placeholder) ===
WizardInstalling=O'rnatilmoqda
InstallingLabel=[name] dasturi o'rnatilmoqda, kuting...

; === Finished page (uses [name] placeholder) ===
FinishedHeadingLabel=[name] o'rnatildi!
FinishedLabelNoIcons=[name] kompyuteringizga muvaffaqiyatli o'rnatildi.
FinishedLabel=[name] kompyuteringizga muvaffaqiyatli o'rnatildi.
ClickFinish=O'rnatish ustasidan chiqish uchun "Tugatish" tugmasini bosing.

; === Exit confirmation ===
ExitSetupTitle=O'rnatishni to'xtatish
ExitSetupMessage=Dasturni o'rnatish hali yakunlanmadi. Agar hozir chiqsangiz, dastur o'rnatilmaydi.%n%nO'rnatishdan chiqishni xohlaysizmi?

; === Status messages ===
StatusClosingApplications=Dasturlar yopilmoqda...
StatusCreateDirs=Papkalar yaratilmoqda...
StatusExtractFiles=Fayllar chiqarilmoqda...
StatusCreateIcons=Yorliqlar yaratilmoqda...
StatusCreateIniEntries=INI yozuvlari yaratilmoqda...
StatusCreateRegistryEntries=Ro'yxatga olish kitobi yozuvlari yaratilmoqda...
StatusRegisterFiles=Fayllar ro'yxatga olinmoqda...
StatusSavingUninstall=O'chirish ma'lumotlari saqlanmoqda...
StatusRunProgram=O'rnatish yakunlanmoqda...
StatusRestartingApplications=Dasturlar qayta ishga tushirilmoqda...
StatusRollback=O'zgarishlar bekor qilinmoqda...

; === Misc ===
SetupAppRunningError=O'rnatish %1 dasturi hozirda ishlayotganini aniqladi.%n%nIltimos, barcha oynalarini yoping va davom etish uchun OK tugmasini bosing yoki bekor qilish uchun Bekor qilish tugmasini bosing.
UninstallAppRunningError=O'chirish %1 dasturi hozirda ishlayotganini aniqladi.%n%nIltimos, barcha oynalarini yoping va davom etish uchun OK tugmasini bosing yoki bekor qilish uchun Bekor qilish tugmasini bosing.
