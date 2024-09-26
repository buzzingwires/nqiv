#!/bin/sh -eu

clang-format -i --style=file src/*.c test/*.c src/*.h test/*.h
