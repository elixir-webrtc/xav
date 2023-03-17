#!/bin/bash

if [[ $# -eq 0 ]] ; then
  echo "USAGE: $0 <FFMPEG_VERSION>"
  exit 0
fi

FFMPEG_VERSION=$1

mkdir ffmpeg
cd ffmpeg

mkdir test_include
cd test_include
touch a.h
touch b.h
cd ..
touch x.so

tar -czvf ffmpeg_$FFMPEG_VERSION-x86_64-linux.tar.gz test_include x.so