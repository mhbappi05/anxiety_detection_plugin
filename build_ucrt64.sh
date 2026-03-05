#!/bin/bash
# Build script for anxiety plugin — run from MSYS2 ucrt64 shell
cd "$(dirname "$0")"

CXX=g++
FLAGS="-O2 -Wall -std=c++17 -D__WXMSW__ -DwxUSE_UNICODE=1 -D_UNICODE -DUNICODE -DHAVE_W32API_H -Isdk -I/ucrt64/include/wx-3.2 -I/ucrt64/lib/wx/include/msw-unicode-3.2 -fPIC"
WXLIB="-L/ucrt64/lib -lwx_mswu_core-3.2 -lwx_baseu-3.2 -lwx_baseu_xml-3.2 -Wl,--out-implib,libanxiety_plugin.a"

SRCS="src/AnxietyPlugin.cpp src/EventMonitor.cpp src/PythonBridge.cpp src/InterventionManager.cpp"
OBJS=""
for src in $SRCS; do
    obj="${src%.cpp}.o"
    echo "Compiling $src..."
    $CXX $FLAGS -c "$src" -o "$obj"
    if [ $? -ne 0 ]; then
        echo "FAILED: $src"
        exit 1
    fi
    OBJS="$OBJS $obj"
done

echo "Linking anxiety_plugin.dll..."
$CXX -shared -o anxiety_plugin.dll $OBJS $WXLIB
if [ $? -ne 0 ]; then
    echo "LINK FAILED"
    exit 1
fi

echo
echo "=== SUCCESS: anxiety_plugin.dll ==="
objdump -p anxiety_plugin.dll | grep "Member-Name" | grep -E "CreatePlugin|FreePlugin|GetPlugin"
