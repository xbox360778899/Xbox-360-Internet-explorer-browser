@echo off
cd /d "%~dp0"
python scripts\configure.py
if errorlevel 1 exit /b 1
python bridge\server.py --browser chrome
pause
