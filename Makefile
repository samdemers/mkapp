SUBDIRS=src doc
PREFIX=/usr/local

.PHONY: all install uninstall clean

all:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir; \
	done

doc/%: src/%
	$(MAKE) -C doc $*

src/%:
	$(MAKE) -C src $*

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
