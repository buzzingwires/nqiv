Developing nqiv
===============

* [Building](#building)
  - [Dependencies](#dependencies)
  - [Make Targets](#make-targets)
  - [Platform Notes](#platform-notes)
    + [FreeBSD](#freebsd)
    + [Windows (Cross Compiling)](#windows--cross-compiling-)
    + [Linux (AppImage)](#linux--appimage-)
* [Contributing](#contributing)
* [Helper Scripts](#helper-scripts)
* [Codebase Overview](#codebase-overview)
* [Branches](#branches)

Building
--------

In short, a release version of nqiv can be built by the typical steps of:

```sh
autoreconf --install
./configure
make
make install
```

It is recommended to also check `./configure --help`. Note that `CFLAGS` will be
appended after any automatic ones.

### Dependencies ###

* [SDL2](https://www.libsdl.org/)
* [libvips](https://www.libvips.org/)
* [jemalloc](https://jemalloc.net/) (Not strictly required, but highly recommended for personal use, and required for binary releases.)
* [optparse](https://github.com/skeeto/optparse/tree/master) (Bundled into the project with minimal edits)
* [GLib](https://docs.gtk.org/glib/) (Should be included by libvips but worth
  mentioning, since it's also used for some utility functions.)

### Make Targets ###

* `default-optimization`: Minimalist build option with no specified optimizations, and assertions left on, leaving full freedom to specify whatever through `CFLAGS`.
* `all`: Default target. Make a release build of nqiv. Optimizations on, assertions off, and executable stripped.
* `debug`: Make a debug build of nqiv. Debugging symbols and assertions on. No optimizations.
* `tests`: Build tests. Run them with test/tester.
* `all-debug`: Do the debug and tests targets.

### Platform Notes ###

#### FreeBSD ####

FreeBSD will require `./configure --enable-no-jemalloc` because it already uses jemalloc as its default allocator.

#### Windows (Cross-Compiling) ####

A full Windows package may be made through the following workflow.

```sh
# Setup MXE and needed dependencies, then create scripts to configure and to set environment for
# make (or other tasks.) This will build an appropriate GCC, so it will take a while and likely
# use a couple GB of space.
./scripts/setup_cross_env.sh load-toolchain make-environment make-configure

# Generate and run configure.
./make_configure.sh

# Specify make with default target.
echo "make" >> ./env.sh

# Build nqiv.
./env.sh

# Make the zip package.
./scripts/setup_cross_env.sh make-package
```

#### Linux (AppImage) ###

A few changes to the standard process are needed to make the AppImage. Requires `./configure --prefix=/usr` and for `./scripts/make_appimage.sh` to then be run after building.

Contributing
------------

It is recommended to understand the tools and processes described in this file before contributing to nqiv. When making a pull request, please be prepared to explain what your contribution does, the rationale for making it, and a high-level overview of how it is implemented.

`make tests` or `make all-debug` will build `test/tester`. Please run it and strongly consider writing tests for your changes, especially if they are easily testable. See `test/tester.c` for instructions for adding tests.

Run `scripts/update_default_cfg.sh` and check whether `default.cfg` has been unintentionally changed by then running `git diff`

It is recommended to run the linters `scripts/lint_cppcheck.sh` and `scripts/lint_clang-tidy.sh` (in that order of importance).

Also consider `scripts/show_icon.sh` and `scripts/show_logo.sh` for basic functionality tests.

Further, consider running nqiv with valgrind.

Before you are finished, please use `scripts/format_code.sh` to format your code. You may use `/* clang-format off */` and `/* clang-format on */` if there is a section you feel just doesn't work well with the auto-formatting.

Feel free to add yourself to `AUTHORS.md`

Helper Scripts
--------------

These scripts are specifically written against the dash shell. They try to be portable and POSIX compliant, with the exception that the `local` keyword is used extensively.

* `format_code.sh`: Use clang-format to format code according to the standard for nqiv.

* `lint_cppcheck.sh`: Lint nqiv code using cppcheck. Quite slow but recommended. This linter is less zealous than clang-tidy, so warnings should be avoided.

* `lint_clang-tidy.sh`: Lint nqiv code using clang-tidy. Consider this more of a barometer for potential issues with your code, rather than something absolute.

* `make_appimage.sh`: Grab linuxdeploy and package nqiv as an AppImage. Make sure nqiv is built with the `--prefix=/usr` configured.

* `setup_cross_env.sh`: Script containing tools for building the Windows version of nqiv. Bootstrap MXE, prepare Windows dependencies, set up an environment, and package the build.

* `show_icon.sh`: nqiv's icon is in fact a screenshot. Take a shot of the window opened by this.

* `show_logo.sh`: nqiv's logo is also a screenshot. Take a shot of the window opened by this.

* `update_default_cfg.sh`: nqiv's config format is tested against itself between updates. Use `git diff` to see the new config file generated by nqiv with the old config file as input, and make sure any changes to it are intentional.

Codebase Overview
-----------------

nqiv lives in `src/` and is coded against the C99 standard.

* `optparse.h`: Only slightly modified optparse implementation from <https://github.com/skeeto/optparse/tree/master>

* `queue.h`: Thread safe queue and priority queue implementation. Backed by array.h

* `logging.h`: Thread safe logging faculty supporting different levels and output streams.

* `worker.h`: Worker threads where tasks may be offloaded from the master.

* `drawing.h`: Drawing primitives for rendering UI elements.

* `thumbnail.h`: Freedesktop Thumbnail Managing Standard implementation.

* `typedefs.h`: Just some forward declares and other circularly shared things, when necessary. This file shouldn't include any other nqiv file and should be included before the circularly referenced elements are needed.

* `array.h`: Essential data structure functioning in various roles, including array, stack, and string builder. It is very important to get this right, and ideally performant, too.

* `cmd.h`: Config/cmd file implementation.

* `event.h`: Events dispatched to worker threads.

* `image.h`: Loading, storage, and manipulation of image data.

* `keybinds.h`: Bind keyboard and mouse gestures to actions.

* `keyrate.h`: Control how key presses are registered, including key up, key down, and repeat rates.

* `montage.h`: Manage the layout of thumbnails in montage view.

* `state.h`: Central state object and related data. Other objects compose this.

* `pruner.h`: Control conditional unloading of images to preserve memory.

* `platform.h`: Platform specific code. This header should be included first. Most compile-time checks should also be performed here. Conditional compilation should remain within this header and platform.c.

Branches
--------

* `master` contains a stable version of nqiv and should be almost always be updated for a release.

* `dev` is code that's actively in development. Contributions go here before being merged into `master`

* Other branches may be opened for special projects such as major features, extensive rewrites, etc'.
