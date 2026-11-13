@echo off
powershell.exe -NoLogo -NoProfile -File "%~dp0Uninstall.ps1"
if errorlevel 1 (
  echo Operation failed. See the message above.
  pause
  exit /b 1
)
pause
