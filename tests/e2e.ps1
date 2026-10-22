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

# Cyrillic profile on the Russian layout. The dashboard's keyboard layout is switched to Russian
# for this part and always switched back.
function Cyr([int[]]$codes) { return (($codes | ForEach-Object { [string][char]$_ }) -join '') }
$ruKeys = @{}
$ruKeys[(Cyr 0x0443)] = 0x45   # у  (E key)
$ruKeys[(Cyr 0x043A)] = 0x52   # к  (R key)
$ruKeys[(Cyr 0x0433)] = 0x55   # г  (U key)
$ruKeys[(Cyr 0x0445)] = 0xDB   # х  ([ key)
$ruKeys[(Cyr 0x0438)] = 0x42   # и  (B key)
$ruKeys[(Cyr 0x0437)] = 0x50   # з  (P key)
$ruKeys[(Cyr 0x0431)] = 0xBC   # б  (, key)
$ruKeys[(Cyr 0x0435)] = 0x54   # е  (T key)

function TypeRu([string]$text) {
  foreach ($c in $text.ToCharArray()) {
    if ($c -eq '|') { Start-Sleep -Milliseconds 600; continue }
    Tap ([byte]$ruKeys[[string][char]::ToLower($c)]) ([char]::IsUpper($c))
  }
  Start-Sleep -Milliseconds 120
}
function Show([string]$s) { return (($s.ToCharArray() | ForEach-Object { if ([int]$_ -lt 128) { [string]$_ } else { '\u{0:X4}' -f [int]$_ } }) -join '') }

$procId = [uint32]0
$tid = [K]::GetWindowThreadProcessId($dash, [ref]$procId)
$origLayout = [K]::GetKeyboardLayout($tid)
$ruLayout = [K]::LoadKeyboardLayout("00000419", 0)
try {
  [void][K]::PostMessage($dash, 0x800A, [IntPtr]::Zero, $ruLayout)   # test-only message: switch the dashboard's layout
  Start-Sleep -Milliseconds 500
  $langNow = [int]([K]::GetKeyboardLayout($tid).ToInt64() -band 0xFFFF)
  if ($langNow -ne 0x0419) {
    Write-Host ("FAIL   could not switch the dashboard to the Russian layout (now 0x{0:X4})" -f $langNow); $fail++
  } else {
    $cyrCases = @(
      @((Cyr 0x0443,0x0443),                 (Cyr 0x045E)),                       # уу   -> ў
      @((Cyr 0x0423,0x0423),                 (Cyr 0x040E)),                       # УУ   -> Ў
      @((Cyr 0x0443,0x0443,0x0443),          (Cyr 0x0443,0x0443)),                # ууу  -> уу
      @((Cyr 0x043A,0x043A),                 (Cyr 0x049B)),                       # кк   -> қ
      @((Cyr 0x041A,0x041A),                 (Cyr 0x049A)),                       # КК   -> Қ
      @((Cyr 0x0433,0x0433),                 (Cyr 0x0493)),                       # гг   -> ғ
      @((Cyr 0x0445,0x0445),                 (Cyr 0x04B3)),                       # хх   -> ҳ
      @((Cyr 0x0443,0x0443,0x0437,0x0431,0x0435,0x043A), (Cyr 0x045E,0x0437,0x0431,0x0435,0x043A)),  # уузбек -> ўзбек
      @((Cyr 0x0438,0x043A,0x043A,0x043A,0x0438),        (Cyr 0x0438,0x043A,0x043A,0x0438)),         # иккки -> икки
      @(((Cyr 0x0443) + '|' + (Cyr 0x0443)), (Cyr 0x0443,0x0443))                                        # у|у   -> уу
    )
    foreach ($case in $cyrCases) {
      ClearEdit
      Tap 0x7C $false
      TypeRu $case[0]
      $got = ReadEdit
      $ok = ($got -ceq $case[1])
      if (-not $ok) { $fail++ }
      "{0,-6} RU {1,-24} -> {2,-20} expected {3}" -f $(if ($ok) { 'ok' } else { 'FAIL' }), (Show $case[0]), (Show $got), (Show $case[1])
    }
  }
} finally {
  [void][K]::PostMessage($dash, 0x800A, [IntPtr]::Zero, $origLayout)
  Start-Sleep -Milliseconds 300
}

$proc | Stop-Process -Force
if ($fail -eq 0) { Write-Host "e2e: all passed" } else { Write-Host "e2e: $fail FAILED" }
exit $fail
