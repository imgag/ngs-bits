#c++14 support (only needed for Qt5. Qt6 is on C++17 or higher anyway)
CONFIG += c++14

#define project root folder
PROJECT_ROOT = $$absolute_path($$PWD/..)
#message(PROJECT_ROOT is $$PROJECT_ROOT)
#export projectr root as preprocessor define for C++
DEFINES += PROJECT_ROOT=\\\"$$PROJECT_ROOT\\\"

#define bin/dll destiantion dir
DESTDIR = $$PROJECT_ROOT/bin/
#message(DESTDIR is $$DESTDIR)

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

#include zlib library
LIBS += -lz
