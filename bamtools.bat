REM create and enter build folder
del /F /Q bamtools\build bamtools\bin bamtools\include bamtools\lib
rmdir /Q bamtools\build
mkdir bamtools\build
cd bamtools\build

REM add MinGW to path
SET QT=C:\Qt\Qt5.5.0\Tools\mingw492_32\bin\
IF NOT EXIST "%QT%" SET QT=C:\Qt\Qt5.6.0\Tools\mingw492_32\bin\
SET PATH=%PATH%;%QT%

REM build
cmake -G "MinGW Makefiles" ..
%QT%mingw32-make.exe
REM go back
cd ..
cd ..
