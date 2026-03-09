include("../app_gui.pri")

QT       += gui widgets network sql xml printsupport charts svg
QTPLUGIN += QSQLMYSQL

TARGET = GSviewer

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include VISUAL library
INCLUDEPATH += $$PWD/../cppVISUAL
LIBS += -L$$PWD/../../bin -lcppVISUAL

RESOURCES += GSviewer.qrc

SOURCES += main.cpp \
    MainWindow.cpp

HEADERS += MainWindow.h

FORMS += MainWindow.ui
