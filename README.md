nqiv
====

nqiv (**n**eo **q**uick **i**mage **v**iewer) is inspired by the lineage of `qiv` and intended to implement modern features such as multithreading and GPU rendering, as well as to reduce dependencies to a handful of mature and well-supported options.

Getting nqiv
------------

See the releases for builds. Linux builds are provided as AppImages, while Windows builds are provided as .zip archives containing an executable and needed directory.

Run nqiv with `-h` for basic help. nqiv is very portable and strives to not change anything on a system without being explicitly told to. Upon running nqiv from the command line, it will look for a config file in a default location, and will offer a command to create it if it isn't available. Given the graphical emphasis of Windows, `scripts/nqiv_deploy_cfg.bat` is also provided to do this same task with a single click.

Features
--------

* libvips supports fast and memory-efficient encoding and decoding of a wide range of images.

* Massive images are supported by means of partial loading for best fidelity, or by resizing the entire image to the maximum texture size (typically 16K by 16K) for best performance.

* nqiv runs on a variety of platforms, including GNU Linux, Linux with libmusl, and FreeBSD. A Windows build is also available and 32-bit builds have been tested.

* There is mouse and keyboard support through a configurable bind system.

* The FreeDesktop Thumbnail Managing standard is can be used to manipulate thumbnails that are shared with other software.

* nqiv is designed to work with other Unix-like apps, both by accepting instructions through stdin, and printing a list of selected images through stdout.

* Configurable dynamic unloading of images allows managing of nqiv's memory footprint while browsing a massive number of images.
