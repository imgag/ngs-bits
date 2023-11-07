#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractSocket>
#include "ui_MainWindow.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "BusyDialog.h"
#include "IgvDialog.h"
#include "FilterCascade.h"
#include "ReportSettings.h"
#include "DelayedInitializationTimer.h"
#include "SomaticReportSettings.h"
#include "FileLocationProviderLocal.h"
#include "FileLocationProviderRemote.h"
#include "VersatileTextStream.h"
#include "Log.h"
#include "ClickableLabel.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);

	///Returns the application name
	QString appName() const;

	/// Gets server API information to make sure ther the server is currently running
	bool isServerRunning();

	///Returns the result of applying filters to the variant list
	void applyFilters(bool debug_time);
	///Returns the LOG files corresponding to the variant list.
	QStringList getLogFiles();	
	///Adds a file to the recent processed sample list
	void addToRecentSamples(QString ps);
	///Updates recent processed samples menu
	void updateRecentSampleMenu();
	///Updates IGV menu
    void updateIGVMenu();
	///Enabled/disables actions/buttons depending if NGSD is enabled or disabled.
	void updateNGSDSupport();
	///Returns 'nobr' paragraph start for Qt tooltips
	static QString nobr();

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
	///Lets the user select a processed sample from the current variant list. If only one processed sample is contained, it is returned. If the user aborts, "" is returned.
	QString selectProcessedSample();
	///Target region information of filter widget.
	const TargetRegionInfo& targetRegion();

	///Performs batch import of table rows
	void importBatch(QString title, QString text, QString table, QStringList fields);

	///Returns the variant lists of the currently loaded sample
	const VariantList& getSmallVariantList();
	const CnvList& getCnvList();
	const BedpeFile& getSvList();

	/// Checks if there is a new client version available
	void checkClientUpdates();
    QString getCurrentFileName();
    AnalysisType getCurrentAnalysisType();

public slots:
	///Upload variant to Clinvar
	void uploadToClinvar(int variant_index1, int variant_index2=-1);
	/// Checks (only in clinet-server mode) if the server is currently running
	void checkServerAvailability();
	/// Checks (only in clinet-server mode) if there is some new information needed to be displayed to the user (e.g. downtimes, maintenance, reboots, updates)
	void checkUserNotifications();
	///Loads a variant list. Unloads the variant list if no file name is given
	void loadFile(QString filename="", bool show_only_error_issues=false);
	///Checks if variant list is outdated
	void checkVariantList(QList<QPair<Log::LogLevel, QString>>& issues);
	///Checks if processed samples have bad quality or other problems
	void checkProcessedSamplesInNGSD(QList<QPair<Log::LogLevel, QString>>& issues);
	///Shows a dialog with issues in analysis. Returns the DialogCode.
	int showAnalysisIssues(QList<QPair<Log::LogLevel, QString> >& issues, bool show_only_error_issues);
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
	void on_actionExportTestData_triggered();
	void on_actionImportSequencingRuns_triggered();
	void on_actionImportTestData_triggered();
	void on_actionImportMids_triggered();
	void on_actionImportStudy_triggered();
	void on_actionImportSamples_triggered();
	void on_actionImportProcessedSamples_triggered();
	void on_actionImportSampleRelations_triggered();
	void on_actionImportSampleHpoTerms_triggered();
	void on_actionImportCfDNAPanels_triggered();
	void on_actionMidClashDetection_triggered();
	void on_actionVariantValidation_triggered();
	void on_actionChangePassword_triggered();
	void on_actionStudy_triggered();
	void on_actionGaps_triggered();
	void on_actionReplicateNGSD_triggered();
	void on_actionPrepareGhgaUpload_triggered();
	void on_actionCohortAnalysis_triggered();
	void on_actionMaintenance_triggered();
	void on_actionNotifyUsers_triggered();

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
	///Somatic pathways
	void on_actionEditSomaticPathways_triggered();
	///Somatic pathways-gene association
	void on_actionEditSomaticPathwayGeneAssociations_triggered();
	///Opens online documentation
	void on_actionOpenDocumentation_triggered();
	///Approved symbols dialog
	void on_actionConvertHgnc_triggered();
	///HPO to regions conversion dialog
	void on_actionPhenoToGenes_triggered();
	///Genes to regions conversion dialog
	void on_actionGenesToRegions_triggered();
	///Subpanel archive dialog
	void on_actionManageSubpanels_triggered();
	///Close current variant list
	void on_actionClose_triggered();
	///Close all meta data tabs
	void on_actionCloseMetaDataTabs_triggered();
	///Clear IGV
	void on_actionIgvClear_triggered();
	///Open IGV documentation in browser
	void on_actionIgvDocumentation_triggered();
	///Delete IGV folder
	void on_actionDeleteIgvFolder_triggered();
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
	/// Open dialog to add excluded regions
	void on_actionCfDNAAddExcludedRegions_triggered();
	///Open expression data widget
	void on_actionExpressionData_triggered();
	///Open exon expression data widget
	void on_actionExonExpressionData_triggered();
	///Open RNA splicing widget
	void on_actionShowSplicing_triggered();
	///Open RNA fusion widget
	void on_actionShowRnaFusions_triggered();
	///Open expression stats of processing systems
	void on_actionShowProcessingSystemCoverage_triggered();
	///Open gene OMIM info dialog.
	void on_actionGeneOmimInfo_triggered();
	///Open folder of variant list in explorer.
	void openVariantListFolder();
	///Open variant list qcML files.
	void openVariantListQcFiles();
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
	///Action for region conversion (region > genes)
	void on_actionRegionToGenes_triggered();
	///Open SNV search dialog
	void on_actionSearchSNVs_triggered();
	///Open CNV search dialog
	void on_actionSearchCNVs_triggered();
	///Open SV search dialog
	void on_actionSearchSVs_triggered();
	///Open the manual upload of variants to ClinVar
	void on_actionUploadVariantToClinVar_triggered();
	///Shows published variants dialog
	void on_actionShowPublishedVariants_triggered();
	///Shows allele balance calculation
	void on_actionAlleleBalance_triggered();
	///Shows lift-over dialog
	void on_actionLiftOver_triggered();
	///Get reference sequence
	void on_actionGetGenomicSequence_triggered();
	///Perform BLAT search
	void on_actionBlatSearch_triggered();
	///Opens a virus table based on a corresponding TSV file
	void on_actionVirusDetection_triggered();
	///Perform Burden test
	void on_actionBurdenTest_triggered();
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
	///Add/edit other causal Variant
	void editOtherCausalVariant();
	///Delete other causal Variant
	void deleteOtherCausalVariant();
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


	///Shows the variant header context menu
	void varHeaderContextMenu(QPoint pos);
	///Updated the variant context menu
	void updateVariantDetails();
	///Updates the variant table once the variant list changed
	void refreshVariantTable(bool keep_widths = true);
	///Opens the recent processed sample defined by the sender action text
	void openRecentSample();
	///Loads the command line input file.
	void delayedInitialization();
	///A variant has been double-clicked > open in IGV
	void variantCellDoubleClicked(int row, int col);
	///A variant header has beed double-clicked > edit report config
	void variantHeaderDoubleClicked(int row);
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
	///Shows a BAF histogram
	void showBafHistogram();
	///Shows an allele frequency histogram
	void showAfHistogram(bool filtered);
	///Show encryption helper
	void on_actionEncrypt_triggered();
	///Show settings dialog
	void on_actionSettings_triggered();
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
	///Adds and shows a modeless dialog
	void deleteClosedModelessDialogs();
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
	void openProcessingSystemTab(QString system_name);
	///Open project tab
	void openProjectTab(QString name);
	///Opens a tab and returns its index.
	int openTab(QIcon icon, QString name, QWidget* widget);
	///Closes a tab by index
	void closeTab(int index);

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
	///Execute custom context menu actions (see also registerCustomContextMenuActions())
	void execContextMenuAction(QAction* action, int index);
	//Open Alamut visualization
	void openAlamut(QAction* action);
	//Show matching CNVs and SVs
	void showMatchingCnvsAndSvs(BedLine region);

    //Open a window with the IGV command history
    void displayIgvHistoryTable(QPoint pos);
    //Show in the status bar that IGV is currently executing a command
    void changeIgvIconToActive();
    //Show in the status bar that IGV is idle
    void changeIgvIconToNormal();

    ///close the app and logout (if in client-sever mode)
    void closeAndLogout();

protected:
	virtual void dragEnterEvent(QDragEnterEvent* e);
	virtual void dropEvent(QDropEvent* e);
	void closeEvent(QCloseEvent* event);
	///Determines normal sample name from filename_, return "" otherwise (tumor-normal pairs)
	QString normalSampleName();
	///	Wrapper function for QInputDialog::getItem to handle long URLs:
	/// the list visible to the user will contain only file names (not entire URLs). It makes the
	/// list easier to read and saves some screen real estate
	QString getFileSelectionItem(QString window_title, QString label_text, QStringList file_list, bool *ok);
    /// Removes a user's session on the server (in client-server mode)
    void performLogout();

private:
	//GUI
	Ui::MainWindow ui_;
	int var_last_;
	BusyDialog* busy_dialog_;
	QList<QSharedPointer<QDialog>> modeless_dialogs_;
	QLabel* notification_label_;
    ClickableLabel* igv_history_label_;

	//DATA
	QString filename_;
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
	PhenotypeList last_phenos_; //phenotypes used to generate phenotype ROI (needed to check if they changed)
	PhenotypeSettings last_pheno_settings_; //phenotype settings used to generate phenotype ROI (needed to check if they changed)
	BedFile phenotype_roi_;
	QHash<QByteArray, BedFile> gene2region_cache_;
	ReportSettings report_settings_;
	QString germline_report_ps_;
	SomaticReportSettings somatic_report_settings_;
	VariantList somatic_control_tissue_variants_;

	bool cf_dna_available;
	QToolButton* rna_menu_btn_;
	QToolButton* cfdna_menu_btn_;
	int igv_port_manual = -1;
	QToolBar *update_info_toolbar_;

	//single vars context menu
	struct ContextMenuActions
	{
		QAction* a_report_edit;
		QAction* a_report_del;
		QAction* a_var_class;
		QAction* a_var_class_somatic;
		QAction* a_var_interpretation_somatic;
		QAction* a_var_comment;
		QAction* a_var_val;
		QAction* seperator;
	};
	ContextMenuActions context_menu_actions_;
	void registerCustomContextMenuActions();

	//SPECIAL
	DelayedInitializationTimer init_timer_;
	QString displayed_maintenance_message_id_;

    //current server version (if in client-server mode)
    QString server_version_;

};

#endif // MAINWINDOW_H
