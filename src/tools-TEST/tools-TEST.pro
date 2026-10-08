include("../test.pri")

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppNGSD library
QT       += sql
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#include libxml2
win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2
unix: INCLUDEPATH += $$system(pkg-config --cflags libxml-2.0)
unix: !macx: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#special treatment for VcfToBedpe
INCLUDEPATH += $$PWD/../VcfToBedpe

SOURCES += $$files(./*.cpp)
