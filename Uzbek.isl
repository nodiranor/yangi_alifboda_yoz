; *** Inno Setup Uzbek (new Latin alphabet) Messages ***
; Translation overlay for the YAY installer.
; Uses Default.isl as the base and overrides it with Uzbek text.

[LangOptions]
LanguageName=Özbekça
LanguageID=$0443
LanguageCodePage=0

[Messages]
; === Window titles ===
SetupAppTitle=Örnatiş
SetupWindowTitle=Örnatiş - %1
UninstallAppTitle=Öçiriş
UninstallAppFullTitle=%1 dasturini öçiriş

; === Common titles ===
InformationTitle=Maʼlumot
ConfirmTitle=Tasdiqlaş
ErrorTitle=Xatolik

; === Startup ===
SetupLdrStartupMessage=%1 dasturi kompyuteringizga örnatiladi. Davom ettirişni xohlaysizmi?
LdrCannotCreateTemp=Vaqtinçalik fayl yaratib bölmadi. Örnatiş töxtatildi
LdrCannotExecTemp=Vaqtinçalik papkada faylni işga tuşirib bölmadi. Örnatiş töxtatildi
HelpTextNote=

; === Language selection ===
SelectLanguageTitle=Örnatiş tilini tanlang
SelectLanguageLabel=Örnatiş jarayonida foydalaniladigan tilni tanlang:

; === Welcome page (uses [name] and [name/ver] placeholders) ===
WelcomeLabel1=[name] örnatiş ustasiga xuş kelibsiz!
WelcomeLabel2=Bu dastur [name/ver] ni kompyuteringizga örnatadi.%n%nDavom etişdan oldin boşqa dasturlarni yopiş tavsiya etiladi.

; === Navigation ===
ClickNext=Davom etiş uçun "Keyingisi" tugmasini, örnatişdan çiqiş uçun esa "Bekor qiliş" tugmasini bosing.
BeveledLabel=Özbekça

; === Buttons ===
ButtonNext=&Keyingisi >
ButtonBack=< &Orqaga
ButtonInstall=Ö&rnatiş
ButtonOK=OK
ButtonCancel=Bekor qiliş
ButtonYes=&Ha
ButtonYesToAll=Barçasiga &ha
ButtonNo=&Yöq
ButtonNoToAll=Barçasiga yö&q
ButtonFinish=&Tugatiş
ButtonBrowse=&Köriş...

; === Install mode selection (PrivilegesRequired=lowest with overrides) ===
PrivilegesRequiredOverrideTitle=Örnatiş rejimini tanlang
PrivilegesRequiredOverrideInstruction=Örnatiş usulini tanlang
PrivilegesRequiredOverrideText1=%1 dasturini barça foydalanuvçilar uçun (administrator huquqi kerak) yoki faqat siz uçun örnatiş mumkin.
PrivilegesRequiredOverrideText2=%1 dasturini faqat siz uçun yoki barça foydalanuvçilar uçun (administrator huquqi kerak) örnatiş mumkin.
PrivilegesRequiredOverrideAllUsers=&Barça foydalanuvçilar uçun örnatiş
PrivilegesRequiredOverrideAllUsersRecommended=&Barça foydalanuvçilar uçun örnatiş (tavsiya etiladi)
PrivilegesRequiredOverrideCurrentUser=Faqat &men uçun örnatiş
PrivilegesRequiredOverrideCurrentUserRecommended=Faqat &men uçun örnatiş (tavsiya etiladi)

; === Directory selection (uses [name] placeholder) ===
WizardSelectDir=Örnatiş papkasini tanlaş
SelectDirDesc=[name] dasturi qayerga örnatilsin?
SelectDirLabel3=[name] quyidagi papkaga örnatiladi.
SelectDirBrowseLabel=Boşqa papkaga örnatiş uçun "Köriş" tugmasini bosing:
DiskSpaceMBLabel=Örnatiş uçun kamida [mb] MB böş joy kerak.

; === Tasks selection ===
WizardSelectTasks=Qöşimça sozlamalar
SelectTasksDesc=Qanday qöşimça amallar bajarilsin?
SelectTasksLabel2=Bajarilişi kerak bölgan qöşimça amallarni tanlang:

; === Ready page (uses [name] placeholder) ===
WizardReady=Örnatişga tayyor
ReadyLabel1=[name] dasturini kompyuterga örnatiş uçun barça parametrlar tayyor.
ReadyLabel2a=Örnatişni boşlaş uçun "Örnatiş" tugmasini bosing:
ReadyLabel2b=Örnatişni boşlaş uçun "Örnatiş" tugmasini bosing.

; === Installing page (uses [name] placeholder) ===
WizardInstalling=Örnatilmoqda
InstallingLabel=[name] dasturi örnatilmoqda, kuting...

; === Finished page (uses [name] placeholder) ===
FinishedHeadingLabel=[name] örnatildi!
FinishedLabelNoIcons=[name] kompyuteringizga muvaffaqiyatli örnatildi.
FinishedLabel=[name] kompyuteringizga muvaffaqiyatli örnatildi.
ClickFinish=Örnatiş ustasidan çiqiş uçun "Tugatiş" tugmasini bosing.

; === Exit confirmation ===
ExitSetupTitle=Örnatişni töxtatiş
ExitSetupMessage=Dasturni örnatiş hali yakunlanmadi. Agar hozir çiqsangiz, dastur örnatilmaydi.%n%nÖrnatişdan çiqişni xohlaysizmi?

; === Status messages ===
StatusClosingApplications=Dasturlar yopilmoqda...
StatusCreateDirs=Papkalar yaratilmoqda...
StatusExtractFiles=Fayllar çiqarilmoqda...
StatusCreateIcons=Yorliqlar yaratilmoqda...
StatusCreateIniEntries=INI yozuvlari yaratilmoqda...
StatusCreateRegistryEntries=Röyxatga oliş kitobi yozuvlari yaratilmoqda...
StatusRegisterFiles=Fayllar röyxatga olinmoqda...
StatusSavingUninstall=Öçiriş maʼlumotlari saqlanmoqda...
StatusRunProgram=Örnatiş yakunlanmoqda...
StatusRestartingApplications=Dasturlar qayta işga tuşirilmoqda...
StatusRollback=Özgarişlar bekor qilinmoqda...

; === Misc ===
SetupAppRunningError=Örnatiş %1 dasturi hozirda işlayotganini aniqladi.%n%nIltimos, barça oynalarini yoping va davom etiş uçun OK tugmasini bosing yoki bekor qiliş uçun Bekor qiliş tugmasini bosing.
UninstallAppRunningError=Öçiriş %1 dasturi hozirda işlayotganini aniqladi.%n%nIltimos, barça oynalarini yoping va davom etiş uçun OK tugmasini bosing yoki bekor qiliş uçun Bekor qiliş tugmasini bosing.
