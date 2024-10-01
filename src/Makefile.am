bin_PROGRAMS = nqiv
nqiv_CFLAGS = $(E_EXTRA_CFLAGS) `pkg-config sdl2 --cflags $(E_EXTRA_PKG_CONFIG_OPTS)` `pkg-config vips --cflags $(E_EXTRA_PKG_CONFIG_OPTS)` 
nqiv_SOURCES = main.c array.c queue.c thumbnail.c montage.c logging.c image.c keybinds.c keyrate.c drawing.c worker.c platform.c cmd.c state.c pruner.c 
LIBS = $(E_JEMALLOC_LIBS) `pkg-config sdl2 --libs $(E_EXTRA_PKG_CONFIG_OPTS)` `pkg-config vips --libs $(E_EXTRA_PKG_CONFIG_OPTS)` 
