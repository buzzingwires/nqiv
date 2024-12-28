nqiv Changelog
==============

Unreleased
----------

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
