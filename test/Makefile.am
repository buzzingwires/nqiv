AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = tester
tester_CFLAGS = -std=c99 -Wall -Wpedantic -Wextra -Winline -Wcast-align -Werror -fopenmp -O0 -g
tester_SOURCES = tester.c logging_tests.c array_tests.c ../src/logging.c ../src/array.c
