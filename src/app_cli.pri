#c++11 support
CONFIG += c++11 
QT += network

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#include cppCORE library
INCLUDEPATH += $$PWD/cppCORE
LIBS += -L$$PWD/../bin -lcppCORE

#include cppXML library
INCLUDEPATH += $$PWD/cppXML
LIBS += -L$$PWD/../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/cppNGS
LIBS += -L$$PWD/../bin -lcppNGS

#include htslib library
INCLUDEPATH += $$PWD/../htslib/include/
LIBS += -L$$PWD/../htslib/lib/ -lhts

#include zlib library
LIBS += -lz

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

#copy EXE to bin folder
DESTDIR = ../../bin/
