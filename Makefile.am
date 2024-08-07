L_CFLAGS_STANDARDS=-std=c99 -Wall -Wpedantic -Wextra -Winline -Wcast-align -Werror -D_REENTRANT -fopenmp 
L_CFLAGS_DEBUG_OPTS=-O0 -g 
L_CFLAGS_RELEASE_OPTS=-DNDEBUG -O2 -flto=auto 

if ISCROSS
L_JEMALLOC_LIBS=`pkg-config jemalloc --libs --define-prefix` 
L_CFLAGS_STANDARDS+=-mconsole 
else
L_JEMALLOC_LIBS=-L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs` 
endif

default-optimization:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} "
all:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_RELEASE_OPTS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} " LDFLAGS="-Wl,-s"
debug:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_DEBUG_OPTS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} "
tests:
	$(MAKE) -C test E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_DEBUG_OPTS} " E_JEMALLOC_LIBS="${L_JEMALLOC_LIBS} "
all-debug: debug tests
