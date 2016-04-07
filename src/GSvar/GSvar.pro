
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
    FilterColumnWidget.cpp \
    ApprovedGenesDialog.cpp \
    GeneInfoDialog.cpp \
    PhenoToRoiDialog.cpp \
    PhenotypeSelector.cpp

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
    FilterColumnWidget.h \
    ApprovedGenesDialog.h \
    GeneInfoDialog.h \
    PhenoToRoiDialog.h \
    PhenotypeSelector.h


FORMS    += MainWindow.ui \
    FilterDockWidget.ui \
    ExternalToolDialog.ui \
    ReportDialog.ui \
	SampleInformationDialog.ui \
    TrioDialog.ui \
    ClassificationDialog.ui \
    ValidationDialog.ui \
    FilterColumnWidget.ui \
    ApprovedGenesDialog.ui \
    GeneInfoDialog.ui \
    PhenoToRoiDialog.ui \
    PhenotypeSelector.ui
    

include("../app_gui.pri")


#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

RESOURCES += \
    resources.qrc

DISTFILES += \
    Icons/Keep_gray.png \
    Icons/Remove_gray.png
