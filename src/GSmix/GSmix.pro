
QT       += gui widgets sql
QTPLUGIN += QSQLMYSQL

TARGET = GSmix
TEMPLATE = app
RC_FILE	 = icon.rc

SOURCES += main.cpp \
        MainWindow.cpp \
        SearchBox.cpp \
    MidList.cpp \
    MidCache.cpp \
    RunPlanner.cpp

HEADERS  += MainWindow.h \
        SearchBox.h \
    MidList.h \
    MidCache.h \
    RunPlanner.h

FORMS    += MainWindow.ui \
        SearchBox.ui \
    MidList.ui \
    RunPlanner.ui

include("../app_gui.pri")


#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

RESOURCES += \
    resources.qrc

