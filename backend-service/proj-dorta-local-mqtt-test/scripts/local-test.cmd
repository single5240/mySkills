@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0local-test.ps1" %*
exit /b %errorlevel%
