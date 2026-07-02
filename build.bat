@echo off
rem Build the C version of TinyRetroPad using the Visual Studio Developer Command Prompt
cl /nologo /W4 /Zi trpad.c /link /DEBUG user32.lib kernel32.lib comdlg32.lib shell32.lib gdi32.lib
