@echo off
echo Building Anxiety Detection Plugin (Full Debug Mode)...
echo.

REM Set compiler path
set CXX=C:\msys64\mingw64\bin\g++.exe

REM Include paths
set INCLUDE=-I"C:\msys64\ucrt64\include\wx-3.2" -I"C:\msys64\ucrt64\include" -I"C:\msys64\mingw64\include"

REM Defines
set DEFINES=-DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D__MINGW64__ -D_WIN32_WINNT=0x0601

REM Compiler flags
set CXXFLAGS=-O2 -g -Wall -std=c++17 %DEFINES% %INCLUDE%

echo Compiling AnxietyPlugin.cpp...
echo Command: %CXX% %CXXFLAGS% -c ../src/AnxietyPlugin.cpp -o AnxietyPlugin.o
echo.
%CXX% %CXXFLAGS% -c ../src/AnxietyPlugin.cpp -o AnxietyPlugin.o 2> build_error.txt
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Failed to compile AnxietyPlugin.cpp
    echo Error level: %errorlevel%
    echo.
    echo ===== COMPILER ERROR OUTPUT =====
    type build_error.txt
    echo =================================
    goto error
)

echo Compiling EventMonitor.cpp...
%CXX% %CXXFLAGS% -c ../src/EventMonitor.cpp -o EventMonitor.o 2>> build_error.txt
if %errorlevel% neq 0 goto error

echo Compiling PythonBridge.cpp...
%CXX% %CXXFLAGS% -c ../src/PythonBridge.cpp -o PythonBridge.o 2>> build_error.txt
if %errorlevel% neq 0 goto error

echo Compiling InterventionManager.cpp...
%CXX% %CXXFLAGS% -c ../src/InterventionManager.cpp -o InterventionManager.o 2>> build_error.txt
if %errorlevel% neq 0 goto error

echo Linking...
%CXX% -shared -o anxiety_plugin.dll AnxietyPlugin.o EventMonitor.o PythonBridge.o InterventionManager.o %LDFLAGS% 2>> build_error.txt
if %errorlevel% neq 0 goto error

echo Cleaning up...
del *.o
del build_error.txt 2>nul

echo.
echo SUCCESS! Created anxiety_plugin.dll
dir anxiety_plugin.dll
goto end

:error
echo.
echo BUILD FAILED!
echo Check build_error.txt for details
type build_error.txt
exit /b 1

:end