REM build dlls
cd C:\Qt\Qt5.5.0\qt-everywhere-opensource-src-5.5.0\qtbase\src\plugins\sqldrivers\mysql\
qmake "INCLUDEPATH+=C:\Qt\Qt5.5.0\mysql-5.7.9-win32\include" "LIBS+=C:\Qt\Qt5.5.0\mysql-5.7.9-win32\lib\libmysql.lib" -o Makefile mysql.pro
mingw32-make.exe

REM copy dlls
copy C:\Qt\Qt5.5.0\qt-everywhere-opensource-src-5.5.0\qtbase\plugins\sqldrivers\qsqlmysql*.dll C:\Qt\Qt5.5.0\5.5\mingw492_32\plugins\sqldrivers\