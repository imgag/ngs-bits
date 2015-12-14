
QT       += gui widgets network sql xml xmlpatterns
QTPLUGIN += QSQLMYSQL

TARGET = GSvar
TEMPLATE = app
RC_FILE	 = icon.rc

SOURCES += main.cpp\
        MainWindow.cpp \
    FilterDockWidget.cpp \
    ExternalToolDialog.cpp \
    ReportDialog.cpp \
	ReportWorker.cpp \
	SampleInformationDialog.cpp \
    TrioDialog.cpp \
    HttpHandler.cpp \
    ValidationDialog.cpp \
    ClassificationDialog.cpp \
    DBAnnotationWorker.cpp \
    FilterColumnWidget.cpp

HEADERS  += MainWindow.h \
    FilterDockWidget.h \
    ExternalToolDialog.h \
    ReportDialog.h \
    ReportWorker.h \
	SampleInformationDialog.h \
    TrioDialog.h \
    HttpHandler.h \
    ValidationDialog.h \
    ClassificationDialog.h \
    DBAnnotationWorker.h \
    FilterColumnWidget.h


FORMS    += MainWindow.ui \
    FilterDockWidget.ui \
    ExternalToolDialog.ui \
    ReportDialog.ui \
	SampleInformationDialog.ui \
    TrioDialog.ui \
    ClassificationDialog.ui \
    ValidationDialog.ui \
    FilterColumnWidget.ui
    

include("../app_gui.pri")


#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

RESOURCES += \
    resources.qrc

DISTFILES += \
    Icons/Keep_gray.png \
    Icons/Remove_gray.png
