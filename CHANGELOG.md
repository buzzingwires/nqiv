nqiv Changelog
==============

Unreleased
----------

### BREAKING CHANGES

* The `-s` command line option is removed completely and replaced by `set cmd from_stdin true`

* Pruner description behavior much more consistent. Technically a breaking change, but normal use is unlikely to cause issues. They primarily prevent mixing unloading and checking for whether unloading should be done.

* Command errors are now printed through the logger, keeping stdout as clear as possible, save for explicit command output and `set cmd acknowledgement`.

## New Config Options

* `set preload image` can now be used to preload images around the currently viewed one, instead of just gallery thumbnails.

* `append thread ...` allows user-specified threads to handle specific types of events. This is especially handy for having threads always ready to react to loading images and animation frames without interruption from gallery loading.

* `set cmd acknowledge` will print messages informing of processed commands and when nqiv is reading from stdin.

* Much of nqiv's internal state can now be viewed through `help internal ...`

### Keybinds

* Added a `clear_marked` key action. By default, it will be bound to `ctrl+shift+[` and will remove any marks from all loaded images.

* Add `marked_previous` and `marked_next` key actions respectively bound to `shift+,` and `shift+.` to jump between marked images.

* The default keybind to quit nqiv is now `shift+Q` instead of just `Q` to reduce the likelihood of it accidentally being pressed.

### Fixes

* Fix some issues with animations not starting on frame one.

* Fix some issues with loading of very large images

* nqiv should properly quit on cmd-related errors.

* nqiv should actually respond to sendkey commands now.

* dumpcfg should now require valid arguments (like the help commands).

* Fix issue with montage page length not changing once the end of the images is reached.

* Fix issue with montage selection outline not showing at tiny sizes.

* Don't keep zooming out if thumbnail is smaller than zoomed pixels.

* More robust parsing of integers.

### Improvements

* nqiv now handles color profiles.

* Optimize command parsing to be about 1.89 times faster.

* There are now a variety of icon sizes and a more flexible script to generate them. They are also copied by `make install`

* OpenMP is no longer part of the project so nqiv builds on platforms where it's not readily available (such as OpenBSD)

* nqiv idle CPU usage should be reduced and responsiveness improved.

* Cleaned up redundant log messages.

### nqivf.sh

* `nqivf.sh` now version sorts its results as the default behavior. It should correctly sort numbers (1, 2, 10, etc' instead of 1, 10, 2), in addition to letters.

* `-p` is now handled before `-t`, so time sorting still takes priority over path sorting.

* nqivf.sh now follows symlinks

* nqivf.sh now supports very unusual filenames, including newlines.

* nqivf.sh no longer specifies that nqiv will accept commands from stdin by default. Will default to config file/user-set options such as `set cmd from_stdin`

* Add `-l` and `-q` to nqiv.sh to get a list of retrieved files for later use, and to not actually start nqiv.

1.1.0-beta
----------

* Reduce clang-tidy complaining about intended behavior.
* Add code of conduct.
* Add GitHub-related templates and other meta-info.
* Make 'LOADING' display in title.
* Change montage behavior to scroll by row instead of page
* Reduce default prune delay to one-tenth.
* Add nqivf.sh script to scan directories for sorted files for nqiv
* More up to date and descriptive descriptions for some thread settings.
* Fix montage dimension bugs.
* Don't try to do mouse-ops on non-existent images.
* Fix some erroneous log messages.
* Refactor marking actions to be less repetitious.
* Rename DEVELOPING.md to CONTRIBUTING.md
* Copy licenses with binaries in Windows package.

1.0.0-beta
----------

* First public release.
