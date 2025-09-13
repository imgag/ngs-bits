include("../app_cli.pri")

SOURCES += main.cpp \
    Auxilary.cpp \
    ThreadCoordinator.cpp \
    ExportWorker.cpp

HEADERS += \
    Auxilary.h \
    ExportWorker.h \
    ThreadCoordinator.h

#include cppNGS library
QT       += sql
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

