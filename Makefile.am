L_CFLAGS_STANDARDS=-std=c99 -Wall -Wpedantic -Wextra -Winline -Wcast-align -Werror 
L_CFLAGS_DEBUG_OPTS=-O0 -g 
L_CFLAGS_RELEASE_OPTS=-DNDEBUG -O2 -flto=auto -s 

default-optimization:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} "
all:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_RELEASE_OPTS} "
debug:
	$(MAKE) -C src E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_DEBUG_OPTS} "
tests:
	$(MAKE) -C test E_EXTRA_CFLAGS="${L_CFLAGS_STANDARDS} ${L_CFLAGS_DEBUG_OPTS} "
all-debug: debug tests
