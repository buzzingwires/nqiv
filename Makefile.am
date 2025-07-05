L_CFLAGS_STANDARDS=-std=c99 -Wall -Wpedantic -Wextra -Winline -Werror -D_REENTRANT 
L_CFLAGS_DEBUG_OPTS=-O0 -g 
L_CFLAGS_RELEASE_OPTS=-DNDEBUG -O3 

L_JEMALLOC_LIBS= 

if DEFINEPREFIX
L_EXTRA_PKG_CONFIG_OPTS=--define-prefix 
else
L_EXTRA_PKG_CONFIG_OPTS= 
endif

if USECOV
L_CFLAGS_STANDARDS+=--coverage 
endif

if ISCROSS
L_CFLAGS_STANDARDS+=-mconsole 
endif

if USEJEMALLOC
L_JEMALLOC_LIBS+=`pkg-config jemalloc --libs ${L_EXTRA_PKG_CONFIG_OPTS}` 
L_CFLAGS_STANDARDS+=`pkg-config jemalloc --cflags ${L_EXTRA_PKG_CONFIG_OPTS}`
endif

default-optimization:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} " E_EXTRA_PKG_CONFIG_OPTS="${L_EXTRA_PKG_CONFIG_OPTS} "
all:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_RELEASE_OPTS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} " LDFLAGS="-Wl,-s" E_EXTRA_PKG_CONFIG_OPTS="${L_EXTRA_PKG_CONFIG_OPTS} "
debug:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_DEBUG_OPTS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} " E_EXTRA_PKG_CONFIG_OPTS="${L_EXTRA_PKG_CONFIG_OPTS} "
tests:
	$(MAKE) -C test E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_DEBUG_OPTS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} " E_EXTRA_PKG_CONFIG_OPTS="${L_EXTRA_PKG_CONFIG_OPTS} "
all-debug: debug tests

install-exec-local:
	mkdir -p "$(DESTDIR)$(bindir)"
	(test -e src/nqiv && cp src/nqiv "$(DESTDIR)$(bindir)") \
	|| (test -e src/nqiv.exe && cp src/nqiv.exe "$(DESTDIR)$(bindir)") \
	|| true
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/16x16/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/22x22/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/128x128/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/256x256/apps/"
	mkdir -p "$(DESTDIR)$(datadir)/applications/"
	cp media/icons/16x16/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/16x16/apps/nqiv.png"
	cp media/icons/22x22/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/22x22/apps/nqiv.png"
	cp media/icons/24x24/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/24x24/apps/nqiv.png"
	cp media/icons/32x32/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/32x32/apps/nqiv.png"
	cp media/icons/48x48/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/48x48/apps/nqiv.png"
	cp media/icons/128x128/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/128x128/apps/nqiv.png"
	cp media/icons/256x256/nqiv.png "$(DESTDIR)$(datadir)/icons/hicolor/256x256/apps/nqiv.png"
	cp scripts/nqiv.desktop "$(DESTDIR)$(datadir)/applications/"
