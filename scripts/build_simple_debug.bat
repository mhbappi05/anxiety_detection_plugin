@echo off
echo Building Anxiety Detection Plugin (Debug Mode)...
echo.

REM Set compiler path
set CXX=C:\msys64\mingw64\bin\g++.exe

REM Include paths
set INCLUDE=-I"C:\msys64\ucrt64\include\wx-3.2" -I"C:\msys64\ucrt64\include" -I"C:\msys64\mingw64\include"

REM Defines
set DEFINES=-DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D__MINGW64__ -D_WIN32_WINNT=0x0601

REM Compiler flags
set CXXFLAGS=-O2 -g -Wall -std=c++17 %DEFINES% %INCLUDE%

REM Linker flags
set LDFLAGS=-L"C:\msys64\ucrt64\lib" -L"C:\msys64\mingw64\lib" -lwx_mswu_core-3.2 -lwx_baseu-3.2 -lwx_mswu_adv-3.2 -lwx_mswu_html-3.2 -lwx_mswu_qa-3.2 -lwx_mswu_xrc-3.2 -lwx_baseu_net-3.2 -lwx_baseu_xml-3.2

echo Compiling AnxietyPlugin.cpp...
echo Command: %CXX% %CXXFLAGS% -c ../src/AnxietyPlugin.cpp -o AnxietyPlugin.o
echo.
%CXX% %CXXFLAGS% -c ../src/AnxietyPlugin.cpp -o AnxietyPlugin.o
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Failed to compile AnxietyPlugin.cpp
    echo Error level: %errorlevel%
    goto error
)

echo Compiling EventMonitor.cpp...
%CXX% %CXXFLAGS% -c ../src/EventMonitor.cpp -o EventMonitor.o
if %errorlevel% neq 0 goto error

echo Compiling PythonBridge.cpp...
%CXX% %CXXFLAGS% -c ../src/PythonBridge.cpp -o PythonBridge.o
if %errorlevel% neq 0 goto error

echo Compiling InterventionManager.cpp...
%CXX% %CXXFLAGS% -c ../src/InterventionManager.cpp -o InterventionManager.o
if %errorlevel% neq 0 goto error

echo Linking...
%CXX% -shared -o anxiety_plugin.dll AnxietyPlugin.o EventMonitor.o PythonBridge.o InterventionManager.o %LDFLAGS%
if %errorlevel% neq 0 goto error

echo Cleaning up...
del *.o

echo.
echo SUCCESS! Created anxiety_plugin.dll
dir anxiety_plugin.dll
goto end

:error
echo.
echo BUILD FAILED!
echo Check the error messages above.
exit /b 1

:end