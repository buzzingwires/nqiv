AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = tester
tester_CFLAGS = $(E_EXTRA_CFLAGS) -fopenmp `pkg-config vips --cflags` `sdl2-config --cflags`
tester_SOURCES = tester.c logging_tests.c array_tests.c pruner_tests.c ../src/logging.c ../src/array.c ../src/pruner.c ../src/queue.c ../src/cmd.c ../src/keybinds.c ../src/state.c ../src/image.c ../src/drawing.c
LIBS = `pkg-config vips --libs` `sdl2-config --libs`
