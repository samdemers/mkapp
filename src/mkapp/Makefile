CC=gcc

PKG=gtk+-2.0 webkit-1.0 glib-2.0 gthread-2.0

LDFLAGS=`pkg-config --libs $(PKG)` \
	-L../libmkapp -lmkapp -O0 -g
CFLAGS=`pkg-config --cflags $(PKG)` \
	-I../libmkapp -Wall -pedantic -O0 -g -std=gnu99

OUT=mkglade mkmachine mkapp mkstore mkhtml machine2dot

BIN_DIR=/usr/local/bin

.PHONY: all clean install uninstall

all: $(OUT)

mkapp: mkapp.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

mkglade: mkglade.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

mkmachine: mkmachine.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

machine2dot: machine2dot.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

mkstore: mkstore.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

mkhtml: mkhtml.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

install: $(OUT)
	cp $(OUT) $(BIN_DIR)

uninstall:
	for file in $(OUT); do \
	  rm -f $(BIN_DIR)/$$file; \
	done

clean:
	rm -f $(OUT)
