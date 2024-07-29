bin_PROGRAMS = nqiv
nqiv_CFLAGS = $(E_EXTRA_CFLAGS) `pkg-config vips --cflags --libs` -fopenmp -lSDL2 -L`jemalloc-config --libdir`  -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
nqiv_SOURCES = main.c md5.c array.c queue.c thumbnail.c montage.c logging.c image.c keybinds.c keyrate.c drawing.c worker.c platform.c cmd.c state.c pruner.c
