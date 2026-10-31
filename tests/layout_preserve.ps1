# Checks that "YAY.exe /addlayout" (what the installer runs) leaves every other input method alone.
#
# It takes a snapshot of the user's input methods, runs the exe, takes a second snapshot and fails
# if anything that was in the first is missing from the second. Only additions are allowed.
# The snapshot reads the registry, where Windows keeps all input methods, including third-party
# ones (Google or Sogou Pinyin...) that Get-WinUserLanguageList may not report.
# The check only means something on a machine that has such an input method installed.
#
# It also checks that Windows loaded the Uzbek (Latin) layout into the running session. Without that
# the layout is in the language list but missing from the tray until the next sign-in.
#
# Warning: like the installer, this adds Uzbek (Latin) to your language list if it is missing.
#
# Usage: powershell -File tests\layout_preserve.ps1 [-Exe path\to\YAY.exe]   (default: YAY.exe from build.cmd)

param([string]$Exe = (Join-Path $PSScriptRoot "..\YAY.exe"))

Add-Type -MemberDefinition '[DllImport("user32.dll")] public static extern int GetKeyboardLayoutList(int n, IntPtr[] list);' -Name Kb -Namespace Yay

# The language ids (low 16 bits) of the layouts that are loaded in this session right now.
function Get-LoadedLayoutLanguages {
  $count = [Yay.Kb]::GetKeyboardLayoutList(0, $null)
  $list = New-Object IntPtr[] $count
  [void][Yay.Kb]::GetKeyboardLayoutList($count, $list)
  return @($list | ForEach-Object { $_.ToInt64() -band 0xFFFF })
}

# Every input method entry as one text line each, so that two snapshots can be compared as sets.
function Get-InputMethodSnapshot {
  $lines = New-Object System.Collections.Generic.HashSet[string]
  $roots = 'HKCU:\Control Panel\International\User Profile',
           'HKCU:\Software\Microsoft\CTF\Assemblies',
           'HKCU:\Software\Microsoft\CTF\TIP',
           'HKCU:\Keyboard Layout\Preload',
           'HKCU:\Keyboard Layout\Substitutes'
  foreach ($root in $roots) {
    if (-not (Test-Path $root)) { continue }
    foreach ($key in @(Get-Item $root) + @(Get-ChildItem $root -Recurse -ErrorAction SilentlyContinue)) {
      [void]$lines.Add("KEY $($key.Name)")
      foreach ($name in $key.GetValueNames()) {
        $value = $key.GetValue($name)
        if ($value -is [string[]]) {
          # A list value such as "Languages": one line per item, so that adding an item adds a line.
          foreach ($item in $value) { [void]$lines.Add("VAL $($key.Name) :: $name[] = $item") }
        } else {
          [void]$lines.Add("VAL $($key.Name) :: $name = $value")
        }
      }
    }
  }
  # Assign first: written directly after "in", foreach would see the whole list as one single item.
  $userLanguages = Get-WinUserLanguageList
  foreach ($lang in $userLanguages) {
    foreach ($tip in $lang.InputMethodTips) { [void]$lines.Add("TIP $($lang.LanguageTag) $tip") }
  }
  return $lines
}

if (-not (Test-Path $Exe)) { Write-Host "FAIL: $Exe not found. Run build.cmd first."; exit 1 }

$before = Get-InputMethodSnapshot
Write-Host "Before: $($before.Count) input method entries"

$proc = Start-Process -FilePath $Exe -ArgumentList '/addlayout' -Wait -PassThru
Start-Sleep -Milliseconds 800   # Windows finishes registering the layout a moment after the call returns

$after = Get-InputMethodSnapshot
Write-Host "After:  $($after.Count) input method entries, /addlayout exit code $($proc.ExitCode)"

$failed = $false
if ($proc.ExitCode -ne 0) { Write-Host "FAIL: /addlayout returned $($proc.ExitCode)"; $failed = $true }

$lost = @($before | Where-Object { -not $after.Contains($_) })
foreach ($line in $lost) { Write-Host "FAIL: lost  $line" }
if ($lost.Count -gt 0) { $failed = $true }

# The Uzbek (Latin) entry must be there now: language 0443 on the US keyboard.
if (-not ($after | Where-Object { $_ -match '^TIP uz\S* 0443:' })) {
  Write-Host "FAIL: the Uzbek (Latin) input method is not in the language list"
  $failed = $true
}

# ...and loaded in this session (language id 0x0443), which is what the tray shows.
if ((Get-LoadedLayoutLanguages) -notcontains 0x0443) {
  Write-Host "FAIL: Windows did not load the Uzbek (Latin) layout into the session (missing from the tray)"
  $failed = $true
}

if ($failed) { exit 1 }
Write-Host "ok    nothing lost, Uzbek (Latin) present and loaded"
exit 0
