#!/bin/bash
export PATH=/usr/bin:/mingw64/bin:$PATH
PROJ="/e/UIU/12th Tri/FYDP II/anxiety_detection_intervention_plugin/anxiety_detection_plugin"
FLAGS="-O2 -g -std=c++17 -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0601"
INC="-I/c/msys64/ucrt64/lib/wx/include/msw-unicode-3.2 -I/c/msys64/ucrt64/include/wx-3.2 -I/c/msys64/ucrt64/include -I/c/msys64/mingw64/include"

cd "$PROJ"

for f in AnxietyPlugin EventMonitor PythonBridge InterventionManager; do
    echo "=== Compiling ${f}.cpp ==="
    g++ $FLAGS $INC -c "src/${f}.cpp" -o "/tmp/${f}.o" 2>"/tmp/${f}_err.txt"
    RC=$?
    echo "Exit code: $RC"
    if [ -s "/tmp/${f}_err.txt" ]; then
        cat "/tmp/${f}_err.txt"
    else
        echo "(no errors)"
    fi
    echo
done
