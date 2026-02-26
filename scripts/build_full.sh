#!/bin/bash
export PATH=/usr/bin:/mingw64/bin:$PATH
PROJ="/e/UIU/12th Tri/FYDP II/anxiety_detection_intervention_plugin/anxiety_detection_plugin"
FLAGS="-O2 -g -std=c++17 -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0601"
INC="-I/c/msys64/ucrt64/lib/wx/include/msw-unicode-3.2 -I/c/msys64/ucrt64/include/wx-3.2 -I/c/msys64/ucrt64/include -I/c/msys64/mingw64/include"
LDFLAGS="-L/c/msys64/ucrt64/lib -L/c/msys64/mingw64/lib -lwx_mswu_core-3.2 -lwx_baseu-3.2 -lwx_mswu_adv-3.2 -lwx_mswu_html-3.2 -lwx_mswu_qa-3.2 -lwx_mswu_xrc-3.2 -lwx_baseu_net-3.2 -lwx_baseu_xml-3.2"

cd "$PROJ"

echo "=== Compiling source files ==="
for f in AnxietyPlugin EventMonitor PythonBridge InterventionManager; do
    echo "Compiling ${f}.cpp..."
    g++ $FLAGS $INC -c "src/${f}.cpp" -o "/tmp/${f}.o" 2>/tmp/${f}_err.txt
    RC=$?
    if [ $RC -ne 0 ]; then
        echo "ERROR compiling ${f}.cpp (exit $RC):"
        cat "/tmp/${f}_err.txt"
        exit 1
    fi
done

echo "=== Linking DLL ==="
g++ -shared -o "anxiety_plugin.dll" \
    /tmp/AnxietyPlugin.o /tmp/EventMonitor.o /tmp/PythonBridge.o /tmp/InterventionManager.o \
    $LDFLAGS 2>/tmp/link_err.txt
RC=$?
echo "Link exit code: $RC"
if [ -s /tmp/link_err.txt ]; then
    echo "Link errors:"
    cat /tmp/link_err.txt
fi

if [ $RC -eq 0 ]; then
    echo "=== SUCCESS: anxiety_plugin.dll created ==="
    ls -la anxiety_plugin.dll
else
    echo "=== LINK FAILED ==="
    exit 1
fi
