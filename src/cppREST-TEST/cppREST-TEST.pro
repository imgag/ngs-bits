include("../test.pri")

QT += sql

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include NGSD library
INCLUDEPATH += $$PWD/../cppREST
LIBS += -L$$PWD/../../bin -lcppREST

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

SOURCES += \
        FileMetaCache_Test.cpp \
        HtmlEngine_Test.cpp \
        HttpProcessor_Test.cpp \
        RequestParser_Test.cpp \
        ServerHelper_Test.cpp \
        UrlManager_Test.cpp \
        main.cpp
