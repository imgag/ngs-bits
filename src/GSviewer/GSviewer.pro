include("../app_gui.pri")

QT       += gui widgets network sql xml printsupport charts svg \
    core5compat
QTPLUGIN += QSQLMYSQL

TARGET = GSviewer
RC_FILE	 = icon.rc

#include VISUAL library
INCLUDEPATH += $$PWD/../cppVISUAL
LIBS += -L$$PWD/../../bin -lcppVISUAL

RESOURCES += GSviewer.qrc

SOURCES += main.cpp \
    CommandListener.cpp \
    CommandServer.cpp \
    MainWindow.cpp

HEADERS += MainWindow.h \
    CommandListener.h \
    CommandServer.h

FORMS += MainWindow.ui
