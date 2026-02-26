# Makefile for Anxiety Detection Plugin — CB 25.03 compatible (ucrt64 dynamic wx)
#
# Uses ucrt64 g++ so CRT matches CB 25.03.
# Wx DLLs + transitive deps from ucrt64/bin must be present in CB's dir.
# Run scripts/copy_wx_dlls.ps1 (as Admin) to deploy them.

CXX = C:/msys64/ucrt64/bin/g++

WX_INC_BASE  = C:/msys64/ucrt64/include/wx-3.2
WX_INC_SETUP = C:/msys64/ucrt64/lib/wx/include/msw-unicode-3.2

CXXFLAGS = -O2 -Wall -std=c++17 \
           -D__WXMSW__ -DwxUSE_UNICODE=1 -D_UNICODE -DUNICODE -DHAVE_W32API_H \
           -I"$(WX_INC_BASE)" \
           -I"$(WX_INC_SETUP)"

LDFLAGS = -shared \
          -L"$(WX_LIB_DIR)" \
          -lwx_mswu_core-3.2 \
          -lwx_baseu-3.2 \
          -lwx_baseu_xml-3.2 \
          -Wl,--out-implib,libanxiety_plugin.a

WX_LIB_DIR = C:/msys64/ucrt64/lib

SOURCES = src/AnxietyPlugin.cpp src/EventMonitor.cpp src/PythonBridge.cpp src/InterventionManager.cpp
HEADERS = src/AnxietyPlugin.h src/EventMonitor.h src/PythonBridge.h src/InterventionManager.h
OBJECTS = $(SOURCES:.cpp=.o)
TARGET  = anxiety_plugin.dll

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "=== DLL runtime dependencies ==="
	@objdump -p $@ | grep "DLL Name"
	@echo "==================================="

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) libanxiety_plugin.a

.PHONY: all clean