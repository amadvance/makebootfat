bin_PROGRAMS = makebootfat

makebootfat_SOURCES = \
	makebootfat.c \
	disk.c \
	fat.c \
	error.c \
	part.c \
	getopt.c

noinst_HEADERS = \
	fat.h \
	disk.h \
	error.h \
	part.h \
	portable.h

man_MANS = doc/makebootfat.1

clean-local:
	rm -f makebootfat.exe
	rm -f mbrfat.lst

maintainer-clean-local:
	rm -f README AUTHORS HISTORY INSTALL doc/copying.txt
	rm -f doc/*.hh
	rm -f mbrfat.bin

EXTRA_DIST = \
	autogen.sh \
	noautomake.sh \
	configure.windows \
	README AUTHORS HISTORY INSTALL COPYING \
	doc/makebootfat.1 \
	 doc/makebootfat.d doc/history.d doc/readme.d doc/authors.d doc/install.d \
	doc/makebootfat.txt doc/history.txt doc/readme.txt doc/authors.txt doc/install.txt \
	doc/makebootfat.html doc/history.html doc/readme.html doc/authors.html doc/install.html \
	mbrfat.asm mbrfat.bin

mbrfat.bin: mbrfat.asm
	nasm -O99 -f bin -l mbrfat.lst -o mbrfat.bin mbrfat.asm

# Docs

%.1 : %.d
	advd2 man < $(srcdir)/$< > $@

%.txt : %.d
	advd2 txt < $(srcdir)/$< | todos > $@

%.html : %.d
	advd2 html < $(srcdir)/$< > $@

%.hh : %.d
	advd2 frame < $(srcdir)/$< > $@

# Archives

README: doc/readme.txt
	cat $< | fromdos > $@

AUTHORS: doc/authors.txt
	cat $< | fromdos > $@

INSTALL: doc/install.txt
	cat $< | fromdos > $@

HISTORY: doc/history.txt
	cat $< | fromdos > $@

doc/copying.txt: COPYING
	cat $< | todos > $@

DISTDOS_ROOT = \
	doc/readme.txt doc/authors.txt doc/history.txt doc/copying.txt \
	makebootfat.exe mbrfat.bin

DISTDOS_DOC = \
	doc/makebootfat.txt \
	doc/readme.txt doc/authors.txt doc/history.txt \
	doc/makebootfat.html \
	doc/readme.html doc/authors.html doc/history.html 

distwindows: $(DISTDOS_ROOT) $(DISTDOS_DOC)
	rm -f $(PACKAGE)-$(VERSION)-windows-pentium.zip
	mkdir tmp
	cp $(DISTDOS_ROOT) tmp
	mkdir tmp/doc
	cp $(DISTDOS_DOC) tmp/doc
	cd tmp && zip -r ../$(PACKAGE)-$(VERSION)-windows-pentium.zip *
	rm -r tmp

DISTWEB = \
	doc/makebootfat.hh

web: $(DISTWEB)


