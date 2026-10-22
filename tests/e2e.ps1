# End-to-end typing test for YAY. Run through tests\e2e.cmd, which builds build\YAY_test.exe first.
#
# The test exe differs from the real one in two ways (see YAY_ACCEPT_INJECTED in src): it also
# processes keys injected by other programs, and it works on the English and Russian layouts.
# This script starts it, types real key events into the dashboard's test box and reads the box back.
# The Cyrillic part briefly switches the dashboard's keyboard layout to Russian and switches it back.
#
# Warning: stops any running YAY.exe / YAY_test.exe first.

param([string]$Exe = (Join-Path $PSScriptRoot "..\build\YAY_test.exe"))

Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public static class K {
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string c, string t);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, StringBuilder l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, string l);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern short GetKeyState(int vk);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr LoadKeyboardLayout(string id, uint flags);
  [DllImport("user32.dll")] public static extern IntPtr GetKeyboardLayout(uint tid);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern uint MapVirtualKeyEx(uint code, uint type, IntPtr hkl);
  [DllImport("user32.dll")] public static extern int ToUnicodeEx(uint vk, uint scan, byte[] state, [Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder buf, int n, uint flags, IntPtr hkl);
}
"@

$null_ = [NullString]::Value
$IDC_EDIT_TEST = 3006
$WM_GETTEXT = 0x0D
$WM_SETTEXT = 0x0C

function Tap([byte]$vk, [bool]$shift = $false) {
  if ($shift) { [K]::keybd_event(0x10, 0, 0, [UIntPtr]::Zero) }
  [K]::keybd_event($vk, 0, 0, [UIntPtr]::Zero)
  [K]::keybd_event($vk, 0, 2, [UIntPtr]::Zero)
  if ($shift) { [K]::keybd_event(0x10, 0, 2, [UIntPtr]::Zero) }
  Start-Sleep -Milliseconds 45
}

# Key script: letters (uppercase = with Shift), ' apostrophe, space, < Backspace, | 600 ms pause.
function TypeKeys([string]$keys) {
  foreach ($c in $keys.ToCharArray()) {
    if ($c -ge 'a' -and $c -le 'z') { Tap ([byte][char]::ToUpper($c)) $false }
    elseif ($c -ge 'A' -and $c -le 'Z') { Tap ([byte]$c) $true }
    elseif ($c -eq "'") { Tap 0xDE $false }
    elseif ($c -eq ' ') { Tap 0x20 $false }
    elseif ($c -eq '<') { Tap 0x08 $false }
    elseif ($c -eq '|') { Start-Sleep -Milliseconds 600 }
  }
  Start-Sleep -Milliseconds 120
}

function ReadEdit {
  $sb = New-Object System.Text.StringBuilder 256
  [void][K]::SendMessage($script:edit, $WM_GETTEXT, [IntPtr]256, $sb)
  return $sb.ToString()
}
# The status line at the top of the dashboard (IDC_STATUS_TEXT). It uses the same mode lookup as the
# tray tooltip, so it shows what the tooltip shows.
function ReadStatus {
  $sb = New-Object System.Text.StringBuilder 300
  [void][K]::SendMessage([K]::GetDlgItem($script:dash, 3007), $WM_GETTEXT, [IntPtr]300, $sb)
  return $sb.ToString()
}
# Waits for the window's once-a-second refresh, then checks that the status line contains `part`.
function CheckStatus([string]$label, [string]$part) {
  Start-Sleep -Milliseconds 1300
  $text = ReadStatus
  $ok = ($text -like "*$part*")
  if (-not $ok) { $script:fail++ }
  "{0,-6} status line on {1}: contains [{2}] -> {3}" -f $(if ($ok) { 'ok' } else { 'FAIL' }), $label, $part, $ok
}

function ClearEdit { [void][K]::SendMessage($script:edit, $WM_SETTEXT, [IntPtr]::Zero, "") }

Get-Process -Name YAY, YAY_test -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
if (-not (Test-Path $Exe)) { Write-Host "Missing $Exe. Run tests\e2e.cmd."; exit 2 }
$proc = Start-Process -FilePath $Exe -PassThru

$dash = [IntPtr]::Zero
for ($i = 0; $i -lt 100 -and $dash -eq [IntPtr]::Zero; $i++) {
  Start-Sleep -Milliseconds 50
  $dash = [K]::FindWindow("YAYDashboardClass", $null_)
}
if ($dash -eq [IntPtr]::Zero) { Write-Host "Dashboard window never appeared"; $proc | Stop-Process -Force; exit 2 }
Start-Sleep -Milliseconds 600

# Windows refuses SetForegroundWindow from a background process unless Alt was just pressed.
[K]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); [K]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
[void][K]::SetForegroundWindow($dash)
Start-Sleep -Milliseconds 500
if ([K]::GetForegroundWindow() -ne $dash) { Write-Host "Could not bring the dashboard to the foreground"; $proc | Stop-Process -Force; exit 2 }
if (([K]::GetKeyState(0x14) -band 1) -ne 0) { Tap 0x14 $false }   # make sure Caps Lock is off

$edit = [K]::GetDlgItem($dash, $IDC_EDIT_TEST)

$S = [string][char]0x015F; $SU = [string][char]0x015E
$C = [string][char]0x00E7; $O = [string][char]0x00F6; $G = [string][char]0x011F

$cases = @(
  @('ss', $S), @('SS', $SU), @('Ss', $SU), @('sss', 'ss'), @('SSS', 'SS'), @('ssss', 'sss'),
  @('oo', $O), @('ooo', 'oo'), @('gg', $G), @('ggg', 'gg'), @('cc', $C), @('ccc', 'cc'),
  @('isssiq', 'issiq'), @('issiq', "i${S}iq"), @('Rosssiya', 'Rossiya'), @('Asssalomu', 'Assalomu'),
  @('ishsiz', "i${S}siz"), @('tushsa', "tu${S}sa"), @('tusssa', 'tussa'),
  @('shahar', "${S}ahar"), @('ssahar', "${S}ahar"), @('Shahar', "${SU}ahar"),
  @('choy', "${C}oy"), @('ccoy', "${C}oy"),
  @("o'rdak", "${O}rdak"), @('oordak', "${O}rdak"), @("g'alaba", "${G}alaba"), @('ggalaba', "${G}alaba"),
  @('Toshkent', "To${S}kent"), @('s|s', 'ss'), @("ma'no", "ma'no"), @('ss<s', 's'), @('s s', 's s')
)

$fail = 0
foreach ($case in $cases) {
  ClearEdit
  Tap 0x7C $false   # F13: any unrelated key resets the engine between cases
  TypeKeys $case[0]
  $got = ReadEdit
  $ok = ($got -ceq $case[1])
  if (-not $ok) { $fail++ }
  $gotShow = ($got.ToCharArray() | ForEach-Object { if ([int]$_ -lt 128) { [string]$_ } else { '\u{0:X4}' -f [int]$_ } }) -join ''
  $expShow = ($case[1].ToCharArray() | ForEach-Object { if ([int]$_ -lt 128) { [string]$_ } else { '\u{0:X4}' -f [int]$_ } }) -join ''
  "{0,-6} {1,-10} -> {2,-12} expected {3}" -f $(if ($ok) { 'ok' } else { 'FAIL' }), $case[0], $gotShow, $expShow
}

# The test build also runs the Latin rules on the English layout. The status line must describe
# the mode, not claim that the Uzbek layout is selected.
CheckStatus "EN" "Lotin rejimi"

# Coach pill: appears after a legacy digraph, disappears on the next key.
ClearEdit
Tap 0x7C $false
TypeKeys 'sh'
$pill = [K]::FindWindow("YAYPopupClass", $null_)
$shown = ($pill -ne [IntPtr]::Zero) -and [K]::IsWindowVisible($pill)
TypeKeys 'a'
$hidden = -not [K]::IsWindowVisible($pill)
"{0,-6} coach pill shown after 'sh': {1}, hidden after next key: {2}" -f $(if ($shown -and $hidden) { 'ok' } else { 'FAIL' }), $shown, $hidden
if (-not ($shown -and $hidden)) { $fail++ }

# Cyrillic to Latin on the Russian and Uzbek (Cyrillic) layouts. The dashboard's keyboard layout
# is switched for this part and always switched back, and Caps Lock is left as it was.
function Cyr([int[]]$codes) { return (($codes | ForEach-Object { [string][char]$_ }) -join '') }
function Show([string]$s) { return (($s.ToCharArray() | ForEach-Object { if ([int]$_ -lt 128) { [string]$_ } else { '\u{0:X4}' -f [int]$_ } }) -join '') }

# Which key (and Shift state) produces each character on a layout, asked from Windows itself.
function Build-KeyMap($hkl) {
  $map = New-Object 'System.Collections.Generic.Dictionary[string,object]'
  foreach ($vk in 0x30..0xDF) {
    foreach ($shift in $false, $true) {
      $state = New-Object byte[] 256
      if ($shift) { $state[0x10] = 0x80 }
      $sb = New-Object System.Text.StringBuilder 4
      $scan = [K]::MapVirtualKeyEx($vk, 0, $hkl)
      if ([K]::ToUnicodeEx($vk, $scan, $state, $sb, 4, 0, $hkl) -eq 1) {
        $ch = [string]$sb[0]
        if (-not $map.ContainsKey($ch)) { $map[$ch] = @($vk, $shift) }
      }
    }
  }
  return $map
}

# Text: Cyrillic letters, ' ' space, '<' Backspace, '|' 600 ms pause.
function TypeCyr([string]$text, $map) {
  foreach ($c in $text.ToCharArray()) {
    if ($c -eq ' ') { Tap 0x20 $false }
    elseif ($c -eq '<') { Tap 0x08 $false }
    elseif ($c -eq '|') { Start-Sleep -Milliseconds 600 }
    else { $e = $map[[string]$c]; Tap ([byte]$e[0]) ([bool]$e[1]) }
  }
  Start-Sleep -Milliseconds 120
}

function Run-CyrCases($cases, $map, [string]$label) {
  foreach ($case in $cases) {
    $missing = $case[0].ToCharArray() | Where-Object { $_ -notin ' ', '<', '|' -and -not $map.ContainsKey([string]$_) }
    if ($missing) { "{0,-6} {1} {2}: key not found on this layout, skipped" -f 'skip', $label, (Show $case[0]); continue }
    ClearEdit
    Tap 0x7C $false
    TypeCyr $case[0] $map
    $got = ReadEdit
    $ok = ($got -ceq $case[1])
    if (-not $ok) { $script:fail++ }
    "{0,-6} {1} {2,-22} -> {3,-18} expected {4}" -f $(if ($ok) { 'ok' } else { 'FAIL' }), $label, (Show $case[0]), (Show $got), (Show $case[1])
  }
}

$OU = [string][char]0x00D6

$procId = [uint32]0
$tid = [K]::GetWindowThreadProcessId($dash, [ref]$procId)
$origLayout = [K]::GetKeyboardLayout($tid)
$capsWasOn = (([K]::GetKeyState(0x14) -band 1) -ne 0)
try {
  foreach ($layoutCase in @(@("00000419", "RU"), @("00000843", "UZ"))) {
    $hkl = [K]::LoadKeyboardLayout($layoutCase[0], 0)
    $label = $layoutCase[1]
    if ($hkl -eq [IntPtr]::Zero) { "skip   $label layout is not available"; continue }
    [void][K]::PostMessage($dash, 0x800A, [IntPtr]::Zero, $hkl)   # test-only message: switch the dashboard's layout
    Start-Sleep -Milliseconds 500
    $langNow = [int]([K]::GetKeyboardLayout($tid).ToInt64() -band 0xFFFF)
    $langWant = [Convert]::ToInt32($layoutCase[0], 16)
    if ($langNow -ne $langWant) { Write-Host ("FAIL   could not switch the dashboard to layout {0} (now 0x{1:X4})" -f $layoutCase[0], $langNow); $fail++; continue }
    $map = Build-KeyMap $hkl
    CheckStatus $label "Kirillda yozing"

    $words = @(
      @((Cyr 0x0447,0x043E,0x0439),                                   "${C}oy"),                # чой
      @((Cyr 0x0448,0x0430,0x0445,0x0430,0x0440),                     "${S}axar"),              # шахар
      @((Cyr 0x0428,0x0430,0x0445,0x0430,0x0440),                     "${SU}axar"),             # Шахар
      @((Cyr 0x044E,0x0440,0x0430,0x043A),                            "yurak"),                 # юрак
      @((Cyr 0x0415,0x0432,0x0440,0x043E,0x043F,0x0430),              "Yevropa"),               # Европа
      @((Cyr 0x043F,0x043E,0x0435,0x0437,0x0434),                     "poyezd"),                # поезд
      @((Cyr 0x0440,0x043E,0x0441,0x0441,0x0438,0x044F),              "rossiya"),               # россия
      @((Cyr 0x044D,0x0448,0x0438,0x043A),                            "e${S}ik"),               # эшик
      @((Cyr 0x0431,0x0435,0x0020,0x0435),                            "be ye"),                 # бе е
      @((Cyr 0x0431,0x0435,0x003C,0x0435),                            "be")                     # бе<е  (Backspace)
    )
    Run-CyrCases $words $map $label

    if ($label -eq "UZ") {
      $uzWords = @(
        @((Cyr 0x0448,0x0430,0x04B3,0x0430,0x0440),                   "${S}ahar"),              # шаҳар
        @((Cyr 0x040E,0x0437,0x0431,0x0435,0x043A),                   "${OU}zbek"),             # Ўзбек
        @((Cyr 0x0493,0x0430,0x043B,0x0430,0x0431,0x0430),            "${G}alaba"),             # ғалаба
        @((Cyr 0x049B,0x0430,0x043B,0x0430,0x043C),                   "qalam"),                 # қалам
        @((Cyr 0x04B3,0x0430,0x0432,0x043E),                          "havo")                   # ҳаво
      )
      Run-CyrCases $uzWords $map $label
    }

    if ($label -eq "RU") {
      # Caps Lock typing capitals gives full capitals for the two-letter results.
      if (-not $capsWasOn) { Tap 0x14 $false }
      Run-CyrCases @(
        @((Cyr 0x044E,0x043B,0x0434,0x0443,0x0437),                   "YULDUZ"),                # юлдуз
        @((Cyr 0x0448,0x0430,0x0445,0x0430,0x0440),                   "${SU}AXAR")              # шахар
      ) $map $label
      if (-not $capsWasOn) { Tap 0x14 $false }

      # Punctuation is not converted: the '.' key on the Russian layout types a dot.
      ClearEdit; Tap 0x7C $false
      TypeCyr (Cyr 0x0430) $map
      Tap 0xBF $false
      Start-Sleep -Milliseconds 120
      $got = ReadEdit
      $ok = ($got -ceq "a.")
      if (-not $ok) { $fail++ }
      "{0,-6} {1} letter then the dot key        -> {2,-18} expected a." -f $(if ($ok) { 'ok' } else { 'FAIL' }), $label, (Show $got)
    }
  }
} finally {
  [void][K]::PostMessage($dash, 0x800A, [IntPtr]::Zero, $origLayout)
  if ((([K]::GetKeyState(0x14) -band 1) -ne 0) -ne $capsWasOn) { Tap 0x14 $false }
  Start-Sleep -Milliseconds 300
}

$proc | Stop-Process -Force
if ($fail -eq 0) { Write-Host "e2e: all passed" } else { Write-Host "e2e: $fail FAILED" }
exit $fail
