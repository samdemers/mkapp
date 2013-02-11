CC=gcc

PKG=gtk+-2.0 glib-2.0

LDFLAGS=`pkg-config --libs $(PKG)` \
	-shared -O0 -g
CFLAGS=`pkg-config --cflags $(PKG)` \
	-fPIC -Wall -pedantic -O0 -g -std=gnu99

OBJ=parser.o mkapp_parser.o mkmachine_parser.o store_key_value.o \
    gobject_info.o gobject_command.o mkapp_commands.o \
    transition.o module.o store_node.o

OUT=libmkapp.so
HEADERS=*.h

PREFIX=/usr/local

.PHONY: all clean install uninstall

all: $(OUT)

libmkapp.so: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

install: $(OUT)
	cp $(OUT) $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/libmkapp
	cp $(HEADERS) $(PREFIX)/include/libmkapp

uninstall:
	rm -rf $(PREFIX)/include/libmkapp
	rm -f  $(PREFIX)/lib/$(OUT)

clean:
	rm -f $(OUT) $(OBJ)
