bin_PROGRAMS = nqiv
nqiv_CFLAGS = $(E_EXTRA_CFLAGS) `pkg-config vips --cflags --define-prefix` `pkg-config sdl2 --cflags --define-prefix` 
nqiv_SOURCES = main.c md5.c array.c queue.c thumbnail.c montage.c logging.c image.c keybinds.c keyrate.c drawing.c worker.c platform.c cmd.c state.c pruner.c 
LIBS = `pkg-config vips --libs --define-prefix` `pkg-config sdl2 --libs  --define-prefix` $(E_JEMALLOC_LIBS) 
