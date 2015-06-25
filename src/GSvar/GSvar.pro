#-------------------------------------------------
#
# Project created by QtCreator 2013-08-13T08:31:34
#
#-------------------------------------------------

QT       += gui widgets network sql xml xmlpatterns
QTPLUGIN += QSQLMYSQL

TARGET = GSvar
TEMPLATE = app
RC_FILE	 = icon.rc

SOURCES += main.cpp\
        MainWindow.cpp \
    FilterDockWidget.cpp \
    StatisticsWorker.cpp \
    ExternalToolDialog.cpp \
    ReportDialog.cpp \
	ReportWorker.cpp \
	SampleInformationDialog.cpp \
    TrioDialog.cpp \
    HttpHandler.cpp \
    ValidationDialog.cpp

    

HEADERS  += MainWindow.h \
    FilterDockWidget.h \
    StatisticsWorker.h \
    ExternalToolDialog.h \
    ReportDialog.h \
    ReportWorker.h \
	SampleInformationDialog.h \
    TrioDialog.h \
    HttpHandler.h \
    ValidationDialog.h

FORMS    += MainWindow.ui \
    FilterDockWidget.ui \
    ExternalToolDialog.ui \
    ReportDialog.ui \
	SampleInformationDialog.ui \
    TrioDialog.ui \
    ValidationDialog.ui
    

include("../app_gui.pri")


#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

RESOURCES += \
    resources.qrc
