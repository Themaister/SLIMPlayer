TARGET = muplay

TARGET_SRC := $(wildcard */*.cpp)
TARGET_OBJ := $(TARGET_SRC:.cpp=.o)
HEADERS := $(wildcard */*.hpp)

LIBS := -lrsound -lglfw -lCg -lCgGL
FFMPEG_LIBS := -lavutil -lavformat -lavcodec
INCDIRS := -I. -Icore

CXX := g++ -std=gnu++0x -Wall -O0 -g

all: $(TARGET)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INCDIRS)


$(TARGET): $(TARGET_OBJ)
	$(CXX) -o $@ $(TARGET_OBJ) $(FFMPEG_LIBS) $(LIBS)

clean:
	rm -f $(TARGET_OBJ)
	rm -f $(TARGET)

.PHONY: clean
