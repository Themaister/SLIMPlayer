TARGET = slimplay

TARGET_SRC := $(wildcard */*.cpp)
TARGET_OBJ := $(TARGET_SRC:.cpp=.o)

LIBS := -lrsound
FFMPEG_LIBS := -lavutil -lavformat -lavcodec
INCDIRS := -I. -Icore

CXX := g++ -std=gnu++0x -Wall

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INCDIRS)


$(TARGET): $(TARGET_OBJ)
	$(CXX) -o $@ $(TARGET_OBJ) $(FFMPEG_LIBS) $(LIBS)

clean:
	rm -f $(TARGET_OBJ)
	rm -f $(TARGET)

.PHONY: clean