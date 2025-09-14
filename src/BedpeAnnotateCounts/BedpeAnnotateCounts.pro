include("../app_cli.pri")

SOURCES += main.cpp

#include cppNGS library
QT       += sql
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD
