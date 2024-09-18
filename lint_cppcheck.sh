#!/bin/sh -eu

cppcheck --enable=all --std=c99 --check-level=exhaustive \
		 --suppress=missingIncludeSystem --suppress=constParameterCallback --inline-suppr \
		 src/ test/
