#!/bin/sh -eu

clang-tidy src/*.c test/*.c src/*.h test/*.h      \
		   --                                     \
		   $(pkg-config glib-2.0 --cflags-only-I) \
		   -DVERSION=\"CLANGTIDY\"                \
