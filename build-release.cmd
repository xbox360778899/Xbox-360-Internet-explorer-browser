@echo off
setlocal
set "XEDK=C:\Program Files (x86)\Microsoft Xbox 360 SDK"
"%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" "%~dp0X360Browser.vcxproj" /t:Rebuild /p:Configuration=Release /p:Platform="Xbox 360" /v:minimal /nologo
if errorlevel 1 exit /b 1
copy /y "%~dp0Release\X360Browser.xex" "%~dp0Release\default.xex" >nul
if not exist "%~dp0Release\default.xex" exit /b 2
echo Built Release\default.xex
