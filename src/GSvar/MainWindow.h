#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractSocket>
#include "ui_MainWindow.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "BusyDialog.h"
#include "FilterCascade.h"
#include "ReportSettings.h"
#include "DelayedInitializationTimer.h"
#include "SomaticReportSettings.h"
#include "GlobalServiceProvider.h"
#include "FileLocationProviderLocal.h"
#include "FileLocationProviderRemote.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);
	///Returns the result of applying filters to the variant list
	void applyFilters(bool debug_time);
	///Returns the LOG files corresponding to the variant list.
	QStringList getLogFiles();	
	///Adds a file to the recent file list
	void addToRecentFiles(QString filename);
	///Updates recent files menu
	void updateRecentFilesMenu();
	///Updates IGV menu
    void updateIGVMenu();
	///Updates menu and toolbar according to NGSD-support
	void updateNGSDSupport();
	///Returns 'nobr' paragraph start for Qt tooltips
	static QString nobr();
	///Upload variant to LOVD
	void uploadtoLovd(int variant_index, int variant_index2 = -1);

	///Context menu for single variant
	void contextMenuSingleVariant(QPoint pos, int index);
	///Context menu for two variants
	void contextMenuTwoVariants(QPoint pos, int index1, int index2);

	///Edit classification of a variant
	void editVariantClassification(VariantList& variant, int index, bool is_somatic = false);

	///Returns if germline report is supported for current variant list.
	bool germlineReportSupported(bool require_ngsd = true);
	///Returns the processed sample name for which report configuration is set and the report is generated.
	QString germlineReportSample();
	///Returns if somatic tumor-normal report is supported for current variant list.
	bool somaticReportSupported();
	///Returns if somatic tumpr-only report is supported for current variant list.
	bool tumoronlyReportSupported();

	///Lets the user select a gene. If the user aborts, "" is returned.
	static QString selectGene();

	///Performs batch import of table rows
	void importBatch(QString title, QString text, QString table, QStringList fields);

	///Returns the IGV port to use
	int igvPort() const;

public slots:
	///Loads a variant list. Unloads the variant list if no file name is given
	void loadFile(QString filename="");
	///Checks if variant list is outdated
	void checkVariantList(QStringList messages);
	///Open dialog
	void on_actionOpen_triggered();
	///Open dialog by name (using NGSD)
	void on_actionOpenByName_triggered();
	///ChangeLog action
	void on_actionChangeLog_triggered();
	///About dialog
	void on_actionAbout_triggered();
	///Open processed sample tabs for all samples of the current analysis
	void openProcessedSampleTabsCurrentAnalysis();
	///Open processed sample tab by name
	void on_actionOpenProcessedSampleTabByName_triggered();
	///Open sequencing run tab by name
	void on_actionOpenSequencingRunTabByName_triggered();
    ///Open sequencing gene tab by gene name
    void on_actionOpenGeneTabByName_triggered();
	///Open variant tab by search
	void on_actionOpenVariantTab_triggered();
	///Open processing system by short name
	void on_actionOpenProcessingSystemTab_triggered();
	///Open project tab by search
	void on_actionOpenProjectTab_triggered();

	///NGSD menu
	void on_actionStatistics_triggered();
	void on_actionDevice_triggered();
	void on_actionGenome_triggered();
	void on_actionMID_triggered();
	void on_actionProcessingSystem_triggered();
	void on_actionProject_triggered();
	void on_actionSample_triggered();
	void on_actionSampleGroup_triggered();
	void on_actionSender_triggered();
	void on_actionSpecies_triggered();
	void on_actionUsers_triggered();
	void on_actionImportMids_triggered();
	void on_actionImportStudy_triggered();
	void on_actionImportSamples_triggered();
	void on_actionImportProcessedSamples_triggered();
	void on_actionImportSampleRelations_triggered();
	void on_actionMidClashDetection_triggered();
	void on_actionVariantValidation_triggered();
	void on_actionChangePassword_triggered();
	void on_actionStudy_triggered();
	void on_actionGaps_triggered();

    ///Gender determination
	void on_actionGenderXY_triggered();
	///Gender determination
	void on_actionGenderHet_triggered();
	///Gender determination
	void on_actionGenderSRY_triggered();
	///File information BED
	void on_actionStatisticsBED_triggered();
	///Sample correlation BAM
	void on_actionSampleSimilarityBAM_triggered();
	///Sample correlation VCF
	void on_actionSampleSimilarityVCF_triggered();
	///Sample correlation GSvar
	void on_actionSampleSimilarityGSvar_triggered();
	///Sample ancestry
	void on_actionSampleAncestry_triggered();
	///Sample analysis status
	void on_actionAnalysisStatus_triggered();
	///Lookup gaps in low-coverage BED file
	void on_actionGapsLookup_triggered();
	///Calculate gaps based on current target region
	void on_actionGapsRecalculate_triggered();
	///VCF export
	void exportVCF();
	///GSvar export
	void exportGSvar();
	///Preferred transcript list
	void on_actionPreferredTranscripts_triggered();
	///Somatic gene roles
	void on_actionEditSomaticGeneRoles_triggered();
	///Opens online documentation
	void on_actionOpenDocumentation_triggered();
	///Approved symbols dialog
	void on_actionConvertHgnc_triggered();
	///HPO to regions conversion dialog
	void on_actionPhenoToGenes_triggered();
	///Genes to regions conversion dialog
	void on_actionGenesToRegions_triggered();
	///Subpanel archive dialog
	void on_actionArchiveSubpanel_triggered();
	///Close current variant list
	void on_actionClose_triggered();
	///Close all meta data tabs
	void on_actionCloseMetaDataTabs_triggered();
	///Clear IGV
	void on_actionIgvClear_triggered();
	///Override IGV prot
	void on_actionIgvPort_triggered();
	///Open IGV documentation in browser
	void on_actionIgvDocumentation_triggered();
	///Open CNV dialog
	void on_actionCNV_triggered();
	///Open ROH dialog
	void on_actionROH_triggered();
	///Open SV dialog
	void on_actionSV_triggered();
	///Open gene picker dialog
	void on_actionGeneSelector_triggered();
	///Open Circos plot
	void on_actionCircos_triggered();
	///Open RE dialog
	void on_actionRE_triggered();
	///Open PRS dialog
	void on_actionPRS_triggered();
	///Open cfDNA panel design dialog
	void on_actionDesignCfDNAPanel_triggered();
	/// Open the generated cfDNA BED file
	void on_actionShowCfDNAPanel_triggered();
	///Open disease course dialog (cfDNA)
	void on_actionCfDNADiseaseCourse_triggered();
	///Open expression data Widget
	void on_actionExpressionData_triggered();
	///Open gene OMIM info dialog.
	void on_actionGeneOmimInfo_triggered();
	///Open folder of variant list in explorer.
	void openVariantListFolder();
	///Upload variant that is not part of the variant list to LOVD.
	void on_actionPublishVariantInLOVD_triggered();
	///Batch export for ClinVar
	void on_actionBatchExportClinVar_triggered();
	///Re-analyze current sample/case
	void on_actionReanalyze_triggered();
	///Action for debugging
	void on_actionDebug_triggered();
	///Action for variant conversion (VCF > GSvar)
	void on_actionConvertVcfToGSvar_triggered();
	///Action for variant conversion (HGVS.c > GSvar)
	void on_actionConvertHgvsToGSvar_triggered();
	///Action for variant conversion (GSvar > VCF)
	void on_actionConvertGSvarToVcf_triggered();
	///Action for region conversion (Cytoband > BED)
	void on_actionCytobandsToRegions_triggered();
	///Open SNV search dialog
	void on_actionSearchSNVs_triggered();
	///Open CNV search dialog
	void on_actionSearchCNVs_triggered();
	///Open SV search dialog
	void on_actionSearchSVs_triggered();
	///Shows published variants dialog
	void on_actionShowPublishedVariants_triggered();
	///Shows allele ballance calculation
	void on_actionAlleleBalance_triggered();

	///Load report configuration
	void loadReportConfig();
	///Store report configuration
	void storeReportConfig();
	///Load somatic report configuration
	void loadSomaticReportConfig();
	///Store somatic report configuration
	void storeSomaticReportConfig();
	///Prints a variant sheet based on the report configuration
	void generateEvaluationSheet();
	///Trigger somatic data transfer to MTB
	void transferSomaticData();
	///Shows information about the report config
	void showReportConfigInfo();
	///Finalize report configuration
	void finalizeReportConfig();
	///Generate report
	void generateReport();
	///Generates a report (somatic pair) in .rtf format
	void generateReportSomaticRTF();
	///Generates a report (tumor only!) in .rtf format
	void generateReportTumorOnly();
	///Generates a report (germline)
	void generateReportGermline();
	///Finished the report generation (germline)
	void reportGenerationFinished(bool success);

	///Shows the variant list context menu
	void varsContextMenu(QPoint pos);
	///Shows the variant header context menu
	void varHeaderContextMenu(QPoint pos);
	///Updated the variant context menu
	void updateVariantDetails();
	///Updates the variant table once the variant list changed
	void refreshVariantTable(bool keep_widths = true);
	///Opens the recent file defined by the sender action text
	void openRecentFile();
	///Loads the command line input file.
	void delayedInitialization();
	///A variant has been double-clicked > open in IGV
	void variantCellDoubleClicked(int row, int col);
	///A variant header has beed double-clicked > edit report config
	void variantHeaderDoubleClicked(int row);
	///Initializes IGV for current samples. Returns if the initialization was successfull.
	bool initializeIGV(QAbstractSocket& socket);
	///Open region in IGV
	void openInIGV(QString region);
	///Opens a custom track in IGV
	void openCustomIgvTrack();

	///Edit validation status of current variant
	void editVariantValidation(int index);
	///Edit comment of current variant
	void editVariantComment(int index);
	///Show allele frequency histogram (all variants)
	void showAfHistogram_all();
	///Show allele frequency histogram (after filter)
	void showAfHistogram_filtered();
	///Shows a CN histogram
	void showCnHistogram();
	///Shows an allele frequency histogram
	void showAfHistogram(bool filtered);
	///Show encryption helper
	void on_actionEncrypt_triggered();
	///reannotates VICC data from NGSD to list and stores updated list to variants file
	void on_actionAnnotateSomaticVariantInterpretation_triggered();
	///Show sample search dialog
	void on_actionSampleSearch_triggered();
	///Show run overview
	void on_actionRunOverview_triggered();

	///Subpanel design dialog
	void openSubpanelDesignDialog(const GeneSet& genes = GeneSet());

	///Adds and shows a modeless dialog
	void addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize=false);
	///Removes all modeless dialogs that have been closed
	void cleanUpModelessDialogs();

	///Imports phenotype data from NGSD
	void importPhenotypesFromNGSD();
	///Create sub-panel from phenotype
	void createSubPanelFromPhenotypeFilter();

	///Opens a sample based on the processed sample name
	void openProcessedSampleFromNGSD(QString processed_sample_name, bool search_multi=true);
	///Opens a sample based on the sample name
	void openSampleFromNGSD(QString sample_name);

	///Check mendelian error rate of trio.
	void checkMendelianErrorRate(double cutoff_perc=5.0);

	///Open processed sample tab
	void openProcessedSampleTab(QString ps_name);
	///Open run tab
	void openRunTab(QString run_name);
    ///Open gene tab
	void openGeneTab(QString symbol);
	///Open variant tab
	void openVariantTab(Variant variant);
	///Open pocessing system tab
	void openProcessingSystemTab(QString name_short);
	///Open project tab
	void openProjectTab(QString name);
	///Opens a tab and returns its index.
	int openTab(QIcon icon, QString name, QWidget* widget);
	///Closes a tab by index
	void closeTab(int index);

	///Sends commands to IGV through the socket. Returns if the commands executed successfully.
	void executeIGVCommands(QStringList commands, bool init_if_not_done=false);

	///Edits the variant configuration for the variant with the given index
	void editVariantReportConfiguration(int index);
	///Updates the variant table icon for the variant with the given index
	void updateReportConfigHeaderIcon(int index);

	///Mark the current variant list as changed. It is stored when the sample is closed.
	void markVariantListChanged(const Variant& variant, QString column, QString text);
	///Store the current variant list. Do not call direclty - use markVariantListChanged instead!
	void storeCurrentVariantList();

	///Check for variant validations that need action.
	void checkPendingVariantValidations();
	///Shows a notification.
	void showNotification(QString text);

	///Rank variants by GSvar score.
	void variantRanking();

	///Clears somatic report settings
	void clearSomaticReportSettings(QString ps_id_in_other_widget);


	///Edit somatic variant interpretation (VICC consortium)
	void editSomaticVariantInterpretation(const VariantList& vl, int index);
	///Updates somatic variant interpreation annotation for specific variant of GSvar file
	void updateSomaticVariantInterpretationAnno(int index, QString vicc_interpretation, QString vicc_comment);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* e);
	virtual void dropEvent(QDropEvent* e);
	void closeEvent(QCloseEvent* event);
	///Determines normal sample name from filename_, return "" otherwise (tumor-normal pairs)
	QString normalSampleName();

private:
	//GUI
	Ui::MainWindow ui_;
	int var_last_;
	BusyDialog* busy_dialog_;
	QList<QSharedPointer<QDialog>> modeless_dialogs_;
	QLabel* notification_label_;

	//DATA
	QString filename_;
	bool igv_initialized_;
	VariantList variants_;
	struct VariantListChange
	{
		Variant variant;
		QString column;
		QString text;
	};
	QList<VariantListChange> variants_changed_;
	CnvList cnvs_;
	BedpeFile svs_;
	FilterResult filter_result_;
	QString last_report_path_;
	PhenotypeList last_phenos_;
	BedFile last_phenos_roi_;
    QHash<QByteArray, BedFile> gene2region_cache_;
	ReportSettings report_settings_;
	QString germline_report_ps_;
	SomaticReportSettings somatic_report_settings_;
	VariantList somatic_control_tissue_variants_;

	bool cf_dna_available;
	QToolButton* cfdna_menu_btn_;
	int igv_port_manual = -1;

	//SPECIAL
	DelayedInitializationTimer init_timer_;
};

#endif // MAINWINDOW_H
