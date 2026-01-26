include("../app_cli.pri")

#base settings
QT += sql

QTPLUGIN += QSQLMYSQL

CONFIG   += console
CONFIG   -= app_bundle

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

SOURCES += \
    Controller_Test.cpp \
    ServerIntegration_Test.cpp \
    main.cpp

RESOURCES += \
    ../GSvarServer/GSvarServer.qrc
