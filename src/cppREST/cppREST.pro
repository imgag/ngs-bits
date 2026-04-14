include("../lib.pri")

#base settings
QT       -= gui
QT       += sql xml httpserver
QTPLUGIN += QSQLMYSQL
TARGET = cppREST
DEFINES += CPPREST_LIBRARY

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

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

SOURCES += \
    BasicServer.cpp \
    EndpointManager.cpp \
    FastFileInfo.cpp \
    FileMetaCache.cpp \
    HtmlEngine.cpp \
    HttpUtils.cpp \
    HttpRequest.cpp \
    HttpResponse.cpp \
    QueuingEngineApiHelper.cpp \
    RequestParser.cpp \
    RequestWorker.cpp \
    ServerDB.cpp \
    ServerHelper.cpp \
    SessionAndUrlBackupWorker.cpp \
    SessionManager.cpp \
    SslServer.cpp \
    UrlManager.cpp

HEADERS += \
    BasicServer.h \
    EndpointManager.h \
    FastFileInfo.h \
    FileMetaCache.h \
    HtmlEngine.h \
    HttpParts.h \
    HttpUtils.h \
    HttpRequest.h \
    HttpResponse.h \
    QueuingEngineApiHelper.h \
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
