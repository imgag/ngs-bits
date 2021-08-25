#!/bin/bash
echo "Building Windows version of GSvar app"
git clone --recursive https://github.com/imgag/ngs-bits.git
cd ngs-bits
git checkout server
git submodule update --recursive --init
rm -rf htslib
cp ../htslib-1.13 ./htslib
mkdir htslib/lib
mkdir htslib/include
mkdir htslib/include/htslib
cp htslib/hts-3.dll htslib/lib
cp htslib/hts.dll.a htslib/lib
cp htslib/libhts.a htslib/lib
cp htslib/htslib/* htslib/include/htslib
mkdir gsvar-win-binary
cd gsvar-win-binary
/bin/i686-w64-mingw32-qmake-qt5 ../src/tools_gui.pro
make
