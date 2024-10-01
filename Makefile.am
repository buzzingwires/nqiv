L_CFLAGS_STANDARDS=-std=c99 -Wall -Wpedantic -Wextra -Winline -Wcast-align -Werror -D_REENTRANT -fopenmp 
L_CFLAGS_DEBUG_OPTS=-O0 -g 
L_CFLAGS_RELEASE_OPTS=-DNDEBUG -O3 -flto=auto 

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
