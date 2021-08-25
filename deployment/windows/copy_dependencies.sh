#!/bin/bash
echo "Copy all mingw dependencies  to make the app portable"
mkdir dependencies
cp /usr/i686-w64-mingw32/sys-root/mingw/bin/*.dll dependencies/
cp /usr/i686-w64-mingw32/sys-root/mingw/lib/*.a dependencies/
cp -r /usr/i686-w64-mingw32/sys-root/mingw/lib/qt5/plugins/* dependencies/
