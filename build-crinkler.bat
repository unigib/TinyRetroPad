@echo off
rem Build trpad.c to x86 object then compress/link with Crinkler
rem Run this from a Visual Studio Developer Command Prompt

setlocal enabledelayedexpansion

rem Find the x86 MSVC toolchain if VCINSTALLDIR is available.
set "X86_TOOLS="
set "MSVC_VER="
if defined VCINSTALLDIR (
  for /f "delims=" %%V in ('dir /b /ad "%VCINSTALLDIR%Tools\MSVC" 2^>nul') do set "MSVC_VER=%%V"
)
if defined MSVC_VER (
  set "X86_TOOLS=%VCINSTALLDIR%Tools\MSVC\%MSVC_VER%\bin\Hostx64\x86\"
)

if not defined X86_TOOLS (
  for /f "delims=" %%V in ('where cl 2^>nul ^| findstr /i "\\Hostx64\\x86\\cl.exe"') do (
    set "X86_TOOLS=%%~dpV"
    goto :have_x86_tools
  )
)
:have_x86_tools

if not defined X86_TOOLS (
  echo Unable to locate the x86 MSVC toolchain.
  echo Please run this from an x86 Visual Studio Developer Command Prompt or install the x86 toolchain.
  exit /b 1
)

if not exist "%X86_TOOLS%\cl.exe" (
  echo x86 cl.exe not found at "%X86_TOOLS%\cl.exe".
  exit /b 1
)
if not exist "%X86_TOOLS%\link.exe" (
  echo x86 link.exe not found at "%X86_TOOLS%\link.exe".
  exit /b 1
)

set "LINK_RES="

del trpad.obj 2>nul
del trpad.res 2>nul
del trpad-crinkled.exe 2>nul

if exist trpad.rc (
  where rc.exe >nul 2>nul
  if errorlevel 1 (
    echo rc.exe not found; building without icon resource.
  ) else (
    rc /fo trpad.res trpad.rc
    if errorlevel 1 exit /b %errorlevel%
    set "LINK_RES=trpad.res"
  )
)

rem "%X86_TOOLS%\cl.exe" /nologo /GS- /W4 /Zi /c trpad.c 
"%X86_TOOLS%\cl.exe" /c /O1 /Os /GS- /GR- trpad.c

if errorlevel 1 exit /b %errorlevel%

if exist .\crinkler.exe (
  echo Compressing with Crinkler...
  set "CRINKLER_INPUT=trpad.obj"
  if defined LINK_RES set "CRINKLER_INPUT=trpad.obj %LINK_RES%"
  rem .\crinkler.exe /ENTRY:WinMain /OUT:trpad-crinkled.exe /SUBSYSTEM:WINDOWS kernel32.lib user32.lib shell32.lib comdlg32.lib gdi32.lib trpad.obj
  .\crinkler.exe %CRINKLER_INPUT% ^
    /OUT:trpad-crinkled.exe ^
    /ENTRY:WinMain ^
    /SUBSYSTEM:WINDOWS ^
    /NOINITIALIZERS ^
    /TINYIMPORT ^
    /ORDERTRIES:2000 ^
    /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x86" ^
     kernel32.lib user32.lib shell32.lib comdlg32.lib gdi32.lib trpad.obj
) else (
  echo Crinkler not found; attempting x86 link with link.exe...
  if defined LINK_RES (
    "%X86_TOOLS%link.exe" /OUT:trpad-crinkled.exe trpad.obj %LINK_RES% kernel32.lib user32.lib shell32.lib comdlg32.lib gdi32.lib /SUBSYSTEM:WINDOWS /ENTRY:_WinMain@16
  ) else (
    "%X86_TOOLS%link.exe" /OUT:trpad-crinkled.exe trpad.obj kernel32.lib user32.lib shell32.lib comdlg32.lib gdi32.lib /SUBSYSTEM:WINDOWS /ENTRY:_WinMain@16
  )
)

endlocal
