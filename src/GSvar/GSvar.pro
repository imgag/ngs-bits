
QT       += gui widgets network sql xml xmlpatterns printsupport
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
    SampleDetailsDockWidget.cpp \
    TrioDialog.cpp \
    HttpHandler.cpp \
    ValidationDialog.cpp \
    ClassificationDialog.cpp \
    DBAnnotationWorker.cpp \
    ApprovedGenesDialog.cpp \
    GeneInfoDialog.cpp \
    PhenoToGenesDialog.cpp \
    PhenotypeSelector.cpp \
    GenesToRegionsDialog.cpp \
    VariantDetailsDockWidget.cpp \
    SubpanelDesignDialog.cpp \
    SubpanelArchiveDialog.cpp \
    IgvDialog.cpp \
    GapDialog.cpp \
    GapValidationLabel.cpp \
    CnvWidget.cpp \
    GeneSelectorDialog.cpp \
    MultiSampleDialog.cpp \
    NGSDReannotationDialog.cpp \
    DiseaseInfoDialog.cpp \
    CandidateGeneDialog.cpp \
    PhenotypeSelectionWidget.cpp \
    LovdUploadDialog.cpp \
    RohWidget.cpp \
    ReportHelper.cpp \
    DiagnosticStatusWidget.cpp \
    DiagnosticStatusOverviewDialog.cpp \
    SvWidget.cpp \
    AnalysisStatusDialog.cpp \
    SingleSampleAnalysisDialog.cpp \
    SomaticDialog.cpp \
    VariantSampleOverviewDialog.cpp \
    SomaticReportConfiguration.cpp \
    FilterEditDialog.cpp

HEADERS  += MainWindow.h \
    FilterDockWidget.h \
    ExternalToolDialog.h \
    ReportDialog.h \
    ReportWorker.h \
    SampleDetailsDockWidget.h \
    TrioDialog.h \
    HttpHandler.h \
    ValidationDialog.h \
    ClassificationDialog.h \
    DBAnnotationWorker.h \
    ApprovedGenesDialog.h \
    GeneInfoDialog.h \
    PhenoToGenesDialog.h \
    PhenotypeSelector.h \
    GenesToRegionsDialog.h \
    VariantDetailsDockWidget.h \
    SubpanelDesignDialog.h \
    SubpanelArchiveDialog.h \
    IgvDialog.h \
    GapDialog.h \
    GapValidationLabel.h \
    CnvWidget.h \
    GeneSelectorDialog.h \
    MultiSampleDialog.h \
    NGSDReannotationDialog.h \
    DiseaseInfoDialog.h \
    CandidateGeneDialog.h \
    PhenotypeSelectionWidget.h \
    LovdUploadDialog.h \
    RohWidget.h \
    ReportHelper.h \
    DiagnosticStatusWidget.h \
    DiagnosticStatusOverviewDialog.h \
    SvWidget.h \
    AnalysisStatusDialog.h \
    SingleSampleAnalysisDialog.h \
    SomaticDialog.h \
    VariantSampleOverviewDialog.h \
    SomaticReportConfiguration.h \
    FilterEditDialog.h

FORMS    += MainWindow.ui \
    FilterDockWidget.ui \
    ExternalToolDialog.ui \
    ReportDialog.ui \
    SampleDetailsDockWidget.ui \
    TrioDialog.ui \
    ClassificationDialog.ui \
    ValidationDialog.ui \
    ApprovedGenesDialog.ui \
    GeneInfoDialog.ui \
    PhenoToGenesDialog.ui \
    PhenotypeSelector.ui \
    GenesToRegionsDialog.ui \
    VariantDetailsDockWidget.ui \
    SubpanelDesignDialog.ui \
    SubpanelArchiveDialog.ui \
    IgvDialog.ui \
    GapDialog.ui \
    GapValidationLabel.ui \
    CnvWidget.ui \
    GeneSelectorDialog.ui \
    MultiSampleDialog.ui \
    NGSDReannotationDialog.ui \
    DiseaseInfoDialog.ui \
    CandidateGeneDialog.ui \
    PhenotypeSelectionWidget.ui \
    LovdUploadDialog.ui \
    RohWidget.ui \
    DiagnosticStatusWidget.ui \
    DiagnosticStatusOverviewDialog.ui \
    SvWidget.ui \
    AnalysisStatusDialog.ui \
    SingleSampleAnalysisDialog.ui \
    SomaticDialog.ui \
    VariantSampleOverviewDialog.ui \
    SomaticReportConfiguration.ui \
    FilterEditDialog.ui
    

include("../app_gui.pri")


#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

RESOURCES += \
    GSvar.qrc
