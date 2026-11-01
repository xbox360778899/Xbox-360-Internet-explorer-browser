@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\allow-xbox-firewall.ps1 %*
if errorlevel 1 echo Run this file as Administrator to allow the Xbox through Windows Firewall.
pause
