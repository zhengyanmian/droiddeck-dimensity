@echo off
rem Double-click entry for diagnose-phone.sh
rem Finds adb in PATH or common SDK locations, then runs the bash diagnostic.

setlocal
set "PROJ=%~dp0.."
set "ADB="

where adb >nul 2>&1 && set "ADB=adb"

if not defined ADB (
  if exist "%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe" set "ADB=%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"
)
if not defined ADB (
  if exist "C:\platform-tools\adb.exe" set "ADB=C:\platform-tools\adb.exe"
)

if not defined ADB (
  echo [ERROR] adb not found.
  echo Install Android platform-tools, or download from:
  echo   https://dl.google.com/android/repository/platform-tools-latest-windows.zip
  echo Unzip to C:\platform-tools and run this again.
  pause
  exit /b 1
)

echo Using adb: %ADB%
set "PATH=%PATH%;%__CD__%"

for /f "delims=" %%i in ('where bash 2^>nul') do (
  set "BASH=%%i"
  goto :run
)

echo [ERROR] bash (Git Bash) not found in PATH.
pause
exit /b 1

:run
"%BASH%" -lc "PATH='$(dirname \"%ADB%\")':$PATH bash '%PROJ%\tools\diagnose-phone.sh'"
pause
