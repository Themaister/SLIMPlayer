TARGET = slimplay

TARGET_OBJ := $(patsubst $(wildcard **/*.cpp),.cpp,.o)
LIBS := -lrsound
FFMPEG_LIBS := -lavutil -lavformat -lavcodec

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


$(TARGET): $(TARGET_OBJ)
	$(CXX) -o $@ $< $(FFMPEG_LIBS) $(LIBS)

clean:
	rm -f $(TARGET_OBJ)
	rm -f $(TARGET)

.PHONY: clean
