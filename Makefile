TARGET = slimplayer
PREFIX = /usr/local
DESTDIR =

TARGET_SRC := $(wildcard */*.cpp)
TARGET_OBJ := $(TARGET_SRC:.cpp=.o)
HEADERS := $(wildcard */*.hpp)

LIBS := -lasound -lglfw -lCg -lCgGL -lass
FFMPEG_LIBS := -lavutil -lavformat -lavcodec
INCDIRS := -I. -Icore

CXX := g++ -std=gnu++0x -Wall -O3 -g

all: $(TARGET)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INCDIRS)


$(TARGET): $(TARGET_OBJ)
	$(CXX) -o $@ $(TARGET_OBJ) $(FFMPEG_LIBS) $(LIBS)

clean:
	rm -f $(TARGET_OBJ)
	rm -f $(TARGET)

install: all
	install -m755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: clean install uninstall
