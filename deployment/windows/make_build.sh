#!/bin/bash

# Htslib
# Downloading hts release version
echo "******************"
echo "Downloading htslib"
echo "******************"
curl https://github.com/samtools/htslib/releases/download/1.13/htslib-1.13.tar.bz2 --location --output htslib-1.13.tar.bz2
tar -xjf ./htslib-1.13.tar.bz2
rm ./htslib-1.13.tar.bz2
cd /root/build/htslib-1.13

#Building htslib with HTTPS support
echo ""
echo "***************"
echo "Building htslib"
echo "***************"
export CC=/usr/bin/i686-w64-mingw32-gcc
./configure --host=i686-w646-mingw32 --enable-libcurl
make
cd ../

# GSvar
echo ""
echo "****************************"
echo "Cloning the GSvar repository"
echo "****************************"
git clone --recursive https://github.com/imgag/ngs-bits.git
cd ngs-bits
git checkout server
git submodule update --recursive --init
rm -rf htslib
echo ""
echo "***************************************"
echo "Copying htslib to the repository folder"
echo "***************************************"
mkdir htslib
mkdir htslib/lib
mkdir htslib/include
mkdir htslib/include/htslib
cp ../htslib-1.13/hts-3.dll htslib/lib/
cp ../htslib-1.13/hts.dll.a htslib/lib/
cp ../htslib-1.13/libhts.a htslib/lib/
cp -r ../htslib-1.13/htslib/* htslib/include/htslib/
mkdir gsvar-win-binary
cd gsvar-win-binary
/bin/i686-w64-mingw32-qmake-qt5 ../src/tools_gui.pro
echo ""
echo "*************************************"
echo "Building Windows version of GSvar app"
echo "*************************************"
make
cd ../../

# Copy depdendencies to a volume
echo ""
echo "****************************************************"
echo "Copy all mingw dependencies to make the app portable"
echo "****************************************************"
mkdir dependencies
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/*.dll dependencies/
cp /usr/i686-w64-mingw32/sys-root/mingw/lib/*.a dependencies/
cp -r /usr/i686-w64-mingw32/sys-root/mingw/lib/qt5/plugins/* dependencies/

echo ""
echo "***************************************************"
echo "Copy GSvar and all its dependencies into the volume"
echo "***************************************************"
cp -r dependencies/* /root/output/
cp -r ngs-bits/bin/* /root/output/
mkdir /root/output/htslib
cp -r ngs-bits/htslib/* /root/output/htslib

echo ""
echo "*********************************"
echo "The processing has been completed"
echo "*********************************"