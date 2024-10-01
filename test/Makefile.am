AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = tester
tester_CFLAGS = $(E_EXTRA_CFLAGS) `pkg-config vips --cflags $(E_EXTRA_PKG_CONFIG_OPTS)` `pkg-config sdl2 --cflags $(E_EXTRA_PKG_CONFIG_OPTS)` 
tester_SOURCES = tester.c logging_tests.c array_tests.c pruner_tests.c keybind_tests.c queue_tests.c keyrate_tests.c ../src/logging.c ../src/array.c ../src/pruner.c ../src/queue.c ../src/cmd.c ../src/keybinds.c ../src/keyrate.c ../src/state.c ../src/image.c ../src/thumbnail.c ../src/platform.c ../src/drawing.c ../src/montage.c
LIBS = `pkg-config vips --libs $(E_EXTRA_PKG_CONFIG_OPTS)` `pkg-config sdl2 --libs  $(E_EXTRA_PKG_CONFIG_OPTS)` 
