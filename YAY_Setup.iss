#define MyAppName "YAY - Yangi Alifboda Yoz"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "YAY"
#define MyAppExeName "YAY.exe"

[Setup]
AppId={{8E4A9B2C-5D1F-4A8E-9C3B-7F2D6E1A4B90}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName={autopf}\YAY
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=Output
OutputBaseFilename=YAY_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=force
SourceDir=.

; Wizard offered in Uzbek and Russian ONLY
[Languages]
Name: "uzbek"; MessagesFile: "compiler:Default.isl,Uzbek.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "Ish stolida yorliq yaratish / Создать ярлык на рабочем столе"; GroupDescription: "Qo'shimcha amallar / Дополнительно:"
Name: "startup"; Description: "Windows bilan birga avtomatik ishga tushirish / Автозапуск вместе с Windows"; GroupDescription: "Qo'shimcha sozlamalar / Дополнительные параметры:"
Name: "addinputmethod"; Description: "Windows Til paneliga O'zbek Lotin tilini qo'shish (Win+Space) / Добавить Узбекский (Латиница) в языковую панель"; GroupDescription: "Qo'shimcha sozlamalar / Дополнительные параметры:"

[Files]
Source: "{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; Auto-start with Windows in Tray
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "YAY_UzbekKeyboard"; ValueData: """{app}\{#MyAppExeName}"" /tray"; Flags: uninsdeletevalue; Tasks: startup

[Run]
; Add uz-Latn-UZ to Windows Input Methods silently
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command ""$list = Get-WinUserLanguageList; $found = $false; foreach ($lang in $list) {{ if ($lang.LanguageTag -like 'uz*') {{ $found = $true; break }} }}; if (-not $found) {{ $newLang = New-WinUserLanguageList 'uz-Latn-UZ'; foreach ($nl in $newLang) {{ $list.Add($nl) }}; Set-WinUserLanguageList $list -Force }}"""; Flags: runhidden waituntilterminated; Tasks: addinputmethod

; Launch the app immediately with visible dashboard
Filename: "{app}\{#MyAppExeName}"; Description: "Dasturni hozir ishga tushirish / Запустить программу сейчас"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Cleanly stop the running daemon before uninstalling
Filename: "taskkill.exe"; Parameters: "/F /IM {#MyAppExeName}"; Flags: runhidden; RunOnceId: "StopYAY"

[UninstallDelete]
Type: files; Name: "{app}\{#MyAppExeName}"
