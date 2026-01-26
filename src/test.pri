include("base.pri")

#base settings
TEMPLATE = app
QT       -= gui
QT       += network
CONFIG   += console
CONFIG   -= app_bundle

#reduce optimization to improve compile time (this is only a test, thus we don't need optimization)
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE *= -O0

#include cppCORE library
INCLUDEPATH += $$PWD/cppCORE
LIBS += -L$$PWD/../bin -lcppCORE

#include cppTFW library
INCLUDEPATH += $$PWD/cppTFW
