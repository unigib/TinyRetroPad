@echo off
rem Build the C version of TinyRetroPad using the Visual Studio Developer Command Prompt
del trpad.exe 2>nul
del trpad.obj 2>nul
del trpad.res 2>nul
set "RES_FILE="

if exist trpad.rc (
  where rc.exe >nul 2>nul
  if errorlevel 1 (
    echo rc.exe not found; building without icon resource.
  ) else (
    rc /fo trpad.res trpad.rc
    if errorlevel 1 exit /b %errorlevel%
    set "RES_FILE=trpad.res"
  )
)

if defined RES_FILE (
  cl /nologo /W4 /Zi /GS- trpad.c %RES_FILE% /link /DEBUG user32.lib kernel32.lib comdlg32.lib shell32.lib gdi32.lib
) else (
  cl /nologo /W4 /Zi /GS- trpad.c /link /DEBUG user32.lib kernel32.lib comdlg32.lib shell32.lib gdi32.lib
)
