SVGNATIVEDIR = ../../svgnative/svg-native-viewer/svgnative
SKIA_DIR = ../../svgnative/svg-native-viewer/third_party/skia
SKIA_INCLUDES  = -I$(SKIA_DIR) -I$(SKIA_DIR)/include -I$(SKIA_DIR)/include/core
INCLUDES = -I$(SVGNATIVEDIR)/ports/cairo/ -I$(SVGNATIVEDIR)/ports/skia/ -I$(SVGNATIVEDIR)/include $(shell pkg-config cairo librsvg-2.0 --cflags) -I../../tmp-sources/gdk-pixbuf/install_dir/include/gdk-pixbuf-2.0/ $(SKIA_INCLUDES)
LIBS := $(SVGNATIVEDIR)/build/linux/libSVGNativeViewerLib.a $(shell pkg-config cairo librsvg-2.0 --static --libs) -Wl,-rpath=$(SVGNATIVEDIR)/build/linux/ -ljpeg -lSDL2 $(SKIA_DIR)/out/Debug/libskia.a -ljpeg -lfreetype -ldl -lfontconfig -lpthread -lGL
LIBPATH=../../tmp-sources/gdk-pixbuf/install_dir/lib/x86_64-linux-gnu
all:
	g++ -g -ggdb -O0 main.cpp -o build/main  $(LIBPATH)/libgdk_pixbuf-2.0.so -Wl,-rpath=$(LIBPATH) $(LIBS) $(INCLUDES)
