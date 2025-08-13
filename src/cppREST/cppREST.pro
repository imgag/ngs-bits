#c++11 and c++14 support
CONFIG += c++11 

#base settings
QT       -= gui
QT       += sql
QT       += xml
QT       += network
QTPLUGIN += QSQLMYSQL
TEMPLATE = lib
TARGET = cppREST
DEFINES += CPPREST_LIBRARY

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#copy DLL to bin folder
DESTDIR = ../../../../bin/

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include cppXML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#include zlib library
LIBS += -lz

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

SOURCES += \
    EndpointManager.cpp \
    FastFileInfo.cpp \
    FileMetaCache.cpp \
    HtmlEngine.cpp \
    HttpUtils.cpp \
    HttpRequest.cpp \
    HttpResponse.cpp \
    RequestParser.cpp \
    RequestWorker.cpp \
    ServerDB.cpp \
    ServerHelper.cpp \
    SessionAndUrlBackupWorker.cpp \
    SessionManager.cpp \
    SslServer.cpp \
    UrlManager.cpp

HEADERS += \
    EndpointManager.h \
    FastFileInfo.h \
    FileMetaCache.h \
    HtmlEngine.h \
    HttpParts.h \
    HttpUtils.h \
    HttpRequest.h \
    HttpResponse.h \
    RequestParser.h \
    RequestWorker.h \
    ServerDB.h \
    ServerHelper.h \
    Session.h \
    SessionAndUrlBackupWorker.h \
    SessionManager.h \
    SslServer.h \
    ThreadSafeHashMap.h \
    UrlEntity.h \
    UrlManager.h

RESOURCES +=

DISTFILES +=
