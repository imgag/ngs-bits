
QT       += gui widgets network sql xml xmlpatterns printsupport charts svg
QTPLUGIN += QSQLMYSQL

TARGET = MVHub
TEMPLATE = app

SOURCES += main.cpp \
	HttpHandler.cpp \
    MainWindow.cpp

HEADERS += MainWindow.h \
	HttpHandler.h

FORMS    += MainWindow.ui

include("../app_gui.pri")

RESOURCES += \
    resources.qrc
