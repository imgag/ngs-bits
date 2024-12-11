
QT       += gui widgets network sql xml xmlpatterns printsupport charts svg
QTPLUGIN += QSQLMYSQL

TARGET = MVHub
TEMPLATE = app

SOURCES += main.cpp \
    HttpHandler.cpp \
    MVHub.cpp

HEADERS += MVHub.h \
	HttpHandler.h

FORMS    += MVHub.ui

include("../app_gui.pri")

RESOURCES += \
    resources.qrc
