SUBDIRS=libmkapp mkapp

.PHONY: all install uninstall clean

all:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir; \
	done

install: all
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir install; \
	done

uninstall:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir uninstall; \
	done

clean:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
