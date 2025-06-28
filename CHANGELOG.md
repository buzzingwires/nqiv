nqiv Changelog
==============

Unreleased
----------

* The default keybind to quit nqiv is now `shift+Q` instead of just `Q` to reduce the likelihood of it accidentally being pressed.

* nqivf.sh -p is now handled before -t, so time sorting still takes priority.

* nqivf.sh now follows symlinks

* nqivf.sh now supports very unusual filenames, including newlines.

* Command errors are now printed through the logger, keeping stdout as clear as possible, save for explicit command output and `set cmd acknowledgement`

* Fix issue with montage page length not changing once the end of the images is reached.

* Fix issue with montage selection outline not showing at tiny sizes.

* Don't keep zooming out if thumbnail is smaller than zoomed pixels.

* nqiv now handles color profiles.

* Optimize command parsing to be about 1.89 times faster.

* Fix some issues with animations not starting on frame one.

* Fix some issues with loading of very large images

* OpenMP is no longer part of the project so nqiv builds on platforms where it's not readily available (such as OpenBSD)

* `set preload image` can now be used to preload images around the currently viewed one, instead of just gallery thumbnails.

* Pruner description behavior much more consistent. Technically a breaking change, but normal use is unlikely to cause issues. They primarily prevent mixing unloading and checking for whether unloading should be done.

* Cleaned up redundant log messages.

* Add 'marked\_previous' and 'marked\_next' key actions to jump between marked images.

* Add -l and -q to nqiv.sh to get a list of retrieved files for later use, and to not actually start nqiv.

* More robust parsing of integers.

* `append thread ...` allows user-specified threads to handle specific types of events. This is especially handy for having threads always ready to react to loading images and animation frames without interruption from gallery loading.

* nqiv still accepts stdin input when started by nqivf.sh

* `set cmd acknowledge` will print messages informing of processed commands and when nqiv is reading from stdin.

* Much of nqiv's internal state can now be viewed through `help internal ...`

* dumpcfg should now require valid arguments (like the help commands).

* nqiv idle CPU usage should be reduced and improved responsiveness.

* nqiv should properly quit on cmd-related errors.

* nqiv should actually respond to sendkey commands now.

* nqiv can now read commands from stdin while it's running when `-s` is passed.

* `nqivf.sh` now version sorts its results as the default behavior. It should correctly sort numbers (1, 2, 10, etc' instead of 1, 10, 2), in addition to letters.

* Added a `clear_marked` key action. By default, it will be bound to `ctrl+shift+[` and will remove any marks from all loaded images.

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
