#define MyAppName "YAY - Yangi Alifboda Yoz"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "YAY"
#define MyAppExeName "YAY.exe"
#define SponsorUrl "https://t.me/juraboshibot"

; Installer for YAY, built with Inno Setup 6 (build.cmd runs it). The wizard is offered in Uzbek
; and Russian. Run build.cmd first: this script packs the YAY.exe that build.cmd produces.

[Setup]
AppId={{8E4A9B2C-5D1F-4A8E-9C3B-7F2D6E1A4B90}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName={autopf}\YAY
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; Install for the current user, so no administrator rights are needed. The wizard still offers
; "all users" as a choice (the line below).
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Windows 7 with Service Pack 1 or newer.
MinVersion=6.1sp1
OutputDir=Output
OutputBaseFilename=YAY_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
; Close a running YAY so its file can be replaced (see also PrepareToInstall below).
CloseApplications=force
SetupIconFile=res\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
SourceDir=.

; Wizard offered in Uzbek and Russian
[Languages]
Name: "uzbek"; MessagesFile: "compiler:Default.isl,Uzbek.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

; Optional tasks, shown on the wizard page "additional tasks". All are ticked by default.
; MinVersion 6.2 hides the layout task on Windows 7, where the PowerShell command does not exist
; (the program offers its own Windows 7 path from its window).
[Tasks]
Name: "desktopicon"; Description: "Iş stolida yorliq yaratiş / Создать ярлык на рабочем столе"; GroupDescription: "Qöşimça amallar / Дополнительно:"
Name: "startup"; Description: "Windows bilan birga avtomatik işga tuşiriş / Автозапуск вместе с Windows"; GroupDescription: "Qöşimça sozlamalar / Дополнительные параметры:"
Name: "addinputmethod"; Description: "Windows Til paneliga Özbek Lotin tilini qöşiş (Win+Space) / Добавить Узбекский (Латиница) в языковую панель"; GroupDescription: "Qöşimça sozlamalar / Дополнительные параметры:"; MinVersion: 6.2

[Files]
Source: "{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "install_uzbek_layout.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

; Start with Windows, only if the "startup" task is ticked. This is the same value the program
; writes itself (src/settings.cpp). uninsdeletevalue removes it again on uninstall.
[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "YAY_UzbekKeyboard"; ValueData: """{app}\{#MyAppExeName}"" /tray"; Flags: uninsdeletevalue; Tasks: startup

[Run]
; Windows 8+ only: the cmdlet does not exist on Windows 7. The app offers its own Windows 7 path.
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File ""{app}\install_uzbek_layout.ps1"""; Flags: runhidden waituntilterminated; Tasks: addinputmethod; MinVersion: 6.2
Filename: "{app}\{#MyAppExeName}"; Description: "Dasturni hozir işga tuşiriş / Запустить программу сейчас"; Flags: nowait postinstall

; Stop YAY before its files are removed.
[UninstallRun]
Filename: "taskkill.exe"; Parameters: "/F /IM {#MyAppExeName}"; Flags: runhidden; RunOnceId: "StopYAY"

[UninstallDelete]
Type: files; Name: "{app}\{#MyAppExeName}"
Type: files; Name: "{app}\install_uzbek_layout.ps1"

[Code]
var
  SponsorLink: TNewStaticText;

// Stop a running YAY before files are copied, otherwise Windows cannot replace YAY.exe.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;

// Opens the sponsor page in the default browser.
procedure SponsorLinkClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('open', '{#SponsorUrl}', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

// Adds the clickable sponsor line to the last page of the wizard, at the bottom left.
procedure InitializeWizard();
begin
  SponsorLink := TNewStaticText.Create(WizardForm);
  SponsorLink.Parent := WizardForm.FinishedPage;
  SponsorLink.Caption := 'Homiy / Спонсор: t.me/juraboshibot';
  SponsorLink.Cursor := crHand;
  SponsorLink.Font.Color := clBlue;
  SponsorLink.Font.Style := [fsUnderline];
  SponsorLink.OnClick := @SponsorLinkClick;
  SponsorLink.Left := WizardForm.FinishedLabel.Left;
  SponsorLink.Top := WizardForm.FinishedPage.Height - SponsorLink.Height - ScaleY(12);
end;
