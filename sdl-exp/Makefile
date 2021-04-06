SVGNATIVEDIR = ../../svgnative/svg-native-viewer/svgnative
INCLUDES = -I$(SVGNATIVEDIR)/ports/cairo/ -I$(SVGNATIVEDIR)/include $(shell pkg-config cairo --cflags)
LIBS := $(SVGNATIVEDIR)/build/linux/libSVGNativeViewerLib.a $(shell pkg-config cairo --static --libs) -Wl,-rpath=$(SVGNATIVEDIR)/build/linux/ -ljpeg -lSDL2
all:
	g++ -g -ggdb -O0 main.cpp -o build/main $(LIBS) $(INCLUDES)
