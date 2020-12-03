
REM clean up old includes/libs
rmdir /S /Q include
rmdir /S /Q lib
mkdir include
mkdir lib

REM copy header to 
xcopy htslib include\htslib\ /S

REM add MinGW to path
SET PATH=%PATH%;C:\Qt\Qt5.9.5\Tools\mingw530_32\bin\

REM build
%QT%mingw32-make.exe lib-shared

REM move files to 'lib' folder
move hts-3.dll lib\hts-3.dll
move hts.dll.a lib\hts.dll.a

