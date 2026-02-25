# Makefile for Anxiety Detection Plugin
CXX = C:/msys64/mingw64/bin/g++.exe
CXXFLAGS = -O2 -g -Wall -DHAVE_W32API_H -D__WXMSW__ -DWXUSINGDLL -D_UNICODE -DUNICODE \
           -I"C:/Program Files/CodeBlocks/src/include" \
           -I"C:/Program Files/CodeBlocks/src/sdk" \
           -I"C:/msys64/ucrt64/include/wx-3.2" \
           -I"C:/msys64/ucrt64/include" \
           -I"C:/msys64/mingw64/include" \
           -std=c++17

LDFLAGS = -L"C:/msys64/ucrt64/lib" \
          -L"C:/msys64/mingw64/lib" \
          -lwx_mswu_core-3.2 -lwx_baseu-3.2 -lwx_mswu_adv-3.2 \
          -lwx_mswu_html-3.2 -lwx_mswu_qa-3.2 -lwx_mswu_xrc-3.2 \
          -lwx_baseu_net-3.2 -lwx_baseu_xml-3.2

SOURCES = src/AnxietyPlugin.cpp src/EventMonitor.cpp src/PythonBridge.cpp src/InterventionManager.cpp
HEADERS = src/AnxietyPlugin.h src/EventMonitor.h src/PythonBridge.h src/InterventionManager.h
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = anxiety_plugin.dll

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install:
	cp $(TARGET) "C:/Program Files/CodeBlocks/share/CodeBlocks/plugins/"
	mkdir -p "C:/Program Files/CodeBlocks/share/CodeBlocks/python"
	cp python/*.py "C:/Program Files/CodeBlocks/share/CodeBlocks/python/"
	mkdir -p "C:/Program Files/CodeBlocks/share/CodeBlocks/anxiety_models"
	cp models/*.pkl "C:/Program Files/CodeBlocks/share/CodeBlocks/anxiety_models/"

.PHONY: all clean install