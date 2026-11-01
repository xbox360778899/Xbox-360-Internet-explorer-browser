@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
if not exist obj mkdir obj
cl /nologo /EHsc /TP /I src /I vendor\orbita tests\host_checks.cpp vendor\orbita\xui_gfx.c vendor\orbita\xui_text.c /Foobj\ /Feobj\host_checks.exe
if errorlevel 1 exit /b 1
obj\host_checks.exe
