#!/bin/sh -eu

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
D_ENV_PATH="./env.sh"
D_CONFIGURE_MAKER_PATH="./make_configure.sh"
D_EXE_PATH="./src/nqiv.exe"
D_RELEASE_PATH="."
D_RELEASE_VERSION="1.1.0-beta"

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
	errcho "'-e' : Path to environment setup file created by 'make-environment'. Default: $D_ENV_PATH"
	errcho
	errcho "'-c' : Path to environment setup file created by 'make-configure'. Default: $D_CONFIGURE_MAKER_PATH"
	errcho
	errcho "'-E' : Path to compiled executable. Default: $D_EXE_PATH"
	errcho
	errcho "'-R' : Path to write the release zip file to. Default: $D_RELEASE_PATH"
	errcho
	errcho "'-r' : Version of nqiv to label release with. Default: $D_RELEASE_VERSION"
	errcho
	errcho "Actions:"
	errcho
	errcho "'clean-mxe' : Delete the ENTIRE MXE directory and all of its contents."
	errcho
	errcho "'load-mxe' : Download MXE and set up its environment."
	errcho
	errcho "'clean-vips' : Delete libvips packages and installed files."
	errcho
	errcho "'load-vips' : 'clean-vips', then download libvips and install it in the MXE environment."
	errcho
	errcho "'clean-jemalloc' : Uninstall jemalloc, then delete its packages."
	errcho
	errcho "'load-jemalloc' : 'clean-jemalloc', then download jemalloc, build it, and install it in the MXE environment."
	errcho
	errcho "'clean-toolchain' : Alias for clean-mxe. Deletes the ENTIRE MXE directory and all of its contents."
	errcho
	errcho "'load-toolchain' : Alias for load-mxe, load-vips, and load-jemalloc"
	errcho
	errcho "'make-environment' : Generate a script that can be sourced to set the appropriate environment variables but take no further option. Write it to ${D_ENV_PATH}"
	errcho
	errcho "'make-configure' : Generate a script to set up the build environment with appropriate environment variables and so on. Write it to ${D_CONFIGURE_MAKER_PATH}"
	errcho
	errcho "'clean-package-tmp' : Delete temporary files associated with 'make-package'. Write it to: ${D_RELEASE_PATH}/nqiv-${D_MXE_TARGET}-${D_RELEASE_VERSION}"
	errcho
	errcho "'clean-package' : Run 'clean-package-tmp', then delete the zip file containing nqiv.exe and its dependencies from ${D_RELEASE_PATH}/nqiv-${D_MXE_TARGET}-${D_RELEASE_VERSION}[.zip]"
	errcho
	errcho "'make-package' : Generate the zip file containing nqiv.exe and its dependencies. Write it to: ${D_RELEASE_PATH}/nqiv-${D_MXE_TARGET}-${D_RELEASE_VERSION}.zip"

	exit "$L_CODE"
}

a_mxe_target="$D_MXE_TARGET"
a_mxe_packages="$D_MXE_PACKAGES"
a_mxe_dir="$D_MXE_DIR"
a_package_dir="$D_PACKAGE_DIR"
a_vips_url="$D_VIPS_URL"
a_jemalloc_url="$D_JEMALLOC_URL"
a_env_path="$D_ENV_PATH"
a_configure_maker_path="$D_CONFIGURE_MAKER_PATH"
a_exe_path="$D_EXE_PATH"
a_release_path="$D_RELEASE_PATH"
a_release_version="$D_RELEASE_VERSION"

action_clean_mxe()
{
	rm -vrf "$a_mxe_dir"
}

action_load_mxe()
{
	mkdir -vp "$a_mxe_dir"
	git clone "$G_MXE_URL" "$a_mxe_dir" || true
	cd "$a_mxe_dir"
	git pull
	make $a_mxe_packages MXE_TARGETS=$a_mxe_target
	cd -
}

action_set_mxe_env()
{
	export PKG_CONFIG_PATH="$a_mxe_dir/usr/$a_mxe_target/lib/pkgconfig/"
	export PATH="$a_mxe_dir/usr/bin:$PATH"
}

action_clean_vips()
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

action_load_vips()
{
	action_clean_vips
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

action_clean_jemalloc()
{
	if [ -e "$a_package_dir/$G_JEMALLOC_DIR" ]
	then
		action_set_mxe_env
		cd "$a_package_dir/$G_JEMALLOC_DIR"
		make uninstall || true
		cd -
	fi
	rm -vrf "${a_package_dir:?}/$G_JEMALLOC_DIR"
	rm -vf "${a_package_dir:?}/$G_JEMALLOC_PACKAGE"
}

action_load_jemalloc()
{
	action_set_mxe_env
	action_clean_jemalloc
	mkdir -vp "$a_package_dir/$G_JEMALLOC_DIR"
	get_package "$a_jemalloc_url" "$a_package_dir/$G_JEMALLOC_PACKAGE"
	tar jxvf "$a_package_dir/$G_JEMALLOC_PACKAGE" --strip-components=1 -C"$a_package_dir/$G_JEMALLOC_DIR"
	cd "$a_package_dir/$G_JEMALLOC_DIR"
	./configure --prefix="$a_mxe_dir/usr/$a_mxe_target" --host="$a_mxe_target" --enable-static --enable-shared
	make
	make install
	cd -
}

action_clean_toolchain()
{
	action_clean_mxe
}

action_load_toolchain()
{
	action_load_mxe
	action_load_vips
	action_load_jemalloc
}

print_environment_file()
{
	echo "export PKG_CONFIG_PATH=\"$a_mxe_dir/usr/$a_mxe_target/lib/pkgconfig/\""
	echo "export PATH=\"$a_mxe_dir/usr/bin:\$PATH\""
}

action_make_environment()
{
	print_environment_file > "$a_env_path"
	chmod +x "$a_env_path"
}

action_make_configure()
{
	print_environment_file > "$a_configure_maker_path"
	echo "autoreconf --install" >> "$a_configure_maker_path"
	echo "./configure --prefix=\"$a_mxe_dir/usr/$a_mxe_target\" --host=\"$a_mxe_target\" --enable-cross-compile=yes --enable-define-prefix=yes" >> "$a_configure_maker_path"
	chmod +x "$a_configure_maker_path"
}

action_clean_package_tmp()
{
	local L_RELEASE_BASE="$a_release_path/nqiv-$a_mxe_target-$a_release_version"

	rm -rvf "$L_RELEASE_BASE"
}

action_clean_package()
{
	action_clean_package_tmp

	local L_RELEASE_BASE="$a_release_path/nqiv-$a_mxe_target-$a_release_version"

	rm -vf  "$L_RELEASE_BASE.zip"
}

action_make_package()
{
	action_clean_package

	local L_LIBPATH="$a_mxe_dir/usr/$a_mxe_target/bin"
	local L_RELEASE_BASE="$a_release_path/nqiv-$a_mxe_target-$a_release_version"

	mkdir -v "$L_RELEASE_BASE"

	cp -v "./scripts/nqiv_deploy_cfg.bat" "$L_RELEASE_BASE"
	cp -v "./README.md" "$L_RELEASE_BASE"
	cp -v "./CHANGELOG.md" "$L_RELEASE_BASE"
	cp -v "./AUTHORS.md" "$L_RELEASE_BASE"
	cp -v "./COPYING" "$L_RELEASE_BASE/LICENSE"

	local L_SDL_LICENSE_PATH="$a_package_dir/SDL2.LICENSE"
	if [ ! -e "$L_SDL_LICENSE_PATH" ]
	then
		curl -L "https://raw.githubusercontent.com/libsdl-org/SDL/refs/heads/SDL2/LICENSE.txt" -o "$L_SDL_LICENSE_PATH"
	fi
	cp -v "$L_SDL_LICENSE_PATH" "$L_RELEASE_BASE/"

	local L_VIPS_DIR=
	L_VIPS_DIR="$(find "$a_package_dir/$G_VIPS_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
	cp -v "$L_VIPS_DIR/LICENSE" "$L_RELEASE_BASE/libvips.LICENSE"

	cp -v "$a_package_dir/$G_JEMALLOC_DIR/COPYING" "$L_RELEASE_BASE/jemalloc.LICENSE"

	cp -v "$a_exe_path" "$L_RELEASE_BASE"
	cp -v "$L_LIBPATH"/*.dll "$L_RELEASE_BASE"
	cp -v "$a_mxe_dir/usr/$a_mxe_target/lib/jemalloc.dll" "$L_RELEASE_BASE"

	zip -r -9 "$L_RELEASE_BASE.zip" "$L_RELEASE_BASE/"
}

main()
{
	set -eu
	while getopts "ht:p:d:D:v:j:e:c:E:R:r:" opt;
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
		'e')
			a_env_path="$OPTARG"
			;;
		'c')
			a_configure_maker_path="$OPTARG"
			;;
		'E')
			a_exe_path="$OPTARG"
			;;
		'R')
			a_release_path="$OPTARG"
			;;
		'r')
			a_release_version="$OPTARG"
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
		'clean-mxe')
			action_clean_mxe
			;;
		'load-mxe')
			action_load_mxe
			;;
		'clean-vips')
			action_clean_vips
			;;
		'load-vips')
			action_load_vips
			;;
		'clean-jemalloc')
			action_clean_jemalloc
			;;
		'load-jemalloc')
			action_load_jemalloc
			;;
		'clean-toolchain')
			action_clean_toolchain
			;;
		'load-toolchain')
			action_load_toolchain
			;;
		'make-environment')
			action_make_environment
			;;
		'make-configure')
			action_make_configure
			;;
		'clean-package-tmp')
			action_clean_package_tmp
			;;
		'clean-package')
			action_clean_package
			;;
		'make-package')
			action_make_package
			;;
		*)
			errcho "Error: Unrecognized action '$action'."
			exit_usage 1
			;;
		esac
	done
}

main "$@"
