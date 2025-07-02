CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11
LDFLAGS = -static -lgdi32 -luser32 -limm32 -lshell32 -mwindows

TARGET = imei.exe
SOURCES = IMEIndicator.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
