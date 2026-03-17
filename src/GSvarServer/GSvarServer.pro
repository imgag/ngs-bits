QT += core
QT += network
QT -= gui
QT += sql
QT += httpserver

QTPLUGIN += QSQLMYSQL

CONFIG += console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        QueuingEngineController.cpp \
        QueuingEngineControllerGeneric.cpp \
        QueuingEngineControllerSge.cpp \
        QueuingEngineControllerSlurm.cpp \
        ServerController.cpp \
        ServerWrapper.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    QueuingEngineController.h \
    QueuingEngineControllerGeneric.h \
    QueuingEngineControllerSge.h \
    QueuingEngineControllerSlurm.h \
    ServerController.h \
    ServerWrapper.h

include("../app_cli.pri")

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include REST library
INCLUDEPATH += $$PWD/../cppREST
LIBS += -L$$PWD/../../bin -lcppREST

RESOURCES += \
    GSvarServer.qrc
