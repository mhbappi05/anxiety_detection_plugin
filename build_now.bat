@echo off
setlocal
cd /d "E:\UIU\12th Tri\FYDP II\anxiety_detection_intervention_plugin\anxiety_detection_plugin"
set CXX=C:\msys64\ucrt64\bin\g++.exe
set WXINC=-IC:\msys64\ucrt64\include\wx-3.2 -IC:\msys64\ucrt64\lib\wx\include\msw-unicode-3.2
set DEFS=-D__WXMSW__ -DwxUSE_UNICODE=1 -D_UNICODE -DUNICODE -DHAVE_W32API_H
set FLAGS=-O2 -Wall -std=c++17 %DEFS% -Isdk %WXINC% -fPIC
set WXLIB=-LC:\msys64\ucrt64\lib -lwx_mswu_core-3.2 -lwx_baseu-3.2 -lwx_baseu_xml-3.2 -Wl,--out-implib,libanxiety_plugin.a

echo === Compiling AnxietyPlugin.cpp ===
%CXX% %FLAGS% -c src\AnxietyPlugin.cpp -o src\AnxietyPlugin.o 2>build_err_ap.txt
echo Exit code: %ERRORLEVEL%
type build_err_ap.txt
if errorlevel 1 goto err

echo === Compiling EventMonitor.cpp ===
%CXX% %FLAGS% -c src\EventMonitor.cpp -o src\EventMonitor.o
if errorlevel 1 goto err

echo === Compiling PythonBridge.cpp ===
%CXX% %FLAGS% -c src\PythonBridge.cpp -o src\PythonBridge.o
if errorlevel 1 goto err

echo === Compiling InterventionManager.cpp ===
%CXX% %FLAGS% -c src\InterventionManager.cpp -o src\InterventionManager.o
if errorlevel 1 goto err

echo === Linking anxiety_plugin.dll ===
%CXX% -shared -o anxiety_plugin.dll src\AnxietyPlugin.o src\EventMonitor.o src\PythonBridge.o src\InterventionManager.o %WXLIB%
if errorlevel 1 goto err

echo === SUCCESS ===
goto end
:err
echo === BUILD FAILED ===
exit /b 1
:end
