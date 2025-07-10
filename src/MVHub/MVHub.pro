
QT       += gui widgets network sql xml printsupport charts svg
QTPLUGIN += QSQLMYSQL

TARGET = MVHub
TEMPLATE = app

SOURCES += main.cpp \
    ExportHistoryDialog.cpp \
    HttpHandler.cpp \
    MVHub.cpp

HEADERS += MVHub.h \
	ExportHistoryDialog.h \
	HttpHandler.h

FORMS    += MVHub.ui \
    ExportHistoryDialog.ui

include("../app_gui.pri")

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

#include VISUAL library
INCLUDEPATH += $$PWD/../cppVISUAL
LIBS += -L$$PWD/../bin -lcppVISUAL

RESOURCES += \
    resources.qrc
