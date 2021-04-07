SVGNATIVEDIR = ../../svgnative/svg-native-viewer/svgnative
INCLUDES = -I$(SVGNATIVEDIR)/ports/cairo/ -I$(SVGNATIVEDIR)/include $(shell pkg-config cairo --cflags) -I../../tmp-sources/gdk-pixbuf/install_dir/include/gdk-pixbuf-2.0/
LIBS := $(SVGNATIVEDIR)/build/linux/libSVGNativeViewerLib.a $(shell pkg-config cairo --static --libs) -Wl,-rpath=$(SVGNATIVEDIR)/build/linux/ -ljpeg -lSDL2
LIBPATH=../../tmp-sources/gdk-pixbuf/install_dir/lib/x86_64-linux-gnu
all:
	g++ -g -ggdb -O0 main.cpp -o build/main  $(LIBPATH)/libgdk_pixbuf-2.0.so -Wl,-rpath=$(LIBPATH) $(LIBS) $(INCLUDES)
