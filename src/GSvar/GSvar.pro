
QT       += gui widgets network sql xml xmlpatterns printsupport charts
QTPLUGIN += QSQLMYSQL

TARGET = GSvar
TEMPLATE = app
RC_FILE	 = icon.rc

SOURCES += main.cpp\
    MainWindow.cpp \
    ExternalToolDialog.cpp \
    ReportDialog.cpp \
    ReportWorker.cpp \
    TrioDialog.cpp \
    HttpHandler.cpp \
    ValidationDialog.cpp \
    ClassificationDialog.cpp \
    ApprovedGenesDialog.cpp \
    GeneWidget.cpp \
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
    SmallVariantSearchWidget.cpp \
    PhenotypeSelectionWidget.cpp \
    LovdUploadDialog.cpp \
    RohWidget.cpp \
    DiagnosticStatusWidget.cpp \
    DiagnosticStatusOverviewDialog.cpp \
    SvWidget.cpp \
    SingleSampleAnalysisDialog.cpp \
    SomaticDialog.cpp \
    FilterEditDialog.cpp \
    SampleDiseaseInfoWidget.cpp \
    DBTableWidget.cpp \
    ProcessedSampleWidget.cpp \
    DBQCWidget.cpp \
    DBComboBox.cpp \
    DBSelector.cpp \
    DiseaseInfoWidget.cpp \
    SequencingRunWidget.cpp \
    AnalysisStatusWidget.cpp \
    VariantTable.cpp \
    RtfDocument.cpp \
    SampleSearchWidget.cpp \
    SomaticReportHelper.cpp \
    SampleRelationDialog.cpp \
    ProcessedSampleSelector.cpp \
    FilterWidgetCNV.cpp \
    FilterCascadeWidget.cpp \
    FilterWidget.cpp \
    ReportSettings.cpp \
    ReportVariantDialog.cpp \
    GSvarHelper.cpp \
    SomaticRnaReport.cpp \
    ProcessedSampleDataDeletionDialog.cpp \
    VariantWidget.cpp \
    ProcessingSystemWidget.cpp \
    ProjectWidget.cpp \
    DBEditor.cpp \
    FilterWidgetSV.cpp \
    DBTableAdministration.cpp \
    SequencingRunOverview.cpp \
    SomaticReportVariantDialog.cpp \
    SomaticReportDialog.cpp \
    MidCheckWidget.cpp \
    CnvSearchWidget.cpp \
    VariantValidationWidget.cpp \
    GeneOmimInfoWidget.cpp \
    LoginDialog.cpp \
    GeneInfoDBs.cpp \
    VariantConversionWidget.cpp \
    PasswordDialog.cpp \
    CircosPlotWidget.cpp \
    EmailDialog.cpp \
    CytobandToRegionsDialog.cpp\
    SvSearchWidget.cpp \
    RepeatExpansionWidget.cpp \
    PRSWidget.cpp \
    SomaticDataTransferWidget.cpp \
    EvaluationSheetEditDialog.cpp \
    PublishedVariantsWidget.cpp \
    PreferredTranscriptsWidget.cpp\
    TumorOnlyReportWorker.cpp \
    TumorOnlyReportDialog.cpp \
    CfDNAPanelDesignDialog.cpp \
    DiseaseCourseWidget.cpp \
    CfDNAPanelWidget.cpp

HEADERS += MainWindow.h \
    ExternalToolDialog.h \
    ReportDialog.h \
    ReportWorker.h \
    TrioDialog.h \
    HttpHandler.h \
    ValidationDialog.h \
    ClassificationDialog.h \
    ApprovedGenesDialog.h \
    GeneWidget.h \
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
    SmallVariantSearchWidget.h \
    PhenotypeSelectionWidget.h \
    LovdUploadDialog.h \
    RohWidget.h \
    DiagnosticStatusWidget.h \
    DiagnosticStatusOverviewDialog.h \
    SvWidget.h \
    SingleSampleAnalysisDialog.h \
    SomaticDialog.h \
    FilterEditDialog.h \
    SampleDiseaseInfoWidget.h \
    DBTableWidget.h \
    ProcessedSampleWidget.h \
    DBQCWidget.h \
    DBComboBox.h \
    DBSelector.h \
    DiseaseInfoWidget.h \
    SequencingRunWidget.h \
    AnalysisStatusWidget.h \
    VariantTable.h \
    RtfDocument.h \
    SampleSearchWidget.h \
    SomaticReportHelper.h \
    SampleRelationDialog.h \
    ProcessedSampleSelector.h \
    FilterWidgetCNV.h \
    FilterCascadeWidget.h \
    FilterWidget.h \
    ReportSettings.h \
    ReportVariantDialog.h \
    GSvarHelper.h \
    SomaticRnaReport.h \
    ProcessedSampleDataDeletionDialog.h \
    VariantWidget.h \
    ProcessingSystemWidget.h \
    ProjectWidget.h \
    DBEditor.h \
    FilterWidgetSV.h \
    DBTableAdministration.h \
    SequencingRunOverview.h \
    SomaticReportVariantDialog.h \
    SomaticReportDialog.h \
    MidCheckWidget.h \
    CnvSearchWidget.h \
    VariantValidationWidget.h \
    GeneOmimInfoWidget.h \
    LoginDialog.h \
    GeneInfoDBs.h \
    VariantConversionWidget.h \
    PasswordDialog.h \
    CircosPlotWidget.h \
    EmailDialog.h \
    CytobandToRegionsDialog.h\
    SvSearchWidget.h \
    RepeatExpansionWidget.h \
    PRSWidget.h \
    SomaticDataTransferWidget.h \
    EvaluationSheetEditDialog.h \
    PublishedVariantsWidget.h \
    PreferredTranscriptsWidget.h \
    TumorOnlyReportWorker.h \
    TumorOnlyReportDialog.h \
    CfDNAPanelDesignDialog.h \
    DiseaseCourseWidget.h \
    CfDNAPanelWidget.h

FORMS    += MainWindow.ui \
    ExternalToolDialog.ui \
    ReportDialog.ui \
    TrioDialog.ui \
    ClassificationDialog.ui \
    ValidationDialog.ui \
    ApprovedGenesDialog.ui \
    GeneWidget.ui \
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
    SmallVariantSearchWidget.ui \
    PhenotypeSelectionWidget.ui \
    LovdUploadDialog.ui \
    RohWidget.ui \
    DiagnosticStatusWidget.ui \
    DiagnosticStatusOverviewDialog.ui \
    SvWidget.ui \
    SingleSampleAnalysisDialog.ui \
    SomaticDialog.ui \
    FilterEditDialog.ui \
    SampleDiseaseInfoWidget.ui \
    ProcessedSampleWidget.ui \
    DBQCWidget.ui \
    DiseaseInfoWidget.ui \
    SequencingRunWidget.ui \
    AnalysisStatusWidget.ui \
    SampleSearchWidget.ui \
    SampleRelationDialog.ui \
    ProcessedSampleSelector.ui \
    FilterWidgetCNV.ui \
    FilterCascadeWidget.ui \
    FilterWidget.ui \
    ReportVariantDialog.ui \
    ProcessedSampleDataDeletionDialog.ui \
    VariantWidget.ui \
    ProcessingSystemWidget.ui \
    ProjectWidget.ui \
    FilterWidgetSV.ui \
    DBTableAdministration.ui \
    SequencingRunOverview.ui \
    SomaticReportVariantDialog.ui \
    SomaticReportDialog.ui \
    MidCheckWidget.ui \
    CnvSearchWidget.ui \
    VariantValidationWidget.ui \
    GeneOmimInfoWidget.ui \
    LoginDialog.ui \
    VariantConversionWidget.ui \
    PasswordDialog.ui \
    CircosPlotWidget.ui \
    EmailDialog.ui \
    CytobandToRegionsDialog.ui \
    SvSearchWidget.ui \
    RepeatExpansionWidget.ui \
    PRSWidget.ui \
    SomaticDataTransferWidget.ui \
    EvaluationSheetEditDialog.ui \
    PublishedVariantsWidget.ui \
    PreferredTranscriptsWidget.ui \
    TumorOnlyReportDialog.ui \
    CfDNAPanelDesignDialog.ui \
    DiseaseCourseWidget.ui \
    CfDNAPanelWidget.ui
include("../app_gui.pri")

#include NGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

RESOURCES += \
    GSvar.qrc
