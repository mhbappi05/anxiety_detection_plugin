@echo off
cd /d "E:\UIU\12th Tri\FYDP II\anxiety_detection_intervention_plugin\anxiety_detection_plugin"
echo Working dir: %CD%
echo.
echo === Compiling AnxietyPlugin.cpp ===
C:\msys64\mingw64\bin\g++.exe -O2 -g -Wall -std=c++17 -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D__MINGW64__ -D_WIN32_WINNT=0x0601 -I"C:\msys64\ucrt64\include\wx-3.2" -I"C:\msys64\ucrt64\include" -I"C:\msys64\mingw64\include" -c src\AnxietyPlugin.cpp -o AnxietyPlugin.o 2>errors_anxiety.txt
echo Exit code: %errorlevel%
type errors_anxiety.txt
echo.
echo === Compiling EventMonitor.cpp ===
C:\msys64\mingw64\bin\g++.exe -O2 -g -Wall -std=c++17 -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D__MINGW64__ -D_WIN32_WINNT=0x0601 -I"C:\msys64\ucrt64\include\wx-3.2" -I"C:\msys64\ucrt64\include" -I"C:\msys64\mingw64\include" -c src\EventMonitor.cpp -o EventMonitor.o 2>errors_event.txt
echo Exit code: %errorlevel%
type errors_event.txt
echo.
echo === Compiling PythonBridge.cpp ===
C:\msys64\mingw64\bin\g++.exe -O2 -g -Wall -std=c++17 -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D__MINGW64__ -D_WIN32_WINNT=0x0601 -I"C:\msys64\ucrt64\include\wx-3.2" -I"C:\msys64\ucrt64\include" -I"C:\msys64\mingw64\include" -c src\PythonBridge.cpp -o PythonBridge.o 2>errors_python.txt
echo Exit code: %errorlevel%
type errors_python.txt
echo.
echo === Compiling InterventionManager.cpp ===
C:\msys64\mingw64\bin\g++.exe -O2 -g -Wall -std=c++17 -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D__MINGW64__ -D_WIN32_WINNT=0x0601 -I"C:\msys64\ucrt64\include\wx-3.2" -I"C:\msys64\ucrt64\include" -I"C:\msys64\mingw64\include" -c src\InterventionManager.cpp -o InterventionManager.o 2>errors_intervention.txt
echo Exit code: %errorlevel%
type errors_intervention.txt
echo.
echo Done.
