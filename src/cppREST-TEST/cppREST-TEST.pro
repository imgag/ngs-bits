#c++11 support
CONFIG += c++11

#base settings
QT += core
QT += network
QT -= gui
QT += sql

QTPLUGIN += QSQLMYSQL

CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
DESTDIR = ../../../../bin/

include("../app_cli.pri")

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include NGSD library
INCLUDEPATH += $$PWD/../cppREST
LIBS += -L$$PWD/../../bin -lcppREST

#the server itself
INCLUDEPATH += $$PWD/../GSvarServer

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

HEADERS += \
    FileMetaCache_Test.h \
    HtmlEngine_Test.h \
    HttpProcessor_Test.h \
    RequestParser_Test.h \
    ServerHelper_Test.h \
    UrlManager_Test.h

SOURCES += \
        main.cpp

DISTFILES +=

RESOURCES +=

