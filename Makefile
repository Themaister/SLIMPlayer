TARGET = slimplay

TARGET_OBJ := $(patsubst .cpp,.o,$(wildcard */*.cpp))

LIBS := -lrsound
FFMPEG_LIBS := -lavutil -lavformat -lavcodec

CXX := g++ -std=gnu++0x

all: print_obj $(TARGET)


print_obj:
	@echo OBJ: $(TARGET_OBJ)


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


$(TARGET): $(TARGET_OBJ)
	$(CXX) -o $@ $< $(FFMPEG_LIBS) $(LIBS)

clean:
	rm -f $(TARGET_OBJ)
	rm -f $(TARGET)

.PHONY: clean
