#!/bin/bash

if [[ $# -eq 0 ]] ; then
  echo "USAGE: $0 <FFMPEG_VERSION>"
  exit 0
fi

FFMPEG_VERSION=$1

echo -e "CLONING FFMPEG $FFMPEG_VERSION\n"

git clone --depth 1 --branch n$FFMPEG_VERSION https://git.ffmpeg.org/ffmpeg.git ffmpeg

echo -e "CONFIGURING FFMPEG\n"

cd ffmpeg
mkdir ffmpeg_build

# should we include fPIC in cflags?
./configure \
--prefix="$PWD/ffmpeg_build" \
--extra-cflags="-I$PWD/ffmpeg_build/include" \
--extra-ldflags="-L$PWD/ffmpeg_build/lib" \
--enable-shared 

echo -e "COMPILING\n"

make

echo -e "INSTALLING\n"

make install

echo -e "CREATING TARBALL\n"

tar -cvzhf ffmpeg_$FFMPEG_VERSION-x86_64-linux.tar.gz \
ffmpeg_build/include \
ffmpeg_build/lib/libavcodec.so \
ffmpeg_build/lib/libavdevice.so \
ffmpeg_build/lib/libavfilter.so \
ffmpeg_build/lib/libavformat.so \
ffmpeg_build/lib/libavutil.so \
ffmpeg_build/lib/libswresample.so \
ffmpeg_build/lib/libswscale.so