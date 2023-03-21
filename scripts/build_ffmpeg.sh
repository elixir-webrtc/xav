#!/bin/bash

if [[ $# -eq 0 ]] ; then
  echo "USAGE: $0 <FFMPEG_VERSION>"
  exit 0
fi

FFMPEG_VERSION=$1

echo -e "CLONING FFMPEG $FFMPEG_VERSION\n"

git clone --depth 1 --branch n$FFMPEG_VERSION https://github.com/FFmpeg/FFmpeg.git ffmpeg

echo -e "CONFIGURING FFMPEG\n"

cd ffmpeg
mkdir ffmpeg_build

PKG_CONFIG_PATH="$PWD/ffmpeg_build/lib/pkgconfig" ./configure \
--prefix="$PWD/ffmpeg_build" \
--pkg-config-flags="--static" \
--extra-cflags="-fPIC -I$PWD/ffmpeg_build/include" \
--extra-ldflags="-L$PWD/ffmpeg_build/lib" \
--enable-pic
--disable-programs

echo -e "COMPILING\n"

make -j5

echo -e "INSTALLING\n"

make install

echo -e "CREATING TARBALL\n"

tar -cvzf ffmpeg_$FFMPEG_VERSION-x86_64-linux.tar.gz \
ffmpeg_build/include \
ffmpeg_build/lib/
