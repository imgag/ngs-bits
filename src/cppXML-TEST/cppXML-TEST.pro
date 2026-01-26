include("../test.pri")
QT += xml

#include XML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

SOURCES += \
    XmlValidation_Test.cpp \
    main.cpp

RESOURCES += \
    cppXML-TEST.qrc

