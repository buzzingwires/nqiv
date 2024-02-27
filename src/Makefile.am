bin_PROGRAMS = nqiv
nqiv_CFLAGS = -std=c99 -Wall -Wpedantic -Wextra -Winline -Wcast-align -Werror `pkg-config --cflags --libs MagickCore` `pkg-config --cflags --libs MagickWand` -fopenmp -lSDL2
nqiv_SOURCES = main.c md5.c array.c queue.c thumbnail.c montage.c logging.c image.c keybinds.c keyrate.c drawing.c worker.c platform.c
