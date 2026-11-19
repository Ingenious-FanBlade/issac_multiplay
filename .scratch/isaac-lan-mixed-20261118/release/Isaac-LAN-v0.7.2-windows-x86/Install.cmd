@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" -Mode Install
set "installer_exit=%errorlevel%"
pause
exit /b %installer_exit%
