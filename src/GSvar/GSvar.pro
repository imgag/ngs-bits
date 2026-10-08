include("../app_gui.pri")

QT       += gui widgets network sql xml printsupport charts svg
QTPLUGIN += QSQLMYSQL

TARGET = GSvar
RC_FILE	 = icon.rc

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include VISUAL library
INCLUDEPATH += $$PWD/../cppVISUAL
LIBS += -L$$PWD/../../bin -lcppVISUAL

RESOURCES += GSvar.qrc

mac {
ICON = Icons/Icon.icns
}

SOURCES = $$files($$PWD/*.cpp)
SOURCES += $$files($$PWD/Background/*.cpp)

HEADERS = $$files($$PWD/*.h)
HEADERS += $$files($$PWD/Background/*.h)

FORMS = $$files($$PWD/*.ui)
FORMS += $$files($$PWD/Background/*.ui)