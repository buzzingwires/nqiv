bin_PROGRAMS = nqiv
nqiv_CFLAGS = $(E_EXTRA_CFLAGS) -fopenmp `pkg-config vips --cflags` `sdl2-config --cflags`
nqiv_SOURCES = main.c md5.c array.c queue.c thumbnail.c montage.c logging.c image.c keybinds.c keyrate.c drawing.c worker.c platform.c cmd.c state.c pruner.c
LIBS = `pkg-config vips --libs` `sdl2-config --libs` -L`jemalloc-config --libdir`  -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
