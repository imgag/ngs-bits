#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "ui_MainWindow.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "FileWatcher.h"
#include "BusyDialog.h"
#include "FilterCascade.h"

struct IgvFile
{
	QString id; //sample identifier/name (for visualization)
	QString type; //file type (for grouping)
	QString filename; //file name
};

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);
	///Loads a variant list
	void loadFile(QString filename);
	///Returns the result of applying filters to the variant list
	void applyFilters(bool debug_time);
	///Returns the LOG files corresponding to the variant list.
	QStringList getLogFiles();
	///Returns the BAM files for the analysis.
	QList<IgvFile> getBamFiles();
	///Returns CNV SEG files for the analysis.
	QList<IgvFile> getSegFilesCnv();
	///Returns BAF SEG files for the analysis.
	QList<IgvFile> getIgvFilesBaf();
	///Adds a file to the recent file list
	void addToRecentFiles(QString filename);
	///Updates recent files menu
	void updateRecentFilesMenu();
	///Updates IGV menu
	void updateIGVMenu();
	///Loads preferred transcripts from settings.ini
	QMap<QString, QStringList> loadPreferredTranscripts() const;
	///Updates menu and toolbar according to NGSD-support
	void updateNGSDSupport();
	///Returns 'nobr' paragraph start for Qt tooltips
	static QString nobr();
	///Upload variant to LOVD
	void uploadtoLovd(int variant_index, int variant_index2 = -1);
	///Returns the target file name without extension and date part prefixed with '_', or an empty string if no target file is set
	QString targetFileName() const;
	///Returns the processed sample name (in case of a somatic variant list, the tumor is returned).
	QString processedSampleName();
	///Returns the sample name (in case of a somatic variant list, the tumor is returned).
	QString sampleName();
	///Returns all filters defined in the filters INI file
	QStringList loadFilterNames() const;
	///Returns all filters defined in the filters INI file
	FilterCascade loadFilter(QString name) const;
	///Gets a processed sample name from the user - or "" if cancelled.
	QString processedSampleUserInput();

	///Context menu for single variant
	void contextMenuSingleVariant(QPoint pos, int index);
	///Context menu for two variants
	void contextMenuTwoVariants(QPoint pos, int index1, int index2);

public slots:
	///Open dialog
	void on_actionOpen_triggered();
	///Open dialog by name (using NGSD)
	void on_actionOpenByName_triggered();
	///ChangeLog action
	void on_actionChangeLog_triggered();
	///About dialog
	void on_actionAbout_triggered();
	///Report dialog
	void on_actionReport_triggered();
	///Open processed sample tabs
	void on_actionOpenProcessedSampleTabs_triggered();
	///Open processed sample tab by name
	void on_actionOpenProcessedSampleTabByName_triggered();
	///Open sequencing run tab by name
	void on_actionOpenSequencingRunTabByName_triggered();
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
	///Sample correlation TSV
	void on_actionSampleSimilarityTSV_triggered();
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
	///Opens online documentation
	void on_actionOpenDocumentation_triggered();
	///Approved symbols dialog
	void on_actionConvertHgnc_triggered();
	///Germline gene information dialog
	void on_actionGeneInfo_triggered();
	///HPO to regions conversion dialog
	void on_actionPhenoToGenes_triggered();
	///Genes to regions conversion dialog
	void on_actionGenesToRegions_triggered();
	///Subpanel archive dialog
	void on_actionArchiveSubpanel_triggered();
	///Close current file
	void on_actionClose_triggered();
	///Force IGV initializazion
	void on_actionIgvInit_triggered();
	///Clear IGV
	void on_actionIgvClear_triggered();
	///Open CNV dialog
	void on_actionCNV_triggered();
	///Open ROH dialog
	void on_actionROH_triggered();
	void on_actionSV_triggered();
	///Open gene picker dialog
	void on_actionGeneSelector_triggered();
	///Open NGSD annotation dialog.
	void on_actionNGSDAnnotation_triggered();
	///Open gene variant info check dialog.
	void on_actionGeneVariantInfo_triggered();
	///Open folder of variant list in explorer.
	void openVariantListFolder();
	///Upload variant that is not part of the variant list to LOVD.
	void on_actionPublishVariantInLOVD_triggered();
	///Show diagnostic status overview
	void on_actionDiagnosticStatusOverview_triggered();
	///Re-analyze current sample/case
	void on_actionReanalyze_triggered();
	///Annotate germline file with somatic variants
	void on_actionAnnotateSomaticVariants_triggered();

	///Generates a report (somatic) in .rtf format
	void generateReportSomaticRTF();
	///Generates a report (germline)
	void generateReport();
	///Finished the report generation (germline)
	void reportGenerationFinished(bool success);

	///Finished NGSD annotation
	void databaseAnnotationFinished(bool success);
	///Shows the variant list contect menu
	void varsContextMenu(QPoint pos);
	///Updated the variant context menu
	void updateVariantDetails();
	///Apply filter from filter menu
	void applyFilter(QAction* action);
	///Updates the variant table once the variant list changed
	void refreshVariantTable(bool keep_widths = true);
	///Resets the annotation status
	void resetAnnotationStatus();
	///Opens the recent file defined by the sender action text
	void openRecentFile();
	///Loads the command line input file.
	void delayedInizialization();
	///Handles the re-loading the variant list when the file changes.
	void handleInputFileChange();
	///A variant has been double-clicked > open in IGV
	void variantCellDoubleClicked(int row, int col);
	///Open region in IGV
	void openInIGV(QString region);
	///Edit classification of current variant
	void editVariantClassification();
	///Edit validation status of current variant
	void editVariantValidation();
	///Edit comment of current variant
	void editVariantComment();
	///Shows a sample overview for the current variant;
	void showVariantSampleOverview();
	///Show allele frequency histogram
	void showAfHistogram();
	///Show encryption helper
	void on_actionEncrypt_triggered();

	///Returns a gene list where the inheritance information is not set (selected variants only)
	QStringList geneInheritanceMissing(QBitArray selected);

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

	///Opens a sample based on the sample name
	void openProcessedSampleFromNGSD(QString processed_sample_name);

	///Check mendelian error rate of trio.
	void checkMendelianErrorRate(double cutoff_perc=5.0);

	///Open processed sample tab
	void openProcessedSampleTab(QString ps_name);
	///Open run tab
	void openRunTab(QString run_name);
	///Process a tab close request
	void closeTab(int index);

	///Sends commands to IGV through the default socket. Returns if the commands executed successfully.
	bool executeIGVCommands(QStringList commands);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* e);
	virtual void dropEvent(QDropEvent* e);

private:
	//GUI
	Ui::MainWindow ui_;
	int var_last_;
	BusyDialog* busy_dialog_;

	//DATA
	QString filename_;
	FileWatcher filewatcher_;
	enum {YES, NO, ROI} db_annos_updated_;
	bool igv_initialized_;
	VariantList variants_;
	FilterResult filter_result_;
	QMap<QString, QString> link_columns_;
	QSet<int> link_indices_;
	QString last_roi_filename_;
	BedFile last_roi_;
	QString last_report_path_;
	QList<Phenotype> last_phenos_;
	BedFile last_phenos_roi_;
	QMap<QString, QStringList> preferred_transcripts_;
	QList<QSharedPointer<QDialog>> modeless_dialogs_;
	GeneSet imprinting_genes_;

	//SPECIAL
	///Timer to delay some initialization, e.g. load CLI argument after the main window is visible
	QTimer delayed_init_timer_;
};

#endif // MAINWINDOW_H
