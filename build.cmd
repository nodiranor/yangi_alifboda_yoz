@echo off
setlocal
cd /d "%~dp0"

rem Builds YAY.exe (32-bit, linked without the C runtime) and the installer.
rem Needs Visual Studio 2022 with the C++ desktop workload. Inno Setup 6 is optional.

set "VSDEVCMD="
for %%E in (Community Professional Enterprise BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\Tools\VsDevCmd.bat" (
    set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\Tools\VsDevCmd.bat"
  )
)
if "%VSDEVCMD%"=="" (
  echo Visual Studio 2022 not found.
  exit /b 1
)
call "%VSDEVCMD%" -arch=x86 -host_arch=x64 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1

if not exist build mkdir build

set CFLAGS=/nologo /W3 /O1 /Os /GL /GS- /Gy /Gw /EHs-c- /GR- /utf-8 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 /DNDEBUG /Fobuild\
set LFLAGS=/nologo /LTCG /OPT:REF /OPT:ICF /MANIFEST:NO /SUBSYSTEM:WINDOWS,6.01 /INCREMENTAL:NO /NODEFAULTLIB /ENTRY:YayStartup
set LIBS=kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib

echo [1/4] Tests
cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /DUNICODE /D_UNICODE /Fobuild\ /Febuild\test_engine.exe tests\test_engine.cpp src\engine.cpp
if errorlevel 1 exit /b 1
build\test_engine.exe
if errorlevel 1 exit /b 1
cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /DUNICODE /D_UNICODE /Fobuild\ /Febuild\test_keymap.exe tests\test_keymap.cpp src\keymap.cpp user32.lib
if errorlevel 1 exit /b 1
build\test_keymap.exe
if errorlevel 1 exit /b 1
cl /nologo /W3 /O1 /MT /EHs-c- /utf-8 /Fobuild\ /Febuild\test_translit.exe tests\test_translit.cpp src\translit.cpp
if errorlevel 1 exit /b 1
build\test_translit.exe
if errorlevel 1 exit /b 1

echo [2/4] Resources
pushd res
rc /nologo /fo ..\build\yay.res yay.rc
if errorlevel 1 ( popd & exit /b 1 )
popd

echo [3/4] YAY.exe
rem A running copy locks the exe and would make the link step fail.
taskkill /F /IM YAY.exe >nul 2>&1
rem memset/memcpy helpers must be built without /GL (whole-program optimisation)
cl /nologo /c /O1 /Os /GS- /Gy /utf-8 /DUNICODE /D_UNICODE /Fobuild\ src\nocrt.cpp
if errorlevel 1 exit /b 1
cl %CFLAGS% /FeYAY.exe src\main.cpp src\hook.cpp src\engine.cpp src\inject.cpp src\keymap.cpp src\layout.cpp src\settings.cpp src\tray.cpp src\translit.cpp src\pill.cpp src\dashboard.cpp build\nocrt.obj build\yay.res /link %LFLAGS% %LIBS%
if errorlevel 1 exit /b 1
for %%F in (YAY.exe) do echo YAY.exe: %%~zF bytes

echo [4/4] Installer
set "ISCC="
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if "%ISCC%"=="" (
  echo Inno Setup 6 not found, installer skipped.
) else (
  "%ISCC%" /Q YAY_Setup.iss
  if errorlevel 1 exit /b 1
  for %%F in (Output\YAY_Setup.exe) do echo Output\YAY_Setup.exe: %%~zF bytes
)
echo Done.
