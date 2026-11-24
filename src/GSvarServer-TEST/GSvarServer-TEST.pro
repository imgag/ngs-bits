include("../app_cli.pri")

#base settings
QT += sql httpserver

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

HEADERS += \
    ../GSvarServer/ServerController.h \
    ../GSvarServer/XmlImportValidator.h

SOURCES += \
    Controller_Test.cpp \
    ServerIntegration_Test.cpp \
    ../GSvarServer/ServerController.cpp \
    ../GSvarServer/XmlImportValidator.cpp \
    main.cpp

RESOURCES += \
    ../GSvarServer/GSvarServer.qrc
