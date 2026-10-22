@echo off
setlocal
cd /d "%~dp0.."

rem Builds build\YAY_test.exe (accepts injected keys, English layout) and runs tests\e2e.ps1.
rem Run build.cmd once first so build\yay.res and build\nocrt.obj exist.

set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
for %%E in (Professional Enterprise BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\Tools\VsDevCmd.bat" (
    set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\Tools\VsDevCmd.bat"
  )
)
call "%VSDEVCMD%" -arch=x86 -host_arch=x64 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1

if not exist build\yay.res ( echo Run build.cmd first. & exit /b 1 )
if not exist build\test mkdir build\test

cl /nologo /W3 /O1 /Os /GL /GS- /Gy /Gw /EHs-c- /GR- /utf-8 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 /DNDEBUG /DYAY_ACCEPT_INJECTED /Fobuild\test\ /Febuild\YAY_test.exe src\main.cpp src\hook.cpp src\engine.cpp src\inject.cpp src\keymap.cpp src\layout.cpp src\settings.cpp src\tray.cpp src\translit.cpp src\pill.cpp src\dashboard.cpp build\nocrt.obj build\yay.res /link /nologo /LTCG /OPT:REF /OPT:ICF /MANIFEST:NO /SUBSYSTEM:WINDOWS,6.01 /INCREMENTAL:NO /NODEFAULTLIB /ENTRY:YayStartup kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib
if errorlevel 1 exit /b 1

powershell -NoProfile -ExecutionPolicy Bypass -File tests\e2e.ps1
exit /b %errorlevel%
