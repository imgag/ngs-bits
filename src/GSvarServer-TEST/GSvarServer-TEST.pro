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
DESTDIR = ../../bin/

include("../app_cli.pri")

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

#include NGSD library
INCLUDEPATH += $$PWD/../cppREST
LIBS += -L$$PWD/../bin -lcppREST

#the server itself
INCLUDEPATH += $$PWD/../GSvarServer

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

HEADERS += \
    Controller-Test.h \
    Server-IntegrationTest.h \
    VersatileFile-Test.h

SOURCES += \
        main.cpp

RESOURCES += \
    ../GSvarServer/GSvarServer.qrc
