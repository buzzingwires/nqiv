AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = tester
tester_CFLAGS = $(E_EXTRA_CFLAGS) `pkg-config vips --cflags --define-prefix` `pkg-config sdl2 --cflags --define-prefix` 
tester_SOURCES = tester.c logging_tests.c array_tests.c pruner_tests.c keybind_tests.c ../src/logging.c ../src/array.c ../src/pruner.c ../src/queue.c ../src/cmd.c ../src/keybinds.c ../src/state.c ../src/image.c ../src/drawing.c  ../src/montage.c
LIBS = `pkg-config vips --libs --define-prefix` `pkg-config sdl2 --libs  --define-prefix` 
