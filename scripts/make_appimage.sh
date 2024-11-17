#!/bin/sh -efu

G_VERSION="1.1.0-beta"

errcho()
{
	>&2 echo "$@";
}

G_ARCH="$(uname -m)"
if [ "$(echo "$G_ARCH" | sed --quiet --posix --sandbox -E -e 's/^(x86|i[3-9]86)$/\1/p')" = "$G_ARCH" ]
then
	G_ARCH="i386"
elif [ "$G_ARCH" != "x86_64" ]
then
	errcho "Failed to find linuxdeploy version for architecture $G_ARCH"
	exit 1
fi

G_LINUXDEPLOY="linuxdeploy-$G_ARCH.AppImage"
G_LINUXDEPLOY_LINK="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/$G_LINUXDEPLOY"

if [ ! -e "./$G_LINUXDEPLOY" ]
then
	curl -L "$G_LINUXDEPLOY_LINK" -O 
	chmod +x "$G_LINUXDEPLOY"
fi

rm -rf ./src/AppDir/
make install DESTDIR=./src/AppDir
"./$G_LINUXDEPLOY"  --appdir=./src/AppDir/                    \
					--executable=./src/AppDir/usr/bin/nqiv    \
					--icon-file ./media/nqiv_icon_256x256.png \
					--desktop-file ./scripts/nqiv.desktop     \
					--output appimage

mv -v "nqiv-$G_ARCH.AppImage" "nqiv-$G_VERSION-$G_ARCH.AppImage"
