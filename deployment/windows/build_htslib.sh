#!/bin/bash
# Downloading hts release version
wget https://github.com/samtools/htslib/releases/download/1.13/htslib-1.13.tar.bz2
tar -xjf ./htslib-1.13.tar.bz2
rm ./htslib-1.13.tar.bz2
cd /root/build/htslib-1.13
#Building htslib with HTTPS support
export CC=/usr/bin/i686-w64-mingw32-gcc
./configure --host=i686-w646-mingw32 --enable-libcurl
make
