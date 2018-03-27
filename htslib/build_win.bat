
REM clean up old includes/libs
rmdir /S /Q include
rmdir /S /Q lib
mkdir include
mkdir lib

REM copy header to 
xcopy htslib include\htslib\ /S

REM add MinGW to path
SET QT=C:\Qt\Qt5.5.0\Tools\mingw492_32\bin\
IF NOT EXIST "%QT%" SET QT=C:\Qt\Qt5.6.0\Tools\mingw492_32\bin\
SET PATH=%PATH%;%QT%

REM build
%QT%mingw32-make.exe lib-shared

REM move files to 'lib' folder
move hts-2.dll lib\hts-2.dll
move hts.dll.a lib\hts.dll.a

