REM create and enter build folder
del /F /Q bamtools\build bamtools\bin bamtools\include bamtools\lib
rmdir /Q bamtools\build
mkdir bamtools\build
cd bamtools\build

REM add MinGW to path
SET PATH=%PATH%;C:\Qt\Qt5.5.0\Tools\mingw492_32\bin

REM build
cmake -G "MinGW Makefiles" ..
C:\Qt\Qt5.5.0\Tools\mingw492_32\bin\mingw32-make.exe
REM go back
cd ..
cd ..
