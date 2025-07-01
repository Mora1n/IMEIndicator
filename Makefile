CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11
LDFLAGS = -lgdi32 -luser32 -limm32 -lshell32 -mwindows

TARGET = ime_indicator.exe
SOURCES = ime_indicator.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
