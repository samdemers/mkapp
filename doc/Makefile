MAN_DIR=/usr/local/man/man1
MAN=mkglade.1 mkmachine.1 mkapp.1 machine2dot.1 mkstore.1 mkhtml.1
DOC=html latex

mkglade_desc="run a Gtk+ GUI created with Glade"
mkmachine_desc="execute a state machine"
mkapp_desc="process management shell"
machine2dot_desc="convert an mkmachine file to GraphViz graphics"
mkstore_desc="manage the contents of a text file with assignation commands"
mkhtml_desc="run a graphical user interface made with web technologies"

.PHONY: all clean install uninstall man doxygen

all: man doxygen

install: $(MAN) 
	mkdir -p $(MAN_DIR)
	cp $(MAN) $(MAN_DIR)

uninstall:
	for file in $(MAN); do \
	  rm -f $(MAN_DIR)/$$file; \
	done

man: $(MAN)

doxygen:
	doxygen Doxyfile

%.1: ../src/mkapp/%
	LD_LIBRARY_PATH=../src/libmkapp \
	help2man -N -n $($*_desc) $< > $@

clean:
	rm -f  $(MAN)
	rm -rf $(DOC)
