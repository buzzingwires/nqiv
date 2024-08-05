#!/bin/sh -efu

G_MXE_URL="https://github.com/mxe/mxe.git"
G_VIPS_PACKAGE="libvips.zip"
G_VIPS_MANIFEST="libvips"
G_VIPS_DIR="libvips"
G_JEMALLOC_PACKAGE="jemalloc.tar.bz2"
G_JEMALLOC_DIR="jemalloc"

D_MXE_TARGET="x86_64-w64-mingw32.shared"
D_MXE_PACKAGES="cc sdl2 gettext"
D_MXE_DIR="./cross/mxe"
D_PACKAGE_DIR="./cross/packages"
D_VIPS_URL="https://github.com/libvips/build-win64-mxe/releases/download/v8.15.2/vips-dev-w64-web-8.15.2.zip"
D_JEMALLOC_URL="https://github.com/jemalloc/jemalloc/releases/download/5.3.0/jemalloc-5.3.0.tar.bz2"

errcho()
{
	>&2 echo "$@";
}

tracked_delete()
{
	local L_LIST="$1"

	if [ -e "$L_LIST" ]
	then
		xargs --exit --max-args=1 --no-run-if-empty rm -v < "$L_LIST"
		rm -v "$L_LIST"
	fi
}

filter_stdin_dirs()
{
	while IFS= read -r line
	do
		if [ ! -d "$line" ]
		then
			echo "$line"
		fi
	done
}

tracked_copy()
{
	local L_LIST="$1"
	local L_TO_COPY="$2"
	local L_DESTINATION="$3"

	cp -rvn "$L_TO_COPY" "$L_DESTINATION" | sed --quiet --posix --sandbox -E -e "s/'.+' -> '(.+)'/\1/p" | filter_stdin_dirs > "$L_LIST"
}

exit_usage()
{
	local L_CODE="$1"

	errcho "Usage: $0 [option]... [action]..."
	errcho
	errcho "Options:"
	errcho
	errcho "'-h' : Print this message, then quit."
	errcho
	errcho "'-t' : MXE_TARGETS variable- determines MinGW type. Default: $D_MXE_TARGET"
	errcho
	errcho "'-p' : Packages to install with MXE. Default: $D_MXE_PACKAGES"
	errcho
	errcho "'-d' : Install MXE here. Paths will be made as necessary. Default: $D_MXE_DIR"
	errcho
	errcho "'-D' : Manage packages here. Default: $D_PACKAGE_DIR"
	errcho
	errcho "'-v' : Download VIPS from here. Try checking https://github.com/libvips/build-win64-mxe/releases for choices. Default: $D_VIPS_URL"
	errcho
	errcho "'-j' : Download jemalloc from here. Try checking https://github.com/jemalloc/jemalloc/releases for choices. Default: $D_JEMALLOC_URL"
	errcho
	errcho "Actions:"
	errcho
	errcho "'cleanmxe' : Delete the ENTIRE MXE directory and all of its contents."
	errcho
	errcho "'loadmxe' : Download MXE and set up its environment."
	errcho
	errcho "'cleanvips' : Delete libvips packages and installed files."
	errcho
	errcho "'loadvips' : 'cleanvips', then download libvips and install it in the MXE environment."
	errcho
	errcho "'cleanjemalloc' : Uninstall jemalloc, then delete its packages."
	errcho
	errcho "'loadjemalloc' : 'cleanjemalloc', then download jemalloc, build it, and install it in the MXE environment."
	errcho
	errcho "'genbuilder' : Generate a build script with appropriate environment variables and so on. Write it to stdout."

	exit "$L_CODE"
}

a_mxe_target="$D_MXE_TARGET"
a_mxe_packages="$D_MXE_PACKAGES"
a_mxe_dir="$D_MXE_DIR"
a_package_dir="$D_PACKAGE_DIR"
a_vips_url="$D_VIPS_URL"
a_jemalloc_url="$D_JEMALLOC_URL"

action_cleanmxe()
{
	rm -vrf "$a_mxe_dir"
}

action_loadmxe()
{
	mkdir -vp "$a_mxe_dir"
	git clone "$G_MXE_URL" "$a_mxe_dir" || true
	cd "$a_mxe_dir"
	git pull
	make $a_mxe_packages MXE_TARGETS=$a_mxe_target
	cd -
}

action_setmxeenv()
{
	export PKG_CONFIG_PATH="$a_mxe_dir/usr/$a_mxe_target/lib/pkgconfig/"
	export PATH="$a_mxe_dir/usr/bin:$PATH"
}

action_cleanvips()
{
	tracked_delete "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-bin.log"
	tracked_delete "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-share.log"
	tracked_delete "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-etc.log"
	tracked_delete "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-lib.log"
	tracked_delete "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-include.log"
	rm -vrf "${a_package_dir:?}/$G_VIPS_DIR"
	rm -vf "${a_package_dir:?}/$G_VIPS_PACKAGE"
}

get_package()
{
	L_URL="$1"
	L_LINKNAME="$2"
	L_FULLNAME="$a_package_dir/$(basename "$L_URL")"
	if [ ! -e "$L_FULLNAME" ]
	then
		curl -L "$L_URL" -o "$L_FULLNAME"
	fi
	ln -sfv "$L_FULLNAME" "$L_LINKNAME"
}

action_loadvips()
{
	action_cleanvips
	mkdir -vp "$a_package_dir/$G_VIPS_DIR"
	get_package "$a_vips_url" "$a_package_dir/$G_VIPS_PACKAGE"
	unzip "$a_package_dir/$G_VIPS_PACKAGE" -d "$a_package_dir/$G_VIPS_DIR"
	local L_VIPS_DIR=
	L_VIPS_DIR="$(find "$a_package_dir/$G_VIPS_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
	tracked_copy "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-bin.log" "$L_VIPS_DIR/bin" "$a_mxe_dir/usr/$a_mxe_target"
	tracked_copy "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-share.log" "$L_VIPS_DIR/share" "$a_mxe_dir/usr/$a_mxe_target"
	tracked_copy "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-etc.log" "$L_VIPS_DIR/etc" "$a_mxe_dir/usr/$a_mxe_target"
	tracked_copy "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-lib.log" "$L_VIPS_DIR/lib" "$a_mxe_dir/usr/$a_mxe_target"
	tracked_copy "$a_package_dir/$a_mxe_target-$G_VIPS_MANIFEST-include.log" "$L_VIPS_DIR/include" "$a_mxe_dir/usr/$a_mxe_target"
}

action_cleanjemalloc()
{
	if [ -e "$a_package_dir/$G_JEMALLOC_DIR" ]
	then
		action_setmxeenv
		cd "$a_package_dir/$G_JEMALLOC_DIR"
		make uninstall || true
		cd -
	fi
	rm -vrf "${a_package_dir:?}/$G_JEMALLOC_DIR"
	rm -vf "${a_package_dir:?}/$G_JEMALLOC_PACKAGE"
}

action_loadjemalloc()
{
	action_setmxeenv
	action_cleanjemalloc
	mkdir -vp "$a_package_dir/$G_JEMALLOC_DIR"
	get_package "$a_jemalloc_url" "$a_package_dir/$G_JEMALLOC_PACKAGE"
	tar jxvf "$a_package_dir/$G_JEMALLOC_PACKAGE" --strip-components=1 -C"$a_package_dir/$G_JEMALLOC_DIR"
	cd "$a_package_dir/$G_JEMALLOC_DIR"
	./configure --prefix="$a_mxe_dir/usr/$a_mxe_target" --host="$a_mxe_target" --enable-static --enable-shared
	make
	make install
	cd -
}

action_genbuilder()
{
	echo "export PKG_CONFIG_PATH=\"$a_mxe_dir/usr/$a_mxe_target/lib/pkgconfig/\""
	echo "export PATH=\"$a_mxe_dir/usr/bin:\$PATH\""
	echo "autoreconf --install"
	echo "./configure --prefix=\"$a_mxe_dir/usr/$a_mxe_target\" --host=\"$a_mxe_target\" --enable-cross-compile=yes"
}

main()
{
	set -efu
	while getopts "ht:p:d:D:v:j:" opt;
	do
		case "$opt" in
		'h')
			exit_usage 0
			;;
		't')
			a_mxe_target="$OPTARG"
			;;
		'p')
			a_mxe_packages="$OPTARG"
			;;
		'd')
			a_mxe_dir="$OPTARG"
			;;
		'D')
			a_package_dir="$OPTARG"
			;;
		'v')
			a_vips_url="$OPTARG"
			;;
		'j')
			a_jemalloc_url="$OPTARG"
			;;
		*)
			errcho "Error: Unrecognized flag '$opt'."
			exit_usage 1
			;;
		esac
	done
	shift $((OPTIND - 1))

	mkdir -vp "$a_package_dir"
	mkdir -vp "$a_mxe_dir"
	a_package_dir="$(realpath "$a_package_dir")"
	a_mxe_dir="$(realpath "$a_mxe_dir")"

	local L_VIPS_DIR=
	for action in "$@"
	do
		case "$action" in
		'cleanmxe')
			action_cleanmxe
			;;
		'loadmxe')
			action_loadmxe
			;;
		'cleanvips')
			action_cleanvips
			;;
		'loadvips')
			action_loadvips
			;;
		'cleanjemalloc')
			action_cleanjemalloc
			;;
		'loadjemalloc')
			action_loadjemalloc
			;;
		'genbuilder')
			action_genbuilder
			;;
		*)
			errcho "Error: Unrecognized action '$action'."
			exit_usage 1
			;;
		esac
	done
}

main "$@"
