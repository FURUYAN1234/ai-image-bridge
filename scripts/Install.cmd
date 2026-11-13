@echo off
powershell.exe -NoLogo -NoProfile -File "%~dp0Install.ps1"
if errorlevel 1 (
  echo Operation failed. See the message above.
  pause
  exit /b 1
)
pause
