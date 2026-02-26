@echo off
echo Building Anxiety Detection Plugin...

REM Change to project root (one level up from scripts/)
cd /d "%~dp0\.."
echo Project root: %CD%

REM Use MSYS2 bash to run the reliable build script (avoids quoting issues with paths)
C:\msys64\usr\bin\bash.exe scripts\build_full.sh
if %errorlevel% neq 0 goto error

echo.
echo SUCCESS! Created anxiety_plugin.dll
goto end

:error
echo.
echo ERROR: Build failed! Check error messages above.
exit /b 1

:end